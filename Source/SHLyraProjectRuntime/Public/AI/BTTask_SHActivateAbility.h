// Copyright SH. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"

#include "BTTask_SHActivateAbility.generated.h"

/**
 * UBTTask_SHActivateAbility
 *
 * 제어 중인 폰의 ASC에서 AbilityTag로 어빌리티를 발동하는 범용 BT 태스크.
 * 발동 성공 시 Succeeded, 쿨다운·마나 부족 등 실패 시 Failed를 반환한다.
 * BT의 Wait 노드로 쿨다운 간격을 제어한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API UBTTask_SHActivateAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_SHActivateAbility();

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 발동할 어빌리티의 AbilityTags에 포함된 태그.
	UPROPERTY(EditAnywhere, Category = "SH|AI")
	FGameplayTag AbilityTag;
};
