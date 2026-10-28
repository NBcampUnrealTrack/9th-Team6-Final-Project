"""One-time targeted authoring of NPC skills, weapon tiers and the requested UI.

Balance future changes in the DTs. Re-running this script reapplies these defaults.
Original sample Niagara systems and weapon meshes are never overwritten.
"""
import array
import copy
import csv
import io
import json
import math
import random
import wave
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
BASE = '/Game/IsekaiClass/Data/DataTable/'
FX_ROOT = '/Game/IsekaiClass/VFX/CombatEffect/Skill/NPC'
OUT = ROOT / 'Saved/NPCSkillReview'
OUT.mkdir(parents=True, exist_ok=True)
TOOLS = u.AssetToolsHelpers.get_asset_tools()
TABLES = {}
REPORT = {'weapons': [], 'skills': [], 'sounds': [], 'widgets': []}

def save(asset):
    assert u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), asset

def table(relative):
    if relative not in TABLES:
        asset = u.load_asset(BASE + relative)
        assert asset, relative
        rows = json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(asset))
        TABLES[relative] = asset, {r['Name']: r for r in rows}
    return TABLES[relative][1]

def flush_tables():
    for relative, (asset, rows) in TABLES.items():
        old = asset.get_editor_property('ignore_missing_fields')
        asset.set_editor_property('ignore_missing_fields', True)
        try:
            assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(asset, json.dumps(list(rows.values()), ensure_ascii=False)), relative
        finally:
            asset.set_editor_property('ignore_missing_fields', old)
        save(asset)
        text = u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset)
        reader=csv.DictReader(io.StringIO(text));output=io.StringIO(newline='')
        writer=csv.DictWriter(output,fieldnames=reader.fieldnames,lineterminator='\n');writer.writeheader();writer.writerows(reader)
        (ROOT / 'Data' / (relative + '.csv')).write_text(output.getvalue(), encoding='utf-8', newline='')

STRINGS_PATH = ROOT / 'Data/StringTables/ST_IC_Definitions.csv'
STRINGS = {r['Key']: r['SourceString'] for r in csv.DictReader(STRINGS_PATH.open(encoding='utf-8-sig', newline=''))}

def name(row, identifier, display, description):
    row.update(Name=identifier, ID=identifier, NameKey=identifier + '.NAME', DescriptionKey=identifier + '.DESCRIPTION', bDeprecated=False)
    STRINGS[identifier + '.NAME'] = display
    STRINGS[identifier + '.DESCRIPTION'] = description

def localized(key):
    return f'LOCTABLE("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions", "{key}")'

REGISTRY = table('Registry/DT_IC_IDRegistry')
REGISTRY_TEMPLATE = copy.deepcopy(next(iter(REGISTRY.values())))

def register(identifier, kind, relative):
    row = copy.deepcopy(REGISTRY.get(identifier, REGISTRY_TEMPLATE))
    row.update(Name=identifier, ID=identifier, DefinitionType=kind, RegionID='R01', ReplacementID='None',
               DefinitionTable=BASE + relative + '.' + relative.rsplit('/', 1)[-1], bDeprecated=False,
               Notes='NPC skill and weapon progression expansion.')
    REGISTRY[identifier] = row

def package_of_file(path):
    return '/Game/' + path.relative_to(ROOT / 'Content').with_suffix('').as_posix()

def mesh_path(folder_name, mesh_name=None):
    candidates = [p for p in (ROOT / 'Content/IsekaiClass/Items/Weapons').rglob('*.uasset')
                  if p.parent.name == 'StaticMeshes' and p.parent.parent.name == folder_name
                  and (mesh_name is None or p.stem == mesh_name)]
    assert len(candidates) == 1, (folder_name, candidates)
    return package_of_file(candidates[0])

def vec(value):
    return {'X': value.x, 'Y': value.y, 'Z': value.z}

