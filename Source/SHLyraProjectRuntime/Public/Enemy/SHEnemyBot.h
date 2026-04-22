// Copyright SH. All Rights Reserved.

#pragma once

#include "Enemy/SHEnemyBase.h"

#include "SHEnemyBot.generated.h"

/**
 * ASHEnemyBot
 *
 * SHLyraProject의 일반 졸개(잡몹) 캐릭터.
 * 공통 동작은 모두 ASHEnemyBase가 담당하며, 이 클래스는 졸개 고유의 확장 훅이 필요할 때만 사용한다.
 *
 * 졸개 차별화(체력, 어빌리티, 메시)는 `DA_SHEnemyBotPawnData`(ULyraPawnData)에서 수행한다.
 * 이 클래스를 Blueprint로 파생해 BP_SHEnemyBot을 만들고, 해당 BP를 PawnData.PawnClass로 지정한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHEnemyBot : public ASHEnemyBase
{
	GENERATED_BODY()

public:

	ASHEnemyBot(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
