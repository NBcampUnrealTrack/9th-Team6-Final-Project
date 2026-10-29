"""Add boss/guide HUD overlays and four warp endpoints without rebuilding dev's map UI."""
import hashlib
import json
import shutil
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out = root / 'Saved/RequestedHUDWarps'
backup = out / 'BeforeFiles'
out.mkdir(parents=True, exist_ok=True)
game = '/Game/IsekaiClass'
points = [('01_HOME', '집', -53740., 629630., -5155., True),
          ('02_INN', '마을여관', -55695., 608350., 6125., False),
          ('03_BRIDGE', '추억의 다리', -60715., 561595., -6495., False),
          ('04_RUINS', '폐허 마을', -52830., 505280., -6430., False)]


def backup_file(relative):
    source, target = root / relative, backup / relative
    if source.exists() and not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)


def xyz(value):
    return [value.x, value.y, value.z]


def enum_value(enum, *names):
    for name in names:
        if hasattr(enum, name):
            return getattr(enum, name)
    raise RuntimeError('Missing enum value: ' + str(names))


for relative in ['Content/IsekaiClass/World/Maps/L_OpenWorld.umap',
                 'Content/IsekaiClass/UI/Widget/WBP_GameHUD.uasset',
                 'Content/IsekaiClass/Data/DataTable/World/DT_IC_Warp.uasset',
                 'Content/IsekaiClass/Data/DataTable/World/DT_IC_Anchor.uasset',
                 'Data/World/DT_IC_Warp.csv', 'Data/World/DT_IC_Anchor.csv']:
    backup_file(relative)

protected = ['Content/IsekaiClass/UI/Widget/WBP_IC_Minimap.uasset',
             'Content/IsekaiClass/UI/Widget/WBP_IC_Map.uasset',
             'Content/IsekaiClass/World/Maps/SubLevel/L_SAY_AddObject.umap',
             'Content/IsekaiClass/World/Maps/SubLevel/NPCAnchor.umap']
hashes = {path: hashlib.sha256((root / path).read_bytes()).hexdigest() for path in protected if (root / path).exists()}
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_name() != 'L_OpenWorld':
    u.EditorLoadingAndSavingUtils.load_map(game + '/World/Maps/L_OpenWorld')
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.get_editor_subsystem(u.LevelEditorSubsystem).set_current_level_by_name('L_OpenWorld')
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
before_actors = {a.get_path_name() for a in actors.get_all_level_actors()}
stones = {str(a.get_editor_property('WarpID')): a for a in actors.get_all_level_actors() if isinstance(a, u.ICWarpStone)}
anchors = {str(a.get_editor_property('AnchorID')): a for a in actors.get_all_level_actors() if isinstance(a, u.ICWorldAnchor)}
portal_bp = u.load_asset(game + '/World/Blueprints/BP_ICWarpStone')
anchor_bp = u.load_asset(game + '/World/Blueprints/BP_ICWorldAnchor')
assert portal_bp and anchor_bp
warp_table = u.load_asset(game + '/Data/DataTable/World/DT_IC_Warp')
anchor_table = u.load_asset(game + '/Data/DataTable/World/DT_IC_Anchor')
warps = {row['Name']: row for row in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(warp_table))}
anchor_rows = {row['Name']: row for row in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(anchor_table))}
object_types = [enum_value(u.ObjectTypeQuery, 'OBJECT_TYPE_QUERY' + str(index), name)
                for index, name in [(1, 'ECC_WORLD_STATIC'), (2, 'ECC_WORLD_DYNAMIC'), (3, 'ECC_PAWN')]]
pawn_channel = enum_value(u.CollisionChannel, 'ECC_PAWN', 'CC_PAWN', 'PAWN')
report = {'destinations': [], 'protected_assets_unchanged': False, 'existing_actors_preserved': False}


def ground_at(x, y, expected_z):
    hit = u.SystemLibrary.line_trace_single(world, u.Vector(x, y, expected_z + 250), u.Vector(x, y, -50000),
                                            u.TraceTypeQuery.ECC_VISIBILITY, True, list(stones.values()),
                                            u.DrawDebugTrace.NONE, True)
    if not hit:
        raise RuntimeError(f'No ground at {x}, {y}')
    data = hit.to_dict()
    if not data.get('blocking_hit') or data['impact_normal'].z < .7:
        raise RuntimeError(f'No walkable ground at {x}, {y}')
    return data['impact_point']


def clear_arrival(position):
    components = u.SystemLibrary.capsule_overlap_components(world, position, 42, 96, object_types, None, list(stones.values()))
    if isinstance(components, tuple):
        components = components[-1]
    for component in components or []:
        response = component.get_collision_response_to_channel(pawn_channel)
        if getattr(response, 'value', None) == 2 or 'BLOCK' in str(response).upper():
            return False
    return True


