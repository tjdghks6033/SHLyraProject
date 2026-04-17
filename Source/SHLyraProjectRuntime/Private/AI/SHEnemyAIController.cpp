// Copyright SH. All Rights Reserved.

#include "AI/SHEnemyAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/Pawn.h"

ASHEnemyAIController::ASHEnemyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// -------------------------------------------------------
	// Sight 감지 설정
	// -------------------------------------------------------
	SHPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("SHPerceptionComponent"));

	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius                          = 1500.0f;
	SightConfig->LoseSightRadius                      = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees         = 60.0f;
	SightConfig->SetMaxAge(5.0f);

	// Enemy/Neutral(플레이어) 모두 감지. Friendly(동료 적)는 제외.
	SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals   = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	SHPerceptionComponent->ConfigureSense(*SightConfig);
	SHPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void ASHEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	// 생성자가 아닌 BeginPlay에서 바인딩: 컴포넌트가 완전히 초기화된 이후에 실행된다.
	SHPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this, &ASHEnemyAIController::OnTargetPerceptionUpdated);
}

void ASHEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 빙의된 폰을 향한 기본 포커스 초기화
	ClearFocus(EAIFocusPriority::Gameplay);
}

void ASHEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CurrentTarget)
	{
		return;
	}

	const float DistToTarget = GetDistanceToTarget();

	if (DistToTarget <= MeleeRange)
	{
		// 근접 사거리 도달: 이동 중지 + 타겟 응시
		// TODO: 추후 이 시점에 근접 공격 어빌리티(ASHMeleeAttack) 발동 가능
		StopMovement();
		SetFocus(CurrentTarget);
	}
	else
	{
		// 타겟을 향해 이동
		MoveToActor(CurrentTarget, MeleeRange);
		SetFocus(CurrentTarget);
	}
}

void ASHEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 자기 자신은 무시
	if (Actor == GetPawn())
	{
		return;
	}

	// 폰이 아닌 액터는 무시 (배경 오브젝트 등)
	if (!Actor->IsA<APawn>())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// 새로운 타겟 감지: 더 가까운 대상이 있으면 교체
		if (!CurrentTarget)
		{
			CurrentTarget = Actor;
		}
		else
		{
			// 현재 타겟보다 새로 감지된 액터가 더 가까우면 교체
			const float CurrentDist  = GetDistanceToTarget();
			const float NewActorDist = FVector::Dist(
				GetPawn()->GetActorLocation(), Actor->GetActorLocation());

			if (NewActorDist < CurrentDist)
			{
				CurrentTarget = Actor;
			}
		}
	}
	else
	{
		// 감지 대상이 시야에서 벗어남
		if (CurrentTarget == Actor)
		{
			CurrentTarget = nullptr;
			StopMovement();
			ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}

float ASHEnemyAIController::GetDistanceToTarget() const
{
	if (!CurrentTarget || !GetPawn())
	{
		return MAX_FLT;
	}

	return FVector::Dist(GetPawn()->GetActorLocation(), CurrentTarget->GetActorLocation());
}
