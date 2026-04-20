// Copyright SH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"
#include "GameplayAbilitySpec.h"

#include "SHManaComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class USHManaSet;

// -------------------------------------------------------
// 메시지 구조체: GameplayMessageSubsystem을 통해
// UI로 마나 변화를 전달할 때 사용하는 데이터 컨테이너
// -------------------------------------------------------
USTRUCT(BlueprintType)
struct FSHManaChangedMessage
{
	GENERATED_BODY()

	// 마나가 변화한 액터 (보통 캐릭터)
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> Owner = nullptr;

	// 변화 후 현재 마나
	UPROPERTY(BlueprintReadOnly)
	float CurrentMana = 0.0f;

	// 최대 마나
	UPROPERTY(BlueprintReadOnly)
	float MaxMana = 0.0f;

	// 0.0 ~ 1.0 정규화 값 (UI 프로그레스바용)
	UPROPERTY(BlueprintReadOnly)
	float ManaNormalized = 0.0f;
};


// -------------------------------------------------------
// USHManaComponent
//
// USHManaSet의 이벤트를 구독하여 마나 상태를 관리하는 컴포넌트.
// USHStaminaComponent와 동일한 패턴으로 설계됐습니다.
//
// 주요 역할:
//   1. ASC 준비 완료 후 USHManaSet 델리게이트 바인딩
//   2. 마나 소진/회복 시 GameplayTag 관리 (SH.Status.OutOfMana)
//   3. GameplayMessageSubsystem으로 SH.Message.Mana.Changed 브로드캐스트
//   4. Regen GE 최적화 관리:
//      - 마나가 가득 차면 Regen GE 제거 (불필요한 틱 차단)
//      - 마나가 소비되면 Regen GE 재적용
//   5. 마나 < MagicManaCostThreshold 시 Ability.Type.Action.Magic 어빌리티 차단
//
// 주의: 마법 어빌리티의 마나 소비 자체는 GA_SHMagicProjectile의
//       CostGameplayEffectClass(GE_SHMagicManaCost)가 처리합니다.
//       이 컴포넌트는 UI 브로드캐스트, 태그 관리, 블록/언블록만 담당합니다.
//       스태미나처럼 AbilityActivatedCallbacks로 소비 GE를 별도 적용하지 않습니다.
//
// GameFeatureAction_AddComponents를 통해 ALyraCharacter에 주입됩니다.
// -------------------------------------------------------
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SHLYRAPROJECTRUNTIME_API USHManaComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:

	USHManaComponent(const FObjectInitializer& ObjectInitializer);

	// 같은 액터에서 이 컴포넌트를 찾는 편의 함수
	UFUNCTION(BlueprintPure, Category = "SH|Mana")
	static USHManaComponent* FindSHManaComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<USHManaComponent>() : nullptr);
	}

	// 현재 마나를 0~1 범위로 반환 (UI용)
	UFUNCTION(BlueprintCallable, Category = "SH|Mana")
	float GetManaNormalized() const;

	// 마나가 0인 상태 여부
	UFUNCTION(BlueprintCallable, Category = "SH|Mana")
	bool IsOutOfMana() const;

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
	// USHManaSet 델리게이트 핸들러
	// -------------------------------------------------------

	// 마나 값이 변경될 때 호출 (서버: GE 실행 후 / 클라이언트: OnRep)
	void HandleManaChanged(AActor* Instigator, AActor* Causer,
		const FGameplayEffectSpec* Spec, float Magnitude,
		float OldValue, float NewValue);

	// 마나가 처음으로 0이 됐을 때 호출
	void HandleOutOfMana(AActor* Instigator, AActor* Causer,
		const FGameplayEffectSpec* Spec, float Magnitude,
		float OldValue, float NewValue);

	// -------------------------------------------------------
	// Regen GE 관리
	//
	// Regen은 마나가 MaxMana 미만일 때만 활성화합니다.
	// 가득 찬 상태에서는 GE를 제거해 불필요한 Periodic 틱을 차단합니다.
	// -------------------------------------------------------

	void ApplyRegenEffect();
	void RemoveRegenEffect();

	// -------------------------------------------------------
	// 내부 처리
	// -------------------------------------------------------

	// GameplayMessageSubsystem으로 UI에 마나 정보 브로드캐스트
	void BroadcastManaChange(float CurrentMana, float MaxMana);

	// -------------------------------------------------------
	// 멤버 변수
	// -------------------------------------------------------

	// Periodic GE로 구현된 마나 회복 이펙트.
	// DA_SHAbilitySet에 등록하지 않고 이 컴포넌트가 직접 관리합니다.
	// 마나가 소비될 때 적용, 가득 찼을 때 제거합니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Mana|Regen")
	TSubclassOf<UGameplayEffect> ManaRegenEffect;

	// 마법 어빌리티 한 번에 필요한 최소 마나.
	// 이 값 미만이면 Ability.Type.Action.Magic 태그를 가진 어빌리티가 차단됩니다.
	// GE_SHMagicManaCost와 동일한 값으로 BP에서 조정합니다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Mana|Block")
	float MagicManaCostThreshold = 20.0f;

	// ASC 초기화 이후 캐싱됩니다. Regen GE 적용/제거에 사용합니다.
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	// 현재 활성화된 Regen GE의 핸들. 제거 시 이 핸들로 식별합니다.
	FActiveGameplayEffectHandle RegenEffectHandle;

	// ASC에서 가져온 마나 어트리뷰트 셋 (읽기 전용 참조)
	UPROPERTY()
	TObjectPtr<const USHManaSet> ManaSet;

	// 마나 소진 상태 플래그 (중복 처리 방지)
	bool bIsOutOfMana = false;

	// 마법 어빌리티가 현재 차단된 상태인지 추적합니다 (중복 Block/Unblock 호출 방지).
	bool bMagicBlocked = false;
};
