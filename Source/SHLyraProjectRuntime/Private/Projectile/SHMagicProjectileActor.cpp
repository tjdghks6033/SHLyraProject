// Copyright SH. All Rights Reserved.

#include "Projectile/SHMagicProjectileActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayCueNotify_Static.h"
#include "Kismet/GameplayStatics.h"

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

		// 데미지 적용
		FGameplayEffectSpecHandle DamageSpec =
			InstigatorASC->MakeOutgoingSpec(DamageEffect, AbilityLevel, Context);
		if (DamageSpec.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
		}

		// 추가 효과 적용 (빙결 스택, 점화 등)
		for (const TSubclassOf<UGameplayEffect>& EffectClass : OnHitEffects)
		{
			if (!EffectClass)
			{
				continue;
			}
			FGameplayEffectSpecHandle EffectSpec =
				InstigatorASC->MakeOutgoingSpec(EffectClass, AbilityLevel, Context);
			if (EffectSpec.IsValid())
			{
				InstigatorASC->ApplyGameplayEffectSpecToTarget(*EffectSpec.Data.Get(), TargetASC);
			}
		}

		// 넉백 — 발사체 위치→피격자 방향으로 발사
		if (KnockbackStrength > 0.f)
		{
			if (ACharacter* HitChar = Cast<ACharacter>(OtherActor))
			{
				const FVector Dir = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
				HitChar->LaunchCharacter(Dir * KnockbackStrength + FVector(0.f, 0.f, KnockbackZStrength), true, true);
			}
		}

		// HitStop — FTimerManager는 실시간 기준이라 TimeDilation 보정 불필요
		if (HitStopTimeDilation > 0.f)
		{
			UGameplayStatics::SetGlobalTimeDilation(this, HitStopTimeDilation);

			FTimerHandle HitStopTimer;
			GetWorldTimerManager().SetTimer(HitStopTimer,
				FTimerDelegate::CreateWeakLambda(this,
					[this]()
					{
						UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
					}),
				HitStopDuration, false);
		}

		DamagedActors.Add(OtherActor);
	}

	if (bDestroyOnHit)
	{
		// 폰(ASC 보유)에 충돌: 대상 ASC에서 큐를 실행해 대상 위치에 VFX를 붙인다.
		// 대상 ASC가 없으면 인스티게이터 ASC에서 실행.
		ExecuteImpactCue(SweepResult.ImpactPoint, SweepResult.ImpactNormal, TargetASC);
		DestroyProjectile();
	}
}

void ASHMagicProjectileActor::OnProjectileStopped(const FHitResult& ImpactResult)
{
	// 벽/지형 충돌 — 데미지 없이 임팩트 GameplayCue만 재생
	if (bDestroyOnHit)
	{
		ExecuteImpactCue(ImpactResult.ImpactPoint, ImpactResult.ImpactNormal, nullptr);
		DestroyProjectile();
	}
}

void ASHMagicProjectileActor::ExecuteImpactCue(const FVector& Location, const FVector& Normal,
	UAbilitySystemComponent* TargetASC)
{
	if (!ImpactGameplayCueTag.IsValid() || !InstigatorASC.IsValid())
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.Location = Location;
	CueParams.Normal = Normal;

	// 폰(ASC 보유)에 맞으면 대상 ASC에서 실행 — GCN에서 MyTarget으로 대상 캐릭터에 접근 가능.
	// 벽 등 ASC 없는 대상에 맞으면 인스티게이터 ASC에서 실행 — GCN은 CueParams.Location 사용.
	UAbilitySystemComponent* ExecutingASC = (TargetASC != nullptr) ? TargetASC : InstigatorASC.Get();
	ExecutingASC->ExecuteGameplayCue(ImpactGameplayCueTag, CueParams);
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
