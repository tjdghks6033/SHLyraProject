// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/SHGameplayAbility.h"

#include "SHParryAbility.generated.h"

struct FGameplayEventData;

/** Opens a short defensive window that converts an incoming melee hit into a parry. */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHParryAbility : public USHGameplayAbility
{
	GENERATED_BODY()

public:
	USHParryAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	/** Duration in seconds during which a melee hit can be parried. */
	UPROPERTY(EditDefaultsOnly, Category="SH|Parry", meta=(ClampMin="0.05"))
	float ParryWindowDuration = 0.3f;

private:
	UFUNCTION()
	void OnParrySucceeded(FGameplayEventData Payload);

	UFUNCTION()
	void OnParryWindowExpired();

	bool bParryTagApplied = false;
};
