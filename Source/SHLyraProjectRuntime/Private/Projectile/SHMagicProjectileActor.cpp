// Copyright SH. All Rights Reserved.

#include "Projectile/SHMagicProjectileActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"

ASHMagicProjectileActor::ASHMagicProjectileActor()
{
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(15.f);
	CollisionSphere->SetCollisionProfileName(TEXT("Projectile"));
	RootComponent = CollisionSphere;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	InitialLifeSpan = 5.f;
}

void ASHMagicProjectileActor::InitProjectile(UAbilitySystemComponent* InInstigatorASC,
	TSubclassOf<UGameplayEffect> InDamageEffect,
	float InAbilityLevel)
{
	InstigatorASC = InInstigatorASC;
	DamageEffect = InDamageEffect;
	AbilityLevel = InAbilityLevel;

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASHMagicProjectileActor::OnSphereOverlap);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ASHMagicProjectileActor::OnProjectileStopped);
}

void ASHMagicProjectileActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetInstigator())
	{
		return;
	}

	if (DamagedActors.Contains(OtherActor))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);

	if (TargetASC && InstigatorASC.IsValid() && DamageEffect)
	{
		FGameplayEffectContextHandle Context = InstigatorASC->MakeEffectContext();
		Context.AddHitResult(SweepResult);
		FGameplayEffectSpecHandle SpecHandle =
			InstigatorASC->MakeOutgoingSpec(DamageEffect, AbilityLevel, Context);

		if (SpecHandle.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}

		DamagedActors.Add(OtherActor);
	}

	if (bDestroyOnHit)
	{
		SpawnImpactEffect(GetActorLocation());
		DestroyProjectile();
	}
}

void ASHMagicProjectileActor::OnProjectileStopped(const FHitResult& ImpactResult)
{
	// 벽/지형 등 Block 대상 충돌 — 데미지 없이 임팩트 VFX만 재생
	if (bDestroyOnHit)
	{
		SpawnImpactEffect(ImpactResult.ImpactPoint);
		DestroyProjectile();
	}
}

void ASHMagicProjectileActor::SpawnImpactEffect(const FVector& Location)
{
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, ImpactEffect, Location, GetActorRotation());
	}
}

void ASHMagicProjectileActor::DestroyProjectile()
{
	// ProjectileMovement 비활성화 후 파괴 — 이동 중 중복 충돌 방지
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}
	Destroy();
}
