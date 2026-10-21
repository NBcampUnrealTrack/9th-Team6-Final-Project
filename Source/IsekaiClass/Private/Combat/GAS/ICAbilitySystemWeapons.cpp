#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"

EICWeaponFamily UICAbilitySystemComponent::GetEquippedWeaponFamily() const
{
    if (Runtime.Kind == EICCharacterKind::Player)
        if (const auto* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UICInventoryComponent>() : nullptr)
            return Inventory->GetEquippedWeaponFamily();
    const int32 Slot = static_cast<int32>(EPlayerEquipSlot::Weapon);
    if (!Runtime.Equipment.IsValidIndex(Slot) || Runtime.Equipment[Slot].IsEmpty() || !Definitions()) return EICWeaponFamily::None;
    FICDefinitionResult Result;
    const auto* Item = Definitions()->Find<FICItemRow>(Runtime.Equipment[Slot].ItemID, Result);
    const auto* Gear = Item ? Definitions()->Find<FICEquipmentRow>(Item->EquipmentID, Result) : nullptr;
    return Gear ? Gear->WeaponFamily : EICWeaponFamily::None;
}

EICWeaponFamily UICAbilitySystemComponent::GetSkillWeaponFamily(FName SkillID) const
{
    FICDefinitionResult Result;
    const auto* SkillRow = Definitions() ? Definitions()->Find<FICSkillRow>(SkillID, Result) : nullptr;
    if (!SkillRow) return EICWeaponFamily::None;
    if (SkillRow->RequiredWeaponFamily != EICWeaponFamily::None) return SkillRow->RequiredWeaponFamily;
    FName ClassID = SkillRow->ClassID;
    if (ClassID.IsNone() && !SkillRow->NPCID.IsNone())
        if (const auto* NPC = Definitions()->Find<FICNPCRow>(SkillRow->NPCID, Result)) ClassID = NPC->ClassID;
    const auto* Class = ClassID.IsNone() ? nullptr : Definitions()->Find<FICClassRow>(ClassID, Result);
    return Class ? Class->WeaponFamily : EICWeaponFamily::None;
}

FName UICAbilitySystemComponent::GetCurrentWeaponSkill() const
{
    const auto Family = GetEquippedWeaponFamily();
    if (Family == EICWeaponFamily::None) return NAME_None;
    if (const FName* Equipped = Runtime.EquippedWeaponSkills.Find(Family))
        return Runtime.UnlockedSkillIDs.Contains(*Equipped) && GetSkillWeaponFamily(*Equipped) == Family ? *Equipped : NAME_None;
    TArray<FName> Skills = Runtime.UnlockedSkillIDs.Array();
    Skills.Sort(FNameLexicalLess());
    for (FName ID : Skills) if (GetSkillWeaponFamily(ID) == Family) return ID;
    return NAME_None;
}

bool UICAbilitySystemComponent::EquipWeaponSkill(EICWeaponFamily Family, FName SkillID)
{
    if (Runtime.Kind != EICCharacterKind::Player || Family == EICWeaponFamily::None || Family > EICWeaponFamily::Wand) return false;
    if (!SkillID.IsNone() && (!Runtime.UnlockedSkillIDs.Contains(SkillID) || GetSkillWeaponFamily(SkillID) != Family)) return false;
    Runtime.EquippedWeaponSkills.Add(Family, SkillID);
    OnStateChanged.Broadcast();
    return true;
}

double UICAbilitySystemComponent::GetSkillCooldownRemaining(FName SkillID) const
{
    return FMath::Max(0., Runtime.Combat.SkillReadyAtPlaySecond.FindRef(SkillID) - PlaySecond());
}
