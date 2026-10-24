"""Normalize the existing combat dragon BPs without rebuilding their DTs or test map."""
import unreal as u

names=['BP_DragonSD_'+number for number in ('07','03','164','153','150','114','107','10','38','08')]
names.append('BP_R01_Boss_DragonTheUsurper')
for name in names:
    path='/Game/IsekaiClass/Enemy/Blueprints/Monsters/'+name
    bp=u.load_asset(path)
    assert bp,path
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    default=u.get_default_object(bp.generated_class())
    mesh=default.get_component_by_class(u.SkeletalMeshComponent)
    mesh.set_editor_property('relative_rotation',u.Rotator(pitch=0,yaw=-90,roll=0))
    movement=default.get_component_by_class(u.CharacterMovementComponent)
    movement.set_editor_property('orient_rotation_to_movement',False)
    movement.set_editor_property('use_controller_desired_rotation',True)
    movement.set_editor_property('rotation_rate',u.Rotator(pitch=0,yaw=540,roll=0))
    assert u.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False),path
    assert abs(mesh.get_editor_property('relative_rotation').yaw+90)<.01,path
u.log('IC_MONSTER_FACING_FIXED count='+str(len(names)))
