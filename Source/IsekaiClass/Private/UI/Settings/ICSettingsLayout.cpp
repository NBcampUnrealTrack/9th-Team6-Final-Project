#include "UI/Settings/ICSettingsLayout.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#define LOCTEXT_NAMESPACE "ICSettings"

const TArray<FText>& ICSettingsLayout::Tabs()
{
    static const TArray<FText> Values={LOCTEXT("TabCombat","전투"),LOCTEXT("TabControls","조작 · 키 설정"),LOCTEXT("TabGraphics","그래픽 · 화면"),LOCTEXT("TabAudio","오디오"),LOCTEXT("TabOther","기타")};return Values;
}
const TArray<FICSettingSpec>& ICSettingsLayout::Specs()
{
    static const TArray<FICSettingSpec> Result=[]()
    {
        TArray<FICSettingSpec> Out;
        auto Toggle=[&](FName ID,int32 Tab,FText Label,FText Help){Out.Add({ID,Tab,Label,Help,EICSettingControl::Toggle});};
        auto Slider=[&](FName ID,int32 Tab,FText Label,FText Help,double Min,double Max,double Step){Out.Add({ID,Tab,Label,Help,EICSettingControl::Slider,Min,Max,Step});};
        auto Choice=[&](FName ID,int32 Tab,FText Label,FText Help,TArray<FText> Names,TArray<double> Values){FICSettingSpec Spec{ID,Tab,Label,Help,EICSettingControl::Choice};Spec.Choices=MoveTemp(Names);Spec.Values=MoveTemp(Values);Out.Add(MoveTemp(Spec));};
        Toggle(TEXT("bAutoSelectTarget"),0,LOCTEXT("AutoTarget","공격 시 대상 자동 선택"),LOCTEXT("AutoTargetHelp","대상이 없을 때 카메라 앞의 적을 선택합니다. 끄면 바라보는 방향으로 자유롭게 공격합니다."));
        Toggle(TEXT("bCombatFacing"),0,LOCTEXT("CombatFacing","전투 중 조준 방향 유지"),LOCTEXT("CombatFacingHelp","전투 이동 중 근접 무기는 선택한 적, 원거리 무기는 화면 중앙을 바라봅니다. 끄면 이동 방향으로 회전합니다."));
        Toggle(TEXT("bProjectileAimAssist"),0,LOCTEXT("AimAssist","조준점 거리 보정"),LOCTEXT("AimAssistHelp","화면 중앙의 적이나 장애물까지 거리를 계산해 손과 카메라의 위치 차이를 보정합니다. 끄면 최대 사거리의 화면 중앙을 조준합니다."));
        Toggle(TEXT("bDamageNumbers"),0,LOCTEXT("DamageNumbers","피해량 숫자 표시"),LOCTEXT("DamageNumbersHelp","플레이어와 적에게 가한 피해량을 화면에 표시합니다."));
        Toggle(TEXT("bEnemyHealthBars"),0,LOCTEXT("EnemyHealth","적 체력 막대 표시"),LOCTEXT("EnemyHealthHelp","몬스터 머리 위 체력 막대를 표시합니다."));
        Slider(TEXT("CameraSensitivity"),1,LOCTEXT("Sensitivity","마우스 감도"),LOCTEXT("SensitivityHelp","마우스의 수평·수직 카메라 회전 속도입니다."),.1,3,.05);
        Toggle(TEXT("bInvertCameraX"),1,LOCTEXT("InvertX","카메라 좌우 반전"),LOCTEXT("InvertXHelp","마우스의 수평 회전 방향을 반대로 바꿉니다."));
        Toggle(TEXT("bInvertCameraY"),1,LOCTEXT("InvertY","카메라 상하 반전"),LOCTEXT("InvertYHelp","마우스의 수직 회전 방향을 반대로 바꿉니다."));
        Slider(TEXT("CameraFOV"),1,LOCTEXT("FOV","시야각"),LOCTEXT("FOVHelp","플레이어 카메라의 시야각입니다. 넓을수록 주변이 더 많이 보입니다."),60,110,1);
        Slider(TEXT("CameraZoomStep"),1,LOCTEXT("ZoomStep","휠 확대·축소 속도"),LOCTEXT("ZoomStepHelp","마우스 휠 한 칸으로 변경하는 카메라 거리입니다."),10,100,5);
        Choice(TEXT("Monitor"),2,LOCTEXT("Monitor","화면 / 모니터"),LOCTEXT("MonitorHelp","연결된 디스플레이 중 게임을 표시할 화면을 선택합니다."),{},{});
        Choice(TEXT("WindowMode"),2,LOCTEXT("DisplayMode","디스플레이 모드"),LOCTEXT("DisplayModeHelp","전체 화면, 전체 창, 창 모드를 선택합니다. 전체 창은 바탕 화면 해상도를 사용합니다."),{LOCTEXT("Fullscreen","전체 화면"),LOCTEXT("Borderless","전체 창"),LOCTEXT("Windowed","창 모드")},{0,1,2});
        Choice(TEXT("Resolution"),2,LOCTEXT("Resolution","해상도"),LOCTEXT("ResolutionHelp","선택한 모니터가 지원하는 해상도입니다. 전체 창 모드에서는 변경할 수 없습니다."),{},{});
        Choice(TEXT("RefreshRate"),2,LOCTEXT("RefreshRate","화면 주사율"),LOCTEXT("RefreshRateHelp","모니터의 실제 주사율(Hz)입니다. 프레임 제한과 별개이며 15초 내 유지 확인이 필요합니다."),{},{});
        Toggle(TEXT("VSync"),2,LOCTEXT("VSync","수직 동기화"),LOCTEXT("VSyncHelp","화면 찢어짐을 줄입니다. 모니터 주사율에 따라 프레임이 제한될 수 있습니다."));
        Choice(TEXT("FrameLimit"),2,LOCTEXT("FrameLimit","최대 프레임"),LOCTEXT("FrameLimitHelp","게임 렌더링의 초당 프레임 상한입니다. 실제 모니터 주사율은 변경하지 않습니다."),{LOCTEXT("Unlimited","제한 없음"),FText::AsNumber(30),FText::AsNumber(60),FText::AsNumber(90),FText::AsNumber(120),FText::AsNumber(144),FText::AsNumber(165),FText::AsNumber(180),FText::AsNumber(240),FText::AsNumber(360)},{0,30,60,90,120,144,165,180,240,360});
        const TArray<FText> Quality={LOCTEXT("Low","낮음"),LOCTEXT("Medium","보통"),LOCTEXT("High","높음"),LOCTEXT("Epic","매우 높음")};
        auto Presets=Quality;Presets.Add(LOCTEXT("Custom","사용자 지정"));
        Choice(TEXT("Quality"),2,LOCTEXT("Quality","그래픽 프리셋"),LOCTEXT("QualityHelp","아래 세부 품질을 한 번에 변경합니다. 개별 변경 시 사용자 지정으로 표시됩니다."),Presets,{0,1,2,3,-1});
        Slider(TEXT("ResolutionScale"),2,LOCTEXT("ResolutionScale","렌더링 비율 (%)"),LOCTEXT("ResolutionScaleHelp","3D 화면의 내부 해상도 비율입니다. 낮추면 성능이 개선되고 화면이 흐려집니다. UI 해상도는 유지됩니다."),50,100,1);
        Toggle(TEXT("DynamicResolution"),2,LOCTEXT("DynamicResolution","동적 해상도"),LOCTEXT("DynamicResolutionHelp","엔진과 그래픽 장치가 지원할 때 부하에 따라 내부 렌더링 해상도를 조절합니다."));
        Choice(TEXT("ViewDistance"),2,LOCTEXT("ViewDistance","시야 거리"),LOCTEXT("ViewDistanceHelp","멀리 있는 물체의 표시 거리입니다."),Quality,{0,1,2,3});
        Choice(TEXT("Texture"),2,LOCTEXT("Texture","텍스처 품질"),LOCTEXT("TextureHelp","표면 이미지의 선명도와 그래픽 메모리 사용량입니다."),Quality,{0,1,2,3});
        Choice(TEXT("Shadow"),2,LOCTEXT("Shadow","그림자 품질"),LOCTEXT("ShadowHelp","그림자 해상도와 표시 거리입니다."),Quality,{0,1,2,3});
        Choice(TEXT("GlobalIllumination"),2,LOCTEXT("GI","간접 조명 품질"),LOCTEXT("GIHelp","주변에서 반사되어 들어오는 빛의 품질입니다. 높은 단계는 성능에 영향을 줍니다."),Quality,{0,1,2,3});
        Choice(TEXT("Reflection"),2,LOCTEXT("Reflection","반사 품질"),LOCTEXT("ReflectionHelp","물과 금속 등 반사 표면의 품질입니다."),Quality,{0,1,2,3});
        Choice(TEXT("AntiAliasing"),2,LOCTEXT("AAQuality","안티앨리어싱 품질"),LOCTEXT("AAQualityHelp","물체 가장자리의 계단 현상을 줄이는 품질입니다."),Quality,{0,1,2,3});
        Choice(TEXT("AntiAliasingMethod"),2,LOCTEXT("AAMethod","안티앨리어싱 방식"),LOCTEXT("AAMethodHelp","TSR은 낮은 내부 해상도를 복원합니다. TAA는 시간 기반 보정, FXAA는 가벼운 화면 후처리입니다."),{LOCTEXT("Off","끄기"),FText::FromString(TEXT("FXAA")),FText::FromString(TEXT("TAA")),FText::FromString(TEXT("TSR"))},{0,1,2,4});
        Choice(TEXT("VisualEffect"),2,LOCTEXT("Effects","이펙트 품질"),LOCTEXT("EffectsHelp","파티클과 전투 효과의 세부 품질입니다."),Quality,{0,1,2,3});
        Choice(TEXT("PostProcessing"),2,LOCTEXT("PostProcessing","후처리 품질"),LOCTEXT("PostProcessingHelp","화면 보정과 피사계 심도 등 후처리 품질입니다."),Quality,{0,1,2,3});
        Choice(TEXT("Foliage"),2,LOCTEXT("Foliage","풀 · 수목 품질"),LOCTEXT("FoliageHelp","풀과 식생의 표시 밀도 및 거리입니다."),Quality,{0,1,2,3});
        Choice(TEXT("Shading"),2,LOCTEXT("Shading","셰이딩 품질"),LOCTEXT("ShadingHelp","표면 재질과 조명의 세부 표현입니다."),Quality,{0,1,2,3});
        Choice(TEXT("Landscape"),2,LOCTEXT("Landscape","지형 품질"),LOCTEXT("LandscapeHelp","지형의 세부 표현과 품질입니다."),Quality,{0,1,2,3});
        Toggle(TEXT("bMotionBlur"),2,LOCTEXT("MotionBlur","모션 블러"),LOCTEXT("MotionBlurHelp","빠르게 움직일 때의 잔상 효과입니다."));
        Toggle(TEXT("bBloom"),2,LOCTEXT("Bloom","빛 번짐"),LOCTEXT("BloomHelp","밝은 조명과 공격 이펙트의 빛 번짐 효과입니다."));
        Toggle(TEXT("bDepthOfField"),2,LOCTEXT("DOF","피사계 심도"),LOCTEXT("DOFHelp","초점 밖 영역의 흐림 효과를 허용합니다."));
        Toggle(TEXT("bAmbientOcclusion"),2,LOCTEXT("AO","주변광 차폐"),LOCTEXT("AOHelp","물체가 접하는 구석의 입체감을 강조합니다."));
        Slider(TEXT("DisplayGamma"),2,LOCTEXT("Gamma","화면 밝기 (감마)"),LOCTEXT("GammaHelp","게임 화면의 중간 밝기를 조절합니다."),1.8,2.6,.05);
        Slider(TEXT("MasterVolume"),3,LOCTEXT("MasterVolume","전체 음량"),LOCTEXT("MasterVolumeHelp","모든 소리의 전체 음량입니다."),0,1,.01);
        Slider(TEXT("MusicVolume"),3,LOCTEXT("MusicVolume","배경 음악"),LOCTEXT("MusicVolumeHelp","배경 음악 사운드 클래스의 음량입니다."),0,1,.01);
        Slider(TEXT("EffectsVolume"),3,LOCTEXT("EffectsVolume","효과음"),LOCTEXT("EffectsVolumeHelp","공격, 피격, 채집 등 게임 효과음의 음량입니다."),0,1,.01);
        Slider(TEXT("AmbientVolume"),3,LOCTEXT("AmbientVolume","환경음"),LOCTEXT("AmbientVolumeHelp","바람, 물소리 등 환경 사운드의 음량입니다."),0,1,.01);
        Slider(TEXT("VoiceVolume"),3,LOCTEXT("VoiceVolume","음성"),LOCTEXT("VoiceVolumeHelp","음성 사운드 클래스의 음량입니다."),0,1,.01);
        Slider(TEXT("UIVolume"),3,LOCTEXT("UIVolume","UI 효과음"),LOCTEXT("UIVolumeHelp","메뉴 선택과 버튼 사운드의 음량입니다."),0,1,.01);
        Toggle(TEXT("bMuteUnfocused"),3,LOCTEXT("MuteUnfocused","백그라운드에서 음소거"),LOCTEXT("MuteUnfocusedHelp","다른 창으로 전환하면 게임 소리를 끕니다."));
        Slider(TEXT("UIScale"),4,LOCTEXT("UIScale","HUD · 메뉴 글자 크기"),LOCTEXT("UIScaleHelp","HUD와 메뉴 안내 문구를 75%~130% 크기로 조절합니다."),.75,1.3,.05);
        Toggle(TEXT("bShowFPS"),4,LOCTEXT("FPS","FPS 표시"),LOCTEXT("FPSHelp","현재 렌더링 프레임 수를 표시합니다."));
        return Out;
    }();return Result;
}

