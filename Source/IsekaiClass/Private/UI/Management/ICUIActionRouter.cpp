#include "UI/Management/ICUIActionRouter.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Input/CommonAnalogCursor.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SViewport.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/SWindow.h"

namespace
{
class FICMenuAnalogCursor : public FCommonAnalogCursor
{
public:
    explicit FICMenuAnalogCursor(const UCommonUIActionRouterBase& Router) : FCommonAnalogCursor(Router) {}
    virtual void Tick(float DeltaTime, FSlateApplication& Slate, TSharedRef<ICursor> Cursor) override
    {
        // Menu stick navigation is handled as focus input; it must never move
        // the physical pointer or switch it to a hidden virtual pointer.
        const auto* Player=ActionRouter.GetLocalPlayer();
        const auto* UI=Player?Player->GetSubsystem<UICUIManagerSubsystem>():nullptr;
        if(!UI||!UI->IsPointerUIActive())FCommonAnalogCursor::Tick(DeltaTime,Slate,Cursor);
        RefreshCursorVisibility();
    }
    virtual bool ShouldVirtualAcceptSimulateMouseButton(const FKeyEvent&, EInputEvent) const override { return false; }
    virtual bool HandleAnalogInputEvent(FSlateApplication& Slate, const FAnalogInputEvent& Event) override
    {
        const auto* Player=ActionRouter.GetLocalPlayer();
        const auto* UI=Player?Player->GetSubsystem<UICUIManagerSubsystem>():nullptr;
        const auto User=Slate.GetUser(GetOwnerUserIndex());
        const auto Viewport=GetViewportClient()?GetViewportClient()->GetGameViewportWidget():nullptr;
        const bool OwnInput=Event.GetUserIndex()==GetOwnerUserIndex()&&User&&Viewport&&User->IsWidgetInFocusPath(Viewport);
        // CommonInput 5.8 detects buttons and pointer events, but its analog handler
        // does not refresh the active device. Deliberate stick/trigger input must too.
        if(OwnInput&&Event.GetKey().IsGamepadKey()&&FMath::Abs(Event.GetAnalogValue())>.25f)
            ActionRouter.GetInputSubsystem().SetCurrentInputType(ECommonInputType::Gamepad);
        const bool Horizontal=Event.GetKey()==EKeys::Gamepad_LeftX;
        if(OwnInput&&UI&&UI->IsPointerUIActive()&&(Horizontal||Event.GetKey()==EKeys::Gamepad_LeftY))
        {
            const int32 Axis=Horizontal?0:1;
            const float Value=Event.GetAnalogValue();
            if(FMath::Abs(Value)<.25f){NextStickNavigation[Axis]=0;return true;}
            const double Now=FPlatformTime::Seconds();
            if(FMath::Abs(Value)>.55f&&Now>=NextStickNavigation[Axis])
            {
                const bool Repeat=NextStickNavigation[Axis]!=0;
                NextStickNavigation[Axis]=Now+(Repeat?.16:.28);
                const FKey Key=Horizontal?(Value>0?EKeys::Gamepad_DPad_Right:EKeys::Gamepad_DPad_Left)
                    :(Value>0?EKeys::Gamepad_DPad_Up:EKeys::Gamepad_DPad_Down);
                Slate.ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),Event.GetUserIndex(),Repeat,0,0));
                Slate.ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),Event.GetUserIndex(),false,0,0));
            }
            return true;
        }
        return FCommonAnalogCursor::HandleAnalogInputEvent(Slate,Event);
    }
protected:
    virtual void RefreshCursorVisibility() override
    {
        const auto* Player = ActionRouter.GetLocalPlayer();
        const auto* UI = Player ? Player->GetSubsystem<UICUIManagerSubsystem>() : nullptr;
        const bool Show = UI && UI->IsPointerUIActive();
        auto& Slate = FSlateApplication::Get();
        const auto User = Slate.GetUser(GetOwnerUserIndex());
        const auto* Client = GetViewportClient();
        const auto Focused = User ? User->GetFocusedWidget() : nullptr;
        // UMG focus may be on the viewport overlay rather than SViewport itself.
        // Only the owning game window may change the shared physical cursor.
        const bool OwnWindow=User&&Client&&Focused&&Slate.FindWidgetWindow(Focused.ToSharedRef())==Client->GetWindow();
        if (!OwnWindow) return;
        // CommonInput replaces the cursor with a faux cursor on every switch to
        // gamepad. Keep the real mouse connected while a pointer UI is open.
        if(Show&&User==Slate.GetCursorUser())Slate.UsePlatformCursorForCursorUser(true);
        User->SetCursorVisibility(Show);
        // Restore the hardware cursor too: CommonUI's default implementation hides it
        // separately from PlayerController::bShowMouseCursor when a gamepad is used.
        Slate.SetPlatformCursorVisibility(Show);
    }
private:
    double NextStickNavigation[2]={0,0};
};
}

TSharedRef<FCommonAnalogCursor> UICUIActionRouter::MakeAnalogCursor() const
{
    return FCommonAnalogCursor::CreateAnalogCursor<FICMenuAnalogCursor>(*this);
}
