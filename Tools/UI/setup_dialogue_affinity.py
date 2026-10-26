"""Add a reusable five-heart affinity display beside the existing NPC dialogue name."""
import json
from pathlib import Path
import unreal as u

folder = '/Game/IsekaiClass/UI/Widget'
name = 'WBP_IC_AffinityHearts'
path = folder + '/' + name
hearts = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
if not hearts:
    factory = u.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', u.ICAffinityHeartsWidget)
    hearts = u.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, u.WidgetBlueprint, factory)
assert hearts, path
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(hearts, 'AffinityHearts')
u.BlueprintEditorLibrary.compile_blueprint(hearts)
assert isinstance(u.get_default_object(hearts.generated_class()), u.ICAffinityHeartsWidget)
assert u.EditorAssetLibrary.save_loaded_asset(hearts, only_if_is_dirty=False)

dialogue = u.load_asset(folder + '/WBP_IC_NPCDialogue')
assert dialogue
before = set(str(value) for value in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(dialogue))
assert u.ICBlueprintMaintenanceLibrary.add_dialogue_affinity_hearts(dialogue)
u.BlueprintEditorLibrary.compile_blueprint(dialogue)
assert u.EditorAssetLibrary.save_loaded_asset(dialogue, only_if_is_dirty=False)
after = set(str(value) for value in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(dialogue))
assert before <= after, 'Preserve all existing dialogue controls'
assert {'SpeakerHeader', 'SpeakerText', 'AffinityHearts', 'DialogueText', 'Entries'} <= after
heart_names = set(str(value) for value in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(hearts))
assert {'AffinityHeart' + str(index) for index in range(1, 6)} <= heart_names
report = {
    'dialogue': dialogue.get_path_name(),
    'hearts': hearts.get_path_name(),
    'widgets': sorted(after),
    'points_per_heart': 100,
    'maximum_affinity': 500,
    'partial_fill': True,
}
output = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir())) / 'NPCDialogueHearts'
output.mkdir(parents=True, exist_ok=True)
(output / 'Setup.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_DIALOGUE_AFFINITY_VERIFIED ' + json.dumps(report))
