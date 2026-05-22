// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHAerialComboAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_AerialCombo_Launch, "Event.SH.AerialCombo.Launch");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_AerialCombo_Hit,    "Event.SH.AerialCombo.Hit");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_AerialCombo_Slam,   "Event.SH.AerialCombo.Slam");

USHAerialComboAbility::USHAerialComboAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy      = ELyraAbilityActivationPolicy::OnInputTriggered;
	bLockMovementOnActivate = true;
	LockedWalkSpeed         = 0.f;
}

void USHAerialComboAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("[AerialCombo] ActivateAbility called"));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AerialCombo] CommitAbility FAILED"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetCharacter = FindNearestEnemy();
	if (!TargetCharacter.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AerialCombo] No target found"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AerialCombo] Target found: %s"), *TargetCharacter->GetName());

	StartLaunchPhase();
}

void USHAerialComboAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 어빌리티 종료 시 플레이어 이동 모드 복원 (Flying 중에 끊겼을 경우 대비)
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		if (ACharacter* PlayerChar = Cast<ACharacter>(AvatarActor))
		{
			UCharacterMovementComponent* CMC = PlayerChar->GetCharacterMovement();
			if (CMC->MovementMode == MOVE_Flying)
			{
				CMC->SetMovementMode(MOVE_Falling);
			}
		}
	}

	// 슬램 없이 어빌리티가 취소된 경우 Launched GE가 남아 있을 수 있으므로 정리
	if (TargetCharacter.IsValid() && LaunchedStatusHandle.IsValid())
	{
		if (UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter.Get()))
		{
			TargetASC->RemoveActiveGameplayEffect(LaunchedStatusHandle);
		}
	}
	LaunchedStatusHandle.Invalidate();

	TargetCharacter.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 1: 런치
// ─────────────────────────────────────────────────────────────────────────────

void USHAerialComboAbility::StartLaunchPhase()
{
	if (!LaunchMontage)
	{
		StartTeleportPhase();
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, LaunchMontage, 1.0f, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &USHAerialComboAbility::OnLaunchMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &USHAerialComboAbility::OnLaunchMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &USHAerialComboAbility::OnLaunchMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &USHAerialComboAbility::OnLaunchMontageCancelled);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* EventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_Event_AerialCombo_Launch, nullptr, true, true);

	EventTask->EventReceived.AddDynamic(this, &USHAerialComboAbility::OnLaunchEventReceived);
	EventTask->ReadyForActivation();
}

void USHAerialComboAbility::OnLaunchEventReceived(FGameplayEventData Payload)
{
	if (!HasAuthority(&CurrentActivationInfo) || !TargetCharacter.IsValid())
	{
		return;
	}

	// 보스를 공중 위치로 순간 이동 (LaunchCharacter + 즉시 Velocity 제거 대신 직접 배치)
	const FVector AerialPosition = TargetCharacter->GetActorLocation() + FVector(0.f, 0.f, LaunchZForce);
	TargetCharacter->SetActorLocation(AerialPosition, false, nullptr, ETeleportType::TeleportPhysics);

	// GE_SHLaunchedStatus(Infinite) 적용 → Status.SH.Launched 태그 → OnLaunchedTagChanged 발화
	// 핸들을 저장해 두고, Slam 이벤트 또는 EndAbility 시 명시적으로 제거한다
	UAbilitySystemComponent* InstigatorASC = CurrentActorInfo->AbilitySystemComponent.Get();
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter.Get());

	LaunchedStatusHandle = ApplyEffectToTarget(LaunchedStatusEffect, InstigatorASC, TargetASC);

	PlayCameraShake(LaunchShake);
}

void USHAerialComboAbility::OnLaunchMontageCompleted()
{
	StartTeleportPhase();
}

void USHAerialComboAbility::OnLaunchMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 2: 텔레포트
// ─────────────────────────────────────────────────────────────────────────────

