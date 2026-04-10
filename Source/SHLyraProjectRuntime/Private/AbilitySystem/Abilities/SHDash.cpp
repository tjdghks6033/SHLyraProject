// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHDash.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "NativeGameplayTags.h"

// USHStaminaComponent 가 대쉬 어빌리티를 식별하는 데 사용하는 태그.
// BlockAbilitiesWithTags / AbilityActivatedCallbacks 모두 이 태그를 기준으로 동작합니다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Type_Action_Dash, "Ability.Type.Action.Dash");

USHDash::USHDash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ELyraAbilityActivationPolicy::OnInputTriggered;

	// AbilityTags 에 Dash 태그를 등록해 USHStaminaComponent 와 연동합니다.
	// - BlockAbilitiesWithTags: 스태미나 부족 시 이 태그로 발동 차단
	// - AbilityActivatedCallbacks: 활성화 감지 후 스태미나 차감 GE 적용
	AbilityTags.AddTag(TAG_Ability_Type_Action_Dash);
}

void USHDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 쿨다운 적용. 실패 시 즉시 종료.
	// 스태미나 비용은 USHStaminaComponent 가 AbilityActivatedCallbacks 로 처리하므로
	// 이 어빌리티에는 Cost GE 를 설정하지 않습니다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const ACharacter* Character = ActorInfo
		? Cast<ACharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;

	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 입력 방향에 따라 대쉬 월드 방향과 몽타주를 결정합니다.
	FVector DashDirection;
	UAnimMontage* SelectedMontage = nullptr;
	ResolveDashDirection(Character, DashDirection, SelectedMontage);

	// UAbilityTask_ApplyRootMotionConstantForce:
	//   FRootMotionSource_ConstantForce 기반 — CMC 예측과 호환되어 멀티플레이에서 안전합니다.
	//   LaunchCharacter / AddImpulse 와 달리 클라이언트 예측 데이터에 포함됩니다.
	UAbilityTask_ApplyRootMotionConstantForce* DashTask =
		UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this,
			NAME_None,
			DashDirection,
			DashStrength,
			DashDuration,
			false,                                       // bIsAdditive: false → 기존 속도 대체
			nullptr,                                     // StrengthOverTime curve: 없음 (일정 힘)
			ERootMotionFinishVelocityMode::ClampVelocity,
			FVector::ZeroVector,                         // SetVelocityOnFinish: ClampVelocity 모드에서 미사용
			DashFinishClampVelocity,                     // 종료 후 속도 클램프
			false                                        // bEnableGravity: 대쉬 중 중력 비활성화
		);

	DashTask->OnFinish.AddDynamic(this, &USHDash::OnDashFinished);
	DashTask->ReadyForActivation();

	if (SelectedMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, SelectedMontage, 1.0f, NAME_None, true);
		MontageTask->ReadyForActivation();
	}
}

void USHDash::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USHDash::OnDashFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USHDash::ResolveDashDirection(const ACharacter* Character,
	FVector& OutWorldDirection, UAnimMontage*& OutMontage) const
{
	// 이동 입력이 없으면 전방 대쉬로 폴백합니다.
	const FVector LastInput = Character->GetCharacterMovement()->GetLastInputVector();
	if (LastInput.IsNearlyZero())
	{
		OutWorldDirection = Character->GetActorForwardVector();
		OutMontage        = DashMontage_Forward;
		return;
	}

	// 입력 벡터를 캐릭터 로컬 좌표로 투영해 전후/좌우 성분을 구합니다.
	const FVector Forward = Character->GetActorForwardVector();
	const FVector Right   = Character->GetActorRightVector();

	const float ForwardDot = FVector::DotProduct(LastInput, Forward);
	const float RightDot   = FVector::DotProduct(LastInput, Right);

	// 절댓값이 더 큰 축으로 4방향 중 하나를 선택합니다.
	if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
	{
		if (ForwardDot >= 0.0f)
		{
			OutWorldDirection = Forward;
			OutMontage        = DashMontage_Forward;
		}
		else
		{
			OutWorldDirection = -Forward;
			OutMontage        = DashMontage_Backward;
		}
	}
	else
	{
		if (RightDot >= 0.0f)
		{
			OutWorldDirection = Right;
			OutMontage        = DashMontage_Right;
		}
		else
		{
			OutWorldDirection = -Right;
			OutMontage        = DashMontage_Left;
		}
	}
}
