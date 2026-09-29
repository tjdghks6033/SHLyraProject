// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHMeleeAttackBase.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_Parrying, "Status.SH.Parrying");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_Stunned, "Status.SH.Stunned");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_SH_Parry_Success, "Event.SH.Parry.Success");

void USHMeleeAttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 비용/쿨다운 적용. 실패 시 즉시 종료.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!AttackMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: AttackMontage가 설정되지 않았습니다."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AttackMontage, 1.0f, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &USHMeleeAttackBase::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &USHMeleeAttackBase::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &USHMeleeAttackBase::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &USHMeleeAttackBase::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// AnimNotify_GameplayEvent가 HitDetect 태그를 발화할 때까지 대기.
	// OnlyTriggerOnce=false: 한 공격 내에서 여러 번 판정이 필요한 경우를 대비
	UAbilityTask_WaitGameplayEvent* WaitEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GetHitDetectTag(), nullptr, false, true);

	WaitEventTask->EventReceived.AddDynamic(this, &USHMeleeAttackBase::OnGameplayEventReceived);
	WaitEventTask->ReadyForActivation();
}

void USHMeleeAttackBase::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USHMeleeAttackBase::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USHMeleeAttackBase::OnGameplayEventReceived(FGameplayEventData Payload)
{
	// 히트 판정은 서버(Authority)에서만 실행한다.
	// 싱글플레이어/Listen Server 에서는 항상 true.
	if (HasAuthority(&CurrentActivationInfo))
	{
		PerformHit(CurrentActorInfo);
	}
}

void USHMeleeAttackBase::PerformHit(const FGameplayAbilityActorInfo* ActorInfo)
{
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor || !DamageEffect)
	{
		return;
	}

	UAbilitySystemComponent* InstigatorASC = ActorInfo->AbilitySystemComponent.Get();
	if (!InstigatorASC)
	{
		return;
	}

	// 판정 볼륨(시작/끝/반지름)은 파생이 계산한다.
	FVector TraceStart;
	FVector TraceEnd;
	float   TraceRadius = 0.f;
	if (!ComputeHitTrace(ActorInfo, TraceStart, TraceEnd, TraceRadius))
	{
		return;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	const EDrawDebugTrace::Type DebugType = bDrawDebugTrace
		? EDrawDebugTrace::ForDuration
		: EDrawDebugTrace::None;

	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(
		AvatarActor,
		TraceStart,
		TraceEnd,
		TraceRadius,
		ObjectTypes,
		false,               // bTraceComplex
		TArray<AActor*>(),   // ActorsToIgnore (bIgnoreSelf 가 자신을 제외)
		DebugType,
		HitResults,
		true                 // bIgnoreSelf
	);

	// 한 번의 공격에서 같은 대상에게 중복 데미지 방지
	TSet<AActor*> DamagedActors;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

		if (!TargetASC)
		{
			continue;
		}

		if (TryHandleParry(HitActor, InstigatorASC))
		{
			// A successful parry consumes the whole attack swing. No damage,
			// knockback, or ordinary hit-stop is applied to any target.
			return;
		}

		// 데미지 적용. HitResult를 넘겨 컨텍스트에 히트 정보를 싣는다.
		// 빙결 등 상태이상에 따른 배율은 USHDamageExecution이 처리한다.
		ApplyEffectToTarget(DamageEffect, InstigatorASC, TargetASC, &Hit);

		// 넉백 — 공격자→피격자 수평 방향으로 발사
		if (KnockbackStrength > 0.f)
		{
			if (ACharacter* HitChar = Cast<ACharacter>(HitActor))
			{
				const FVector Dir = (HitActor->GetActorLocation() - AvatarActor->GetActorLocation()).GetSafeNormal2D();
				HitChar->LaunchCharacter(Dir * KnockbackStrength + FVector(0.f, 0.f, KnockbackZStrength), true, true);
			}
		}

		DamagedActors.Add(HitActor);
	}

	// HitStop — 히트 성공 시에만 발동. FTimerManager는 실시간 기준이라 TimeDilation 보정 불필요.
	if (!DamagedActors.IsEmpty() && HitStopTimeDilation > 0.f)
	{
		UGameplayStatics::SetGlobalTimeDilation(AvatarActor, HitStopTimeDilation);

		FTimerHandle HitStopTimer;
		AvatarActor->GetWorldTimerManager().SetTimer(HitStopTimer,
			FTimerDelegate::CreateWeakLambda(AvatarActor,
				[AvatarActor]()
				{
					UGameplayStatics::SetGlobalTimeDilation(AvatarActor, 1.0f);
				}),
			HitStopDuration, false);
	}
}

bool USHMeleeAttackBase::TryHandleParry(AActor* HitActor, UAbilitySystemComponent* InstigatorASC)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	AActor* Attacker = GetAvatarActorFromActorInfo();
	if (!TargetASC || !InstigatorASC || !Attacker ||
		!TargetASC->HasMatchingGameplayTag(TAG_Status_SH_Parrying))
	{
		return false;
	}

	// End the attack ability itself before starting the reaction. Stopping only
	// the montage leaves the montage task and movement lock alive.
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);

	FGameplayEventData EventData;
	EventData.EventTag = TAG_Event_SH_Parry_Success;
	EventData.Instigator = Attacker;
	EventData.Target = HitActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		HitActor, TAG_Event_SH_Parry_Success, EventData);

	InstigatorASC->AddLooseGameplayTag(TAG_Status_SH_Stunned);
	if (ACharacter* AttackingCharacter = Cast<ACharacter>(Attacker))
	{
		FVector RecoilDirection = Attacker->GetActorLocation() - HitActor->GetActorLocation();
		RecoilDirection.Z = 0.0f;
		RecoilDirection = RecoilDirection.GetSafeNormal();
		AttackingCharacter->LaunchCharacter(
			RecoilDirection * ParryRecoilStrength,
			true,
			false);
	}
	const TWeakObjectPtr<UAbilitySystemComponent> WeakInstigatorASC = InstigatorASC;
	FTimerHandle StunTimer;
	Attacker->GetWorldTimerManager().SetTimer(
		StunTimer,
		FTimerDelegate::CreateWeakLambda(Attacker,
			[WeakInstigatorASC]()
			{
				if (UAbilitySystemComponent* ASC = WeakInstigatorASC.Get())
				{
					ASC->RemoveLooseGameplayTag(TAG_Status_SH_Stunned);
				}
			}),
		ParryStunDuration,
		false);

	return true;
}
