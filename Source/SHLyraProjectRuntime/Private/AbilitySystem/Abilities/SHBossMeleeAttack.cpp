// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHBossMeleeAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NativeGameplayTags.h"

// 보스 근접 히트 판정 시점 태그. 몽타주에 AnimNotify_GameplayEvent 로 추가한다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_SH_Boss_HitDetect, "Event.SH.Boss.HitDetect");

USHBossMeleeAttack::USHBossMeleeAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 공격 중 이동 고정 — 보스가 선입 동작 도중 미끄러지지 않도록.
	bLockMovementOnActivate = true;
}

void USHBossMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!AttackMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("USHBossMeleeAttack: AttackMontage가 설정되지 않았습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AttackMontage, 1.0f, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &USHBossMeleeAttack::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &USHBossMeleeAttack::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &USHBossMeleeAttack::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &USHBossMeleeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_Event_SH_Boss_HitDetect, nullptr, false, true);

	WaitEventTask->EventReceived.AddDynamic(this, &USHBossMeleeAttack::OnGameplayEventReceived);
	WaitEventTask->ReadyForActivation();
}

void USHBossMeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USHBossMeleeAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USHBossMeleeAttack::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USHBossMeleeAttack::OnGameplayEventReceived(FGameplayEventData Payload)
{
	if (HasAuthority(&CurrentActivationInfo))
	{
		PerformBossHit(CurrentActorInfo);
	}
}

void USHBossMeleeAttack::PerformBossHit(const FGameplayAbilityActorInfo* ActorInfo)
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

	// 보스 메인 스켈레탈 메시의 AttackSocket(hand_r) 위치를 판정 중심으로 사용.
	// hand_r 소켓은 보스 무기가 부착된 위치이므로 공격 범위와 자연스럽게 일치한다.
	FVector TraceOrigin;

	ACharacter* BossCharacter = Cast<ACharacter>(AvatarActor);
	USkeletalMeshComponent* SkelMesh = BossCharacter ? BossCharacter->GetMesh() : nullptr;

	if (SkelMesh && SkelMesh->DoesSocketExist(AttackSocketName))
	{
		TraceOrigin = SkelMesh->GetSocketLocation(AttackSocketName);
	}
	else
	{
		// 폴백: 캐릭터 중심(60cm 높이)에서 전방 FallbackRange 절반 거리
		TraceOrigin = AvatarActor->GetActorLocation()
			+ FVector(0.f, 0.f, 60.f)
			+ AvatarActor->GetActorForwardVector() * (FallbackRange * 0.5f);
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	const EDrawDebugTrace::Type DebugType = bDrawDebugTrace
		? EDrawDebugTrace::ForDuration
		: EDrawDebugTrace::None;

	// Start == End: 단일 위치의 구체 오버랩
	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::SphereTraceMultiForObjects(
		AvatarActor,
		TraceOrigin,
		TraceOrigin,
		HitRadius,
		ObjectTypes,
		false,
		TArray<AActor*>(),
		DebugType,
		HitResults,
		true           // bIgnoreSelf
	);

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

		FGameplayEffectContextHandle Context = InstigatorASC->MakeEffectContext();
		Context.AddHitResult(Hit);
		FGameplayEffectSpecHandle SpecHandle =
			InstigatorASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), Context);

		if (SpecHandle.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}

		DamagedActors.Add(HitActor);
	}
}
