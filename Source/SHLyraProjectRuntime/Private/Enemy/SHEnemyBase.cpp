// Copyright SH. All Rights Reserved.

#include "Enemy/SHEnemyBase.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_Frozen,      "Status.SH.Frozen");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_Launched,    "Status.SH.Launched");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_KnockedDown, "Status.SH.KnockedDown");

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

	// 넉다운 상태 태그 변경 감지 — 넉다운 몽타주/BT 제어
	ASC->RegisterGameplayTagEvent(TAG_Status_SH_KnockedDown, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ASHEnemyBase::OnKnockedDownTagChanged);

	// Lyra 데미지 메시지 구독 — 피격 시 HitReactMontage 재생 (상태 조건부)
	UGameplayMessageSubsystem& MsgSub = UGameplayMessageSubsystem::Get(this);
	DamageMessageHandle = MsgSub.RegisterListener(TAG_Lyra_Damage_Message, this, &ASHEnemyBase::OnDamageMessageReceived);
}

void ASHEnemyBase::OnFrozenTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		GetCharacterMovement()->MaxWalkSpeed = 0.f;
		CustomTimeDilation = 0.f;

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
		CustomTimeDilation = 1.f;

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

		if (LaunchReactionMontage)
		{
			const float Duration = PlayAnimMontage(LaunchReactionMontage);
			if (Duration > 0.f && LaunchLoopMontage)
			{
				GetWorldTimerManager().SetTimer(
					LaunchReactionTimerHandle,
					this,
					&ASHEnemyBase::OnLaunchReactionFinished,
					Duration,
					false);
			}
		}

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
		GetWorldTimerManager().ClearTimer(LaunchReactionTimerHandle);

		UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
		if (AnimInst)
		{
			if (LaunchReactionMontage) { AnimInst->Montage_Stop(0.25f, LaunchReactionMontage); }
			if (LaunchLoopMontage)     { AnimInst->Montage_Stop(0.25f, LaunchLoopMontage); }
		}

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

void ASHEnemyBase::OnKnockedDownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		if (KnockedDownMontage)
		{
			// PlayAnimMontage 반환값 = 실제 재생 시간(초). 이 시간 후에 GE를 제거한다.
			const float MontageDuration = PlayAnimMontage(KnockedDownMontage);
			if (MontageDuration > 0.f)
			{
				GetWorldTimerManager().SetTimer(
					KnockedDownTimerHandle,
					this,
					&ASHEnemyBase::RemoveKnockedDownEffect,
					MontageDuration,
					false);
			}
		}

		if (AAIController* AIC = GetController<AAIController>())
		{
			if (UBehaviorTreeComponent* BTC = AIC->FindComponentByClass<UBehaviorTreeComponent>())
			{
				BTC->PauseLogic(TEXT("KnockedDown"));
			}
		}
	}
	else
	{
		GetWorldTimerManager().ClearTimer(KnockedDownTimerHandle);

		if (AAIController* AIC = GetController<AAIController>())
		{
			if (UBehaviorTreeComponent* BTC = AIC->FindComponentByClass<UBehaviorTreeComponent>())
			{
				BTC->ResumeLogic(TEXT("KnockedDown"));
			}
		}
	}
}

void ASHEnemyBase::RemoveKnockedDownEffect()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_Status_SH_KnockedDown));
	}
}

void ASHEnemyBase::OnLaunchReactionFinished()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !ASC->HasMatchingGameplayTag(TAG_Status_SH_Launched) || !LaunchLoopMontage)
	{
		return;
	}

	UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInst)
	{
		return;
	}

	AnimInst->Montage_Play(LaunchLoopMontage);
	const FName Section = LaunchLoopMontage->GetSectionName(0);
	AnimInst->Montage_SetNextSection(Section, Section, LaunchLoopMontage);
}

void ASHEnemyBase::OnDamageMessageReceived(FGameplayTag Channel, const FLyraVerbMessage& Payload)
{
	if (Payload.Target != this)
	{
		return;
	}

	if (!HitReactMontage)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// 공중 발사, 넉다운, 빙결 상태 중엔 전용 몽타주가 이미 재생 중이거나 TimeDilation=0 — 충돌 방지
	if (ASC->HasMatchingGameplayTag(TAG_Status_SH_Launched) ||
		ASC->HasMatchingGameplayTag(TAG_Status_SH_KnockedDown) ||
		ASC->HasMatchingGameplayTag(TAG_Status_SH_Frozen))
	{
		return;
	}

	PlayAnimMontage(HitReactMontage);
}
