from pathlib import Path
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))/'Saved/KeyboardPrologueReview'
for name,path in [('MorpheusButton','MorpheusGUI/textures/Buttons/t_GlowBtn'),('MorpheusPanel','MorpheusGUI/textures/Panels/t_BasicPanel'),('FirefightButton','FirefightUI/Textures/06Panel/T_Panel_BtnPurpleNormal'),('FirefightPanel','FirefightUI/Textures/06Panel/T_Panel_Bg')]:
    asset=u.load_asset('/Game/IsekaiClass/UI/SampleUI/'+path);assert asset,path
    task=u.AssetExportTask();task.object=asset;task.filename=str(root/(name+'.png'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=u.TextureExporterPNG()
    assert u.Exporter.run_asset_export_task(task)
