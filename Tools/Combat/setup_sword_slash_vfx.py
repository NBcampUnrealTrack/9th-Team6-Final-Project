"""Derive a minimal normal sword slash from the project's Free Magic crescent.

Run with -ICApplySwordSlashVFX -run=pythonscript -script=<this file>.
Only the three NORMAL_SWORD visual rows and the new derived assets are saved.
"""
import csv
import hashlib
import io
import json
import shutil
from pathlib import Path
import unreal as u

assert '-ICApplySwordSlashVFX' in u.SystemLibrary.get_command_line()
ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / 'Saved/SwordSlashVFX'
OUT.mkdir(parents=True, exist_ok=True)
FOLDER = '/Game/IsekaiClass/VFX/CombatEffect/NormalAttack/Sword'
SOURCE = '/Game/IsekaiClass/VFX/SampleVFX/Free/Free_Magic'
TABLE = '/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_CombatVFX'


def fingerprint():
    paths = list((ROOT / 'Content/IsekaiClass/VFX/SampleVFX/Free/Free_Magic').rglob('*.uasset'))
    paths += list((ROOT / 'Content/IsekaiClass/Animation/Player').rglob('AM_Combo_Sword_*.uasset'))
    paths += [ROOT / 'Content/IsekaiClass/Data/DataTable/Character/DT_IC_Class.uasset']
    return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}


def save(asset):
    assert u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), str(asset)


before = fingerprint()
table = u.load_asset(TABLE)
assert table
backup = OUT / 'Backups'
backup.mkdir(exist_ok=True)
for relative in ('Data/Combat/DT_IC_CombatVFX.csv', 'Content/IsekaiClass/Data/DataTable/Combat/DT_IC_CombatVFX.uasset'):
    target = backup / Path(relative).name
    if not target.exists():
        shutil.copy2(ROOT / relative, target)

material = u.load_asset(FOLDER + '/M_IC_NormalSwordSlash')
if not material:
    material = u.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_IC_NormalSwordSlash', FOLDER, u.Material, u.MaterialFactoryNew())
material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('blend_mode', u.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property('two_sided', True)
u.MaterialEditingLibrary.set_material_usage(material, u.MaterialUsage.MATUSAGE_NIAGARA_MESH_PARTICLES)
u.MaterialEditingLibrary.delete_all_material_expressions(material)
lib = u.MaterialEditingLibrary
position = lib.create_material_expression(material, u.MaterialExpressionPreSkinnedPosition, -900, 0)
interpolator = lib.create_material_expression(material, u.MaterialExpressionVertexInterpolator, -700, 0)
assert lib.connect_material_expressions(position, '', interpolator, '')
color = lib.create_material_expression(material, u.MaterialExpressionParticleColor, -700, 180)
shape = lib.create_material_expression(material, u.MaterialExpressionCustom, -400, 0)
shape.set_editor_property('description', 'One tapered crescent, white core and pale cyan edge')
shape.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT4)
inputs = []
for name in ('P', 'Tint', 'Alpha'):
    value = u.CustomInput()
    value.set_editor_property('input_name', name)
    inputs.append(value)
shape.set_editor_property('inputs', inputs)
shape.set_editor_property('code', '''
float radius = length(P.xy);
float edgeDistance = P.x * 1.279942 - abs(P.y);
float sector = saturate(edgeDistance / 5.0);
float width = 18.0 * saturate(edgeDistance / 36.0);
float inner = 100.0 - width;
float band = saturate((radius - inner) / 3.0) * saturate((101.0 - radius) / 2.0);
float core = saturate(1.0 - abs(radius - (inner + width * 0.65)) / max(1.0,width * 0.22));
return float4(lerp(Tint, float3(1.2,1.3,1.4), core * 0.8), sector * band * saturate(Alpha));
''')
assert lib.connect_material_expressions(interpolator, '', shape, 'P')
assert lib.connect_material_expressions(color, 'RGB', shape, 'Tint')
assert lib.connect_material_expressions(color, 'A', shape, 'Alpha')
rgb = lib.create_material_expression(material, u.MaterialExpressionComponentMask, -100, 0)
for key, value in [('r', True), ('g', True), ('b', True), ('a', False)]:
    rgb.set_editor_property(key, value)
alpha = lib.create_material_expression(material, u.MaterialExpressionComponentMask, -100, 120)
for key, value in [('r', False), ('g', False), ('b', False), ('a', True)]:
    alpha.set_editor_property(key, value)
assert lib.connect_material_expressions(shape, '', rgb, '')
assert lib.connect_material_expressions(shape, '', alpha, '')
assert lib.connect_material_property(rgb, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
assert lib.connect_material_property(alpha, '', u.MaterialProperty.MP_OPACITY)
lib.recompile_material(material)
save(material)

system = u.ICCombatVFXSetupLibrary.create_simple_sword_slash(
    FOLDER + '/NS_IC_NormalSwordSlash', u.load_asset(SOURCE + '/VFX_Niagara/NS_Free_Magic_Slash'),
    u.load_asset(SOURCE + '/Mesh/SM_Free_Magic_Slash1'), material)
assert system and u.ICCombatVFXSetupLibrary.is_combat_niagara_ready(system), 'New slash must compile'
save(system)

reader = csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table)))
fields, rows = reader.fieldnames, list(reader)
original = {row['---']: dict(row) for row in rows}
changed = []
for row in rows:
    if row['---'] not in ('NORMAL_SWORD_1', 'NORMAL_SWORD_2', 'NORMAL_SWORD_3'):
        continue
    duration = .15 if row['---'].endswith('3') else .14
    row['Slash'] = ('(bEnabled=True,System="%s",Color=(R=0.32,G=0.7,B=1,A=1),Intensity=1.05,'
                    'Scale=(X=1,Y=1,Z=0.25),bUseComponentScale=True,Duration=%s,ReferenceDuration=0,'
                    'Offset=(X=0,Y=0,Z=0),Rotation=(Pitch=0,Yaw=0,Roll=0),WidthCm=2)') % (system.get_path_name(), duration)
    row['Range'] = row['Range'].replace('bEnabled=True', 'bEnabled=False')
    row['bLegacyMeshFallback'] = 'False'
    row['bFollowWeaponSwing'] = 'True'
    changed.append(row['---'])
assert len(changed) == 3
output = io.StringIO(newline='')
writer = csv.DictWriter(output, fieldnames=fields, lineterminator='\n')
writer.writeheader()
writer.writerows(rows)
assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, output.getvalue())
check = list(csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table))))
assert all(row == original[row['---']] for row in check if row['---'] not in changed), 'Other VFX rows changed'
assert before == fingerprint(), 'Source VFX, animations and combat balance must be untouched'
save(table)
csv_path = ROOT / 'Data/Combat/DT_IC_CombatVFX.csv'
csv_export = OUT / 'DT_IC_CombatVFX.csv'
csv_export.write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table), encoding='utf-8', newline='')
csv_export.replace(csv_path)
(OUT / 'Setup.json').write_text(json.dumps({'complete': True, 'changed_rows': changed,
    'system': system.get_path_name(), 'source': SOURCE, 'source_and_animation_hashes_unchanged': True}, indent=2), encoding='utf-8')
u.log('IC_SWORD_SLASH_SETUP_COMPLETE')
