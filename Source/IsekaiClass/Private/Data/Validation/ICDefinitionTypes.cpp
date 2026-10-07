#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "Debug/ICLog.h"
#include "Data/Validation/ICDefinitionLibrary.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"

EDataValidationResult FICDefinitionRow::IsDataValid(FDataValidationContext& Context) const
{
    IC_TRACE_FUNCTION();
    bool bValid = true;
    if (!UICDefinitionLibrary::IsDefinitionIDValid(ID))
    { Context.AddError(FText::FromString(TEXT("Definition ID must use A-Z, 0-9, underscore, at most 40 characters."))); bValid = false; }
    if (DataVersion < 1)
    { Context.AddError(FText::FromString(TEXT("DataVersion must be positive."))); bValid = false; }
    if(!bValid) UE_LOG(LogICData, Log, TEXT("[Row.Validation] ID=%s DataVersion=%d: ID 형식 또는 버전 검사 실패"),*ID.ToString(),DataVersion);
    return bValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
