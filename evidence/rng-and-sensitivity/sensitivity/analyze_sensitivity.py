#!/usr/bin/env python3
"""Paired sensitivity estimates and precision; no significance-driven stopping."""
import argparse
import csv
import json
import math
from pathlib import Path
import statistics

T95={2:12.706204736432095,3:4.302652729696142,4:3.182446305284263,5:2.776445105197799,6:2.570581835636314,7:2.4469118511449692,8:2.3646242510102993,9:2.306004135204166,10:2.2621571627409915}
METRICS=('PDR_percent','energy_per_node_day_J','energy_per_delivery_mJ')

def interval(values):
    n=len(values)
    if n<2:return None
    mean=statistics.mean(values);half=T95[n]*statistics.stdev(values)/math.sqrt(n)
    return {'paired_n':n,'mean':mean,'CI95_low':mean-half,'CI95_high':mean+half,'half_width':half}

def effects(block,metric):
    if any(r.get(metric) is None or not math.isfinite(r[metric]) for r in block):return {}
    return {label:math.fsum((2*r['A']-1 if label=='A' else 2*r['G']-1 if label=='G' else (2*r['A']-1)*(2*r['G']-1))*r[metric] for r in block)/(2 if label in ('A','G') else 1) for label in ('A','G','AG')}

def csvout(path,rows):
    if not rows:return
    with path.open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)

def analyze(campaign,output,final=False):
    rows=[]
    for f in sorted(campaign.glob('*/accepted.json')):
        receipt=json.loads(f.read_text())
        if receipt['status']!='PASS':raise ValueError('Unaccepted receipt')
        rows.append(receipt['row'])
    output.mkdir(parents=True,exist_ok=True);csvout(output/'run-results.csv',rows)
    conditions=sorted({r['scenario'] for r in rows});by={(r['scenario'],r['repetition'],r['A'],r['G']):r for r in rows}
    contrasts=[];checks={}
    for scenario in conditions:
        for rep in range(10):
            if not all((scenario,rep,a,g) in by for a in (0,1) for g in (0,1)):continue
            block=[by[scenario,rep,a,g] for a in (0,1) for g in (0,1)]
            checks[f'{scenario}-r{rep}']=len({r['fingerprint_sha256'] for r in block})==1
            for metric in METRICS:
                for effect,value in effects(block,metric).items():contrasts.append({'scenario':scenario,'repetition':rep,'effect':effect,'metric':metric,'contrast':value})
    csvout(output/'paired-seed-contrasts.csv',contrasts)
    estimates=[];changes=[]
    for scenario in conditions:
        for effect in ('A','G','AG'):
            for metric in METRICS:
                subset=[r for r in contrasts if r['scenario']==scenario and r['effect']==effect and r['metric']==metric]
                ci=interval([r['contrast'] for r in subset])
                if ci:
                    estimates.append({'scenario':scenario,'effect':effect,'metric':metric,**ci,'target_1pp_met':metric=='PDR_percent' and effect=='G' and ci['half_width']<=1.0,'interval_scope':'pointwise paired Student-t; no multiplicity adjustment'})
                if scenario!='baseline':
                    base={r['repetition']:r['contrast'] for r in contrasts if r['scenario']=='baseline' and r['effect']==effect and r['metric']==metric}
                    ci=interval([r['contrast']-base[r['repetition']] for r in subset if r['repetition'] in base])
                    if ci:changes.append({'scenario':scenario,'effect':effect,'metric':metric,**ci,'comparison':'scenario effect minus baseline effect, within seed'})
    csvout(output/'sensitivity-effects-ci95.csv',estimates);csvout(output/'paired-change-from-baseline-ci95.csv',changes)
    complete=len(rows)==320 and len(checks)==80 and all(checks.values())
    status={'status':'PASS' if complete else 'INCOMPLETE','accepted_analyzed':len(rows),'target':320,'paired_blocks':len(checks),'paired_checks':checks,'pointwise_effect_intervals':len(estimates),'pointwise_change_intervals':len(changes),'complete_baseline_10':all(('baseline',rep,a,g) in by for rep in range(10) for a in (0,1) for g in (0,1)),'precision_target_pp':1.0,'fixed_n':10,'no_outcome_based_extension':True,'undefined_energy_per_delivery_runs':sum(r['energy_per_delivery_mJ'] is None for r in rows)}
    (output/'status.json').write_text(json.dumps(status,indent=2)+'\n')
    lines=['# Item 6 — sensitivity and precision','',f'Accepted analyzed runs: {len(rows)}/320. The final design fixes 10 repetitions; intervals below are preliminary until all paired blocks are complete.','',
        '| Condition | G effect on PDR (pp) | 95% CI (pp) | Half-width (pp) | Paired n |','|---|---:|---|---:|---:|']
    for r in estimates:
        if r['effect']=='G' and r['metric']=='PDR_percent':lines.append(f"| {r['scenario']} | {r['mean']:.4f} | [{r['CI95_low']:.4f}, {r['CI95_high']:.4f}] | {r['half_width']:.4f} | {r['paired_n']} |")
    if complete:lines[2]='All 320 preregistered cells are accepted and paired, including 20 reused baseline runs and 300 additional native runs. No sample size was chosen from significance.'
    lines += ['','The G effect averages gateway treatment differences across the two airtime conditions with ADR enabled. A averages the airtime effect across G; AG is a difference of differences. Scenario-minus-baseline changes are paired by seed; their own intervals are supplied rather than inferred from overlap between separate intervals.',
        '', 'Energy/node/day uses the actual 12/11/10/9-day measurement window for warm-up 0/1/2/3 days. Energy/delivery and PDR use the same generated-packet cohort/window conventions as the original experiment. Pending MAC packets are not silently dropped from the denominator.',
        '', 'This study retains the item-3 MAC progress behavior and the scheduler header assumption identified by items 2/5. It tests robustness of those implementation-conditional effects, not the performance of the separately corrected MAC candidate. Gamma alternatives are stress bounds at fixed sigma/reference, not empirical calibration. Warm-up sensitivity does not itself prove stationarity.',
        '', 'The prior five-seed SD predicted a 1.335-pp half-width at n=10 and a normal-approximation requirement of 14 for a 1-pp target. The user ceiling is 10; achieved precision is reported even when the target is missed. Pointwise intervals are exploratory, with no multiplicity-controlled discovery claim. No failed run is assigned PDR and undefined energy/delivery remains undefined.']
    (output/'REPORT.md').write_text('\n'.join(lines)+'\n')
    if final and not complete:raise ValueError('Final analysis requires all preregistered paired cells')
    return status

