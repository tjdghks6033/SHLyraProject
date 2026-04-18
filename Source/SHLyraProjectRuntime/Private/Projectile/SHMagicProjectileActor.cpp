// Copyright SH. All Rights Reserved.

#include "Projectile/SHMagicProjectileActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

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
		FGameplayEffectSpecHandle SpecHandle =
			InstigatorASC->MakeOutgoingSpec(DamageEffect, AbilityLevel, InstigatorASC->MakeEffectContext());

		if (SpecHandle.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}

		DamagedActors.Add(OtherActor);
	}

	if (bDestroyOnHit)
	{
		Destroy();
	}
}