def geometry(mesh, family, actor_class):
    box = mesh.get_bounding_box()
    dimensions = [box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z]
    index = max(range(3), key=dimensions.__getitem__)
    axis = u.Vector(*[1 if i == index else 0 for i in range(3)])
    if family!='BOW':axis=axis*-1  # These imported blades/staves point toward the negative major axis.
    cdo = u.get_default_object(u.load_class(None, actor_class))
    visual = cdo.get_editor_property('visual')
    original_rotation = u.MathLibrary.conv_rotator_to_quaternion(visual.get_editor_property('relative_rotation'))
    alignment = u.MathLibrary.quat_find_between_normals(axis, cdo.get_editor_property('length_axis'))
    a, b = original_rotation, alignment
    rotation = u.Quat(a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
                      a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
                      a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
                      a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z)
    length = {'SWORD': 100, 'BOW': 95, 'STAFF': 145, 'WAND': 110}[family]
    scale = length / max(1, dimensions[index])
    center = u.Vector((box.min.x+box.max.x)/2, (box.min.y+box.max.y)/2, (box.min.z+box.max.z)/2)
    fraction = {'SWORD': .14, 'BOW': .5, 'STAFF': .42, 'WAND': .33}[family]
    grip = center + axis * (dimensions[index] * (fraction - .5))
    offset = u.MathLibrary.quat_rotate_vector(rotation, grip * scale) * -1
    return {'Rotation': {'X': rotation.x, 'Y': rotation.y, 'Z': rotation.z, 'W': rotation.w},
            'Translation': vec(offset), 'Scale3D': {'X': scale, 'Y': scale, 'Z': scale}}, vec(axis)

FAMILIES = ['SWORD', 'BOW', 'STAFF', 'WAND']
FAMILY_NAMES = ['검', '활', '마법스태프', '치유완드']
REQUIRED_STATS = ['STR', 'DEX', 'INT', 'CON']
TIER_NAMES = ['초보자', '시골 마을', '단단한', '물고기 비늘', '무지개']
MESHES = [
    ['Bastard_Sword', 'Bow', 'Bronze_Spear', 'bird_skull_staff'],
    ['Bone_Sword', 'Bow_3', 'Double_Spear', 'dark_magical_staff_2'],
    ['Messer_Sword', 'bone_spike_bow', 'Tribal_Spear_2', 'magic_crystal_staff'],
    ['Sun_Sword', 'elven_bow_2', 'tribal_spear_4', 'fantasy_staff'],
]
COSTS = {
    2: [('ITM_R01_ORE_01', 2), ('ITM_R01_PLANT_01', 2)],
    3: [('ITM_R01_ORE_01', 1), ('ITM_R01_ORE_02', 2), ('ITM_R01_ORE_03', 2)],
    4: [('ITM_R01_ORE_04', 4)],
    5: [('ITM_R01_FISH_10', 1), ('ITM_R01_ORE_06', 2), ('ITM_R01_RANCH_01', 1),
        ('ITM_R01_CROP_05', 1), ('ITM_R01_PLANT_09', 1)],
}
ITEMS = table('Items/DT_IC_Item')
GEAR = table('Equipment/DT_IC_Equipment')
RECIPES = table('Recipes/DT_IC_Recipe')
BASE_RECIPE = copy.deepcopy(next(r for r in RECIPES.values() if any(v['ItemID'] == 'EQP_WPN_SWORD_001' for v in r['Results'])))
STRINGS['Recipe.AnyFish'] = '물고기 아무 종류'
for family_index, family in enumerate(FAMILIES):
    original_item = copy.deepcopy(ITEMS[f'EQP_WPN_{family}_001'])
    original_gear = copy.deepcopy(GEAR[f'EQU_WPN_{family}_001'])
    for tier in range(1, 6):
        item_id = f'EQP_WPN_{family}_{tier:03d}'
        gear_id = f'EQU_WPN_{family}_{tier:03d}'
        gear = copy.deepcopy(GEAR.get(gear_id, original_gear))
        damage_stat = 'PhysicalAttack' if family in ('SWORD', 'BOW') else 'MagicAttack'
        gear.update(Name=gear_id, ID=gear_id, ItemID=item_id,
                    RequiredMainStat=REQUIRED_STATS[family_index], RequiredMainStatValue=(tier-1)*10,
                    BaseModifiers=[{'Stat': damage_stat, 'Flat': tier*10, 'Percent': 0}])
        if tier > 1:
            source_mesh = mesh_path(MESHES[tier-2][family_index])
            mesh = u.load_asset(source_mesh)
            assert isinstance(mesh, u.StaticMesh), source_mesh
            transform, axis = geometry(mesh, family, original_gear['WeaponActorClass'])
            gear.update(WeaponMesh=source_mesh + '.' + source_mesh.rsplit('/', 1)[-1],
                        bOverrideWeaponGeometry=True, WeaponVisualTransform=transform, WeaponLengthAxis=axis)
        GEAR[gear_id] = gear
        if tier == 1:
            continue
        display = TIER_NAMES[tier-1] + ' ' + FAMILY_NAMES[family_index]
        item = copy.deepcopy(ITEMS.get(item_id, original_item))
        name(item, item_id, display, f'{display}. 기본 공격력 {tier*10}. 요구 {REQUIRED_STATS[family_index]} {(tier-1)*10}.')
        item.update(ItemID=item_id, EquipmentID=gear_id, DisplayName=localized(item_id+'.NAME'), BaseAttack=tier*10,
                    WorldMesh='/Engine/BasicShapes/Cube.Cube', WorldMeshScale={'X': .25, 'Y': .25, 'Z': .25}, SellPrice=tier*50)
        ITEMS[item_id] = item
        recipe_id = f'RCP_WPN_{family}_{tier:03d}'
        recipe = copy.deepcopy(RECIPES.get(recipe_id, BASE_RECIPE))
        name(recipe, recipe_id, display+' 제작', display+' 제작법. 재료와 제작 시간은 DT에서 변경할 수 있습니다.')
        recipe.update(RecipeID=recipe_id, DisplayName=localized(recipe_id+'.NAME'),
                      Ingredients=[{'ItemID': identifier, 'Count': count} for identifier, count in COSTS[tier]],
                      Results=[{'ItemID': item_id, 'Count': 1}], IngredientGroups=[], WorkDurationPlaySeconds=5, SPCost=0)
        if tier == 4:
            recipe['IngredientGroups'] = [{'DisplayName': localized('Recipe.AnyFish'),
                                          'ItemIDs': [f'ITM_R01_FISH_{i:02d}' for i in range(1, 11)], 'Count': 10}]
        RECIPES[recipe_id] = recipe
        register(item_id, 'Item', 'Items/DT_IC_Item')
        register(gear_id, 'Equipment', 'Equipment/DT_IC_Equipment')
        register(recipe_id, 'Recipe', 'Recipes/DT_IC_Recipe')
        REPORT['weapons'].append({'item_id': item_id, 'equipment_id': gear_id, 'name': display, 'mesh': source_mesh,
                                  'damage': tier*10, 'required_stat': REQUIRED_STATS[family_index], 'minimum': (tier-1)*10,
                                  'recipe_id': recipe_id})

