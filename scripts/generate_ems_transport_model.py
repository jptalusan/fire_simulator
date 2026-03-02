"""
Generate EMS transport model statistics for the fire simulator.

This script analyzes the relationship between EMS medic behavior and fire
apparatus timelines to produce stat files used by the simulator's EMS
transport model.

Data sources (both from data/Vanderbilt_Fire/DataForVandy011426/):
  - ems_apparatus.csv:  Per-medic-unit records with scene/hospital timestamps
  - fire_apparatus.csv: Per-apparatus records with dispatch/arrival/clear times

Output files (written to data/ems_stats/):
  - scene_time_model_params.csv:         Medic scene time coupling model parameters
  - scene_time_coupling.csv:             Binned medic scene time vs fire resolution
  - transport_non_transport_stats.csv:   Non-transporting medic clearing behavior
  - transport_multi_medic_dist.csv:      Transport count distribution by medic count

Usage:
    python scripts/generate_ems_transport_model.py
"""

import os
import pandas as pd
import numpy as np

# ============================================================
# Paths
# ============================================================
DATA_DIR = os.path.join(os.path.dirname(__file__), '..', 'data')
RAW_DIR = os.path.join(DATA_DIR, 'Vanderbilt_Fire', 'DataForVandy011426')
OUT_DIR = os.path.join(DATA_DIR, 'ems_stats')

EMS_PATH = os.path.join(RAW_DIR, 'ems_apparatus.csv')
FIRE_PATH = os.path.join(RAW_DIR, 'fire_apparatus.csv')

# Outlier thresholds (seconds)
MAX_SCENE_TIME = 7200       # 2 hours
MAX_FIRE_RESOLUTION = 7200  # 2 hours
MAX_CLEAR_GAP = 7200        # 2 hours

EMS_DATE_COLS = [
    'Incident_PSAP_Call_Date_Time',
    'Incident_Unit_En_Route_Date_Time',
    'Incident_Unit_Arrived_On_Scene_Date_Time',
    'Incident_Unit_Left_Scene_Date_Time',
    'Incident_Patient_Arrived_At_Destination_Date_Time',
    'Incident_Unit_Back_In_Service_Date_Time',
]

FIRE_DATE_COLS = [
    'Apparatus_Resource_Arrival_Date_Time',
    'Apparatus_Resource_Dispatch_Date_Time',
    'Basic_Incident_Alarm_Time',
]


def load_data():
    """Load and parse EMS and fire apparatus CSVs."""
    print(f"Loading EMS data from {EMS_PATH} ...")
    ems = pd.read_csv(EMS_PATH, parse_dates=EMS_DATE_COLS, low_memory=False)
    print(f"  {len(ems)} EMS apparatus records, {ems['Incident_Number'].nunique()} incidents")

    print(f"Loading fire data from {FIRE_PATH} ...")
    fire = pd.read_csv(FIRE_PATH, parse_dates=FIRE_DATE_COLS, low_memory=False)
    print(f"  {len(fire)} fire apparatus records, {fire['Basic_Incident_Number'].nunique()} incidents")

    # Tag transported medics
    ems['transported'] = ems['Destination'].notna()

    # Count medics per incident
    medic_counts = ems.groupby('Incident_Number').size().rename('medic_count')
    ems = ems.merge(medic_counts, on='Incident_Number')

    return ems, fire


def compute_fire_resolution(fire):
    """
    Compute per-incident fire resolution timeline from non-medic apparatus.

    Fire resolution = time from first fire apparatus arrival to last fire
    apparatus clearing. Only non-medic vehicle types are included (engines,
    trucks, rescues, etc.) since medic clear times follow a different pattern.
    """
    fire_non_medic = fire[fire['VehicleType'] != 'Medic'].copy()
    fire_non_medic['clear_time'] = (
        fire_non_medic['Apparatus_Resource_Dispatch_Date_Time']
        + pd.to_timedelta(fire_non_medic['Apparatus_Resource_Dispatch_to_Cleared_in_Seconds'], unit='s')
    )

    fire_stats = fire_non_medic.groupby('Basic_Incident_Number').agg(
        first_fire_arrival=('Apparatus_Resource_Arrival_Date_Time', 'min'),
        last_fire_clear=('clear_time', 'max'),
    ).dropna()

    fire_stats['fire_resolution_sec'] = (
        fire_stats['last_fire_clear'] - fire_stats['first_fire_arrival']
    ).dt.total_seconds()

    return fire_stats


