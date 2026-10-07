#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Debug/ICLog.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "ObjectTools.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace
{
bool HasKorean(const FString& Text)
{
    IC_TRACE_FUNCTION();
    for (TCHAR Character : Text)
        if (Character >= 0xAC00 && Character <= 0xD7A3) return true;
    return false;
}

bool IsProjectType(const UObject* Type)
{
    IC_TRACE_FUNCTION();
    return Type->GetOutermost()->GetFName() == TEXT("/Script/IsekaiClass");
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICEditorDocumentationCheck, "IC.EditorDocumentation.KoreanTooltips",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICEditorDocumentationCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    int32 TypeCount = 0, PropertyCount = 0, FunctionCount = 0, ParameterCount = 0, EnumValueCount = 0;
    for (TObjectIterator<UStruct> It; It; ++It)
    {
        UStruct* Type = *It;
        if (!IsProjectType(Type) || (!Type->IsA<UClass>() && !Type->IsA<UScriptStruct>())) continue;
        ++TypeCount;
        TestTrue(Type->GetPathName() + TEXT(" type tooltip"), HasKorean(Type->GetToolTipText().ToString()));

        for (TFieldIterator<FProperty> PropertyIt(Type, EFieldIterationFlags::None); PropertyIt; ++PropertyIt)
        {
            const FProperty* Property = *PropertyIt;
            ++PropertyCount;
            TestTrue(Property->GetPathName() + TEXT(" property tooltip"), HasKorean(Property->GetToolTipText().ToString()));
        }
        for (TFieldIterator<UFunction> FunctionIt(Type, EFieldIterationFlags::None); FunctionIt; ++FunctionIt)
        {
            const UFunction* Function = *FunctionIt;
            if (Function->HasAnyFunctionFlags(FUNC_Delegate)) continue;
            ++FunctionCount;
            TestTrue(Function->GetPathName() + TEXT(" node tooltip"), HasKorean(Function->GetToolTipText().ToString()));
            for (TFieldIterator<FProperty> PinIt(Function, EFieldIterationFlags::None); PinIt; ++PinIt)
            {
                const FProperty* Pin = *PinIt;
                if (!Pin->HasAnyPropertyFlags(CPF_Parm)) continue;
                ++ParameterCount;
                const FString PinTooltip = Pin->HasAnyPropertyFlags(CPF_ReturnParm)
                    ? ObjectTools::GetDefaultTooltipForFunctionReturn(Function)
                    : ObjectTools::GetDefaultTooltipForFunctionParam(Function, Pin->GetName());
                TestTrue(Function->GetPathName() + TEXT(" pin ") + Pin->GetName(), HasKorean(PinTooltip));
            }
        }
    }
    for (TObjectIterator<UEnum> It; It; ++It)
    {
        const UEnum* Enum = *It;
        if (!IsProjectType(Enum)) continue;
        TestTrue(Enum->GetPathName() + TEXT(" enum tooltip"), HasKorean(Enum->GetToolTipText().ToString()));
        for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
        {
            if (Index == Enum->NumEnums() - 1 && Enum->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX"))) continue;
            ++EnumValueCount;
            TestTrue(Enum->GetNameStringByIndex(Index) + TEXT(" value tooltip"), HasKorean(Enum->GetToolTipTextByIndex(Index).ToString()));
        }
    }
    TestEqual(TEXT("All native class/struct/interface descriptions present"), TypeCount, 168);
    TestEqual(TEXT("All declared property descriptions compiled"), PropertyCount, 1305);
    TestEqual(TEXT("All declared function descriptions compiled"), FunctionCount, 243);
    TestEqual(TEXT("All declared enum value descriptions compiled including hidden compatibility values"), EnumValueCount, 404);
    AddInfo(FString::Printf(TEXT("Korean metadata verified: %d types, %d properties, %d functions, %d parameter/return pins, %d enum values."),
        TypeCount, PropertyCount, FunctionCount, ParameterCount, EnumValueCount));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICBlueprintDocumentationCheck, "IC.EditorDocumentation.BlueprintDescriptions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICBlueprintDocumentationCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    const TCHAR* Paths[] = {
        TEXT("/Game/IsekaiClass/Core/GameFramework/BP_ICGameMode"),
        TEXT("/Game/IsekaiClass/Core/GameFramework/BP_ICGameState"),
        TEXT("/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController"),
        TEXT("/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerState"),
        TEXT("/Game/IsekaiClass/Player/Blueprints/BP_ICPlayerCharacter"),
        TEXT("/Game/IsekaiClass/UI/Images/Legacy/WBP_MainMenu_Legacy"),
        TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_CharacterCreate"),
        TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_LoadConfirm"),
        TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_SaveLoad"),
        TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_SaveSlot"),
        TEXT("/Game/IsekaiClass/UI/Widget/WBP_MainMenu")
    };
    int32 VariableCount = 0;
    for (const TCHAR* Path : Paths)
    {
        UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, Path);
        if (!TestNotNull(FString(Path), Blueprint)) continue;
        TestTrue(FString(Path) + TEXT(" content browser description"), HasKorean(Blueprint->BlueprintDescription));
        bool bChanged = false;
        for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
        {
            ++VariableCount;
            FString Tooltip;
            FBlueprintEditorUtils::GetBlueprintVariableMetaData(Blueprint, Variable.VarName, nullptr, TEXT("ToolTip"), Tooltip);
            // Default test runs are read-only. The explicit authoring flag seeds the
            // existing BP-only variable that Unreal Python cannot reflect directly.
            if (!HasKorean(Tooltip) && FParse::Param(FCommandLine::Get(), TEXT("ICApplyBlueprintVariableDocumentation"))
                && Variable.VarName == TEXT("IC_Create_Male_IdleMaterial"))
            {
                Tooltip = TEXT("캐릭터 생성 WBP에 직접 만든 남성 외형 미리보기용 머티리얼 변수입니다. 실제 머티리얼 참조를 지정하고 사용하는 BP 그래프와 연결하세요. 변수에 에셋을 지정하는 것만으로 외형 표시가 자동 갱신되지는 않습니다. 능력치·직업·WorldState 원본을 변경하는 값과 구분합니다.");
                FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, Variable.VarName, nullptr, TEXT("ToolTip"), Tooltip);
                bChanged = true;
            }
            TestTrue(FString(Path) + TEXT(" variable ") + Variable.VarName.ToString(), HasKorean(Tooltip));
        }
        if (bChanged)
        {
            FKismetEditorUtilities::CompileBlueprint(Blueprint);
            UPackage* Package = Blueprint->GetOutermost();
            FSavePackageArgs Args;
            Args.TopLevelFlags = RF_Public | RF_Standalone;
            Args.SaveFlags = SAVE_NoError;
            const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            TestTrue(FString(Path) + TEXT(" explicit documentation authoring save"),
                UPackage::SavePackage(Package, Blueprint, *Filename, Args));
        }
        TestTrue(FString(Path) + TEXT(" compiled blueprint"),
            Blueprint->Status == BS_UpToDate || Blueprint->Status == BS_UpToDateWithWarnings);
    }
    AddInfo(FString::Printf(TEXT("Verified Korean asset descriptions for %d Blueprints and %d BP-owned variable tooltips."),
        UE_ARRAY_COUNT(Paths), VariableCount));
    return !HasAnyErrors();
}
#endif
