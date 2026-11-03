"""Approved rollout of NPC003's raw hair + whole-character outline exclusion."""
import hashlib,json,shutil
from pathlib import Path
import unreal as u
import apply_npc003_character_outline as approved
import setup_npc_reactive_lighting as npc
import setup_npc003_hair_only as hair
import setup_player_world_lighting as player

ROOT=npc.ROOT
OUT=ROOT/'Saved/CharacterLighting/AllCharacterOutline_20261103'
PLAYER_BP='/Game/IsekaiClass/Player/Blueprints/BP_ICPlayerCharacter'
# Individually inspected original 2048x2048 body atlases. Coordinates are UVs.
RECTS={
 'NPC_R01_001':[(.3745,.2495,.6255,.3755),(.255,.38,.37,.495)],
 'NPC_R01_002':[(.5001,.0001,.6249,.2499),(.6251,.0001,.7499,.1249)],
 'NPC_R01_003':hair.RECTS,
 'NPC_R01_004':[(.5626,.2501,.8124,.3749)],
 'NPC_R01_005':[(.7501,.0001,.8749,.2499),(.8801,.1301,.9999,.2499)],
 'NPC_R01_006':[(.5001,.0001,.7499,.4999)],
 'NPC_R01_007':[(.6251,.0001,.7499,.1249),(.5001,.1251,.6249,.2499)],
 'NPC_R01_008':[(.5001,.0001,.9999,.2499)],
 'NPC_R01_009':[(.3751,.0001,.4999,.1249),(.2501,.1251,.3749,.2499)],
 'NPC_R01_010':[(.5001,.5001,.9999,.7499)],
 'NPC_R01_011':[(.2501,.1251,.4999,.2499)],
 'Player_M':[(.2501,.0001,.3749,.1249)],
 'Player_F':[(0.,0.,1.,1.)],
}

def write(name,data):(OUT/name).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def load_audit():return json.loads((OUT/'Audit.json').read_text(encoding='utf-8'))
def blueprint(identifier):return PLAYER_BP if identifier.startswith('Player_') else npc.blueprint_path(identifier)
def changed_slots(subject):return [row for row in subject['materials'] if row['slot']==1] if subject['id']!='Player_F' else [row for row in subject['materials'] if row['category']=='hair']

def check(scope):
    for name,value in scope['protected_hashes'].items():assert digest(ROOT/name)==value,'Protected file changed: '+name

