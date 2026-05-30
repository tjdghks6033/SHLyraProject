// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/SHMeleeAttackBase.h"

#include "SHBossMeleeAttack.generated.h"

/**
 * USHBossMeleeAttack
 *
 * 보스 근접 공격. 공통 흐름은 USHMeleeAttackBase가 담당하고,
 * 이 클래스는 스켈레탈 메시 소켓(hand_r) 중심의 단일 위치 구체 오버랩 판정을 제공한다.
 * 소켓이 없으면 캐릭터 전방 구체 트레이스로 폴백한다.
 *
 * BT 연동:
 *   BTTask_SHActivateAbility 가 AbilityTag=Ability.SH.Boss.Melee 로 이 어빌리티를 발동한다.
 *   쿨다운 간격은 BT 의 Wait 노드로 제어한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHBossMeleeAttack : public USHMeleeAttackBase
{
	GENERATED_BODY()

public:

	USHBossMeleeAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~ USHMeleeAttackBase 타입별 훅
	virtual FGameplayTag GetHitDetectTag() const override;
	virtual bool ComputeHitTrace(const FGameplayAbilityActorInfo* ActorInfo,
		FVector& OutStart, FVector& OutEnd, float& OutRadius) const override;
	//~ End of USHMeleeAttackBase 타입별 훅

	// 히트 판정 중심이 될 스켈레탈 메시 소켓 이름.
	// 소켓이 없으면 캐릭터 전방 구체 트레이스로 폴백.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Boss|Melee")
	FName AttackSocketName = FName("hand_r");

	// 히트 판정 구체 반지름 (cm). 보스 공격 범위에 맞게 넉넉히 설정.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Boss|Melee", meta = (ClampMin = "1.0"))
	float HitRadius = 120.0f;

	// 소켓이 없을 때 전방 트레이스 거리 (cm).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Boss|Melee", meta = (ClampMin = "1.0"))
	float FallbackRange = 200.0f;
};
