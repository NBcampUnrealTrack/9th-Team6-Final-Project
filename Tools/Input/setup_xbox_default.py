"""Author the requested Xbox default without changing the user's other preferences."""
import unreal as u,pathlib,shutil
root=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
saved=root/'Saved/Config/WindowsEditor/GameUserSettings.ini'
backup=root/'Saved/Polish/Before_Xbox_GameUserSettings.ini'
if saved.exists() and not backup.exists():shutil.copy2(saved,backup)
settings=u.GameUserSettings.get_game_user_settings();assert isinstance(settings,u.ICGameUserSettings)
settings.set_editor_property('GamepadLayout',1);settings.save_settings()
bp=u.load_asset('/Game/IsekaiClass/UI/Widget/WBP_IC_Settings')
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,'Settings')
u.BlueprintEditorLibrary.compile_blueprint(bp);assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
u.log('XBOX_DEFAULT_SETUP_OK')
