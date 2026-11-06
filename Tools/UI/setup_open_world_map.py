"""Import the approved top-down capture and its exact projection, changing only UI assets.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Source: Content/IsekaiClass/UI/Map/Source/L_OpenWorld_DevelopedArea_TopDown.png
Capture: centre (-56000, 610000) cm, square orthographic width 65000 cm,
camera facing -Z with yaw/roll zero: image right = +Y, image down = -X.
"""
import json
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
DEST = '/Game/IsekaiClass/UI/Map'
source = ROOT / 'Content/IsekaiClass/UI/Map/Source/L_OpenWorld_DevelopedArea_TopDown.png'
assert source.is_file(), source
tools = u.AssetToolsHelpers.get_asset_tools()
task = u.AssetImportTask()
task.set_editor_property('filename', str(source))
task.set_editor_property('destination_path', DEST)
task.set_editor_property('destination_name', 'T_OpenWorld_Village_UI')
task.set_editor_property('automated', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('save', False)
tools.import_asset_tasks([task])
texture = u.load_asset(DEST + '/T_OpenWorld_Village_UI')
assert isinstance(texture, u.Texture2D)
texture.set_editor_property('lod_group', u.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('compression_no_alpha', True)
texture.set_editor_property('mip_gen_settings', u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('srgb', True)
texture.set_editor_property('never_stream', True)
texture.set_editor_property('address_x', u.TextureAddress.TA_CLAMP)
texture.set_editor_property('address_y', u.TextureAddress.TA_CLAMP)
assert u.EditorAssetLibrary.save_loaded_asset(texture, False)

table = u.load_asset(DEST + '/DT_IC_LevelMap')
if not table:
    factory = u.DataTableFactory()
    factory.set_editor_property('struct', u.load_object(None, '/Script/IsekaiClass.ICMapRow'))
    table = tools.create_asset('DT_IC_LevelMap', DEST, u.DataTable, factory)
assert table
rows = {r['Name']: r for r in json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))}
rows['L_OpenWorld'] = dict(
    Name='L_OpenWorld', ID='MAP_OPEN_WORLD_UI', NameKey='', DescriptionKey='',
    DataVersion=1, bDeprecated=False, bTestOnly=False, Type='Town', RegionID='R01',
    MinimumZoom=1, MaximumZoom=8, VisibleMarkers=['Player', 'Shop', 'Warp', 'Quest'],
    Layers=[dict(
        InteriorFloorID='None', DisplayNameKey='',
        MapTexture=texture.get_path_name(),
        WorldOriginCm='(X=-56000,Y=610000)', MapOriginUV='(X=0.5,Y=0.5)',
        WorldAxisU='(X=0,Y=1)', WorldAxisV='(X=-1,Y=0)',
        WorldCmPerUV='(X=65000,Y=65000)',
        WorldBoundsMinCm='(X=-88500,Y=577500)',
        WorldBoundsMaxCm='(X=-23500,Y=642500)',
    )],
)
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(list(rows.values())))
assert u.EditorAssetLibrary.save_loaded_asset(table, False)
u.log('IC_OPEN_WORLD_MAP_SETUP_OK: ' + texture.get_path_name())
