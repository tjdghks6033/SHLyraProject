// Copyright SH. All Rights Reserved.

#include "Character/SHDamageOverlayComponent.h"

#include "Character/LyraHealthComponent.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Camera/LyraCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHDamageOverlayComponent)

USHDamageOverlayComponent::USHDamageOverlayComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false); // Camera effects belong to the owning player's local view only.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OverlayAsset(
		TEXT("/SHLyraProject/Game/UI/M_SH_DamageVignette.M_SH_DamageVignette"));
	if (OverlayAsset.Succeeded())
	{
		OverlayMaterial = OverlayAsset.Object;
	}
}

void USHDamageOverlayComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
	{
		PawnExtComp->OnAbilitySystemInitialized_RegisterAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	}
}

void USHDamageOverlayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(GetOwner()))
	{
		HealthComp->OnHealthChanged.RemoveDynamic(this, &ThisClass::OnHealthChanged);
		HealthComp->OnMaxHealthChanged.RemoveDynamic(this, &ThisClass::OnMaxHealthChanged);
	}

	if (CameraComponent && OverlayInstance)
	{
		CameraComponent->AddOrUpdateBlendable(OverlayInstance, 0.0f);
	}

	Super::EndPlay(EndPlayReason);
}

void USHDamageOverlayComponent::OnAbilitySystemInitialized()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC || !PC->IsLocalController() || !OverlayMaterial)
	{
		return;
	}

	ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(GetOwner());
	if (!HealthComp)
	{
		return;
	}

	CameraComponent = ULyraCameraComponent::FindCameraComponent(GetOwner());
	if (!CameraComponent)
	{
		return;
	}

	OverlayInstance = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
	HealthComp->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::OnHealthChanged);
	HealthComp->OnMaxHealthChanged.AddUniqueDynamic(this, &ThisClass::OnMaxHealthChanged);
	RefreshOverlay(HealthComp);
}

void USHDamageOverlayComponent::RefreshOverlay(ULyraHealthComponent* HealthComponent)
{
	if (CameraComponent && OverlayInstance && HealthComponent)
	{
		const float MaxHealth = HealthComponent->GetMaxHealth();
		const bool bLowHealth = MaxHealth > 0.0f && HealthComponent->GetHealth() > 0.0f
			&& HealthComponent->GetHealth() / MaxHealth <= HealthThreshold;
		CameraComponent->AddOrUpdateBlendable(OverlayInstance, bLowHealth ? 1.0f : 0.0f);
	}
}

void USHDamageOverlayComponent::OnHealthChanged(ULyraHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* InstigatorActor)
{
	RefreshOverlay(HealthComponent);
}

void USHDamageOverlayComponent::OnMaxHealthChanged(ULyraHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* InstigatorActor)
{
	RefreshOverlay(HealthComponent);
}
