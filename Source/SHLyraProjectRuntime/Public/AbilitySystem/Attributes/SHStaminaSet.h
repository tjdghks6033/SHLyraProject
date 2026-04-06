// Copyright SH. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "SHStaminaSet.generated.h"

class UObject;
struct FFrame;
struct FGameplayEffectModCallbackData;

// GameplayAbilities의 AttributeSet.h가 제공하는 네 가지 헬퍼 매크로를 하나로 묶습니다.
// LyraAttributeSet.h와 동일한 패턴이지만, LyraAttributeSet.h는 PrivateDependency이므로
// 여기서 독립적으로 정의합니다.
#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

// 어트리뷰트 변화 시 발화되는 델리게이트 타입 (LyraAttributeSet의 FLyraAttributeEvent와 동일한 시그니처)
DECLARE_MULTICAST_DELEGATE_SixParams(FSHAttributeEvent,
	AActor* /*Instigator*/, AActor* /*Causer*/,
	const FGameplayEffectSpec* /*Spec*/, float /*Magnitude*/,
	float /*OldValue*/, float /*NewValue*/);


/**
 * USHStaminaSet
 *
 * 스태미나 리소스를 정의하는 AttributeSet.
 * Lyra에는 스태미나 개념이 없으므로 처음부터 직접 구현합니다.
 *
 * 어트리뷰트 구성:
 *   Stamina     — 현재 스태미나 (0 ~ MaxStamina, 복제됨)
 *   MaxStamina  — 최대 스태미나 (복제됨)
 *   StaminaCost — 메타 어트리뷰트. GE가 이 값을 설정하면
 *                 PostGameplayEffectExecute에서 Stamina를 해당 만큼 차감합니다.
 *                 복제되지 않습니다.
 *
 * 회복(Regen)은 이 Set이 직접 처리하지 않습니다.
 * USHStaminaComponent가 Periodic GE(GE_SHStaminaRegen)를 관리합니다.
 */
UCLASS(BlueprintType)
class USHStaminaSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	USHStaminaSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ATTRIBUTE_ACCESSORS(USHStaminaSet, Stamina);
	ATTRIBUTE_ACCESSORS(USHStaminaSet, MaxStamina);
	ATTRIBUTE_ACCESSORS(USHStaminaSet, StaminaCost);

	// 스태미나 값이 변경될 때 발화합니다 (서버: GE 실행 후 / 클라이언트: OnRep).
	mutable FSHAttributeEvent OnStaminaChanged;

	// 스태미나가 0이 될 때 최초 1회 발화합니다.
	mutable FSHAttributeEvent OnOutOfStamina;

protected:

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	// GE 실행 전에 변경 전 값을 저장합니다.
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;

	// GE 실행 후 클램핑, 델리게이트 발화를 처리합니다.
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// 어트리뷰트 값을 유효 범위로 제한합니다.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	// 현재 스태미나. 0 이하로 내려가지 않으며 MaxStamina를 초과하지 않습니다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "SH|Stamina", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Stamina;

	// 최대 스태미나. 1 이상을 보장합니다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "SH|Stamina", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxStamina;

	// 메타 어트리뷰트: GE가 소비량을 기록하는 임시 값.
	// PostGameplayEffectExecute에서 Stamina -= StaminaCost 처리 후 0으로 초기화됩니다.
	UPROPERTY(BlueprintReadOnly, Category = "SH|Stamina", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData StaminaCost;

	// 스태미나 소진 상태를 추적합니다. 중복 OnOutOfStamina 발화를 막습니다.
	bool bOutOfStamina;

	// GE 실행 전 스태미나 값 (PostGameplayEffectExecute에서 변화 감지에 사용).
	float StaminaBeforeAttributeChange;
};