def generate_scene_time_coupling(ems, fire_stats):
    """
    Generate scene time coupling model.

    The medic's on-scene time is coupled with fire apparatus resolution time:
      medic_scene_time ≈ max(baseline, min(fire_resolution, ceiling)) + noise

    Method:
      1. Join transporting EMS records with fire resolution times on incident
      2. Compute medic_scene_time = left_scene - arrived_on_scene
      3. Bin by fire resolution time and compute medic scene time per bin
      4. Fit baseline (floor when fire is short), ceiling (cap when fire is long)
      5. Compute residual noise parameters

    Baseline: median medic scene time when fire resolves in < 5 min
    Ceiling:  median medic scene time when fire takes > 30 min
    """
    print("\n--- Scene Time Coupling Model ---")

    transport = ems[ems['transported']].copy()
    merged = transport.merge(fire_stats, left_on='Incident_Number', right_index=True, how='inner')
    merged = merged.dropna(subset=[
        'Incident_Unit_Arrived_On_Scene_Date_Time',
        'Incident_Unit_Left_Scene_Date_Time',
    ])
    merged['medic_scene_time_sec'] = (
        merged['Incident_Unit_Left_Scene_Date_Time']
        - merged['Incident_Unit_Arrived_On_Scene_Date_Time']
    ).dt.total_seconds()

    valid = merged[
        (merged['medic_scene_time_sec'] > 0)
        & (merged['medic_scene_time_sec'] < MAX_SCENE_TIME)
        & (merged['fire_resolution_sec'] > 0)
        & (merged['fire_resolution_sec'] < MAX_FIRE_RESOLUTION)
    ].copy()

    print(f"  Valid records: {len(valid)}")

    # --- Binned lookup table ---
    bins = [0, 120, 300, 600, 900, 1200, 1800, 3600, 7200]
    labels = ['0-2min', '2-5min', '5-10min', '10-15min', '15-20min', '20-30min', '30-60min', '60-120min']
    valid['fire_res_bucket'] = pd.cut(valid['fire_resolution_sec'], bins=bins, labels=labels)

    binned = valid.groupby('fire_res_bucket', observed=True)['medic_scene_time_sec'].agg(
        ['mean', 'median', 'std', 'count']
    ).reset_index()
    binned.columns = ['fire_resolution_bucket', 'medic_scene_mean', 'medic_scene_median', 'medic_scene_std', 'count']

    out_path = os.path.join(OUT_DIR, 'scene_time_coupling.csv')
    binned.to_csv(out_path, index=False)
    print(f"  Saved: {out_path}")
    print(binned.to_string(index=False))

    # --- Model parameters ---
    baseline = valid.loc[valid['fire_resolution_sec'] < 300, 'medic_scene_time_sec'].median()
    ceiling = valid.loc[valid['fire_resolution_sec'] > 1800, 'medic_scene_time_sec'].median()

    # Residual = actual - clip(fire_resolution, baseline, ceiling)
    valid['predicted'] = valid['fire_resolution_sec'].clip(lower=baseline, upper=ceiling)
    residual_mean = (valid['medic_scene_time_sec'] - valid['predicted']).mean()
    residual_std = (valid['medic_scene_time_sec'] - valid['predicted']).std()

    params = pd.DataFrame([
        {'parameter': 'baseline_sec', 'value': baseline},
        {'parameter': 'ceiling_sec', 'value': ceiling},
        {'parameter': 'residual_mean_sec', 'value': residual_mean},
        {'parameter': 'residual_std_sec', 'value': residual_std},
    ])

    out_path = os.path.join(OUT_DIR, 'scene_time_model_params.csv')
    params.to_csv(out_path, index=False)
    print(f"  Saved: {out_path}")
    print(f"  Baseline: {baseline:.0f}s ({baseline/60:.1f} min)")
    print(f"  Ceiling:  {ceiling:.0f}s ({ceiling/60:.1f} min)")
    print(f"  Residual: mean={residual_mean:.0f}s, std={residual_std:.0f}s")


