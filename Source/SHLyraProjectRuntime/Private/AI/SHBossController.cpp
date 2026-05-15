// Copyright SH. All Rights Reserved.

#include "AI/SHBossController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/PlayerState.h"
#include "Teams/LyraTeamSubsystem.h"
#include "Teams/SHLyraProjectTeamIds.h"

static const FName BB_TargetActor = TEXT("TargetActor");

ASHBossController::ASHBossController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASHBossController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void ASHBossController::InitPlayerState()
{
	Super::InitPlayerState();

	if (!HasAuthority())
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		if (ULyraTeamSubsystem* TeamSubsystem = World->GetSubsystem<ULyraTeamSubsystem>())
		{
			TeamSubsystem->ChangeTeamForActor(PlayerState.Get(), SHLyraProject::TeamIds::SHEnemy);
		}
	}
}

void ASHBossController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Super가 CurrentTarget을 갱신한 뒤 BB에 동기화한다.
	Super::OnTargetPerceptionUpdated(Actor, Stimulus);

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(BB_TargetActor, CurrentTarget);
	}

	// Focus를 설정하면 AIController가 매 틱 ControlRotation을 타겟 방향으로 갱신한다.
	// SpawnProjectile이 GetControlRotation()으로 발사 방향을 결정하므로 이것이 필수다.
	if (CurrentTarget)
	{
		SetFocus(CurrentTarget);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}
}
