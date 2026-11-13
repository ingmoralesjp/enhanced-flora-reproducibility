#!/usr/bin/env python3
"""Freeze a bounded sensitivity design before any additional performance runs."""
import csv
import hashlib
import json
import math
from pathlib import Path
import shutil
import statistics

HERE=Path(__file__).resolve().parent
SCENARIOS={
    'baseline':{'mean_interval_s':1000,'gamma':2.32,'sigma_dB':7.08,'warmup_days':2},
    'load_lower':{'mean_interval_s':2000,'gamma':2.32,'sigma_dB':7.08,'warmup_days':2},
    'load_higher':{'mean_interval_s':500,'gamma':2.32,'sigma_dB':7.08,'warmup_days':2},
    'gamma_lower':{'mean_interval_s':1000,'gamma':2.0,'sigma_dB':7.08,'warmup_days':2},
    'gamma_higher':{'mean_interval_s':1000,'gamma':2.7,'sigma_dB':7.08,'warmup_days':2},
    'warmup_0d':{'mean_interval_s':1000,'gamma':2.32,'sigma_dB':7.08,'warmup_days':0},
    'warmup_1d':{'mean_interval_s':1000,'gamma':2.32,'sigma_dB':7.08,'warmup_days':1},
    'warmup_3d':{'mean_interval_s':1000,'gamma':2.32,'sigma_dB':7.08,'warmup_days':3},
}

def main():
    parent=HERE.parent/'observation-3/testbed/flora';out=HERE/'testbed/flora';out.mkdir(parents=True,exist_ok=False)
    shutil.copy2(parent/'Makefile',out/'Makefile')
    shutil.copytree(parent/'src',out/'src',ignore=shutil.ignore_patterns('Makefile','*.so','out','__pycache__'))
    shutil.copytree(parent/'simulations',out/'simulations')
    shutil.copy2(parent/'src/libflora.so',out/'src/libflora.so')
    parent_sources={str(f.relative_to(parent)):hashlib.sha256(f.read_bytes()).hexdigest() for f in parent.rglob('*') if f.is_file() and not f.is_symlink() and f.suffix in ('.cc','.h','.ned','.msg','.xml','.ini')}
    text='include reviewer-factorial.ini\n\n[Config SensitivityBase]\nextends = AuditFactorialBase\nrepeat = 10\n**.numberOfNodes = 500\n'
    jobs=[]
    for name,settings in SCENARIOS.items():
        for a in (0,1):
            for g in (0,1):
                config=f'Sens_{name}_A{a}G{g}'
                text+=f'''\n[Config {config}]
extends = SensitivityBase
warmup-period = {settings['warmup_days']}d
**.timeToFirstPacket = exponential({settings['mean_interval_s']}s, 1)
**.timeToNextPacket = exponential({settings['mean_interval_s']}s, 1)
**.LoRaMedium.pathLoss.gamma = {settings['gamma']}
**.LoRaMedium.pathLoss.sigma = {settings['sigma_dB']}
**.airtimeModel = "{'semtech' if a else 'legacyFixedPayload'}"
**.dutyCycleModel = "{'toa' if g else 'legacySfConstants'}"
**.networkServer.**.evaluateADRinServer = true
**.evaluateADRinNode = true
'''
                for rep in range(10):jobs.append({'run_id':f'{name}-A{a}G{g}-n500-r{rep}','scenario':name,'A':a,'G':g,'D':1,'nodes':500,'repetition':rep,'seed':rep,'native_run':40+rep,'config':config,'settings':settings,'reuse_original':name=='baseline' and rep<5})
    ini=out/'simulations/examples/reviewer-sensitivity.ini';ini.write_text(text)
    # Planning uses only the already-delivered five-repetition baseline contrasts.
    prior=HERE.parent/'observation-3/results/analysis-v1/paired-seed-contrasts.csv'
    with prior.open() as f:prior_rows=list(csv.DictReader(f))
    contrast=[float(r['contrast']) for r in prior_rows if r['nodes']=='500' and r['effect']=='G_given_D1' and r['metric']=='PDR_percent']
    sd=statistics.stdev(contrast);predicted=2.2621571627409915*sd/math.sqrt(10)
    design={'version':'sensitivity-20261030-v1','source_parent':'observation-3 factorial source; same known MAC progress limitation, no silent mixing with item-4 corrected candidate','parent_library_sha256':hashlib.sha256((parent/'src/libflora.so').read_bytes()).hexdigest(),'parent_sources_sha256':parent_sources,'config_sha256':hashlib.sha256(ini.read_bytes()).hexdigest(),'nodes':500,'gateways':1,'repetitions_fixed':10,'horizon_days':12,'conditions':SCENARIOS,'total_cells':320,'reused_accepted_runs':20,'new_native_runs':300,'parallel_jobs_limit':8,'planning':{'primary_contrast':'Gateway G averaged over two A conditions, ADR enabled; airtime A and AG are additional contrasts','useful_target_PDR_half_width_pp':1.0,'prior_n':5,'prior_G_SD_pp':sd,'predicted_half_width_at_10_pp':predicted,'normal_approx_required_n_for_1pp':math.ceil((1.96*sd)**2),'decision':'Fix n=10 before new runs, respect user ceiling even if the 1pp target is not reached. No sequential significance-based stopping; report achieved precision and unresolved signs.'},'jobs':jobs}
    (HERE/'DESIGN.json').write_text(json.dumps(design,indent=2)+'\n')
    (HERE/'DESIGN.md').write_text('''# Item 6 — preregistered bounded sensitivity

This design is frozen before additional performance simulations. It diagnoses
the same implementation used by item 3, including its documented MAC limitation;
it is not a performance evaluation of the separately corrected item-4 candidate.

One gateway, stationary 500-ED deployment, 12 days, ADR enabled. Within every
condition and seed, the four A/G cells are paired for deployment and offered
traffic. Application traffic has its own RNG stream, distinct from deployment,
path loss and network delay. The instrumentation adds no random draws.

Baseline plus half/double offered traffic, gamma 2.0/2.7 versus 2.32 at fixed
sigma=7.08 dB and the same 1-km Friis reference, and warm-up 0/1/3 days versus
2 days. Gamma 2.0 brackets the free-space distance exponent and 2.7 tests a
larger loss slope; these are model-parameter stress bounds, not fitted urban
or suburban calibration claims. Vary one scenario parameter at a time. Initial
and subsequent interpacket means change together in the traffic conditions.

The primary contrast is G with ADR enabled, averaged over A. Also report A and
the AG interaction for PDR, measured energy/node/day and energy/delivery.
Warm-up comparisons use their actual 12/11/10/9-day measurement windows;
energy/node/day is normalized to avoid confusing window length with power.
The generation cohort defines the PDR denominator; queued packets remain pending.

Use the five delivered baseline repetitions to estimate precision; a useful
target is 1 pp PDR half-width. Fix 10 repetitions for every condition **before**
the new runs. The estimate may require more than the authorized 10, in which
case report achieved precision and do not extend or stop on significance.
Reuse the 20 accepted baseline A/G runs at 500 EDs and seeds 0–4; run only
the 300 missing cells with at most 8 simultaneous native processes.

All intervals are pointwise paired Student-t intervals with 9 df. Direction
stability is descriptive unless the interval resolves the sign. Differences
between scenario contrasts are themselves paired by seed. No hardware,
multi-gateway or mobility-causation claim is introduced.
''')
    print(json.dumps({k:design[k] for k in ['total_cells','reused_accepted_runs','new_native_runs','planning']},indent=2))

if __name__=='__main__':main()
