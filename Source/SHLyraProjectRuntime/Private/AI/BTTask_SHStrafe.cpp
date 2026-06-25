// Copyright SH. All Rights Reserved.

#include "AI/BTTask_SHStrafe.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_SHStrafe::UBTTask_SHStrafe()
{
	NodeName = TEXT("SH Strafe");

	// 이동 완료 콜백을 멤버 변수에 저장하므로 노드 인스턴싱이 필요하다.
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_SHStrafe::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	const APawn* OwnerPawn = AIController->GetPawn();
	if (!OwnerPawn)
	{
		return EBTNodeResult::Failed;
	}

	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return EBTNodeResult::Failed;
	}

	const AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKey));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent();
	if (!PathFollowing)
	{
		return EBTNodeResult::Failed;
	}

	// 플레이어 기준 현재 보스 방향 각도를 구해 좌/우 랜덤으로 StrafeArcDegrees만큼 회전한 위치를 목표로 삼는다.
	const FVector PlayerPos     = TargetActor->GetActorLocation();
	const FVector BossPos       = OwnerPawn->GetActorLocation();
	const FVector DirFromPlayer = (BossPos - PlayerPos).GetSafeNormal2D();

	const float CurrentAngle = FMath::Atan2(DirFromPlayer.Y, DirFromPlayer.X);
	const float StrafeSign   = FMath::RandBool() ? 1.f : -1.f;
	const float NewAngle     = CurrentAngle + FMath::DegreesToRadians(StrafeArcDegrees * StrafeSign);

	FVector StrafeTarget = PlayerPos + FVector(FMath::Cos(NewAngle), FMath::Sin(NewAngle), 0.f) * StrafeRadius;
	StrafeTarget.Z = BossPos.Z;  // NavMesh가 Z를 보정하므로 현재 Z를 초깃값으로 사용

	FAIMoveRequest MoveRequest(StrafeTarget);
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);

	const FPathFollowingRequestResult Result = AIController->MoveTo(MoveRequest);

	if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}

	if (Result.Code != EPathFollowingRequestResult::RequestSuccessful)
	{
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp     = &OwnerComp;
	CachedPathFollowing = PathFollowing;
	MoveRequestID       = Result.MoveId;

	MoveCompletedHandle = PathFollowing->OnRequestFinished.AddUObject(
		this, &UBTTask_SHStrafe::OnMoveCompleted);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_SHStrafe::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->StopMovement();
	}
	Cleanup();
	return EBTNodeResult::Aborted;
}

void UBTTask_SHStrafe::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Cleanup();
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_SHStrafe::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (RequestID != MoveRequestID)
	{
		return;
	}

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	Cleanup();

	if (OwnerComp)
	{
		FinishLatentTask(*OwnerComp, Result.IsSuccess() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
}

void UBTTask_SHStrafe::Cleanup()
{
	if (CachedPathFollowing.IsValid() && MoveCompletedHandle.IsValid())
	{
		CachedPathFollowing->OnRequestFinished.Remove(MoveCompletedHandle);
	}

	MoveCompletedHandle.Reset();
	MoveRequestID       = FAIRequestID::InvalidRequest;
	CachedPathFollowing.Reset();
	CachedOwnerComp.Reset();
}
