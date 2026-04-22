// Copyright SH. All Rights Reserved.

#pragma once

#include "Enemy/SHEnemyBase.h"

#include "SHEnemyBoss.generated.h"

/**
 * ASHEnemyBoss
 *
 * SHLyraProject의 보스 캐릭터. 현재는 ASHEnemyBase와 동일하게 동작하는 뼈대 클래스이며,
 * 보스 전용 Phase(페이즈) 관리·특수 공격·보스 전용 Attribute 등을 이 클래스에 추가한다.
 *
 * 확장 예정:
 *   - CurrentPhase (int32, Replicated) — 현재 보스 페이즈
 *   - 페이즈 전환 이벤트 / 델리게이트
 *   - 보스 전용 AttributeSet (예: `USHBossAttributeSet` — 분노 게이지 등)
 *   - GameplayCue 연동 (페이즈 전환 연출)
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHEnemyBoss : public ASHEnemyBase
{
	GENERATED_BODY()

public:

	ASHEnemyBoss(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
