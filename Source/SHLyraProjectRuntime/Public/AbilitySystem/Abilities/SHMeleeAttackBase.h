// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/SHGameplayAbility.h"

#include "SHMeleeAttackBase.generated.h"

class UAnimMontage;
class UGameplayEffect;
struct FGameplayEventData;

/**
 * USHMeleeAttackBase
 *
 * 근접 공격 어빌리티 공통 베이스 (추상).
 *
 * 공통 흐름:
 *   1. ActivateAbility → CommitAbility (비용/쿨다운)
 *   2. UAbilityTask_PlayMontageAndWait 로 AttackMontage 재생
 *   3. UAbilityTask_WaitGameplayEvent 로 GetHitDetectTag() 대기
 *      → AnimNotify_GameplayEvent 발화 시 OnGameplayEventReceived 호출
 *   4. PerformHit: ComputeHitTrace() 가 계산한 볼륨으로 SphereTrace → DamageEffect 적용
 *   5. 몽타주 완료/취소 → EndAbility
 *
 * 파생은 히트 트리거 태그(GetHitDetectTag)와 판정 볼륨(ComputeHitTrace)만 구현한다.
 * 히트 판정은 서버(Authority)에서만 실행된다.
 */
UCLASS(Abstract)
class SHLYRAPROJECTRUNTIME_API USHMeleeAttackBase : public USHGameplayAbility
{
	GENERATED_BODY()

protected:

	//~ UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	//~ End of UGameplayAbility interface

	// '= 0' 대신 PURE_VIRTUAL — UCLASS는 추상이라도 CDO 생성 때문에 C++상 concrete여야 한다.

	// 히트를 트리거하는 AnimNotify_GameplayEvent 태그 (예: Event.SH.Melee.HitDetect).
	virtual FGameplayTag GetHitDetectTag() const PURE_VIRTUAL(USHMeleeAttackBase::GetHitDetectTag, return FGameplayTag(););

	// 히트 판정 구체 스윕의 시작/끝/반지름을 계산한다. 유효하면 true.
	// (Start==End 이면 단일 위치 오버랩)
	virtual bool ComputeHitTrace(const FGameplayAbilityActorInfo* ActorInfo,
		FVector& OutStart, FVector& OutEnd, float& OutRadius) const
		PURE_VIRTUAL(USHMeleeAttackBase::ComputeHitTrace, return false;);

	// 공격 애니메이션 몽타주. AnimNotify_GameplayEvent(GetHitDetectTag())를 포함해야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 히트 성공 시 대상 ASC에 적용할 데미지 GE.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// 개발 중 트레이스 궤적 시각화 여부. 배포 빌드에서는 꺼야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee|Debug")
	bool bDrawDebugTrace = false;

private:

	// 몽타주 정상 완료(OnCompleted / OnBlendOut)
	UFUNCTION()
	void OnMontageCompleted();

	// 몽타주 취소(OnCancelled / OnInterrupted)
	UFUNCTION()
	void OnMontageCancelled();

	// AnimNotify_GameplayEvent 발화 시 호출. 서버에서만 PerformHit 실행.
	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);

	// ComputeHitTrace 기반 SphereTrace 후 범위 내 대상 ASC에 DamageEffect 적용.
	void PerformHit(const FGameplayAbilityActorInfo* ActorInfo);
};