# Foods and growth books can be selected for the player or a living party member.
old_flags_path=OUT/'OriginalItemUseFlags.json'
old_flags=json.loads(old_flags_path.read_text(encoding='utf-8')) if old_flags_path.exists() else {}
for item in ITEMS.values():
    if item['Kind'] == 'Consumable' and item['UseEffect'] in ('Recovery', 'GrowthBook'):
        item['UseTarget'] = 'LivingParty'
        item['bAllowNonCompanionRecruitUse'] = old_flags.get(item['ID'],item['bAllowNonCompanionRecruitUse'])

def sound(kind):
    asset_path = '/Game/IsekaiClass/Audio/Combat/S_IC_' + kind
    asset = u.load_asset(asset_path)
    if not asset:
        folder = ROOT / 'Data/Audio/Sources'
        folder.mkdir(parents=True, exist_ok=True)
        filename = folder / ('S_IC_' + kind + '.wav')
        rate, duration = 24000, .28 if kind != 'Healing' else .5
        rng = random.Random(kind)
        samples = array.array('h')
        for index in range(int(rate * duration)):
            t = index / rate
            envelope = min(1, t/.012) * math.exp(-t/(duration*.27))
            if kind == 'Healing':
                value = sum(math.sin(2*math.pi*f*t) for f in (523.25, 659.25, 783.99))/3
            elif kind == 'Shield':
                value = math.sin(2*math.pi*220*t)+.35*math.sin(2*math.pi*880*t)
            elif kind in ('Arrow', 'Slash'):
                value = rng.uniform(-1, 1)*.7+math.sin(2*math.pi*(650*t+1100*t*t))*.3
            elif kind == 'Impact':
                value = rng.uniform(-1, 1)*.5+math.sin(2*math.pi*(180*t-190*t*t))*.5
            else:
                value = math.sin(2*math.pi*(400*t+1800*t*t))*.65+math.sin(2*math.pi*900*t)*.35
            samples.append(int(max(-1, min(1, value*envelope*.2))*32767))
        with wave.open(str(filename), 'wb') as stream:
            stream.setnchannels(1);stream.setsampwidth(2);stream.setframerate(rate);stream.writeframes(samples.tobytes())
        task = u.AssetImportTask();task.filename = str(filename);task.destination_path = '/Game/IsekaiClass/Audio/Combat'
        task.destination_name = 'S_IC_' + kind;task.automated = True;task.save = True
        TOOLS.import_asset_tasks([task]);asset = u.load_asset(asset_path)
    assert isinstance(asset, u.SoundWave), asset_path
    REPORT['sounds'].append(asset_path)
    return asset_path + '.' + asset_path.rsplit('/', 1)[-1]

