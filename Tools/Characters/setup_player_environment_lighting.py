"""Apply comparison method 7 to both player presets; never save world assets."""
import hashlib
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal as u
import setup_player_world_lighting as SOURCE

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
OUT=ROOT/'Saved/CharacterLighting/EnvironmentLighting'
DEST='/Game/IsekaiClass/Player/Materials/ToonLighting'


def apply():
    assert '-ICApplyEnvironmentLighting' in u.SystemLibrary.get_command_line()
    assert not u.EditorLevelLibrary.get_pie_worlds(False)
    OUT.mkdir(parents=True,exist_ok=True)
    backup=OUT/'Backups'/datetime.now().strftime('%Y%m%d-%H%M%S')
    backup.mkdir(parents=True,exist_ok=True)
    files=[ROOT/'Config/DefaultEngine.ini',ROOT/'Content/IsekaiClass/World/Maps/L_OpenWorld.umap',
           ROOT/'Content/IsekaiClass/World/Blueprints/BP_ICDayNightController.uasset']
    files+=list((ROOT/'Content/IsekaiClass/Player/Materials').rglob('*.uasset'))
    files+=[p for p in (ROOT/'Content/IsekaiClass/Player/SamplePlayer').rglob('*.uasset') if p.name not in ['SK_Player_M.uasset','SK_Player_F.uasset']]
    hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    original=SOURCE.source_materials()
    converted={}
    # Reuse exactly the Unlit copies used for the selected screenshots.
    for sex,materials in original.items():
        converted[sex]=[]
        for source in materials:
            material=u.load_asset(f'{DEST}/{sex}/{source.get_name()}_Toon')
            assert material,source.get_path_name()
            assert SOURCE.effective_shading_model(material)==u.MaterialShadingModel.MSM_UNLIT
            assert 'ICCharacterBrightness' in [str(n) for n in u.MaterialEditingLibrary.get_scalar_parameter_names(material)]
            assert abs(u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(material,'ICCharacterBrightness')-1.)<.0001
            for name in u.MaterialEditingLibrary.get_static_switch_parameter_names(source):
                assert u.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(material,name)==u.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(source,name),str(name)
            converted[sex].append(material)
    rows=[]
    for sex,folder in SOURCE.PLAYERS:
        path=SOURCE.mesh_path(sex,folder)
        file=ROOT/'Content'/Path(path.removeprefix('/Game/')).with_suffix('.uasset')
        shutil.copy2(file,backup/file.name)
        mesh=u.load_asset(path);slots=list(mesh.get_editor_property('materials'))
        assert len(slots)==len(converted[sex])
        for index,(slot,material) in enumerate(zip(slots,converted[sex])):
            previous=slot.get_editor_property('material_interface').get_path_name()
            slot.set_editor_property('material_interface',material)
            rows.append({'sex':sex,'slot':index,'previous':previous,'assigned':material.get_path_name()})
        mesh.set_editor_property('materials',slots)
        assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
    for path,digest in hashes.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest,path
    result={'selected_method':'07_environment','formula':'clamp(0.18 + 0.34*sqrt(sky) + 0.30*sqrt(visibleSun/13.7), 0.18, 0.82)',
            'slots':rows,'backup':str(backup),'protected_files_unchanged':True,'protected_hashes':hashes}
    (OUT/'Applied.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    u.log('IC_ENVIRONMENT_LIGHTING_APPLIED')
    return result
