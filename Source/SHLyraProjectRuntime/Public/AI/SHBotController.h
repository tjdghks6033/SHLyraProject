// Copyright SH. All Rights Reserved.

#pragma once

#include "AI/SHEnemyControllerBase.h"

#include "SHBotController.generated.h"

/**
 * ASHBotController
 *
 * SHLyraProject 졸개 AI 컨트롤러. BehaviorTree 없이 Tick 기반으로 타겟을 추격한다.
 *
 * 동작:
 *   1. 베이스(ASHEnemyControllerBase)가 Perception 감지 + CurrentTarget 관리를 수행
 *   2. Tick: CurrentTarget이 MeleeRange 밖이면 MoveToActor, 이내면 정지 + Focus
 *
 * 봇 초기화:
 *   - InitPlayerState 시점에 PlayerState를 Team 10(SH Enemy)으로 할당
 *   - ULyraBotCreationComponent는 외부 모듈에서 C++ 상속이 불가(MinimalAPI 없음)하므로
 *     봇 자체가 스스로 "나는 적"이라고 선언하는 형태로 우회한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHBotController : public ASHEnemyControllerBase
{
	GENERATED_BODY()

public:

	ASHBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void Tick(float DeltaTime) override;

	//~AController interface
	virtual void InitPlayerState() override;
	//~End of AController interface

	// 이동을 멈추는 근접 거리 임계값 (cm). 이 거리 이내면 MoveToActor를 호출하지 않는다.
	// 추후 근접 공격 어빌리티 발동 조건으로도 활용 가능.
	UPROPERTY(EditDefaultsOnly, Category = "SH|AI", meta = (ClampMin = "50.0"))
	float MeleeRange = 200.0f;
};
