// Copyright SH. All Rights Reserved.

#include "Character/SHHealthComponent.h"

// 라이라 헬스 시스템 — 이벤트 구독 대상
#include "Character/LyraHealthComponent.h"

// ASC 초기화 콜백 등록 대상
#include "Character/LyraPawnExtensionComponent.h"

// GameplayTag 부여/제거에 사용
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NativeGameplayTags.h"

// UI에 체력 변화를 전달하는 메시지 버스
#include "GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHHealthComponent)

// UI 메시지 채널 태그 — GameplayTag로 구분된 메시지 버스 주소
// 이 태그를 UI 위젯에서도 구독하여 체력 변화를 수신합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Message_Health_Changed, "SH.Message.Health.Changed");

// 위험 상태를 나타내는 GameplayTag
// ASC에 이 태그가 있는 동안 "저체력" 상태로 간주됩니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Status_LowHealth, "SH.Status.LowHealth");


USHHealthComponent::USHHealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 이 컴포넌트는 매 프레임 Tick이 필요 없습니다.
	// 체력 변화는 이벤트 기반으로 처리합니다.
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	// 서버와 클라이언트 모두에서 생성됩니다.
	// (bIsLowHealth 같은 상태는 서버에서 판정 후 태그로 복제)
	SetIsReplicatedByDefault(true);
}

void USHHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// -------------------------------------------------------
	// 핵심: ULyraPawnExtensionComponent를 통해 ASC 준비 완료를
	// 기다린 후 바인딩합니다.
	//
	// 라이라의 초기화 순서:
	//   BeginPlay → PawnData 로드 → AbilitySet 부여 → ASC 준비 완료 → 콜백 발화
	//
	// RegisterAndCall: 이미 준비가 완료되어 있으면 즉시 호출,
	//                  아직이라면 완료 시점에 호출합니다.
	// -------------------------------------------------------
	if (ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
	{
		PawnExtComp->OnAbilitySystemInitialized_RegisterAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));

		PawnExtComp->OnAbilitySystemUninitialized_Register(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
	}
}

void USHHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 델리게이트 바인딩은 ULyraHealthComponent 쪽에서 자동 해제되므로
	// 캐시 포인터만 정리합니다.
	LyraHealthComponent = nullptr;

	Super::EndPlay(EndPlayReason);
}

void USHHealthComponent::OnAbilitySystemInitialized()
{
	// ASC가 준비된 시점 — 이제 같은 액터에 LyraHealthComponent가 존재합니다.
	LyraHealthComponent = ULyraHealthComponent::FindHealthComponent(GetOwner());
	if (!LyraHealthComponent)
	{
		// 이 컴포넌트는 LyraHealthComponent가 없는 액터에는 동작하지 않습니다.
		return;
	}

	// -------------------------------------------------------
	// ULyraHealthComponent의 공개 델리게이트에 바인딩합니다.
	// 라이라 소스를 수정하지 않고 이벤트를 수신합니다.
	// -------------------------------------------------------
	LyraHealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::HandleHealthChanged);
	LyraHealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::HandleDeathStarted);

	// 현재 체력을 기준으로 초기 상태를 설정합니다.
	const float NormalizedHealth = LyraHealthComponent->GetHealthNormalized();
	CheckHealthThreshold(NormalizedHealth);
	BroadcastHealthChange(LyraHealthComponent->GetHealth(), LyraHealthComponent->GetMaxHealth());
}

void USHHealthComponent::OnAbilitySystemUninitialized()
{
	// 플레이어 퇴장 등으로 ASC가 해제될 때 바인딩을 정리합니다.
	if (LyraHealthComponent)
	{
		LyraHealthComponent->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleHealthChanged);
		LyraHealthComponent->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandleDeathStarted);
		LyraHealthComponent = nullptr;
	}
}

void USHHealthComponent::HandleHealthChanged(ULyraHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	const float MaxHealth = HealthComponent->GetMaxHealth();
	const float NormalizedHealth = (MaxHealth > 0.0f) ? (NewValue / MaxHealth) : 0.0f;

	// 위험 상태 판정 및 GameplayTag 처리
	CheckHealthThreshold(NormalizedHealth);

	// UI로 체력 변화 브로드캐스트
	BroadcastHealthChange(NewValue, MaxHealth);
}

void USHHealthComponent::HandleDeathStarted(AActor* OwningActor)
{
	// 사망 시 위험 상태 태그 제거 (사망 상태는 별도 태그로 관리)
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		if (ASC->HasMatchingGameplayTag(TAG_SH_Status_LowHealth))
		{
			ASC->RemoveLooseGameplayTag(TAG_SH_Status_LowHealth);
		}
	}

	bIsLowHealth = false;

	// 체력 0 메시지 브로드캐스트 (UI가 사망 상태를 표시할 수 있도록)
	BroadcastHealthChange(0.0f, LyraHealthComponent ? LyraHealthComponent->GetMaxHealth() : 0.0f);
}

void USHHealthComponent::CheckHealthThreshold(float NormalizedHealth)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	const bool bShouldBeLowHealth = (NormalizedHealth <= LowHealthThreshold);

	// 상태가 변경될 때만 태그를 갱신합니다 (불필요한 태그 연산 방지)
	if (bShouldBeLowHealth && !bIsLowHealth)
	{
		// 위험 상태 진입
		ASC->AddLooseGameplayTag(TAG_SH_Status_LowHealth);
		bIsLowHealth = true;
	}
	else if (!bShouldBeLowHealth && bIsLowHealth)
	{
		// 위험 상태 해제 (체력 회복)
		ASC->RemoveLooseGameplayTag(TAG_SH_Status_LowHealth);
		bIsLowHealth = false;
	}
}

void USHHealthComponent::BroadcastHealthChange(float CurrentHealth, float MaxHealth)
{
	// GameplayMessageSubsystem: 발신자와 수신자(UI)가 서로를 모르는 채로
	// 메시지 태그를 통해 통신합니다 (느슨한 결합).
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);

	FSHHealthChangedMessage Message;
	Message.Owner          = GetOwner();
	Message.CurrentHealth  = CurrentHealth;
	Message.MaxHealth      = MaxHealth;
	Message.HealthNormalized = (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f;

	MessageSystem.BroadcastMessage(TAG_SH_Message_Health_Changed, Message);
}

float USHHealthComponent::GetHealthNormalized() const
{
	return LyraHealthComponent ? LyraHealthComponent->GetHealthNormalized() : 0.0f;
}

bool USHHealthComponent::IsLowHealth() const
{
	return bIsLowHealth;
}
