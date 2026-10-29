#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"

/** Runtime instances always use saved WBP designs; C++ supplies only data and behavior. */
namespace ICWidgetTemplates
{
    template<class T=UUserWidget> T* Create(UUserWidget* Owner,const TCHAR* Asset,FName Name=NAME_None)
    {
        const auto Path=FString::Printf(TEXT("/Game/IsekaiClass/UI/Widget/%s.%s_C"),Asset,Asset);
        auto* Type=LoadClass<T>(nullptr,*Path);
        if(!ensureMsgf(Type,TEXT("Required WBP is missing: %s"),*Path))return nullptr;
        return CreateWidget<T>(Owner,Type,Name);
    }
    inline UUserWidget* Text(UUserWidget* Owner,const FText& Value)
    {
        auto* Row=Create(Owner,TEXT("WBP_IC_TextLine"));
        if(Row)if(auto* Label=Cast<UTextBlock>(Row->GetWidgetFromName(TEXT("TXT_Label"))))Label->SetText(Value);
        return Row;
    }
}
