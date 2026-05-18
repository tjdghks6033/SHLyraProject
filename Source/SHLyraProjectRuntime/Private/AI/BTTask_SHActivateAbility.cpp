// Copyright SH. All Rights Reserved.

#include "AI/BTTask_SHActivateAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/PlayerState.h"

UBTTask_SHActivateAbility::UBTTask_SHActivateAbility()
{
	NodeName = TEXT("SH Activate Ability");

	// 상태(CachedOwnerComp 등)를 멤버 변수에 저장하므로 노드 인스턴싱이 필요하다.
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_SHActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
		AIController->GetPlayerState<APlayerState>());
	if (!ASC)
	{
		return EBTNodeResult::Failed;
	}

	if (!AbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(AbilityTag);

	if (!ASC->TryActivateAbilitiesByTag(TagContainer))
	{
		return EBTNodeResult::Failed;
	}

	if (!bWaitForAbilityEnd)
	{
		return EBTNodeResult::Succeeded;
	}

	// 발동 직후 이미 종료됐는지 확인 (CommitAbility 실패 등)
	bool bIsStillActive = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTagExact(AbilityTag) && Spec.IsActive())
		{
			bIsStillActive = true;
			break;
		}
	}

	if (!bIsStillActive)
	{
		return EBTNodeResult::Succeeded;
	}

	CachedOwnerComp = &OwnerComp;
	CachedASC       = ASC;

	AbilityEndedHandle = ASC->AbilityEndedCallbacks.AddUObject(
		this, &UBTTask_SHActivateAbility::OnAbilityEnded);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_SHActivateAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Cleanup();
	return EBTNodeResult::Aborted;
}

void UBTTask_SHActivateAbility::OnAbilityEnded(UGameplayAbility* Ability)
{
	if (!Ability || !Ability->GetAssetTags().HasTagExact(AbilityTag))
	{
		return;
	}

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	Cleanup();

	if (OwnerComp)
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTTask_SHActivateAbility::Cleanup()
{
	if (CachedASC.IsValid() && AbilityEndedHandle.IsValid())
	{
		CachedASC->AbilityEndedCallbacks.Remove(AbilityEndedHandle);
	}

	AbilityEndedHandle.Reset();
	CachedASC.Reset();
	CachedOwnerComp.Reset();
}
