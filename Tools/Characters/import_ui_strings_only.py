"""Import UI source strings and configure the shared composite font."""

from pathlib import Path

import unreal


project = Path(unreal.Paths.project_dir())
table = unreal.load_asset('/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI')
if not table:
    raise RuntimeError('ST_IC_UI asset was not found')

source = project / 'Data' / 'StringTables' / 'ST_IC_UI.csv'
if not unreal.ICBlueprintMaintenanceLibrary.import_ui_strings(table, str(source)):
    raise RuntimeError(f'Could not import {source}')
if not unreal.EditorAssetLibrary.save_loaded_asset(table):
    raise RuntimeError('Could not save ST_IC_UI')
print('ST_IC_UI_IMPORTED', source)

if not unreal.ICBlueprintMaintenanceLibrary.ensure_frontend_font_fallback():
    raise RuntimeError('Could not configure FONT_IC_UI fallback')
font = unreal.load_asset('/Game/IsekaiClass/UI/Fonts/FONT_IC_UI')
if not unreal.EditorAssetLibrary.save_loaded_asset(font):
    raise RuntimeError('Could not save FONT_IC_UI')
print('FONT_IC_UI_FALLBACK_CONFIGURED')
