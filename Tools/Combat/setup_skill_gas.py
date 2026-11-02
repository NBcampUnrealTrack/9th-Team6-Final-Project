"""Run with UnrealEditor-Cmd after compiling C++ to author skill GAS assets and DT links."""
import csv
import io
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
csv_path = root / 'Data/Combat/DT_IC_Skill.csv'
with csv_path.open('r', encoding='utf-8-sig', newline='') as source:
    reader = csv.DictReader(source)
    rows = list(reader)
    fields = list(reader.fieldnames)
for field in ('AbilityClass', 'CooldownEffectClass', 'CooldownTag'):
    if field not in fields:
        fields.append(field)

tools = u.AssetToolsHelpers.get_asset_tools()
library = u.EditorAssetLibrary


def blueprint(path, parent):
    if library.does_asset_exist(path):
        return u.load_asset(path)
    folder, name = path.rsplit('/', 1)
    factory = u.BlueprintFactory()
    factory.set_editor_property('parent_class', parent)
    result = tools.create_asset(name, folder, u.Blueprint, factory)
    if not result:
        raise RuntimeError('Could not create ' + path)
    return result


def default(bp):
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    return u.get_default_object(bp.generated_class())


def asset(path):
    result = u.load_asset(path)
    if not result:
        raise RuntimeError('Missing ' + path)
    return result


def gameplay_tag(name):
    result = u.GameplayTag()
    if not result.import_text(name):
        raise RuntimeError('Unknown gameplay tag: ' + name)
    return result


for row in rows:
    skill_id = row['ID']
    for field in ('UnlockConditions', 'CancelRules', 'AutoRules', 'Presentation'):
        if row[field] in ('', 'None'):
            row[field] = '()'
    tag = 'IC.Cooldown.' + skill_id
    ability_path = '/Game/IsekaiClass/Combat/GAS/Abilities/GA_' + skill_id
    effect_path = '/Game/IsekaiClass/Combat/GAS/Cooldowns/GE_CD_' + skill_id
    ability = blueprint(ability_path, u.ICSkillGameplayAbility)
    effect = blueprint(effect_path, u.ICSkillCooldownEffect)
    ability_default = default(ability)
    ability_default.set_editor_property('skill_id', skill_id)
    ability_default.set_editor_property('cooldown_tag', gameplay_tag(tag))
    library.save_loaded_asset(ability, only_if_is_dirty=False)
    library.save_loaded_asset(effect, only_if_is_dirty=False)
    row['AbilityClass'] = ability_path + '.GA_' + skill_id + '_C'
    row['CooldownEffectClass'] = effect_path + '.GE_CD_' + skill_id + '_C'
    row['CooldownTag'] = tag


cue_dir = '/Game/IsekaiClass/Combat/GAS/Cues'
sample_vfx = '/Game/IsekaiClass/VFX/SampleVFX/Free/Free_Spells/VFX_Niagara/'
sample_audio = '/Game/IsekaiClass/Audio/SampleAudio/Strategy_Game_Sound_Pack/wav/Strategy_Game/'
for name, tag, vfx, sound in (
    ('GC_IC_Combat_Cast', 'GameplayCue.IC.Combat.Cast', 'NS_Free_Spells_Circle', 'Combat_Weapon_Swing-001'),
    ('GC_IC_Combat_Impact', 'GameplayCue.IC.Combat.Impact', 'NS_Free_Spells_Shockwave', 'Combat_Punch'),
):
    cue = blueprint(cue_dir + '/' + name, u.ICCombatCueNotify)
    cue_default = default(cue)
    cue_default.set_editor_property('gameplay_cue_tag', gameplay_tag(tag))
    cue_default.set_editor_property('default_vfx', asset(sample_vfx + vfx))
    cue_default.set_editor_property('default_sound', asset(sample_audio + sound))
    library.save_loaded_asset(cue, only_if_is_dirty=False)


buffer = io.StringIO(newline='')
writer = csv.DictWriter(buffer, fieldnames=fields, lineterminator='\n')
writer.writeheader()
writer.writerows(rows)
csv_path.write_text(buffer.getvalue(), encoding='utf-8', newline='')
table = asset('/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Skill')
if not u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, buffer.getvalue()):
    raise RuntimeError('DT_IC_Skill import failed')
library.save_loaded_asset(table, only_if_is_dirty=False)
print('IC_SKILL_GAS_SETUP_OK skills=' + str(len(rows)) + ' cues=2')
