// Copyright SH. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"

#include "SHEnemySpawnerComponent.generated.h"

class ASHEnemyCharacter;
class ASHEnemySpawnPoint;
class ULyraExperienceDefinition;

/**
 * USHEnemySpawnerComponent
 *
 * GameState에 부착되어 Experience 로드 완료 시 적을 스폰하는 컴포넌트.
 *
 * 스폰 흐름:
 *   1. Experience 로드 완료 콜백 수신
 *   2. 월드의 모든 ASHEnemySpawnPoint 순회
 *   3. 각 위치에 EnemyClass 스폰
 *      → 스폰된 ASHEnemyCharacter의 BeginPlay에서 ASC가 자체 초기화됨
 *
 * DA_SHMeleeExperience Actions:
 *   GameFeatureAction_AddComponents → LyraGameState → BP_SHEnemySpawnerComponent
 */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class SHLYRAPROJECTRUNTIME_API USHEnemySpawnerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:

	USHEnemySpawnerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void BeginPlay() override;

private:

	void OnExperienceLoaded(const ULyraExperienceDefinition* Experience);
	void SpawnEnemyAtPoint(ASHEnemySpawnPoint* SpawnPoint);

protected:

	// 스폰할 적 클래스 (BP_SHEnemy)
	UPROPERTY(EditDefaultsOnly, Category = "SH|Enemy")
	TSubclassOf<ASHEnemyCharacter> EnemyClass;
};
