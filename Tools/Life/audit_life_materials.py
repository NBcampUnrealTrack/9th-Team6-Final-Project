import json
from pathlib import Path
import unreal as u

out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/'LifePrototype'
inventory=json.loads((out/'AssetInventory.json').read_text(encoding='utf-8'))
report={'materials':{},'expression_api':[n for n in dir(u.MaterialEditingLibrary) if any(s in n for s in ['input','expression','connection'])]}
for path,info in inventory['materials'].items():
 if 'Dreamscape' not in path:continue
 material=u.load_asset(path);chain=[];current=material
 while isinstance(current,u.MaterialInstanceConstant):
  chain.append(current.get_path_name());current=current.get_editor_property('parent')
 data={'chain':chain,'root':str(current),'textures':{str(n):str(u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(material,n)) for n in u.MaterialEditingLibrary.get_texture_parameter_names(material)},
 'vectors':{str(n):str(u.MaterialEditingLibrary.get_material_instance_vector_parameter_value(material,n)) for n in u.MaterialEditingLibrary.get_vector_parameter_names(material)}}
 if isinstance(current,u.Material):
  for property in [u.MaterialProperty.MP_BASE_COLOR,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES]:
   node=u.MaterialEditingLibrary.get_material_property_input_node(current,property)
   data[str(property)]=str(node)
  data['expressions']=[]
  for obj in u.ObjectIterator(u.MaterialExpression):
   if obj.get_outer()!=current:continue
   d={'class':obj.get_class().get_name(),'name':obj.get_name(),'inputs':[n.get_name() if n else None for n in u.MaterialEditingLibrary.get_inputs_for_material_expression(current,obj)],'input_names':list(u.MaterialEditingLibrary.get_material_expression_input_names(obj))}
   for name in ['parameter_name','default_value','desc','a','b','alpha','input','declaration','material_function','texture','name']:
    try:d[name]=str(obj.get_editor_property(name))
    except Exception:pass
   data['expressions'].append(d)
 report['materials'][path]=data
(out/'MaterialAudit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_LIFE_MATERIAL_AUDIT_OK')
