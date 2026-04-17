// Copyright SH. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"

#include "SHEnemyCharacter.generated.h"

class ULyraAbilitySystemComponent;
class ULyraHealthComponent;
class UGameplayEffect;

/**
 * ASHEnemyCharacter
 *
 * ACharacter를 직접 상속하고 IAbilitySystemInterface를 구현하는 적 캐릭터.
 * ALyraCharacter의 복잡한 PawnExtensionComponent 초기화 체인을 거치지 않고
 * BeginPlay에서 ASC를 직접 초기화한다.
 *
 * GAS 초기화 흐름 (BeginPlay):
 *   1. ASC::InitAbilityActorInfo
 *   2. LyraHealthSet 부여 (AddAttributeSetSubobject)
 *   3. InitHealthEffect 적용 → Health/MaxHealth 초기값 설정
 *   4. LyraHealthComponent::InitializeWithAbilitySystem
 *   5. OnDeathStarted 바인딩
 *
 * USHMeleeAttack::PerformMeleeHit은 IAbilitySystemInterface를 통해 ASC를 찾으므로
 * 이 구조에서 데미지가 정상 적용된다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API ASHEnemyCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	ASHEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	UFUNCTION()
	void HandleEnemyDeathStarted(AActor* OwningActor);

	void DestroyEnemy();

protected:

	// 직접 소유하는 ASC. BeginPlay에서 InitAbilityActorInfo로 초기화된다.
	// ULyraHealthComponent::InitializeWithAbilitySystem이 ULyraAbilitySystemComponent를 요구한다.
	UPROPERTY(VisibleAnywhere, Category = "SH|Enemy|GAS")
	TObjectPtr<ULyraAbilitySystemComponent> AbilitySystemComponent;

	// 체력 관리 및 사망 이벤트 처리. BeginPlay에서 ASC와 연결된다.
	UPROPERTY(VisibleAnywhere, Category = "SH|Enemy|GAS")
	TObjectPtr<ULyraHealthComponent> HealthComponent;

	// Health / MaxHealth 초기값을 설정하는 Instant GameplayEffect.
	// BP_SHEnemy에서 GE_SHInitEnemyHealth를 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Enemy|GAS")
	TSubclassOf<UGameplayEffect> InitHealthEffect;

	// 사망 후 액터를 파괴하기까지 대기 시간 (초).
	UPROPERTY(EditDefaultsOnly, Category = "SH|Enemy|Death", meta = (ClampMin = "0.0"))
	float DestroyDelay = 3.0f;
};
