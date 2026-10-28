"""Repair only the generated NPC copies; never edits the source sample effects."""
import unreal as u

registry=u.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
assets=registry.get_assets_by_path('/Game/IsekaiClass/VFX/CombatEffect/Skill/NPC',recursive=True)
systems=[a for a in assets if str(a.asset_class_path.asset_name)=='NiagaraSystem']
assert len(systems)==24,len(systems)
for index,asset in enumerate(systems,1):
    system=asset.get_asset()
    assert u.ICCombatVFXSetupLibrary.repair_combat_niagara_color(system),str(asset.package_name)
    assert u.EditorAssetLibrary.save_loaded_asset(system,False)
    u.log(f'NPC_COLOR_REPAIRED {index}/24 {asset.package_name}')
u.log('NPC_VFX_COLOR_REPAIR_OK')
