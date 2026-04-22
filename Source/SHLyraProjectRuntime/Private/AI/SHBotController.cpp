// Copyright SH. All Rights Reserved.

#include "AI/SHBotController.h"

#include "GameFramework/PlayerState.h"
#include "Teams/LyraTeamSubsystem.h"
#include "Teams/SHLyraProjectTeamIds.h"

ASHBotController::ASHBotController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASHBotController::InitPlayerState()
{
	// Super(=ASHEnemyControllerBase)가 PS 생성·팀 델리게이트 바인딩·BroadcastOnPlayerStateChanged를 수행한다.
	Super::InitPlayerState();

	// 봇 스폰은 서버에서만 일어나므로 AIController는 항상 Authority. 방어적으로 가드만 한 번.
	if (!HasAuthority())
	{
		return;
	}

	// 팀 할당.
	// TeamCreationComponent는 HighPriority, BotCreationComponent는 LowPriority로
	// OnExperienceLoaded를 구독하므로 이 시점에는 TeamSubsystem에 Team 10이 등록돼 있다.
	//
	// NOTE: 봇 전용 PawnData 주입은 하지 않는다. ALyraPlayerState::PostInitializeComponents에서
	// Experience를 구독해 `OnExperienceLoaded` 즉시 실행 → `SetPawnData(DefaultPawnData)`를 먼저
	// 수행하기 때문에 우리 오버라이드에서 Super::InitPlayerState 호출 후에는 PS의 PawnData가
	// 이미 확정돼 있어 덮어쓸 수 없다. 봇을 다른 폰으로 스폰하려면 ALyraGameMode/ALyraPlayerState를
	// 모두 서브클래싱하는 큰 작업이 필요하므로 Phase 7에서는 팀 색상(DisplayAsset)만으로 구분한다.
	if (const UWorld* World = GetWorld())
	{
		if (ULyraTeamSubsystem* TeamSubsystem = World->GetSubsystem<ULyraTeamSubsystem>())
		{
			TeamSubsystem->ChangeTeamForActor(PlayerState.Get(), SHLyraProject::TeamIds::SHEnemy);
		}
	}
}

void ASHBotController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CurrentTarget)
	{
		return;
	}

	const float DistToTarget = GetDistanceToTarget();

	if (DistToTarget <= MeleeRange)
	{
		// 근접 사거리 도달: 이동 중지 + 타겟 응시.
		// TODO: 근접 공격 어빌리티(ASHMeleeAttack) 발동 지점.
		StopMovement();
		SetFocus(CurrentTarget);
	}
	else
	{
		MoveToActor(CurrentTarget, MeleeRange);
		SetFocus(CurrentTarget);
	}
}
