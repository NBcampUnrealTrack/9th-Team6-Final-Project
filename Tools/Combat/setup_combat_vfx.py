"""Author editable combat Niagara systems and the matching DT defaults."""
import json
import csv
import io
import re
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
FOLDER = '/Game/IsekaiClass/VFX/CombatEffect'


def particle_material():
    path = FOLDER + '/Materials/M_IC_CombatParticle'
    material = u.load_asset(path)
    if material:
        return material
    if not material:
        material = u.AssetToolsHelpers.get_asset_tools().create_asset(
            'M_IC_CombatParticle', FOLDER + '/Materials', u.Material, u.MaterialFactoryNew())
    material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property('two_sided', True)
    u.MaterialEditingLibrary.set_material_usage(material, u.MaterialUsage.MATUSAGE_NIAGARA_MESH_PARTICLES)
    u.MaterialEditingLibrary.delete_all_material_expressions(material)
    color = u.MaterialEditingLibrary.create_material_expression(material, u.MaterialExpressionParticleColor)
    u.MaterialEditingLibrary.connect_material_property(color, 'RGB', u.MaterialProperty.MP_EMISSIVE_COLOR)
    u.MaterialEditingLibrary.recompile_material(material)
    assert u.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def save(asset):
    assert u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), asset


def csv_text(fields, rows):
    output = io.StringIO(newline='')
    writer = csv.DictWriter(output, fieldnames=fields, lineterminator='\n')
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue()


def patch_table(relative, callback):
    table = u.load_asset('/Game/IsekaiClass/Data/DataTable/' + relative)
    assert table, relative
    reader = csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table)))
    fields, rows = reader.fieldnames, list(reader)
    for row in rows:
        callback(row)
    text = csv_text(fields, rows)
    assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, text)
    save(table)
    (ROOT / 'Data' / (relative + '.csv')).write_text(text, encoding='utf-8', newline='')


particle_material()
report = {'systems': [], 'rows': []}
systems = {}
for group in ('NormalAttack', 'Skill'):
    for role, shape in (('SwordSlash', 'Slash'), ('ConeRange', 'Range'), ('Hit', 'Impact'),
                        ('Arrow', 'Arrow'), ('MagicOrb', 'Orb'), ('ProjectileTrail', 'Trail')):
        name = 'NS_IC_' + ('Normal_' if group == 'NormalAttack' else 'Skill_') + role
        path = FOLDER + '/' + group + '/' + name
        system = u.ICCombatVFXSetupLibrary.create_combat_niagara(path, shape)
        assert system and u.ICCombatVFXSetupLibrary.is_combat_niagara_ready(system), path
        save(system)
        systems[group, role] = path + '.' + name
        report['systems'].append(path)

# Keep the current normal attack silhouette. Existing project spell assets are
# copied separately, retaining their original material/texture dependencies.
for name, source in (
    ('NS_IC_Skill_ProjectCircle', '/Game/IsekaiClass/VFX/SampleVFX/Free/Free_Spells/VFX_Niagara/NS_Free_Spells_Circle'),
    ('NS_IC_Skill_ProjectSlash', '/Game/IsekaiClass/VFX/SampleVFX/Free/Free_Magic/VFX_Niagara/NS_Free_Magic_Slash')):
    path = FOLDER + '/Skill/' + name
    system = u.ICCombatVFXSetupLibrary.duplicate_combat_niagara(path, u.load_asset(source))
    assert system and u.ICCombatVFXSetupLibrary.is_combat_niagara_ready(system), path
    save(system)
    systems['Skill', name] = path + '.' + name
    report['systems'].append(path)

TABLE = '/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_CombatVFX'
COLORS = {'SWORD': (.5, .8, 1), 'BOW': (1, .6, .12), 'STAFF': (.12, .4, 1), 'WAND': (.15, 1, .52)}


def style(system=None, color=(1, 1, 1), intensity=3, scale=1, duration=0, reference=0, width=2):
    return '(bEnabled=%s,System="%s",Color=(R=%s,G=%s,B=%s,A=1),Intensity=%s,Scale=(X=%s,Y=%s,Z=%s),Duration=%s,ReferenceDuration=%s,WidthCm=%s)' % (
        'True' if system else 'False', system or 'None', *color, intensity, scale, scale, scale, duration, reference, width)


