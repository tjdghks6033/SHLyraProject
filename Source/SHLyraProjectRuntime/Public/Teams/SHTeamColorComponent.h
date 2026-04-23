// Copyright SH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"

#include "SHTeamColorComponent.generated.h"

class ULyraTeamDisplayAsset;

/**
 * USHTeamColorComponent
 *
 * 오너 액터의 팀 ID 변경을 감지해 ULyraTeamDisplayAsset의 파라미터를
 * 오너의 모든 MeshComponent 머티리얼(MID)에 자동 주입한다.
 *
 * GameFeatureAction_AddComponents를 통해 ALyraCharacter에 주입되므로
 * 플레이어(BP_SHCharacter)와 봇(BP_SHCharacter로 스폰) 모두 동일 로직으로 팀 색상이 적용된다.
 *
 * 오너는 ILyraTeamAgentInterface를 구현해야 한다(ALyraCharacter가 이미 구현).
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SHLYRAPROJECTRUNTIME_API USHTeamColorComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:

	USHTeamColorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 오너의 팀 ID가 변경될 때 호출된다. OnRep_MyTeamID를 포함해 서버/클라 양쪽에서 수신.
	UFUNCTION()
	void OnTeamChanged(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	// 현재 팀의 DisplayAsset을 조회해 오너의 모든 MeshComponent 머티리얼에 파라미터를 주입한다.
	void ApplyTeamColorsToOwner();
};
