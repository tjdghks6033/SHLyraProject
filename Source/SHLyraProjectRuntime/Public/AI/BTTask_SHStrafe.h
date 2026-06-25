// Copyright SH. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "Navigation/PathFollowingComponent.h"

#include "BTTask_SHStrafe.generated.h"

/**
 * UBTTask_SHStrafe
 *
 * BB_TargetActor 기준 원호 스트레이프 BT 태스크.
 * 플레이어로부터 StrafeRadius 거리를 유지하면서 좌/우 랜덤으로
 * StrafeArcDegrees만큼 호를 그려 이동한다. 이동 완료 시 Succeeded.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API UBTTask_SHStrafe : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_SHStrafe();

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	// 플레이어와 유지할 이동 반경 (cm)
	UPROPERTY(EditAnywhere, Category = "SH|AI", meta = (ClampMin = "100.0"))
	float StrafeRadius = 350.f;

	// 1회 스트레이프로 이동할 호 각도 (도)
	UPROPERTY(EditAnywhere, Category = "SH|AI", meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float StrafeArcDegrees = 60.f;

	// 목표 지점 도착 인정 반경 (cm)
	UPROPERTY(EditAnywhere, Category = "SH|AI", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 60.f;

	// TargetActor를 저장하는 블랙보드 키 이름
	UPROPERTY(EditAnywhere, Category = "SH|AI")
	FName TargetActorKey = TEXT("TargetActor");

private:

	void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
	void Cleanup();

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	TWeakObjectPtr<UPathFollowingComponent> CachedPathFollowing;
	FAIRequestID MoveRequestID;
	FDelegateHandle MoveCompletedHandle;
};
