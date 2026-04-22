// Copyright SH. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Teams/LyraTeamAgentInterface.h"

#include "SHEnemyControllerBase.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/**
 * ASHEnemyControllerBase
 *
 * SHLyraProject의 적 AI 컨트롤러 공통 베이스.
 *
 * Lyra의 `ALyraPlayerBotController`를 상속하지 않는다: 해당 클래스는 UCLASS에 MinimalAPI가 없고
 * 멤버에 UE_API가 없어 외부 모듈에서 링크가 불가능하다. 우리는 `AAIController` + `ILyraTeamAgentInterface`
 * 를 직접 구현해 동일한 역할(팀 ID는 PlayerState가 보유하고, 컨트롤러는 PlayerState 변경을 구독해
 * 팀 변경 델리게이트를 브로드캐스트)을 수행한다.
 *
 * 공통 책임:
 *   - `AIPerceptionComponent` + `UAISenseConfig_Sight` 구성 (생성자)
 *   - `OnTargetPerceptionUpdated` 콜백으로 `CurrentTarget` 관리
 *   - `ILyraTeamAgentInterface` 구현 (팀 ID는 PlayerState 위임)
 *
 * 이동·공격 등 구체 행동은 파생 클래스(`ASHBotController` Tick 기반, `ASHBossController` BT 기반)가 구현한다.
 */
UCLASS(Abstract)
class SHLYRAPROJECTRUNTIME_API ASHEnemyControllerBase : public AAIController, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:

	ASHEnemyControllerBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ILyraTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~End of ILyraTeamAgentInterface

	virtual void OnUnPossess() override;

protected:

	virtual void BeginPlay() override;

	//~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

	// Perception 콜백. 파생 클래스에서 재정의해 반응을 커스터마이즈할 수 있다.
	UFUNCTION()
	virtual void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// 현재 제어 중인 폰과 CurrentTarget 사이 거리(cm). 타겟이 없으면 MAX_FLT.
	float GetDistanceToTarget() const;

protected:

	// 감지 컴포넌트. 생성자에서 SightConfig를 구성해 등록한다.
	UPROPERTY(VisibleAnywhere, Category = "SH|AI")
	TObjectPtr<UAIPerceptionComponent> SHPerceptionComponent;

	// 현재 추격 대상. 파생 클래스의 Tick/BT에서 사용.
	UPROPERTY(BlueprintReadOnly, Category = "SH|AI")
	TObjectPtr<AActor> CurrentTarget;

	UPROPERTY(EditDefaultsOnly, Category = "SH|AI", meta = (ClampMin = "100.0"))
	float SightRadius = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SH|AI", meta = (ClampMin = "100.0"))
	float LoseSightRadius = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SH|AI", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngle = 60.0f;

private:

	// PlayerState 재바인딩 공통 헬퍼. Lyra가 ALyraPlayerBotController에서 하는 동일 흐름.
	void BroadcastOnPlayerStateChanged();

	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;
};
