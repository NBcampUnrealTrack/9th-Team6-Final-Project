"""Retarget three normal attacks per weapon for both players and import class tuning.

Run in Unreal Editor Python after building IsekaiClassEditor. Source clips and
existing locomotion, physics, skills and weapon calibration are preserved.
"""
import csv
import io
import json
import re
import sys
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0, str(ROOT / 'Tools/Characters'))
import setup_assets as setup

SPECS = {
    'Sword': ('Sword_Animations', [f'Combo_Attack_01_0{i}_Seq' for i in range(1, 4)], .38, 0, 0),
    'Bow': ('Archer', [f'AS_Combo_Attack_01_0{i}_Seq' for i in range(1, 4)], .48, 3000, 3000),
    'Staff': ('Mage', ['AS_Attack_02_01_Seq', 'AS_Attack_02_02_Seq', 'AS_Attack_01_02_Seq'], .46, 1800, 1800),
    'Wand': ('Mage', ['AS_Attack_02_01_Seq', 'AS_Attack_02_02_Seq', 'AS_Attack_01_02_Seq'], .46, 1800, 1800),
}


# Contact frames in the retargeted Combo_Attack_01 clips, on a 60 Hz timeline.
# The second stage reverses its slash; the finisher rises and then strikes down.
SWORD_CONTACT_FRAMES = ((9,), (9, 27), (11, 44))


def build_combo_montage(folder, family, stage, clip, fraction, projectile):
    times = [frame / 60 for frame in SWORD_CONTACT_FRAMES[stage]] if family == 'Sword' else [round(clip.get_play_length() * fraction, 6)]
    path = folder + f'/AM_Combo_{family}_{stage + 1}'
    montage = u.ICCharacterSetupLibrary.create_normal_attack_montage(path, clip, times[0], projectile)
    if not montage:
        raise RuntimeError('Attack montage generation failed: ' + path)
    for index, time in enumerate(times[1:], 1):
        notify = u.AnimationLibrary.add_animation_notify_event(montage, 'ICNormalAttack', time, u.ICCombatAnimNotify)
        notify.set_editor_property('event', 'AN_HitStart')
        notify.set_editor_property('hit_index', index)
    setup.save(montage)
    return montage, times[0]


def glow_material():
    material = setup.create(setup.GAME + '/Combat/Effects/M_AttackGlow', u.Material, u.MaterialFactoryNew())
    material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property('two_sided', True)
    u.MaterialEditingLibrary.delete_all_material_expressions(material)
    color = u.MaterialEditingLibrary.create_material_expression(material, u.MaterialExpressionVectorParameter)
    color.set_editor_property('parameter_name', 'Color')
    color.set_editor_property('default_value', u.LinearColor(.4, .7, 1, 1))
    u.MaterialEditingLibrary.connect_material_property(color, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    u.MaterialEditingLibrary.recompile_material(material)
    setup.save(material)


def tune_classes(timing):
    file = ROOT / 'Data/Character/DT_IC_Class.csv'
    with file.open(encoding='utf-8-sig', newline='') as source:
        reader = csv.DictReader(source)
        fields, rows = reader.fieldnames, list(reader)
    for row in rows:
        family = row['WeaponFamily']
        if family not in timing:
            continue
        _, _, _, distance, speed = SPECS[family]
        # Split only on stage boundaries; Hit and Vector nested structs remain intact.
        stages = re.split(r'(?<=\)),(?=\(DamageCoefficient=)', row['NormalAttackStages'][1:-1])
        if len(stages) != 3:
            raise RuntimeError('Expected exactly three class attacks: ' + family)
        output = []
        for index, stage in enumerate(stages):
            hit_time, length = timing[family][index]
            values = {
                'DamageCoefficient': [1, 1.1, 1.4][index],
                'StartOffsetPlaySeconds': hit_time,
                'TraceRadiusCm': 8 if family == 'Sword' else 7 if family == 'Bow' else 12,
                'GuardDodgeCancelStartPlaySeconds': min(length - .05, hit_time + .1),
                'GuardDodgeCancelEndPlaySeconds': length,
            }
            if family == 'Sword':
                frames = SWORD_CONTACT_FRAMES[index]
                values.update(RepeatCount=len(frames), RepeatIntervalPlaySeconds=(frames[-1]-frames[0])/60,
                              GuardDodgeCancelStartPlaySeconds=frames[-1]/60 + .15)
            if distance:
                values.update(ProjectileSpeedCmPerSecond=speed, ProjectileLifetimePlaySeconds=distance / speed)
                stage = re.sub(r'Shape=\w+', 'Shape=Projectile', stage)
                if 'ProjectileMaxDistanceCm=' not in stage:
                    stage = stage.replace('ProjectileClass=', f'ProjectileMaxDistanceCm={distance:.6f},ProjectileClass=')
                else:
                    values['ProjectileMaxDistanceCm'] = distance
            for key, value in values.items():
                stage, count = re.subn(r'(?<!\w)' + key + r'=[0-9.]+', (f'{key}={int(value)}' if key == 'RepeatCount' else f'{key}={value:.6f}'), stage, count=1)
                if count != 1:
                    raise RuntimeError(f'Could not tune {family}/{index}/{key}')
            output.append(stage)
        row['NormalAttackStages'] = '(' + ','.join(output) + ')'
    buffer = io.StringIO(newline='')
    writer = csv.DictWriter(buffer, fieldnames=fields, lineterminator='\n')
    writer.writeheader()
    writer.writerows(rows)
    file.write_text(buffer.getvalue(), encoding='utf-8')
    table = setup.asset(setup.GAME + '/Data/DataTable/Character/DT_IC_Class')
    if not u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, buffer.getvalue()):
        raise RuntimeError('Class table import failed')
    setup.save(table)


