"""Hide fishing-area overhead labels in the live editor and BP defaults."""
import json
from pathlib import Path
import shutil
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
output = root / 'Saved/FishingLabels'
output.mkdir(parents=True, exist_ok=True)
asset_path = '/Game/IsekaiClass/Life/Fishing/Blueprints/BP_R01_RandomFishingZone'
asset_file = root / 'Content/IsekaiClass/Life/Fishing/Blueprints/BP_R01_RandomFishingZone.uasset'
backup = output / 'BP_R01_RandomFishingZone.before_hide.uasset'
if not backup.exists():
    shutil.copy2(asset_file, backup)


def hide(actor):
    actor.modify()
    actor.set_editor_property('show_life_label', False)
    for name in ('life_label', 'label'):
        component = actor.get_editor_property(name)
        component.set_visibility(False)
        component.set_hidden_in_game(True)


bp = u.load_asset(asset_path)
assert bp, asset_path
hide(u.get_default_object(bp.generated_class()))
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp, False), asset_path

report = {'blueprint': asset_path, 'instances': [], 'level_saved': False}
with u.ScopedEditorTransaction('Hide fishing area overhead labels'):
    for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
        if not isinstance(actor, u.ICLifeActor) or actor.life_type != u.ICDefinitionType.FISHING_POINT:
            continue
        text = str(actor.get_editor_property('interaction_text'))
        hide(actor)
        assert not actor.get_editor_property('show_life_label')
        assert not actor.life_label.is_visible()
        assert str(actor.get_editor_property('interaction_text')) == text
        report['instances'].append({'name': actor.get_actor_label(), 'label_visible': False})

default = u.get_default_object(bp.generated_class())
assert not default.get_editor_property('show_life_label')
(output / 'Applied.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_FISHING_LABELS_HIDDEN: BP saved; live map edits preserved without saving unrelated changes.')