SOUNDS = {kind: sound(kind) for kind in ('Magic', 'Arrow', 'Slash', 'Impact', 'Healing', 'Shield')}
AUDIT = json.loads((OUT / 'Audit/Assets.json').read_text(encoding='utf-8'))
VFX = table('Combat/DT_IC_CombatVFX')
VFX_TEMPLATE = copy.deepcopy(next(iter(VFX.values())))
STYLE_TEMPLATE = copy.deepcopy(VFX_TEMPLATE['Impact'])
SKILLS = table('Combat/DT_IC_Skill')
EFFECTS = table('Combat/DT_IC_SkillEffect')
STATUSES = table('Combat/DT_IC_StatusEffect')
EFFECT_TEMPLATE = copy.deepcopy(EFFECTS['SFE_START_MAGIC_001'])
COLORS = [( .12,1,.25),(.30,.27,.62),(1,.68,.84),(1,.88,.40),(.7,.38,.20),(.46,.23,1),
          (.7,.78,.9),(1,.53,.18),(1,.77,.30),(.24,.34,.5),(.68,.42,.76)]
SPECS = [
    # name, shape, cast, cooldown, mana, radius/range, flat, coefficient, repeats, audio
    ('선형 폭발', 'Cone', .45, 9, 12, 1000, 25, 1.8, 1, 'Magic'),
    ('섬광 베기', 'Cone', .12, 6, 8, 340, 18, 1.7, 1, 'Slash'),
    ('치유의 빛', 'Direct', .6, 8, 14, 1600, 45, 1.8, 1, 'Healing'),
    ('마력 화살', 'Projectile', .25, 5, 7, 2400, 16, 1.5, 1, 'Magic'),
    ('원형 충격파', 'Sphere', .4, 11, 14, 430, 28, 1.7, 1, 'Impact'),
    ('마력 참격', 'Cone', .25, 7, 9, 600, 20, 2.1, 1, 'Slash'),
    ('관통 화살', 'Projectile', .3, 7, 9, 3200, 24, 1.8, 1, 'Arrow'),
    ('별빛 대폭발', 'GroundArea', .9, 25, 30, 600, 85, 3.5, 3, 'Magic'),
    ('회복의 선율', 'Sphere', .5, 15, 18, 650, 0, 0, 1, 'Healing'),
    ('수호의 방패', 'Direct', .35, 18, 15, 0, 0, 0, 1, 'Shield'),
    ('연속 화살', 'Projectile', .25, 9, 12, 3200, 12, 1.0, 3, 'Arrow'),
]
HIT_SOURCES = {
    2: '/Game/IsekaiClass/VFX/SampleVFX/MagicProjectilesVol5/Particles/NiagaraSystems/Hits/NS_Hit_Slash01',
    4: '/Game/IsekaiClass/VFX/SampleVFX/MagicProjectilesVol4/Particles/NiagaraSystems/NS_Hit_Shard_V1',
    6: '/Game/IsekaiClass/VFX/SampleVFX/MagicProjectilesVol5/Particles/NiagaraSystems/Hits/NS_Hit_Slash01',
    7: '/Game/IsekaiClass/VFX/SampleVFX/MagicProjectilesVol4/Particles/NiagaraSystems/NS_Hit_Arrow01',
    11: '/Game/IsekaiClass/VFX/SampleVFX/MagicProjectilesVol4/Particles/NiagaraSystems/NS_Hit_Arrow05',
}

def duplicate(source, path):
    original = u.load_asset(source)
    assert isinstance(original, u.NiagaraSystem), source
    system = u.ICCombatVFXSetupLibrary.duplicate_combat_niagara(path, original)
    assert system and u.ICCombatVFXSetupLibrary.repair_combat_niagara_color(system), path
    save(system)
    return path + '.' + path.rsplit('/', 1)[-1]

