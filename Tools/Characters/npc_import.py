"""MCP-callable, resumable single-NPC asset generation in Unreal Python.

Importing this module does not register listeners or generate/replace assets.
An MCP worker explicitly calls build(config). World registration is opt-in and requires
the authored world references; otherwise registration_draft.json is emitted.
"""
import csv
import importlib.util
import io
import json
import re
import traceback
from pathlib import Path

import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
SAVED = ROOT / 'Saved' / 'NPCImport'
GAME = '/Game/IsekaiClass'
FAMILIES = {'Warrior': 'Sword', 'Archer': 'Bow', 'Mage': 'Staff', 'Healer': 'Wand'}
TABLES = {'npc': 'NPC/DT_IC_NPC', 'combat': 'Character/DT_IC_RecruitCombat',
          'resource': 'Presentation/DT_IC_ResourceReference', 'registry': 'Registry/DT_IC_IDRegistry',
          'region': 'World/DT_IC_Region'}
_helpers = None


def _read_json(path, default):
    return json.loads(path.read_text(encoding='utf-8-sig')) if path.exists() else default


def _write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')
    temporary.replace(path)


def _csv(name):
    path = ROOT / 'Data' / (name + '.csv')
    with path.open(encoding='utf-8-sig', newline='') as stream:
        reader = csv.DictReader(stream)
        return list(reader.fieldnames), list(reader)


def _csv_text(fields, rows):
    stream = io.StringIO(newline='')
    writer = csv.DictWriter(stream, fields, lineterminator='\n')
    writer.writeheader(); writer.writerows(rows)
    return stream.getvalue()


def _object_path(value):
    return value.get_path_name() if hasattr(value, 'get_path_name') else value + '.' + value.rsplit('/', 1)[-1]


def _load(path):
    result = u.load_asset(path)
    if not result:
        raise RuntimeError('에셋을 찾을 수 없습니다: ' + path)
    return result


def _save(asset):
    if not u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('저장 실패: ' + asset.get_path_name())


def _setup_helpers():
    global _helpers
    if _helpers is None:
        spec = importlib.util.spec_from_file_location('ic_character_assets', ROOT / 'Tools/Characters/setup_assets.py')
        _helpers = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(_helpers)
    return _helpers


def resolve_vrm(selected):
    """Resolve only the original VRM mesh, never its humanoid/mannequin copies."""
    if isinstance(selected, u.VrmAssetListObject):
        result = selected
    elif isinstance(selected, u.SkeletalMesh):
        folder = selected.get_path_name().rsplit('/', 1)[0]
        candidates = []
        for path in u.EditorAssetLibrary.list_assets(folder, recursive=False):
            data = u.EditorAssetLibrary.find_asset_data(path)
            if str(data.asset_class_path.asset_name) != 'VrmAssetListObject':
                continue
            candidate = data.get_asset()
            if candidate.get_editor_property('skeletal_mesh') == selected:
                candidates.append(candidate)
        if len(candidates) != 1:
            raise RuntimeError('원본 VRM AssetList 또는 그 원본 Skeletal Mesh를 하나 선택하세요.')
        result = candidates[0]
    else:
        raise RuntimeError('VRM AssetList 또는 원본 Skeletal Mesh를 선택하세요.')
    mesh = result.get_editor_property('skeletal_mesh')
    meta = result.get_editor_property('vrm_meta_object')
    if not mesh or not meta or not mesh.get_path_name().startswith('/Game/'):
        raise RuntimeError('프로젝트 Content에 임포트한 VRM 메시와 메타데이터가 필요합니다.')
    return result, mesh, meta


def suggested_id(region='R01'):
    used = {row['ID'].upper() for row in _csv(TABLES['registry'])[1]}
    used.update(path.parent.name.upper() for path in SAVED.glob('NPC_*/report.json'))
    for number in range(1, 10000):
        value = f'NPC_{region}_{number:03}'
        if value not in used:
            return value
    raise RuntimeError('사용 가능한 NPC ID가 없습니다.')


