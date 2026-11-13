#!/usr/bin/env python3
"""Independent current/VI-time oracle for the complete native F5 state path."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
from component_oracle import current

def main():
    p=argparse.ArgumentParser();p.add_argument('directory',type=Path);a=p.parse_args();d=a.directory
    rows=[]
    for r in csv.DictReader((d/'component-observed.csv').open()):
        if r['group']=='F5_native_power':expected=3.3*current(float(r['inputs_semicolon_delimited']))/1000
        elif r['group']=='F5_native_state_energy':
            start,end,warm=map(float,r['inputs_semicolon_delimited'].split(';'));ma={'TX':31,'RX':9.7,'idle':.0001,'sleep':.0001}[r['case']];expected=3.3*ma/1000*max(0,end-max(start,warm))
        else:continue
        error=abs(float(r['observed'])-expected)
        rows.append(dict(r,expected=expected,error=error,tolerance=1e-12,pass_=error<=1e-12))
    with (d/'native-energy-comparison.csv').open('x',newline='') as f:
        w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
    status={'status':'PASS' if len(rows)==77 and all(r['pass_'] for r in rows) else 'FAIL','cases':len(rows),'failures':[r for r in rows if not r['pass_']],'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'model_boundary':'Controlled state input with actual initialized XML parameters, production computePowerBreakdown and integrateEnergyUntil; not a hardware measurement.'}
    (d/'native-energy-status.json').write_text(json.dumps(status,indent=2)+'\n');print(json.dumps(status,indent=2))
    if status['status']!='PASS':raise SystemExit(1)

if __name__=='__main__':main()
