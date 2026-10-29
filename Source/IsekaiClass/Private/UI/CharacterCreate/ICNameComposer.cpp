#include "UI/CharacterCreate/ICNameComposer.h"

namespace
{
const FString Initials = TEXT("ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ");
const FString Vowels = TEXT("ㅏㅐㅑㅒㅓㅔㅕㅖㅗㅘㅙㅚㅛㅜㅝㅞㅟㅠㅡㅢㅣ");
const FString Finals = TEXT(" ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ");
int32 Index(const FString& Set, TCHAR C) { return Set.Find(FString::Chr(C)); }
int32 Compound(int32 A, int32 B, bool Vowel)
{
    const int32 V[][3]={{8,0,9},{8,1,10},{8,20,11},{9,20,10},{13,4,14},{13,5,15},{13,20,16},{14,20,15},{18,20,19}};
    const int32 T[][3]={{1,19,3},{4,22,5},{4,27,6},{8,1,9},{8,16,10},{8,17,11},{8,19,12},{8,25,13},{8,26,14},{8,27,15},{17,19,18}};
    if(Vowel){for(const auto& Row:V)if(Row[0]==A&&Row[1]==B)return Row[2];}
    else {for(const auto& Row:T)if(Row[0]==A&&Row[1]==B)return Row[2];}
    return INDEX_NONE;
}
void SplitFinal(int32 T,int32& Keep,int32& Move)
{
    Keep=0;Move=T;
    const int32 Pairs[][3]={{3,1,19},{5,4,22},{6,4,27},{9,8,1},{10,8,16},{11,8,17},{12,8,19},{13,8,25},{14,8,26},{15,8,27},{18,17,19}};
    for(const auto& Row:Pairs)if(Row[0]==T){Keep=Row[1];Move=Row[2];return;}
}
TCHAR Syllable(int32 L,int32 V,int32 T) { return static_cast<TCHAR>(0xAC00+(L*21+V)*28+T); }
}

bool FICNameComposer::Append(TCHAR C)
{
    const FString Before=Text;
    const int32 L=Index(Initials,C),V=Index(Vowels,C),T=Index(Finals,C);
    bool Composed=false;
    if(!Text.IsEmpty())
    {
        TCHAR& Last=Text[Text.Len()-1];const int32 Code=Last-0xAC00;
        if(Code>=0&&Code<11172)
        {
            const int32 OldL=Code/588,OldV=(Code/28)%21,OldT=Code%28;
            if(V!=INDEX_NONE)
            {
                if(OldT==0)
                {const int32 NewV=Compound(OldV,V,true);if(NewV!=INDEX_NONE){Last=Syllable(OldL,NewV,0);Composed=true;}}
                else
                {
                    int32 Keep,Move;SplitFinal(OldT,Keep,Move);
                    const int32 NewL=Index(Initials,Finals[Move]);
                    if(NewL!=INDEX_NONE){Last=Syllable(OldL,OldV,Keep);Text.AppendChar(Syllable(NewL,V,0));Composed=true;}
                }
            }
            else if(L!=INDEX_NONE&&T>0)
            {
                const int32 NewT=OldT==0?T:Compound(OldT,T,false);
                if(NewT!=INDEX_NONE){Last=Syllable(OldL,OldV,NewT);Composed=true;}
            }
        }
        else if(V!=INDEX_NONE)
        {
            const int32 OldL=Index(Initials,Last);
            if(OldL!=INDEX_NONE){Last=Syllable(OldL,V,0);Composed=true;}
            else {const int32 NewV=Compound(Index(Vowels,Last),V,true);if(NewV!=INDEX_NONE){Last=Vowels[NewV];Composed=true;}}
        }
    }
    if(!Composed)Text.AppendChar(C);
    if(Text.Len()>12){Text=Before;return false;}
    History.Add(Before);return true;
}
void FICNameComposer::Backspace()
{
    if(!History.IsEmpty())Text=History.Pop();
    else if(!Text.IsEmpty())Text.LeftChopInline(1);
}
