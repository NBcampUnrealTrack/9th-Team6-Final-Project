"""Author the five-tab WBP and route project audio through persistent user buses.

Run after building the editor. -ICRenderExpandedSettings also renders each tab.
Existing sound data is retained; only the sound-class routing is changed.
"""
import json
import runpy
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
FOLDER = '/Game/IsekaiClass/Audio/Settings'
TOOLS = u.AssetToolsHelpers.get_asset_tools()
EAL = u.EditorAssetLibrary


def create(name, cls, factory):
    path = FOLDER + '/' + name
    result = u.load_asset(path) if EAL.does_asset_exist(path) else TOOLS.create_asset(name, FOLDER, cls, factory)
    if not result:
        raise RuntimeError('Could not create ' + path)
    return result


def save(asset):
    if not EAL.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + asset.get_path_name())


def category(path):
    path = path.lower()
    if any(word in path for word in ('music', '/bgm', 'soundtrack', '/theme')):
        return 'Music'
    if any(word in path for word in ('/ui_', 'interface', '/menu', '/button', 'cursor')):
        return 'UI'
    if any(word in path for word in ('voice', 'dialogue', 'speech')):
        return 'Voice'
    if any(word in path for word in ('ambient', 'ambience', '/wind', 'water', 'river', '/rain', '/bird', 'environment')):
        return 'Ambient'
    return 'Effects'


def main():
    runpy.run_path(str(ROOT / 'Tools/UI/setup_settings_widget.py'))
    classes = {name: create('SC_' + name, u.SoundClass, u.SoundClassFactory())
               for name in ('Master', 'Music', 'Effects', 'Ambient', 'Voice', 'UI')}
    # Add one child at a time so SoundClass's editor callback updates every parent.
    classes['Master'].set_editor_property('child_classes', [])
    children = []
    for name, value in classes.items():
        if name != 'Master':
            children.append(value)
            classes['Master'].set_editor_property('child_classes', children)
            assert value.get_editor_property('parent_class') == classes['Master']
    for name, value in classes.items():
        properties = value.get_editor_property('properties')
        properties.set_editor_property('volume', 1.)
        if name == 'Music':
            properties.set_editor_property('is_music', True)
        if name == 'UI':
            properties.set_editor_property('is_ui_sound', True)
        value.set_editor_property('properties', properties)
        save(value)
    mix = create('SM_UserSettings', u.SoundMix, u.SoundMixFactory())
    mix.set_editor_property('duration', -1.)
    save(mix)
    registry = u.AssetRegistryHelpers.get_asset_registry()
    registry.wait_for_completion()
    report = {'classes': [value.get_path_name() for value in classes.values()], 'sounds': [], 'complete': False}
    dependencies = u.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
    native_references = '\n'.join(path.read_text(encoding='utf-8-sig', errors='replace')
                                  for folder in ('Source/IsekaiClass', 'Data') for path in (ROOT / folder).rglob('*')
                                  if path.suffix in ('.cpp', '.h', '.csv'))
    report['unused_library_sounds_skipped'] = 0
    for data in registry.get_assets_by_path('/Game/IsekaiClass', recursive=True):
        if str(data.asset_class_path.asset_name) not in ('SoundWave', 'SoundCue', 'MetaSoundSource'):
            continue
        package = str(data.package_name)
        referenced_by_game = any(str(ref).startswith('/Game/IsekaiClass/') and '/SampleAudio/' not in str(ref)
                                 for ref in registry.get_referencers(data.package_name, dependencies))
        if not referenced_by_game and package not in native_references:
            report['unused_library_sounds_skipped'] += 1
            continue
        sound = data.get_asset()
        chosen = classes[category(sound.get_path_name())]
        old = sound.get_editor_property('sound_class_object')
        if old != chosen:
            sound.set_editor_property('sound_class_object', chosen)
            save(sound)
        report['sounds'].append({'path': sound.get_path_name(), 'class': chosen.get_name(),
                                 'previous_class': old.get_path_name() if old else None})
    report['complete'] = True
    # Sounds without an explicit class use the effects bus, including future imports.
    engine_config = ROOT / 'Config/DefaultEngine.ini'
    config = engine_config.read_text(encoding='utf-8-sig')
    section = '[/Script/Engine.AudioSettings]'
    default_class = 'DefaultSoundClassName=/Game/IsekaiClass/Audio/Settings/SC_Effects.SC_Effects'
    if section not in config:
        engine_config.write_text(config.rstrip() + '\n\n' + section + '\n' + default_class + '\n', encoding='utf-8')
    (ROOT / 'Saved/ExpandedSettingsAssetsReport.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if '-ICRenderExpandedSettings' in u.SystemLibrary.get_command_line():
        out = ROOT / 'Saved/ExpandedSettingsReview'
        out.mkdir(parents=True, exist_ok=True)
        widget = u.load_asset('/Game/IsekaiClass/UI/Widget/WBP_IC_Settings')
        for tab, label in enumerate(('Combat', 'Controls', 'Graphics', 'Audio', 'Other')):
            for width, height in ((1600, 900), (1280, 720)):
                if not u.ICBlueprintMaintenanceLibrary.render_frontend_preview(widget, str(out / f'{label}_{width}.png'), width, height, tab):
                    raise RuntimeError('Preview failed: ' + label)
    u.log(f'IC_EXPANDED_SETTINGS_SETUP_COMPLETE sounds={len(report["sounds"])}')


try:
    main()
finally:
    if '-ICRenderExpandedSettings' in u.SystemLibrary.get_command_line():
        u.SystemLibrary.quit_editor()
