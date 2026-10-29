#include "UI/Framework/ICUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"

namespace
{
// w_menu_item_template uses this flat panel for both normal and selected rows.
FSlateBrush MorpheusBrush(FLinearColor Color)
{
    FSlateBrush Brush;
    Brush.SetResourceObject(LoadObject<UTexture2D>(nullptr,TEXT("/Game/IsekaiClass/UI/SampleUI/MorpheusGUI/textures/Panels/t_BasicPanel.t_BasicPanel")));
    Brush.DrawAs=ESlateBrushDrawType::Image;Brush.ImageSize=FVector2D(32,32);Brush.TintColor=Color;
    return Brush;
}
void Visit(UWidget* Widget,TFunctionRef<void(UWidget*)> Action)
{
    if(!Widget)return;
    if(auto* User=Cast<UUserWidget>(Widget))
    {
        // Menu styling must never enter the Firefight HUD or its sample widgets.
        if(User->GetClass()->GetPathName().Contains(TEXT("/SampleUI/FirefightUI/")))return;
        Action(Widget);if(User->WidgetTree)Visit(User->WidgetTree->RootWidget,Action);
    }
    else {Action(Widget);if(auto* Panel=Cast<UPanelWidget>(Widget))for(auto* Child:Panel->GetAllChildren())Visit(Child,Action);}
}
TMap<TWeakObjectPtr<UButton>,bool> FocusedButtons;
TMap<TWeakObjectPtr<UButton>,FButtonStyle> AuthoredStyles;
}
namespace ICUITheme
{
FSlateBrush PanelBrush(){return MorpheusBrush(FLinearColor::White);}
void SetSelected(UButton* Button,bool Selected)
{
    if(!Button)return;
    if(!AuthoredStyles.Contains(Button))AuthoredStyles.Add(Button,Button->GetStyle());
    auto Style=AuthoredStyles.FindChecked(Button);
    if(Selected)Style.Normal=Style.Hovered;
    Button->SetStyle(Style);
}
void Apply(UWidgetTree* Tree)
{
    if(!Tree)return;
    Visit(Tree->RootWidget,[](UWidget* Widget)
    {
        if(auto* Button=Cast<UButton>(Widget))
        {
            auto Style=Button->GetStyle();
            Style.Normal=MorpheusBrush(FLinearColor(.107f,.175f,.258f,.22f));
            Style.Hovered=MorpheusBrush(FLinearColor(.258f,.896f,1.f,.32f));
            Style.Pressed=MorpheusBrush(FLinearColor(.258f,.896f,1.f,.52f));
            Style.Disabled=MorpheusBrush(FLinearColor(.08f,.1f,.14f,.12f));
            Button->SetStyle(Style);Button->SetBackgroundColor(FLinearColor::White);FocusedButtons.Remove(Button);
        }
        else if(auto* Border=Cast<UBorder>(Widget))
        {
            const FString Name=Border->GetName();
            const auto* Texture=Cast<UTexture2D>(Border->Background.GetResourceObject());
            const bool Firefight=Texture&&Texture->GetPathName().Contains(TEXT("/SampleUI/FirefightUI/"));
            if(Firefight||Name==TEXT("Card")||Name.Contains(TEXT("Frame"))||Name.Contains(TEXT("Panel"))||Name==TEXT("MenuCard"))
            {Border->SetBrush(PanelBrush());Border->SetBrushColor(FLinearColor(.007f,.016f,.026f,.96f));}
        }
    });
}
void UpdateFocus(UWidget* Root)
{
    Visit(Root,[](UWidget* Widget)
    {
        auto* Button=Cast<UButton>(Widget);if(!Button)return;
        const bool Focus=Button->HasKeyboardFocus()||Button->HasFocusedDescendants();
        const auto* Previous=FocusedButtons.Find(Button);
        if(!Previous&&!Focus)return;
        if(Previous&&*Previous==Focus)return;
        FocusedButtons.Add(Button,Focus);SetSelected(Button,Focus);
    });
    if(FocusedButtons.Num()>1024)for(auto It=FocusedButtons.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    if(AuthoredStyles.Num()>1024)for(auto It=AuthoredStyles.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
}
}
