// Copyright SH. All Rights Reserved.

#include "AI/SHBossController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/LyraHealthComponent.h"
#include "GameFramework/PlayerState.h"
#include "Teams/LyraTeamSubsystem.h"
#include "Teams/SHLyraProjectTeamIds.h"

static const FName BB_TargetActor = TEXT("TargetActor");
static const FName BB_HPPercent   = TEXT("HPPercent");

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

	// BB_HPPercent를 1.0으로 초기화 → Phase 1에서 게임이 시작되도록 보장.
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsFloat(BB_HPPercent, 1.0f);
	}

	// 보스 HP 변화를 감지해 BB_HPPercent를 갱신한다.
	if (ULyraHealthComponent* HealthComp = InPawn->FindComponentByClass<ULyraHealthComponent>())
	{
		HealthComp->OnHealthChanged.AddDynamic(this, &ASHBossController::OnBossHealthChanged);
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

void ASHBossController::OnBossHealthChanged(ULyraHealthComponent* HealthComponent,
	float OldValue, float NewValue, AActor* InstigatorActor)
{
	const float MaxHealth = HealthComponent->GetMaxHealth();
	const float Percent = (MaxHealth > 0.f) ? (NewValue / MaxHealth) : 1.f;

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsFloat(BB_HPPercent, Percent);
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
