// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Attributes/SHStaminaSet.h"

#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHStaminaSet)


USHStaminaSet::USHStaminaSet()
	: Stamina(100.0f)
	, MaxStamina(100.0f)
	, StaminaCost(0.0f)
	, bOutOfStamina(false)
	, StaminaBeforeAttributeChange(0.0f)
{
}

void USHStaminaSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 스태미나와 최대 스태미나는 항상 복제합니다.
	// REPNOTIFY_Always: 값이 동일하더라도 OnRep를 호출해 클라이언트 델리게이트가 누락되지 않게 합니다.
	DOREPLIFETIME_CONDITION_NOTIFY(USHStaminaSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USHStaminaSet, MaxStamina, COND_None, REPNOTIFY_Always);
}

void USHStaminaSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	// GAS 예측 시스템에 클라이언트 측 변경을 통보합니다.
	GAMEPLAYATTRIBUTE_REPNOTIFY(USHStaminaSet, Stamina, OldValue);

	const float CurrentStamina = GetStamina();
	const float Magnitude = CurrentStamina - OldValue.GetCurrentValue();

	// 클라이언트에서도 UI가 반응할 수 있도록 델리게이트를 발화합니다.
	// 서버 정보(Instigator, EffectSpec)는 클라이언트에서 알 수 없으므로 nullptr입니다.
	OnStaminaChanged.Broadcast(nullptr, nullptr, nullptr, Magnitude, OldValue.GetCurrentValue(), CurrentStamina);

	if (!bOutOfStamina && CurrentStamina <= 0.0f)
	{
		OnOutOfStamina.Broadcast(nullptr, nullptr, nullptr, Magnitude, OldValue.GetCurrentValue(), CurrentStamina);
	}

	bOutOfStamina = (CurrentStamina <= 0.0f);
}

void USHStaminaSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USHStaminaSet, MaxStamina, OldValue);
}

bool USHStaminaSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	// GE 실행 전 현재 스태미나를 저장합니다.
	// PostGameplayEffectExecute에서 실제로 값이 변했는지 비교할 때 사용합니다.
	StaminaBeforeAttributeChange = GetStamina();

	return true;
}

void USHStaminaSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	AActor* Causer     = EffectContext.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetStaminaCostAttribute())
	{
		// StaminaCost 메타 어트리뷰트가 설정된 경우 — 어빌리티 소비 처리.
		// StaminaCost 값만큼 Stamina를 차감하고, 메타 어트리뷰트는 0으로 초기화합니다.
		const float NewStamina = FMath::Clamp(GetStamina() - GetStaminaCost(), 0.0f, GetMaxStamina());
		SetStamina(NewStamina);
		SetStaminaCost(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		// Stamina를 직접 수정하는 GE(예: Regen)가 실행된 경우 클램핑합니다.
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}

	// 실제로 스태미나가 변했을 때만 델리게이트를 발화합니다.
	if (GetStamina() != StaminaBeforeAttributeChange)
	{
		OnStaminaChanged.Broadcast(Instigator, Causer, &Data.EffectSpec,
			Data.EvaluatedData.Magnitude, StaminaBeforeAttributeChange, GetStamina());
	}

	// 스태미나가 처음으로 0이 됐을 때만 발화합니다.
	if ((GetStamina() <= 0.0f) && !bOutOfStamina)
	{
		OnOutOfStamina.Broadcast(Instigator, Causer, &Data.EffectSpec,
			Data.EvaluatedData.Magnitude, StaminaBeforeAttributeChange, GetStamina());
	}

	bOutOfStamina = (GetStamina() <= 0.0f);
}

void USHStaminaSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// GE 실행이 아닌 일반 어트리뷰트 변경에도 클램핑이 적용됩니다.
	ClampAttribute(Attribute, NewValue);
}

void USHStaminaSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		// MaxStamina가 0이 되면 나눗셈 오류가 발생하므로 최솟값을 보장합니다.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
