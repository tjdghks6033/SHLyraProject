// Copyright SH. All Rights Reserved.

#include "Enemy/SHEnemyBase.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_Frozen, "Status.SH.Frozen");

ASHEnemyBase::ASHEnemyBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASHEnemyBase::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	OriginalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// 빙결 상태 태그 변경 감지 — 이동/BT 제어
	ASC->RegisterGameplayTagEvent(TAG_Status_SH_Frozen, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ASHEnemyBase::OnFrozenTagChanged);
}

void ASHEnemyBase::OnFrozenTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		GetCharacterMovement()->MaxWalkSpeed = 0.f;

		if (AAIController* AIC = GetController<AAIController>())
		{
			if (UBehaviorTreeComponent* BTC = AIC->FindComponentByClass<UBehaviorTreeComponent>())
			{
				BTC->PauseLogic(TEXT("Frozen"));
			}
		}
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;

		if (AAIController* AIC = GetController<AAIController>())
		{
			if (UBehaviorTreeComponent* BTC = AIC->FindComponentByClass<UBehaviorTreeComponent>())
			{
				BTC->ResumeLogic(TEXT("Frozen"));
			}
		}
	}
}

void ASHEnemyBase::OnDeathFinished(AActor* OwningActor)
{
	// ALyraCharacter 기본 구현은 SetTimerForNextTick → DestroyDueToDeath로 즉시 파괴한다.
	// 적은 시체 연출(애니메이션, VFX) 시간을 확보하기 위해 DestroyDelay만큼 지연한다.
	// 이동/충돌 비활성화는 OnDeathStarted에서 부모가 이미 처리했으므로 여기서 중복하지 않는다.

	GetWorldTimerManager().SetTimer(
		DestroyTimerHandle,
		this,
		&ASHEnemyBase::HandleDestroyAfterDelay,
		DestroyDelay,
		false);
}

void ASHEnemyBase::HandleDestroyAfterDelay()
{
	// Lyra 정식 파괴 경로 — K2_OnDeathFinished(BP 이벤트) 발화 + UninitAndDestroy(ASC uninit + Destroy) 실행.
	DestroyDueToDeath();
}
