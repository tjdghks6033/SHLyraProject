// Copyright SH. All Rights Reserved.

#include "Character/SHManaComponent.h"

#include "AbilitySystem/Attributes/SHManaSet.h"

// ASC 초기화 콜백 등록 대상
#include "Character/LyraPawnExtensionComponent.h"

// GameplayTag 부여/제거, Block/Unblock에 사용
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NativeGameplayTags.h"

// UI에 마나 변화를 전달하는 메시지 버스
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHManaComponent)

// 마나 소진 상태를 나타내는 GameplayTag.
// 이 태그가 ASC에 있는 동안 마나를 소비하는 어빌리티는 발동할 수 없습니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Status_OutOfMana, "SH.Status.OutOfMana");

// UI 메시지 채널 태그. 이 채널을 구독하는 위젯이 마나 변화를 수신합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Message_Mana_Changed, "SH.Message.Mana.Changed");

// 마법 어빌리티 분류 태그. GA_SHMagicProjectile의 AbilityTags에 추가되어야
// BlockAbilitiesWithTags로 차단 가능합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Type_Action_Magic, "Ability.Type.Action.Magic");


USHManaComponent::USHManaComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	// 서버와 클라이언트 모두에서 생성됩니다.
	SetIsReplicatedByDefault(true);
}

void USHManaComponent::BeginPlay()
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

void USHManaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Regen GE가 남아 있으면 정리합니다.
	RemoveRegenEffect();

	CachedASC = nullptr;
	ManaSet   = nullptr;

	Super::EndPlay(EndPlayReason);
}

void USHManaComponent::OnAbilitySystemInitialized()
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	// USHManaSet은 DA_SHAbilitySet을 통해 ASC에 부여됩니다.
	// 이 시점에는 이미 등록되어 있어야 합니다.
	const USHManaSet* FoundSet = ASC->GetSet<USHManaSet>();
	if (!FoundSet)
	{
		return;
	}

	CachedASC = ASC;
	ManaSet   = FoundSet;

	// 마나 변화 이벤트 바인딩
	ManaSet->OnManaChanged.AddUObject(this, &ThisClass::HandleManaChanged);
	ManaSet->OnOutOfMana.AddUObject(this, &ThisClass::HandleOutOfMana);

	// 초기 상태 처리:
	// 마나가 최대치 미만이라면 즉시 Regen을 시작합니다.
	const float CurrentMana = ManaSet->GetMana();
	const float MaxMana     = ManaSet->GetMaxMana();

	if (CurrentMana < MaxMana && ManaRegenEffect)
	{
		ApplyRegenEffect();
	}

	// 초기 마나가 마법 비용 미만이라면 즉시 차단합니다.
	if (CurrentMana < MagicManaCostThreshold)
	{
		ASC->BlockAbilitiesWithTags(FGameplayTagContainer(TAG_Ability_Type_Action_Magic));
		bMagicBlocked = true;
	}

	BroadcastManaChange(CurrentMana, MaxMana);
}

void USHManaComponent::OnAbilitySystemUninitialized()
{
	// 플레이어 퇴장 등으로 ASC가 해제될 때 정리합니다.
	if (ManaSet)
	{
		ManaSet->OnManaChanged.RemoveAll(this);
		ManaSet->OnOutOfMana.RemoveAll(this);
		ManaSet = nullptr;
	}

	if (CachedASC)
	{
		// ASC 해제 전에 차단 상태를 정리합니다.
		if (bMagicBlocked)
		{
			CachedASC->UnBlockAbilitiesWithTags(FGameplayTagContainer(TAG_Ability_Type_Action_Magic));
			bMagicBlocked = false;
		}
	}

	RemoveRegenEffect();
	CachedASC = nullptr;
}

