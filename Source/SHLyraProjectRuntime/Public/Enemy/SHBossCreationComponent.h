// Copyright SH. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "SHBossCreationComponent.generated.h"

class ASHEnemyBoss;
class ASHBossController;
class ULevelSequence;
class ULevelSequencePlayer;
class ULyraExperienceDefinition;
class ULyraPawnData;

/**
 * ASHBossSpawner
 *
 * 맵에 배치하는 보스 스포너 액터.
 * Experience 로드 완료 후 자신의 Transform 위치에 보스를 스폰하고,
 * IntroSequence가 지정된 경우 컷씬을 재생한 뒤 SH.Message.Boss.Engaged를 브로드캐스트한다.
 *
 * 사용법:
 *   1. BP_SHBossSpawner를 레벨에 배치하고 원하는 위치에 이동
 *   2. BossPawnClass / BossControllerClass / BossPawnData 설정
 *   3. (선택) IntroSequence에 LS_SHBossIntro 지정
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHBossSpawner : public AActor
{
	GENERATED_BODY()

public:

	ASHBossSpawner(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void BeginPlay() override;

private:

	void OnExperienceLoaded(const ULyraExperienceDefinition* Experience);
	void ServerSpawnBoss();

	// 보스 스폰 완료 후 컷씬 재생 또는 즉시 Engaged 브로드캐스트.
	void PlayIntroOrEngage();

	// 컷씬 종료 콜백. 입력 복원 후 SH.Message.Boss.Engaged 브로드캐스트.
	UFUNCTION()
	void OnIntroSequenceFinished();

	void BroadcastBossEngaged();

	// 스폰할 보스 폰 클래스. BP에서 BP_SHEnemyBoss 지정.
	UPROPERTY(EditAnywhere, Category = "SH|Boss")
	TSubclassOf<ASHEnemyBoss> BossPawnClass;

	// 보스 AI 컨트롤러 클래스. BP에서 BP_SHBossController 지정.
	UPROPERTY(EditAnywhere, Category = "SH|Boss")
	TSubclassOf<ASHBossController> BossControllerClass;

	// 보스 폰에 주입할 PawnData. LyraGameMode::SpawnDefaultPawnAtTransform의
	// PawnExtComp->SetPawnData 단계를 수동으로 대체한다. DA_SHEnemyPawnData 지정.
	UPROPERTY(EditAnywhere, Category = "SH|Boss")
	TObjectPtr<const ULyraPawnData> BossPawnData;

	// 보스 등장 컷씬. 비워두면 스폰 즉시 전투 시작.
	UPROPERTY(EditAnywhere, Category = "SH|Boss|Cutscene")
	TObjectPtr<ULevelSequence> IntroSequence;

	// 런타임 참조 — GC 방지용.
	UPROPERTY()
	TObjectPtr<ASHEnemyBoss> SpawnedBoss;

	UPROPERTY()
	TObjectPtr<ULevelSequencePlayer> IntroSequencePlayer;
};
