// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

#include "SHGameplayAbility.generated.h"

class ACharacter;

/**
 * USHGameplayAbility
 *
 * SHLyraProject 공용 GameplayAbility 베이스.
 *
 * 제공 기능:
 *   - bLockMovementOnActivate: 활성 동안 캐릭터 MaxWalkSpeed를
 *     LockedWalkSpeed로 낮춰 '커밋형' 어빌리티(근접 공격, 캐스트 마법 등)의
 *     타격감을 만든다. EndAbility에서 캐릭터 클래스 CDO 의 기본값으로 복원.
 *
 * 대쉬처럼 이동 자체가 목적인 어빌리티는 bLockMovementOnActivate=false로 둔다.
 *
 * 설계 메모:
 *   런타임 값을 캐시하지 않고 CDO 기본값으로 복원한다.
 *   → 중복 활성/이벤트 누락으로 인한 '락 스턱' 버그가 구조적으로 발생하지 않는다.
 *   → 단점: 다른 시스템이 런타임에 속도를 수정하고 있었다면 그 값도 함께 리셋됨.
 *     (현 프로젝트에는 해당 케이스 없음)
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

	// 락을 건 캐릭터. ActorInfo 의존 없이 EndAbility 에서 안전하게 복원하기 위해 보관.
	TWeakObjectPtr<ACharacter> LockedCharacter;
};
