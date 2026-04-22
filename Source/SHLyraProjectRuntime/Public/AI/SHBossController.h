// Copyright SH. All Rights Reserved.

#pragma once

#include "AI/SHEnemyControllerBase.h"

#include "SHBossController.generated.h"

/**
 * ASHBossController
 *
 * SHLyraProject 보스 AI 컨트롤러. 향후 BehaviorTree / BlackboardData 기반으로
 * 페이즈 전환·특수 공격 로직을 구현할 자리.
 *
 * 현재는 베이스(ASHEnemyControllerBase)의 Perception + CurrentTarget 기능만 사용하는
 * 뼈대 클래스다. 보스 Phase 작업 시 RunBehaviorTree 및 Blackboard 키 동기화를 추가한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHBossController : public ASHEnemyControllerBase
{
	GENERATED_BODY()

public:

	ASHBossController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
