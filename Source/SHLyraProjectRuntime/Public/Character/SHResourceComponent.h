// Copyright SH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

#include "SHResourceComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

// -------------------------------------------------------
// USHResourceComponent
//
// 마나/스태미나처럼 "어트리뷰트 + 자동 회복 + 소진 차단"을 가진 자원 컴포넌트의
// 공통 골격을 담은 추상 베이스. (Template Method 패턴)
//
// 베이스가 담당하는 공통 흐름:
//   1. ASC 준비 완료 후 자원 어트리뷰트 셋 델리게이트 바인딩
//   2. 값 변화 시 GameplayTag 관리 (소진 태그) + UI 브로드캐스트
//   3. Regen GE 최적화 관리 (가득 차면 제거, 소비되면 재적용)
//   4. 값 < 코스트 임계값 시 특정 어빌리티 태그 Block/Unblock
//
// 타입별로 다른 부분(어트리뷰트 셋 타입, 델리게이트, 태그, 메시지 payload)은
// 순수 가상 훅으로 파생 클래스에 위임한다.
//
// OnPostInitialize / OnPreUninitialize 는 "한 파생만 필요한 추가 동작"을 위한
// 빈 기본 구현 훅이다. 예: 스태미나는 ShooterCore의 GA_Hero_Dash(무수정 원칙상
// 코스트 GE를 박을 수 없는 빌려온 어빌리티)에 비용을 외부에서 물리기 위해
// AbilityActivatedCallbacks를 여기서 바인딩한다. 마나는 자체 어빌리티라
// CostGameplayEffectClass로 정석 차감하므로 이 훅이 필요 없다.
//
// 파생 concrete 클래스(USHManaComponent/USHStaminaComponent)가
// GameFeatureAction_AddComponents로 ALyraCharacter에 주입된다.
// -------------------------------------------------------
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

	// -------------------------------------------------------
	// ASC 초기화 콜백 (베이스 구현)
	// -------------------------------------------------------
	void OnAbilitySystemInitialized();
	void OnAbilitySystemUninitialized();

	// -------------------------------------------------------
	// 공통 로직 (베이스 구현) — 파생의 델리게이트 핸들러가 호출한다.
	// -------------------------------------------------------

	// 자원 값이 변할 때마다: Regen GE 토글 + 소진 태그 해제 + 임계값 Block/Unblock + 브로드캐스트
	void HandleValueChanged(float OldValue, float NewValue);

	// 자원이 처음 0이 됐을 때: 소진 태그 부여 + Regen 시작
	void HandleResourceDepleted();

	void ApplyRegenEffect();
	void RemoveRegenEffect();

	// 현재 값을 0~1로 반환 (UI용). 파생의 BlueprintCallable 래퍼가 사용한다.
	float GetValueNormalized() const;

	bool IsResourceDepleted() const { return bIsResourceDepleted; }

	// -------------------------------------------------------
	// 타입별 훅 (파생이 구현)
	// -------------------------------------------------------

	// UCLASS는 추상이라도 CDO 생성을 위해 C++상 concrete여야 하므로
	// '= 0' 대신 PURE_VIRTUAL 매크로를 쓴다 (베이스 호출 시 런타임 assert, 파생이 override).

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

	// -------------------------------------------------------
	// 확장 훅 (빈 기본 구현 — 필요한 파생만 override)
	// -------------------------------------------------------

	// 초기화 마지막에 호출. 한 파생만 필요한 추가 바인딩에 사용 (스태미나 대쉬 코스트 감시 등).
	virtual void OnPostInitialize() {}

	// 해제 시작에 호출. OnPostInitialize에서 건 바인딩을 정리한다 (CachedASC 유효 시점).
	virtual void OnPreUninitialize() {}

	// -------------------------------------------------------
	// 공통 멤버 변수
	// -------------------------------------------------------

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
