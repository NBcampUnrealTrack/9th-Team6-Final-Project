"""Read the existing Firefight HUD; never creates art or modifies source assets."""
import json
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out = root / 'Saved' / 'FirefightHUD'
out.mkdir(parents=True, exist_ok=True)
bp = u.load_asset('/Game/IsekaiClass/UI/SampleUI/FirefightUI/Blueprints/UserInterface/BP_Firefight_HUD')
assert bp
result = {'blueprint': bp.get_path_name(), 'widgets': []}
for name in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp):
    widget = u.find_object(None, bp.get_path_name() + ':WidgetTree.' + str(name))
    entry = {'name': str(name), 'found': bool(widget)}
    if widget:
        entry['class'] = widget.get_class().get_name()
        entry['parent'] = str(widget.get_parent().get_name()) if widget.get_parent() else None
        if isinstance(widget, u.Image):
            brush = widget.get_editor_property('brush')
            texture = brush.get_editor_property('resource_object')
            entry['image_size'] = str(brush.get_editor_property('image_size'))
            if isinstance(texture, u.Texture2D):
                entry['texture_size'] = [texture.blueprint_get_size_x(), texture.blueprint_get_size_y()]
        for prop in ('slot', 'brush', 'widget_style', 'text', 'color_and_opacity', 'render_transform', 'visibility'):
            try:
                value = widget.get_editor_property(prop)
                entry[prop] = str(value)
                if prop == 'slot' and value:
                    entry['layout'] = {key: str(value.get_editor_property(key)) for key in ('layout_data', 'z_order', 'auto_size')}
            except Exception:
                pass
    result['widgets'].append(entry)
(out / 'source_audit.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('IC_FIREFIGHT_AUDIT_DONE ' + str(out))
if '-ICRenderFirefight' in u.SystemLibrary.get_command_line():
    assert u.ICBlueprintMaintenanceLibrary.render_frontend_preview(bp, str(out / 'source.png'), 1920, 1080)
    u.SystemLibrary.quit_editor()
