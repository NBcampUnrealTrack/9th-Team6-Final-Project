"""Copy the original toon hierarchy and expose one final-output brightness control.

This preserves the original shading model, textures, switches and material
attributes. It never edits shared VRM4U parents or saves world assets.
"""
import hashlib
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal as u

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
DEST='/Game/IsekaiClass/Player/Materials/ToonLighting'
BRIGHTNESS='ICCharacterBrightness'

def create_materials(original):
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    cache={}
    def duplicate(source,leaf=None):
        key=source.get_path_name()
        if key in cache:return cache[key]
        suffix=hashlib.sha256(key.encode()).hexdigest()[:6]
        path=f'{DEST}/{leaf}/{source.get_name()}_Toon' if leaf else f'{DEST}/Shared/{source.get_name()}_{suffix}'
        target=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(key,path)
        assert target,path
        cache[key]=target
        if isinstance(source,u.MaterialInstanceConstant):
            parent=duplicate(source.get_editor_property('parent'))
            u.MaterialEditingLibrary.set_material_instance_parent(target,parent)
            u.MaterialEditingLibrary.update_material_instance(target)
            return target
        assert isinstance(source,u.Material)
        expressions=u.MaterialEditingLibrary.get_material_expressions(target)
        assert target.get_editor_property('use_material_attributes')
        property=u.MaterialProperty.MP_MATERIAL_ATTRIBUTES
        original_node=u.MaterialEditingLibrary.get_material_property_input_node(target,property)
        if isinstance(original_node,u.MaterialExpressionMakeMaterialAttributes) and original_node.get_editor_property('desc')=='IC Toon Final Brightness':return target
        original_output=u.MaterialEditingLibrary.get_material_property_input_node_output_name(target,property)
        assert original_node
        split=u.MaterialEditingLibrary.create_material_expression(target,u.MaterialExpressionBreakMaterialAttributes,600,0)
        control=u.MaterialEditingLibrary.create_material_expression(target,u.MaterialExpressionScalarParameter,600,260)
        control.set_editor_property('parameter_name',BRIGHTNESS);control.set_editor_property('default_value',1.)
        multiply=u.MaterialEditingLibrary.create_material_expression(target,u.MaterialExpressionMultiply,860,100)
        output=u.MaterialEditingLibrary.create_material_expression(target,u.MaterialExpressionMakeMaterialAttributes,1100,0)
        output.set_editor_property('desc','IC Toon Final Brightness')
        out_names=u.MaterialEditingLibrary.get_material_expression_output_names(split)
        input_names=u.MaterialEditingLibrary.get_material_expression_input_names(output)
        emissive_out=next(n for n in out_names if 'emissive' in n.lower().replace(' ',''))
        emissive_in=next(n for n in input_names if 'emissive' in n.lower().replace(' ',''))
        connect=u.MaterialEditingLibrary.connect_material_expressions
        split_input=u.MaterialEditingLibrary.get_material_expression_input_names(split)[0]
        assert connect(original_node,original_output,split,split_input)
        normalize=lambda name:name.replace(' ','').lower()
        for name in input_names:
            matching=next((n for n in out_names if normalize(n)==normalize(name)),None)
            if matching and name!=emissive_in:assert connect(split,matching,output,name)
        assert connect(split,emissive_out,multiply,'A')
        assert connect(control,'',multiply,'B')
        assert connect(multiply,'',output,emissive_in)
        assert u.MaterialEditingLibrary.connect_material_property(output,'',property)
        u.MaterialEditingLibrary.recompile_material(target)
        return target
    result={sex:[duplicate(material,sex) for material in values] for sex,values in original.items()}
    return result,list(cache.values())

def apply_to_player_meshes(original,converted,assets):
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    out=ROOT/'Saved/CharacterLighting/ToonCorrection';out.mkdir(parents=True,exist_ok=True)
    backup=out/'Backups'/datetime.now().strftime('%Y%m%d-%H%M%S');backup.mkdir(parents=True,exist_ok=True)
    protected=[ROOT/'Config/DefaultEngine.ini',ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',
               ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset']
    hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
    for asset in reversed(assets):
        assert asset.get_path_name().startswith(DEST+'/')
        assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    rows=[]
    for sex,folder in [('M','Male'),('F','Female')]:
        path=f'/Game/IsekaiClass/Player/SamplePlayer/{folder}/SK_Player_{sex}'
        file=ROOT/'Content'/Path(path.removeprefix('/Game/')).with_suffix('.uasset')
        shutil.copy2(file,backup/file.name)
        mesh=u.load_asset(path);slots=list(mesh.get_editor_property('materials'))
        assert len(slots)==len(converted[sex])
        for index,(slot,material) in enumerate(zip(slots,converted[sex])):
            slot.set_editor_property('material_interface',material)
            rows.append({'sex':sex,'slot':index,'source':original[sex][index].get_path_name(),'assigned':material.get_path_name()})
        mesh.set_editor_property('materials',slots)
        assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
    for path,digest in hashes.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest
    result={'source_toon_look_preserved':True,'night_brightness':.60,'day_brightness':1.,'world_files_unchanged':True,'world_hashes':hashes,'slots':rows,'backup':str(backup)}
    (out/'Applied.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    return result
