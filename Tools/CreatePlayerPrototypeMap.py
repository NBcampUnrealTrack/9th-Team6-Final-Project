import unreal

path = '/Game/PlayerPrototype/L_PlayerPrototype'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.log('Player prototype map already exists; keeping it unchanged.')
else:
    if not levels.new_level(path):
        raise RuntimeError('Could not create player prototype map')
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    mode = unreal.load_class(None, '/Script/IsekaiClass.PlayerPrototypeGameMode')
    if not mode:
        raise RuntimeError('Compile the IsekaiClassEditor module first')
    world.get_world_settings().set_editor_property('default_game_mode', mode)
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0, 0, 120))
    if not levels.save_current_level():
        raise RuntimeError('Could not save player prototype map')
    unreal.log('Player prototype map created: ' + path)