def style(system, color, scale=1, duration=1, intensity=2, offset=(0,0,0), reference=0):
    value = copy.deepcopy(STYLE_TEMPLATE)
    value.update(bEnabled=True, System=system, Color=dict(zip(('R','G','B','A'), (*color,1))),
                 Intensity=intensity, Scale={'X':scale,'Y':scale,'Z':scale}, bUseComponentScale=True, Duration=duration,
                 ReferenceDuration=reference, Offset=dict(zip(('X','Y','Z'),offset)), Rotation={'Pitch':0,'Yaw':0,'Roll':0})
    return value

def meta(identifier, display, description):
    return {'Name': identifier, 'ID': identifier, 'NameKey': identifier+'.NAME', 'DescriptionKey': identifier+'.DESCRIPTION',
            'Tags': {'GameplayTags': []}, 'DataVersion': 1, 'bDeprecated': False, 'bTestOnly': False}

for index, spec in enumerate(SPECS, 1):
    label, shape, cast, cooldown, mana, radius, flat, coefficient, repeats, audio = spec
    skill_id = f'SKL_NPC_R01_{index:03d}'
    skill = SKILLS[skill_id]
    color = COLORS[index-1]
    folder = FX_ROOT + f'/R01_{index:02d}'
    source = AUDIT['systems'][index-1]['source'][0]
    main = duplicate(source, folder+f'/NS_R01_{index:02d}_Skill')
    impact = duplicate(HIT_SOURCES.get(index, source), folder+f'/NS_R01_{index:02d}_Hit')
    visuals = copy.deepcopy(VFX.get(f'NPC_R01_{index:03d}', VFX_TEMPLATE))
    visuals.update(Name=f'NPC_R01_{index:03d}', Description=f'R01_{index:02d} · {label}',
                   CastLayers=[], ImpactLayers=[], bCastAtTarget=index==8,
                   bImpactFollowsTarget=True, bLegacyMeshFallback=False)
    for role in ('Slash','Range','Projectile','Trail','Cast','Impact'):
        visuals[role] = copy.deepcopy(STYLE_TEMPLATE)
        visuals[role].update(bEnabled=False, System='None')
    intensity = 3 if index==8 else 1.8
    visuals['Cast'] = style(main, color, scale=.7 if shape=='Projectile' else 1, duration=max(.6,cast+.6), intensity=intensity)
    visuals['Impact'] = style(impact, color, scale=.6, duration=.7, intensity=intensity)
    if shape == 'Projectile':
        visuals['Cast'] = style(main,color,.28,.35,intensity)
        visuals['Projectile'] = style(main,color,.7,0,intensity)
    if index in (3,9,10):
        visuals['Impact'] = style(impact,color,1,12 if index==10 else 10 if index==9 else 1.5,1.5)
    if index==5:
        visuals['Cast'] = style(main,color,.9,1.2,2,offset=(0,0,-75))
        visuals['Impact'] = style(impact,color,.25,.6,2)
    if index==8:
        circle_source = AUDIT['systems'][4]['source'][0]
        line_source = AUDIT['systems'][0]['source'][0]
        circle = duplicate(circle_source, folder+'/NS_R01_08_NovaRing')
        line = duplicate(line_source, folder+'/NS_R01_08_NovaBurst')
        visuals['Cast'] = style(main,color,1.6,2.1,3.2)
        visuals['CastLayers'] = [style(circle,color,1.6,1.7,3,offset=(0,0,-70)),style(line,(1,.85,.5),1.1,1.7,3)]
        visuals['Impact'] = style(impact,color,.9,1.1,3.2)
        visuals['ImpactLayers'] = [style(circle,(1,.8,.38),.6,.8,3)]
    VFX[visuals['Name']] = visuals
    effect_id = f'SFE_NPC_R01_{index:03d}'
    effect = copy.deepcopy(EFFECTS.get(effect_id, EFFECT_TEMPLATE))
    name(effect,effect_id,label+' 효과',label+'의 피해·회복·상태 효과 수치입니다.')
    family = skill['RequiredWeaponFamily']
    attack_stat = 'PhysicalAttack' if family in ('Sword','Bow') else 'MagicAttack'
    effect.update(Kind='Heal' if index==3 else 'ApplyStatus' if index in (9,10) else 'Damage',
                  DamageType='Physical' if family in ('Sword','Bow') else 'Magical',
                  Magnitude={'SourceStat':attack_stat,'Coefficient':coefficient,'FlatAmount':flat,'bApplyPersonalCHA':False},
                  bCanCritical=index not in (3,9,10), AppliedStatusEffectIDs=[])
    if index in (9,10):
        status_id = f'STE_NPC_R01_{index:03d}'
        status = meta(status_id,label,label+'의 지속 효과입니다.')
        status.update(Kind='Shield' if index==10 else 'HealOverTime', bHarmful=False, TargetSide='Friendly',
                      DurationPlaySeconds=12 if index==10 else 10, TickIntervalPlaySeconds=0 if index==10 else 2,
                      MaximumStacks=1, DamageDealtMultiplier=1, DamageReceivedMultiplier=1, HealingReceivedMultiplier=1)
        if index==10:
            status['InitialShieldMagnitude']={'SourceStat':'MaxHP','Coefficient':.4,'FlatAmount':50,'bApplyPersonalCHA':False}
        else:
            status['PeriodicMagnitude']={'SourceStat':'MagicAttack','Coefficient':.5,'FlatAmount':12,'bApplyPersonalCHA':False}
        STATUSES[status_id]=status;effect['AppliedStatusEffectIDs']=[status_id]
        STRINGS[status_id+'.NAME']=label;STRINGS[status_id+'.DESCRIPTION']=label+' 지속 효과'
        register(status_id,'StatusEffect','Combat/DT_IC_StatusEffect')
    effect['Presentation']=copy.deepcopy(effect['Presentation'])
    effect['Presentation'].update(Visuals={'DataTable':BASE+'Combat/DT_IC_CombatVFX.DT_IC_CombatVFX','RowName':visuals['Name']},ImpactVFX=impact,ImpactSound=SOUNDS['Healing' if index in (3,9) else 'Shield' if index==10 else 'Impact'])
    EFFECTS[effect_id]=effect;register(effect_id,'SkillEffect','Combat/DT_IC_SkillEffect')
    friendly=index in (3,9,10)
    purpose='Heal' if index in (3,9) else 'Shield' if index==10 else 'Attack'
    target='Self' if index==10 else 'LivingParty' if friendly else 'Enemy' if index==8 else 'Forward'
    skill.update(Purpose=purpose, MPCost=mana, CastPlaySeconds=cast, bUseDataTableTiming=True,
                 FirstEffectOffsetPlaySeconds=cast, FirstEffectEvent='EffectCommit',RecoveryPlaySeconds=.7 if index==8 else .45,
                 CooldownPlaySeconds=cooldown)
    skill['TargetRules'].update(Target=target,bRequiresTarget=friendly or index==8,bAllowCaster=friendly,
                                MaximumRangeCm=2000 if index==8 else radius if friendly else 0,bRequireLineOfSight=index==8)
    hit=copy.deepcopy(skill['Hits'][0])
    hit.update(EffectID=effect_id,StartOffsetPlaySeconds=cast,RepeatCount=repeats,RepeatIntervalPlaySeconds=.2 if repeats>1 else 0,
               Shape=shape,RadiusCm=radius,ConeAngleDegrees=35 if index==1 else 110 if index==2 else 75,
               MaximumTargets=8 if index==8 else 4 if friendly else 5,TraceRadiusCm=12,
               ProjectileSpeedCmPerSecond=2100,ProjectileLifetimePlaySeconds=2,ProjectileMaxDistanceCm=radius if shape=='Projectile' else 0,
               bPierceTargets=index==7)
    skill['Hits']=[hit]
    skill['AutoRules'].update(Type='Heal' if index in (3,9) else 'Buff' if index==10 else 'Attack',HealAtOrBelowHPRatio=.65)
    skill['Presentation'].update(CastVFX=main,ImpactVFX=impact,CastSound=SOUNDS[audio],ImpactSound=effect['Presentation']['ImpactSound'],Visuals=effect['Presentation']['Visuals'])
    STRINGS[skill_id+'.NAME']=label
    STRINGS[skill_id+'.DESCRIPTION']=f'R01_{index:02d} 고유 스킬. 시전 {cast}초 · 재사용 {cooldown}초 · MP {mana}. 범위와 피해·회복 수치는 스킬/효과 DT에서 변경합니다.'
    REPORT['skills'].append({'id':skill_id,'name':label,'source':source,'main':main,'hit':impact,'shape':shape,'cast':cast,
                             'cooldown':cooldown,'mana':mana,'range_or_radius':radius,'flat':flat,'coefficient':coefficient,'repeats':repeats,'color':color})
    u.log(f'NPC_SKILL_AUTHORED {index}/11 {skill_id}')

