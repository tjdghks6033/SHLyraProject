// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Executions/SHDamageExecution.h"

#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/LyraGameplayEffectContext.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"
#include "Teams/LyraTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SHDamageExecution)

// 빙결 상태 태그 — 이 태그를 가진 타겟은 모든 데미지가 2배로 적용된다.
// GE_SHFrozenStatus의 GrantedTags에 의해 부여되며, 2.5초 후 자동 제거된다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_SH_Frozen, "Status.SH.Frozen");

struct FSHDamageStatics
{
	// Source(공격자)의 BaseDamage를 스냅샷으로 캡처한다.
	// 스냅샷(bSnapshot=true): Spec 생성 시점의 값을 고정 — 공격 후 버프가 해제돼도 데미지 보정.
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;

	FSHDamageStatics()
	{
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(
			ULyraCombatSet::GetBaseDamageAttribute(),
			EGameplayEffectAttributeCaptureSource::Source,
			true);
	}
};

static FSHDamageStatics& SHDamageStatics()
{
	static FSHDamageStatics Statics;
	return Statics;
}

USHDamageExecution::USHDamageExecution()
{
	RelevantAttributesToCapture.Add(SHDamageStatics().BaseDamageDef);
}

void USHDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FLyraGameplayEffectContext* TypedContext = FLyraGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	check(TypedContext);

	// Source/Target 태그 수집 — 어트리뷰트 계산과 상태이상 판별에 사용
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	// BaseDamage 캡처 (공격자 LyraCombatSet.BaseDamage)
	float BaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		SHDamageStatics().BaseDamageDef, EvaluateParameters, BaseDamage);

	const AActor* EffectCauser = TypedContext->GetEffectCauser();
	const FHitResult* HitActorResult = TypedContext->GetHitResult();

	AActor* HitActor = nullptr;

	if (HitActorResult)
	{
		HitActor = HitActorResult->HitObjectHandle.FetchActor();
	}

	// HitResult가 없거나 액터를 찾지 못한 경우 TargetASC의 아바타로 폴백
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!HitActor)
	{
		HitActor = TargetASC ? TargetASC->GetAvatarActor_Direct() : nullptr;
	}

	// 팀 데미지 허용 여부 (아군 오사 방지)
	float DamageInteractionAllowedMultiplier = 0.0f;
	if (HitActor)
	{
		ULyraTeamSubsystem* TeamSubsystem = HitActor->GetWorld()->GetSubsystem<ULyraTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			DamageInteractionAllowedMultiplier = TeamSubsystem->CanCauseDamage(EffectCauser, HitActor) ? 1.0f : 0.0f;
		}
	}

	// TargetTags는 GE가 Execute되는 시점에 캡처되므로,
	// GE_SHFrozenStatus가 부여한 Status.SH.Frozen 태그가 정확히 반영된다.
	const bool bTargetIsFrozen = TargetTags && TargetTags->HasTagExact(TAG_Status_SH_Frozen);
	const float FrozenMultiplier = bTargetIsFrozen ? 2.0f : 1.0f;

	// 최종 데미지 = BaseDamage × 팀판정 × 빙결배율
	// 참고: LyraDamageExecution의 거리감쇠·재질감쇠(ILyraAbilitySourceInterface)는
	// LYRAGAME_API 미노출로 외부 모듈에서 링크 불가 → 근접/마법 프로젝트에서 불필요하므로 생략
	const float DamageDone = FMath::Max(
		BaseDamage * DamageInteractionAllowedMultiplier * FrozenMultiplier,
		0.0f);

	if (DamageDone > 0.0f)
	{
		// LyraHealthSet.Damage 메타 어트리뷰트에 최종 데미지를 더한다.
		// PostGameplayEffectExecute에서 Health 차감 + Lyra.Damage.Message 브로드캐스트가 이어진다.
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(ULyraHealthSet::GetDamageAttribute(), EGameplayModOp::Additive, DamageDone));
	}

#endif // #if WITH_SERVER_CODE
}
