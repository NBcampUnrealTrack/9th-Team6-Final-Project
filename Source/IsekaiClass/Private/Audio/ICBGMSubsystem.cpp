#include "Audio/ICBGMSubsystem.h"
#include "Audio/ICBGMZone.h"
#include "Core/Controller/ICPlayerController.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "EngineUtils.h"
#include "AudioDevice.h"
#include "Sound/SoundMix.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "HAL/PlatformTime.h"
#include "Core/Flow/ICGameFlowSubsystem.h"

UICBGMSubsystem::UICBGMSubsystem()
{MusicTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/Audio/DT_IC_BGM.DT_IC_BGM")));}
bool UICBGMSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{return Type==EWorldType::Game||Type==EWorldType::PIE;}
TStatId UICBGMSubsystem::GetStatId() const
{RETURN_QUICK_DECLARE_CYCLE_STAT(UICBGMSubsystem,STATGROUP_Tickables);}
FName UICBGMSubsystem::ResolveMusicRow(FVector Position,bool InCombat) const
{
    if(InCombat)return CombatRow;
    if(!OverrideRow.IsNone())return OverrideRow;
    const AICBGMZone* Best=nullptr;double BestVolume=MAX_dbl;
    if(GetWorld())for(TActorIterator<AICBGMZone> It(GetWorld());It;++It)
    {
        if(It->MusicRowID.IsNone()||!It->ContainsPosition(Position))continue;
        const FVector E=It->Bounds->GetScaledBoxExtent();const double Volume=E.X*E.Y*E.Z;
        if(!Best||It->Priority>Best->Priority||(It->Priority==Best->Priority&&(Volume<BestVolume||(Volume==BestVolume&&It->GetName()<Best->GetName()))))
        {Best=*It;BestVolume=Volume;}
    }
    return Best?Best->MusicRowID:DefaultAreaRow;
}
void UICBGMSubsystem::Tick(float DeltaSeconds)
{
    const auto* PC=Cast<AICPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    if(PC&&PC->IsDead()){if(!bDeathMuted)SilenceForDeath();return;}
    if(bDeathMuted){UGameplayStatics::PopSoundMixModifier(GetWorld(),DeathMute);if(auto Audio=GetWorld()->GetAudioDevice())Audio->SetTransientPrimaryVolume(1.f);bDeathMuted=false;RefreshIn=0;}
    if(FPlatformTime::Seconds()<SilentUntil)return;
    RefreshIn-=DeltaSeconds;if(RefreshIn>0)return;RefreshIn=.2f;
    const auto* Flow=GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>():nullptr;
    if(Flow&&(Flow->Phase==EICGameFlowPhase::Prologue||Flow->bPendingNewGame)){ChangeMusic(PrologueRow);return;}
    if(PC&&(PC->Menu==TEXT("Create")||(Flow&&Flow->Phase==EICGameFlowPhase::Questions))){ChangeMusic(TEXT("BGM_CharacterCreate"));return;}
    if(PC&&PC->IsMainMenuLevel()){ChangeMusic(TEXT("BGM_MainMenu"));return;}
    if(!PC||!PC->bCreated||!PC->GetPawn()){ChangeMusic(NAME_None);return;}
    ChangeMusic(ResolveMusicRow(PC->GetPawn()->GetActorLocation(),PC->bInCombat));
}
void UICBGMSubsystem::StopMusic()
{
    if(Current){Current->OnAudioFinished.RemoveDynamic(this,&ThisClass::MusicFinished);Current->Stop();Current->DestroyComponent();}
    Current=nullptr;CurrentSource=nullptr;CurrentRow=NAME_None;RefreshIn=0;
}
void UICBGMSubsystem::ResetForRestore()
{
    StopMusic();SilentUntil=FPlatformTime::Seconds()+.4;
    if(auto Audio=GetWorld()->GetAudioDevice())Audio->Flush(GetWorld());
}
void UICBGMSubsystem::SilenceForDeath()
{
    StopMusic();if(auto Audio=GetWorld()->GetAudioDevice()){Audio->Flush(GetWorld());Audio->SetTransientPrimaryVolume(0.f);}
    if(!DeathMute)
    {
        DeathMute=NewObject<USoundMix>(this);DeathMute->Duration=-1;DeathMute->FadeInTime=0;DeathMute->FadeOutTime=0;
        FSoundClassAdjuster Adjuster;Adjuster.SoundClassObject=LoadObject<USoundClass>(nullptr,TEXT("/Game/IsekaiClass/Audio/Settings/SC_Master.SC_Master"));Adjuster.VolumeAdjuster=0;Adjuster.bApplyToChildren=true;
        DeathMute->SoundClassEffects.Add(Adjuster);
    }
    if(!bDeathMuted)UGameplayStatics::PushSoundMixModifier(GetWorld(),DeathMute);bDeathMuted=true;
}
bool UICBGMSubsystem::CanPlayEffects(const UWorld* World)
{
    if(!World)return false;
    const auto* System=World->GetSubsystem<UICBGMSubsystem>();if(System&&(System->bDeathMuted||FPlatformTime::Seconds()<System->SilentUntil))return false;
    const auto* GI=World->GetGameInstance();const auto* State=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    const auto* PC=Cast<AICPlayerController>(UGameplayStatics::GetPlayerController(World,0));
    return (!PC||!PC->IsDead())&&(!State||!State->IsRestoringState());
}
void UICBGMSubsystem::ChangeMusic(FName RowID)
{
    auto* Table=MusicTable.LoadSynchronous();const auto* Row=Table&&!RowID.IsNone()?Table->FindRow<FICBGMRow>(RowID,TEXT("BGM"),false):nullptr;
    auto* Sound=Row?Row->Music.LoadSynchronous():nullptr;
    if(CurrentRow==RowID&&CurrentSource==Sound)
    {if(Current&&Row)Current->SetVolumeMultiplier(FMath::Max(0.f,Row->Volume));return;}
    if(Current)
    {
        Current->OnAudioFinished.RemoveDynamic(this,&ThisClass::MusicFinished);
        Current->bAutoDestroy=true;Current->FadeOut(FMath::Max(0.f,CurrentFadeOut),0.f);Current=nullptr;
    }
    CurrentRow=RowID;CurrentSource=Sound;if(!Row||!Sound)return;
    USoundBase* Playback=Sound;
    if(Row->bLoop)if(auto* Wave=Cast<USoundWave>(Sound))
    {
        // Runtime wrapper keeps looping local to BGM; the imported sound asset remains reusable.
        auto* Cue=NewObject<USoundCue>(this);auto* Node=NewObject<USoundNodeWavePlayer>(Cue);
        Node->SetSoundWave(Wave);Node->bLooping=true;Cue->FirstNode=Node;Cue->VolumeMultiplier=1.f;Playback=Cue;
    }
    Current=UGameplayStatics::CreateSound2D(GetWorld(),Playback,FMath::Max(0.f,Row->Volume),1.f,0.f,nullptr,false,false);
    bCurrentLoop=Row->bLoop;CurrentFadeOut=Row->FadeOutSeconds;
    if(Current)
    {
        Current->bIsUISound=true;Current->SoundClassOverride=LoadObject<USoundClass>(nullptr,TEXT("/Game/IsekaiClass/Audio/Settings/SC_Music.SC_Music"));
        Current->OnAudioFinished.AddUniqueDynamic(this,&ThisClass::MusicFinished);
        Current->FadeIn(FMath::Max(0.f,Row->FadeInSeconds),FMath::Max(0.f,Row->Volume));
    }
}
void UICBGMSubsystem::MusicFinished(){if(Current&&bCurrentLoop)Current->Play();}
void UICBGMSubsystem::Deinitialize()
{
    StopMusic();if(bDeathMuted&&DeathMute){UGameplayStatics::PopSoundMixModifier(GetWorld(),DeathMute);if(auto Audio=GetWorld()->GetAudioDevice())Audio->SetTransientPrimaryVolume(1.f);}Super::Deinitialize();
}