def inspect_vrm(path):
    """Read-only discovery result suitable for an MCP tool's JSON response."""
    asset_list, mesh, meta = resolve_vrm(_load(path))
    return {'vrm': asset_list.get_path_name(), 'mesh': mesh.get_path_name(), 'metadata': meta.get_path_name(),
            'suggested_id': suggested_id(), 'roles': list(FAMILIES),
            'physics_plan': u.ICCharacterSetupLibrary.configure_imported_npc_physics(mesh, meta, False)}


def plan(config):
    """Validate the selected model and proposed identity without creating/saving assets."""
    _, mesh, meta = resolve_vrm(_load(config['vrm']))
    physics = _validate(config, mesh, meta)
    identifier = config['id']
    return {'id': identifier, 'mesh': mesh.get_path_name(), 'physics': physics,
            'animation_folder': GAME+'/Animation/NPC/'+identifier,
            'blueprint': GAME+'/NPC/Blueprints/Generated/BP_'+identifier,
            'register_world': bool(config.get('register_world'))}


def _validate(config, mesh, meta):
    identifier = config['id']
    if not re.fullmatch(r'NPC_[A-Z0-9]+_[A-Z0-9_]+', identifier):
        raise RuntimeError('NPC ID는 NPC_R01_012처럼 영문 대문자·숫자·밑줄로 입력하세요.')
    if config['role'] not in FAMILIES:
        raise RuntimeError('직업은 Warrior, Archer, Mage, Healer 중 하나여야 합니다.')
    if not config.get('name', '').strip():
        raise RuntimeError('표시 이름을 입력하세요.')
    _, registry = _csv(TABLES['registry'])
    keys = {identifier, 'RCB_' + identifier, 'RES_' + identifier}
    if any(row['ID'].upper() in keys for row in registry):
        # A completed generation is handled before validation; an unrelated
        # existing ID must never be repurposed for a new imported appearance.
        raise RuntimeError('이미 등록된 ID입니다. 새로운 NPC ID를 사용하세요: ' + identifier)
    for key in ('npc', 'combat', 'resource'):
        if any(row['ID'].upper() in keys for row in _csv(TABLES[key])[1]):
            raise RuntimeError('기존 데이터와 ID가 겹칩니다: ' + identifier)
    physics = u.ICCharacterSetupLibrary.configure_imported_npc_physics(mesh, meta, False)
    if physics.startswith('ERROR:'):
        raise RuntimeError(physics)
    if config.get('register_world'):
        for field, table in [('region', TABLES['region']), ('anchor', 'World/DT_IC_Anchor'),
                             ('home', 'World/DT_IC_Home'), ('schedule', 'NPC/DT_IC_Schedule'),
                             ('dialogue', 'NPC/DT_IC_Dialogue'), ('unique_skill', 'Combat/DT_IC_Skill')]:
            value = config.get(field, '')
            rows = {row['ID']: row for row in _csv(table)[1]}
            if not value or value == 'None' or value not in rows:
                raise RuntimeError(f'월드 등록에는 작성된 {field} ID가 필요합니다: {value}')
            row = rows[value]
            if field == 'home' and row.get('OwnerNPCID') != identifier:
                raise RuntimeError('집의 OwnerNPCID가 새 NPC ID와 일치해야 합니다.')
            if field == 'dialogue' and row.get('NPCID') != identifier:
                raise RuntimeError('대화의 NPCID가 새 NPC ID와 일치해야 합니다.')
            if field == 'unique_skill' and (row.get('NPCID') != identifier or row.get('OwnerType') != 'RecruitUnique'):
                raise RuntimeError('고유 기술의 소유자가 새 NPC와 일치해야 합니다.')
        if not config.get('portrait') or not isinstance(_load(config['portrait']), u.Texture2D):
            raise RuntimeError('월드 등록에는 이 NPC의 초상화가 필요합니다.')
    return physics


