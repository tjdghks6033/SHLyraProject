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

	// 공중으로 띄워질 때 재생. 애니메이션 확보 후 BP에서 설정. (optional)
	UPROPERTY(EditDefaultsOnly, Category = "SH|Enemy|Montage")
	TObjectPtr<UAnimMontage> LaunchReactionMontage;

	// 슬램으로 바닥에 내리꽂힐 때 재생하는 넉다운 애니메이션.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Enemy|Montage")
	TObjectPtr<UAnimMontage> KnockedDownMontage;

private:

	// Status.SH.Frozen 태그 부여/제거 시 호출. 이동 불가 + BT 일시정지를 처리한다.
	void OnFrozenTagChanged(const FGameplayTag Tag, int32 NewCount);

	// Status.SH.Launched 태그 부여/제거 시 호출. MOVE_Flying으로 공중 부양 + BT 일시정지.
	void OnLaunchedTagChanged(const FGameplayTag Tag, int32 NewCount);

	// Status.SH.KnockedDown 태그 부여/제거 시 호출. 넉다운 몽타주 재생 + BT 일시정지.
	void OnKnockedDownTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 사망 후 액터를 파괴하기까지 대기 시간 (초).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Enemy", meta = (ClampMin = "0.0"))
	float DestroyDelay = 3.0f;

	// OnAbilitySystemInitialized 시점에 저장한 기본 이동 속도. 빙결/착지 해제 시 복원.
	float OriginalMaxWalkSpeed = 0.f;

	// OnAbilitySystemInitialized 시점에 저장한 기본 중력 스케일. 착지 해제 시 복원.
	float OriginalGravityScale = 1.f;

	FTimerHandle DestroyTimerHandle;
	FTimerHandle KnockedDownTimerHandle;

	void HandleDestroyAfterDelay();
	void RemoveKnockedDownEffect();
};
