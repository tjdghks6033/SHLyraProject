// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Attributes/SHManaSet.h"

#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHManaSet)


USHManaSet::USHManaSet()
	: Mana(100.0f)
	, MaxMana(100.0f)
	, ManaCost(0.0f)
	, bOutOfMana(false)
	, ManaBeforeAttributeChange(0.0f)
{
}

void USHManaSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 마나와 최대 마나는 항상 복제합니다.
	// REPNOTIFY_Always: 값이 동일하더라도 OnRep를 호출해 클라이언트 델리게이트가 누락되지 않게 합니다.
	DOREPLIFETIME_CONDITION_NOTIFY(USHManaSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USHManaSet, MaxMana, COND_None, REPNOTIFY_Always);
}

void USHManaSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	// GAS 예측 시스템에 클라이언트 측 변경을 통보합니다.
	GAMEPLAYATTRIBUTE_REPNOTIFY(USHManaSet, Mana, OldValue);

	const float CurrentMana = GetMana();
	const float Magnitude = CurrentMana - OldValue.GetCurrentValue();

	// 클라이언트에서도 UI가 반응할 수 있도록 델리게이트를 발화합니다.
	// 서버 정보(Instigator, EffectSpec)는 클라이언트에서 알 수 없으므로 nullptr입니다.
	OnManaChanged.Broadcast(nullptr, nullptr, nullptr, Magnitude, OldValue.GetCurrentValue(), CurrentMana);

	if (!bOutOfMana && CurrentMana <= 0.0f)
	{
		OnOutOfMana.Broadcast(nullptr, nullptr, nullptr, Magnitude, OldValue.GetCurrentValue(), CurrentMana);
	}

	bOutOfMana = (CurrentMana <= 0.0f);
}

void USHManaSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USHManaSet, MaxMana, OldValue);
}

bool USHManaSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	// GE 실행 전 현재 마나를 저장합니다.
	// PostGameplayEffectExecute에서 실제로 값이 변했는지 비교할 때 사용합니다.
	ManaBeforeAttributeChange = GetMana();

	return true;
}

void USHManaSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	AActor* Causer     = EffectContext.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetManaCostAttribute())
	{
		// ManaCost 메타 어트리뷰트가 설정된 경우 — 어빌리티 소비 처리.
		// ManaCost 값만큼 Mana를 차감하고, 메타 어트리뷰트는 0으로 초기화합니다.
		const float NewMana = FMath::Clamp(GetMana() - GetManaCost(), 0.0f, GetMaxMana());
		SetMana(NewMana);
		SetManaCost(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		// Mana를 직접 수정하는 GE(예: Regen)가 실행된 경우 클램핑합니다.
		SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
	}

	// 실제로 마나가 변했을 때만 델리게이트를 발화합니다.
	if (GetMana() != ManaBeforeAttributeChange)
	{
		OnManaChanged.Broadcast(Instigator, Causer, &Data.EffectSpec,
			Data.EvaluatedData.Magnitude, ManaBeforeAttributeChange, GetMana());
	}

	// 마나가 처음으로 0이 됐을 때만 발화합니다.
	if ((GetMana() <= 0.0f) && !bOutOfMana)
	{
		OnOutOfMana.Broadcast(Instigator, Causer, &Data.EffectSpec,
			Data.EvaluatedData.Magnitude, ManaBeforeAttributeChange, GetMana());
	}

	bOutOfMana = (GetMana() <= 0.0f);
}

void USHManaSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// GE 실행이 아닌 일반 어트리뷰트 변경에도 클램핑이 적용됩니다.
	ClampAttribute(Attribute, NewValue);
}

void USHManaSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		// MaxMana가 0이 되면 나눗셈 오류가 발생하므로 최솟값을 보장합니다.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
