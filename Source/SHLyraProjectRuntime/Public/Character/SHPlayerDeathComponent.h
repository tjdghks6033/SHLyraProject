// Copyright SH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"

#include "SHPlayerDeathComponent.generated.h"

// WBP_SHResultScreen이 구독하는 플레이어 사망 메시지 페이로드.
USTRUCT(BlueprintType)
struct FSHPlayerDiedMessage
{
	GENERATED_BODY()

	// 사망한 플레이어 캐릭터 액터
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> Player = nullptr;
};

// -------------------------------------------------------
// USHPlayerDeathComponent
//
// 로컬 플레이어 캐릭터의 사망을 GameplayMessageSubsystem으로 중계하는 컴포넌트.
//
// 주요 역할:
//   1. ASC 준비 완료 후 LyraHealthComponent.OnDeathStarted 바인딩
//   2. 로컬 플레이어 사망 시 SH.Message.Player.Died 브로드캐스트
//
// Lyra 설계 원칙 (캐릭터 → 컴포넌트 → 메시지 버스 → UI) 준수.
// USHStaminaComponent / USHManaComponent 와 동일한 패턴.
// GameFeatureAction_AddComponents로 ALyraCharacter에 주입됩니다.
// -------------------------------------------------------
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SHLYRAPROJECTRUNTIME_API USHPlayerDeathComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	USHPlayerDeathComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnAbilitySystemInitialized();

	UFUNCTION()
	void OnDeathStarted(AActor* OwningActor);
};
