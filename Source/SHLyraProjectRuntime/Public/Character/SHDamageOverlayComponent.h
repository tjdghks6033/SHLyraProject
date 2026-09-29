// Copyright SH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"
#include "SHDamageOverlayComponent.generated.h"

class ULyraCameraComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class ULyraHealthComponent;

/** Applies a local camera overlay while the player's health is critically low. */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SHLYRAPROJECTRUNTIME_API USHDamageOverlayComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	USHDamageOverlayComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Post Process domain material that draws the red edge vignette.
	UPROPERTY(EditDefaultsOnly, Category="Damage Overlay")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY(EditDefaultsOnly, Category="Damage Overlay", meta=(ClampMin="0.0", ClampMax="1.0"))
	float HealthThreshold = 0.35f;

private:
	void OnAbilitySystemInitialized();
	void RefreshOverlay(ULyraHealthComponent* HealthComponent);

	UFUNCTION()
	void OnHealthChanged(ULyraHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* InstigatorActor);

	UFUNCTION()
	void OnMaxHealthChanged(ULyraHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* InstigatorActor);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OverlayInstance;

	UPROPERTY(Transient)
	TObjectPtr<ULyraCameraComponent> CameraComponent;
};