def snapshot(audit):
    allowed={npc.asset_file(row['material']) for subject in audit for row in changed_slots(subject)}
    allowed.update(npc.asset_file(blueprint(s['id'])) for s in audit)
    allowed.update([ROOT/'Config/DefaultEngine.ini',ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap'])
    allowed.update((ROOT/'Content/IsekaiClass/NPC/Materials/WorldReactive/NPC_R01_003/SelectedRawRGB').rglob('*.uasset'))
    files=set()
    for folder in ['Content/IsekaiClass/Player','Content/IsekaiClass/NPC','Plugins/VRM4U/Content','Plugins/KawaiiPhysics']:
        files.update((ROOT/folder).rglob('*.uasset'))
    files.update((ROOT/'Content/IsekaiClass/World/SampleLevel/Ruined_Village/materials/postprocess_materials').rglob('*.uasset'))
    files.update([ROOT/'IsekaiClass.uproject',ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset'])
    before=OUT/'Before';before.mkdir(exist_ok=True)
    path=OUT/'Scope.json'
    if path.exists():return json.loads(path.read_text(encoding='utf-8'))
    for file in sorted(allowed):
        target=before/file.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(file,target)
    scope={'allowed_changes':{str(p.relative_to(ROOT)):digest(p) for p in sorted(allowed)},'protected_hashes':{str(p.relative_to(ROOT)):digest(p) for p in sorted(files) if p not in allowed},'backup':str(before),'subjects':13}
    write('Scope.json',scope);return scope

def material(subject,row):
    identifier=subject['id'];lib=u.MaterialEditingLibrary;body=u.load_asset(row['material'])
    before=hair.parameters(body);names={kind:list(v) for kind,v in before.items()}
    texture=lib.get_material_instance_texture_parameter_value(body,'gltf_tex_diffuse');tint=lib.get_material_instance_vector_parameter_value(body,'gltf_basecolor');assert texture
    if identifier=='NPC_R01_003':
        template=u.load_asset(approved.DEST+'/MI_NPC003FullOutlinePreview')
        lib.set_material_instance_parent(body,template.get_editor_property('parent'))
        master=u.load_asset(approved.MASTER)
    else:
        folder=('/Game/IsekaiClass/Player/Materials/WorldReactive/'+identifier[-1] if identifier.startswith('Player_') else npc.DEST+'/'+identifier)+'/SelectedRawRGB/'+str(row['slot'])
        path=folder+'/M_ICRawHairLit'
        master=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(approved.MASTER,path)
        output=lib.get_material_property_input_node(master,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
        pins={str(n).replace(' ','').lower():v for n,v in zip(lib.get_material_expression_input_names(output),lib.get_inputs_for_material_expression(master,output))}
        choose=pins['shadingmodel'];select={str(n).replace(' ','').lower():v for n,v in zip(lib.get_material_expression_input_names(choose),lib.get_inputs_for_material_expression(master,choose))};mask=select['a']
        assert isinstance(mask,u.MaterialExpressionCustom)
        terms=[f'(step({x0},UV.x)*step(UV.x,{x1})*step({y0},UV.y)*step(UV.y,{y1}))' for x0,y0,x1,y1 in RECTS[identifier]]
        mask.set_editor_property('code','return saturate('+'+'.join(terms)+');')
        output.set_editor_property('desc','IC Raw RGB Hair '+identifier+' slot '+str(row['slot']))
        assert not lib.recompile_material(master)
        cache=[master]
        def parent(source):
            if isinstance(source,u.Material):return master
            target_path=folder+'/Parents/'+source.get_name()
            target=u.load_asset(target_path) if u.EditorAssetLibrary.does_asset_exist(target_path) else u.EditorAssetLibrary.duplicate_asset(source.get_path_name(),target_path)
            lib.set_material_instance_parent(target,parent(source.get_editor_property('parent')))
            overrides=target.get_editor_property('base_property_overrides');overrides.set_editor_property('override_shading_model',False);target.set_editor_property('base_property_overrides',overrides)
            lib.update_material_instance(target);cache.append(target);return target
        lib.set_material_instance_parent(body,parent(body.get_editor_property('parent')))
        for asset in cache:assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    overrides=body.get_editor_property('base_property_overrides');overrides.set_editor_property('override_shading_model',False);body.set_editor_property('base_property_overrides',overrides)
    for name,value in [('ICHairScatter',0.),('ICHairDensity',1.)]:lib.set_material_instance_scalar_parameter_value(body,name,value)
    lib.set_material_instance_texture_parameter_value(body,'ICResearchHairDiffuse',texture);lib.set_material_instance_vector_parameter_value(body,'ICResearchHairTint',tint)
    lib.update_material_instance(body)
    after=hair.parameters(body,names);expected=json.loads(json.dumps(before))
    for name,value in [('ICHairScatter',0.),('ICHairDensity',1.)]:
        if name in expected['scalar']:expected['scalar'][name]=value
    assert after==expected,'Original authored material parameters changed: '+identifier
    assert u.EditorAssetLibrary.save_loaded_asset(body,False)
    return {'slot':row['slot'],'material':body.get_path_name(),'root':master.get_path_name(),'diffuse':texture.get_path_name(),'hair_uv_rectangles':RECTS[identifier],'raw_diffuse_scale':.65,'hair_extra_normal_removed':True,'emissive':0.}

def _apply(audit,scope):
    result=[];saved_blueprints=set()
    for subject in audit:
        identifier=subject['id'];u.log('IC_ALL_OUTLINE_MATERIAL '+identifier)
        mesh=u.load_asset(subject['mesh']);assert npc.non_material_state(mesh)==subject['non_material']
        rows=[material(subject,row) for row in changed_slots(subject)]
        bp_path=blueprint(identifier)
        if bp_path not in saved_blueprints:
            bp=u.load_asset(bp_path);u.BlueprintEditorLibrary.compile_blueprint(bp);component=u.get_default_object(bp.generated_class()).get_editor_property('mesh')
            assert not list(component.get_editor_property('override_materials'))
            component.set_editor_property('render_custom_depth',True);component.set_editor_property('custom_depth_stencil_value',249)
            u.BlueprintEditorLibrary.compile_blueprint(bp);component=u.get_default_object(bp.generated_class()).get_editor_property('mesh')
            assert component.get_editor_property('render_custom_depth') and component.get_editor_property('custom_depth_stencil_value')==249
            assert u.EditorAssetLibrary.save_loaded_asset(bp,False);saved_blueprints.add(bp_path)
        assert npc.non_material_state(mesh)==subject['non_material']
        result.append({'id':identifier,'mesh':subject['mesh'],'blueprint':bp_path,'changed_materials':rows,'non_material_state':subject['non_material']})
        write('Progress.json',{'completed':len(result),'of':13,'last':identifier})
    world=u.EditorLoadingAndSavingUtils.load_map(approved.MAP);before=approved.world_state(world);write('WorldBefore.json',before);count=0
    for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
        if not isinstance(actor,u.PostProcessVolume):continue
        settings=actor.get_editor_property('settings');weighted=settings.get_editor_property('weighted_blendables');entries=list(weighted.get_editor_property('array'))
        for entry in entries:
            old=entry.get_editor_property('object')
            if old and old.get_path_name().split('.')[0]==approved.SOURCE_PP:entry.set_editor_property('object',u.load_asset(approved.PP));count+=1
        weighted.set_editor_property('array',entries);settings.set_editor_property('weighted_blendables',weighted);actor.set_editor_property('settings',settings)
    assert count==1
    after=approved.world_state(world);approved.same_world(before,after);write('WorldAfter.json',after)
    assert u.EditorLoadingAndSavingUtils.save_map(world,approved.MAP)
    config=ROOT/'Config/DefaultEngine.ini';data=config.read_bytes();assert data.count(b'r.CustomDepth=1')==1;config.write_bytes(data.replace(b'r.CustomDepth=1',b'r.CustomDepth=3',1))
    check(scope)
    write('Applied.json',{'subjects':result,'appearance_count':13,'blueprint_count':len(saved_blueprints),'changed_material_count':sum(len(r['changed_materials']) for r in result),'postprocess':approved.PP,'stencil':249,
        'outline_strength':0.,'raw_rgb_method':'NPC003 method 07','world_lights_exposure_and_other_postprocess_settings_unchanged':True,'skeleton_physics_animations_and_slot_assignments_preserved':True,'backup':scope['backup']})
    u.log('IC_ALL_CHARACTER_OUTLINE_APPLIED_13')

def apply():
    assert '-ICApplyAllCharacterOutline' in u.SystemLibrary.get_command_line();assert not u.EditorLevelLibrary.get_pie_worlds(False)
    audit=load_audit();assert len(audit)==13;scope=snapshot(audit);check(scope)
    for name,value in scope['allowed_changes'].items():assert digest(ROOT/name)==value,'Target changed after snapshot: '+name
    preview=json.loads((approved.OUT/'PreviewVerification.json').read_text(encoding='utf-8'));assert preview['complete'] and preview['world_control_complete']
    assert json.loads((approved.OUT/'WorldVisualCheck.json').read_text(encoding='utf-8'))['passes_static_world_check']
    try:_apply(audit,scope)
    except Exception:
        # Loaded packages can be memory-mapped. Restore after this editor exits.
        write('RollbackNeeded.json',{'restore_after_editor_exit':True,'backup':scope['backup']});raise

if __name__=='__main__':apply()
