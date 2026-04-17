// Copyright SH. All Rights Reserved.

#include "Enemy/SHEnemySpawnPoint.h"
#include "Components/ArrowComponent.h"

ASHEnemySpawnPoint::ASHEnemySpawnPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 에디터에서 스폰 방향을 시각적으로 확인하기 위한 화살표 컴포넌트
	UArrowComponent* Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetArrowColor(FLinearColor::Red);
	Arrow->bIsScreenSizeScaled = true;
	SetRootComponent(Arrow);

	// 런타임에는 틱이 필요 없다
	PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}