def _target_rig(mesh, folder):
    helper = _setup_helpers()
    source_folder = mesh.get_path_name().rsplit('/', 1)[0]
    matches = []
    for path in u.EditorAssetLibrary.list_assets(source_folder, recursive=False):
        data = u.EditorAssetLibrary.find_asset_data(path)
        if str(data.asset_class_path.asset_name) != 'IKRigDefinition':
            continue
        rig = data.get_asset()
        if 'Mannequin' in rig.get_name() and u.IKRigController.get_controller(rig).get_skeletal_mesh() == mesh:
            matches.append(rig)
    if len(matches) != 1:
        raise RuntimeError('VRM4U 임포트 시 원본 메시용 Mannequin IK Rig를 생성해야 합니다. 후보: ' + str(len(matches)))
    path = folder + '/IK_Target'
    rig = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(matches[0].get_path_name(), path)
    if not rig:
        raise RuntimeError('대상 IK Rig 복제에 실패했습니다.')
    _save(rig)
    return rig


def _make_assets(config, mesh, meta, report):
    helper = _setup_helpers()
    identifier = config['id']; family = FAMILIES[config['role']]
    folder = GAME + '/Animation/NPC/' + identifier
    bp_path = GAME + '/NPC/Blueprints/Generated/BP_' + identifier
    prepared = u.ICCharacterSetupLibrary.prepare_imported_npc_mesh(mesh, meta)
    if not prepared:
        raise RuntimeError('VRM humanoid 메타데이터의 좌우 손 뼈를 확인하세요.')
    _save(mesh)
    rig = _target_rig(mesh, folder)
    pack, source = helper.source_sets()['Staff' if family == 'Wand' else family]
    clips = helper.retarget(pack, source, mesh, rig, folder + '/' + pack)
    for clip in dict.fromkeys(clips.values()):
        clip.set_editor_property('force_root_lock', True)
        clip.set_editor_property('root_motion_root_lock', u.RootMotionRootLock.REF_POSE)
        _save(clip)
    blend = u.ICCharacterSetupLibrary.create_locomotion(folder+'/BS_'+family, clips['idle'], clips['walk'], clips['run'])
    if not blend:
        raise RuntimeError('걷기·달리기 BlendSpace 생성 실패')
    _save(blend)
    actions = {}
    for action in ('attack', 'skill', 'hit'):
        clip = clips['heal'] if action == 'skill' and family == 'Wand' else clips[action]
        actions[action] = u.ICCharacterSetupLibrary.create_action_montage(folder+'/AM_'+family+'_'+action, clip, action != 'hit')
        if not actions[action]:
            raise RuntimeError('Montage 생성 실패: ' + action)
        _save(actions[action])
    factory = u.DataAssetFactory()
    factory.set_editor_property('data_asset_class', u.ICCharacterAnimationProfile)
    profile = helper.create(folder+'/DA_ANIM_'+identifier, u.ICCharacterAnimationProfile, factory)
    family_key = getattr(u.ICWeaponFamily, family.upper())
    profile.set_editor_property('weapon_sets', {family_key: u.ICCharacterAnimationSet(
        locomotion=blend, jump=clips['jump'], attack=actions['attack'], skill=actions['skill'], hit=actions['hit'])})
    profile.set_editor_property('default_family', family_key)
    # Reuse the current common animation contract; no bed assets or sleep
    # calibration are edited by this importer.
    sleep = helper.retarget('Sleep_Animation', {'sleep': helper.sequence('Sleep_Animation', 'anim_Sleep_06_L')}, mesh, rig, folder+'/Sleep')['sleep']
    profile.set_editor_property('sleep', sleep)
    if not u.ICCharacterSetupLibrary.calibrate_imported_npc(mesh, profile):
        raise RuntimeError('발 높이 자동 보정에 실패했습니다. 메시 크기와 리타겟 자세를 확인하세요.')
    _save(profile)
    physics = u.ICCharacterSetupLibrary.configure_imported_npc_physics(mesh, meta, True)
    if physics.startswith('ERROR:'):
        raise RuntimeError(physics)
    parent = u.EditorAssetLibrary.load_blueprint_class(GAME+'/NPC/Blueprints/BP_ICNPCBase')
    bp = helper.blueprint(bp_path, parent)
    default = helper.cdo(bp)
    default.set_editor_property('npc_definition_id', identifier)
    default.set_editor_property('animation_profile', profile)
    component = default.get_editor_property('mesh')
    component.set_editor_property('skeletal_mesh', mesh)
    component.set_editor_property('relative_location', u.Vector(0, 0, -88))
    component.set_editor_property('relative_rotation', u.Rotator(pitch=0, yaw=-90, roll=0))
    component.set_editor_property('anim_class', u.ICCharacterAnimInstance)
    u.BlueprintEditorLibrary.compile_blueprint(bp); _save(bp)
    skeleton = mesh.get_editor_property('skeleton')
    checks = {'target_skeleton': all(clip.get_editor_property('skeleton') == skeleton for clip in clips.values()),
              'weapon_sockets_prepared': prepared,
              'kawaii': physics.startswith('OK:')}
    if not all(checks.values()):
        raise RuntimeError('생성 에셋의 정합성 검사 실패: ' + str(checks))
    report.update(profile=profile.get_path_name(), blueprint=bp.get_path_name(), physics=physics, checks=checks,
                  ground_offset_cm=profile.weapon_sets[family_key].ground_offset_cm,
                  initial_weapon='EQU_WPN_'+family.upper()+'_001')
    return profile, bp


