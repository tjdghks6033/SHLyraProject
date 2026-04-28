// Copyright SH. All Rights Reserved.

#include "AI/SHBossController.h"
#include "GameFramework/PlayerState.h"
#include "Teams/LyraTeamSubsystem.h"
#include "Teams/SHLyraProjectTeamIds.h"

ASHBossController::ASHBossController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
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
