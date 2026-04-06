// Copyright SH. All Rights Reserved.

#include "Character/SHStaminaComponent.h"

#include "AbilitySystem/Attributes/SHStaminaSet.h"

// ASC 초기화 콜백 등록 대상
#include "Character/LyraPawnExtensionComponent.h"

// GameplayTag 부여/제거에 사용
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "NativeGameplayTags.h"

// UI에 스태미나 변화를 전달하는 메시지 버스
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHStaminaComponent)

// 스태미나 소진 상태를 나타내는 GameplayTag.
// 이 태그가 ASC에 있는 동안 스태미나를 소비하는 어빌리티는 발동할 수 없습니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Status_OutOfStamina, "SH.Status.OutOfStamina");

// UI 메시지 채널 태그. 이 채널을 구독하는 위젯이 스태미나 변화를 수신합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Message_Stamina_Changed, "SH.Message.Stamina.Changed");

// GA_Hero_Dash에 부여된 어빌리티 태그. 이 태그로 대쉬 어빌리티를 식별합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Type_Action_Dash, "Ability.Type.Action.Dash");


USHStaminaComponent::USHStaminaComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	// 서버와 클라이언트 모두에서 생성됩니다.
	SetIsReplicatedByDefault(true);
}

void USHStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	// LyraPawnExtensionComponent를 통해 ASC 준비 완료를 기다립니다.
	// RegisterAndCall: 이미 준비됐으면 즉시 호출, 아직이라면 준비 완료 시 호출합니다.
	if (ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
	{
		PawnExtComp->OnAbilitySystemInitialized_RegisterAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));

		PawnExtComp->OnAbilitySystemUninitialized_Register(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
	}
}

void USHStaminaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Regen GE가 남아 있으면 정리합니다.
	RemoveRegenEffect();

	CachedASC  = nullptr;
	StaminaSet = nullptr;

	Super::EndPlay(EndPlayReason);
}

void USHStaminaComponent::OnAbilitySystemInitialized()
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}
	
	// USHStaminaSet은 DA_SHAbilitySet을 통해 ASC에 부여됩니다.
	// 이 시점에는 이미 등록되어 있어야 합니다.
	const USHStaminaSet* FoundSet = ASC->GetSet<USHStaminaSet>();
	if (!FoundSet)
	{
		return;
	}

	CachedASC  = ASC;
	StaminaSet = FoundSet;

	// 스태미나 변화 이벤트 바인딩
	StaminaSet->OnStaminaChanged.AddUObject(this, &ThisClass::HandleStaminaChanged);
	StaminaSet->OnOutOfStamina.AddUObject(this, &ThisClass::HandleOutOfStamina);

	// 대쉬 어빌리티 활성화 감지: GA_Hero_Dash가 켜질 때마다 스태미나를 소비합니다.
	ASC->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated);

	// 초기 상태 처리:
	// 스태미나가 최대치 미만이라면 즉시 Regen을 시작합니다.
	const float CurrentStamina = StaminaSet->GetStamina();
	const float MaxStamina     = StaminaSet->GetMaxStamina();

	if (CurrentStamina < MaxStamina && StaminaRegenEffect)
	{
		ApplyRegenEffect();
	}

	// 초기 스태미나가 대쉬 비용 미만이라면 즉시 차단합니다.
	if (CurrentStamina < DashStaminaCostThreshold)
	{
		ASC->BlockAbilitiesWithTags(FGameplayTagContainer(TAG_Ability_Type_Action_Dash));
		bDashBlocked = true;
	}

	BroadcastStaminaChange(CurrentStamina, MaxStamina);
}

void USHStaminaComponent::OnAbilitySystemUninitialized()
{
	// 플레이어 퇴장 등으로 ASC가 해제될 때 정리합니다.
	if (StaminaSet)
	{
		StaminaSet->OnStaminaChanged.RemoveAll(this);
		StaminaSet->OnOutOfStamina.RemoveAll(this);
		StaminaSet = nullptr;
	}

	if (CachedASC)
	{
		CachedASC->AbilityActivatedCallbacks.RemoveAll(this);

		// ASC 해제 전에 차단 상태를 정리합니다.
		if (bDashBlocked)
		{
			CachedASC->UnBlockAbilitiesWithTags(FGameplayTagContainer(TAG_Ability_Type_Action_Dash));
			bDashBlocked = false;
		}
	}

	RemoveRegenEffect();
	CachedASC = nullptr;
}

