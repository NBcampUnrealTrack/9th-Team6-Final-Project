"""Author the four open-world Warp Stones and capture their real level overview.

Run after building the editor with -ExecutePythonScript=<this file> -RenderOffscreen.
Existing map edits are retained; changed files are backed up before authoring.
"""
import csv
import hashlib
import json
import math
import shutil
import time
import traceback
from pathlib import Path
import unreal as u
u.EditorPythonScripting.set_keep_python_script_alive(True)

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / 'Saved/OpenWorldUpgrade'
OUT.mkdir(parents=True, exist_ok=True)
BACKUP = OUT / 'Before'
GAME = '/Game/IsekaiClass'
TABLE_ROOT = GAME + '/Data/DataTable/'
ACTORS = u.get_editor_subsystem(u.EditorActorSubsystem)
TABLES = {}
REPORT = {'destinations': [], 'checks': {}, 'capture': {}, 'complete': False}
POINTS = [
    ('01_HOME', '집', 53740., 629630., -5155., True),
    ('02_INN', '마을여관', 55695., 608350., 6125., False),
    ('03_BRIDGE', '추억의 다리', -60715., 561595., -6495., False),
    ('04_RUINS', '폐허 마을', -52830., 505280., -6430., False),
]


def backup_file(relative):
    src, dest = ROOT / relative, BACKUP / relative
    if src.exists() and not dest.exists():
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)


def backup_asset(path):
    backup_file('Content/' + path.removeprefix('/Game/') + ('.umap' if path.endswith('L_OpenWorld') else '.uasset'))


def save(asset):
    assert u.EditorAssetLibrary.save_loaded_asset(asset, False), asset.get_path_name()


def table(relative):
    if relative not in TABLES:
        asset = u.load_asset(TABLE_ROOT + relative)
        assert asset, relative
        backup_asset(TABLE_ROOT + relative)
        TABLES[relative] = (asset, {r['Name']: r for r in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(asset))})
    return TABLES[relative][1]


def definition(identifier):
    return dict(Name=identifier, ID=identifier, NameKey=identifier + '.NAME', DescriptionKey='', DataVersion=1, bDeprecated=False, bTestOnly=False)


def register(identifier, kind, relative):
    table('Registry/DT_IC_IDRegistry')[identifier] = dict(
        definition(identifier), DefinitionType=kind, RegionID='R01',
        DefinitionTable=TABLE_ROOT + relative + '.' + relative.rsplit('/', 1)[-1],
        Notes='Open-world four-destination travel')


def floor_at(x, y, expected_z, ignore=()):
    # Start near the authored point to avoid selecting a distant rooftop/cliff.
    # The inn's supplied Z is positive; the long trace finds its actual ground.
    hit = u.SystemLibrary.line_trace_single(
        WORLD, u.Vector(x, y, expected_z + 500), u.Vector(x, y, -50000),
        u.TraceTypeQuery.ECC_VISIBILITY, True, list(ignore), u.DrawDebugTrace.NONE, True)
    if not hit:
        hit = u.SystemLibrary.line_trace_single(
            WORLD, u.Vector(x, y, 120000), u.Vector(x, y, -50000),
            u.TraceTypeQuery.ECC_VISIBILITY, True, list(ignore), u.DrawDebugTrace.NONE, True)
    if not hit:
        raise RuntimeError(f'No ground at ({x}, {y}, {expected_z})')
    data = hit.to_dict()
    normal = data.get('impact_normal', u.Vector(0, 0, 1))
    if normal.z < .65:
        raise RuntimeError(f'Unsafe slope at ({x}, {y})')
    return data.get('impact_point', data['location'])


def transform_text(p):
    return f'(Rotation=(X=0,Y=0,Z=0,W=1),Translation=(X={p.x},Y={p.y},Z={p.z}),Scale3D=(X=1,Y=1,Z=1))'


