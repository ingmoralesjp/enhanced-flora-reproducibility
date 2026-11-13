#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Recompute historical cell summaries and seed-matched contrasts, no simulation."""
import argparse
import collections
import csv
import json
from pathlib import Path
import statistics

ROOT = Path(__file__).resolve().parents[1]
T95 = 2.7764451051977987


def write_csv(path, rows):
    with path.open('x', newline='') as stream:
        writer=csv.DictWriter(stream,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)


def mean_interval(values):
    assert len(values)==5
    return statistics.mean(values),T95*statistics.stdev(values)/(5**.5)


def calculate():
    with (ROOT/'data/historical/enhanced-runs.csv').open() as stream:
        runs=list(csv.DictReader(stream))
    with (ROOT/'evidence/source-comparison/evidencia/checkpoint-metrics.csv').open() as stream:
        raw=list(csv.DictReader(stream))
    native={(r['population'],int(r['nodes']),r['stage'],int(r['repetition'])):r for r in raw}
    groups=collections.defaultdict(list)
    blocks=collections.defaultdict(dict)
    for row in runs:
        sent=int(row['unique_uplinks_sent']);received=int(row['unique_uplinks_received']);energy=float(row['end_device_energy_mJ']);n=int(row['end_devices'])
        key=(row['population_original'],n,row['stage'],int(row['repetition']))
        base=native[key]
        metrics=dict(pdr_pp=100*received/sent,energy_per_delivery_mJ=energy/received,energy_per_device_J=energy/1000/n,mean_node_airtime_ms=1000*float(base['mean_time_on_air_s']))
        record=dict(population=row['population'],nodes=n,stage=row['stage'],repetition=int(row['repetition']),pair_id=row['pair_id'],seed_set=row['seed'],metrics=metrics)
        groups[(record['population'],n,record['stage'])].append(record)
        blocks[(record['population'],n,record['repetition'])][record['stage']]=record
    assert len(runs)==900 and len(groups)==180 and len(blocks)==150
    summaries=[]
    for (population,n,stage), records in sorted(groups.items()):
        assert len(records)==5
        output=dict(population=population,end_devices=n,stage=stage,repetitions=5)
        for metric in records[0]['metrics']:
            mean,half=mean_interval([r['metrics'][metric] for r in records])
            output[metric+'_mean']=mean;output[metric+'_ci95_half_width']=half
        summaries.append(output)
    contrasts=[]
    for population,n in sorted({(r['population'],r['nodes']) for r in [next(iter(b.values())) for b in blocks.values()]}):
        for stage in range(1,6):
            for kind,old in (('adjacent','F'+str(stage-1)),('cumulative','F0')):
                new='F'+str(stage);changes=collections.defaultdict(list)
                for repetition in range(5):
                    block=blocks[(population,n,repetition)];a=block[old];b=block[new]
                    assert a['pair_id']==b['pair_id'] and a['seed_set']==b['seed_set']
                    for metric in a['metrics']:changes[metric].append(b['metrics'][metric]-a['metrics'][metric])
                    changes['energy_per_delivery_percent'].append(100*(b['metrics']['energy_per_delivery_mJ']/a['metrics']['energy_per_delivery_mJ']-1))
                output=dict(population=population,end_devices=n,contrast=kind,baseline=old,stage=new,pairs=5)
                for metric,values in changes.items():
                    mean,half=mean_interval(values);output['delta_'+metric+'_mean']=mean;output['delta_'+metric+'_ci95_half_width']=half
                contrasts.append(output)
    assert len(summaries)==180 and len(contrasts)==300
    return summaries,contrasts


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    out=args.output.resolve()
    if out.exists():parser.error('Choose a new, nonexistent output directory')
    summaries,contrasts=calculate();out.mkdir(parents=True)
    write_csv(out/'summaries-ci95.csv',summaries);write_csv(out/'paired-contrasts-ci95.csv',contrasts)
    status=dict(status='PASS',runs=900,cells=180,complete_six_stage_blocks=150,contrasts=300,network_simulations=0,scope='Arithmetic regeneration from existing exports; pointwise five-pair intervals, not physical validation')
    (out/'status.json').write_text(json.dumps(status,indent=2)+'\n');print(json.dumps(status))


if __name__=='__main__':main()
