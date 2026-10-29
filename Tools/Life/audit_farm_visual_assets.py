"""Read-only asset audit; export texture previews into Saved, never save assets."""
import json
from pathlib import Path
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/FarmVisualAudit';OUT.mkdir(parents=True,exist_ok=True)
BASE='/Game/IsekaiClass/World/SampleLevel/'
FARM=BASE+'DreamscapeSeries/DreamscapeFarm/'
paths=[
    '/Game/IsekaiClass/Life/Farming/Blueprints/BP_IC_FarmPlot',
    '/Game/IsekaiClass/Life/Farming/Materials/M_IC_PlotSoil',
    FARM+'Materials/Terrain/MI_Farm_Dirt_01',
    FARM+'Materials/Paths/MI_Farm_DirtPath',
    FARM+'Materials/Terrain/MI_LandscapeFarm_Demo',
    BASE+'Asian_Village/materials/building_materials/base_materials/MI_soil_01_Inst',
    BASE+'Stylized_Ruins/materials/building_materials/base_materials/MI_ground_mud_01_Inst',
    FARM+'Meshes/Props/ExteriorProps/SM_Farm_Planter_01',
    FARM+'Meshes/Props/ExteriorProps/SM_Farm_Planter_02',
    FARM+'Meshes/Props/Pots/SM_Farm_Pot_A_Soil_01',
    FARM+'Meshes/Terrain/SM_Path_1',
    FARM+'Meshes/Terrain/SM_Path_2',
    FARM+'Meshes/Terrain/SM_Path_3',
    FARM+'Textures/Terrain/Dirt/DirtFlat/T_Farm_Dirt_Flat_C',
    FARM+'Textures/Terrain/Dirt/DirtFlat/T_Farm_Dirt_FlatDark_CH',
    BASE+'Wildlands/TEXTURE/Landscape/Soil/T_Soil_BaseColor',
    BASE+'Zenscape_Island/Texture/Landscape/T_Dirt_BaseColor',
]
report=[];textures={}
for path in paths:
    item={'path':path}
    try:
        asset=u.load_asset(path)
        if not asset:raise RuntimeError('Asset did not load')
        item['type']=asset.get_class().get_name()
        if isinstance(asset,u.Blueprint):
            default=u.get_default_object(asset.generated_class());soil=default.get_editor_property('soil')
            item['soil_mesh']=soil.get_editor_property('static_mesh').get_path_name()
            item['soil_materials']=[soil.get_material(i).get_path_name() if soil.get_material(i) else None for i in range(soil.get_num_materials())]
        if isinstance(asset,u.StaticMesh):
            box=asset.get_bounding_box();size=box.max-box.min;item['dimensions_cm']=[size.x,size.y,size.z]
            item['materials']=[entry.material_interface.get_path_name() if entry.material_interface else None for entry in asset.get_editor_property('static_materials')]
            binary=(ROOT/'Content'/path.removeprefix('/Game/')).with_suffix('.uasset').read_bytes()
            start=binary.find(b'\x89PNG\r\n\x1a\n')
            if start>=0:
                cursor=start+8
                while cursor+12<=len(binary):
                    length=int.from_bytes(binary[cursor:cursor+4],'big');kind=binary[cursor+4:cursor+8];cursor+=length+12
                    if kind==b'IEND':
                        preview=OUT/('Thumbnail_'+asset.get_name()+'.png');preview.write_bytes(binary[start:cursor]);item['thumbnail']=str(preview);break
        if isinstance(asset,u.MaterialInterface):
            root=asset;parents=[]
            while isinstance(root,u.MaterialInstance):
                root=root.get_editor_property('parent')
                if not root:break
                parents.append(root.get_path_name())
            item['parents']=parents
            if isinstance(root,u.Material):item['domain']=str(root.get_editor_property('material_domain'))
            values={}
            for name in u.MaterialEditingLibrary.get_texture_parameter_names(asset):
                texture=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(asset,name) if isinstance(asset,u.MaterialInstanceConstant) else u.MaterialEditingLibrary.get_material_default_texture_parameter_value(asset,name)
                if texture:
                    values[str(name)]=texture.get_path_name()
                    if any(word in str(name).lower() for word in ('base','diffuse','albedo','color','colour','texture')):textures[texture.get_path_name()]=texture
            item['texture_parameters']=values
        if isinstance(asset,u.Texture2D):textures[asset.get_path_name()]=asset
    except Exception as error:item['error']=str(error)
    report.append(item)
exports=[]
for path,texture in textures.items():
    if any(word in texture.get_name().lower() for word in ('normal','_nro','_orm','mask','noise')):continue
    if len(exports)>=12:break
    try:
        pack=next((name for name in ('DreamscapeFarm','Asian_Village','Stylized_Ruins','Wildlands','Zenscape_Island') if '/'+name+'/' in path),'Shared')
        task=u.AssetExportTask();task.object=texture;task.filename=str(OUT/(pack+'_'+texture.get_name()+'.png'));task.automated=True;task.prompt=False;task.replace_identical=True
        success=u.Exporter.run_asset_export_task(task)
        exports.append(dict(asset=path,file=task.filename,success=success))
    except Exception as error:exports.append(dict(asset=path,error=str(error)))
(OUT/'Report.json').write_text(json.dumps(dict(assets=report,exports=exports),ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_FARM_VISUAL_AUDIT_COMPLETE')