void USHManaComponent::HandleManaChanged(AActor* Instigator, AActor* Causer,
	const FGameplayEffectSpec* Spec, float Magnitude,
	float OldValue, float NewValue)
{
	if (!CachedASC || !ManaSet)
	{
		return;
	}

	const float MaxMana = ManaSet->GetMaxMana();
	const bool  bWasFull = (OldValue >= MaxMana);
	const bool  bIsFull  = (NewValue >= MaxMana);

	// -------------------------------------------------------
	// Regen GE 관리:
	//   가득 찼을 때 → GE 제거 (불필요한 Periodic 틱 차단)
	//   소비로 인해 가득 참 해제 → GE 재적용
	// -------------------------------------------------------
	if (bWasFull && !bIsFull && ManaRegenEffect)
	{
		ApplyRegenEffect();
	}
	else if (!bWasFull && bIsFull)
	{
		RemoveRegenEffect();
	}

	// 마나가 회복돼 OutOfMana 상태에서 벗어났다면 태그를 제거합니다.
	if (bIsOutOfMana && NewValue > 0.0f)
	{
		CachedASC->RemoveLooseGameplayTag(TAG_SH_Status_OutOfMana);
		bIsOutOfMana = false;
	}

	// -------------------------------------------------------
	// 마법 어빌리티 차단/해제:
	//   마나 < MagicManaCostThreshold → 차단
	//   마나 ≥ MagicManaCostThreshold → 차단 해제
	// -------------------------------------------------------
	const bool bCanCast = (NewValue >= MagicManaCostThreshold);
	if (bMagicBlocked && bCanCast)
	{
		CachedASC->UnBlockAbilitiesWithTags(FGameplayTagContainer(TAG_Ability_Type_Action_Magic));
		bMagicBlocked = false;
	}
	else if (!bMagicBlocked && !bCanCast)
	{
		CachedASC->BlockAbilitiesWithTags(FGameplayTagContainer(TAG_Ability_Type_Action_Magic));
		bMagicBlocked = true;
	}

	BroadcastManaChange(NewValue, MaxMana);
}

void USHManaComponent::HandleOutOfMana(AActor* Instigator, AActor* Causer,
	const FGameplayEffectSpec* Spec, float Magnitude,
	float OldValue, float NewValue)
{
	if (!CachedASC)
	{
		return;
	}

	// SH.Status.OutOfMana 태그를 부여합니다.
	CachedASC->AddLooseGameplayTag(TAG_SH_Status_OutOfMana);
	bIsOutOfMana = true;

	// Regen GE가 없으면 마나는 영원히 0으로 유지됩니다.
	// Regen이 설정돼 있다면 소진 직후 회복을 시작합니다.
	if (ManaRegenEffect && !RegenEffectHandle.IsValid())
	{
		ApplyRegenEffect();
	}
}

void USHManaComponent::ApplyRegenEffect()
{
	if (!CachedASC || !ManaRegenEffect || RegenEffectHandle.IsValid())
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = CachedASC->MakeEffectContext();
	ContextHandle.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(
		ManaRegenEffect, 1.0f, ContextHandle);

	if (SpecHandle.IsValid())
	{
		RegenEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void USHManaComponent::RemoveRegenEffect()
{
	if (CachedASC && RegenEffectHandle.IsValid())
	{
		CachedASC->RemoveActiveGameplayEffect(RegenEffectHandle);
	}

	RegenEffectHandle = FActiveGameplayEffectHandle();
}

void USHManaComponent::BroadcastManaChange(float CurrentMana, float MaxMana)
{
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);

	FSHManaChangedMessage Message;
	Message.Owner          = GetOwner();
	Message.CurrentMana    = CurrentMana;
	Message.MaxMana        = MaxMana;
	Message.ManaNormalized = (MaxMana > 0.0f) ? (CurrentMana / MaxMana) : 0.0f;

	MessageSystem.BroadcastMessage(TAG_SH_Message_Mana_Changed, Message);
}

float USHManaComponent::GetManaNormalized() const
{
	if (!ManaSet)
	{
		return 0.0f;
	}

	const float MaxMana = ManaSet->GetMaxMana();
	return (MaxMana > 0.0f) ? (ManaSet->GetMana() / MaxMana) : 0.0f;
}

bool USHManaComponent::IsOutOfMana() const
{
	return bIsOutOfMana;
}
