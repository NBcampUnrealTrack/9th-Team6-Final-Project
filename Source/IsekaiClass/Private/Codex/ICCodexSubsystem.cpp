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
{return Index>=0&&Index<PetCount?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState().Codex.PetEssence.FindRef(Index):0;}
bool UICCodexSubsystem::IsPetUnlocked(int32 Index) const{return Index>=0&&Index<PetCount&&GetPetEssence(Index)>=EssenceRequired;}
bool UICCodexSubsystem::AddPetEssence(int32 Index,int32 Amount)
{
    if(Index<0||Index>=PetCount||Amount<=0)return false;auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();auto Next=State->ExportState();
    if(!Next.bPlayerCreated)return false;
    auto& Count=Next.Codex.PetEssence.FindOrAdd(Index);Count=static_cast<int32>(FMath::Min<int64>(MAX_int32,int64(Count)+Amount));
    return State->CommitGameplayTransaction(Next,FGuid::NewGuid());
}
const FICMonsterRow* UICCodexSubsystem::FindPet(int32 Index) const
{
    if(Index<0||Index>=PetCount)return nullptr;return PetDefinitions()[Index];
}
TArray<const FICMonsterRow*> UICCodexSubsystem::PetDefinitions() const
{
    TArray<const FICMonsterRow*> Pets;Pets.SetNumZeroed(PetCount);TArray<FName> IDs;Definitions()->GetDefinitionIDs(EICDefinitionType::Monster,IDs);IDs.Sort(FNameLexicalLess());FICDefinitionResult R;
    for(FName ID:IDs)if(const auto* Row=Definitions()->Find<FICMonsterRow>(ID,R);Row&&Row->PetCodexIndex>=0&&Row->PetCodexIndex<PetCount&&!Pets[Row->PetCodexIndex])Pets[Row->PetCodexIndex]=Row;
    return Pets;
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
{if(Skill.RequiredWeaponFamily!=EICWeaponFamily::None)return Skill.RequiredWeaponFamily;FICDefinitionResult R;const auto* Class=Definitions()->Find<FICClassRow>(Skill.ClassID,R);return Class?Class->WeaponFamily:EICWeaponFamily::None;}
TArray<FName> UICCodexSubsystem::SkillCells() const
{
    TArray<FName> Cells;Cells.SetNum(60);TArray<FName> IDs;Definitions()->GetDefinitionIDs(EICDefinitionType::Skill,IDs);IDs.Sort(FNameLexicalLess());FICDefinitionResult R;
    // Explicit positions take precedence; unspecified skills fill the remaining cells in stable ID order.
    for(int32 Pass=0;Pass<2;++Pass)for(FName ID:IDs)if(const auto* S=Definitions()->Find<FICSkillRow>(ID,R);S&&S->OwnerType==EICSkillOwner::CharacterClass)
    {
        const int32 Family=int32(SkillFamily(*S))-1;if(Family<0||Family>=4||(Pass==0)!=(S->CodexIndex>=0))continue;
        int32 Position=S->CodexIndex;
        if(Pass==1){for(Position=0;Position<15;++Position)if(Cells[(Position/3)*12+Family*3+Position%3].IsNone())break;}
        if(Position>=0&&Position<15){auto& Cell=Cells[(Position/3)*12+Family*3+Position%3];if(Cell.IsNone())Cell=ID;}
    }
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
    const auto* S=Definitions()->Find<FICSkillRow>(ID,R);if(!Player||!Player->Inventory||!S||S->OwnerType!=EICSkillOwner::CharacterClass)return false;
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
        int32 Kind=Recipe->Type==EPlayerRecipeType::Cook?2:1;auto Results=Recipe->Results;
        if(Results.IsEmpty())if(const auto* P=Definitions()->Find<FICProductionRow>(Recipe->ProductionID,R))Results=P->Results;
        if(Kind==1)for(const auto& Yield:Results)if(const auto* Item=Definitions()->Find<FICItemRow>(Yield.ItemID,R);Item&&Item->EquipSlot==EPlayerEquipSlot::Weapon){Kind=0;break;}
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
    }
    return FText::FromString(Text.TrimStartAndEnd());
}
FText UICCodexSubsystem::RecipeDescription(FName ID) const
{
    FICDefinitionResult R;const auto* Row=Definitions()->Find<FICRecipeRow>(ID,R);if(!Row||!IsRecipeKnown(ID))return FText::GetEmpty();
    auto Ingredients=Row->Ingredients,Results=Row->Results;auto Tools=Row->Tools;double Duration=Row->WorkDurationPlaySeconds,SP=Row->SPCost;
    if(const auto* Production=Definitions()->Find<FICProductionRow>(Row->ProductionID,R))
    {if(Ingredients.IsEmpty())for(const auto& M:Production->Materials)Ingredients.Add(M.Item);if(Results.IsEmpty())Results=Production->Results;if(Tools.IsEmpty())Tools=Production->Tools;Duration=Production->DurationPlaySeconds;SP=Production->SPCost;}
    FString Text=DefinitionText(Row->DescriptionKey).ToString()+TEXT("\n\n재료");
    if(Ingredients.IsEmpty())Text+=TEXT("\n없음");else for(const auto& I:Ingredients)Text+=FString::Printf(TEXT("\n%s × %d"),*ItemName(I.ItemID).ToString(),I.Count);
    if(!Tools.IsEmpty()){Text+=TEXT("\n\n필요 도구");for(const auto& I:Tools)Text+=FString::Printf(TEXT("\n%s × %d"),*ItemName(I.ItemID).ToString(),I.Count);}
    Text+=TEXT("\n\n제작 결과");for(const auto& I:Results)Text+=FString::Printf(TEXT("\n%s × %d"),*ItemName(I.ItemID).ToString(),I.Count);
    Text+=FString::Printf(TEXT("\n\n제작 시간 %.1f초   SP %.1f"),Duration,SP);return FText::FromString(Text.TrimStartAndEnd());
}
