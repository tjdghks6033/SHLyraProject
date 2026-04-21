// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHGameplayAbility.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

USHGameplayAbility::USHGameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USHGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!bLockMovementOnActivate || !ActorInfo)
	{
		return;
	}

	// CharacterMovementComponent의 MaxWalkSpeed를 잠근다.
	// 클라/서버 모두 동일하게 적용하므로 복제 예측과 충돌하지 않는다.
	if (const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			CachedWalkSpeed = Movement->MaxWalkSpeed;
			Movement->MaxWalkSpeed = LockedWalkSpeed;
			bWalkSpeedModified = true;
		}
	}
}

void USHGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWalkSpeedModified && ActorInfo)
	{
		if (const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = CachedWalkSpeed;
			}
		}
		bWalkSpeedModified = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
