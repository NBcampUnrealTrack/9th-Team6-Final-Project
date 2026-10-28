"""Read-only inventory before the life/shop/codex expansion."""
import json
import re
from pathlib import Path
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/LifeExpansion';out.mkdir(parents=True,exist_ok=True)
registry=u.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
assets=registry.get_assets_by_path('/Game/IsekaiClass',recursive=True)
report={'tables':{},'animations':[],'tools':[],'beds':[],'fishing_blueprints':[]}
for asset in assets:
    path=str(asset.package_name);kind=str(asset.asset_class_path.asset_name);name=str(asset.asset_name)
    if kind=='DataTable' and name in ('DT_IC_Monster','DT_IC_NPC','DT_IC_Service','DT_IC_Shop','DT_IC_Production','DT_IC_Recipe','DT_IC_Crop','DT_IC_Fish','DT_IC_FishingPoint','DT_IC_Home','DT_IC_Codex','DT_IC_Anchor'):
        table=asset.get_asset();report['tables'][name]=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    if kind=='AnimSequence' and re.search('collect|gather|harvest|plant|farm|hoe|seed|fish|mining|pick|sleep|chop|water',name,re.I):report['animations'].append(path)
    if kind=='StaticMesh' and re.search('pick|axe|tool',name,re.I):report['tools'].append(path)
    if kind in ('StaticMesh','Blueprint') and re.search('bed|chest',name,re.I) and '/World/' in path:report['beds'].append({'path':path,'kind':kind})
    if kind=='Blueprint' and path.startswith('/Game/IsekaiClass/Life/Fishing/') and '/Underwater_life/' not in path:
        bp=asset.get_asset();cdo=u.get_default_object(bp.generated_class())
        report['fishing_blueprints'].append({'path':path,'class':cdo.get_class().get_path_name()})
(out/'Audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('LIFE_EXPANSION_AUDIT_OK')
