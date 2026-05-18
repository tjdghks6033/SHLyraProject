// Copyright SH. All Rights Reserved.

#pragma once

#include "AI/SHEnemyControllerBase.h"

#include "SHBossController.generated.h"

class UBehaviorTree;
class ULyraHealthComponent;  // Character/LyraHealthComponent.h

/**
 * ASHBossController
 *
 * SHLyraProject 보스 AI 컨트롤러.
 * BehaviorTree + Blackboard 기반으로 동작하며, Perception으로 감지한
 * CurrentTarget을 BB_TargetActor 키에 동기화해 BT가 읽을 수 있게 한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHBossController : public ASHEnemyControllerBase
{
	GENERATED_BODY()

public:

	ASHBossController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void InitPlayerState() override;
	virtual void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus) override;

	// 에디터에서 BB_SHBoss + BT_SHBoss 에셋을 연결한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

private:

	// LyraHealthComponent.OnHealthChanged 콜백. BB_HPPercent 갱신에 사용.
	UFUNCTION()
	void OnBossHealthChanged(ULyraHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* InstigatorActor);
};
