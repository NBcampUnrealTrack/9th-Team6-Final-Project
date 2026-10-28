#include "Codex/ICCodexSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/StringTable.h"

UICDefinitionSubsystem* UICCodexSubsystem::Definitions() const{return GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();}
FText UICCodexSubsystem::DefinitionText(const FString& Key,FName Fallback) const
{
    auto* Strings=LoadObject<UStringTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"));
    const auto Text=UICTextLibrary::ResolveText(Strings,Key);return Text.bSuccess?Text.Text:Fallback.IsNone()?FText::GetEmpty():FText::FromName(Fallback);
}
FText UICCodexSubsystem::ItemName(FName ID) const
{FICDefinitionResult R;const auto* Item=Definitions()->Find<FICItemRow>(ID,R);return Item&&!Item->DisplayName.IsEmpty()?Item->DisplayName:DefinitionText(Item?Item->NameKey:FString(),ID);}
int32 UICCodexSubsystem::GetPetEssence(int32 Index) const
{return Index>=0?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState().Codex.PetEssence.FindRef(Index):0;}
bool UICCodexSubsystem::IsPetUnlocked(int32 Index) const{return FindPet(Index)&&GetPetEssence(Index)>=EssenceRequired;}
bool UICCodexSubsystem::AddPetEssence(int32 Index,int32 Amount)
{
    if(!FindPet(Index)||Amount<=0)return false;auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();auto Next=State->ExportState();
    if(!Next.bPlayerCreated)return false;
    auto& Count=Next.Codex.PetEssence.FindOrAdd(Index);Count=static_cast<int32>(FMath::Min<int64>(MAX_int32,int64(Count)+Amount));
    return State->CommitGameplayTransaction(Next,FGuid::NewGuid());
}
const FICMonsterRow* UICCodexSubsystem::FindPet(int32 Index) const
{
    if(Index<0)return nullptr;for(const auto* Pet:PetDefinitions())if(Pet->PetCodexIndex==Index)return Pet;return nullptr;
}
TArray<const FICMonsterRow*> UICCodexSubsystem::PetDefinitions() const
{
    TArray<const FICMonsterRow*> Pets;TArray<FName> IDs;Definitions()->GetDefinitionIDs(EICDefinitionType::Monster,IDs);IDs.Sort(FNameLexicalLess());FICDefinitionResult R;
    for(FName ID:IDs)if(const auto* Row=Definitions()->Find<FICMonsterRow>(ID,R);Row&&Row->PetCodexIndex>=0&&!Row->bDeprecated)Pets.Add(Row);
    Pets.Sort([](const FICMonsterRow& A,const FICMonsterRow& B){return A.PetCodexIndex<B.PetCodexIndex;});
    return Pets;
}
TArray<FName> UICCodexSubsystem::GetPetIDs() const
{TArray<FName> IDs;for(const auto* Pet:PetDefinitions())IDs.Add(Pet->ID);return IDs;}
FText UICCodexSubsystem::PetUsageText(int32 Index) const
{
    const auto State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();TArray<FText> Names;
    for(FName Member:GetPartyMembers())if(State.Codex.CharacterPets.FindRef(Member)==Index&&State.Codex.CharacterPets.Contains(Member))Names.Add(CharacterName(Member));
    return Names.IsEmpty()?FText::GetEmpty():FText::Format(NSLOCTEXT("ICCodex","PetInUse","사용중: {0}"),FText::Join(FText::FromString(TEXT(", ")),Names));
}
FText UICCodexSubsystem::PetName(int32 Index) const
{const auto* Row=FindPet(Index);return Row?(!Row->PetName.IsEmpty()?Row->PetName:DefinitionText(Row->NameKey,Row->ID)):FText::Format(NSLOCTEXT("ICCodex","PetSlotName","펫 {0}"),FText::AsNumber(Index+1));}
FText UICCodexSubsystem::PetBuffText(int32 Index) const
{
    const auto* Row=FindPet(Index);if(Row&&!Row->PetBuffDescription.IsEmpty())return Row->PetBuffDescription;
    TArray<FText> Lines;if(Row)for(const auto& M:Row->PetBuffs)
        Lines.Add(FText::FromString(FString::Printf(TEXT("%s  %+.2f  (%+.1f%%)"),*StaticEnum<EICStat>()->GetNameStringByValue(int64(M.Stat)),M.Flat,M.Percent*100)));
    return Lines.IsEmpty()?NSLOCTEXT("ICCodex","NoPetBuff","버프 능력 미등록"):FText::Join(FText::FromString(TEXT("\n")),Lines);
}
TArray<FName> UICCodexSubsystem::GetPartyMembers() const
{
    const auto World=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();TArray<FName> Result;
    if(World.Characters.Contains(UICWorldStateSubsystem::PlayerId()))Result.Add(UICWorldStateSubsystem::PlayerId());
    for(int32 I=0;I<3;++I)for(const auto& Pair:World.Characters)
        if(Pair.Value.Kind==EICCharacterKind::Recruitable&&Pair.Value.Mode==EICCharacterMode::Companion&&Pair.Value.PartySlot==I){Result.Add(Pair.Key);break;}
    return Result;
}
FText UICCodexSubsystem::CharacterName(FName ID) const
{
    if(ID==UICWorldStateSubsystem::PlayerId())return FText::FromString(GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState().PlayerName);
    FICDefinitionResult R;const auto* NPC=Definitions()->Find<FICNPCRow>(ID,R);return DefinitionText(NPC?NPC->NameKey:FString(),ID);
}
UTexture2D* UICCodexSubsystem::CharacterPortrait(FName ID) const
{
    FICDefinitionResult R;if(ID==UICWorldStateSubsystem::PlayerId())
    {const auto World=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();const auto* P=Definitions()->Find<FICPlayerPresetRow>(World.Preset==1?TEXT("PRESET_PLAYER_F"):TEXT("PRESET_PLAYER_M"),R);return P?P->Portrait.LoadSynchronous():nullptr;}
    const auto* NPC=Definitions()->Find<FICNPCRow>(ID,R);return NPC?NPC->Portrait.LoadSynchronous():nullptr;
}
int32 UICCodexSubsystem::GetEquippedPet(FName ID) const
{const auto Data=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState();const auto* Index=Data.Codex.CharacterPets.Find(ID);return Index?*Index:INDEX_NONE;}
UTexture2D* UICCodexSubsystem::GetEquippedPetPortrait(FName ID) const
{const int32 Index=GetEquippedPet(ID);const auto* Row=IsPetUnlocked(Index)?FindPet(Index):nullptr;return Row?Row->Portrait.LoadSynchronous():nullptr;}
bool UICCodexSubsystem::EquipPet(int32 Index,FName ID)
{
    if(!IsPetUnlocked(Index)||!GetPartyMembers().Contains(ID))return false;
    auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();if(!State->FlushParticipants())return false;
    auto Next=State->ExportState();Next.Codex.CharacterPets.Add(ID,Index);return State->CommitGameplayTransaction(Next,FGuid::NewGuid());
}
bool UICCodexSubsystem::HasAcquiredItem(FName ID) const
{return GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState().Codex.AcquiredItemIDs.Contains(ID);}
bool UICCodexSubsystem::IsRecipeKnown(FName ID) const
{FICDefinitionResult R;const auto* Recipe=Definitions()->Find<FICRecipeRow>(ID,R);return Recipe&&(Recipe->bUnlockedByDefault||GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState().UnlockedRecipeIDs.Contains(ID));}
EICWeaponFamily UICCodexSubsystem::SkillFamily(const FICSkillRow& Skill) const
{if(Skill.RequiredWeaponFamily!=EICWeaponFamily::None)return Skill.RequiredWeaponFamily;FICDefinitionResult R;FName ClassID=Skill.ClassID;if(ClassID.IsNone()&&!Skill.NPCID.IsNone())if(const auto* NPC=Definitions()->Find<FICNPCRow>(Skill.NPCID,R))ClassID=NPC->ClassID;const auto* Class=Definitions()->Find<FICClassRow>(ClassID,R);return Class?Class->WeaponFamily:EICWeaponFamily::None;}
TArray<FName> UICCodexSubsystem::SkillCells() const
{
    TArray<FName> Cells,IDs;Definitions()->GetDefinitionIDs(EICDefinitionType::Skill,IDs);IDs.Sort(FNameLexicalLess());FICDefinitionResult R;
    for(FName ID:IDs)if(const auto* S=Definitions()->Find<FICSkillRow>(ID,R);S&&S->OwnerType!=EICSkillOwner::Monster&&!S->bDeprecated&&!S->bTestOnly&&SkillFamily(*S)!=EICWeaponFamily::None)Cells.Add(ID);
    return Cells;
}
TArray<FName> UICCodexSubsystem::EquippedSkills() const
{
    const auto World=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->GetSnapshot(UICWorldStateSubsystem::PlayerId());
    TArray<FName> Result;Result.SetNum(4);auto IDs=World.UnlockedSkillIDs.Array();IDs.Sort(FNameLexicalLess());FICDefinitionResult R;
    for(int32 I=0;I<4;++I)
    {const auto Family=EICWeaponFamily(I+1);if(const auto* ID=World.EquippedWeaponSkills.Find(Family)){Result[I]=*ID;continue;}
     for(FName ID:IDs)if(const auto* Skill=Definitions()->Find<FICSkillRow>(ID,R);Skill&&SkillFamily(*Skill)==Family){Result[I]=ID;break;}}
    return Result;
}
bool UICCodexSubsystem::SetPlayerSkill(FName ID,FText& Error)
{
    auto* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;FICDefinitionResult R;
    const auto* S=Definitions()->Find<FICSkillRow>(ID,R);if(!Player||!Player->Inventory||!S||S->OwnerType==EICSkillOwner::Monster)return false;
    const auto Family=SkillFamily(*S);const auto Weapons=Player->Inventory->GetWeaponLoadout();const int32 I=int32(Family)-1;
    if(!Weapons.IsValidIndex(I)||Weapons[I].IsEmpty()){Error=NSLOCTEXT("ICCodex","WeaponMissing","해당 무기가 장착 안되어있습니다.");return false;}
    auto* ASC=Player->GetICAbilitySystem();if(!ASC||!ASC->EquipWeaponSkill(Family,ID)){Error=NSLOCTEXT("ICCodex","SkillLocked","아직 획득하지 않은 스킬입니다.");return false;}
    // Keep GAS and save state in sync, including when the menu pauses the game clock.
    if(auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>())
    {auto Snapshot=State->GetSnapshot(UICWorldStateSubsystem::PlayerId());Snapshot.EquippedWeaponSkills=ASC->Capture().EquippedWeaponSkills;Snapshot.Equipment.Reset();if(!State->SetCharacterState(Snapshot))return false;}
    Error=FText::GetEmpty();return true;
}
bool UICCodexSubsystem::IsSkillUnlocked(FName ID) const
{return !ID.IsNone()&&GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->GetSnapshot(UICWorldStateSubsystem::PlayerId()).UnlockedSkillIDs.Contains(ID);}
bool UICCodexSubsystem::IsPlayerSkillEquipped(FName ID) const
{return !ID.IsNone()&&EquippedSkills().Contains(ID);}
bool UICCodexSubsystem::UnsetPlayerSkill(FName ID)
{
    if(!IsPlayerSkillEquipped(ID))return false;FICDefinitionResult R;const auto* Skill=Definitions()->Find<FICSkillRow>(ID,R);
    auto* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;auto* Player=PC?Cast<AICPlayerCharacter>(PC->GetPawn()):nullptr;auto* ASC=Player?Player->GetICAbilitySystem():nullptr;
    if(!Skill||!ASC||!ASC->EquipWeaponSkill(SkillFamily(*Skill),NAME_None))return false;
    auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();auto Snapshot=State->GetSnapshot(UICWorldStateSubsystem::PlayerId());Snapshot.Equipment.Reset();Snapshot.EquippedWeaponSkills=ASC->Capture().EquippedWeaponSkills;
    return State->SetCharacterState(Snapshot);
}
TArray<FName> UICCodexSubsystem::ProductItems(int32 Category) const
{
    TSet<FName> Items;auto* D=Definitions();TArray<FName> IDs;FICDefinitionResult R;
    auto Add=[&](const TArray<FICItemAmount>& Yields,int32 Type){if(Category==Type)for(const auto& Item:Yields)Items.Add(Item.ItemID);};
    D->GetDefinitionIDs(EICDefinitionType::Resource,IDs);for(FName ID:IDs)if(const auto* Row=D->Find<FICResourceRow>(ID,R))Add(Row->Yields,Row->Activity==EICLifeActivity::Mining?3:0);
    IDs.Reset();D->GetDefinitionIDs(EICDefinitionType::Crop,IDs);for(FName ID:IDs)if(const auto* Row=D->Find<FICCropRow>(ID,R))Add(Row->HarvestYields,1);
    IDs.Reset();D->GetDefinitionIDs(EICDefinitionType::Fish,IDs);for(FName ID:IDs)if(const auto* Row=D->Find<FICFishRow>(ID,R);Row&&Category==2)Items.Add(Row->ItemID);
    IDs.Reset();D->GetDefinitionIDs(EICDefinitionType::Production,IDs);for(FName ID:IDs)if(const auto* Row=D->Find<FICProductionRow>(ID,R))
    {int32 Type=INDEX_NONE;switch(Row->Activity){case EICLifeActivity::Harvesting:Type=1;break;case EICLifeActivity::Fishing:Type=2;break;case EICLifeActivity::Mining:Type=3;break;case EICLifeActivity::Gathering:case EICLifeActivity::Woodcutting:Type=0;break;default:break;}if(Type>=0)Add(Row->Results,Type);}
    IDs.Reset();D->GetDefinitionIDs(EICDefinitionType::Item,IDs);for(FName ID:IDs)if(const auto* Row=D->Find<FICItemRow>(ID,R))
    {if(Row->CodexProductCategory!=EICCodexProductCategory::Automatic){Items.Remove(ID);if(int32(Row->CodexProductCategory)-1==Category)Items.Add(ID);}
     else if((Row->Category==EPlayerItemCategory::Gathering&&Category==0)||(Row->Category==EPlayerItemCategory::Fish&&Category==2))Items.Add(ID);}
    auto Result=Items.Array();Result.RemoveAll([&](FName ID){return !D->Find<FICItemRow>(ID,R);});Result.Sort(FNameLexicalLess());return Result;
}
TArray<FName> UICCodexSubsystem::Recipes(int32 Category) const
{
    TArray<FName> IDs,Result;Definitions()->GetDefinitionIDs(EICDefinitionType::Recipe,IDs);FICDefinitionResult R;
    for(FName ID:IDs)if(const auto* Recipe=Definitions()->Find<FICRecipeRow>(ID,R))
    {
        int32 Kind=Recipe->Type==EPlayerRecipeType::Cook?1:INDEX_NONE;auto Results=Recipe->Results;
        if(Results.IsEmpty())if(const auto* P=Definitions()->Find<FICProductionRow>(Recipe->ProductionID,R))Results=P->Results;
        if(Kind==INDEX_NONE)for(const auto& Yield:Results)if(const auto* Item=Definitions()->Find<FICItemRow>(Yield.ItemID,R);Item&&Item->EquipSlot==EPlayerEquipSlot::Weapon){Kind=0;break;}
        if(Kind==Category)Result.Add(ID);
    }
    Result.Sort(FNameLexicalLess());return Result;
}
FText UICCodexSubsystem::SkillDescription(FName ID) const
{
    FICDefinitionResult R;const auto* Skill=Definitions()->Find<FICSkillRow>(ID,R);if(!Skill)return FText::GetEmpty();
    FString Text=DefinitionText(Skill->DescriptionKey).ToString();
    Text+=FString::Printf(TEXT("\n\nMP %d   재사용 %.1f초   시전 %.1f초"),Skill->MPCost,Skill->CooldownPlaySeconds,Skill->CastPlaySeconds);
    for(const auto& Hit:Skill->Hits)if(const auto* Effect=Definitions()->Find<FICSkillEffectRow>(Hit.EffectID,R))
    {
        const auto& M=Effect->Magnitude;
        if(Effect->Kind==EICSkillEffectKind::Damage||Effect->Kind==EICSkillEffectKind::Heal)
            Text+=FString::Printf(TEXT("\n%s: %s × %.1f%% + %.1f"),Effect->Kind==EICSkillEffectKind::Heal?TEXT("회복"):TEXT("피해"),*StaticEnum<EICStat>()->GetNameStringByValue(int64(M.SourceStat)),M.Coefficient*100,M.FlatAmount);
        else if(Effect->ReviveHPRatio>0)Text+=FString::Printf(TEXT("\n부활 HP %.1f%%"),Effect->ReviveHPRatio*100);
        else if(Effect->MovementDistanceCm>0)Text+=FString::Printf(TEXT("\n이동 거리 %.1fm"),Effect->MovementDistanceCm/100);
        for(FName StatusID:Effect->AppliedStatusEffectIDs)if(const auto* Status=Definitions()->Find<FICStatusEffectRow>(StatusID,R))
        {
            const bool Shield=Status->Kind==EICStatusKind::Shield;const auto& Amount=Shield?Status->InitialShieldMagnitude:Status->PeriodicMagnitude;
            Text+=FText::Format(NSLOCTEXT("ICCodex","StatusMagnitude","\n{0}: {1} × {2}% + {3} · {4}초"),
                Shield?NSLOCTEXT("ICCodex","Shield","보호막"):NSLOCTEXT("ICCodex","PeriodicRecovery","지속 회복"),
                FText::FromString(StaticEnum<EICStat>()->GetNameStringByValue(int64(Amount.SourceStat))),FText::AsNumber(Amount.Coefficient*100),
                FText::AsNumber(Amount.FlatAmount),FText::AsNumber(Status->DurationPlaySeconds)).ToString();
            if(!Shield)Text+=FText::Format(NSLOCTEXT("ICCodex","PeriodicInterval"," ({0}초마다)"),FText::AsNumber(Status->TickIntervalPlaySeconds)).ToString();
        }
    }
    return FText::FromString(Text.TrimStartAndEnd());
}
FText UICCodexSubsystem::RecipeDescription(FName ID) const
{
    FICDefinitionResult R;const auto* Row=Definitions()->Find<FICRecipeRow>(ID,R);if(!Row||!IsRecipeKnown(ID))return FText::GetEmpty();
    auto Ingredients=Row->Ingredients,Results=Row->Results;auto Tools=Row->Tools;double Duration=Row->WorkDurationPlaySeconds,SP=Row->SPCost;
    if(const auto* Production=Definitions()->Find<FICProductionRow>(Row->ProductionID,R))
    {if(Ingredients.IsEmpty()&&Row->IngredientGroups.IsEmpty())for(const auto& M:Production->Materials)Ingredients.Add(M.Item);if(Results.IsEmpty())Results=Production->Results;if(Tools.IsEmpty())Tools=Production->Tools;}
    FString Text=DefinitionText(Row->DescriptionKey).ToString()+TEXT("\n\n재료");
    if(Ingredients.IsEmpty()&&Row->IngredientGroups.IsEmpty())Text+=TEXT("\n없음");else for(const auto& I:Ingredients)Text+=FString::Printf(TEXT("\n%s × %d"),*ItemName(I.ItemID).ToString(),I.Count);
    for(const auto& Group:Row->IngredientGroups)Text+=FText::Format(NSLOCTEXT("ICCodex","AnyIngredient","\n{0} × {1} (종류 혼합 가능)"),Group.DisplayName,FText::AsNumber(Group.Count)).ToString();
    if(!Tools.IsEmpty()){Text+=TEXT("\n\n필요 도구");for(const auto& I:Tools)Text+=FString::Printf(TEXT("\n%s × %d"),*ItemName(I.ItemID).ToString(),I.Count);}
    Text+=TEXT("\n\n제작 결과");for(const auto& I:Results)Text+=FString::Printf(TEXT("\n%s × %d"),*ItemName(I.ItemID).ToString(),I.Count);
    Text+=FString::Printf(TEXT("\n\n제작 시간 %.1f초   SP %.1f"),Duration,SP);return FText::FromString(Text.TrimStartAndEnd());
}

