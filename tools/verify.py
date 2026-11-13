#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Integrity, accounting, pairing and optional numerical reanalysis; no simulator."""
import argparse
import collections
import csv
from decimal import Decimal
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for part in iter(lambda:stream.read(1024*1024),b''):h.update(part)
    return h.hexdigest()


def rows(path):
    with path.open() as stream:return list(csv.DictReader(stream))


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--reanalyze-propagation',action='store_true');args=parser.parse_args()
    manifest=json.loads((ROOT/'MANIFEST.json').read_text())
    assert all((ROOT/name).is_file() and sha(ROOT/name)==record['sha256'] and (ROOT/name).stat().st_size==record['bytes'] for name,record in manifest['files'].items()),'File integrity failure'
    subprocess.run([sys.executable, str(ROOT/'tools/check_publication_contents.py')], check=True)
    records=rows(ROOT/'data/historical/enhanced-runs.csv');assert len(records)==900
    by_id={r['run_id']:r for r in records};assert len(by_id)==900
    cells=collections.Counter();blocks=collections.defaultdict(set)
    totals=collections.defaultdict(Decimal);device_rows=0
    for r in records:
        assert r['accepted']=='true' and r['failure_reason']=='' and r['control_policy']=='lorawanReference'
        assert r['simulated_days']=='12' and r['warmup_days']=='2'
        sent=Decimal(r['unique_uplinks_sent']);got=Decimal(r['unique_uplinks_received']);energy=Decimal(r['end_device_energy_mJ'])
        assert sent>0 and 0<got<=sent and energy>0
        assert abs(got/sent-Decimal(r['pdr_recomputed']))<Decimal('1e-12')
        assert abs(energy/got-Decimal(r['energy_per_delivery_mJ_recomputed']))<Decimal('1e-8')
        cells[(r['population'],r['end_devices'],r['stage'])]+=1;blocks[r['pair_id']].add(r['stage'])
    assert len(cells)==180 and set(cells.values())=={5}
    assert len(blocks)==150 and all(stages=={'F0','F1','F2','F3','F4','F5'} for stages in blocks.values())
    with (ROOT/'data/historical/enhanced-device-summary.csv').open() as stream:
        for r in csv.DictReader(stream):totals[r['run_id']]+=Decimal(r['energy_mJ']);device_rows+=1
    assert device_rows==495000 and set(totals)==set(by_id)
    assert all(abs(total-Decimal(by_id[key]['end_device_energy_mJ']))<Decimal('1e-4') for key,total in totals.items())
    official=rows(ROOT/'data/historical/official-flora-1.1-runs.csv');assert len(official)==100
    assert sum(r['accepted']=='true' for r in official)==92
    assert sum(r['accepted']=='false' for r in official)==8
    for r in official:
        if r['accepted']=='false':
            assert r['failure_reason'] and all(not r[key] for key in ('unique_uplinks_sent','unique_uplinks_received','native_end_device_energy_mJ','pdr_recomputed'))
    counters=rows(ROOT/'data/historical/enhanced-downlink-counters.csv');assert len(counters)==900
    assert all(int(r['gateway_forwarded_to_phy'])+int(r['gateway_rejected_duty'])==int(r['gateway_attempts_derived']) for r in counters)
    mechanism=rows(ROOT/'data/mechanisms/enhanced-mechanisms.csv');assert len(mechanism)==900
    assert {r['run_id'] for r in mechanism}==set(by_id)
    for archive in ROOT.glob('archives/*.zip'):
        with zipfile.ZipFile(archive) as z:assert z.testzip() is None
    if args.reanalyze_propagation:
        archive=ROOT/'archives/P3_propagacion_rc11_verificada_20261108_v1-source-only.zip'
        with tempfile.TemporaryDirectory(prefix='enhanced-propagation-check-') as directory:
            dest=Path(directory)
            with zipfile.ZipFile(archive) as z:
                for info in z.infolist():assert (dest/info.filename).resolve().is_relative_to(dest.resolve())
                z.extractall(dest)
            folders=[p for p in dest.iterdir() if p.is_dir()];assert len(folders)==1
            subprocess.run([sys.executable,'verify_package.py','--root','.'],cwd=folders[0],env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1'),check=True)
    print(json.dumps(dict(status='PASS',files_verified=len(manifest['files']),enhanced_runs=900,cells=180,blocks=150,device_rows=device_rows,official_accepted=92,official_failed=8,mechanism_rows=900,network_simulations=0,propagation_reanalysis=args.reanalyze_propagation,interpretation='Integrity/accounting checks, not universal model correctness')))


if __name__=='__main__':main()
