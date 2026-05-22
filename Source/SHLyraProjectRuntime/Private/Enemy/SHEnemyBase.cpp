// Copyright SH. All Rights Reserved.

#include "Enemy/SHEnemyBase.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_Frozen,    "Status.SH.Frozen");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_Launched,  "Status.SH.Launched");

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
	OriginalGravityScale  = GetCharacterMovement()->GravityScale;

	// 빙결 상태 태그 변경 감지 — 이동/BT 제어
	ASC->RegisterGameplayTagEvent(TAG_Status_SH_Frozen, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ASHEnemyBase::OnFrozenTagChanged);

	// 공중 발사 상태 태그 변경 감지 — 중력 제어/BT 제어
	ASC->RegisterGameplayTagEvent(TAG_Status_SH_Launched, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ASHEnemyBase::OnLaunchedTagChanged);
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

void ASHEnemyBase::OnLaunchedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();

	if (NewCount > 0)
	{
		// MOVE_Flying: 중력 없이 현재 위치에 정지. GravityScale 조작보다 신뢰성 높음.
		CMC->SetMovementMode(MOVE_Flying);
		CMC->Velocity       = FVector::ZeroVector;
		CMC->MaxWalkSpeed   = 0.f;

		if (AAIController* AIC = GetController<AAIController>())
		{
			if (UBehaviorTreeComponent* BTC = AIC->FindComponentByClass<UBehaviorTreeComponent>())
			{
				BTC->PauseLogic(TEXT("Launched"));
			}
		}
	}
	else
	{
		CMC->SetMovementMode(MOVE_Walking);
		CMC->MaxWalkSpeed = OriginalMaxWalkSpeed;

		if (AAIController* AIC = GetController<AAIController>())
		{
			if (UBehaviorTreeComponent* BTC = AIC->FindComponentByClass<UBehaviorTreeComponent>())
			{
				BTC->ResumeLogic(TEXT("Launched"));
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
