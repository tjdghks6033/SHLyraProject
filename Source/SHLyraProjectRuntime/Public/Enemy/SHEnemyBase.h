// Copyright SH. All Rights Reserved.

#pragma once

#include "Character/LyraCharacter.h"
#include "GameplayTagContainer.h"

#include "SHEnemyBase.generated.h"

class UGameplayEffect;

/**
 * ASHEnemyBase
 *
 * SHLyraProject의 모든 적 캐릭터 공통 베이스.
 * ALyraCharacter를 상속해 Lyra의 PawnExtensionComponent 초기화 체인,
 * 팀 시스템, HealthComponent, GAS 초기화 흐름을 모두 활용한다.
 *
 * 상태이상 책임:
 *   발사체는 GE를 적용하기만 한다. 빙결 발동은 GE_SHFrozenStack의 OverflowEffects로 처리되며,
 *   이동 제한·BT 일시정지 등 "내 상태에 반응하는" 코드만 여기서 처리한다.
 *   OnAbilitySystemInitialized에서 Status.SH.Frozen 태그 이벤트를 구독한다.
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

	// Status.SH.Frozen 태그 부여/제거 시 호출. 이동 불가 + BT 일시정지를 처리한다.
	void OnFrozenTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 사망 후 액터를 파괴하기까지 대기 시간 (초).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Enemy", meta = (ClampMin = "0.0"))
	float DestroyDelay = 3.0f;

	// OnAbilitySystemInitialized 시점에 저장한 기본 이동 속도. 빙결 해제 시 복원.
	float OriginalMaxWalkSpeed = 0.f;

	FTimerHandle DestroyTimerHandle;

	void HandleDestroyAfterDelay();
};
