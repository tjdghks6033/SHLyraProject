// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHMeleeAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NativeGameplayTags.h"

// 히트 판정 시점을 알리는 Gameplay Event 태그.
// 몽타주에 AnimNotify_GameplayEvent를 추가하고 이 태그를 지정해야 한다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_SH_Melee_HitDetect, "Event.SH.Melee.HitDetect");

// 근접 공격 쿨다운 태그.
// GE_SHMeleeCooldown 이 이 태그를 ASC에 부여하고,
// GA_SHMeleeAttack 의 Cooldown Tags 에 동일 태그를 지정해 쿨다운을 감지한다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_SH_Melee_Attack, "Cooldown.SH.Melee.Attack");

USHMeleeAttack::USHMeleeAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 입력 트리거(버튼 누름) 시점에 활성화
	ActivationPolicy = ELyraAbilityActivationPolicy::OnInputTriggered;
}

void USHMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 비용(스태미나) + 쿨다운 적용. 실패 시 즉시 종료.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!AttackMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("USHMeleeAttack: AttackMontage가 설정되지 않았습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 공격 몽타주 재생 태스크
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AttackMontage, 1.0f, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &USHMeleeAttack::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &USHMeleeAttack::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &USHMeleeAttack::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &USHMeleeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// 2. AnimNotify 발화 시점 대기 태스크 (AnimNotify_GameplayEvent → HitDetect 태그)
	//    OnlyTriggerOnce=false: 한 공격 내에서 여러 번 판정이 필요한 경우를 대비
	UAbilityTask_WaitGameplayEvent* WaitEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_Event_SH_Melee_HitDetect, nullptr, false, true);

	WaitEventTask->EventReceived.AddDynamic(this, &USHMeleeAttack::OnGameplayEventReceived);
	WaitEventTask->ReadyForActivation();
}

void USHMeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USHMeleeAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USHMeleeAttack::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USHMeleeAttack::OnGameplayEventReceived(FGameplayEventData Payload)
{
	// 히트 판정은 서버(Authority)에서만 실행한다.
	// 싱글플레이어/Listen Server 에서는 항상 true.
	if (HasAuthority(&CurrentActivationInfo))
	{
		PerformMeleeHit(CurrentActorInfo);
	}
}

void USHMeleeAttack::PerformMeleeHit(const FGameplayAbilityActorInfo* ActorInfo)
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

	// 캐릭터 중심 높이(60cm 오프셋)에서 전방으로 구체 트레이스
	const FVector TraceStart = AvatarActor->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	const FVector TraceEnd   = TraceStart + AvatarActor->GetActorForwardVector() * SphereTraceRange;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(
		AvatarActor,
		TraceStart,
		TraceEnd,
		SphereTraceRadius,
		ObjectTypes,
		false,                    // bTraceComplex
		TArray<AActor*>(),        // ActorsToIgnore (bIgnoreSelf 가 자신을 제외)
		EDrawDebugTrace::None,
		HitResults,
		true                      // bIgnoreSelf
	);

	// 한 번의 공격에서 같은 대상에게 중복 데미지를 방지
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

		// 어빌리티 컨텍스트(인스티게이터, 레벨 등)가 담긴 GE 스펙 생성 후 대상에 적용
		FGameplayEffectSpecHandle SpecHandle =
			MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());

		if (SpecHandle.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(
				*SpecHandle.Data.Get(), TargetASC);
		}

		DamagedActors.Add(HitActor);
	}
}
