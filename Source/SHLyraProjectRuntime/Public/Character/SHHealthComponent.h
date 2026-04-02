// Copyright SH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"

#include "SHHealthComponent.generated.h"

class ULyraAbilitySystemComponent;
class ULyraHealthComponent;

// -------------------------------------------------------
// 메시지 구조체: GameplayMessageSubsystem을 통해
// UI로 체력 변화를 전달할 때 사용하는 데이터 컨테이너
// -------------------------------------------------------
USTRUCT(BlueprintType)
struct FSHHealthChangedMessage
{
	GENERATED_BODY()

	// 체력이 변화한 액터 (보통 캐릭터)
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> Owner = nullptr;

	// 변화 후 현재 체력
	UPROPERTY(BlueprintReadOnly)
	float CurrentHealth = 0.0f;

	// 최대 체력
	UPROPERTY(BlueprintReadOnly)
	float MaxHealth = 0.0f;

	// 0.0 ~ 1.0 정규화 값 (UI 프로그레스바용)
	UPROPERTY(BlueprintReadOnly)
	float HealthNormalized = 0.0f;
};


// -------------------------------------------------------
// USHHealthComponent
//
// ULyraHealthComponent의 이벤트를 구독하여 추가 로직을
// 실행하는 반응형 컴포넌트.
//
// 라이라 소스를 수정하지 않고 GameFeatureAction_AddComponents를
// 통해 ALyraCharacter에 동적으로 부착됩니다.
// -------------------------------------------------------
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SHLYRAPROJECTRUNTIME_API USHHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:

	USHHealthComponent(const FObjectInitializer& ObjectInitializer);

	// 같은 액터에서 이 컴포넌트를 찾는 편의 함수
	UFUNCTION(BlueprintPure, Category = "SH|Health")
	static USHHealthComponent* FindSHHealthComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<USHHealthComponent>() : nullptr);
	}

	// 현재 체력을 0~1 범위로 반환 (UI용)
	UFUNCTION(BlueprintCallable, Category = "SH|Health")
	float GetHealthNormalized() const;

	// 위험 상태(체력 LowHealthThreshold 이하) 여부
	UFUNCTION(BlueprintCallable, Category = "SH|Health")
	bool IsLowHealth() const;

protected:

	// -------------------------------------------------------
	// 컴포넌트 수명주기
	// -------------------------------------------------------

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// -------------------------------------------------------
	// ASC 초기화 콜백
	//
	// ULyraPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall을
	// 통해 등록됨 — ASC와 AttributeSet이 모두 준비된 이후 호출이 보장됨
	// -------------------------------------------------------
	void OnAbilitySystemInitialized();
	void OnAbilitySystemUninitialized();

	// -------------------------------------------------------
	// ULyraHealthComponent 델리게이트 바인딩 핸들러
	// -------------------------------------------------------

	// 체력 변화 시 호출 (데미지/힐 모두 포함)
	UFUNCTION()
	void HandleHealthChanged(ULyraHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	// 사망 시작 시 호출 (체력 0 도달)
	UFUNCTION()
	void HandleDeathStarted(AActor* OwningActor);

	// -------------------------------------------------------
	// 내부 처리
	// -------------------------------------------------------

	// 체력 임계값 체크 → 위험 상태 GameplayTag 부여/제거
	void CheckHealthThreshold(float NormalizedHealth);

	// GameplayMessageSubsystem으로 UI에 체력 정보 브로드캐스트
	void BroadcastHealthChange(float CurrentHealth, float MaxHealth);

	// -------------------------------------------------------
	// 멤버 변수
	// -------------------------------------------------------

	// 바인딩 대상인 라이라 헬스 컴포넌트 (같은 액터에 존재)
	UPROPERTY()
	TObjectPtr<ULyraHealthComponent> LyraHealthComponent;

	// 위험 상태 판정 기준값 (0~1 범위, 기본 30%)
	UPROPERTY(EditDefaultsOnly, Category = "SH|Health", meta=(ClampMin=0.0f, ClampMax=1.0f))
	float LowHealthThreshold = 0.3f;

	// 중복 이벤트 방지를 위한 현재 위험 상태 플래그
	bool bIsLowHealth = false;
};
