// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"

#include "BTTask_SHActivateAbility.generated.h"

class UBehaviorTreeComponent;

/**
 * UBTTask_SHActivateAbility
 *
 * 제어 중인 폰의 ASC에서 AbilityTag로 어빌리티를 발동하는 범용 BT 태스크.
 * 발동 성공 시 Succeeded, 쿨다운·마나 부족 등 실패 시 Failed를 반환한다.
 *
 * bWaitForAbilityEnd=true: 어빌리티가 완전히 끝날 때까지 InProgress를 유지한다.
 * 이 옵션을 켜면 몽타주 도중 BT가 다른 패턴으로 전환하는 것을 방지한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API UBTTask_SHActivateAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_SHActivateAbility();

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 발동할 어빌리티의 AbilityTags에 포함된 태그.
	UPROPERTY(EditAnywhere, Category = "SH|AI")
	FGameplayTag AbilityTag;

	// true: 어빌리티가 EndAbility를 호출할 때까지 BT를 대기시킨다.
	// 몽타주를 끝까지 재생해야 하는 공격 어빌리티에 사용한다.
	UPROPERTY(EditAnywhere, Category = "SH|AI")
	bool bWaitForAbilityEnd = false;

private:

	void OnAbilityEnded(UGameplayAbility* Ability);
	void Cleanup();

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FDelegateHandle AbilityEndedHandle;
};
