"""Inspect the current saved map and ground at the user's four warp coordinates."""
import hashlib
import json
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out = root / 'Saved/RequestedHUDWarps'
out.mkdir(parents=True, exist_ok=True)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_name() != 'L_OpenWorld':
    u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/Maps/L_OpenWorld')
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
all_actors = actors.get_all_level_actors()
stones = [a for a in all_actors if isinstance(a, u.ICWarpStone)]
anchors = [a for a in all_actors if isinstance(a, u.ICWorldAnchor)]


def xyz(value):
    return [value.x, value.y, value.z]


report = {'world': world.get_path_name(), 'actor_count': len(all_actors),
          'stones': [{'name': a.get_actor_label(), 'id': str(a.get_editor_property('WarpID')),
                      'location': xyz(a.get_actor_location()), 'class': a.get_class().get_path_name()} for a in stones],
          'anchors': [{'id': str(a.get_editor_property('AnchorID')), 'location': xyz(a.get_actor_location())}
                      for a in anchors if 'WRP_OW' in str(a.get_editor_property('AnchorID'))],
          'ground': [], 'hud': {}, 'portal_components': []}
for name, x, y, z in [('집', -53740., 629630., -5155.), ('마을여관', -55695., 608350., 6125.),
                      ('추억의 다리', -60715., 561595., -6495.), ('폐허 마을', -52830., 505280., -6430.)]:
    surfaces = []
    top = z + 500
    for index in range(8):
        hit = u.SystemLibrary.line_trace_single(world, u.Vector(x, y, top), u.Vector(x, y, -50000),
                                                u.TraceTypeQuery.ECC_VISIBILITY, True, stones,
                                                u.DrawDebugTrace.NONE, True)
        if not hit:
            break
        data = hit.to_dict()
        if not data.get('blocking_hit'):
            break
        point = data['impact_point']
        surfaces.append({'point': xyz(point), 'normal': xyz(data['impact_normal']),
                         'object': str(data.get('hit_object_handle', ''))})
        top = point.z - 10
    report['ground'].append({'name': name, 'requested': [x, y, z], 'surfaces': surfaces})

bp = u.load_asset('/Game/IsekaiClass/UI/Widget/WBP_GameHUD')
report['hud']['names'] = [str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)]
report['hud']['structure'] = u.ICBlueprintMaintenanceLibrary.inspect_frontend_widget(bp)
portal = u.load_asset('/Game/IsekaiClass/World/Blueprints/BP_ICWarpStone')
for component in u.get_default_object(portal.generated_class()).get_components_by_class(u.StaticMeshComponent):
    report['portal_components'].append({'name': component.get_name(), 'mesh': component.static_mesh.get_path_name() if component.static_mesh else None,
                                        'visible': component.is_visible(), 'transform': str(component.get_relative_transform())})
for relative in ['Content/IsekaiClass/World/Maps/L_OpenWorld.umap', 'Content/IsekaiClass/UI/Widget/WBP_IC_Minimap.uasset']:
    report.setdefault('sha256', {})[relative] = hashlib.sha256((root / relative).read_bytes()).hexdigest()
(out / 'Before.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_REQUESTED_WARPS_AUDIT_COMPLETE')