def generate_non_transport_stats(ems, fire_stats):
    """
    Generate non-transporting medic behavior stats.

    Non-transporting medics (no hospital destination) clear at approximately
    the same time as fire apparatus. This computes:
      - Scene time (arrived -> back in service)
      - Gap from fire clear (back_in_service - last_fire_clear)

    Method:
      1. Filter EMS records where transported=False
      2. Join with fire resolution times
      3. Compute scene time and gap from fire clear
      4. Output summary statistics
    """
    print("\n--- Non-Transporting Medic Stats ---")

    no_transport = ems[~ems['transported']].copy()
    merged = no_transport.merge(fire_stats, left_on='Incident_Number', right_index=True, how='inner')
    valid = merged.dropna(subset=[
        'Incident_Unit_Back_In_Service_Date_Time',
        'last_fire_clear',
        'Incident_Unit_Arrived_On_Scene_Date_Time',
    ]).copy()

    valid['scene_time_sec'] = (
        valid['Incident_Unit_Back_In_Service_Date_Time']
        - valid['Incident_Unit_Arrived_On_Scene_Date_Time']
    ).dt.total_seconds()

    valid['gap_from_fire_clear_sec'] = (
        valid['Incident_Unit_Back_In_Service_Date_Time']
        - valid['last_fire_clear']
    ).dt.total_seconds()

    valid = valid[
        (valid['scene_time_sec'] > 0)
        & (valid['scene_time_sec'] < MAX_SCENE_TIME)
        & (valid['gap_from_fire_clear_sec'].abs() < MAX_CLEAR_GAP)
    ]

    print(f"  Valid records: {len(valid)}")

    stats = pd.DataFrame([
        {'metric': 'scene_time_mean_sec', 'value': valid['scene_time_sec'].mean()},
        {'metric': 'scene_time_median_sec', 'value': valid['scene_time_sec'].median()},
        {'metric': 'scene_time_std_sec', 'value': valid['scene_time_sec'].std()},
        {'metric': 'gap_from_fire_clear_mean_sec', 'value': valid['gap_from_fire_clear_sec'].mean()},
        {'metric': 'gap_from_fire_clear_median_sec', 'value': valid['gap_from_fire_clear_sec'].median()},
        {'metric': 'gap_from_fire_clear_std_sec', 'value': valid['gap_from_fire_clear_sec'].std()},
    ])

    out_path = os.path.join(OUT_DIR, 'transport_non_transport_stats.csv')
    stats.to_csv(out_path, index=False)
    print(f"  Saved: {out_path}")
    print(f"  Scene time:       median={valid['scene_time_sec'].median()/60:.1f} min")
    print(f"  Gap from fire:    median={valid['gap_from_fire_clear_sec'].median()/60:.1f} min")


def generate_multi_medic_dist(ems):
    """
    Generate multi-medic transport count distribution.

    For incidents with N medics dispatched, this computes the probability
    distribution of how many medics actually transport to hospital.

    Method:
      1. Group EMS records by incident
      2. Count total medics and transported medics per incident
      3. For each medic count (1-5), compute P(transport_count = k)
    """
    print("\n--- Multi-Medic Transport Distribution ---")

    transport_counts = ems.groupby('Incident_Number').agg(
        medic_count=('medic_count', 'first'),
        transport_count=('transported', 'sum'),
    )

    rows = []
    for mc in range(1, 6):
        sub = transport_counts[transport_counts['medic_count'] == mc]
        total = len(sub)
        if total == 0:
            continue
        for tc in range(mc + 1):
            count = (sub['transport_count'] == tc).sum()
            rows.append({
                'medic_count': mc,
                'transport_count': tc,
                'probability': count / total,
                'sample_count': int(count),
            })

    dist = pd.DataFrame(rows)

    out_path = os.path.join(OUT_DIR, 'transport_multi_medic_dist.csv')
    dist.to_csv(out_path, index=False)
    print(f"  Saved: {out_path}")
    print(dist.to_string(index=False))


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    ems, fire = load_data()
    fire_stats = compute_fire_resolution(fire)

    generate_scene_time_coupling(ems, fire_stats)
    generate_non_transport_stats(ems, fire_stats)
    generate_multi_medic_dist(ems)

    print("\n" + "=" * 60)
    print("Done. Output files in:", OUT_DIR)
    print("=" * 60)


if __name__ == '__main__':
    main()
