// Copyright SH. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"

#include "BTDecorator_SHCheckDistance.generated.h"

/**
 * UBTDecorator_SHCheckDistance
 *
 * BB의 Actor 키(TargetActorKey)와 소유 폰 사이 거리를 계산해
 * MaxDistance 미만이면 true를 반환하는 BT Decorator.
 *
 * BT_SHBoss 에서 Phase 1/2 근접/원거리 분기에 사용한다.
 * "거리 < 250" 이면 근접 어빌리티, 이상이면 파이어볼을 선택.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API UBTDecorator_SHCheckDistance : public UBTDecorator
{
	GENERATED_BODY()

public:

	UBTDecorator_SHCheckDistance();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

	// 거리 비교 대상이 될 Actor BB 키. BB_SHBoss 의 TargetActor 키를 지정한다.
	UPROPERTY(EditAnywhere, Category = "SH|AI")
	FBlackboardKeySelector TargetActorKey;

	// 이 거리(cm) 미만이면 조건이 true. BT 에서 근접/원거리 분기 기준.
	UPROPERTY(EditAnywhere, Category = "SH|AI", meta = (ClampMin = "0.0"))
	float MaxDistance = 250.0f;
};
