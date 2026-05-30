// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Attributes/SHManaSet.h"
#include "AbilitySystem/Attributes/SHStaminaSet.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

USHGameplayAbility::USHGameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USHGameplayAbility* USHGameplayAbility::FindAbilityCDOByInputTag(UAbilitySystemComponent* ASC, FGameplayTag InputTag)
{
	if (!ASC || !InputTag.IsValid())
	{
		return nullptr;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			return Cast<USHGameplayAbility>(Spec.Ability.Get());
		}
	}

	return nullptr;
}

bool USHGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// 기본 검사(쿨다운 태그 등)를 먼저 통과해야 한다.
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	const UGameplayEffect* CostGE = GetCostGameplayEffect();
	if (!CostGE || !ActorInfo)
	{
		return true;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return true;
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	const int32 AbilityLevel = Spec ? Spec->Level : 1;

	for (const FGameplayModifierInfo& Mod : CostGE->Modifiers)
	{
		if (Mod.ModifierOp != EGameplayModOp::Additive)
		{
			continue;
		}

		float Magnitude = 0.0f;
		if (!Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(AbilityLevel, Magnitude) || Magnitude <= 0.0f)
		{
			continue;
		}

		FGameplayAttribute ResourceAttribute;
		if (Mod.Attribute == USHManaSet::GetManaCostAttribute())
		{
			ResourceAttribute = USHManaSet::GetManaAttribute();
		}
		else if (Mod.Attribute == USHStaminaSet::GetStaminaCostAttribute())
		{
			ResourceAttribute = USHStaminaSet::GetStaminaAttribute();
		}
		else
		{
			continue;
		}

		bool bFound = false;
		const float CurrentValue = ASC->GetGameplayAttributeValue(ResourceAttribute, bFound);

		if (bFound && CurrentValue < Magnitude)
		{
			return false;
		}
	}

	return true;
}

bool USHGameplayAbility::CanPayCostByInputTag(UAbilitySystemComponent* ASC, FGameplayTag InputTag)
{
	if (!ASC || !InputTag.IsValid())
	{
		return true;
	}

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		if (const UGameplayAbility* Ability = Spec.Ability.Get())
		{
			return Ability->CheckCost(Spec.Handle, ASC->AbilityActorInfo.Get());
		}
	}

	return true;
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
