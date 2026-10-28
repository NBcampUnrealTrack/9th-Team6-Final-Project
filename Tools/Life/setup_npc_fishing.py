"""Targeted authoring for H fishing; does not regenerate unrelated life/combat tables."""
import unreal as u,json,pathlib
root=pathlib.Path(u.Paths.project_dir());out=root/'Saved/NPCFishing';out.mkdir(exist_ok=True)
game='/Game/IsekaiClass';assets=u.AssetToolsHelpers.get_asset_tools();actors=u.get_editor_subsystem(u.EditorActorSubsystem)
def save(asset):
    if isinstance(asset,u.Blueprint):u.BlueprintEditorLibrary.compile_blueprint(asset)
    assert u.EditorAssetLibrary.save_loaded_asset(asset,False),asset.get_path_name()
def blueprint(path,parent):
    bp=u.load_asset(path)
    if not bp:
        folder,name=path.rsplit('/',1);u.EditorAssetLibrary.make_directory(folder)
        factory=u.BlueprintFactory();factory.set_editor_property('parent_class',parent)
        bp=assets.create_asset(name,folder,u.Blueprint,factory)
    assert bp;return bp
def material(path,color):
    result=u.load_asset(path)
    if not result:
        folder,name=path.rsplit('/',1);u.EditorAssetLibrary.make_directory(folder)
        result=assets.create_asset(name,folder,u.Material,u.MaterialFactoryNew())
        expression=u.MaterialEditingLibrary.create_material_expression(result,u.MaterialExpressionConstant3Vector,0,0)
        expression.set_editor_property('constant',u.LinearColor(*color,1))
        u.MaterialEditingLibrary.connect_material_property(expression,'',u.MaterialProperty.MP_BASE_COLOR)
        u.MaterialEditingLibrary.connect_material_property(expression,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
        u.MaterialEditingLibrary.recompile_material(result)
    save(result);return result
line=blueprint(game+'/Life/Fishing/Blueprints/FishingPole/BP_IC_FishingLine',u.ICFishingLine)
line_default=u.get_default_object(line.generated_class())
thread=material(game+'/Life/Fishing/Materials/M_IC_FishingLine',(.8,.88,.93))
floating=material(game+'/Life/Fishing/Materials/M_IC_FishingFloat',(1,.12,.015))
for component in line_default.get_components_by_class(u.StaticMeshComponent):
    component.set_material(0,floating if component.get_name()=='Float' else thread)
save(line)
imc=u.load_asset(game+'/Input/Contexts/IMC_Gameplay');fishing=u.load_asset(game+'/Input/Actions/IA_Fishing')
keys=[m.key for m in imc.get_editor_property('default_key_mappings').get_editor_property('mappings') if m.action==fishing]
if not any(str(k.get_editor_property('key_name'))=='H' for k in keys):
    key=u.Key();key.set_editor_property('key_name','H')
    imc.unmap_all_keys_from_action(fishing);imc.map_key(fishing,key);save(imc)
controller=u.load_asset(game+'/Core/GameFramework/BP_ICPlayerController');default=u.get_default_object(controller.generated_class())
bindings=list(default.get_editor_property('CommandActions'))
if not any(b.get_editor_property('command')==u.ICInputCommand.FISHING and b.get_editor_property('action')==fishing for b in bindings):
    binding=u.ICInputCommandBinding();binding.set_editor_property('action',fishing);binding.set_editor_property('command',u.ICInputCommand.FISHING);binding.set_editor_property('slot',0)
    bindings.append(binding);default.set_editor_property('CommandActions',bindings);save(controller)
u.EditorLoadingAndSavingUtils.load_map(game+'/World/L_TestMap')
zone_bp=u.load_asset(game+'/Life/Fishing/Blueprints/BP_R01_RandomFishingZone')
u.get_default_object(zone_bp.generated_class()).set_editor_property('interaction_text',u.Text('물고기 구역 · H 낚시 (20초)'));save(zone_bp)
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.ICLifeActor) and actor.get_editor_property('life_type')==u.ICDefinitionType.FISHING_POINT:
        actor.set_editor_property('interaction_text',u.Text('물고기 구역 · H 낚시 (20초)'))
        actor.set_actor_label('IC_Fishing_Random_H_20sec')
assert u.EditorLoadingAndSavingUtils.save_current_level()
(out/'AuthoredFishing.json').write_text(json.dumps({'line':line.get_path_name(),'fishing_key':'H'},ensure_ascii=False,indent=2),encoding='utf-8')
u.log('NPC_FISHING_SETUP_OK')