TArray<FName> UICCodexSubsystem::GetNPCIDs() const
{
    TArray<FName> IDs;Definitions()->GetDefinitionIDs(EICDefinitionType::NPC,IDs);FICDefinitionResult R;
    IDs.RemoveAll([&](FName ID){const auto* NPC=Definitions()->Find<FICNPCRow>(ID,R);return !NPC||NPC->Type==EICNPCType::Background;});
    IDs.Sort(FNameLexicalLess());return IDs;
}

int32 UICCodexSubsystem::NPCAffinity(FName ID) const
{
    auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();
    if(State->GetStoredCharacter(ID))return State->GetSnapshot(ID).Affinity;
    FICDefinitionResult R;const auto* NPC=Definitions()->Find<FICNPCRow>(ID,R);return NPC?NPC->InitialAffinity:0;
}

FText UICCodexSubsystem::NPCDescription(FName ID) const
{
    FICDefinitionResult R;const auto* NPC=Definitions()->Find<FICNPCRow>(ID,R);if(!NPC)return FText::GetEmpty();
    const auto Character=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->GetSnapshot(ID);
    const auto Relationship=FText::Format(NSLOCTEXT("ICCodex","NPCAffinity","호감도 {0} / 500"),FText::AsNumber(NPCAffinity(ID)));
    FText Detail=DefinitionText(NPC->DescriptionKey);
    if(NPC->bRecruitable)
    {
        const auto Status=Character.Mode==EICCharacterMode::Companion?NSLOCTEXT("ICCodex","NPCInParty","현재 동료"):
            NPCAffinity(ID)>=300?NSLOCTEXT("ICCodex","NPCRecruitReady","동료 편입 가능"):
            NSLOCTEXT("ICCodex","NPCRecruitNeedsAffinity","동료 편입에 호감도 300 필요");
        const auto* Combat=Definitions()->Find<FICRecruitCombatRow>(NPC->RecruitCombatID,R);
        const auto* Skill=Combat?Definitions()->Find<FICSkillRow>(Combat->UniqueSkillID,R):nullptr;
        const FText SkillName=Skill?DefinitionText(Skill->NameKey,Skill->ID):FText::GetEmpty();
        return FText::Format(NSLOCTEXT("ICCodex","NPCRecruitDetail","{0}\n\n{1}\n{2}\n고유 스킬: {3}"),Detail,Relationship,Status,SkillName);
    }
    return FText::Format(NSLOCTEXT("ICCodex","NPCResidentDetail","{0}\n\n{1}\n마을 주민"),Detail,Relationship);
}
