"""Author workbench/kitchen BPs, editable WBPs, and the lower-left test area."""
import csv
import io
import json
import time
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
GAME = '/Game/IsekaiClass'
tools = u.AssetToolsHelpers.get_asset_tools()
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
world = u.EditorLoadingAndSavingUtils.load_map(GAME + '/World/L_TestMap')
report = {'stations': [], 'previous_interactions': []}


def save(asset):
    assert u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), asset


def blueprint(folder, name, parent, widget=False):
    bp = u.load_asset(folder + '/' + name)
    if not bp:
        factory = u.WidgetBlueprintFactory() if widget else u.BlueprintFactory()
        factory.set_editor_property('parent_class', parent)
        bp = tools.create_asset(name, folder, u.WidgetBlueprint if widget else u.Blueprint, factory)
    assert bp
    return bp


# Add basic ranged-weapon recipes using the existing item/equipment assets.
def read_table(relative):
    asset = u.load_asset(GAME + '/Data/DataTable/' + relative)
    reader = csv.DictReader(io.StringIO(u.DataTableFunctionLibrary.export_data_table_to_csv_string(asset)))
    return asset, reader.fieldnames, list(reader)


def write_table(relative, asset, fields, rows):
    output = io.StringIO(newline='')
    writer = csv.DictWriter(output, fieldnames=fields, lineterminator='\n')
    writer.writeheader(); writer.writerows(rows)
    text = output.getvalue()
    assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(asset, text), relative
    save(asset)
    destination = ROOT / 'Data' / (relative + '.csv')
    for attempt in range(8):
        try:
            destination.write_text(text, encoding='utf-8', newline='')
            break
        except OSError:
            if attempt == 7: raise
            time.sleep(.15)


recipes, rf, rr = read_table('Recipes/DT_IC_Recipe')
productions, pf, pr = read_table('Life/DT_IC_Production')
registry, gf, gr = read_table('Registry/DT_IC_IDRegistry')
base_recipe = next(r for r in rr if r['ID'] == 'RCP_CRAFT_001')
base_production = next(r for r in pr if r['ID'] == 'PRD_CRAFT_001')
strings = {}
for family, name in (('BOW', '초보자 활'), ('STAFF', '초보자 마법스태프'), ('WAND', '초보자 치유완드')):
    rid, pid = 'RCP_CRAFT_' + family + '_001', 'PRD_CRAFT_' + family + '_001'
    strings[rid + '.NAME'] = name
    strings[rid + '.DESCRIPTION'] = '생활 재료를 조합해 ' + name + ' 제작.'
    strings[pid + '.NAME'] = name + ' 제작'
    if any(r['ID'] == rid for r in rr):
        r = next(r for r in rr if r['ID'] == rid)
        r['NameKey'] = rid + '.NAME'; r['DescriptionKey'] = rid + '.DESCRIPTION'
        r['DisplayName'] = 'LOCTABLE("' + GAME + '/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions", "' + rid + '.NAME")'
        next(r for r in pr if r['ID'] == pid)['NameKey'] = pid + '.NAME'
        continue
    result = '((ItemID="EQP_WPN_' + family + '_001",Count=1))'
    r = dict(base_recipe)
    r.update({rf[0]: rid, 'ID': rid, 'RecipeID': rid, 'ProductionID': pid, 'DisplayName': name,
              'NameKey': '', 'DescriptionKey': '', 'Results': result, 'WorkDurationPlaySeconds': '5'})
    rr.append(r)
    p = dict(base_production)
    p.update({pf[0]: pid, 'ID': pid, 'NameKey': '', 'DescriptionKey': '', 'Results': result})
    pr.append(p)
    for id, kind, relative in ((rid, 'Recipe', 'Recipes/DT_IC_Recipe'), (pid, 'Production', 'Life/DT_IC_Production')):
        g = {f: '' for f in gf}
        g.update({gf[0]: id, 'ID': id, 'DefinitionType': kind, 'RegionID': 'R01',
                  'DefinitionTable': GAME + '/Data/DataTable/' + relative + '.' + relative.rsplit('/', 1)[-1],
                  'DataVersion': '1', 'bDeprecated': 'False', 'bTestOnly': 'False', 'Notes': 'Crafting station starter recipe.'})
        gr.append(g)
for r in rr:
    if r['ID'] + '.NAME' in strings:
        r['NameKey'] = r['ID'] + '.NAME'; r['DescriptionKey'] = r['ID'] + '.DESCRIPTION'
        r['DisplayName'] = 'LOCTABLE("' + GAME + '/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions", "' + r['ID'] + '.NAME")'
