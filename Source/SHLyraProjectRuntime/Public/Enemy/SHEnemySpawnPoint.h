// Copyright SH. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "SHEnemySpawnPoint.generated.h"

/**
 * ASHEnemySpawnPoint
 *
 * 맵에 배치하는 적 스폰 위치 마커 액터.
 * USHEnemySpawnerComponent가 월드의 모든 ASHEnemySpawnPoint를 찾아
 * 해당 위치에 적을 스폰한다.
 *
 * 사용법:
 *   에디터에서 맵에 BP_SHEnemySpawnPoint를 원하는 위치에 배치하면 된다.
 *   실제 스폰은 USHEnemySpawnerComponent가 Experience 로딩 완료 시 처리한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

public:

	ASHEnemySpawnPoint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
