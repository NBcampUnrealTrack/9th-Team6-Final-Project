"""Read-only widget rendering and saved asset checks; no editor shutdown calls."""
import unreal as u
import json
from pathlib import Path
if '-TestExit=' in u.SystemLibrary.get_command_line():
    # The dedicated automation process owns its normal test-run lifecycle.
    u.EditorPythonScripting.set_keep_python_script_alive(True)
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/LifeWorldUpgrade/Preview';OUT.mkdir(parents=True,exist_ok=True)
bp=u.load_asset('/Game/IsekaiClass/UI/Widget/WBP_IC_Map')
results={}
for name,question in (('WarpList',-1),('WarpConfirm',1)):
    file=OUT/(name+'.png')
    results[name]=u.ICBlueprintMaintenanceLibrary.render_frontend_preview(bp,str(file),1600,900,question)
    assert results[name],name
results['foliage']=[]
for folder,prefix,count in (('Collection','Collection',10),('Mining','Ore',6)):
    for index in range(1,count+1):
        path=f'/Game/IsekaiClass/Life/{folder}/Foliage/FT_R01_{prefix}_{index:02d}'
        asset=u.load_asset(path);assert isinstance(asset,u.FoliageType_Actor)
        assert not asset.get_editor_property('static_mesh_only')
        assert asset.get_editor_property('density')==7.5 and asset.get_editor_property('radius')==200.
        actor=u.get_default_object(asset.get_editor_property('actor_class'))
        assert str(actor.point_id)=='None'
        results['foliage'].append(path)
(OUT/'Review.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('IC_LIFE_UPGRADE_READONLY_PREVIEW_COMPLETE')
