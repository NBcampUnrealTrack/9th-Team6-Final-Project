"""Apply the inspected grip orientation to the twelve imported blades/staves."""
import csv
import io
import json
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Equipment/DT_IC_Equipment')
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
count=0
for row in rows:
    parts=row['Name'].split('_')
    if len(parts)!=4 or parts[:2]!=['EQU','WPN'] or parts[2] not in ('SWORD','STAFF','WAND') or parts[3]=='001':continue
    family=parts[2];mesh=u.load_asset(row['WeaponMesh']);box=mesh.get_bounding_box()
    dimensions=[box.max.x-box.min.x,box.max.y-box.min.y,box.max.z-box.min.z]
    axis=u.Vector(*[-1 if i==max(range(3),key=dimensions.__getitem__) else 0 for i in range(3)])
    cdo=u.get_default_object(u.load_class(None,row['WeaponActorClass']))
    original=u.MathLibrary.conv_rotator_to_quaternion(cdo.get_editor_property('visual').get_editor_property('relative_rotation'))
    b=u.MathLibrary.quat_find_between_normals(axis,cdo.get_editor_property('length_axis'));a=original
    rotation=u.Quat(a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z)
    length={'SWORD':100,'STAFF':145,'WAND':110}[family];scale=length/max(dimensions)
    center=u.Vector((box.min.x+box.max.x)/2,(box.min.y+box.max.y)/2,(box.min.z+box.max.z)/2)
    grip=center+axis*(max(dimensions)*({'SWORD':.14,'STAFF':.42,'WAND':.33}[family]-.5))
    offset=u.MathLibrary.quat_rotate_vector(rotation,grip*scale)*-1
    row['WeaponVisualTransform']={'Rotation':dict(X=rotation.x,Y=rotation.y,Z=rotation.z,W=rotation.w),'Translation':dict(X=offset.x,Y=offset.y,Z=offset.z),'Scale3D':dict(X=scale,Y=scale,Z=scale)}
    row['WeaponLengthAxis']=dict(X=axis.x,Y=axis.y,Z=axis.z);count+=1
assert count==12,count
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows))
assert u.EditorAssetLibrary.save_loaded_asset(table,False)
reader=csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table)))
out=io.StringIO(newline='');writer=csv.DictWriter(out,fieldnames=reader.fieldnames,lineterminator='\n');writer.writeheader();writer.writerows(reader)
(root/'Saved/NPCSkillReview/DT_IC_Equipment.csv').write_text(out.getvalue(),encoding='utf-8',newline='')
u.log('NPC_WEAPON_GRIPS_OK')
