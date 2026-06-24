// Copyright SH. All Rights Reserved.

#include "Enemy/SHBossCreationComponent.h"
#include "AI/SHBossController.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Enemy/SHEnemyBoss.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Player/LyraPlayerState.h"

ASHBossSpawner::ASHBossSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASHBossSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!ensure(GameState))
	{
		return;
	}

	ULyraExperienceManagerComponent* ExperienceComponent =
		GameState->FindComponentByClass<ULyraExperienceManagerComponent>();

	if (ensure(ExperienceComponent))
	{
		ExperienceComponent->CallOrRegister_OnExperienceLoaded(
			FOnLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
}

void ASHBossSpawner::OnExperienceLoaded(const ULyraExperienceDefinition* /*Experience*/)
{
	PlayIntroOrEngage();
}

void ASHBossSpawner::ServerSpawnBoss()
{
	if (!BossPawnClass || !BossControllerClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters PawnSpawnParams;
	PawnSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASHEnemyBoss* BossPawn = World->SpawnActor<ASHEnemyBoss>(
		BossPawnClass, GetActorTransform(), PawnSpawnParams);
	if (!BossPawn)
	{
		return;
	}

	// LyraGameMode::SpawnDefaultPawnAtTransform이 하는 PawnData 주입을 수동으로 대체.
	// Possess 전에 설정해야 init chain(Spawned→DataAvailable)이 Controller 연결 시 즉시 진행된다.
	if (BossPawnData)
	{
		if (ULyraPawnExtensionComponent* PawnExt = BossPawn->FindComponentByClass<ULyraPawnExtensionComponent>())
		{
			PawnExt->SetPawnData(BossPawnData);
		}
	}

	FActorSpawnParameters ControllerSpawnParams;
	ControllerSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASHBossController* BossController = World->SpawnActor<ASHBossController>(
		BossControllerClass, GetActorTransform(), ControllerSpawnParams);
	if (!BossController)
	{
		BossPawn->Destroy();
		return;
	}

	BossController->Possess(BossPawn);

	// ULyraHeroComponent가 보스 폰에 없으면 InitializeAbilitySystem이 자동으로 호출되지 않는다.
	// PlayerState의 ASC로 직접 초기화해 OnAbilitySystemInitialized 체인(InitGameplayEffects 적용 포함)을 완성한다.
	if (ULyraPawnExtensionComponent* PawnExt = BossPawn->FindComponentByClass<ULyraPawnExtensionComponent>())
	{
		if (!PawnExt->GetLyraAbilitySystemComponent())
		{
			if (ALyraPlayerState* LyraPS = BossController->GetPlayerState<ALyraPlayerState>())
			{
				PawnExt->InitializeAbilitySystem(LyraPS->GetLyraAbilitySystemComponent(), LyraPS);
			}
		}
	}

	SpawnedBoss = BossPawn;
	BroadcastBossEngaged();
}

void ASHBossSpawner::PlayIntroOrEngage()
{
	if (!IntroSequence)
	{
		ServerSpawnBoss();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		ServerSpawnBoss();
		return;
	}

	// 컷씬 재생 중 HUD 숨김 + 이동·시점 입력 차단.
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		PC->SetCinematicMode(true, false, true, true, true);
	}

	ALevelSequenceActor* SequenceActor = nullptr;
	FMovieSceneSequencePlaybackSettings Settings;
	IntroSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		World, IntroSequence, Settings, SequenceActor);

	if (!IntroSequencePlayer)
	{
		ServerSpawnBoss();
		return;
	}

	IntroSequencePlayer->OnFinished.AddDynamic(this, &ThisClass::OnIntroSequenceFinished);
	IntroSequencePlayer->Play();
}

void ASHBossSpawner::OnIntroSequenceFinished()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->SetCinematicMode(false, false, true, true, true);

			// Camera Cut 트랙이 뷰 타겟을 CineCameraActor로 교체한 채 끝나므로
			// 플레이어 폰으로 명시적으로 복원한다.
			if (APawn* Pawn = PC->GetPawn())
			{
				PC->SetViewTarget(Pawn);
			}
		}
	}

	ServerSpawnBoss();
}

void ASHBossSpawner::BroadcastBossEngaged()
{
	if (!SpawnedBoss)
	{
		return;
	}

	UGameplayMessageSubsystem& MsgSys = UGameplayMessageSubsystem::Get(this);
	FSHBossMessage Payload;
	Payload.BossActor = SpawnedBoss;
	MsgSys.BroadcastMessage(TAG_SH_Message_Boss_Engaged, Payload);
}
