"""Rebuild only the single-column save list and its one-line slot rows."""
from pathlib import Path
import json
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
strings=u.load_asset('/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI')
assert u.ICBlueprintMaintenanceLibrary.import_ui_strings(strings,str(root/'Data/StringTables/ST_IC_UI.csv'))
assert u.EditorAssetLibrary.save_loaded_asset(strings,False)
out=root/'Saved/SimpleSaveListReview';out.mkdir(parents=True,exist_ok=True)
results={}
for name,kind in [('WBP_IC_SaveSlot','SaveSlot'),('WBP_IC_SaveLoad','SaveLoad')]:
    bp=u.load_asset('/Game/IsekaiClass/UI/Widget/'+name);assert bp,name
    assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,kind)
    assert u.ICBlueprintMaintenanceLibrary.apply_frontend_theme(bp)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
    rows=json.loads(u.ICBlueprintMaintenanceLibrary.inspect_frontend_widget(bp))
    names={row['name'] for row in rows}
    if kind=='SaveSlot':assert all('IC_SaveSlot_Party'+str(i) in names for i in range(4)),name
    assert 'IC_SaveLoad_Detail' not in names,name
    results[name]=rows
    assert u.ICBlueprintMaintenanceLibrary.render_frontend_preview(bp,str(out/(name+'.png')),1920,1080,-1)
(out/'Layout.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_SIMPLE_SAVE_LIST_READY')
