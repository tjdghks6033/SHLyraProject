// Copyright SH. All Rights Reserved.

#include "Enemy/SHEnemyBase.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "Teams/LyraTeamSubsystem.h"
#include "Teams/LyraTeamDisplayAsset.h"
#include "TimerManager.h"

ASHEnemyBase::ASHEnemyBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// ALyraCharacter가 PawnExtensionComponent / HealthComponent / CameraComponent를 이미 서브오브젝트로 생성한다.
	// 적 공통 설정이 필요하면 여기에 추가.
}

void ASHEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// 팀 변경 델리게이트 구독. ALyraCharacter가 이미 ILyraTeamAgentInterface를 구현했으므로
	// 이 델리게이트는 서버/클라이언트 양쪽에서 팀 ID 변경(OnRep_MyTeamID 포함)을 수신한다.
	if (FOnLyraTeamIndexChangedDelegate* TeamDelegate = GetOnTeamIndexChangedDelegate())
	{
		TeamDelegate->AddDynamic(this, &ASHEnemyBase::OnTeamChanged);
	}

	// 이미 팀이 배정된 상태로 BeginPlay에 도달했을 수 있으므로 즉시 한 번 적용 시도.
	ApplyTeamColorsFromCurrentTeam();
}

void ASHEnemyBase::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();

	// ASC 초기화 완료 — 자식 클래스가 Attribute 초기값 세팅이나 어빌리티 바인딩 훅으로 활용 가능.
}

void ASHEnemyBase::OnDeathFinished(AActor* OwningActor)
{
	// ALyraCharacter 기본 구현은 SetTimerForNextTick → DestroyDueToDeath로 즉시 파괴한다.
	// 적은 시체 연출(애니메이션, VFX) 시간을 확보하기 위해 DestroyDelay만큼 지연한다.
	// 이동/충돌 비활성화는 OnDeathStarted에서 부모가 이미 처리했으므로 여기서 중복하지 않는다.

	GetWorldTimerManager().SetTimer(
		DestroyTimerHandle,
		this,
		&ASHEnemyBase::HandleDestroyAfterDelay,
		DestroyDelay,
		false);
}

void ASHEnemyBase::HandleDestroyAfterDelay()
{
	// Lyra 정식 파괴 경로 — K2_OnDeathFinished(BP 이벤트) 발화 + UninitAndDestroy(ASC uninit + Destroy) 실행.
	DestroyDueToDeath();
}

void ASHEnemyBase::OnTeamChanged(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ApplyTeamColorsFromCurrentTeam();
}

void ASHEnemyBase::ApplyTeamColorsFromCurrentTeam()
{
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

	const int32 MyTeamId = GenericTeamIdToInteger(GetGenericTeamId());
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
	// Lyra 내부 구현(매터리얼 파라미터를 MID에 주입)을 재현해 동일 효과를 낸다.
	TArray<UMeshComponent*> MeshComponents;
	GetComponents<UMeshComponent>(MeshComponents, /*bIncludeFromChildActors=*/ true);
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
