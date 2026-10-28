import unreal as u,json,pathlib
mesh=u.load_asset('/Game/IsekaiClass/World/SampleLevel/Medieval_House/meshes/props/SM_bed_01')
verts,tris,normals,uvs,tangents=u.ProceduralMeshLibrary.get_section_from_static_mesh(mesh,0,0)
levels={}
for i in range(0,len(tris),3):
    a,b,c=[verts[n] for n in tris[i:i+3]];ns=[normals[n] for n in tris[i:i+3]]
    x=(a.x+b.x+c.x)/3;y=(a.y+b.y+c.y)/3;z=(a.z+b.z+c.z)/3
    if abs(x)>65 or abs(y)>120 or sum(n.z for n in ns)<2:continue
    area=abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5
    key=round(z,1);levels[key]=levels.get(key,0)+area
root=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))
(root/'Polish/BedSurface.json').write_text(json.dumps({'vertices':len(verts),'horizontal_centre_surfaces':sorted(levels.items(),key=lambda p:p[1],reverse=True)},indent=2),encoding='utf-8');u.log('BED_SURFACE_AUDIT_OK')


