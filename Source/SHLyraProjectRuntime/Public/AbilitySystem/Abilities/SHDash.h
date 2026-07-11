// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GameplayTagContainer.h"

#include "SHDash.generated.h"

class UAnimMontage;

/**
 * USHDash
 *
 * 4방향 대쉬 어빌리티. UAbilityTask_ApplyRootMotionConstantForce로 이동해 CMC 예측과 호환된다.
 *
 * AbilityTags에 'Ability.Type.Action.Dash'를 포함하면 USHStaminaComponent가
 * AbilityActivatedCallbacks로 자동으로 스태미나를 차감하므로 별도 Cost GE가 필요 없다.
 * 쿨다운은 BP 자식 클래스(GA_SHDash)의 CooldownGameplayEffectClass로 지정한다.
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

	// 입력 방향을 캐릭터 로컬 기준 4방향으로 매핑한다. 입력 없으면 전방 폴백.
	void ResolveDashDirection(const ACharacter* Character,
		FVector& OutWorldDirection, UAnimMontage*& OutMontage) const;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash", meta = (ClampMin = "100.0"))
	float DashStrength = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash", meta = (ClampMin = "0.05"))
	float DashDuration = 0.35f;

	// 600 기본값은 Lyra 캐릭터 기본 이동속도(cm/s)에 맞춘 것.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash", meta = (ClampMin = "0.0"))
	float DashFinishClampVelocity = 600.0f;

	// 입력 없을 때 폴백으로도 쓰인다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash|Montages")
	TObjectPtr<UAnimMontage> DashMontage_Forward;

	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash|Montages")
	TObjectPtr<UAnimMontage> DashMontage_Backward;

	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash|Montages")
	TObjectPtr<UAnimMontage> DashMontage_Left;

	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash|Montages")
	TObjectPtr<UAnimMontage> DashMontage_Right;

	// GCN_SH_GhostTrail(GameplayCueNotify_Actor)에서 처리. 비워 두면 잔상 없음.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Dash|VFX", meta = (Categories = "GameplayCue"))
	FGameplayTag GhostTrailCueTag;
};
