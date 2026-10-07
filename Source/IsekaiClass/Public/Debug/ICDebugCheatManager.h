#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ICDebugCheatManager.generated.h"

UCLASS(meta=(ToolTip="개발용 콘솔 명령입니다. 실제 상태 API를 사용하며 Shipping 빌드에서는 실행되지 않습니다. 일반 게임 UI나 콘텐츠 정의 대신 사용하지 마세요.", ShortToolTip="개발용 콘솔 명령입니다."))
class ISEKAICLASS_API UICDebugCheatManager : public UCheatManager
{
    GENERATED_BODY()
public:
    UFUNCTION(Exec, meta=(ToolTip="디버그 콘솔에서 GameClock을 지정 분만큼 점프합니다. 실제 TimeSubsystem 경로를 사용하며 PlayClock은 따로 유지합니다.\n@param Minutes 진행/점프할 GameClock 분입니다. PlayClock 초와 구분합니다. 입력 핀. 자료형: 배정밀도 소수.\n디버그 콘솔 명령입니다. 실제 출시 플레이 입력용으로 연결하지 마세요.")) void ICAddGameMinutes(double Minutes);
    UFUNCTION(Exec, meta=(ToolTip="디버그 콘솔에서 실제 Item DT ID의 아이템을 지급합니다. 유효하지 않은 정의는 거부하고 현재 인벤토리/초과 처리 경로를 사용합니다.\n@param ItemId 지급할 실제 Item DT ID입니다. 입력 핀. 자료형: 이름/ID.\n@param Count 아이템 정수 수량입니다. 양수 값을 사용하세요. 입력 핀. 자료형: 정수.\n디버그 콘솔 명령입니다. 실제 출시 플레이 입력용으로 연결하지 마세요.")) void ICGiveItem(FName ItemId, int32 Count);
    UFUNCTION(Exec, meta=(ToolTip="디버그 콘솔에서 실제 NPC의 관계 총값을 설정합니다. WorldState 변경 경로의 대상·범위 검사를 통과해야 합니다.\n@param NpcId 관계를 변경할 NPC 정의 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param Value 지정한 관계 또는 기본 능력치의 최종 정수값입니다. 더할 양이 아닙니다. 입력 핀. 자료형: 정수.\n디버그 콘솔 명령입니다. 실제 출시 플레이 입력용으로 연결하지 마세요.")) void ICSetAffinity(FName NpcId, int32 Value);
    UFUNCTION(Exec, meta=(ToolTip="디버그 콘솔에서 플레이어를 월드 좌표 cm로 이동시킵니다. X/Y/Z는 레벨의 좌표계입니다.\n@param X 월드 X 좌표 cm입니다. 입력 핀. 자료형: 소수.\n@param Y 월드 Y 좌표 cm입니다. 입력 핀. 자료형: 소수.\n@param Z 월드 Z 좌표 cm입니다. 입력 핀. 자료형: 소수.\n디버그 콘솔 명령입니다. 실제 출시 플레이 입력용으로 연결하지 마세요.")) void ICTeleport(float X, float Y, float Z);
};
