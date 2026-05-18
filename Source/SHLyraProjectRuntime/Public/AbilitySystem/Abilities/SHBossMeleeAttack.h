// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/SHGameplayAbility.h"

#include "SHBossMeleeAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;
struct FGameplayEventData;

/**
 * USHBossMeleeAttack
 *
 * 보스 근접 공격 어빌리티.
 *
 * 실행 흐름:
 *   1. ActivateAbility → CommitAbility
 *   2. UAbilityTask_PlayMontageAndWait 로 AttackMontage 재생
 *   3. UAbilityTask_WaitGameplayEvent 로 'Event.SH.Boss.HitDetect' 대기
 *      → AnimNotify_GameplayEvent 발화 시 OnGameplayEventReceived 호출
 *   4. PerformBossHit: AttackSocket 중심 SphereTrace → DamageEffect 적용
 *      (소켓이 없으면 캐릭터 전방 구체 트레이스로 폴백)
 *   5. 몽타주 완료/취소 → EndAbility
 *
 * BT 연동:
 *   BTTask_SHActivateAbility 가 AbilityTag=Ability.SH.Boss.Melee 로 이 어빌리티를 발동한다.
 *   쿨다운 간격은 BT 의 Wait 노드로 제어한다.
 *
 * 히트 판정은 서버(Authority)에서만 실행된다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHBossMeleeAttack : public USHGameplayAbility
{
	GENERATED_BODY()

public:

	USHBossMeleeAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	// AnimNotify_GameplayEvent 가 'Event.SH.Boss.HitDetect' 를 발화하면 호출됨.
	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);

	// AttackSocket 위치를 중심으로 SphereTrace 를 수행하고
	// 범위 내 대상의 ASC 에 DamageEffect 를 적용한다.
	void PerformBossHit(const FGameplayAbilityActorInfo* ActorInfo);

protected:

	// 공격 애니메이션 몽타주. GA_SHBossMelee BP 에서 지정한다.
	// 몽타주에 AnimNotify_GameplayEvent(Event.SH.Boss.HitDetect) 를 추가해야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Boss|Melee")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 히트 성공 시 대상에 적용할 데미지 GE (GE_SHBossMeleeDamage).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Boss|Melee")
	TSubclassOf<UGameplayEffect> DamageEffect;

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

	UPROPERTY(EditDefaultsOnly, Category = "SH|Boss|Melee|Debug")
	bool bDrawDebugTrace = false;
};
