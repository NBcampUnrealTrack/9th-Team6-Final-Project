"""Update existing dodge montages/settings without retargeting or rebuilding profiles."""
import csv
import json
import sys
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0, str(ROOT / 'Tools/Characters'))
from dodge_timing import configure_dodge_blends

saved = set()
for gender, suffix in [('Male', 'M'), ('Female', 'F')]:
    folder = f'/Game/IsekaiClass/Animation/Player/Main{gender}'
    profile = u.load_asset(f'{folder}/DA_ANIM_PLAYER_{suffix}')
    assert profile, folder
    states = [profile.get_editor_property('unarmed'), *profile.get_editor_property('weapon_sets').values()]
    montages = [m for state in states for m in [state.get_editor_property('dodge'), *state.get_editor_property('directional_dodges')] if m]
    # Keep the older forward-only setup usable as well.
    montages.extend(m for pack in ['Sword_Animations', 'Archer', 'Mage']
                    if (m := u.load_asset(f'{folder}/AM_{pack}_combat_dodge')))
    for montage in montages:
        path = montage.get_path_name()
        if path in saved:
            continue
        configure_dodge_blends(montage)
        assert u.EditorAssetLibrary.save_loaded_asset(montage, False), path
        saved.add(path)

table = u.load_asset('/Game/IsekaiClass/Data/DataTable/Settings/DT_IC_CombatSettings')
rows = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
with (ROOT / 'Data/Settings/DT_IC_CombatSettings.csv').open(encoding='utf-8-sig', newline='') as stream:
    source = next(row for row in csv.DictReader(stream) if row['ID'] == 'CFG_COMBAT_001')
row = next(row for row in rows if row['ID'] == 'CFG_COMBAT_001')
for key in ['DodgeDurationPlaySeconds', 'DodgeTravelPlaySeconds', 'DodgeInvulnerabilityPlaySeconds', 'DodgeAnimationPlayRate', 'PlayerDodgeDistanceScale']:
    row[key] = float(source[key])
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows))
assert u.EditorAssetLibrary.save_loaded_asset(table, False)
(ROOT / 'Saved/PlayerDodgeTimingSetup.json').write_text(json.dumps({'complete': True, 'montages': sorted(saved),
    'duration': row['DodgeDurationPlaySeconds'], 'travel': row['DodgeTravelPlaySeconds'], 'play_rate': row['DodgeAnimationPlayRate'],
    'invulnerability': row['DodgeInvulnerabilityPlaySeconds'], 'distance_scale': row['PlayerDodgeDistanceScale']}, indent=2), encoding='utf-8')
u.log('IC_PLAYER_DODGE_TIMING_SETUP_COMPLETE')