# Import localization before persisting the new FText-backed item and recipe rows.
output=io.StringIO(newline='');writer=csv.writer(output,lineterminator='\n');writer.writerow(['Key','SourceString']);writer.writerows(STRINGS.items())
STRINGS_PATH.write_text(output.getvalue(),encoding='utf-8',newline='')
strings_asset=u.load_asset(BASE+'StringTables/ST_IC_Definitions')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings_asset,str(STRINGS_PATH))
save(strings_asset)
flush_tables()

# Tab uses a single press event and the same action remains configurable in settings.
action=u.load_asset('/Game/IsekaiClass/Input/Actions/IA_TargetNext')
context=u.load_asset('/Game/IsekaiClass/Input/Contexts/IMC_Gameplay')
assert action and context
action.set_editor_property('triggers',[u.InputTriggerPressed(outer=action)])
context.unmap_all_keys_from_action(action)
key=u.Key();key.set_editor_property('key_name','Tab');context.map_key(action,key)
save(action);save(context)

controller_bp=u.load_asset('/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController')
assert controller_bp
controller=u.get_default_object(controller_bp.generated_class())
bindings=list(controller.get_editor_property('command_actions'))
if not any(binding.get_editor_property('command')==u.ICInputCommand.NEXT_TARGET for binding in bindings):
    binding=u.ICInputCommandBinding();binding.set_editor_property('action',action);binding.set_editor_property('command',u.ICInputCommand.NEXT_TARGET)
    bindings.append(binding);controller.set_editor_property('command_actions',bindings);save(controller_bp)

