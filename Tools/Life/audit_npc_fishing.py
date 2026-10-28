import unreal as u,json,pathlib
root=pathlib.Path(u.Paths.project_dir());out=root/'Saved/NPCFishing';out.mkdir(exist_ok=True)
registry=u.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
report={'containers':[],'beds':[],'anchors':[],'input':[],'dialogue':[]}
for asset in registry.get_assets_by_path('/Game',True):
    if str(asset.asset_class_path.asset_name)=='Blueprint' and 'ICNPCContainerActor' in str(asset.get_tag_value('NativeParentClass')):
        report['containers'].append(str(asset.package_name))
imc=u.load_asset('/Game/IsekaiClass/Input/Contexts/IMC_Gameplay')
for m in imc.get_editor_property('default_key_mappings').get_editor_property('mappings'):
    if m.action and 'Fishing' in m.action.get_name():report['input'].append(str(m.key))
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
for a in actors:
    label=a.get_actor_label();cls=a.get_class().get_name()
    if 'bed' in label.lower() or 'bed' in cls.lower():report['beds'].append({'label':label,'class':cls,'location':str(a.get_actor_location()),'rotation':str(a.get_actor_rotation())})
    if isinstance(a,u.ICWorldAnchor):report['anchors'].append({'id':str(a.get_editor_property('anchor_id')),'location':str(a.get_actor_location()),'rotation':str(a.get_actor_rotation())})
try:
    bp=u.load_asset('/Game/IsekaiClass/UI/Widget/WBP_IC_NPCDialogue')
    tree=bp.get_editor_property('widget_tree')
    for w in tree.get_all_widgets():
        row={'name':w.get_name(),'class':w.get_class().get_name()}
        slot=w.get_editor_property('slot')
        if slot:
            row['slot']=slot.get_class().get_name()
            if isinstance(slot,u.CanvasPanelSlot):row.update(anchors=str(slot.get_anchors()),offsets=str(slot.get_offsets()))
        report['dialogue'].append(row)
except Exception as e:report['dialogue_error']=str(e)
(out/'AssetAudit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('NPC_FISHING_AUDIT_OK')
