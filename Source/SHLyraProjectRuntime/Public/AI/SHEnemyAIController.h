// Copyright SH. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"

#include "SHEnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/**
 * ASHEnemyAIController
 *
 * BehaviorTree 없이 순수 C++로 구현한 단순 추격(Chase) AI 컨트롤러.
 *
 * 동작 흐름:
 *   1. UAIPerceptionComponent + UAISenseConfig_Sight로 Pawn 감지
 *   2. OnTargetPerceptionUpdated:
 *      - 성공적으로 감지(bSensed=true)  → CurrentTarget 저장
 *      - 시야에서 벗어남(bSensed=false) → CurrentTarget 초기화, 이동 정지
 *   3. Tick:
 *      - CurrentTarget 유효 + MeleeRange 초과 → MoveToActor
 *      - MeleeRange 이내 도달              → 이동 정지
 *        (추후: ASHMeleeAttack 어빌리티 발동으로 확장 가능)
 *
 * ASHEnemyCharacter가 AIControllerClass로 지정한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:

	ASHEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

private:

	// UAIPerceptionComponent가 타겟 인식 상태 변화를 알릴 때 호출된다.
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// 현재 제어 중인 폰과 타겟 사이의 거리를 반환한다.
	// 타겟이 없으면 MAX_FLT를 반환한다.
	float GetDistanceToTarget() const;

	// 감지 컴포넌트. 생성자에서 SightConfig를 구성한다.
	UPROPERTY(VisibleAnywhere, Category = "SH|AI")
	TObjectPtr<UAIPerceptionComponent> SHPerceptionComponent;

	// 현재 추격 대상. nullptr이면 추격하지 않는다.
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	// 이동을 멈추는 근접 거리 임계값 (cm). 이 거리 이내면 MoveToActor를 호출하지 않는다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|AI", meta = (ClampMin = "50.0"))
	float MeleeRange = 200.0f;
};