rows = []
for family, color in COLORS.items():
    for stage in range(1, 4):
        tint = (1, .65, .18) if family == 'SWORD' and stage == 3 else color
        row = {'---': f'NORMAL_{family}_{stage}', 'Description': f'{family} 일반 공격 {stage}단',
               'Slash': style(systems['NormalAttack', 'SwordSlash'] if family == 'SWORD' else None, tint, duration=.24, width=3 if stage == 3 else 2),
               'Range': style(systems['NormalAttack', 'ConeRange'] if family == 'SWORD' else None, tint, duration=.24, width=.7),
               'Projectile': style(systems['NormalAttack', 'Arrow' if family == 'BOW' else 'MagicOrb'] if family != 'SWORD' else None, color, 4),
               'Trail': style(systems['NormalAttack', 'ProjectileTrail'] if family != 'SWORD' else None, color, 4, width=1.75),
               'Cast': style(), 'Impact': style(systems['NormalAttack', 'Hit'], color, duration=.16, width=1.2),
               'SlashSlope': {1: .22, 2: -.22, 3: 0}[stage], 'bLegacyMeshFallback': 'True'}
        rows.append(row)
    skill = dict(row)
    skill.update({'---': f'SKILL_{family}', 'Description': f'{family} 스킬 · 프로젝트 에셋과 기존 형태 혼합',
                  'Slash': style(), 'Range': style(), 'SlashSlope': 0,
                  'Projectile': style(systems['Skill', 'Arrow' if family == 'BOW' else 'MagicOrb'], color, 4),
                  'Trail': style(systems['Skill', 'ProjectileTrail'], color, 4, width=1.75),
                  'Cast': style(systems['Skill', 'NS_IC_Skill_ProjectCircle'], color, 1, .18, .35, 1),
                  'Impact': style(systems['Skill', 'Hit'], color, duration=.16, width=1.2)})
    rows.append(skill)

table = u.load_asset(TABLE)
if not table:
    factory = u.DataTableFactory()
    factory.set_editor_property('struct', u.load_object(None, '/Script/IsekaiClass.ICCombatVFXRow'))
    table = u.AssetToolsHelpers.get_asset_tools().create_asset('DT_IC_CombatVFX', TABLE.rsplit('/', 1)[0], u.DataTable, factory)
text = csv_text(list(rows[0]), rows)
assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, text)
save(table)
(ROOT / 'Data/Combat/DT_IC_CombatVFX.csv').write_text(text, encoding='utf-8', newline='')
report['rows'] = [r['---'] for r in rows]


def visuals(text, name):
    handle = '(DataTable="' + TABLE + '.DT_IC_CombatVFX",RowName="' + name + '")'
    if re.search(r'Visuals=\([^)]*\)', text):
        return re.sub(r'Visuals=\([^)]*\)', 'Visuals=' + handle, text)
    return text[:-1] + (',' if text != '()' else '') + 'Visuals=' + handle + ')'


def class_row(row):
    family = row['WeaponFamily'].upper()
    if family not in COLORS:
        return
    stages = re.split(r'(?<=\)),(?=\(DamageCoefficient=)', row['NormalAttackStages'][1:-1])
    assert len(stages) == 3, family
    row['NormalAttackStages'] = '(' + ','.join(visuals(s, f'NORMAL_{family}_{i}') for i, s in enumerate(stages, 1)) + ')'


def skill_row(row):
    family = row['RequiredWeaponFamily'].upper()
    if row['---'].startswith('SKL_START_') and family in COLORS:
        row['Presentation'] = visuals(row['Presentation'], 'SKILL_' + family)


patch_table('Character/DT_IC_Class', class_row)
patch_table('Combat/DT_IC_Skill', skill_row)
(ROOT / 'Saved/CombatVFXSetupReport.json').write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
if u.EditorAssetLibrary.does_asset_exist(FOLDER + '/NormalAttack/NS_IC_Test'):
    u.EditorAssetLibrary.delete_asset(FOLDER + '/NormalAttack/NS_IC_Test')
u.log('IC_COMBAT_VFX_SETUP_COMPLETE')
