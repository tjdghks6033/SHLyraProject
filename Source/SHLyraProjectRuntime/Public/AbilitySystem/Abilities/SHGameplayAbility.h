// Copyright SH. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GameplayTagContainer.h"

#include "SHGameplayAbility.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UTexture2D;

/**
 * USHGameplayAbility
 *
 * SHLyraProject 공용 GameplayAbility 베이스.
 *
 * 제공 기능:
 *   - bLockMovementOnActivate: 활성 동안 캐릭터 MaxWalkSpeed를
 *     LockedWalkSpeed로 낮춰 '커밋형' 어빌리티(근접 공격, 캐스트 마법 등)의
 *     타격감을 만든다. EndAbility에서 캐릭터 클래스 CDO 의 기본값으로 복원.
 *
 * 대쉬처럼 이동 자체가 목적인 어빌리티는 bLockMovementOnActivate=false로 둔다.
 *
 * 설계 메모:
 *   런타임 값을 캐시하지 않고 CDO 기본값으로 복원한다.
 *   → 중복 활성/이벤트 누락으로 인한 '락 스턱' 버그가 구조적으로 발생하지 않는다.
 *   → 단점: 다른 시스템이 런타임에 속도를 수정하고 있었다면 그 값도 함께 리셋됨.
 *     (현 프로젝트에는 해당 케이스 없음)
 */
UCLASS(Abstract)
class SHLYRAPROJECTRUNTIME_API USHGameplayAbility : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:

	USHGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ASC에 부여된 어빌리티 중 InputTag가 일치하는 것의 CDO를 반환한다.
	// WBP_SHSkillSlot이 Construct 시점에 아이콘/쿨다운태그를 자동으로 읽기 위해 사용.
	// 매칭되는 스펙이 없으면 nullptr 반환.
	UFUNCTION(BlueprintCallable, Category = "SH|UI")
	static USHGameplayAbility* FindAbilityCDOByInputTag(UAbilitySystemComponent* ASC, FGameplayTag InputTag);

	// InputTag에 해당하는 어빌리티의 코스트 GE를 GAS 내장 CheckCost로 검사한다.
	// 현재 어트리뷰트(마나/스태미나)가 코스트를 감당할 수 있으면 true.
	// 비용 값을 직접 중복 관리하지 않고 코스트 GE 정의를 그대로 활용한다.
	UFUNCTION(BlueprintCallable, Category = "SH|UI")
	static bool CanPayCostByInputTag(UAbilitySystemComponent* ASC, FGameplayTag InputTag);

protected:

	//~ UGameplayAbility interface

	// CheckCost 오버라이드: Lyra는 어트리뷰트 클램핑을 PostGameplayEffectExecute에서 처리하므로
	// 기본 CanApplyAttributeModifiers가 min 값을 인식하지 못한다.
	// 메타 어트리뷰트(ManaCost, StaminaCost) → 실제 자원(Mana, Stamina) 매핑을 직접 비교한다.
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End of UGameplayAbility interface

	// 스킬바 슬롯에 표시할 아이콘.
	// WBP_SHSkillSlot이 InputTag로 이 CDO를 찾아 자동으로 읽는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SH|UI")
	TObjectPtr<UTexture2D> SlotIcon;

	// 쿨다운 GE의 AssetTag. WBP_SHSkillSlot이 Effect Tag Query에 사용한다.
	// 예: "Cooldown.SH.IceBolt" / 쿨다운 없으면 공백
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SH|UI", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	// 활성 중 캐릭터 이동을 제한할지 여부.
	// 근접/마법 같은 '커밋형' 어빌리티는 true로 설정.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Movement")
	bool bLockMovementOnActivate = false;

	// bLockMovementOnActivate=true일 때 적용할 MaxWalkSpeed (cm/s).
	// 0이면 공격 중 완전 정지. 소량의 입력 반응을 주려면 100~200 정도 권장.
	UPROPERTY(EditDefaultsOnly, Category = "SH|Movement",
		meta = (EditCondition = "bLockMovementOnActivate", ClampMin = "0.0"))
	float LockedWalkSpeed = 0.0f;

private:

	// 락을 건 캐릭터. ActorInfo 의존 없이 EndAbility 에서 안전하게 복원하기 위해 보관.
	TWeakObjectPtr<ACharacter> LockedCharacter;
};
