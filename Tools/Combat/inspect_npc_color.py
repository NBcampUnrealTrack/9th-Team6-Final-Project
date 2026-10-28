"""Developer diagnosis of the generated particle-color expressions."""
import unreal as u
system=u.load_asset('/Game/IsekaiClass/VFX/CombatEffect/Skill/NPC/R01_03/NS_R01_03_Skill')
assert u.ICCombatVFXSetupLibrary.dump_combat_niagara(system)
u.log('NPC_COLOR_DUMP_OK')
