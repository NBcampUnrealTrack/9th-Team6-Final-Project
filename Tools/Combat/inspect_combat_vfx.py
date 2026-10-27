"""Dump authored Niagara VM source into Saved for renderer diagnostics."""
import unreal as u
for name in ('SwordSlash', 'MagicOrb', 'Arrow'):
    system = u.load_asset('/Game/IsekaiClass/VFX/CombatEffect/NormalAttack/NS_IC_Normal_' + name)
    assert u.ICCombatVFXSetupLibrary.dump_combat_niagara(system)
u.log('IC_VFX_DIAGNOSTICS_EXPORTED')