def selfcheck(output):
    block=lambda rep:[{'A':a,'G':g,'x':11+(2+rep/10)*(2*a-1)+3*(2*g-1)+5*(2*a-1)*(2*g-1)} for a in (0,1) for g in (0,1)]
    checks=[]
    for rep in range(10):
        e=effects(block(rep),'x');checks.extend([abs(e['A']-(4+rep/5))<1e-12,abs(e['G']-6)<1e-12,abs(e['AG']-20)<1e-12])
    ci=interval([effects(block(rep),'x')['A'] for rep in range(10)])
    expected_half=2.2621571627409915*.2*math.sqrt(82.5/9)/math.sqrt(10)
    checks.extend([abs(ci['mean']-4.9)<1e-12,abs(ci['half_width']-expected_half)<1e-12])
    output.mkdir(parents=True,exist_ok=False);r={'status':'PASS' if all(checks) else 'FAIL','checks':len(checks),'known_A_mean':4.9,'independent_half_width':expected_half,'observed':ci,'scope':'Synthetic known polynomial and independent sample-variance arithmetic; not simulation data.'};(output/'status.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
    if r['status']!='PASS':raise SystemExit(1)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--campaign',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--final',action='store_true');p.add_argument('--selfcheck',action='store_true');a=p.parse_args()
    if a.selfcheck:selfcheck(a.output)
    else:print(json.dumps(analyze(a.campaign,a.output,a.final),indent=2))
