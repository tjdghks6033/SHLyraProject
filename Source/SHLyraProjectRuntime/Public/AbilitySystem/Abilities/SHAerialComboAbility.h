// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/SHGameplayAbility.h"
#include "GameplayEffectTypes.h"

#include "SHAerialComboAbility.generated.h"

class UAnimMontage;
class UCameraShakeBase;
class UGameplayEffect;
class ULyraCameraMode;
struct FGameplayEventData;

/**
 * USHAerialComboAbility
 *
 * R슬롯 궁극기 — 공중 콤보 시퀀스.
 *
 * 실행 흐름:
 *   Phase 1 (런치):
 *     LaunchMontage 재생 → AnimNotify(Event.SH.AerialCombo.Launch)
 *     → LaunchCharacter(보스, Z+) + GE_SHLaunchedStatus 적용 + CS_SH_Launch 셰이크
 *   Phase 2 (텔레포트):
 *     몽타주 완료 시 플레이어를 보스 위치 오프셋으로 순간이동 + CS_SH_Teleport 셰이크
 *     GameplayCue.SH.AerialCombo.Teleport Execute (잔상 VFX)
 *   Phase 3 (공중 콤보):
 *     AerialComboMontage 재생 → AnimNotify(Event.SH.AerialCombo.Hit) ×3
 *     → GE_SHAerialHitDamage 적용 + CS_SH_AerialHit 셰이크 ×3
 *   Phase 4 (그라운드 슬램):
 *     GroundSlamMontage 재생 → AnimNotify(Event.SH.AerialCombo.Slam)
 *     → LaunchCharacter(보스, Z-) + GE_SHSlamDamage 적용
 *     → PlayWorldCameraShake(CS_SH_GroundSlam) + GameplayCue.SH.AerialCombo.Slam Execute
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHAerialComboAbility : public USHGameplayAbility
{
	GENERATED_BODY()

public:

	USHAerialComboAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~ UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End of UGameplayAbility interface

	// ── 몽타주 ──────────────────────────────────────────────
	// AnimNotify_GameplayEvent(Event.SH.AerialCombo.Launch) 포함 필수
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Montage")
	TObjectPtr<UAnimMontage> LaunchMontage;

	// AnimNotify_GameplayEvent(Event.SH.AerialCombo.Hit) ×3 포함 필수
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Montage")
	TObjectPtr<UAnimMontage> AerialComboMontage;

	// AnimNotify_GameplayEvent(Event.SH.AerialCombo.Slam) 포함 필수
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Montage")
	TObjectPtr<UAnimMontage> GroundSlamMontage;

	// ── GameplayEffect ───────────────────────────────────────
	// 보스에 적용: Status.SH.Launched 부여 — Duration Policy: Infinite (코드에서 Slam 시 제거)
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Effects")
	TSubclassOf<UGameplayEffect> LaunchedStatusEffect;

	// 공중 콤보 히트 1회당 데미지 (BaseDamage +15)
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Effects")
	TSubclassOf<UGameplayEffect> AerialHitDamageEffect;

	// 그라운드 슬램 데미지 (BaseDamage +50)
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Effects")
	TSubclassOf<UGameplayEffect> SlamDamageEffect;

	// 슬램 후 보스 넉다운 (Duration = KnockedDownMontage 길이에 맞춤)
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Effects")
	TSubclassOf<UGameplayEffect> KnockedDownEffect;

	// ── 카메라 모드 ──────────────────────────────────────
	// EndAbility 시 자동 복원 (LyraGameplayAbility::ClearCameraMode).
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Camera")
	TSubclassOf<ULyraCameraMode> UltimateCameraMode;

	// ── 카메라 셰이크 ──────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|CameraShake")
	TSubclassOf<UCameraShakeBase> LaunchShake;

	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|CameraShake")
	TSubclassOf<UCameraShakeBase> TeleportShake;

	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|CameraShake")
	TSubclassOf<UCameraShakeBase> AerialHitShake;

	// PlayWorldCameraShake 로 호출 — 슬램 위치 기준 방사형
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|CameraShake")
	TSubclassOf<UCameraShakeBase> GroundSlamShake;

	// PlayWorldCameraShake 유효 반경 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|CameraShake", meta = (ClampMin = "1.0"))
	float SlamShakeRadius = 1500.f;

	// ── 물리 파라미터 ─────────────────────────────────────
	// 보스에 가하는 상향 런치 속도 (cm/s)
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Physics", meta = (ClampMin = "1.0"))
	float LaunchZForce = 1500.f;

	// 슬램 시 보스에 가하는 하향 속도 (cm/s, 양수로 입력)
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Physics", meta = (ClampMin = "1.0"))
	float SlamZForce = 2000.f;

	// ── 범위 / 위치 ───────────────────────────────────────
	// 전방 타겟 탐색 거리 (cm). 플레이어 정면으로 이 거리만큼 스윕하고, 적이 없으면 어빌리티 취소.
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo", meta = (ClampMin = "1.0"))
	float TargetSearchRange = 400.f;

	// 전방 탐색 스윕의 구체 반경 (cm). 클수록 정면에서 약간 빗나가도 잡힌다.
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo", meta = (ClampMin = "1.0"))
	float TargetSearchRadius = 200.f;

	// 텔레포트 시 보스로부터 플레이어가 떨어질 거리 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo", meta = (ClampMin = "0.0"))
	float TeleportStandoffDistance = 150.f;

	// 텔레포트 위치의 Z 오프셋 (cm). 양수 = 보스보다 위, 음수 = 보스보다 아래
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo")
	float TeleportZOffset = 0.f;

	// ── GameplayCue 태그 ──────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Cues")
	FGameplayTag TeleportCueTag;

	// 공중 콤보 히트 1회마다 Execute (보스 위치 기준)
	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Cues")
	FGameplayTag HitCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "SH|AerialCombo|Cues")
	FGameplayTag SlamCueTag;

private:

	// ── Phase 진입 함수 ───────────────────────────────────
	void StartLaunchPhase();
	void StartTeleportPhase();
	void StartAerialComboPhase();
	void StartGroundSlamPhase();

	// ── 몽타주 콜백 ──────────────────────────────────────
	UFUNCTION() void OnLaunchMontageCompleted();
	UFUNCTION() void OnLaunchMontageCancelled();
	UFUNCTION() void OnAerialMontageCompleted();
	UFUNCTION() void OnAerialMontageCancelled();
	UFUNCTION() void OnSlamMontageCompleted();
	UFUNCTION() void OnSlamMontageCancelled();

	// ── AnimNotify 이벤트 콜백 ────────────────────────────
	UFUNCTION() void OnLaunchEventReceived(FGameplayEventData Payload);
	UFUNCTION() void OnAerialHitEventReceived(FGameplayEventData Payload);
	UFUNCTION() void OnSlamEventReceived(FGameplayEventData Payload);

	// ── 헬퍼 ─────────────────────────────────────────────
	// 플레이어 정면을 구체 스윕해 가장 가까운 ASC 보유 캐릭터를 반환 (TargetSearchRange/Radius 사용).
	// 눈앞에 적이 없으면 nullptr → ActivateAbility에서 즉시 EndAbility.
	ACharacter* FindTargetInFront() const;

	// 현재 타겟 (Phase 1에서 찾아 이후 단계 내내 재사용)
	TWeakObjectPtr<ACharacter> TargetCharacter;

	// LaunchedStatus GE 핸들 — Slam 시 또는 EndAbility 시 명시적으로 제거
	FActiveGameplayEffectHandle LaunchedStatusHandle;
};
