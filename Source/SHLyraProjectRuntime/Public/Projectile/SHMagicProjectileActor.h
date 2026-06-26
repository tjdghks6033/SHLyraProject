// Copyright SH. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "SHMagicProjectileActor.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class UAbilitySystemComponent;

/**
 * ASHMagicProjectileActor
 *
 * 마법 발사체 베이스. 아이스볼트/파이어볼 등 모든 마법 발사체가 공유한다.
 *
 * 책임: "맞으면 DamageEffect + OnHitEffects 목록을 적용하고 ImpactGameplayCueTag를 실행한다."
 * 빙결 스택 카운트, 점화 DoT 등 상태이상 로직은 여기에 없다.
 * 비주얼(VFX/사운드)은 직접 Niagara를 스폰하지 않고 GameplayCue 태그로만 신호를 보낸다.
 * 상태이상 반응은 대상 캐릭터(ASHEnemyBase 등)가 자신의 ASC를 구독해 처리한다.
 *
 * BP 자식 클래스별 설정 예:
 *   BP_SHIceBoltProjectile
 *     OnHitEffects          = [GE_SHFrozenStack]
 *     ImpactGameplayCueTag  = GameplayCue.SH.IceBolt.Impact
 *   BP_SHFireballProjectile
 *     OnHitEffects          = [GE_SHIgniteStatus]
 *     ImpactGameplayCueTag  = GameplayCue.SH.Fireball.Impact
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

	// 충돌 시 재생할 임팩트 효과의 GameplayCue 태그.
	// 폰(Pawn)에 맞으면 대상 ASC에서, 벽/지형에 맞으면 인스티게이터 ASC에서 Execute된다.
	// BP 자식 클래스에서 GameplayCue.SH.IceBolt.Impact 등 발사체별 태그를 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Projectile|VFX", meta = (Categories = "GameplayCue"))
	FGameplayTag ImpactGameplayCueTag;

	// 히트 시 데미지와 별개로 타겟에 추가 적용할 GE 목록.
	// 빙결 스택, 점화, 둔화 등 속성별 GE를 BP 자식 클래스에서 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Projectile|Effects")
	TArray<TSubclassOf<UGameplayEffect>> OnHitEffects;

	// 피격 대상에게 적용할 수평 넉백 힘 (cm/s). 0이면 넉백 없음.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Projectile|Effects")
	float KnockbackStrength = 0.f;

	// 넉백 수직 힘 (cm/s). KnockbackStrength > 0 일 때만 적용.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Projectile|Effects")
	float KnockbackZStrength = 80.f;

	// 히트 성공 시 적용할 GlobalTimeDilation. 0이면 HitStop 없음 (권장값: 0.05).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Projectile|HitStop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitStopTimeDilation = 0.f;

	// HitStop 실시간 지속 시간 (초).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Projectile|HitStop", meta = (ClampMin = "0.0"))
	float HitStopDuration = 0.06f;

private:

	// Pawn 등 Overlap 대상에 충돌 시 (데미지 + 임팩트 GameplayCue)
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 벽 등 Block 대상에 충돌해 ProjectileMovement가 멈췄을 때 (임팩트 GameplayCue만)
	UFUNCTION()
	void OnProjectileStopped(const FHitResult& ImpactResult);

	// 충돌 위치에서 ImpactGameplayCueTag를 Execute한다.
	// TargetASC가 유효하면 대상 ASC에서, 없으면 인스티게이터 ASC에서 실행한다.
	void ExecuteImpactCue(const FVector& Location, const FVector& Normal,
		UAbilitySystemComponent* TargetASC = nullptr);

	void DestroyProjectile();

	TSubclassOf<UGameplayEffect> DamageEffect;
	TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC;
	float AbilityLevel = 1.f;

	// 한 번의 발사에서 같은 대상에게 중복 데미지 방지
	TSet<TWeakObjectPtr<AActor>> DamagedActors;
};
