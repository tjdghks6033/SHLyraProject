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
 * 마법 발사체 어빌리티. CastMontage의 AnimNotify가 'Event.SH.Magic.Launch'를 발화하면
 * 서버(Authority)에서만 ASHMagicProjectileActor를 스폰해 DamageEffect를 주입한다.
 *
 * 비용/쿨다운은 BP 자식 클래스(GA_SHMagicProjectile)의
 * CostGameplayEffectClass/CooldownGameplayEffectClass로 지정한다.
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

	// 발사 타이밍에 AnimNotify(Event.SH.Magic.Launch)를 추가해야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic")
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic")
	TSubclassOf<ASHMagicProjectileActor> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// 소켓이 없으면 SpawnOffset 기준으로 대체한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic|Spawn")
	FName SpawnSocketName = FName("hand_r");

	UPROPERTY(EditDefaultsOnly, Category = "SH|Magic|Spawn")
	FVector SpawnOffset = FVector(50.f, 0.f, 50.f);
};