for name,kind in [('WBP_IC_InventoryItemPopup','InventoryItemPopup'),('WBP_IC_InventoryMenu','InventoryMenu'),('WBP_IC_CodexMenu','CodexMenu')]:
    bp=u.load_asset('/Game/IsekaiClass/UI/Widget/'+name)
    assert bp and u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind),(name,kind)
    u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp);REPORT['widgets'].append(name)
for name in ('WBP_IC_GameMenu','WBP_GameHUD'):
    bp=u.load_asset('/Game/IsekaiClass/UI/Widget/'+name)
    assert bp;u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)

doc=['NPC 스킬·무기·인벤토리 설정 안내', '2026-10-27', '',
     'NPC와 플레이어는 같은 SKL_NPC_R01_001~011 행을 사용한다.',
     '원본 SampleVFX는 보존하고 새 Niagara는 /Game/IsekaiClass/VFX/CombatEffect/Skill/NPC에 저장한다.', '',
     '스킬별 기본값 (범위는 cm, 시전·쿨타임은 실제 플레이 초)',
     '피해·회복 계산은 DT_IC_SkillEffect.Magnitude의 SourceStat × Coefficient + FlatAmount이다.',
     'R01_09는 10초 동안 2초마다 회복, R01_10은 12초 보호막이다.',
     'R01_08은 대상 중심 반경 600cm에 3회 피해와 여러 Niagara 레이어를 사용한다.', '']
for entry in REPORT['skills']:
    doc.append(f'{entry["id"]} | {entry["name"]} | 시전 {entry["cast"]} | 쿨타임 {entry["cooldown"]} | MP {entry["mana"]} | {entry["shape"]} {entry["range_or_radius"]} | 기본값 {entry["flat"]} + 계수 {entry["coefficient"]} | {entry["repeats"]}회')
    doc.append('  원본: '+entry['source'])
    doc.append('  새 본체: '+entry['main'])
    doc.append('  새 명중: '+entry['hit'])
