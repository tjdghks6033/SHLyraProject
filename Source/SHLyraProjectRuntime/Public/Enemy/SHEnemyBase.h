// Copyright SH. All Rights Reserved.

#pragma once

#include "Character/LyraCharacter.h"

#include "SHEnemyBase.generated.h"

class ULyraTeamDisplayAsset;

/**
 * ASHEnemyBase
 *
 * SHLyraProject의 모든 적 캐릭터 공통 베이스.
 * ALyraCharacter를 상속해 Lyra의 PawnExtensionComponent 초기화 체인,
 * 팀 시스템, HealthComponent, GAS 초기화 흐름을 모두 활용한다.
 *
 * 공통 책임:
 *   1. 팀 색상 자동 적용 — 팀 ID 변경 시 ULyraTeamDisplayAsset을 액터 머티리얼에 반영
 *   2. 공통 사망 처리 — ALyraCharacter::OnDeathFinished 오버라이드로 추가 후처리
 *
 * 봇(ASHEnemyBot)과 보스(ASHEnemyBoss)는 이 클래스를 상속해 고유 로직만 추가한다.
 */
UCLASS(Abstract)
class SHLYRAPROJECTRUNTIME_API ASHEnemyBase : public ALyraCharacter
{
	GENERATED_BODY()

public:

	ASHEnemyBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface

	//~ALyraCharacter interface
	virtual void OnAbilitySystemInitialized() override;
	virtual void OnDeathFinished(AActor* OwningActor) override;
	//~End of ALyraCharacter interface

	// 팀 변경 시 호출 — 현재 팀의 DisplayAsset을 조회해 머티리얼 파라미터를 적용한다.
	UFUNCTION()
	void OnTeamChanged(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	// 팀 시스템에서 DisplayAsset을 가져와 자신의 모든 머티리얼에 적용한다.
	void ApplyTeamColorsFromCurrentTeam();

private:

	// 사망 후 액터를 파괴하기까지 대기 시간 (초).
	// LyraHealthComponent의 OnDeathFinished가 호출된 후 이만큼 지연시켜 시체 시뮬레이션 시간 확보.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Enemy", meta = (ClampMin = "0.0"))
	float DestroyDelay = 3.0f;

	FTimerHandle DestroyTimerHandle;

	void HandleDestroyAfterDelay();
};
