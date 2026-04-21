// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

#include "SHGameplayAbility.generated.h"

/**
 * USHGameplayAbility
 *
 * SHLyraProject 공용 GameplayAbility 베이스.
 *
 * 제공 기능:
 *   - bLockMovementOnActivate: 활성 동안 캐릭터 MaxWalkSpeed를
 *     LockedWalkSpeed로 낮춰 '커밋형' 어빌리티(근접 공격, 캐스트 마법 등)의
 *     타격감을 만든다. EndAbility에서 원래 속도로 복원한다.
 *
 * 대쉬처럼 이동 자체가 목적인 어빌리티는 bLockMovementOnActivate=false로 둔다.
 */
UCLASS(Abstract)
class SHLYRAPROJECTRUNTIME_API USHGameplayAbility : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:

	USHGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	// 활성 중 캐릭터 이동을 제한할지 여부.
	// 근접/마법 같은 '커밋형' 어빌리티는 true로 설정.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Movement")
	bool bLockMovementOnActivate = false;

	// bLockMovementOnActivate=true일 때 적용할 MaxWalkSpeed (cm/s).
	// 0이면 공격 중 완전 정지. 소량의 입력 반응을 주려면 100~200 정도 권장.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Movement",
		meta = (EditCondition = "bLockMovementOnActivate", ClampMin = "0.0"))
	float LockedWalkSpeed = 0.0f;

private:

	// ActivateAbility 시점에 저장한 원래 MaxWalkSpeed. EndAbility에서 복원용.
	UPROPERTY(Transient)
	float CachedWalkSpeed = 0.0f;

	// 이번 Activate 사이클에서 속도를 실제로 변경했는지. 중복/누락 복원 방지.
	UPROPERTY(Transient)
	bool bWalkSpeedModified = false;
};
