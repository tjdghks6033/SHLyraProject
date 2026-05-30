// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHBossMeleeAttack.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "NativeGameplayTags.h"

// 보스 근접 히트 판정 시점 태그. 몽타주에 AnimNotify_GameplayEvent 로 추가한다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_SH_Boss_HitDetect, "Event.SH.Boss.HitDetect");

USHBossMeleeAttack::USHBossMeleeAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 공격 중 이동 고정 — 보스가 선입 동작 도중 미끄러지지 않도록.
	bLockMovementOnActivate = true;
}

FGameplayTag USHBossMeleeAttack::GetHitDetectTag() const
{
	return TAG_Event_SH_Boss_HitDetect;
}

bool USHBossMeleeAttack::ComputeHitTrace(const FGameplayAbilityActorInfo* ActorInfo,
	FVector& OutStart, FVector& OutEnd, float& OutRadius) const
{
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor)
	{
		return false;
	}

	OutRadius = HitRadius;

	// 보스 메인 스켈레탈 메시의 AttackSocket(hand_r) 위치를 판정 중심으로 사용.
	// hand_r 소켓은 보스 무기가 부착된 위치이므로 공격 범위와 자연스럽게 일치한다.
	ACharacter* BossCharacter = Cast<ACharacter>(AvatarActor);
	USkeletalMeshComponent* SkelMesh = BossCharacter ? BossCharacter->GetMesh() : nullptr;

	if (SkelMesh && SkelMesh->DoesSocketExist(AttackSocketName))
	{
		OutStart = SkelMesh->GetSocketLocation(AttackSocketName);
	}
	else
	{
		// 폴백: 캐릭터 중심(60cm 높이)에서 전방 FallbackRange 절반 거리
		OutStart = AvatarActor->GetActorLocation()
			+ FVector(0.f, 0.f, 60.f)
			+ AvatarActor->GetActorForwardVector() * (FallbackRange * 0.5f);
	}

	// Start == End: 단일 위치의 구체 오버랩
	OutEnd = OutStart;
	return true;
}
