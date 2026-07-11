// Copyright SH. All Rights Reserved.

#pragma once

#include "Character/SHResourceComponent.h"

#include "SHManaComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class USHManaSet;
struct FGameplayEffectSpec;

// GameplayMessageSubsystem을 통해 UI로 마나 변화를 전달할 때 쓰는 데이터 컨테이너
USTRUCT(BlueprintType)
struct FSHManaChangedMessage
{
	GENERATED_BODY()

	// 마나가 변화한 액터 (보통 캐릭터)
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> Owner = nullptr;

	// 변화 후 현재 마나
	UPROPERTY(BlueprintReadOnly)
	float CurrentMana = 0.0f;

	// 최대 마나
	UPROPERTY(BlueprintReadOnly)
	float MaxMana = 0.0f;

	// 0.0 ~ 1.0 정규화 값 (UI 프로그레스바용)
	UPROPERTY(BlueprintReadOnly)
	float ManaNormalized = 0.0f;
};


/**
 * USHManaComponent
 *
 * USHManaSet 기반 마나 자원 컴포넌트. 공통 골격은 USHResourceComponent가 담당하고,
 * 이 클래스는 마나 타입에 특화된 훅(어트리뷰트 셋/델리게이트/태그/메시지)만 구현한다.
 *
 * 마나 소비 자체는 GA_SHMagicProjectile의 CostGameplayEffectClass(GE_SHMagicManaCost)가
 * 처리한다. 이 컴포넌트는 UI 브로드캐스트·태그 관리·블록/언블록만 담당하며,
 * 스태미나와 달리 AbilityActivatedCallbacks로 소비 GE를 별도 적용하지 않는다
 * (OnPostInitialize 훅을 override하지 않음).
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SHLYRAPROJECTRUNTIME_API USHManaComponent : public USHResourceComponent
{
	GENERATED_BODY()

public:

	USHManaComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 같은 액터에서 이 컴포넌트를 찾는 편의 함수
	UFUNCTION(BlueprintPure, Category = "SH|Mana")
	static USHManaComponent* FindSHManaComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<USHManaComponent>() : nullptr);
	}

	// 현재 마나를 0~1 범위로 반환 (UI용)
	UFUNCTION(BlueprintCallable, Category = "SH|Mana")
	float GetManaNormalized() const { return GetValueNormalized(); }

	// 마나가 0인 상태 여부
	UFUNCTION(BlueprintCallable, Category = "SH|Mana")
	bool IsOutOfMana() const { return IsResourceDepleted(); }

protected:

	//~ USHResourceComponent 타입별 훅
	virtual bool ResolveAttributeSet(UAbilitySystemComponent* ASC) override;
	virtual void BindValueDelegates() override;
	virtual void ReleaseAttributeSet() override;
	virtual float GetCurrentValue() const override;
	virtual float GetMaxValue() const override;
	virtual FGameplayTag GetDepletedStatusTag() const override;
	virtual FGameplayTag GetBlockedAbilityTag() const override;
	virtual TSubclassOf<UGameplayEffect> GetRegenEffect() const override;
	virtual float GetCostThreshold() const override;
	virtual void BroadcastChange(float CurrentValue, float MaxValue) const override;
	//~ End of USHResourceComponent 타입별 훅

	// 이하는 USHManaSet 델리게이트 핸들러 — 베이스 공통 로직으로 위임한다.

	// 마나 값이 변경될 때 호출 (서버: GE 실행 후 / 클라이언트: OnRep)
	void HandleManaChanged(AActor* Instigator, AActor* Causer,
		const FGameplayEffectSpec* Spec, float Magnitude,
		float OldValue, float NewValue);

	// 마나가 처음으로 0이 됐을 때 호출
	void HandleOutOfMana(AActor* Instigator, AActor* Causer,
		const FGameplayEffectSpec* Spec, float Magnitude,
		float OldValue, float NewValue);

	// Periodic GE로 구현된 마나 회복 이펙트.
	// DA_SHAbilitySet에 등록하지 않고 이 컴포넌트가 직접 관리합니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Mana|Regen")
	TSubclassOf<UGameplayEffect> ManaRegenEffect;

	// 마법 어빌리티 한 번에 필요한 최소 마나.
	// 이 값 미만이면 Ability.Type.Action.Magic 태그를 가진 어빌리티가 차단됩니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Mana|Block")
	float MagicManaCostThreshold = 20.0f;

	// ASC에서 가져온 마나 어트리뷰트 셋 (읽기 전용 참조)
	UPROPERTY()
	TObjectPtr<const USHManaSet> ManaSet;
};