namespace
{
UTextBlock* SettingsText(UWidgetTree* Tree,FName Name,FText Text,int32 FontSize,FLinearColor Color=FLinearColor(.91f,.93f,.96f))
{
    auto* Label=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);Label->SetText(Text);Label->SetFont(ICFrontendLayout::Font(FontSize));Label->SetColorAndOpacity(Color);Label->SetAutoWrapText(true);return Label;
}
UButton* SettingsButton(UWidgetTree* Tree,FName Name,FText Text)
{
    auto* Button=Tree->ConstructWidget<UButton>(UButton::StaticClass(),Name);Button->SetBackgroundColor(FLinearColor(.11f,.16f,.23f));
    auto* Label=SettingsText(Tree,FName(*(Name.ToString()+TEXT("_Label"))),Text,18);Label->SetAutoWrapText(false);Button->AddChild(Label);return Button;
}
}
void ICSettingsLayout::Build(UWidgetTree* Tree)
{
    auto* Back=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("SettingsBackdrop"));Back->SetBrushColor(FLinearColor(.009f,.014f,.025f,.98f));Back->SetPadding(FMargin(32));Tree->RootWidget=Back;
    auto* Scale=Tree->ConstructWidget<UScaleBox>();Back->SetContent(Scale);
    auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(1400);Size->SetHeightOverride(860);Scale->SetContent(Size);
    auto* Main=Tree->ConstructWidget<UVerticalBox>();Size->SetContent(Main);
    Main->AddChildToVerticalBox(SettingsText(Tree,TEXT("TXT_SettingsTitle"),LOCTEXT("Settings","설정"),36))->SetPadding(FMargin(0,0,0,8));
    Main->AddChildToVerticalBox(SettingsText(Tree,TEXT("TXT_SettingsDescription"),LOCTEXT("Description","플레이 환경을 조정하세요. 변경 사항은 적용한 뒤 저장됩니다."),17,FLinearColor(.56f,.64f,.73f)))->SetPadding(FMargin(0,0,0,24));
    auto* Body=Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("SettingsBody"));Main->AddChildToVerticalBox(Body)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* NavSize=Tree->ConstructWidget<USizeBox>();NavSize->SetWidthOverride(250);Body->AddChildToHorizontalBox(NavSize)->SetPadding(FMargin(0,0,28,0));
    auto* Nav=Tree->ConstructWidget<UVerticalBox>();NavSize->SetContent(Nav);
    for(int32 I=0;I<Tabs().Num();++I)
    {auto* Height=Tree->ConstructWidget<USizeBox>();Height->SetHeightOverride(58);Height->SetContent(SettingsButton(Tree,FName(*FString::Printf(TEXT("BTN_SettingsTab_%d"),I)),Tabs()[I]));Nav->AddChildToVerticalBox(Height)->SetPadding(FMargin(0,0,0,10));}
    auto* Switcher=Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("SettingsTabSwitcher"));Body->AddChildToHorizontalBox(Switcher)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    for(int32 Tab=0;Tab<Tabs().Num();++Tab)
    {
        auto* Scroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),FName(*FString::Printf(TEXT("SettingsScroll_%d"),Tab)));Scroll->SetScrollbarThickness(FVector2D(7,7));Switcher->AddChild(Scroll);
        auto* Content=Tree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Content);
        Content->AddChildToVerticalBox(SettingsText(Tree,NAME_None,Tabs()[Tab],27))->SetPadding(FMargin(4,0,16,20));
        for(const auto& Spec:Specs())if(Spec.Tab==Tab)
        {
            auto* Card=Tree->ConstructWidget<UBorder>();Card->SetBrushColor(FLinearColor(.025f,.04f,.064f));Card->SetPadding(FMargin(18,13));Content->AddChildToVerticalBox(Card)->SetPadding(FMargin(0,0,14,8));
            auto* Line=Tree->ConstructWidget<UHorizontalBox>();Card->SetContent(Line);
            auto* Left=Tree->ConstructWidget<UVerticalBox>();Line->AddChildToHorizontalBox(Left)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            Left->AddChildToVerticalBox(SettingsText(Tree,NAME_None,Spec.Label,19));
            auto* Help=SettingsText(Tree,NAME_None,Spec.Help,14,FLinearColor(.52f,.61f,.72f));Help->SetWrapTextAt(550);Left->AddChildToVerticalBox(Help)->SetPadding(FMargin(0,5,25,0));
            auto* ControlSize=Tree->ConstructWidget<USizeBox>();ControlSize->SetWidthOverride(320);auto* Placement=Line->AddChildToHorizontalBox(ControlSize);Placement->SetVerticalAlignment(VAlign_Center);
            const FName ControlName(*(FString(TEXT("OPTION_"))+Spec.ID.ToString()));
            if(Spec.Control==EICSettingControl::Toggle)
            {auto* Check=Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(),ControlName);ControlSize->SetContent(Check);}
            else if(Spec.Control==EICSettingControl::Choice)
            {
                auto* Choice=Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(),ControlName);
                PRAGMA_DISABLE_DEPRECATION_WARNINGS
                Choice->Font=ICFrontendLayout::Font(17);
                PRAGMA_ENABLE_DEPRECATION_WARNINGS
                for(const auto& Option:Spec.Choices)Choice->AddOption(Option.ToString());ControlSize->SetContent(Choice);
            }
            else
            {
                auto* Controls=Tree->ConstructWidget<UHorizontalBox>();ControlSize->SetContent(Controls);
                auto* Slider=Tree->ConstructWidget<USlider>(USlider::StaticClass(),ControlName);Slider->SetMinValue(Spec.Min);Slider->SetMaxValue(Spec.Max);Slider->SetStepSize(Spec.Step);Slider->RequiresControllerLock=false;Controls->AddChildToHorizontalBox(Slider)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                auto* ValueSize=Tree->ConstructWidget<USizeBox>();ValueSize->SetWidthOverride(68);Controls->AddChildToHorizontalBox(ValueSize)->SetPadding(FMargin(12,0,0,0));ValueSize->SetContent(SettingsText(Tree,FName(*(TEXT("VALUE_")+Spec.ID.ToString())),FText::GetEmpty(),17));
            }
        }
        if(Tab==1)
        {
            Content->AddChildToVerticalBox(SettingsText(Tree,NAME_None,LOCTEXT("BindingsTitle","키 변경"),25))->SetPadding(FMargin(4,24,14,10));
            Content->AddChildToVerticalBox(SettingsText(Tree,NAME_None,LOCTEXT("BindingsHelp","키 버튼을 선택한 뒤 새 키를 누르세요. ESC는 취소 및 메뉴 복귀용으로 유지됩니다. 중복 키는 적용 전에 확인합니다."),16,FLinearColor(.56f,.64f,.73f)))->SetPadding(FMargin(4,0,14,15));
            Content->AddChildToVerticalBox(Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("LIST_KeyBindings")));
        }
    }
    Main->AddChildToVerticalBox(SettingsText(Tree,TEXT("TXT_SettingsStatus"),FText::GetEmpty(),17,FLinearColor(.8f,.68f,.4f)))->SetPadding(FMargin(0,15,0,12));
    auto* Footer=Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("SettingsFooter"));Main->AddChildToVerticalBox(Footer);
    const FName Names[]={TEXT("BTN_Reset"),TEXT("BTN_Apply"),TEXT("BTN_KeepDisplay"),TEXT("BTN_RevertDisplay"),TEXT("BTN_Close")};
    const FText Labels[]={LOCTEXT("Reset","기본값"),LOCTEXT("Apply","적용 · 저장"),LOCTEXT("Keep","변경 유지"),LOCTEXT("Revert","되돌리기"),LOCTEXT("Close","닫기")};
    for(int32 I=0;I<5;++I)
    {auto* S=Tree->ConstructWidget<USizeBox>();S->SetHeightOverride(48);S->SetWidthOverride(I==1?200:150);S->SetContent(SettingsButton(Tree,Names[I],Labels[I]));Footer->AddChildToHorizontalBox(S)->SetPadding(FMargin(0,0,12,0));}
    Main->AddChildToVerticalBox(SettingsText(Tree,NAME_None,LOCTEXT("KeyboardHelp","Tab  이동     방향키  변경     Space / Enter  선택     ESC  닫기"),14,FLinearColor(.46f,.55f,.65f)))->SetPadding(FMargin(0,13,0,0));
}
#undef LOCTEXT_NAMESPACE