for r in pr + gr:
    if r['ID'] + '.NAME' in strings:
        r['NameKey'] = r['ID'] + '.NAME'
string_file = ROOT / 'Data/StringTables/ST_IC_Definitions.csv'
reader = csv.DictReader(io.StringIO(string_file.read_text(encoding='utf-8-sig')))
string_rows = [r for r in reader if r['Key'] not in strings] + [{'Key': key, 'SourceString': value} for key, value in strings.items()]
output = io.StringIO(newline=''); writer = csv.DictWriter(output, fieldnames=['Key', 'SourceString'], lineterminator='\n')
writer.writeheader(); writer.writerows(string_rows); string_file.write_text(output.getvalue(), encoding='utf-8', newline='')
string_asset = u.load_asset(GAME + '/Data/DataTable/StringTables/ST_IC_Definitions')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(string_asset, str(string_file))
save(string_asset)
write_table('Recipes/DT_IC_Recipe', recipes, rf, rr)
write_table('Life/DT_IC_Production', productions, pf, pr)
write_table('Registry/DT_IC_IDRegistry', registry, gf, gr)


existing = actors.get_all_level_actors()
for a in existing:
    if isinstance(a, u.ICInteractionActor) and str(a.get_editor_property('action')) in ('Kitchen', 'Forge'):
        p = a.get_actor_location()
        report['previous_interactions'].append({'name': a.get_actor_label(), 'position': [p.x, p.y, p.z]})

for kind, name, title, recipe_type, anchor, y in (
    ('Crafting', 'CraftingTable', '제작대', u.PlayerRecipeType.CRAFT, 'ANC_FORGE_001', -3100),
    ('Cooking', 'Kitchen', '주방', u.PlayerRecipeType.COOK, 'ANC_KITCHEN_001', -3550)):
    folder = GAME + '/Life/' + kind
    widget = blueprint(folder, 'WBP_IC_' + kind + 'Station', u.ICCraftingStationWidget, True)
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(widget, 'CraftingStation')
    u.BlueprintEditorLibrary.compile_blueprint(widget)
    save(widget)
    bp = blueprint(folder, 'BP_IC_' + name, u.ICCraftingStation)
    cdo = u.get_default_object(bp.generated_class())
    cdo.set_editor_property('recipe_type', recipe_type)
    cdo.set_editor_property('work_anchor_id', anchor)
    cdo.set_editor_property('action', 'Forge' if kind == 'Crafting' else 'Kitchen')
    cdo.set_editor_property('interaction_text', u.Text(title))
    cdo.set_editor_property('station_widget_class', widget.generated_class())
    cdo.get_editor_property('label').set_text(u.Text('CRAFT [F]' if kind == 'Crafting' else 'COOK [F]'))
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    save(bp)
    matches = [a for a in existing if isinstance(a, u.ICCraftingStation) and str(a.get_editor_property('work_anchor_id')) == anchor]
    assert len(matches) <= 1, 'Duplicate station: ' + anchor
    location = u.Vector(2000, y, 40)
    actor = matches[0] if matches else actors.spawn_actor_from_class(bp.generated_class(), location)
    actor.set_actor_location_and_rotation(location, u.Rotator(), False, True)
    actor.set_actor_label('IC_' + name + '_Test')
    actor.set_folder_path(u.Name('IC_Life_Test/' + kind))
    # Keep the left column spacing; make room between the NPC row and plants.
    for old in existing:
        if not isinstance(old, u.ICCraftingStation) and isinstance(old, u.ICInteractionActor) and str(old.get_editor_property('work_anchor_id')) == anchor:
            actors.destroy_actor(old)
    report['stations'].append({'blueprint': bp.get_path_name(), 'widget': widget.get_path_name(),
                               'anchor': anchor, 'position': [2000, y, 40]})

# Move only the two nearby plants to retain clear F-interaction targets.
for actor in actors.get_all_level_actors():
    if isinstance(actor, u.ICLifeActor):
        key = str(actor.get_editor_property('point_id'))
        if key in ('TEST_R01_GATHER_01', 'TEST_R01_GATHER_02'):
            actor.set_actor_location(u.Vector(2450, -3000 if key.endswith('01') else -3420, 0), False, True)

assert u.EditorLoadingAndSavingUtils.save_map(world, GAME + '/World/L_TestMap')
(ROOT / 'Saved/CraftingStationSetup.json').write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
u.log('IC_CRAFTING_STATIONS_SETUP_COMPLETE')
