// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHParryAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_SH_Parry, "Ability.SH.Parry");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_Parrying, "Status.SH.Parrying");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_SH_Parry_Success, "Event.SH.Parry.Success");

USHParryAbility::USHParryAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ELyraAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_SH_Parry);
	SetAssetTags(AssetTags);
}

void USHParryAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Loose tag is present on both predicted client and authority instances, so
	// the authoritative melee trace can validate the parry window.
	// Prediction and authority may execute in the same PIE process. Pinning the
	// count prevents duplicate activation paths from leaving a stale tag behind.
	ASC->SetLooseGameplayTagCount(TAG_Status_SH_Parrying, 1);
	bParryTagApplied = true;

	UAbilityTask_WaitGameplayEvent* SuccessTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TAG_Event_SH_Parry_Success, nullptr, true, true);
	SuccessTask->EventReceived.AddDynamic(this, &ThisClass::OnParrySucceeded);
	SuccessTask->ReadyForActivation();

	UAbilityTask_WaitDelay* WindowTask = UAbilityTask_WaitDelay::WaitDelay(this, ParryWindowDuration);
	WindowTask->OnFinish.AddDynamic(this, &ThisClass::OnParryWindowExpired);
	WindowTask->ReadyForActivation();
}

void USHParryAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (bParryTagApplied && ActorInfo)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->SetLooseGameplayTagCount(TAG_Status_SH_Parrying, 0);
		}
	}
	bParryTagApplied = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USHParryAbility::OnParrySucceeded(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USHParryAbility::OnParryWindowExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
