"""Refresh native camera component templates in the player Blueprint."""
import unreal as u

for path in ["/Game/IsekaiClass/Player/Blueprints/BP_ICPlayerCharacter",
             "/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController"]:
    bp=u.load_asset(path)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    if not u.EditorAssetLibrary.save_loaded_asset(bp,False):
        raise RuntimeError("Blueprint save failed: "+path)
    if "Controller" in path:
        defaults=u.get_default_object(bp.generated_class())
        mapping=defaults.get_editor_property("default_mapping_context")
        if mapping:
            for entry in mapping.get_editor_property("mappings"):
                u.log("IC_CAMERA_INPUT "+str(entry.get_editor_property("action"))+" "+str(entry.get_editor_property("key")))
u.log("IC_PLAYER_CAMERA_SETUP_COMPLETE")
