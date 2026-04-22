// Copyright SH. All Rights Reserved.

#pragma once

#include "GenericTeamAgentInterface.h"

/**
 * SHLyraProject 전용 팀 ID 상수.
 *
 * Lyra 팀 솔버는 두 액터의 TeamId가 다르면 Hostile로 판정한다.
 * 플레이어(0) / SH 적(10) 은 항상 서로 Hostile이며,
 * NoTeam(255) 기본값이면 Friendly/Neutral로 판정돼 AIPerception 필터에 걸리는 것을 방지한다.
 *
 * ShooterCore 봇이 사용하는 Team 1과도 겹치지 않는 번호를 선택했다.
 */
namespace SHLyraProject::TeamIds
{
	// 플레이어 팀
	inline constexpr int32 Player = 0;

	// SH 프로젝트 적 진영 (봇/보스 공통)
	inline constexpr int32 SHEnemy = 10;

	inline FGenericTeamId PlayerTeamId()  { return FGenericTeamId(Player);  }
	inline FGenericTeamId EnemyTeamId()   { return FGenericTeamId(SHEnemy); }
}
