// Copyright SH. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "SHManaSet.generated.h"

class UObject;
struct FFrame;
struct FGameplayEffectModCallbackData;

// GameplayAbilities의 AttributeSet.h가 제공하는 네 가지 헬퍼 매크로를 하나로 묶습니다.
// SHStaminaSet.h와 동일한 패턴이며, 각 Set이 독립적으로 이 매크로를 정의합니다.
#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

// 어트리뷰트 변화 시 발화되는 델리게이트 타입.
// SHStaminaSet의 FSHAttributeEvent와 시그니처는 동일하지만,
// 두 헤더가 동시 포함되는 경우 ODR 충돌을 피하기 위해 별도 이름으로 선언합니다.
DECLARE_MULTICAST_DELEGATE_SixParams(FSHManaAttributeEvent,
	AActor* /*Instigator*/, AActor* /*Causer*/,
	const FGameplayEffectSpec* /*Spec*/, float /*Magnitude*/,
	float /*OldValue*/, float /*NewValue*/);


/**
 * USHManaSet
 *
 * 마나 리소스를 정의하는 AttributeSet.
 * Lyra에는 마나 개념이 없으므로 처음부터 직접 구현합니다.
 * USHStaminaSet과 동일한 패턴이며, 마법 어빌리티의 자원으로 사용됩니다.
 *
 * 어트리뷰트 구성:
 *   Mana     — 현재 마나 (0 ~ MaxMana, 복제됨)
 *   MaxMana  — 최대 마나 (복제됨)
 *   ManaCost — 메타 어트리뷰트. GE가 이 값을 설정하면
 *              PostGameplayEffectExecute에서 Mana를 해당 만큼 차감합니다.
 *              복제되지 않습니다.
 *
 * 회복(Regen)은 이 Set이 직접 처리하지 않습니다.
 * USHManaComponent가 Periodic GE(GE_SHManaRegen)를 관리합니다.
 */
UCLASS(BlueprintType)
class USHManaSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	USHManaSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ATTRIBUTE_ACCESSORS(USHManaSet, Mana);
	ATTRIBUTE_ACCESSORS(USHManaSet, MaxMana);
	ATTRIBUTE_ACCESSORS(USHManaSet, ManaCost);

	// 마나 값이 변경될 때 발화합니다 (서버: GE 실행 후 / 클라이언트: OnRep).
	mutable FSHManaAttributeEvent OnManaChanged;

	// 마나가 0이 될 때 최초 1회 발화합니다.
	mutable FSHManaAttributeEvent OnOutOfMana;

protected:

	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);

	// GE 실행 전에 변경 전 값을 저장합니다.
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;

	// GE 실행 후 클램핑, 델리게이트 발화를 처리합니다.
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// 어트리뷰트 값을 유효 범위로 제한합니다.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	// 현재 마나. 0 이하로 내려가지 않으며 MaxMana를 초과하지 않습니다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "SH|Mana", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Mana;

	// 최대 마나. 1 이상을 보장합니다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "SH|Mana", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxMana;

	// 메타 어트리뷰트: GE가 소비량을 기록하는 임시 값.
	// PostGameplayEffectExecute에서 Mana -= ManaCost 처리 후 0으로 초기화됩니다.
	UPROPERTY(BlueprintReadOnly, Category = "SH|Mana", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ManaCost;

	// 마나 소진 상태를 추적합니다. 중복 OnOutOfMana 발화를 막습니다.
	bool bOutOfMana;

	// GE 실행 전 마나 값 (PostGameplayEffectExecute에서 변화 감지에 사용).
	float ManaBeforeAttributeChange;
};
