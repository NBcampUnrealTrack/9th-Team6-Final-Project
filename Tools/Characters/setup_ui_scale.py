"""Restore the standard camera manager; runtime UI scaling is constructed in C++."""
import unreal as u

controller = u.load_asset('/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController')
u.BlueprintEditorLibrary.compile_blueprint(controller)
u.get_default_object(controller.generated_class()).set_editor_property('player_camera_manager_class', u.PlayerCameraManager)
assert u.EditorAssetLibrary.save_loaded_asset(controller, only_if_is_dirty=False)
u.log('IC_UI_SCALE_READY: native UI ScaleToFit 1920x1080; standard PlayerCameraManager')
