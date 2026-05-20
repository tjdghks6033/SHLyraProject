// Copyright SH. All Rights Reserved.

#include "Character/SHPlayerDeathComponent.h"

#include "Character/LyraPawnExtensionComponent.h"
#include "Character/LyraHealthComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHPlayerDeathComponent)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Message_Player_Died, "SH.Message.Player.Died");

USHPlayerDeathComponent::USHPlayerDeathComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(false);
}

void USHPlayerDeathComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
	{
		PawnExtComp->OnAbilitySystemInitialized_RegisterAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	}
}

void USHPlayerDeathComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(GetOwner()))
	{
		HealthComp->OnDeathStarted.RemoveDynamic(this, &ThisClass::OnDeathStarted);
	}

	Super::EndPlay(EndPlayReason);
}

void USHPlayerDeathComponent::OnAbilitySystemInitialized()
{
	ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(GetOwner());
	if (!HealthComp)
	{
		return;
	}

	HealthComp->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
}

void USHPlayerDeathComponent::OnDeathStarted(AActor* OwningActor)
{
	// PlayerController로 제어되는 폰만 처리 — AIController(봇/보스)는 무시
	// IsLocallyControlled()는 스탠드얼론 PIE에서 AI폰도 true를 반환하므로 사용 불가
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Cast<APlayerController>(Pawn->GetController()))
	{
		return;
	}

	UGameplayMessageSubsystem& MsgSys = UGameplayMessageSubsystem::Get(this);
	FSHPlayerDiedMessage Payload;
	Payload.Player = OwningActor;
	MsgSys.BroadcastMessage(TAG_SH_Message_Player_Died, Payload);
}
