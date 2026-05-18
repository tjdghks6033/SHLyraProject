// Copyright SH. All Rights Reserved.

#include "AI/BTDecorator_SHCheckDistance.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UBTDecorator_SHCheckDistance::UBTDecorator_SHCheckDistance()
{
	NodeName = TEXT("SH Check Distance");

	// BB 키 선택기를 Actor 타입으로 제한한다.
	TargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTDecorator_SHCheckDistance, TargetActorKey),
		AActor::StaticClass());
}

void UBTDecorator_SHCheckDistance::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBData = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBData);
	}
}

bool UBTDecorator_SHCheckDistance::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return false;
	}

	const APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return false;
	}

	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return false;
	}

	const AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Target)
	{
		return false;
	}

	return FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) < MaxDistance;
}

FString UBTDecorator_SHCheckDistance::GetStaticDescription() const
{
	return FString::Printf(TEXT("Distance to %s < %.0f cm"),
		*TargetActorKey.SelectedKeyName.ToString(), MaxDistance);
}
