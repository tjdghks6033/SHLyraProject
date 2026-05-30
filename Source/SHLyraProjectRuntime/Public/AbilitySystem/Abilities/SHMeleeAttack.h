// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/SHMeleeAttackBase.h"

#include "SHMeleeAttack.generated.h"

/**
 * USHMeleeAttack
 *
 * 플레이어 근접 공격. 공통 흐름은 USHMeleeAttackBase가 담당하고,
 * 이 클래스는 무기 메시 소켓(WeaponHilt → WeaponTip) 기반 스윕 판정을 제공한다.
 * 소켓이 없으면 캐릭터 전방 구체 트레이스로 폴백한다.
 *
 * 비용/쿨다운:
 *   BP 자식(GA_SHMeleeAttack)에서
 *   CostGameplayEffectClass     = GE_SHMeleeStaminaCost
 *   CooldownGameplayEffectClass = GE_SHMeleeCooldown 으로 지정한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHMeleeAttack : public USHMeleeAttackBase
{
	GENERATED_BODY()

public:

	USHMeleeAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~ USHMeleeAttackBase 타입별 훅
	virtual FGameplayTag GetHitDetectTag() const override;
	virtual bool ComputeHitTrace(const FGameplayAbilityActorInfo* ActorInfo,
		FVector& OutStart, FVector& OutEnd, float& OutRadius) const override;
	//~ End of USHMeleeAttackBase 타입별 훅

	// 무기 메시 컴포넌트 이름. BP_SHCharacter 의 Static Mesh Component 이름과 일치해야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee|Socket")
	FName WeaponMeshComponentName = FName("WeaponMesh");

	// 트레이스 시작 소켓 이름 (손잡이 끝).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee|Socket")
	FName HiltSocketName = FName("WeaponHilt");

	// 트레이스 종료 소켓 이름 (검 끝).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee|Socket")
	FName TipSocketName = FName("WeaponTip");

	// Sweep 구체 반지름 (cm). 소켓 트레이스 및 폴백 구체 트레이스 모두에 적용.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee", meta = (ClampMin = "1.0"))
	float SweepRadius = 15.0f;

	// 소켓이 없을 때 사용하는 폴백 트레이스 거리 (cm).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee", meta = (ClampMin = "1.0"))
	float FallbackTraceRange = 150.0f;
};