def _definition(fields, identifier, **values):
    row = dict.fromkeys(fields, '')
    row.update({'---': identifier, 'ID': identifier, 'NameKey': identifier+'.NAME', 'Tags': '(GameplayTags=)',
                'DataVersion': '1', 'bDeprecated': 'False', 'bTestOnly': 'False'})
    row.update({key: str(value) for key, value in values.items()})
    return row


def registration_draft(config, mesh, report):
    identifier = config['id']; family = FAMILIES[config['role']]
    combat_id = 'RCB_' + identifier; resource_id = 'RES_' + identifier
    values = {'npc': dict(Type='Recruitable', Race='Human', RegionID=config.get('region', 'R01'),
                         HomeID=config.get('home') or 'None', InitialAnchorID=config.get('anchor') or 'None',
                         ScheduleID=config.get('schedule') or 'None', DialogueIDs='("'+config['dialogue']+'")' if config.get('dialogue') else '()',
                         GiftPreferenceID='None', ServiceIDs='()', RecruitCombatID=combat_id,
                         ClassID='CLS_'+config['role'].upper()+'_001', OccupationTag='None', bRecruitable='True', bEndingTarget='False',
                         bClassmate=str(bool(config.get('classmate'))), InitialAffinity=0, AppearanceResourceID=resource_id,
                         Mesh=mesh.get_path_name(), Portrait=config.get('portrait') or 'None', AnimationClass='/Script/IsekaiClass.ICCharacterAnimInstance',
                         AnimationProfile=report['profile'], ActorClass=report['blueprint']+'_C', ScheduleStateTree='None',
                         MeshTransform='(Rotation=(X=0,Y=0,Z=-0.707107,W=0.707107),Translation=(X=0,Y=0,Z=-88),Scale3D=(X=1,Y=1,Z=1))'),
              'combat': dict(NPCID=identifier, ClassID='CLS_'+config['role'].upper()+'_001', WeaponFamily=family,
                             Role={'Sword': 'Frontline', 'Bow': 'RangedDamage', 'Staff': 'MagicDamage', 'Wand': 'Healer'}[family],
                             InitialBaseStats='(STR=1,DEX=1,INT=1,CON=1,CHA=1,LUK=1)', InitialEquipment='(WeaponID='+report['initial_weapon']+')',
                             UniqueSkillID=config.get('unique_skill') or 'None', InitialAttackDisposition='Defensive', InitialLowHPDisposition='Wait',
                             InitialTargetPriority='PlayerTargetFirst', bInitiallyAutoSkillEnabled='False'),
              'resource': dict(Kind='SkeletalMesh', Asset=mesh.get_path_name(), UsedBy='((Type=NPC,ID='+identifier+'))',
                               Skeleton=mesh.get_editor_property('skeleton').get_path_name(), SkeletonGroup='VRM_AUTO',
                               RequiredSockets='("weapon_r","weapon_l")', RequiredNotifyNames='()', DimensionsCm='(X=0,Y=0,Z=0)',
                               ImportScale='(X=1,Y=1,Z=1)', AxisAndGripNotes='Automatic VRM setup; visual review required.',
                               bVerifiedInRepresentativeScene='False', bVerifiedForCooking='False')}
    ids = {'npc': identifier, 'combat': combat_id, 'resource': resource_id}
    rows = {key: _definition(_csv(TABLES[key])[0], ids[key], **value) for key, value in values.items()}
    registry_fields = _csv(TABLES['registry'])[0]
    rows['registry'] = [_definition(registry_fields, ids[key], DefinitionType=kind, RegionID=config.get('region', 'R01'),
                                   ReplacementID='None', DefinitionTable=_object_path(GAME+'/Data/DataTable/'+TABLES[key]),
                                   Notes='Generated from imported VRM. Visual review required.')
                        for key, kind in [('npc', 'NPC'), ('combat', 'RecruitCombat'), ('resource', 'ResourceReference')]]
    return rows


