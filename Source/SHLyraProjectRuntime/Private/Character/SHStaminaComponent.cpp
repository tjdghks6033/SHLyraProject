// Copyright SH. All Rights Reserved.

#include "Character/SHStaminaComponent.h"

#include "AbilitySystem/Attributes/SHStaminaSet.h"

// GetSet, AbilityActivatedCallbacks, GE 적용에 사용
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "NativeGameplayTags.h"

// UI에 스태미나 변화를 전달하는 메시지 버스
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHStaminaComponent)

// 스태미나 소진 상태를 나타내는 GameplayTag.
// 이 태그가 ASC에 있는 동안 스태미나를 소비하는 어빌리티는 발동할 수 없습니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Status_OutOfStamina, "SH.Status.OutOfStamina");

// UI 메시지 채널 태그. 이 채널을 구독하는 위젯이 스태미나 변화를 수신합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Message_Stamina_Changed, "SH.Message.Stamina.Changed");

// GA_Hero_Dash에 부여된 어빌리티 태그. 이 태그로 대쉬 어빌리티를 식별합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Type_Action_Dash, "Ability.Type.Action.Dash");


USHStaminaComponent::USHStaminaComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool USHStaminaComponent::ResolveAttributeSet(UAbilitySystemComponent* ASC)
{
	// USHStaminaSet은 DA_SHAbilitySet을 통해 ASC에 부여됩니다.
	StaminaSet = ASC->GetSet<USHStaminaSet>();
	return StaminaSet != nullptr;
}

void USHStaminaComponent::BindValueDelegates()
{
	if (StaminaSet)
	{
		StaminaSet->OnStaminaChanged.AddUObject(this, &ThisClass::HandleStaminaChanged);
		StaminaSet->OnOutOfStamina.AddUObject(this, &ThisClass::HandleOutOfStamina);
	}
}

void USHStaminaComponent::ReleaseAttributeSet()
{
	if (StaminaSet)
	{
		StaminaSet->OnStaminaChanged.RemoveAll(this);
		StaminaSet->OnOutOfStamina.RemoveAll(this);
		StaminaSet = nullptr;
	}
}

float USHStaminaComponent::GetCurrentValue() const
{
	return StaminaSet ? StaminaSet->GetStamina() : 0.0f;
}

float USHStaminaComponent::GetMaxValue() const
{
	return StaminaSet ? StaminaSet->GetMaxStamina() : 0.0f;
}

FGameplayTag USHStaminaComponent::GetDepletedStatusTag() const
{
	return TAG_SH_Status_OutOfStamina;
}

FGameplayTag USHStaminaComponent::GetBlockedAbilityTag() const
{
	return TAG_Ability_Type_Action_Dash;
}

TSubclassOf<UGameplayEffect> USHStaminaComponent::GetRegenEffect() const
{
	return StaminaRegenEffect;
}

float USHStaminaComponent::GetCostThreshold() const
{
	return DashStaminaCostThreshold;
}

void USHStaminaComponent::BroadcastChange(float CurrentValue, float MaxValue) const
{
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);

	FSHStaminaChangedMessage Message;
	Message.Owner             = GetOwner();
	Message.CurrentStamina    = CurrentValue;
	Message.MaxStamina        = MaxValue;
	Message.StaminaNormalized = (MaxValue > 0.0f) ? (CurrentValue / MaxValue) : 0.0f;

	MessageSystem.BroadcastMessage(TAG_SH_Message_Stamina_Changed, Message);
}

void USHStaminaComponent::OnPostInitialize()
{
	// 대쉬 어빌리티 활성화 감지: GA_Hero_Dash가 켜질 때마다 스태미나를 소비합니다.
	// (ShooterCore 어빌리티라 코스트 GE를 직접 박을 수 없어 외부에서 적용)
	if (CachedASC)
	{
		CachedASC->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated);
	}
}

void USHStaminaComponent::OnPreUninitialize()
{
	if (CachedASC)
	{
		CachedASC->AbilityActivatedCallbacks.RemoveAll(this);
	}
}

void USHStaminaComponent::HandleStaminaChanged(AActor* Instigator, AActor* Causer,
	const FGameplayEffectSpec* Spec, float Magnitude,
	float OldValue, float NewValue)
{
	HandleValueChanged(OldValue, NewValue);
}

void USHStaminaComponent::HandleOutOfStamina(AActor* Instigator, AActor* Causer,
	const FGameplayEffectSpec* Spec, float Magnitude,
	float OldValue, float NewValue)
{
	HandleResourceDepleted();
}

void USHStaminaComponent::HandleAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	if (!CachedASC || !StaminaDashCostEffect || !ActivatedAbility)
	{
		return;
	}

	// Ability.Type.Action.Dash 태그를 가진 어빌리티만 처리합니다.
	// GA_Hero_Dash(ShooterCore)가 이 태그를 보유합니다.
	if (!ActivatedAbility->GetAssetTags().HasTag(TAG_Ability_Type_Action_Dash))
	{
		return;
	}

	// StaminaCost 메타 어트리뷰트를 통해 스태미나를 차감합니다.
	// SHStaminaSet::PostGameplayEffectExecute에서 Stamina -= StaminaCost 처리됩니다.
	FGameplayEffectContextHandle ContextHandle = CachedASC->MakeEffectContext();
	ContextHandle.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(
		StaminaDashCostEffect, 1.0f, ContextHandle);

	if (SpecHandle.IsValid())
	{
		CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
