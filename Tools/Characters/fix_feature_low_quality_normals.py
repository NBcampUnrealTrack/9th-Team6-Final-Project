"""Soften the face/body normal mismatch at Low material quality.

VRM4U MF_Normal uses world up at MaterialQualityLevel Low. Our separate
authored-feature surface blends toward that fallback without flattening the
whole face. Other qualities retain the original mesh normal. No emissive fill
or global quality override.
"""
import hashlib,json,shutil
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/CharacterLighting/LowQualityFace_20261103'
MATERIALS=[
 '/Game/IsekaiClass/Player/Materials/WorldReactive/Shared/M_ICAuthoredFeatureLit',
 '/Game/IsekaiClass/NPC/Materials/WorldReactive/Shared/M_ICNPCFeatureLit',
]
TAG='IC Low Quality Feature Normal'

def asset_file(path):return ROOT/'Content'/(path.removeprefix('/Game/')+'.uasset')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def patch_material(path):
    lib=u.MaterialEditingLibrary;material=u.load_asset(path);assert material,path
    assert material.get_editor_property('tangent_space_normal')
    assert material.get_editor_property('shading_model')==u.MaterialShadingModel.MSM_DEFAULT_LIT
    output=lib.get_material_property_input_node(material,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    assert output.get_editor_property('desc')=='IC Authored Feature Surface'
    existing=next((node for node in lib.get_material_expressions(material) if isinstance(node,u.MaterialExpressionQualitySwitch) and node.get_editor_property('desc')==TAG),None)
    if existing:
        branches={str(name):node for name,node in zip(lib.get_material_expression_input_names(existing),lib.get_inputs_for_material_expression(material,existing))}
        assert isinstance(branches['Low'],u.MaterialExpressionNormalize)
        blend=lib.get_inputs_for_material_expression(material,branches['Low'])[0]
        assert isinstance(blend,u.MaterialExpressionLinearInterpolate) and blend.get_editor_property('const_alpha')==.75
        return material
    normal_pin=next(pin for pin in lib.get_material_expression_input_names(output) if pin.replace(' ','').lower()=='normal')
    pins=lib.get_material_expression_input_names(output);normal=lib.get_inputs_for_material_expression(material,output)[pins.index(normal_pin)]
    assert normal is None,'Feature surface already has a normal input; inspect before replacing'
    geometry=lib.create_material_expression(material,u.MaterialExpressionConstant3Vector)
    geometry.set_editor_property('constant',u.LinearColor(0,0,1,0));geometry.set_editor_property('desc','Original geometric tangent normal / all other qualities')
    to_tangent=lib.create_material_expression(material,u.MaterialExpressionTransform)
    to_tangent.set_editor_property('transform_source_type',u.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
    to_tangent.set_editor_property('transform_type',u.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
    assert lib.connect_material_expressions(geometry,'',to_tangent,lib.get_material_expression_input_names(to_tangent)[0])
    blend=lib.create_material_expression(material,u.MaterialExpressionLinearInterpolate)
    blend.set_editor_property('const_alpha',.75)
    blend.set_editor_property('desc','Soften low-quality facial shading while retaining face shape')
    assert lib.connect_material_expressions(geometry,'',blend,'A')
    assert lib.connect_material_expressions(to_tangent,'',blend,'B')
    normalize=lib.create_material_expression(material,u.MaterialExpressionNormalize)
    assert lib.connect_material_expressions(blend,'',normalize,lib.get_material_expression_input_names(normalize)[0])
    switch=lib.create_material_expression(material,u.MaterialExpressionQualitySwitch);switch.set_editor_property('desc',TAG)
    assert lib.connect_material_expressions(geometry,'',switch,'Default')
    # The same (0,0,1) represents world up before this transform, and the
    # geometric tangent normal on the Default branch. They share only a value.
    assert lib.connect_material_expressions(normalize,'',switch,'Low')
    assert lib.connect_material_expressions(switch,'',output,normal_pin)
    assert not lib.recompile_material(material)
    return material

def preview():return [patch_material(path) for path in MATERIALS]

def apply():
    assert '-ICApplyFeatureLowQualityFix' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    OUT.mkdir(parents=True,exist_ok=True);backup=OUT/'Before';backup.mkdir(exist_ok=True)
    changes=[]
    for path in MATERIALS:
        file=asset_file(path);copy=backup/file.name
        if not copy.exists():shutil.copy2(file,copy)
        material=patch_material(path)
        assert u.EditorAssetLibrary.save_loaded_asset(material,False)
        changes.append({'material':path,'before_sha256':sha(copy),'after_sha256':sha(file),'backup':str(copy)})
    (OUT/'FixApplied.json').write_text(json.dumps({'materials':changes,'low_normal':'Normalized blend of geometric normal and world up (0.75) to soften the mismatch with VRM4U MF_Normal Low','other_qualities':'Original geometric normal','emissive':0,'world_and_quality_presets_unchanged':True},ensure_ascii=False,indent=2),encoding='utf-8')
    u.log('IC_FEATURE_LOW_QUALITY_NORMAL_FIX_SAVED')

if __name__=='__main__':apply()
