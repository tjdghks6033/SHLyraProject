// Copyright SH. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SHVFXFunctionLibrary.generated.h"

class AActor;

/** VFX helpers used by SH gameplay cues. */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHVFXFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Spawns fire sampled across a character's animated skeletal-mesh surface. */
	UFUNCTION(BlueprintCallable, Category="SH|VFX")
	static void SpawnIgniteBodyFX(AActor* TargetActor);
};
