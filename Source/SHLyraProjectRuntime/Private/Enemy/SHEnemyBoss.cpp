// Copyright SH. All Rights Reserved.

#include "Enemy/SHEnemyBoss.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SH_Message_Boss_Engaged, "SH.Message.Boss.Engaged");

ASHEnemyBoss::ASHEnemyBoss(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASHEnemyBoss::BeginPlay()
{
	Super::BeginPlay();

	UGameplayMessageSubsystem& MsgSys = UGameplayMessageSubsystem::Get(this);
	FSHBossMessage Payload;
	Payload.BossActor = this;
	MsgSys.BroadcastMessage(TAG_SH_Message_Boss_Engaged, Payload);
}

void ASHEnemyBoss::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : InitGameplayEffects)
	{
		if (EffectClass)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			ASC->ApplyGameplayEffectToSelf(EffectClass->GetDefaultObject<UGameplayEffect>(), 1.f, Context);
		}
	}
}
