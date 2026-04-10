// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

#include "SHDash.generated.h"

class UAnimMontage;

/**
 * USHDash
 *
 * 4방향 대쉬 어빌리티. ULyraGameplayAbility 상속.
 *
 * 실행 흐름:
 *   1. ActivateAbility → CommitAbility (쿨다운)
 *   2. 이동 입력 방향을 캐릭터 로컬 좌표로 변환해 4방향(전/후/좌/우) 결정
 *      입력 없으면 전방(Forward) 폴백
 *   3. UAbilityTask_ApplyRootMotionConstantForce 로 해당 방향 대쉬 이동 (CMC 예측 호환)
 *   4. 방향에 맞는 몽타주 재생 (미설정 시 생략)
 *   5. 루트 모션 종료(OnFinish) → EndAbility
 *
 * 스태미나 연동:
 *   AbilityTags 에 'Ability.Type.Action.Dash' 태그를 포함하므로,
 *   USHStaminaComponent 가 AbilityActivatedCallbacks 를 통해 자동으로 스태미나를 차감하고
 *   스태미나 부족 시 BlockAbilitiesWithTags 로 발동을 차단합니다.
 *   이 어빌리티에 별도 Cost GE 를 설정할 필요가 없습니다.
 *
 * 쿨다운:
 *   BP 자식 클래스(GA_SHDash)에서 CooldownGameplayEffectClass 를 지정합니다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHDash : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:

	USHDash(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~ UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End of UGameplayAbility interface

private:

	// 루트 모션 태스크 완료 시 호출. EndAbility 를 트리거합니다.
	UFUNCTION()
	void OnDashFinished();

	// 이동 입력 벡터를 캐릭터 로컬 방향으로 변환해 4방향 중 하나를 선택하고
	// 대응하는 몽타주와 월드 방향 벡터를 반환합니다.
	// 입력이 없으면 전방(Forward)으로 폴백합니다.
	void ResolveDashDirection(const ACharacter* Character,
		FVector& OutWorldDirection, UAnimMontage*& OutMontage) const;

protected:

	// 대쉬 이동 힘의 세기 (cm/s 단위의 루트 모션 힘).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash", meta = (ClampMin = "100.0"))
	float DashStrength = 3000.0f;

	// 대쉬 지속 시간 (초).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash", meta = (ClampMin = "0.05"))
	float DashDuration = 0.35f;

	// 대쉬 종료 후 속도를 클램프할 최대값 (cm/s).
	// 기본값 600 은 Lyra 캐릭터의 일반 이동 속도에 해당합니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash", meta = (ClampMin = "0.0"))
	float DashFinishClampVelocity = 600.0f;

	// 전방 대쉬 몽타주. 입력 없을 때 폴백으로도 사용됩니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash|Montages")
	TObjectPtr<UAnimMontage> DashMontage_Forward;

	// 후방 대쉬 몽타주.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash|Montages")
	TObjectPtr<UAnimMontage> DashMontage_Backward;

	// 좌측 대쉬 몽타주.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash|Montages")
	TObjectPtr<UAnimMontage> DashMontage_Left;

	// 우측 대쉬 몽타주.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash|Montages")
	TObjectPtr<UAnimMontage> DashMontage_Right;
};
