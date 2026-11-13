#!/usr/bin/env python3
"""Check unchanged F4/F5 outcomes while the recorded lookup selector changes."""
import argparse
import csv
import hashlib
import json
from pathlib import Path

def main():
    p=argparse.ArgumentParser();p.add_argument('--metrics',type=Path,required=True);p.add_argument('--devices',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():p.error('Use a fresh output file')
    with a.metrics.open() as s:rows=list(csv.DictReader(s))
    index={(r['stage'],r['population'],r['nodes'],r['repetition']):r for r in rows}
    fields=['sent_data','received_data','pdr','measurement_energy_j','measurement_tx_energy_j','measurement_rx_energy_j','measurement_sleep_energy_j']
    pairs=[]
    for (stage,pop,n,rep),r in index.items():
        if stage!='F4':continue
        other=index['F5',pop,n,rep]
        pairs.append({'pair':f'{pop}-n{n}-r{rep}','equal':{k:float(r[k])==float(other[k]) for k in fields},'lookup_codes':[float(r['tx_current_code']),float(other['tx_current_code'])],'energy_delta_J':float(other['measurement_energy_j'])-float(r['measurement_energy_j'])})
    powers=set();devices=0
    with a.devices.open() as s:
        for r in csv.DictReader(s):
            if r['run_id'].startswith(('enhanced-f4-','enhanced-f5-')):
                devices+=1;powers.update((float(r['initial_tx_power_dBm']),float(r['final_tx_power_dBm'])))
    valid=len(pairs)==150 and all(all(r['equal'].values()) and r['lookup_codes']==[0.,1.] for r in pairs)
    out={'status':'PASS' if valid else 'FAIL','pairs':len(pairs),'device_records':devices,'observed_initial_final_powers_dBm':sorted(powers),'sha256':{str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in (a.metrics,a.devices,Path(__file__))},'boundary':'Initial/final states are observed; integer-only ADR code supplies the invariant between them, not a recovered event-by-event history.','checks':pairs}
    a.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items() if k not in ('sha256','checks')},indent=2))
    if not valid:raise SystemExit(1)

if __name__=='__main__':main()