void USHAerialComboAbility::StartTeleportPhase()
{
	if (!TargetCharacter.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 보스 위치 기준으로 플레이어가 접근하던 방향의 반대에 서도록 오프셋 계산
	const FVector BossLocation  = TargetCharacter->GetActorLocation();
	const FVector PlayerLocation = AvatarActor->GetActorLocation();
	const FVector DirectionToBoss = (BossLocation - PlayerLocation).GetSafeNormal2D();
	const FVector TeleportLocation = BossLocation - DirectionToBoss * TeleportStandoffDistance;

	// 텔레포트 VFX (출발지에서 Execute)
	UAbilitySystemComponent* InstigatorASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (InstigatorASC && TeleportCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = PlayerLocation;
		InstigatorASC->ExecuteGameplayCue(TeleportCueTag, CueParams);
	}

	AvatarActor->TeleportTo(TeleportLocation, AvatarActor->GetActorRotation(), false, true);

	// 텔레포트 후 보스 방향으로 회전 + 공중 유지 (MOVE_Flying)
	if (ACharacter* PlayerChar = Cast<ACharacter>(AvatarActor))
	{
		FRotator LookAtRot = (BossLocation - TeleportLocation).Rotation();
		LookAtRot.Pitch = 0.f;
		LookAtRot.Roll  = 0.f;
		PlayerChar->SetActorRotation(LookAtRot);

		// 공중 콤보 구간 동안 플레이어가 낙하하지 않도록 Flying 모드로 전환
		PlayerChar->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		PlayerChar->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}

	PlayCameraShake(TeleportShake);

	StartAerialComboPhase();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: 공중 콤보
// ─────────────────────────────────────────────────────────────────────────────

void USHAerialComboAbility::StartAerialComboPhase()
{
	if (!AerialComboMontage)
	{
		StartGroundSlamPhase();
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AerialComboMontage, 1.0f, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &USHAerialComboAbility::OnAerialMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &USHAerialComboAbility::OnAerialMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &USHAerialComboAbility::OnAerialMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &USHAerialComboAbility::OnAerialMontageCancelled);
	MontageTask->ReadyForActivation();

	// OnlyTriggerOnce=false — 몽타주 내 히트 Notify 3개 모두 수신
	UAbilityTask_WaitGameplayEvent* EventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_Event_AerialCombo_Hit, nullptr, false, true);

	EventTask->EventReceived.AddDynamic(this, &USHAerialComboAbility::OnAerialHitEventReceived);
	EventTask->ReadyForActivation();
}

void USHAerialComboAbility::OnAerialHitEventReceived(FGameplayEventData Payload)
{
	if (!HasAuthority(&CurrentActivationInfo) || !TargetCharacter.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* InstigatorASC = CurrentActorInfo->AbilitySystemComponent.Get();
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter.Get());

	ApplyEffectToTarget(AerialHitDamageEffect, InstigatorASC, TargetASC);
	PlayCameraShake(AerialHitShake);
}

void USHAerialComboAbility::OnAerialMontageCompleted()
{
	StartGroundSlamPhase();
}

void USHAerialComboAbility::OnAerialMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4: 그라운드 슬램
// ─────────────────────────────────────────────────────────────────────────────

void USHAerialComboAbility::StartGroundSlamPhase()
{
	if (!GroundSlamMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, GroundSlamMontage, 1.0f, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &USHAerialComboAbility::OnSlamMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &USHAerialComboAbility::OnSlamMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &USHAerialComboAbility::OnSlamMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &USHAerialComboAbility::OnSlamMontageCancelled);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* EventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_Event_AerialCombo_Slam, nullptr, true, true);

	EventTask->EventReceived.AddDynamic(this, &USHAerialComboAbility::OnSlamEventReceived);
	EventTask->ReadyForActivation();
}

void USHAerialComboAbility::OnSlamEventReceived(FGameplayEventData Payload)
{
	if (!HasAuthority(&CurrentActivationInfo) || !TargetCharacter.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* InstigatorASC = CurrentActorInfo->AbilitySystemComponent.Get();
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter.Get());

	// Launched GE 먼저 제거 → OnLaunchedTagChanged(0) → MOVE_Walking 복원
	// MOVE_Flying 상태에서는 LaunchCharacter가 동작하지 않으므로 반드시 선행해야 한다
	if (TargetASC && LaunchedStatusHandle.IsValid())
	{
		TargetASC->RemoveActiveGameplayEffect(LaunchedStatusHandle);
		LaunchedStatusHandle.Invalidate();
	}

	// 보스를 땅으로 내리꽂는다 (MOVE_Walking 복원 후라서 정상 동작)
	TargetCharacter->LaunchCharacter(FVector(0.f, 0.f, -SlamZForce), false, true);

	ApplyEffectToTarget(SlamDamageEffect, InstigatorASC, TargetASC);

	// 슬램 착지 VFX
	if (InstigatorASC && SlamCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = TargetCharacter->GetActorLocation();
		InstigatorASC->ExecuteGameplayCue(SlamCueTag, CueParams);
	}

	// 방사형 카메라 셰이크 — 슬램 위치 기준
	if (GroundSlamShake)
	{
		UGameplayStatics::PlayWorldCameraShake(
			GetWorld(),
			GroundSlamShake,
			TargetCharacter->GetActorLocation(),
			0.f,
			SlamShakeRadius,
			1.f);
	}
}

void USHAerialComboAbility::OnSlamMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USHAerialComboAbility::OnSlamMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

// ─────────────────────────────────────────────────────────────────────────────
// 헬퍼
// ─────────────────────────────────────────────────────────────────────────────

ACharacter* USHAerialComboAbility::FindNearestEnemy() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return nullptr;
	}

	// 범위 제한 없이 월드 전체에서 검색 — 궁극기는 어디서든 발동 후 텔레포트로 이동
	TArray<AActor*> AllCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllCharacters);

	ACharacter* Nearest     = nullptr;
	float       NearestDist = FLT_MAX;

	for (AActor* Actor : AllCharacters)
	{
		if (Actor == AvatarActor)
		{
			continue;
		}
		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
		{
			continue;
		}

		const float Dist = FVector::Dist(AvatarActor->GetActorLocation(), Actor->GetActorLocation());
		if (Dist < NearestDist)
		{
			NearestDist = Dist;
			Nearest     = Cast<ACharacter>(Actor);
		}
	}

	return Nearest;
}

void USHAerialComboAbility::PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale) const
{
	if (!ShakeClass)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(
		GetActorInfo().PlayerController.Get()))
	{
		PC->ClientStartCameraShake(ShakeClass, Scale);
	}
}

FActiveGameplayEffectHandle USHAerialComboAbility::ApplyEffectToTarget(TSubclassOf<UGameplayEffect> EffectClass,
	UAbilitySystemComponent* InstigatorASC,
	UAbilitySystemComponent* TargetASC) const
{
	if (!EffectClass || !InstigatorASC || !TargetASC)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = InstigatorASC->MakeEffectContext();
	FGameplayEffectSpecHandle    Spec    =
		InstigatorASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), Context);

	if (Spec.IsValid())
	{
		return InstigatorASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}

	return FActiveGameplayEffectHandle();
}
