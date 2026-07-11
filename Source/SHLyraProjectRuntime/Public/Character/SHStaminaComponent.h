// Copyright SH. All Rights Reserved.

#pragma once

#include "Character/SHResourceComponent.h"

#include "SHStaminaComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
class USHStaminaSet;
struct FGameplayEffectSpec;

// GameplayMessageSubsystem을 통해 UI로 스태미나 변화를 전달할 때 쓰는 데이터 컨테이너
USTRUCT(BlueprintType)
struct FSHStaminaChangedMessage
{
	GENERATED_BODY()

	// 스태미나가 변화한 액터 (보통 캐릭터)
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> Owner = nullptr;

	// 변화 후 현재 스태미나
	UPROPERTY(BlueprintReadOnly)
	float CurrentStamina = 0.0f;

	// 최대 스태미나
	UPROPERTY(BlueprintReadOnly)
	float MaxStamina = 0.0f;

	// 0.0 ~ 1.0 정규화 값 (UI 프로그레스바용)
	UPROPERTY(BlueprintReadOnly)
	float StaminaNormalized = 0.0f;
};


/**
 * USHStaminaComponent
 *
 * USHStaminaSet 기반 스태미나 자원 컴포넌트. 공통 골격은 USHResourceComponent가
 * 담당하고, 이 클래스는 스태미나 타입 특화 훅만 구현한다.
 *
 * ShooterCore의 GA_Hero_Dash는 무수정 원칙상 코스트 GE를 박을 수 없는 빌려온
 * 어빌리티이므로, OnPostInitialize에서 AbilityActivatedCallbacks를 감시해
 * 대쉬 발동 시 StaminaDashCostEffect를 외부에서 적용한다 (마나엔 없는 비대칭).
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SHLYRAPROJECTRUNTIME_API USHStaminaComponent : public USHResourceComponent
{
	GENERATED_BODY()

public:

	USHStaminaComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 같은 액터에서 이 컴포넌트를 찾는 편의 함수
	UFUNCTION(BlueprintPure, Category = "SH|Stamina")
	static USHStaminaComponent* FindSHStaminaComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<USHStaminaComponent>() : nullptr);
	}

	// 현재 스태미나를 0~1 범위로 반환 (UI용)
	UFUNCTION(BlueprintCallable, Category = "SH|Stamina")
	float GetStaminaNormalized() const { return GetValueNormalized(); }

	// 스태미나가 0인 상태 여부
	UFUNCTION(BlueprintCallable, Category = "SH|Stamina")
	bool IsOutOfStamina() const { return IsResourceDepleted(); }

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

	// 대쉬 코스트 외부 적용을 위한 AbilityActivatedCallbacks 감시 (스태미나 고유).
	virtual void OnPostInitialize() override;
	virtual void OnPreUninitialize() override;
	//~ End of USHResourceComponent 타입별 훅

	// 이하는 USHStaminaSet 델리게이트 핸들러 — 베이스 공통 로직으로 위임한다.

	// 스태미나 값이 변경될 때 호출 (서버: GE 실행 후 / 클라이언트: OnRep)
	void HandleStaminaChanged(AActor* Instigator, AActor* Causer,
		const FGameplayEffectSpec* Spec, float Magnitude,
		float OldValue, float NewValue);

	// 스태미나가 처음으로 0이 됐을 때 호출
	void HandleOutOfStamina(AActor* Instigator, AActor* Causer,
		const FGameplayEffectSpec* Spec, float Magnitude,
		float OldValue, float NewValue);

	// ASC::OnAbilityActivated 콜백: Ability.Type.Action.Dash 태그를 가진
	// 어빌리티가 활성화되면 StaminaDashCostEffect를 적용합니다.
	void HandleAbilityActivated(UGameplayAbility* ActivatedAbility);

	// Periodic GE로 구현된 스태미나 회복 이펙트.
	// DA_SHAbilitySet에 등록하지 않고 이 컴포넌트가 직접 관리합니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Stamina|Regen")
	TSubclassOf<UGameplayEffect> StaminaRegenEffect;

	// 대쉬 어빌리티(Ability.Type.Action.Dash) 활성화 시 적용되는 스태미나 소비 GE.
	// SHStaminaSet.StaminaCost를 설정해 Stamina를 차감합니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Stamina|DashCost")
	TSubclassOf<UGameplayEffect> StaminaDashCostEffect;

	// 대쉬 한 번에 필요한 최소 스태미나.
	// 이 값 미만이면 Ability.Type.Action.Dash 태그를 가진 어빌리티가 차단됩니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Stamina|DashCost")
	float DashStaminaCostThreshold = 50.0f;

	// ASC에서 가져온 스태미나 어트리뷰트 셋 (읽기 전용 참조)
	UPROPERTY()
	TObjectPtr<const USHStaminaSet> StaminaSet;
};
