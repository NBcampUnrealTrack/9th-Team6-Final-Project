"""Read-only collision inspection around authored warp arrivals."""
import json, math
from pathlib import Path
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
authored=json.loads((root/'Saved/OpenWorldUpgrade/Authored.json').read_text(encoding='utf-8'))
report=[]
for item in authored['destinations']:
    points=[]
    origin=u.Vector(*item['arrival'])
    for radius in (0,150,250,500,1000,2000):
        for i in range(1 if radius==0 else 12):
            p=origin+u.Vector(math.cos(i*math.pi/6)*radius,math.sin(i*math.pi/6)*radius,0)
            hit=u.SystemLibrary.line_trace_single(world,p+u.Vector(0,0,4000),p-u.Vector(0,0,8000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],u.DrawDebugTrace.NONE,True)
            h=hit.to_dict()
            if h['blocking_hit']:
                loc=h['impact_point'];normal=h['impact_normal']
                points.append(dict(radius=radius,xyz=[loc.x,loc.y,loc.z],nz=normal.z,actor=str(h.get('hit_object_handle',''))))
    report.append(dict(id=item['id'],points=points))
(root/'Saved/OpenWorldUpgrade/GroundInspection.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('IC_WARP_GROUND_INSPECTED')
