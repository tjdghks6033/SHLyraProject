// Copyright SH. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "SHMagicProjectileActor.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class UAbilitySystemComponent;

/**
 * ASHMagicProjectileActor
 *
 * GA_SHMagicProjectile 어빌리티가 스폰하는 마법 발사체.
 *
 * - 어빌리티가 InitProjectile()로 인스티게이터 ASC와 데미지 GE 클래스를 주입한다.
 * - 충돌(Overlap) 시 대상 ASC에 DamageEffect를 적용하고 자신을 파괴한다.
 * - bReplicates = true, ProjectileMovement 복제로 멀티플레이 이동 동기화.
 * - VFX(Niagara)는 BP 자식 클래스에서 컴포넌트를 추가해 연결한다.
 */
UCLASS(Abstract)
class SHLYRAPROJECTRUNTIME_API ASHMagicProjectileActor : public AActor
{
	GENERATED_BODY()

public:

	ASHMagicProjectileActor();

	// 어빌리티가 스폰 직후 호출해 데미지 정보를 주입한다.
	void InitProjectile(UAbilitySystemComponent* InInstigatorASC,
		TSubclassOf<UGameplayEffect> InDamageEffect,
		float InAbilityLevel = 1.f);

protected:

	UPROPERTY(VisibleAnywhere, Category = "SH|Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "SH|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// false 로 설정하면 첫 충돌 후에도 계속 날아간다 (관통형).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Projectile")
	bool bDestroyOnHit = true;

private:

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	TSubclassOf<UGameplayEffect> DamageEffect;
	TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC;
	float AbilityLevel = 1.f;

	// 한 번의 발사에서 같은 대상에게 중복 데미지 방지
	TSet<TWeakObjectPtr<AActor>> DamagedActors;
};
