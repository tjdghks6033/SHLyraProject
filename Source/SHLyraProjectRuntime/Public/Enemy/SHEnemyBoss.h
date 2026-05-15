// Copyright SH. All Rights Reserved.

#pragma once

#include "Enemy/SHEnemyBase.h"

#include "SHEnemyBoss.generated.h"

class UGameplayEffect;

/**
 * FSHBossMessage
 *
 * SH.Message.Boss.Engaged / SH.Message.Boss.Defeated 채널 페이로드.
 * HUD가 보스 액터 참조를 통해 LyraHealthSet 어트리뷰트에 바인딩한다.
 */
USTRUCT(BlueprintType)
struct FSHBossMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> BossActor = nullptr;
};

/**
 * ASHEnemyBoss
 *
 * SHLyraProject 보스 캐릭터.
 * BeginPlay 시 SH.Message.Boss.Engaged를 브로드캐스트해 보스 HUD를 활성화한다.
 * 사망 시 OnDeathStarted에서 SH.Message.Boss.Defeated를 브로드캐스트한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHEnemyBoss : public ASHEnemyBase
{
	GENERATED_BODY()

public:

	ASHEnemyBoss(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void BeginPlay() override;
	virtual void OnAbilitySystemInitialized() override;
	virtual void OnDeathStarted(AActor* OwningActor) override;

	// 보스 HUD 상단에 표시되는 이름.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Boss")
	FText BossDisplayName;

	// 총 페이즈 수. 현재는 1 고정.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Boss", meta = (ClampMin = "1"))
	int32 MaxPhase = 1;

	// ASC 초기화 직후 적용할 GE 목록. BP에서 GE_InitBossHealth 등 지정.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Boss")
	TArray<TSubclassOf<UGameplayEffect>> InitGameplayEffects;
};
