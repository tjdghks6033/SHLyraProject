// Copyright SH. All Rights Reserved.

#include "Character/SHResourceComponent.h"

// ASC 초기화 콜백 등록 대상
#include "Character/LyraPawnExtensionComponent.h"

// GameplayTag 부여/제거, Block/Unblock, Regen GE 적용에 사용
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

// IsPlayerControlled() 사용에 필요
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHResourceComponent)

USHResourceComponent::USHResourceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	// 서버와 클라이언트 모두에서 생성됩니다.
	SetIsReplicatedByDefault(true);
}

void USHResourceComponent::BeginPlay()
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

void USHResourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Regen GE가 남아 있으면 정리합니다.
	RemoveRegenEffect();

	ReleaseAttributeSet();
	CachedASC = nullptr;

	Super::EndPlay(EndPlayReason);
}

void USHResourceComponent::OnAbilitySystemInitialized()
{
	// 플레이어 전용 컴포넌트 — AI 폰(보스 포함)에서는 비활성화.
	// 컴포넌트가 ALyraCharacter 전체에 주입되지만, 보스 ASC가 자원 셋을 보유하면
	// 보스 자원 변화가 플레이어 HUD 채널로 오염되므로 차단한다.
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		if (!Char->IsPlayerControlled())
		{
			return;
		}
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	// 자원 어트리뷰트 셋은 DA_SHAbilitySet을 통해 ASC에 부여됩니다.
	// 이 시점에는 이미 등록되어 있어야 합니다.
	if (!ResolveAttributeSet(ASC))
	{
		return;
	}

	CachedASC = ASC;

	BindValueDelegates();

	// 초기 상태 처리: 값이 최대치 미만이면 즉시 Regen 시작.
	const float CurrentValue = GetCurrentValue();
	const float MaxValue     = GetMaxValue();

	if (CurrentValue < MaxValue && GetRegenEffect())
	{
		ApplyRegenEffect();
	}

	// 초기 값이 코스트 임계값 미만이면 즉시 차단.
	if (CurrentValue < GetCostThreshold())
	{
		ASC->BlockAbilitiesWithTags(FGameplayTagContainer(GetBlockedAbilityTag()));
		bAbilityBlocked = true;
	}

	BroadcastChange(CurrentValue, MaxValue);

	// 파생 고유 초기화 (스태미나 대쉬 코스트 감시 등).
	OnPostInitialize();
}

void USHResourceComponent::OnAbilitySystemUninitialized()
{
	// 파생 고유 정리 먼저 — CachedASC가 아직 유효한 시점에 처리한다.
	OnPreUninitialize();

	// 델리게이트 해제 + 어트리뷰트 셋 포인터 정리.
	ReleaseAttributeSet();

	if (CachedASC)
	{
		// ASC 해제 전에 차단 상태를 정리합니다.
		if (bAbilityBlocked)
		{
			CachedASC->UnBlockAbilitiesWithTags(FGameplayTagContainer(GetBlockedAbilityTag()));
			bAbilityBlocked = false;
		}
	}

	RemoveRegenEffect();
	CachedASC = nullptr;
}

void USHResourceComponent::HandleValueChanged(float OldValue, float NewValue)
{
	if (!CachedASC)
	{
		return;
	}

	const float MaxValue = GetMaxValue();
	const bool  bWasFull = (OldValue >= MaxValue);
	const bool  bIsFull  = (NewValue >= MaxValue);

	// -------------------------------------------------------
	// Regen GE 관리:
	//   가득 찼을 때 → GE 제거 (불필요한 Periodic 틱 차단)
	//   소비로 인해 가득 참 해제 → GE 재적용
	// -------------------------------------------------------
	if (bWasFull && !bIsFull && GetRegenEffect())
	{
		ApplyRegenEffect();
	}
	else if (!bWasFull && bIsFull)
	{
		RemoveRegenEffect();
	}

	// 자원이 회복돼 소진 상태에서 벗어났다면 태그를 제거합니다.
	if (bIsResourceDepleted && NewValue > 0.0f)
	{
		CachedASC->RemoveLooseGameplayTag(GetDepletedStatusTag());
		bIsResourceDepleted = false;
	}

	// -------------------------------------------------------
	// 어빌리티 차단/해제:
	//   값 < 코스트 임계값 → 차단
	//   값 ≥ 코스트 임계값 → 차단 해제
	// -------------------------------------------------------
	const bool bCanAfford = (NewValue >= GetCostThreshold());
	if (bAbilityBlocked && bCanAfford)
	{
		CachedASC->UnBlockAbilitiesWithTags(FGameplayTagContainer(GetBlockedAbilityTag()));
		bAbilityBlocked = false;
	}
	else if (!bAbilityBlocked && !bCanAfford)
	{
		CachedASC->BlockAbilitiesWithTags(FGameplayTagContainer(GetBlockedAbilityTag()));
		bAbilityBlocked = true;
	}

	BroadcastChange(NewValue, MaxValue);
}

void USHResourceComponent::HandleResourceDepleted()
{
	if (!CachedASC)
	{
		return;
	}

	// 소진 태그를 부여합니다. 자원을 소비하는 어빌리티는 이 태그를 검사해 발동을 막습니다.
	CachedASC->AddLooseGameplayTag(GetDepletedStatusTag());
	bIsResourceDepleted = true;

	// Regen GE가 없으면 자원은 영원히 0으로 유지됩니다.
	// Regen이 설정돼 있다면 소진 직후 회복을 시작합니다.
	if (GetRegenEffect() && !RegenEffectHandle.IsValid())
	{
		ApplyRegenEffect();
	}
}

void USHResourceComponent::ApplyRegenEffect()
{
	const TSubclassOf<UGameplayEffect> RegenEffect = GetRegenEffect();
	if (!CachedASC || !RegenEffect || RegenEffectHandle.IsValid())
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = CachedASC->MakeEffectContext();
	ContextHandle.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(
		RegenEffect, 1.0f, ContextHandle);

	if (SpecHandle.IsValid())
	{
		RegenEffectHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void USHResourceComponent::RemoveRegenEffect()
{
	if (CachedASC && RegenEffectHandle.IsValid())
	{
		CachedASC->RemoveActiveGameplayEffect(RegenEffectHandle);
	}

	RegenEffectHandle = FActiveGameplayEffectHandle();
}

float USHResourceComponent::GetValueNormalized() const
{
	const float MaxValue = GetMaxValue();
	return (MaxValue > 0.0f) ? (GetCurrentValue() / MaxValue) : 0.0f;
}
