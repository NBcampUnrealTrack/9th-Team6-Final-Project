"""Inventory Widget Blueprints, asset references and source references without modifying assets."""
import json
import re
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT = ROOT / 'Saved/WidgetUsageAudit'
OUT.mkdir(parents=True, exist_ok=True)
registry = u.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()
options = u.AssetRegistryDependencyOptions()
sources = []
for folder in ('Source', 'Config', 'Tools'):
    for path in (ROOT / folder).rglob('*'):
        if path.suffix not in ('.cpp', '.h', '.ini', '.py') or path == Path(__file__):
            continue
        relative = path.relative_to(ROOT).as_posix()
        sources.append((relative, path.read_text(encoding='utf-8-sig', errors='replace').splitlines()))

assets = registry.get_assets_by_class(u.TopLevelAssetPath('/Script/UMGEditor', 'WidgetBlueprint'), True)
report = {'widgets': [], 'controller_defaults': {}}
for data in sorted(assets, key=lambda value: str(value.package_name)):
    package = str(data.package_name)
    if not package.startswith('/Game/'):
        continue
    name = str(data.asset_name)
    refs = sorted(str(value) for value in registry.get_referencers(data.package_name, options)
                  if str(value) != package)
    deps = sorted(str(value) for value in registry.get_dependencies(data.package_name, options))
    hits = []
    for relative, lines in sources:
        for number, line in enumerate(lines, 1):
            if re.search(r'(?<![A-Za-z0-9_])' + re.escape(name) + r'(?:_C)?(?![A-Za-z0-9_])', line, re.IGNORECASE):
                hits.append({'file': relative, 'line': number, 'text': line.strip()})
    item = {'asset': package, 'name': name, 'referencers': refs, 'dependencies': deps, 'source_references': hits}
    if package.startswith('/Game/IsekaiClass/UI/Widget/'):
        bp = data.get_asset()
        item['parent'] = u.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name()
        item['widget_names'] = [str(value) for value in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)]
        if name == 'WBP_IC_NPCDialogue':
            default = u.get_default_object(bp.generated_class())
            property_name = 'entry_widget_class'
            item[property_name] = str(default.get_editor_property(property_name))
    report['widgets'].append(item)

controller = u.load_asset('/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController')
default = u.get_default_object(controller.generated_class())
for name in ('main_menu_widget_class', 'gameplay_widget_class', 'character_create_widget_class',
             'save_load_widget_class', 'quest_tracker_widget_class', 'dialogue_widget_class'):
    report['controller_defaults'][name] = str(default.get_editor_property(name))
(OUT / 'References.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
main = [widget for widget in report['widgets'] if widget['asset'].startswith('/Game/IsekaiClass/UI/Widget/')]
summary = ['Widget Blueprint 사용 현황', '', f"프로젝트 전체: {len(report['widgets'])}개",
           f'현재 게임 UI 폴더: {len(main)}개', f"샘플 콘텐츠: {len(report['widgets']) - len(main)}개", '',
           '현재 게임 UI 에셋:', *(widget['asset'] for widget in main), '',
           '코드 참조, 에셋 참조, 부모 클래스, 컨트롤러 기본 설정은 References.json에 기록했습니다.']
(OUT / 'Summary.txt').write_text('\n'.join(summary) + '\n', encoding='utf-8')
u.log('IC_WIDGET_USAGE_AUDIT_OK widgets=' + str(len(report['widgets'])))
