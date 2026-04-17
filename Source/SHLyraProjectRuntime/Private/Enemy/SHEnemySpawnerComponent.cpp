// Copyright SH. All Rights Reserved.

#include "Enemy/SHEnemySpawnerComponent.h"
#include "Enemy/SHEnemyCharacter.h"
#include "Enemy/SHEnemySpawnPoint.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "EngineUtils.h"

USHEnemySpawnerComponent::USHEnemySpawnerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USHEnemySpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	AGameStateBase* GameState = Cast<AGameStateBase>(GetOwner());
	ULyraExperienceManagerComponent* ExperienceComp = GameState
		? GameState->FindComponentByClass<ULyraExperienceManagerComponent>()
		: nullptr;

	if (!ExperienceComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("USHEnemySpawnerComponent: LyraExperienceManagerComponent를 찾을 수 없습니다."));
		return;
	}

	ExperienceComp->CallOrRegister_OnExperienceLoaded(
		FOnLyraExperienceLoaded::FDelegate::CreateUObject(
			this, &USHEnemySpawnerComponent::OnExperienceLoaded));
}

void USHEnemySpawnerComponent::OnExperienceLoaded(const ULyraExperienceDefinition* Experience)
{
	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("USHEnemySpawnerComponent: EnemyClass가 설정되지 않았습니다."));
		return;
	}

	for (ASHEnemySpawnPoint* SpawnPoint : TActorRange<ASHEnemySpawnPoint>(GetWorld()))
	{
		SpawnEnemyAtPoint(SpawnPoint);
	}
}

void USHEnemySpawnerComponent::SpawnEnemyAtPoint(ASHEnemySpawnPoint* SpawnPoint)
{
	if (!SpawnPoint)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// ASC 초기화는 ASHEnemyCharacter::BeginPlay에서 자체적으로 처리된다.
	GetWorld()->SpawnActor<ASHEnemyCharacter>(
		EnemyClass, SpawnPoint->GetActorTransform(), SpawnParams);
}
