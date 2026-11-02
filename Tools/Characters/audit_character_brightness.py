"""Read-only material audit for manual brightness troubleshooting. Never saves assets."""
import json
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/CharacterBrightness';OUT.mkdir(parents=True,exist_ok=True)

def value(obj,name):
    try:
        result=obj.get_editor_property(name)
        if isinstance(result,(bool,int,float,str)):return result
        return str(result)
    except Exception:return None

def material_info(material):
    entry={'path':material.get_path_name(),'scalars':{},'vectors':{},'switches':{},'parents':[]}
    if isinstance(material,u.MaterialInstanceConstant):
        for kind in ['scalar','vector','static_switch']:
            names=getattr(u.MaterialEditingLibrary,f'get_{kind}_parameter_names',None)
            getter=getattr(u.MaterialEditingLibrary,f'get_material_instance_{kind}_parameter_value',None)
            if not names or not getter:continue
            for name in names(material):
                try:
                    result=getter(material,name)
                    if kind=='vector':result=[result.r,result.g,result.b,result.a]
                    entry[{'scalar':'scalars','vector':'vectors','static_switch':'switches'}[kind]][str(name)]=result
                except Exception:pass
    current=material
    while isinstance(current,u.MaterialInstance):
        current=current.get_editor_property('parent')
        if not current:break
        entry['parents'].append(current.get_path_name())
    if isinstance(current,u.Material):
        entry['root_properties']={name:value(current,name) for name in ['shading_model','blend_mode','two_sided','use_material_attributes']}
        getter=getattr(u.MaterialEditingLibrary,'get_material_property_input_node',None)
        if getter:
            for prop in ['MP_BASE_COLOR','MP_EMISSIVE_COLOR']:
                try:
                    node=getter(current,getattr(u.MaterialProperty,prop))
                    entry[prop]=node.get_class().get_name() if node else None
                except Exception:pass
    return entry

result={}
meshes={
    'Male':'/Game/IsekaiClass/Player/SamplePlayer/Male/SK_Player_M',
    'Female':'/Game/IsekaiClass/Player/SamplePlayer/Female/SK_Player_F',
    'Monster':'/Game/IsekaiClass/Enemy/SampleEnemies/MEGA_Tiny_Dragon/Meshes/SK_DragonSD_B',
}
for name,path in meshes.items():
    mesh=u.load_asset(path);assert mesh,path
    result[name]={'mesh':mesh.get_path_name(),'materials':[]}
    for slot in mesh.get_editor_property('materials'):
        material=slot.get_editor_property('material_interface')
        if material:result[name]['materials'].append({'slot':str(slot.get_editor_property('material_slot_name')),**material_info(material)})
(OUT/'Materials.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_CHARACTER_BRIGHTNESS_READ_ONLY_AUDIT_COMPLETE')
