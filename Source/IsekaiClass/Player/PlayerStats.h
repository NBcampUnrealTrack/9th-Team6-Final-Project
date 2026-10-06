#pragma once
#include "CoreMinimal.h"
#include "PlayerStats.generated.h"

UENUM(BlueprintType)
enum class EPlayerMainStat : uint8
{
    Strength,
    Dexterity,
    Intelligence,
    Constitution,
    Charisma,
    Luck,
    None
};

USTRUCT(BlueprintType)
struct FPlayerMainStats
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) int32 Strength=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) int32 Dexterity=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) int32 Intelligence=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) int32 Constitution=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) int32 Charisma=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) int32 Luck=1;

    int32 Get(EPlayerMainStat Stat) const
    {
        switch(Stat)
        {
        case EPlayerMainStat::Strength:return Strength;
        case EPlayerMainStat::Dexterity:return Dexterity;
        case EPlayerMainStat::Intelligence:return Intelligence;
        case EPlayerMainStat::Constitution:return Constitution;
        case EPlayerMainStat::Charisma:return Charisma;
        case EPlayerMainStat::Luck:return Luck;
        default:return 0;
        }
    }

    int32 Add(EPlayerMainStat Stat,int32 Amount)
    {
        int32* Value=nullptr;
        switch(Stat)
        {
        case EPlayerMainStat::Strength:Value=&Strength;break;
        case EPlayerMainStat::Dexterity:Value=&Dexterity;break;
        case EPlayerMainStat::Intelligence:Value=&Intelligence;break;
        case EPlayerMainStat::Constitution:Value=&Constitution;break;
        case EPlayerMainStat::Charisma:Value=&Charisma;break;
        case EPlayerMainStat::Luck:Value=&Luck;break;
        default:return 0;
        }
        const int32 Before=*Value;*Value=FMath::Clamp(Before+FMath::Max(0,Amount),1,10000);return *Value-Before;
    }

    bool IsValid() const
    {
        return Strength>=1&&Strength<=10000&&Dexterity>=1&&Dexterity<=10000&&Intelligence>=1&&Intelligence<=10000&&Constitution>=1&&Constitution<=10000&&Charisma>=1&&Charisma<=10000&&Luck>=1&&Luck<=10000;
    }
};
