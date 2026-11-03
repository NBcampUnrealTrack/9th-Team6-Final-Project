"""NPC 001-only, light-responsive head shadow softening. Never edits shared parents."""
import hashlib
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal as u
import setup_npc_reactive_lighting as original

ROOT=original.ROOT
OUT=original.OUT/'NPC001SoftHead'
FOLDER=original.DEST+'/NPC_R01_001'
MASTER=FOLDER+'/M_ICNPC001SoftHead'
HEAD=FOLDER+'/MI_N00_000_00_FaceMouth_00_FACE__Instance__World'

def snapshot():
    OUT.mkdir(parents=True,exist_ok=True)
    state_file=OUT/'Scope.json'
    if state_file.exists():return json.loads(state_file.read_text(encoding='utf-8'))
    applied=json.loads((original.OUT/'Applied.json').read_text(encoding='utf-8'))
    paths={ROOT/relative for relative in applied['protected_hashes']}
    paths.update((ROOT/'Content/IsekaiClass/NPC/Materials/WorldReactive').rglob('*.uasset'))
    paths.update((ROOT/'Content/IsekaiClass/NPC/SampleNPC/R01').rglob('*.uasset'))
    head_file=original.asset_file(HEAD)
    paths.discard(head_file)
    paths.discard(original.asset_file(MASTER))
    backup=OUT/'Backups'/datetime.now().strftime('%Y%m%d-%H%M%S')
    backup.mkdir(parents=True,exist_ok=True)
    shutil.copy2(head_file,backup/head_file.name)
    result={'protected_hashes':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(paths)},
        'head':HEAD,'master':MASTER,'backup':str(backup),'scope':'NPC_R01_001 head material only'}
    state_file.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    return result

def create():
    assert '-ICReviewNPC001SoftHead' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    scope=snapshot()
    lib=u.MaterialEditingLibrary
    master=u.load_asset(MASTER) if u.EditorAssetLibrary.does_asset_exist(MASTER) else u.EditorAssetLibrary.duplicate_asset(original.feature_root_path(),MASTER)
    master.set_editor_property('shading_model',u.MaterialShadingModel.MSM_SUBSURFACE)
    output=lib.get_material_property_input_node(master,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    names=lib.get_material_expression_input_names(output)
    pins={str(n).replace(' ','').lower():v for n,v in zip(names,lib.get_inputs_for_material_expression(master,output))}
    if output.get_editor_property('desc')!='IC NPC001 Soft Head':
        def scalar(name,value):
            node=lib.create_material_expression(master,u.MaterialExpressionScalarParameter)
            node.set_editor_property('parameter_name',name);node.set_editor_property('default_value',value)
            return node
        scatter=scalar('ICHeadScatter',.4)
        density=scalar('ICHeadDensity',.55)
        colour=lib.create_material_expression(master,u.MaterialExpressionMultiply)
        assert lib.connect_material_expressions(pins['basecolor'],'',colour,'A')
        assert lib.connect_material_expressions(scatter,'',colour,'B')
        for key,node in [('subsurfacecolor',colour),('opacity',density)]:
            assert lib.connect_material_expressions(node,'',output,next(n for n in names if str(n).replace(' ','').lower()==key))
        output.set_editor_property('desc','IC NPC001 Soft Head')
    errors=lib.recompile_material(master);assert not errors,str(errors)
    assert u.EditorAssetLibrary.save_loaded_asset(master,False)
    original.assert_protected(scope)
    (OUT/'Created.json').write_text(json.dumps({'master':MASTER,'scope_verified':True},indent=2),encoding='utf-8')
    return master

def apply():
    assert '-ICApplyNPC001SoftHead' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    scope=snapshot();original.assert_protected(scope)
    choice=json.loads((OUT/'Chosen.json').read_text(encoding='utf-8'))
    assert 0<=choice['scatter']<=1 and 0<=choice['density']<=1
    material=u.load_asset(HEAD);master=u.load_asset(MASTER);assert material and master
    lib=u.MaterialEditingLibrary
    lib.set_material_instance_parent(material,master)
    overrides=material.get_editor_property('base_property_overrides')
    overrides.set_editor_property('override_shading_model',False)
    material.set_editor_property('base_property_overrides',overrides)
    for name,value in [('ICHeadScatter',choice['scatter']),('ICHeadDensity',choice['density'])]:
        lib.set_material_instance_scalar_parameter_value(material,name,value)
    lib.update_material_instance(material)
    assert u.EditorAssetLibrary.save_loaded_asset(material,False)
    original.assert_protected(scope)
    result={**choice,'head':HEAD,'master':MASTER,'only_npc001_head_changed':True,
        'body_other_npcs_player_and_world_unchanged':True,'backup':scope['backup']}
    (OUT/'Applied.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    u.log('IC_NPC001_SOFT_HEAD_APPLIED')

if __name__=='__main__':
    if '-ICApplyNPC001SoftHead' in u.SystemLibrary.get_command_line():apply()
    else:create()
