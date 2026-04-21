// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/SHGameplayAbility.h"

#include "SHMagicProjectile.generated.h"

class UAnimMontage;
class UGameplayEffect;
class ASHMagicProjectileActor;
struct FGameplayEventData;

/**
 * USHMagicProjectile
 *
 * 마법 발사체 어빌리티. ULyraGameplayAbility 상속.
 *
 * 실행 흐름:
 *   1. ActivateAbility → CommitAbility (비용/쿨다운)
 *   2. UAbilityTask_PlayMontageAndWait 로 CastMontage 재생
 *   3. UAbilityTask_WaitGameplayEvent 로 'Event.SH.Magic.Launch' 대기
 *      → AnimNotify 가 해당 태그를 발화하면 SpawnProjectile 호출
 *   4. SpawnProjectile: 서버(Authority)에서만 ASHMagicProjectileActor 스폰
 *      → InitProjectile 로 InstigatorASC + DamageEffect 주입
 *   5. 몽타주 완료/취소 → EndAbility
 *
 * 비용/쿨다운 설정:
 *   BP 자식 클래스(GA_SHMagicProjectile)에서
 *   CostGameplayEffectClass     = GE_SHMagicManaCost  (USHManaSet 구현 후 연결)
 *   CooldownGameplayEffectClass = GE_SHMagicCooldown 으로 지정한다.
 *   마나 시스템 구현 전까지는 CostGameplayEffectClass 를 비워둔다.
 *
 * 스폰 위치:
 *   SpawnSocketName 소켓이 AvatarActor 에 존재하면 해당 위치에서 스폰한다.
 *   소켓이 없으면 캐릭터 위치 + SpawnOffset 위치에서 전방으로 발사한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHMagicProjectile : public USHGameplayAbility
{
	GENERATED_BODY()

public:

	USHMagicProjectile(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	// AnimNotify 가 'Event.SH.Magic.Launch' 를 발화하면 호출됨.
	// 서버에서만 발사체를 스폰한다.
	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);

	// 서버 전용 — ASHMagicProjectileActor 스폰 후 InitProjectile 호출
	void SpawnProjectile(const FGameplayAbilityActorInfo* ActorInfo);

protected:

	// 캐스팅 애니메이션 몽타주. BP 자식 클래스에서 지정한다.
	// 발사 타이밍에 AnimNotify(Event.SH.Magic.Launch)를 추가해야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic")
	TObjectPtr<UAnimMontage> CastMontage;

	// 스폰할 발사체 액터 클래스 (BP_SHMagicProjectile).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic")
	TSubclassOf<ASHMagicProjectileActor> ProjectileClass;

	// 충돌 시 대상에게 적용할 데미지 GE (GE_SHMagicDamage).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// 발사체 스폰 기준 소켓 이름.
	// 해당 소켓이 없으면 캐릭터 위치 + SpawnOffset 에서 스폰한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic|Spawn")
	FName SpawnSocketName = FName("hand_r");

	// 소켓이 없을 때 사용하는 캐릭터 기준 스폰 오프셋 (cm).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic|Spawn")
	FVector SpawnOffset = FVector(50.f, 0.f, 50.f);
};