doc.extend(['', '스킬 DT 편집',
    'DT_IC_Skill: MPCost, CastPlaySeconds, FirstEffectOffsetPlaySeconds, RecoveryPlaySeconds, CooldownPlaySeconds, Hits, Presentation.CastSound/ImpactSound.',
    'bUseDataTableTiming=True인 새 NPC 스킬은 DT 시각이 발동을 결정한다. 애니메이션 알림이 시전을 앞당기지 않는다.',
    'Hits: RadiusCm/Shape/MaximumTargets/RepeatCount/RepeatIntervalPlaySeconds/ProjectileSpeedCmPerSecond/ProjectileMaxDistanceCm.',
    'DT_IC_SkillEffect: Magnitude, DamageType, AppliedStatusEffectIDs.',
    'DT_IC_StatusEffect: 지속 회복·보호막의 DurationPlaySeconds, PeriodicMagnitude, InitialShieldMagnitude.',
    'DT_IC_CombatVFX NPC_R01_001~011: Cast/Projectile/Impact, Color/Intensity/Scale/Duration/Offset/Rotation, CastLayers/ImpactLayers.',
    'bImpactFollowsTarget=True이면 명중 이펙트가 피격 대상에 붙어 이동을 따라간다.',
    '색상은 현재 NPC 초상화의 머리색을 참고했다. 검은 머리는 효과가 보이도록 청회색 빛으로 보정했다.',
    '샘플 VFX 폴더에 사운드가 없어 짧은 합성 효과음 6종을 /Game/IsekaiClass/Audio/Combat에 추가했다.',
    '효과음 WAV 원본은 Data/Audio/Sources에 있으며 스킬 DT에서 다른 SoundWave/SoundCue로 바꿀 수 있다.', '',
    '무기 DT 편집',
    '초보자/시골 마을/단단한/물고기 비늘/무지개 공격력은 각각 10/20/30/40/50이다.',
    '요구 주능력치는 각각 0/10/20/30/40이다. 기존 초보 무기는 처음부터 장착할 수 있도록 0을 유지한다.',
    '검 STR, 활 DEX, 마법스태프 INT, 치유완드 CON. 장비 보너스를 제외한 영구 능력치를 검사한다.',
    'DT_IC_Equipment: BaseModifiers, RequiredMainStat, RequiredMainStatValue, WeaponMesh, WeaponVisualTransform, WeaponLengthAxis.',
    '새 메시의 손잡이와 크기는 DT의 bOverrideWeaponGeometry로 보정한다. 기존 초보 무기 BP 외형은 유지한다.',
    '바닥 드롭은 기존 기본 큐브 설정을 유지한다.', ''])
for entry in REPORT['weapons']:
    doc.append(f'{entry["item_id"]} | {entry["name"]} | 공격력 {entry["damage"]} | 요구 {entry["required_stat"]} {entry["minimum"]} | {entry["recipe_id"]}')
doc.extend(['', '제작 재료 (종류별 네 무기 공통)',
    '시골 마을: 광석1 2개 + 들꽃 줄기 2개.',
    '단단한: 광석1 1개 + 광석2 2개 + 광석3 2개.',
    '물고기 비늘: 물고기 종류를 섞어 합계 10개 + 광석4 4개.',
    '무지개: 물고기10 1개 + 광석6 2개 + 달걀 1개 + 해바라기 1개 + 분홍꽃 1개.',
    'DT_IC_Recipe.IngredientGroups로 임의 재료 묶음을 설정한다. 시작 시 실제 종류별 수량을 예약하므로 중복 차감하지 않는다.', '',
    '새 치트',
    'ICRecruitTestParty: NPC_R01_001/002/003의 호감도를 300으로 설정하고 파티 1/2/3번에 바로 편성한다.',
    'ICEquipStarterWeapons: 초보 무기 4종을 지급하고 각 무기 슬롯에 장착한다. 가방 빈칸 4개가 필요하다.',
    '기존 ICGiveStarterWeapons는 가방 지급만 하는 명령으로 유지한다.', '',
    'UI와 조작',
    'Tab: 화면 전방 몬스터 락온/다음 대상. 마지막 대상 다음은 해제, 다시 누르면 첫 대상으로 돌아간다.',
    '락온 중 카메라와 원거리 발사는 지정 몬스터를 향한다. 사망·거리·가림 조건은 기존 전투 설정 DT를 사용한다.',
    '인벤토리 우측에 보유 골드를 표시한다. 아이템 상세의 사용을 누르면 플레이어와 현재 동료를 선택한다.',
    '회복이 필요 없거나 DT가 허용하지 않는 대상이면 사용하지 않고 이유를 표시한다.',
    '퀵슬롯 클릭/Enter로 아이템 상세를 열고, Delete/Backspace로 등록을 해제한다.',
    '퀵슬롯 등록 중 1~4로 슬롯을 선택할 수 있다. 같은 아이템을 여러 슬롯에 등록할 수 있고 해제는 선택한 슬롯에만 적용된다.',
    '도감 NPC 분류에서 초상화·현재 호감도·동료 편입 상태·고유 스킬을 확인한다.', '',
    '주의: 이 스크립트는 초기 설정 도구다. 이후 밸런스를 편집한 뒤 전체 재실행하면 이 기본값이 다시 적용된다.'])
destination=ROOT/'Docs/Combat/NPC_스킬_무기_설정_안내.txt'
destination.write_text('\n'.join(doc)+'\n',encoding='utf-8')
(OUT/'Setup.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('NPC_SKILL_WEAPON_UI_SETUP_OK '+str(OUT/'Setup.json'))