def author():
    global WORLD
    backup_asset(GAME + '/World/Maps/L_OpenWorld')
    WORLD = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    if WORLD.get_name()!='L_OpenWorld':
        u.EditorLoadingAndSavingUtils.load_map(GAME + '/World/Maps/L_OpenWorld')
        WORLD = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    existing = ACTORS.get_all_level_actors()
    stones = {str(a.get_editor_property('WarpID')): a for a in existing if isinstance(a, u.ICWarpStone)}
    anchors = {str(a.get_editor_property('AnchorID')): a for a in existing if isinstance(a, u.ICWorldAnchor)}
    portal_bp = u.load_asset(GAME + '/World/Blueprints/BP_ICWarpStone')
    anchor_bp = u.load_asset(GAME + '/World/Blueprints/BP_ICWorldAnchor')
    assert portal_bp and anchor_bp
    warps, anchor_rows = table('World/DT_IC_Warp'), table('World/DT_IC_Anchor')
    warp_ids, anchor_ids = [], []
    for code, label, x, y, z, default_open in POINTS:
        wid, visit, arrival = 'WRP_OW_' + code, 'ANC_WRP_OW_VISIT_' + code, 'ANC_WRP_OW_ARRIVAL_' + code
        old_stone = stones.get(wid)
        ground = floor_at(x, y, z, [old_stone] if old_stone else [])
        stone_pos = u.Vector(x, y, ground.z + 3)
        # Spawn the landing point beside the stone, not inside its blocking mesh.
        landing = None
        for dx, dy in ((230, 0), (-230, 0), (0, 230), (0, -230)):
            try:
                p = floor_at(x + dx, y + dy, ground.z + 150, [old_stone] if old_stone else [])
                if abs(p.z - ground.z) < 160:
                    landing = u.Vector(p.x, p.y, p.z + 100)
                    break
            except RuntimeError:
                pass
        assert landing is not None, 'No safe landing next to ' + label
        a = old_stone or ACTORS.spawn_actor_from_class(portal_bp.generated_class(), stone_pos)
        a.set_actor_label('Warp_OW_' + code + '_' + label)
        a.set_actor_location(stone_pos, False, True)
        a.set_editor_property('WarpID', wid)
        a.set_editor_property('CostGold', 500)
        a.set_editor_property('InteractionText', label)
        a.get_editor_property('Label').set_text(label + ' · 워프돌')
        # A stone should remain visible above sloping terrain.
        center, extent = a.get_actor_bounds(False)
        a.add_actor_world_offset(u.Vector(0, 0, max(0., ground.z + 3 - (center.z - extent.z))), False, True)
        stone_pos = a.get_actor_location()
        warps[wid] = dict(definition(wid), RegionID='R01', ServiceID='None', VisitAnchorID=visit,
                          ArrivalAnchorID=arrival, VisitRadiusCm=400, bUnlockedByDefault=default_open)
        register(wid, 'Warp', 'World/DT_IC_Warp')
        warp_ids.append(wid)
        for aid, pos, safe in ((visit, stone_pos, False), (arrival, landing, True)):
            anchor_rows[aid] = dict(definition(aid), RegionID='R01', Type='WarpArrival' if safe else 'Transit',
                                   WorldTransform=transform_text(pos), bSafeForArrival=safe,
                                   ClearanceRadiusCm=50, ClearanceHalfHeightCm=96)
            register(aid, 'Anchor', 'World/DT_IC_Anchor')
            anchor_ids.append(aid)
            anchor = anchors.get(aid) or ACTORS.spawn_actor_from_class(anchor_bp.generated_class(), pos)
            anchor.set_actor_label(aid)
            anchor.set_editor_property('AnchorID', aid)
            anchor.set_actor_location(pos, False, True)
        REPORT['destinations'].append(dict(id=wid, name=label, requested=[x, y, z],
            stone=[stone_pos.x, stone_pos.y, stone_pos.z], ground_z=ground.z,
            arrival=[landing.x, landing.y, landing.z], unlocked_by_default=default_open))

    landscapes = [a for a in existing if isinstance(a, u.Landscape)]
    bounds = [a.get_actor_bounds(False) for a in landscapes]
    min_x = min(c.x-e.x for c,e in bounds); max_x = max(c.x+e.x for c,e in bounds)
    min_y = min(c.y-e.y for c,e in bounds); max_y = max(c.y+e.y for c,e in bounds)
    size = max(max_x-min_x, max_y-min_y)
    cx, cy = (min_x+max_x)/2, (min_y+max_y)/2
    min_x, max_x, min_y, max_y = cx-size/2, cx+size/2, cy-size/2, cy+size/2
    texture = GAME + '/UI/Map/T_OpenWorld_Overview.T_OpenWorld_Overview'
    mid = 'MAP_OPENWORLD_001'
    table('World/DT_IC_Map')[mid] = dict(definition(mid), Type='Town', RegionID='R01',
        VisibleMarkers=['Player', 'Shop', 'Warp', 'Quest'], MinimumZoom=1, MaximumZoom=8,
        Layers=[dict(InteriorFloorID='None', DisplayNameKey=mid+'.NAME', MapTexture=texture,
            WorldOriginCm=f'(X={max_x},Y={min_y})', WorldAxisU='(X=0,Y=1)', WorldAxisV='(X=-1,Y=0)',
            MapOriginUV='(X=0,Y=0)', WorldCmPerUV=f'(X={size},Y={size})',
            WorldBoundsMinCm=f'(X={min_x},Y={min_y})', WorldBoundsMaxCm=f'(X={max_x},Y={max_y})')])
    register(mid, 'Map', 'World/DT_IC_Map')
    table('Settings/DT_IC_MapPresentation')['Default'].update(MapID=mid, bUnlockPlacedPortals=False)
    region = table('World/DT_IC_Region')['R01']
    region['WarpIDs'] = list(dict.fromkeys(region.get('WarpIDs', []) + warp_ids))
    region['AnchorIDs'] = list(dict.fromkeys(region.get('AnchorIDs', []) + anchor_ids))
    region['TownMapID'] = mid
    # Preserve old test-world authoring except the live map's travel projection.
    for relative, (asset, rows) in TABLES.items():
        assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(asset, json.dumps(list(rows.values()), ensure_ascii=False)), relative
        save(asset)
        csv_path = ROOT / 'Data' / (relative + '.csv')
        backup_file(str(csv_path.relative_to(ROOT)))
        csv_path.parent.mkdir(parents=True, exist_ok=True)
        csv_path.write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset), encoding='utf-8-sig')

    # Add localization keys without replacing the team's existing strings.
    strings_path = ROOT / 'Data/StringTables/ST_IC_Definitions.csv'
    backup_file(str(strings_path.relative_to(ROOT)))
    strings = {r['Key']:r['SourceString'] for r in csv.DictReader(strings_path.open(encoding='utf-8-sig'))}
    strings[mid+'.NAME'] = '오픈월드'
    for d in REPORT['destinations']:
        strings[d['id']+'.NAME'] = d['name']
    st = u.load_asset(TABLE_ROOT + 'StringTables/ST_IC_Definitions')
    backup_asset(TABLE_ROOT + 'StringTables/ST_IC_Definitions')
    with strings_path.open('w', encoding='utf-8-sig', newline='') as stream:
        writer=csv.writer(stream);writer.writerow(['Key','SourceString']);writer.writerows(strings.items())
    # Existing optional description entries intentionally contain empty strings.
    assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(st, str(strings_path))
    save(st)

    classes=table('Character/DT_IC_Class')
    family_audio={'Sword':'S_IC_Slash','Bow':'S_IC_Arrow','Staff':'S_IC_Magic','Wand':'S_IC_Healing'}
    for row in classes.values():
        sound=family_audio.get(row.get('WeaponFamily'))
        if sound:
            for stage in row.get('NormalAttackStages',[]):
                stage['AttackSound']=GAME+'/Audio/Combat/'+sound+'.'+sound
                stage['ImpactSound']=GAME+'/Audio/Combat/S_IC_Impact.S_IC_Impact'
    asset, rows=TABLES['Character/DT_IC_Class']
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(asset,json.dumps(list(rows.values()),ensure_ascii=False));save(asset)
    backup_file('Data/Character/DT_IC_Class.csv')
    (ROOT/'Data/Character/DT_IC_Class.csv').write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset),encoding='utf-8-sig')

    widgets=[('WBP_IC_InventoryMenu','InventoryMenu'),('WBP_IC_Map','Map'),('WBP_IC_Minimap','Minimap'),('WBP_GameHUD','HUD')]
    for name, kind in widgets:
        path=GAME+'/UI/Widget/'+name;backup_asset(path);bp=u.load_asset(path);assert bp
        assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind)
        if kind=='HUD':assert u.ICBlueprintMaintenanceLibrary.prepare_managed_widget(bp,'HUD')
        u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
        names={str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)}
        if kind=='HUD':assert {'HUD_BossBar','BAR_BossHP','HUD_ControlGuide','TXT_GuideBody','HUD_AimReticle','TXT_GameClock'}<=names
        if kind=='InventoryMenu':assert 'InventorySearch' not in names
    # Widget thumbnail rendering temporarily changes GWorld; save the explicit
    # editor world rather than whichever preview world is currently global.
    assert u.EditorLoadingAndSavingUtils.save_map(WORLD,GAME+'/World/Maps/L_OpenWorld')
    REPORT['checks'].update(map_saved_with_existing_edits=True, only_home_default_open=True, inventory_search_removed=True)
    REPORT['capture'].update(width=4096,height=4096,center=[cx,cy],world_width_cm=size,
        world_bounds=[min_x,min_y,max_x,max_y],texture=texture)
    capture_actor=ACTORS.spawn_actor_from_class(u.SceneCapture2D,u.Vector(cx,cy,180000),u.Rotator(pitch=-90,yaw=0),True)
    capture=capture_actor.get_component_by_class(u.SceneCaptureComponent2D)
    target=u.RenderingLibrary.create_render_target2d(WORLD,4096,4096,u.TextureRenderTargetFormat.RTF_RGBA8_SRGB,u.LinearColor(0,0,0,1))
    capture.set_editor_property('texture_target',target)
    capture.set_editor_property('projection_type',u.CameraProjectionMode.ORTHOGRAPHIC)
    capture.set_editor_property('ortho_width',size)
    capture.set_editor_property('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    capture.set_editor_property('capture_every_frame',False)
    capture.set_editor_property('capture_on_movement',False)
    return capture_actor,capture,target


STATE={'start':time.monotonic(),'phase':'author'}


def finish(error=None):
    if error:REPORT['error']=error
    (OUT/'Authored.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(STATE['callback'])
    u.log('IC_OPENWORLD_UPGRADE_COMPLETE' if REPORT['complete'] else 'IC_OPENWORLD_UPGRADE_FAILED')
    u.SystemLibrary.quit_editor()


def tick(delta):
    if STATE.get('busy'):return
    STATE['busy']=True
    try:
        if time.monotonic()-STATE['start']>420:raise RuntimeError('Open-world authoring timeout: '+STATE['phase'])
        if STATE['phase']=='author':
            STATE['actor'],STATE['capture'],STATE['target']=author()
            STATE.update(phase='warm',since=time.monotonic());return
        if STATE['phase']=='warm' and time.monotonic()-STATE['since']>4:
            STATE['capture'].capture_scene();STATE.update(phase='export',since=time.monotonic());return
        if STATE['phase']=='export' and time.monotonic()-STATE['since']>2:
            path=GAME+'/UI/Map/T_OpenWorld_Overview';backup_asset(path)
            texture=u.load_asset(path)
            if texture:u.RenderingLibrary.convert_render_target_to_texture2d_editor_only(WORLD,STATE['target'],texture)
            else:texture=u.RenderingLibrary.render_target_create_static_texture2d_editor_only(STATE['target'],path,u.TextureCompressionSettings.TC_DEFAULT,u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            assert texture
            texture.set_editor_property('compression_no_alpha',True);texture.set_editor_property('lod_group',u.TextureGroup.TEXTUREGROUP_UI);save(texture)
            u.RenderingLibrary.export_render_target(WORLD,STATE['target'],str(OUT),'OpenWorld_Overview.png')
            STATE['capture'].set_editor_property('texture_target',None)
            # Capture actor was created after saving the map and is never persisted.
            ACTORS.destroy_actor(STATE['actor'])
            REPORT['complete']=True;finish()
    except Exception:
        error=traceback.format_exc();u.log_error(error);finish(error)
    finally:
        STATE['busy']=False


STATE['callback']=u.register_slate_post_tick_callback(tick)
