// Copyright SH. All Rights Reserved.

#include "AI/SHBotController.h"

ASHBotController::ASHBotController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
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
