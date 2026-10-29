"""Save and preview only the fullscreen warp-list widget, preserving the HUD minimap."""
import hashlib
import json
from pathlib import Path
import shutil
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/WarpListUpdate'
out.mkdir(parents=True,exist_ok=True)
source=root/'Content/IsekaiClass/UI/Widget/WBP_IC_Map.uasset'
backup=out/'WBP_IC_Map.before.uasset'
if not backup.exists():shutil.copy2(source,backup)
protected=['Content/IsekaiClass/UI/Widget/WBP_IC_Minimap.uasset','Content/IsekaiClass/UI/Widget/WBP_GameHUD.uasset','Content/IsekaiClass/World/Maps/L_OpenWorld.umap']
hashes={path:hashlib.sha256((root/path).read_bytes()).hexdigest() for path in protected}
bp=u.load_asset('/Game/IsekaiClass/UI/Widget/WBP_IC_Map')
assert u.ICBlueprintMaintenanceLibrary.build_frontend_widget(bp,'Map')
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
names={str(n) for n in u.ICBlueprintMaintenanceLibrary.get_frontend_widget_names(bp)}
assert {'WarpDestinations','WarpConfirm','BTN_WarpConfirm','BTN_WarpCancel','TXT_WarpConfirmError'}<=names and 'IMG_Map' not in names
for filename,question in [('Destinations.png',-1),('Confirmation.png',1)]:
    assert u.ICBlueprintMaintenanceLibrary.render_frontend_preview(bp,str(out/filename),1920,1080,question)
for path,digest in hashes.items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest,path
(out/'Applied.json').write_text(json.dumps({'saved':True,'protected_assets_unchanged':True,'widgets':sorted(names)},ensure_ascii=False,indent=2),encoding='utf-8')
if '-TestExit=' in u.SystemLibrary.get_command_line():u.EditorPythonScripting.set_keep_python_script_alive(True)
u.log('IC_WARP_LIST_WIDGET_REFRESHED')
