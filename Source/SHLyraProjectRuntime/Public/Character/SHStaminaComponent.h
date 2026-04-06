// Copyright SH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"
#include "GameplayAbilitySpec.h"

#include "SHStaminaComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class USHStaminaSet;

// -------------------------------------------------------
// 메시지 구조체: GameplayMessageSubsystem을 통해
// UI로 스태미나 변화를 전달할 때 사용하는 데이터 컨테이너
// -------------------------------------------------------
USTRUCT(BlueprintType)
struct FSHStaminaChangedMessage
{
	GENERATED_BODY()

	// 스태미나가 변화한 액터 (보통 캐릭터)
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> Owner = nullptr;

	// 변화 후 현재 스태미나
	UPROPERTY(BlueprintReadOnly)
	float CurrentStamina = 0.0f;

	// 최대 스태미나
	UPROPERTY(BlueprintReadOnly)
	float MaxStamina = 0.0f;

	// 0.0 ~ 1.0 정규화 값 (UI 프로그레스바용)
	UPROPERTY(BlueprintReadOnly)
	float StaminaNormalized = 0.0f;
};


// -------------------------------------------------------
// USHStaminaComponent
//
// USHStaminaSet의 이벤트를 구독하여 스태미나 상태를 관리하는 컴포넌트.
//
// 주요 역할:
//   1. ASC 준비 완료 후 USHStaminaSet 델리게이트 바인딩
//   2. 스태미나 소진/회복 시 GameplayTag 관리 (SH.Status.OutOfStamina)
//   3. GameplayMessageSubsystem으로 SH.Message.Stamina.Changed 브로드캐스트
//   4. Regen GE 최적화 관리:
//      - 스태미나가 가득 차면 Regen GE 제거 (불필요한 틱 차단)
//      - 스태미나가 소비되면 Regen GE 재적용
//
// GameFeatureAction_AddComponents를 통해 ALyraCharacter에 주입됩니다.
// -------------------------------------------------------
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SHLYRAPROJECTRUNTIME_API USHStaminaComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:

	USHStaminaComponent(const FObjectInitializer& ObjectInitializer);

	// 같은 액터에서 이 컴포넌트를 찾는 편의 함수
	UFUNCTION(BlueprintPure, Category = "SH|Stamina")
	static USHStaminaComponent* FindSHStaminaComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<USHStaminaComponent>() : nullptr);
	}

	// 현재 스태미나를 0~1 범위로 반환 (UI용)
	UFUNCTION(BlueprintCallable, Category = "SH|Stamina")
	float GetStaminaNormalized() const;

	// 스태미나가 0인 상태 여부
	UFUNCTION(BlueprintCallable, Category = "SH|Stamina")
	bool IsOutOfStamina() const;

protected:

	// -------------------------------------------------------
	// 컴포넌트 수명주기
	// -------------------------------------------------------

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// -------------------------------------------------------
	// ASC 초기화 콜백
	// -------------------------------------------------------

	void OnAbilitySystemInitialized();
	void OnAbilitySystemUninitialized();

	// -------------------------------------------------------
	// USHStaminaSet 델리게이트 핸들러
	// -------------------------------------------------------

	// 스태미나 값이 변경될 때 호출 (서버: GE 실행 후 / 클라이언트: OnRep)
	void HandleStaminaChanged(AActor* Instigator, AActor* Causer,
		const FGameplayEffectSpec* Spec, float Magnitude,
		float OldValue, float NewValue);

	// 스태미나가 처음으로 0이 됐을 때 호출
	void HandleOutOfStamina(AActor* Instigator, AActor* Causer,
		const FGameplayEffectSpec* Spec, float Magnitude,
		float OldValue, float NewValue);

	// -------------------------------------------------------
	// Regen GE 관리
	//
	// Regen은 스태미나가 MaxStamina 미만일 때만 활성화합니다.
	// 가득 찬 상태에서는 GE를 제거해 불필요한 Periodic 틱을 차단합니다.
	// -------------------------------------------------------

	void ApplyRegenEffect();
	void RemoveRegenEffect();

	// -------------------------------------------------------
	// 내부 처리
	// -------------------------------------------------------

	// GameplayMessageSubsystem으로 UI에 스태미나 정보 브로드캐스트
	void BroadcastStaminaChange(float CurrentStamina, float MaxStamina);

	// -------------------------------------------------------
	// 멤버 변수
	// -------------------------------------------------------

	// Periodic GE로 구현된 스태미나 회복 이펙트.
	// DA_SHAbilitySet에 등록하지 않고 이 컴포넌트가 직접 관리합니다.
	// 스태미나가 소비될 때 적용, 가득 찼을 때 제거합니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Stamina|Regen")
	TSubclassOf<UGameplayEffect> StaminaRegenEffect;

	// ASC 초기화 이후 캐싱됩니다. Regen GE 적용/제거에 사용합니다.
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	// 현재 활성화된 Regen GE의 핸들. 제거 시 이 핸들로 식별합니다.
	FActiveGameplayEffectHandle RegenEffectHandle;

	// ASC에서 가져온 스태미나 어트리뷰트 셋 (읽기 전용 참조)
	UPROPERTY()
	TObjectPtr<const USHStaminaSet> StaminaSet;

	// 스태미나 소진 상태 플래그 (중복 처리 방지)
	bool bIsOutOfStamina = false;
};
