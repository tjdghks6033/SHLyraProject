// Copyright SH. All Rights Reserved.

#include "Character/SHManaComponent.h"

#include "AbilitySystem/Attributes/SHManaSet.h"

// GetSet, 델리게이트 바인딩에 사용
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"

// UI에 마나 변화를 전달하는 메시지 버스
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHManaComponent)

// 마나 소진 상태를 나타내는 GameplayTag.
// 이 태그가 ASC에 있는 동안 마나를 소비하는 어빌리티는 발동할 수 없습니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Status_OutOfMana, "SH.Status.OutOfMana");

// UI 메시지 채널 태그. 이 채널을 구독하는 위젯이 마나 변화를 수신합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Message_Mana_Changed, "SH.Message.Mana.Changed");

// 마법 어빌리티 분류 태그. GA_SHMagicProjectile의 AbilityTags에 추가되어야
// BlockAbilitiesWithTags로 차단 가능합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Type_Action_Magic, "Ability.Type.Action.Magic");


USHManaComponent::USHManaComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool USHManaComponent::ResolveAttributeSet(UAbilitySystemComponent* ASC)
{
	// USHManaSet은 DA_SHAbilitySet을 통해 ASC에 부여됩니다.
	ManaSet = ASC->GetSet<USHManaSet>();
	return ManaSet != nullptr;
}

void USHManaComponent::BindValueDelegates()
{
	if (ManaSet)
	{
		ManaSet->OnManaChanged.AddUObject(this, &ThisClass::HandleManaChanged);
		ManaSet->OnOutOfMana.AddUObject(this, &ThisClass::HandleOutOfMana);
	}
}

void USHManaComponent::ReleaseAttributeSet()
{
	if (ManaSet)
	{
		ManaSet->OnManaChanged.RemoveAll(this);
		ManaSet->OnOutOfMana.RemoveAll(this);
		ManaSet = nullptr;
	}
}

float USHManaComponent::GetCurrentValue() const
{
	return ManaSet ? ManaSet->GetMana() : 0.0f;
}

float USHManaComponent::GetMaxValue() const
{
	return ManaSet ? ManaSet->GetMaxMana() : 0.0f;
}

FGameplayTag USHManaComponent::GetDepletedStatusTag() const
{
	return TAG_SH_Status_OutOfMana;
}

FGameplayTag USHManaComponent::GetBlockedAbilityTag() const
{
	return TAG_Ability_Type_Action_Magic;
}

TSubclassOf<UGameplayEffect> USHManaComponent::GetRegenEffect() const
{
	return ManaRegenEffect;
}

float USHManaComponent::GetCostThreshold() const
{
	return MagicManaCostThreshold;
}

void USHManaComponent::BroadcastChange(float CurrentValue, float MaxValue) const
{
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);

	FSHManaChangedMessage Message;
	Message.Owner          = GetOwner();
	Message.CurrentMana    = CurrentValue;
	Message.MaxMana        = MaxValue;
	Message.ManaNormalized = (MaxValue > 0.0f) ? (CurrentValue / MaxValue) : 0.0f;

	MessageSystem.BroadcastMessage(TAG_SH_Message_Mana_Changed, Message);
}

void USHManaComponent::HandleManaChanged(AActor* Instigator, AActor* Causer,
	const FGameplayEffectSpec* Spec, float Magnitude,
	float OldValue, float NewValue)
{
	HandleValueChanged(OldValue, NewValue);
}

void USHManaComponent::HandleOutOfMana(AActor* Instigator, AActor* Causer,
	const FGameplayEffectSpec* Spec, float Magnitude,
	float OldValue, float NewValue)
{
	HandleResourceDepleted();
}