def transform_text(position):
    return f'(Rotation=(X=0,Y=0,Z=0,W=1),Translation=(X={position.x},Y={position.y},Z={position.z}),Scale3D=(X=1,Y=1,Z=1))'


for code, label, x, y, requested_z, default_open in points:
    warp_id, visit_id, arrival_id = 'WRP_OW_' + code, 'ANC_WRP_OW_VISIT_' + code, 'ANC_WRP_OW_ARRIVAL_' + code
    assert warp_id in warps and visit_id in anchor_rows and arrival_id in anchor_rows
    ground = ground_at(x, y, requested_z)
    stone = stones.get(warp_id) or actors.spawn_actor_from_class(portal_bp.generated_class(), ground + u.Vector(0, 0, 3))
    assert stone.get_outer().get_outer() == world, 'Warp must be placed in the persistent level'
    stones[warp_id] = stone
    stone.set_actor_label('Warp_OW_' + code + '_' + label)
    stone.set_actor_location(ground + u.Vector(0, 0, 3), False, True)
    stone.set_editor_property('WarpID', warp_id)
    stone.set_editor_property('CostGold', 500)
    stone.set_editor_property('InteractionText', u.Text(label + ' · 워프돌'))
    stone.get_editor_property('Label').set_text(u.Text(label + ' · 워프돌'))
    center, extent = stone.get_actor_bounds(False)
    stone.add_actor_world_offset(u.Vector(0, 0, max(0., ground.z + 3 - (center.z - extent.z))), False, True)
    landing = None
    for dx, dy in [(250, 0), (-250, 0), (0, 250), (0, -250), (350, 350), (-350, 350), (350, -350), (-350, -350), (500, 0), (-500, 0)]:
        try:
            floor = ground_at(x + dx, y + dy, ground.z + 120)
        except RuntimeError:
            continue
        candidate = floor + u.Vector(0, 0, 100)
        if abs(floor.z - ground.z) < 160 and clear_arrival(candidate):
            landing = candidate
            break
    assert landing is not None, 'No clear player arrival beside ' + label
    warps[warp_id].update(VisitAnchorID=visit_id, ArrivalAnchorID=arrival_id, VisitRadiusCm=400, bUnlockedByDefault=default_open)
    for identifier, position, safe in [(visit_id, stone.get_actor_location(), False), (arrival_id, landing, True)]:
        anchor_rows[identifier].update(WorldTransform=transform_text(position), bSafeForArrival=safe,
                                       Type='WarpArrival' if safe else 'Transit', ClearanceRadiusCm=42, ClearanceHalfHeightCm=96)
        actor = anchors.get(identifier) or actors.spawn_actor_from_class(anchor_bp.generated_class(), position)
        actor.set_actor_label(identifier)
        actor.set_editor_property('AnchorID', identifier)
        actor.set_actor_location(position, False, True)
    report['destinations'].append({'id': warp_id, 'name': label, 'requested': [x, y, requested_z],
                                    'stone': xyz(stone.get_actor_location()), 'arrival': xyz(landing),
                                    'ground_z': ground.z, 'default_open': default_open})

for table, rows, name in [(warp_table, warps, 'DT_IC_Warp'), (anchor_table, anchor_rows, 'DT_IC_Anchor')]:
    assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(list(rows.values()), ensure_ascii=False))
    assert u.EditorAssetLibrary.save_loaded_asset(table, False)
    (root / 'Data/World' / (name + '.csv')).write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table), encoding='utf-8-sig')

hud = u.load_asset(game + '/UI/Widget/WBP_GameHUD')
names_before = {str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(hud)}
assert u.ICBlueprintMaintenanceLibrary.add_adventure_hud(hud)
u.BlueprintEditorLibrary.compile_blueprint(hud)
assert u.EditorAssetLibrary.save_loaded_asset(hud, False)
names_after = {str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(hud)}
assert names_before <= names_after, 'Existing dev HUD widgets were removed'
assert {'HUD_BossBar', 'BAR_BossHP', 'TXT_BossName', 'HUD_ControlGuide', 'TXT_GuideBody'} <= names_after
report['hud_added_widgets'] = sorted(names_after - names_before)
report['existing_hud_widgets_preserved'] = True
assert before_actors <= {a.get_path_name() for a in actors.get_all_level_actors()}, 'Existing map actors were removed'
report['existing_actors_preserved'] = True
assert u.EditorLoadingAndSavingUtils.save_map(world, game + '/World/Maps/L_OpenWorld')
for relative, digest in hashes.items():
    assert hashlib.sha256((root / relative).read_bytes()).hexdigest() == digest, 'Protected asset changed: ' + relative
report['protected_assets_unchanged'] = True
report['complete'] = True
(out / 'Applied.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_REQUESTED_HUD_WARPS_APPLIED')
