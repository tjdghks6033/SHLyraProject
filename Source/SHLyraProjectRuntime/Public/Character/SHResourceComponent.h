// Copyright SH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

#include "SHResourceComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * USHResourceComponent
 *
 * 마나/스태미나처럼 "어트리뷰트 + 자동 회복 + 소진 차단"을 가진 자원 컴포넌트의
 * 공통 골격을 담은 추상 베이스. 어트리뷰트 셋 타입·태그·메시지 payload 등
 * 타입별로 다른 부분만 순수 가상 훅으로 파생 클래스에 위임한다.
 *
 * OnPostInitialize/OnPreUninitialize는 한 파생만 필요한 추가 동작을 위한 빈 훅이다.
 * 예: 스태미나는 ShooterCore의 GA_Hero_Dash(무수정 원칙상 코스트 GE를 못 박는 빌려온
 * 어빌리티)에 비용을 외부에서 물리기 위해 AbilityActivatedCallbacks를 여기서 바인딩한다.
 * 마나는 자체 어빌리티라 CostGameplayEffectClass로 정석 차감하므로 이 훅이 필요 없다.
 */
UCLASS(Abstract)
class SHLYRAPROJECTRUNTIME_API USHResourceComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:

	USHResourceComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~ UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of UActorComponent interface

	void OnAbilitySystemInitialized();
	void OnAbilitySystemUninitialized();

	// 이하는 파생의 델리게이트 핸들러가 호출하는 공통 로직.

	// 자원 값이 변할 때마다: Regen GE 토글 + 소진 태그 해제 + 임계값 Block/Unblock + 브로드캐스트
	void HandleValueChanged(float OldValue, float NewValue);

	// 자원이 처음 0이 됐을 때: 소진 태그 부여 + Regen 시작
	void HandleResourceDepleted();

	void ApplyRegenEffect();
	void RemoveRegenEffect();

	// 현재 값을 0~1로 반환 (UI용). 파생의 BlueprintCallable 래퍼가 사용한다.
	float GetValueNormalized() const;

	bool IsResourceDepleted() const { return bIsResourceDepleted; }

	// 이하는 파생이 구현할 타입별 훅. UCLASS는 추상이라도 CDO 생성을 위해 C++상
	// concrete여야 하므로 '= 0' 대신 PURE_VIRTUAL 매크로를 쓴다 (베이스 호출 시 런타임
	// assert, 파생이 override).

	// ASC에서 자원 어트리뷰트 셋을 찾아 파생 멤버에 캐싱한다. 찾으면 true.
	virtual bool ResolveAttributeSet(UAbilitySystemComponent* ASC) PURE_VIRTUAL(USHResourceComponent::ResolveAttributeSet, return false;);

	// 어트리뷰트 셋의 값 변화/소진 델리게이트를 파생 핸들러에 바인딩한다.
	virtual void BindValueDelegates() PURE_VIRTUAL(USHResourceComponent::BindValueDelegates, );

	// 델리게이트 바인딩 해제 + 캐싱한 어트리뷰트 셋 포인터 정리.
	virtual void ReleaseAttributeSet() PURE_VIRTUAL(USHResourceComponent::ReleaseAttributeSet, );

	virtual float GetCurrentValue() const PURE_VIRTUAL(USHResourceComponent::GetCurrentValue, return 0.0f;);
	virtual float GetMaxValue() const PURE_VIRTUAL(USHResourceComponent::GetMaxValue, return 0.0f;);

	// 자원 소진 상태를 나타내는 GameplayTag (예: SH.Status.OutOfMana).
	virtual FGameplayTag GetDepletedStatusTag() const PURE_VIRTUAL(USHResourceComponent::GetDepletedStatusTag, return FGameplayTag(););

	// 자원 부족 시 차단할 어빌리티 분류 태그 (예: Ability.Type.Action.Magic).
	virtual FGameplayTag GetBlockedAbilityTag() const PURE_VIRTUAL(USHResourceComponent::GetBlockedAbilityTag, return FGameplayTag(););

	// 컴포넌트가 직접 관리하는 Periodic 회복 GE.
	virtual TSubclassOf<UGameplayEffect> GetRegenEffect() const PURE_VIRTUAL(USHResourceComponent::GetRegenEffect, return nullptr;);

	// 어빌리티 1회에 필요한 최소 자원. 이 값 미만이면 BlockedAbilityTag를 차단한다.
	virtual float GetCostThreshold() const PURE_VIRTUAL(USHResourceComponent::GetCostThreshold, return 0.0f;);

	// GameplayMessageSubsystem으로 UI에 자원 정보를 브로드캐스트한다 (payload는 파생별).
	virtual void BroadcastChange(float CurrentValue, float MaxValue) const PURE_VIRTUAL(USHResourceComponent::BroadcastChange, );

	// 이하는 필요한 파생만 override하는 확장 훅 (빈 기본 구현).

	// 초기화 마지막에 호출. 한 파생만 필요한 추가 바인딩에 사용 (스태미나 대쉬 코스트 감시 등).
	virtual void OnPostInitialize() {}

	// 해제 시작에 호출. OnPostInitialize에서 건 바인딩을 정리한다 (CachedASC 유효 시점).
	virtual void OnPreUninitialize() {}

	// ASC 초기화 이후 캐싱. Regen GE 적용/제거, Block/Unblock에 사용.
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	// 현재 활성화된 Regen GE 핸들. 제거 시 이 핸들로 식별한다.
	FActiveGameplayEffectHandle RegenEffectHandle;

	// 자원 소진 상태 플래그 (중복 처리 방지).
	bool bIsResourceDepleted = false;

	// 어빌리티가 현재 차단된 상태인지 추적 (중복 Block/Unblock 호출 방지).
	bool bAbilityBlocked = false;
};
