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
}

EBTNodeResult::Type UBTTask_SHActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	// Lyra 아키텍처: ASC는 Pawn이 아닌 PlayerState에 존재한다.
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

	return ASC->TryActivateAbilitiesByTag(TagContainer)
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}
