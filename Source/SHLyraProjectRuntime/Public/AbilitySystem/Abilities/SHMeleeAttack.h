// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

#include "SHMeleeAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;
struct FGameplayEventData;

/**
 * USHMeleeAttack
 *
 * 근접 공격 어빌리티. ULyraGameplayAbility 상속.
 *
 * 실행 흐름:
 *   1. ActivateAbility → CommitAbility (비용/쿨다운)
 *   2. UAbilityTask_PlayMontageAndWait 로 AttackMontage 재생
 *   3. UAbilityTask_WaitGameplayEvent 로 'Event.SH.Melee.HitDetect' 대기
 *      → AnimNotify_GameplayEvent 가 해당 태그를 발화하면 OnGameplayEventReceived 호출
 *   4. PerformMeleeHit: WeaponHilt → WeaponTip 소켓 기반 Sphere Sweep → DamageEffect 적용
 *      (소켓이 없으면 캐릭터 전방 구체 트레이스로 폴백)
 *   5. 몽타주 완료/취소 → EndAbility
 *
 * 비용/쿨다운 설정:
 *   BP 자식 클래스(GA_SHMeleeAttack)에서
 *   CostGameplayEffectClass     = GE_SHMeleeStaminaCost
 *   CooldownGameplayEffectClass = GE_SHMeleeCooldown 으로 지정한다.
 *
 * 소켓 설정:
 *   BP_SHCharacter 의 WeaponMesh 컴포넌트에 WeaponHilt / WeaponTip 소켓을 추가한다.
 *   소켓 이름은 WeaponMeshComponentName / HiltSocketName / TipSocketName 으로 변경 가능.
 *
 * 히트 판정은 서버(Authority)에서만 실행된다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHMeleeAttack : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:

	USHMeleeAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	// 몽타주가 정상 완료(OnCompleted / OnBlendOut)됐을 때 호출
	UFUNCTION()
	void OnMontageCompleted();

	// 몽타주가 취소(OnCancelled / OnInterrupted)됐을 때 호출
	UFUNCTION()
	void OnMontageCancelled();

	// AnimNotify_GameplayEvent 가 'Event.SH.Melee.HitDetect' 를 발화하면 호출됨.
	// 서버에서만 PerformMeleeHit 을 실행한다.
	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);

	// 캐릭터 전방으로 구체 트레이스를 수행하고
	// 범위 내 적 Actor 의 ASC 에 DamageEffect 를 적용한다.
	void PerformMeleeHit(const FGameplayAbilityActorInfo* ActorInfo);

protected:

	// 공격 애니메이션 몽타주. BP 자식 클래스에서 지정한다.
	// 몽타주에 AnimNotify_GameplayEvent(Event.SH.Melee.HitDetect)를 추가해야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 히트 판정 성공 시 대상에게 적용할 데미지 GE (GE_SHMeleeDamage).
	// LyraHealthSet.Damage 메타 어트리뷰트를 수정하도록 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee")
	TSubclassOf<UGameplayEffect> DamageEffect;

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

	// 개발 중 트레이스 궤적 시각화 여부. 배포 빌드에서는 꺼야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Melee|Debug")
	bool bDrawDebugTrace = false;
};
