// Copyright SH. All Rights Reserved.

#pragma once

#include "Character/LyraCharacter.h"

#include "SHEnemyBase.generated.h"

/**
 * ASHEnemyBase
 *
 * SHLyraProject의 모든 적 캐릭터 공통 베이스.
 * ALyraCharacter를 상속해 Lyra의 PawnExtensionComponent 초기화 체인,
 * 팀 시스템, HealthComponent, GAS 초기화 흐름을 모두 활용한다.
 *
 * 공통 책임:
 *   1. 공통 사망 처리 — ALyraCharacter::OnDeathFinished 오버라이드로 시체 연출 시간을 확보
 *
 * 팀 색상 주입은 USHTeamColorComponent(ModularGameplay AddComponents로 주입)가 담당한다.
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

	//~ALyraCharacter interface
	virtual void OnAbilitySystemInitialized() override;
	virtual void OnDeathFinished(AActor* OwningActor) override;
	//~End of ALyraCharacter interface

private:

	// 사망 후 액터를 파괴하기까지 대기 시간 (초).
	// LyraHealthComponent의 OnDeathFinished가 호출된 후 이만큼 지연시켜 시체 시뮬레이션 시간 확보.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Enemy", meta = (ClampMin = "0.0"))
	float DestroyDelay = 3.0f;

	FTimerHandle DestroyTimerHandle;

	void HandleDestroyAfterDelay();
};