void USHStaminaComponent::HandleStaminaChanged(AActor* Instigator, AActor* Causer,
	const FGameplayEffectSpec* Spec, float Magnitude,
	float OldValue, float NewValue)
{
	if (!CachedASC || !StaminaSet)
	{
		return;
	}

	const float MaxStamina = StaminaSet->GetMaxStamina();
	const bool  bWasFull   = (OldValue >= MaxStamina);
	const bool  bIsFull    = (NewValue >= MaxStamina);

	// -------------------------------------------------------
	// Regen GE 관리:
	//   가득 찼을 때 → GE 제거 (불필요한 Periodic 틱 차단)
	//   소비로 인해 가득 참 해제 → GE 재적용
	// -------------------------------------------------------
	if (bWasFull && !bIsFull && StaminaRegenEffect)
	{
		ApplyRegenEffect();
	}
	else if (!bWasFull && bIsFull)
	{
		RemoveRegenEffect();
	}

	// 스태미나가 회복돼 OutOfStamina 상태에서 벗어났다면 태그를 제거합니다.
	if (bIsOutOfStamina && NewValue > 0.0f)
	{
		CachedASC->RemoveLooseGameplayTag(TAG_SH_Status_OutOfStamina);
		bIsOutOfStamina = false;
	}

	// -------------------------------------------------------
	// 대쉬 차단/해제:
	//   스태미나 < DashStaminaCostThreshold → 대쉬 차단
	//   스태미나 ≥ DashStaminaCostThreshold → 차단 해제
	// -------------------------------------------------------
	const bool bCanDash = (NewValue >= DashStaminaCostThreshold);
	if (bDashBlocked && bCanDash)
	{
		CachedASC->UnBlockAbilitiesWithTags(FGameplayTagContainer(TAG_Ability_Type_Action_Dash));
		bDashBlocked = false;
	}
	else if (!bDashBlocked && !bCanDash)
	{
		CachedASC->BlockAbilitiesWithTags(FGameplayTagContainer(TAG_Ability_Type_Action_Dash));
		bDashBlocked = true;
	}

	BroadcastStaminaChange(NewValue, MaxStamina);
}

void USHStaminaComponent::HandleOutOfStamina(AActor* Instigator, AActor* Causer,
	const FGameplayEffectSpec* Spec, float Magnitude,
	float OldValue, float NewValue)
{
	if (!CachedASC)
	{
		return;
	}

	// SH.Status.OutOfStamina 태그를 부여합니다.
	// GA_SHDash는 CanActivateAbility에서 이 태그를 검사해 발동을 막습니다.
	CachedASC->AddLooseGameplayTag(TAG_SH_Status_OutOfStamina);
	bIsOutOfStamina = true;

	// Regen GE가 없으면 스태미나는 영원히 0으로 유지됩니다.
	// Regen이 설정돼 있다면 소진 직후 회복을 시작합니다.
	if (StaminaRegenEffect && !RegenEffectHandle.IsValid())
	{
		ApplyRegenEffect();
	}
}

void USHStaminaComponent::ApplyRegenEffect()
{
	if (!CachedASC || !StaminaRegenEffect || RegenEffectHandle.IsValid())
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = CachedASC->MakeEffectContext();
	ContextHandle.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(
		StaminaRegenEffect, 1.0f, ContextHandle);

	if (SpecHandle.IsValid())
	{
		RegenEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void USHStaminaComponent::RemoveRegenEffect()
{
	if (CachedASC && RegenEffectHandle.IsValid())
	{
		CachedASC->RemoveActiveGameplayEffect(RegenEffectHandle);
	}

	RegenEffectHandle = FActiveGameplayEffectHandle();
}

void USHStaminaComponent::BroadcastStaminaChange(float CurrentStamina, float MaxStamina)
{
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);

	FSHStaminaChangedMessage Message;
	Message.Owner            = GetOwner();
	Message.CurrentStamina   = CurrentStamina;
	Message.MaxStamina       = MaxStamina;
	Message.StaminaNormalized = (MaxStamina > 0.0f) ? (CurrentStamina / MaxStamina) : 0.0f;

	MessageSystem.BroadcastMessage(TAG_SH_Message_Stamina_Changed, Message);
}

void USHStaminaComponent::HandleAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	if (!CachedASC || !StaminaDashCostEffect || !ActivatedAbility)
	{
		return;
	}

	// Ability.Type.Action.Dash 태그를 가진 어빌리티만 처리합니다.
	// GA_Hero_Dash(ShooterCore)가 이 태그를 보유합니다.
	if (!ActivatedAbility->GetAssetTags().HasTag(TAG_Ability_Type_Action_Dash))
	{
		return;
	}

	// StaminaCost 메타 어트리뷰트를 통해 스태미나 50을 차감합니다.
	// SHStaminaSet::PostGameplayEffectExecute에서 Stamina -= StaminaCost 처리됩니다.
	FGameplayEffectContextHandle ContextHandle = CachedASC->MakeEffectContext();
	ContextHandle.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(
		StaminaDashCostEffect, 1.0f, ContextHandle);

	if (SpecHandle.IsValid())
	{
		CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

float USHStaminaComponent::GetStaminaNormalized() const
{
	if (!StaminaSet)
	{
		return 0.0f;
	}

	const float MaxStamina = StaminaSet->GetMaxStamina();
	return (MaxStamina > 0.0f) ? (StaminaSet->GetStamina() / MaxStamina) : 0.0f;
}

bool USHStaminaComponent::IsOutOfStamina() const
{
	return bIsOutOfStamina;
}
