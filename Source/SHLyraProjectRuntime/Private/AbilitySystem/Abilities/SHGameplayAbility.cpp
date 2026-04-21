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

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = LockedWalkSpeed;
		LockedCharacter = Character;
	}
}

void USHGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ACharacter* Character = LockedCharacter.Get())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			// 캐릭터 클래스 CDO 의 CMC 기본 MaxWalkSpeed 로 복원.
			const ACharacter* CharCDO = Character->GetClass()->GetDefaultObject<ACharacter>();
			if (const UCharacterMovementComponent* DefaultMovement = CharCDO ? CharCDO->GetCharacterMovement() : nullptr)
			{
				Movement->MaxWalkSpeed = DefaultMovement->MaxWalkSpeed;
			}
		}
	}
	LockedCharacter.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