def _register_world(config, draft, report):
    """Append only new rows; retain exact source backups and restore on failure."""
    changes = {}
    for key in ('npc', 'combat', 'resource', 'registry', 'region'):
        name = TABLES[key]; fields, rows = _csv(name)
        additions = draft[key] if key == 'registry' else [draft[key]] if key != 'region' else []
        for new in additions:
            if any(row['ID'].casefold() == new['ID'].casefold() for row in rows):
                raise RuntimeError('등록 중 ID 충돌: ' + new['ID'])
        rows.extend(additions)
        if key == 'region':
            row = next(row for row in rows if row['ID'] == config['region'])
            members = re.findall(r'"([^"]+)"', row['NPCIDs'])
            if config['id'] not in members: members.append(config['id'])
            row['NPCIDs'] = '(' + ','.join('"'+member+'"' for member in members) + ')'
        changes[name] = _csv_text(fields, rows)
    string_name = 'StringTables/ST_IC_Definitions'; fields, strings = _csv(string_name)
    if any(row['Key'] == config['id']+'.NAME' for row in strings):
        raise RuntimeError('표시 이름 키가 이미 있습니다.')
    strings.append({'Key': config['id']+'.NAME', 'SourceString': config['name']})
    changes[string_name] = _csv_text(fields, strings)
    originals = {name: (ROOT/'Data'/(name+'.csv')).read_bytes() for name in changes}
    backup = SAVED/config['id']/'DataBeforeRegistration'
    try:
        for name, text in changes.items():
            path = ROOT/'Data'/(name+'.csv'); saved = backup/(name+'.csv')
            saved.parent.mkdir(parents=True, exist_ok=True)
            if not saved.exists(): saved.write_bytes(originals[name])
            path.write_text(text, encoding='utf-8')
        for name, text in changes.items():
            table = _load(GAME+'/Data/DataTable/'+name)
            if name.startswith('StringTables/'):
                good = u.ICBlueprintMaintenanceLibrary.import_ui_strings(table, str(ROOT/'Data'/(name+'.csv')))
            else:
                good = u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, text)
            if not good: raise RuntimeError('데이터 테이블 반영 실패: ' + name)
            _save(table)
    except Exception:
        for name, data in originals.items():
            path = ROOT/'Data'/(name+'.csv'); path.write_bytes(data)
            table = _load(GAME+'/Data/DataTable/'+name)
            if name.startswith('StringTables/'):
                u.ICBlueprintMaintenanceLibrary.import_ui_strings(table, str(path))
            else:
                u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, data.decode('utf-8-sig'))
            _save(table)
        raise
    report['world_registered'] = True