def main():
    glow_material()
    report, timing = [], {}
    for gender, suffix in [('Male', 'M'), ('Female', 'F')]:
        folder = f'{setup.GAME}/Animation/Player/Main{gender}'
        mesh = setup.asset(f'{setup.GAME}/Player/SamplePlayer/{gender}/SK_Player_{suffix}')
        rig = setup.asset(f'{setup.GAME}/Player/SamplePlayer/{gender}/IK_Player_{suffix}_Mannequin')
        profile = setup.asset(folder + '/DA_ANIM_PLAYER_' + suffix)
        sets = dict(profile.get_editor_property('weapon_sets'))
        for family, (pack, names, fraction, distance, _) in SPECS.items():
            source = {str(i): setup.sequence(pack, name) for i, name in enumerate(names)}
            clips = setup.retarget(pack, source, mesh, rig, folder + '/' + pack)
            montages, times = [], []
            for i in range(3):
                clip = clips[str(i)]
                clip.set_editor_property('enable_root_motion', False)
                clip.set_editor_property('force_root_lock', True)
                clip.set_editor_property('root_motion_root_lock', u.RootMotionRootLock.REF_POSE)
                setup.save(clip)
                length = clip.get_play_length()
                montage, hit_time = build_combo_montage(folder, family, i, clip, fraction, bool(distance))
                montages.append(montage)
                times.append((hit_time, length))
                report.append(dict(gender=gender, family=family, stage=i + 1, clip=clip.get_path_name(),
                                   montage=montage.get_path_name(), length=length, hit_time=hit_time, range_cm=distance))
            key = getattr(u.ICWeaponFamily, family.upper())
            state = sets[key]
            state.set_editor_property('attack_combo', montages)
            state.set_editor_property('attack', montages[0])
            sets[key] = state
            if gender == 'Male':
                timing[family] = times
            elif any(abs(a - b) > .001 for pair_a, pair_b in zip(times, timing[family]) for a, b in zip(pair_a, pair_b)):
                raise RuntimeError('Male/female combo timing mismatch: ' + family)
        profile.set_editor_property('weapon_sets', sets)
        setup.save(profile)
    tune_classes(timing)
    (ROOT / 'Saved/PlayerComboSetupReport.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    u.log('IC_PLAYER_COMBO_SETUP_COMPLETE')


if __name__ == '__main__':
    main()
