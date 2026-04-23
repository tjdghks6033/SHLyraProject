// Copyright SH. All Rights Reserved.

#include "Teams/SHTeamColorComponent.h"

#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "Teams/LyraTeamDisplayAsset.h"
#include "Teams/LyraTeamSubsystem.h"

USHTeamColorComponent::USHTeamColorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USHTeamColorComponent::BeginPlay()
{
	Super::BeginPlay();

	// 오너가 ILyraTeamAgentInterface를 구현하고 있어야 팀 변경을 수신할 수 있다.
	// ALyraCharacter는 이 인터페이스를 구현하므로, AddComponents로 주입된 액터는 기본 충족.
	if (ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(GetOwner()))
	{
		if (FOnLyraTeamIndexChangedDelegate* TeamDelegate = TeamAgent->GetOnTeamIndexChangedDelegate())
		{
			TeamDelegate->AddDynamic(this, &ThisClass::OnTeamChanged);
		}
	}

	// 이미 팀이 배정된 상태로 BeginPlay에 도달했을 수 있으므로 즉시 한 번 적용 시도.
	ApplyTeamColorsToOwner();
}

void USHTeamColorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(GetOwner()))
	{
		if (FOnLyraTeamIndexChangedDelegate* TeamDelegate = TeamAgent->GetOnTeamIndexChangedDelegate())
		{
			TeamDelegate->RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void USHTeamColorComponent::OnTeamChanged(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ApplyTeamColorsToOwner();
}

void USHTeamColorComponent::ApplyTeamColorsToOwner()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ULyraTeamSubsystem* TeamSubsystem = World->GetSubsystem<ULyraTeamSubsystem>();
	if (!TeamSubsystem)
	{
		return;
	}

	const ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(Owner);
	if (!TeamAgent)
	{
		return;
	}

	const int32 MyTeamId = GenericTeamIdToInteger(TeamAgent->GetGenericTeamId());
	if (MyTeamId == INDEX_NONE)
	{
		// 아직 팀 배정 전. OnTeamChanged 델리게이트가 나중에 다시 적용한다.
		return;
	}

	// Viewer 팀은 본인과 동일하게 지정(필터가 필요 없는 정보성 조회).
	ULyraTeamDisplayAsset* DisplayAsset = TeamSubsystem->GetTeamDisplayAsset(MyTeamId, MyTeamId);
	if (!DisplayAsset)
	{
		return;
	}

	// ULyraTeamDisplayAsset::ApplyToActor는 UE_API가 없어 외부 모듈에서 링크할 수 없다.
	// Lyra 내부 구현을 재현해 동일하게 MID에 파라미터를 주입한다.
	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents, /*bIncludeFromChildActors=*/ true);
	for (UMeshComponent* MeshComp : MeshComponents)
	{
		if (!MeshComp)
		{
			continue;
		}

		const int32 NumMaterials = MeshComp->GetNumMaterials();
		for (int32 MatIndex = 0; MatIndex < NumMaterials; ++MatIndex)
		{
			UMaterialInstanceDynamic* MID = MeshComp->CreateAndSetMaterialInstanceDynamic(MatIndex);
			if (!MID)
			{
				continue;
			}

			for (const TPair<FName, float>& Pair : DisplayAsset->ScalarParameters)
			{
				MID->SetScalarParameterValue(Pair.Key, Pair.Value);
			}
			for (const TPair<FName, FLinearColor>& Pair : DisplayAsset->ColorParameters)
			{
				MID->SetVectorParameterValue(Pair.Key, Pair.Value);
			}
			for (const TPair<FName, TObjectPtr<UTexture>>& Pair : DisplayAsset->TextureParameters)
			{
				MID->SetTextureParameterValue(Pair.Key, Pair.Value);
			}
		}
	}
}
