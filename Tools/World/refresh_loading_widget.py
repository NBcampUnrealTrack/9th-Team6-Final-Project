"""Refresh native tick metadata without changing the authored loading layout."""
import unreal as u
u.EditorPythonScripting.set_keep_python_script_alive(True)
bp=u.load_asset('/Game/IsekaiClass/UI/Widget/WBP_Loading')
assert bp
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
u.log('IC_LOADING_FIRST_POSE_GATE_READY')