def build(config):
    """Generate one configured NPC. Completed repeats are read-only."""
    config = dict(config)
    defaults = {'classmate': False, 'register_world': False, 'region': 'R01', 'anchor': '', 'home': '',
                'schedule': '', 'dialogue': '', 'unique_skill': '', 'portrait': ''}
    for key, value in defaults.items(): config.setdefault(key, value)
    for field in ('region', 'anchor', 'home', 'schedule', 'dialogue', 'unique_skill'):
        if config.get(field) == 'None': config[field] = ''
    identifier = config.get('id', '')
    if not re.fullmatch(r'NPC_[A-Z0-9]+_[A-Z0-9_]+', identifier):
        raise RuntimeError('올바른 NPC ID를 입력하세요.')
    report_path = SAVED/identifier/'report.json'
    previous = _read_json(report_path, {})
    previous_config = dict(previous.get('config', {}))
    for key, value in defaults.items(): previous_config.setdefault(key, value)
    if previous and previous_config != config:
        if not previous.get('complete') or previous.get('world_registered') or any(previous_config.get(key) != config.get(key) for key in ('vrm', 'id', 'role')):
            raise RuntimeError('이 ID로 다른 생성 작업이 있습니다. 새 ID를 사용하거나 기존 보고서를 확인하세요.')
    if previous.get('complete'):
        if previous_config != config:
            _, mesh, meta = resolve_vrm(_load(config['vrm']))
            _validate(config, mesh, meta)
            draft = registration_draft(config, mesh, previous)
            _write_json(report_path.parent/'registration_draft.json', draft)
            if config['register_world']: _register_world(config, draft, previous)
            previous['config'] = config
            _write_json(report_path, previous)
        return previous
    asset_list, mesh, meta = resolve_vrm(_load(config['vrm']))
    plan = _validate(config, mesh, meta)
    helper = _setup_helpers()
    output = GAME+'/Animation/NPC/'+identifier
    bp_path = GAME+'/NPC/Blueprints/Generated/BP_'+identifier
    if not previous and (u.EditorAssetLibrary.does_directory_exist(output) or u.EditorAssetLibrary.does_asset_exist(bp_path)):
        raise RuntimeError('같은 ID의 에셋 폴더가 이미 있습니다. 기존 에셋을 덮어쓰지 않습니다.')
    report = {'config': config, 'mesh': mesh.get_path_name(), 'plan': plan, 'complete': False, 'world_registered': False,
              'review_required': ['걷기·달리기에서 머리와 옷의 충돌', '무기 그립과 발 접지', '캐릭터별 물리 세부 튜닝']}
    _write_json(report_path, report)
    try:
        _make_assets(config, mesh, meta, report)
        draft = registration_draft(config, mesh, report)
        _write_json(report_path.parent/'registration_draft.json', draft)
        if config.get('register_world'):
            _register_world(config, draft, report)
        report['complete'] = True
        index = _read_json(SAVED/'index.json', {})
        index[asset_list.get_path_name()] = identifier
        _write_json(SAVED/'index.json', index)
    except Exception:
        report['error'] = traceback.format_exc()
        _write_json(report_path, report)
        raise
    _write_json(report_path, report)
    u.log('IC_NPC_IMPORT_COMPLETE ' + str(report_path))
    return report

