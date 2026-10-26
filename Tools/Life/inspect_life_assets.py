import json
from pathlib import Path
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/LifePrototype';out.mkdir(parents=True,exist_ok=True)
lib=u.EditorAssetLibrary
farm='/Game/IsekaiClass/World/SampleLevel/DreamscapeSeries/DreamscapeFarm'
folders=[farm+'/Meshes/Foliage/Crops/GenericCrops',farm+'/Meshes/Farm_Animals',
 '/Game/IsekaiClass/Life/Fishing/Underwater_life/Blueprints',
 '/Game/IsekaiClass/Life/Mining/ShatterStone/StylizedGemstones']
report={'assets':{},'materials':{}}
for folder in folders:
 for path in sorted(lib.list_assets(folder,recursive=True,include_folder=False)):
  asset=u.load_asset(path)
  if not asset:continue
  data={'class':asset.get_class().get_name()}
  if isinstance(asset,u.StaticMesh):
   materials=[m.material_interface for m in asset.get_editor_property('static_materials')]
  elif isinstance(asset,u.SkeletalMesh):
   materials=[m.material_interface for m in asset.get_editor_property('materials')]
   data['skeleton']=str(asset.get_editor_property('skeleton'))
  else:materials=[]
  if materials:
   data['materials']=[m.get_path_name() if m else None for m in materials]
   for material in materials:
    if not material or material.get_path_name() in report['materials']:continue
    md={'class':material.get_class().get_name()}
    if isinstance(material,u.MaterialInstanceConstant):
     md['parent']=str(material.get_editor_property('parent'))
     md['textures']={str(p.parameter_info.name):str(p.parameter_value) for p in material.get_editor_property('texture_parameter_values')}
     md['vectors']={str(p.parameter_info.name):str(p.parameter_value) for p in material.get_editor_property('vector_parameter_values')}
    report['materials'][material.get_path_name()]=md
  if isinstance(asset,u.AnimBlueprint):data['target_skeleton']=str(asset.get_editor_property('target_skeleton'))
  report['assets'][path]=data
(out/'AssetInventory.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_LIFE_INSPECT_OK assets='+str(len(report['assets'])))
