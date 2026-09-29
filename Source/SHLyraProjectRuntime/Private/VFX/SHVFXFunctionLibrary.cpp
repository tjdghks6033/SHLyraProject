// Copyright SH. All Rights Reserved.

#include "VFX/SHVFXFunctionLibrary.h"

#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHVFXFunctionLibrary)

void USHVFXFunctionLibrary::SpawnIgniteBodyFX(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	USkeletalMeshComponent* Mesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!IsValid(Mesh))
	{
		return;
	}

	static TWeakObjectPtr<UNiagaraSystem> CachedFireFX;
	if (!CachedFireFX.IsValid())
	{
		CachedFireFX = LoadObject<UNiagaraSystem>(
			nullptr,
			TEXT("/SHLyraProject/Game/Magic/VFX/NS_SH_IgniteBody.NS_SH_IgniteBody"));
	}

	UNiagaraSystem* FireFX = CachedFireFX.Get();
	if (!IsValid(FireFX))
	{
		return;
	}

	// The Niagara emitter samples the animated skeletal-mesh surface, so one
	// component wraps the whole silhouette without visible bone/socket clusters.
	UNiagaraFunctionLibrary::SpawnSystemAttached(
		FireFX,
		Mesh,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		FVector::OneVector,
		EAttachLocation::KeepRelativeOffset,
		true,
		ENCPoolMethod::AutoRelease,
		true,
		true);

	// This Lyra surface emitter resolves its skeletal source from the attach
	// parent, so attaching directly to Mesh supplies the animated geometry.
}
