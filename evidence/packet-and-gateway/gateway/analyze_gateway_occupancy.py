#!/usr/bin/env python3
"""Actual RF interval union, admission chain and global scheduling constraints."""
import argparse
import csv
import hashlib
import json
from pathlib import Path

def union(intervals):
    total=0.;last=None
    for begin,end in sorted(intervals):
        if last is None:last=[begin,end]
        elif begin<=last[1]:last[1]=max(last[1],end)
        else:total+=last[1]-last[0];last=[begin,end]
    return total+(last[1]-last[0] if last else 0.)

def analyze(path,begin,end,runid):
    with path.open() as f:events=list(csv.DictReader(f))
    scheduled=[r for r in events if r['event']=='GW_DOWNLINK_SCHEDULED']
    starts=[r for r in events if r['event']=='PHY_TX_START' and '.loRaGW[' in r['component']]
    ends=[r for r in events if r['event']=='PHY_TX_END' and '.loRaGW[' in r['component']]
    by_tree={r['packet_tree']:r for r in scheduled}
    end_keys={(r['packet_tree'],float(r['start_s']),float(r['end_s'])) for r in ends}
    request=[r for r in events if r['event']=='GW_DOWNLINK_REQUEST' and begin<=float(r['time_s'])<end]
    sched_window=[r for r in scheduled if begin<=float(r['time_s'])<end]
    rejected=[r for r in events if r['event']=='GW_DOWNLINK_REJECT_DUTY' and begin<=float(r['time_s'])<end]
    intervals=[];previous=None;min_implemented=float('inf');min_rf=float('inf');violations=0;rf_violations=0;mismatches=0;rows=[]
    checks={'unique_scheduled_identity':len(by_tree)==len(scheduled),'scheduled_equals_RF_start_count':len(scheduled)==len(starts),'request_balance':len(request)==len(sched_window)+len(rejected)}
    for r in sorted(starts,key=lambda x:float(x['start_s'])):
        t,finish=map(float,(r['start_s'],r['end_s']));scheduled_event=by_tree.get(r['packet_tree'])
        if scheduled_event is None:raise ValueError('RF start without gateway admission')
        hold,declared=float(scheduled_event['value1']),float(scheduled_event['value2'])
        duration=finish-t;overlap=max(0,min(end,finish)-max(begin,t))
        if overlap:intervals.append((max(begin,t),min(end,finish)))
        if previous:
            gap=t-previous['start']-previous['holdoff'];actual=t-previous['start']-previous['duration']/.01
            min_implemented=min(min_implemented,gap);min_rf=min(min_rf,actual)
            violations+=gap < -1e-8;rf_violations+=actual < -1e-8
        previous={'start':t,'holdoff':hold,'duration':duration}
        mismatches+=abs(duration-declared)>1e-8
        checks['RF_end_'+r['packet_tree']]=(r['packet_tree'],t,finish) in end_keys or finish>end
        rows.append({'run_id':runid,'packet_tree':r['packet_tree'],'RF_start_s':t,'RF_end_s':finish,'actual_RF_airtime_s':duration,'measured_RF_overlap_s':overlap,'scheduled_airtime_s':declared,'scheduled_holdoff_s':hold,'frequency_Hz':r['frequency_Hz'],'SF':r['SF']})
    actual_union=union(intervals);window=end-begin
    record={'run_id':runid,'window_begin_s':begin,'window_end_s':end,'window_duration_s':window,'configured_budget':.01,'resource_scope':'global_gateway_single_timer','requests':len(request),'scheduled':len(sched_window),'rejected_duty':len(rejected),'request_rate_per_s':len(request)/window,'RF_intervals_starting_in_window':sum(begin<=r['RF_start_s']<end for r in rows),'RF_union_s':actual_union,'RF_occupancy':actual_union/window,'scheduled_ToA_over_holdoff':sum(float(r['value2']) for r in sched_window)/sum(float(r['value1']) for r in sched_window) if sched_window else None,'actual_RF_ToA_over_holdoff':sum(r['actual_RF_airtime_s'] for r in rows if begin<=r['RF_start_s']<end)/sum(float(r['value1']) for r in sched_window) if sched_window else None,'implementation_spacing_violations':violations,'RF_budget_spacing_violations':rf_violations,'minimum_implemented_slack_s':min_implemented if previous and len(starts)>1 else None,'minimum_RF_budget_slack_s':min_rf if len(starts)>1 else None,'all_horizon_scheduled_RF_duration_mismatches':mismatches,'frequencies_Hz':';'.join(sorted({r['frequency_Hz'] for r in rows})),'source_trace_sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    record.update(all_horizon_requests=sum(r['event']=='GW_DOWNLINK_REQUEST' for r in events),all_horizon_scheduled=len(scheduled),all_horizon_rejected=sum(r['event']=='GW_DOWNLINK_REJECT_DUTY' for r in events))
    # RF 1% spacing applies only to the corrected G1 treatment, not G0 legacy.
    checks['implemented_spacing']=violations==0
    return record,rows,checks

def main():
    p=argparse.ArgumentParser();p.add_argument('--campaign',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--kind',choices=['controlled','factorial'],required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    records=[];intervals=[];checks={}
    for folder in sorted(a.campaign.iterdir()):
        if not folder.is_dir():continue
        trace=folder/('events.csv' if a.kind=='controlled' else 'adr-events.csv')
        if not trace.exists():continue
        receipt_path=folder/('receipt.json' if a.kind=='controlled' else 'accepted.json')
        receipt=json.loads(receipt_path.read_text())
        if receipt['status']!='PASS' or hashlib.sha256(trace.read_bytes()).hexdigest()!=receipt['files_sha256'][trace.name]:raise ValueError(f'Unaccepted/changed trace {folder.name}')
        begin,end=(600,3600) if a.kind=='controlled' else (172800,1036800)
        row,detail,case_checks=analyze(trace,begin,end,folder.name)
        # Native GW scalars are full-horizon counters (they do not reset at warm-up).
        scalar={};scalarfile=folder/'run.sca'
        if not scalarfile.exists():
            scalarfile=folder/'gateway-scalars.sca'
            proof=json.loads((folder/'source-sca-provenance.json').read_text())
            if proof['source_run_sca_sha256']!=receipt['files_sha256']['run.sca'] or proof['extracted_gateway_scalars_sha256']!=hashlib.sha256(scalarfile.read_bytes()).hexdigest():raise ValueError('Counter extract provenance mismatch')
        for line in scalarfile.read_text().splitlines():
            fields=line.split()
            if len(fields)==4 and fields[0]=='scalar' and '.loRaGW[0].LoRaGWNic.mac' in fields[1] and fields[2] in ('GW_forwardedDown','GW_droppedDC'):
                scalar[fields[2]]=int(float(fields[3]))
        case_checks['native_full_horizon_admissions']=scalar.get('GW_forwardedDown')==row['all_horizon_scheduled']
        case_checks['native_full_horizon_rejections']=scalar.get('GW_droppedDC')==row['all_horizon_rejected']
        if a.kind=='controlled' or folder.name.startswith(('A0G1','A1G1')):case_checks['corrected_RF_budget_spacing']=row['RF_budget_spacing_violations']==0
        records.append(row);intervals.extend(detail);checks[folder.name]=all(case_checks.values())
    for name,rows in [('gateway-occupancy.csv',records),('RF-intervals.csv',intervals)]:
        with (a.output/name).open('x',newline='') as f:
            if rows:
                w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
    expected=4 if a.kind=='controlled' else 120
    status={'status':'PASS' if len(records)==expected and all(checks.values()) else 'FAIL','runs':len(records),'expected':expected,'checks':checks,'interval_union_self_check':union([(0,2),(1,3),(4,5)])==4,'tolerance_s':1e-8,'definition':'RF occupancy = duration(union of actual RF transmission intervals clipped to measurement window))/window duration. Admission counts use event timestamps; carry-in RF intervals are clipped, not lost.'}
    if not status['interval_union_self_check']:status['status']='FAIL'
    (a.output/'status.json').write_text(json.dumps(status,indent=2)+'\n');print(json.dumps({k:v for k,v in status.items() if k!='checks'},indent=2))
    if status['status']!='PASS':raise SystemExit(1)

if __name__=='__main__':main()
