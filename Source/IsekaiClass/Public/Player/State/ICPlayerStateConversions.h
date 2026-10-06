#pragma once
#include "Player/Stats/ICPlayerStats.h"
#include "Life/Survival/ICPlayerSurvival.h"
#include "World/State/ICWorldStateTypes.h"

namespace ICPlayerStateConversions
{
inline FPlayerMainStats Main(const FICMainStats& In)
{ FPlayerMainStats Out; Out.Strength=In.STR;Out.Dexterity=In.DEX;Out.Intelligence=In.INT;Out.Constitution=In.CON;Out.Charisma=In.CHA;Out.Luck=In.LUK;return Out; }
inline FICMainStats Main(const FPlayerMainStats& In)
{ FICMainStats Out; Out.STR=In.Strength;Out.DEX=In.Dexterity;Out.INT=In.Intelligence;Out.CON=In.Constitution;Out.CHA=In.Charisma;Out.LUK=In.Luck;return Out; }
inline FPlayerSurvivalState Survival(const FICCharacterState& In)
{ FPlayerSurvivalState Out;Out.HP=In.HP;Out.MP=In.MP;Out.SP=In.SP;Out.MaxHP=In.MaxHP;Out.MaxMP=In.MaxMP;Out.MaxSP=In.MaxSP;return Out; }
inline void Apply(const FPlayerMainStats& MainStats,const FPlayerSurvivalState& Survival,FICCharacterState& Out)
{ Out.MainStats=Main(MainStats);Out.HP=Survival.HP;Out.MP=Survival.MP;Out.SP=Survival.SP;Out.MaxHP=Survival.MaxHP;Out.MaxMP=Survival.MaxMP;Out.MaxSP=Survival.MaxSP; }
}
