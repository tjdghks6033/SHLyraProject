// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHMeleeAttack.h"

#include "Components/StaticMeshComponent.h"
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

FGameplayTag USHMeleeAttack::GetHitDetectTag() const
{
	return TAG_Event_SH_Melee_HitDetect;
}

bool USHMeleeAttack::ComputeHitTrace(const FGameplayAbilityActorInfo* ActorInfo,
	FVector& OutStart, FVector& OutEnd, float& OutRadius) const
{
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor)
	{
		return false;
	}

	OutRadius = SweepRadius;

	// WeaponMesh 컴포넌트를 이름으로 찾는다.
	UStaticMeshComponent* WeaponMesh = nullptr;
	for (UActorComponent* Comp : AvatarActor->GetComponents())
	{
		if (Comp->GetFName() == WeaponMeshComponentName)
		{
			WeaponMesh = Cast<UStaticMeshComponent>(Comp);
			break;
		}
	}

	const bool bHasSockets = WeaponMesh
		&& WeaponMesh->DoesSocketExist(HiltSocketName)
		&& WeaponMesh->DoesSocketExist(TipSocketName);

	if (bHasSockets)
	{
		// 소켓 기반 Sweep: WeaponHilt → WeaponTip
		OutStart = WeaponMesh->GetSocketLocation(HiltSocketName);
		OutEnd   = WeaponMesh->GetSocketLocation(TipSocketName);
	}
	else
	{
		// 폴백: 캐릭터 중심(60cm 높이)에서 전방으로 구체 트레이스
		OutStart = AvatarActor->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		OutEnd   = OutStart + AvatarActor->GetActorForwardVector() * FallbackTraceRange;
	}

	return true;
}
