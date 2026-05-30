// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHAerialComboAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/LyraCameraMode.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 타겟은 여기서 찾지 않는다. 런치 선동작을 먼저 재생하고,
	// 그 안의 타격 프레임(OnLaunchEventReceived)에서 정면 타겟을 확정한다.
	// 눈앞에 적이 없으면 헛스윙 — 선동작만 재생되고 종료된다.

	if (UltimateCameraMode)
	{
		SetCameraMode(UltimateCameraMode);
	}

	// Hit 리스너를 어빌리티 시작 시점에 등록 — 런치/공중 콤보 어느 단계에서든 수신
	UAbilityTask_WaitGameplayEvent* HitEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_Event_AerialCombo_Hit, nullptr, false, true);
	HitEventTask->EventReceived.AddDynamic(this, &USHAerialComboAbility::OnAerialHitEventReceived);
	HitEventTask->ReadyForActivation();

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
		// 선동작이 없으면 타격 프레임도 없으므로, 여기서 타겟을 확정하고 다음 단계로 넘긴다.
		TargetCharacter = FindTargetInFront();
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
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	// 런치 선동작의 타격 프레임에서 정면 타겟을 확정한다.
	// 눈앞에 적이 없으면 헛스윙 — 보스 처리 없이 종료하고, 몽타주가 끝나면
	// StartTeleportPhase가 타겟 null을 보고 EndAbility 한다.
	TargetCharacter = FindTargetInFront();
	if (!TargetCharacter.IsValid())
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
	FVector TeleportLocation = BossLocation - DirectionToBoss * TeleportStandoffDistance;
	TeleportLocation.Z += TeleportZOffset;

	// 텔레포트 VFX (출발지에서 Execute)
	UAbilitySystemComponent* InstigatorASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (InstigatorASC && TeleportCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location     = PlayerLocation;
		CueParams.EffectCauser = AvatarActor; // DashTrail 등 Attach용 참조
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

	// 히트 VFX — 보스 위치 기준 Execute
	if (InstigatorASC && HitCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = TargetCharacter->GetActorLocation();
		InstigatorASC->ExecuteGameplayCue(HitCueTag, CueParams);
	}
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

	// 넉다운 GE → Status.SH.KnockedDown → OnKnockedDownTagChanged → 바닥 구르기 몽타주 재생
	ApplyEffectToTarget(KnockedDownEffect, InstigatorASC, TargetASC);

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

ACharacter* USHAerialComboAbility::FindTargetInFront() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return nullptr;
	}

	// 플레이어 정면으로 구체 스윕 — 눈앞에 적이 없으면 nullptr 반환(→ 발동 취소).
	const FVector Start = AvatarActor->GetActorLocation();
	const FVector End   = Start + AvatarActor->GetActorForwardVector() * TargetSearchRange;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<FHitResult> Hits;
	UKismetSystemLibrary::SphereTraceMultiForObjects(
		AvatarActor, Start, End, TargetSearchRadius, ObjectTypes,
		false, TArray<AActor*>(), EDrawDebugTrace::None, Hits, /*bIgnoreSelf=*/true);

	// 스윕에 걸린 것 중 가장 가까운 ASC 보유 캐릭터를 타겟으로 선택.
	// ASC 없는 폰(소품 등)이 더 가까이 껴 있어도 건너뛰고 실제 적을 고른다.
	ACharacter* Target      = nullptr;
	float       ClosestDist = FLT_MAX;

	for (const FHitResult& Hit : Hits)
	{
		ACharacter* HitChar = Cast<ACharacter>(Hit.GetActor());
		if (!HitChar || !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitChar))
		{
			continue;
		}

		const float Dist = FVector::Dist(Start, HitChar->GetActorLocation());
		if (Dist < ClosestDist)
		{
			ClosestDist = Dist;
			Target      = HitChar;
		}
	}

	return Target;
}
