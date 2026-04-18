// Copyright SH. All Rights Reserved.

#include "AbilitySystem/Abilities/SHMagicProjectile.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"
#include "Projectile/SHMagicProjectileActor.h"

// 발사체 스폰 타이밍을 알리는 Gameplay Event 태그.
// 몽타주에 AnimNotify_GameplayEvent를 추가하고 이 태그를 지정해야 한다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Event_SH_Magic_Launch, "Event.SH.Magic.Launch");

// 마법 쿨다운 태그.
// GE_SHMagicCooldown 이 이 태그를 ASC에 부여하고,
// GA_SHMagicProjectile 의 Cooldown Tags 에 동일 태그를 지정해 쿨다운을 감지한다.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_SH_Magic, "Cooldown.SH.Magic");

USHMagicProjectile::USHMagicProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ELyraAbilityActivationPolicy::OnInputTriggered;
}

void USHMagicProjectile::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	if (!CastMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("USHMagicProjectile: CastMontage가 설정되지 않았습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, CastMontage, 1.0f, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &USHMagicProjectile::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &USHMagicProjectile::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &USHMagicProjectile::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &USHMagicProjectile::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_Event_SH_Magic_Launch, nullptr, false, true);

	WaitEventTask->EventReceived.AddDynamic(this, &USHMagicProjectile::OnGameplayEventReceived);
	WaitEventTask->ReadyForActivation();
}

void USHMagicProjectile::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USHMagicProjectile::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USHMagicProjectile::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USHMagicProjectile::OnGameplayEventReceived(FGameplayEventData Payload)
{
	if (HasAuthority(&CurrentActivationInfo))
	{
		SpawnProjectile(CurrentActorInfo);
	}
}

void USHMagicProjectile::SpawnProjectile(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ProjectileClass || !ActorInfo)
	{
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor)
	{
		return;
	}

	// 스폰 위치: 소켓 우선, 없으면 캐릭터 위치 + 오프셋
	FVector SpawnLocation;
	FRotator SpawnRotation;

	USkeletalMeshComponent* Mesh = AvatarActor->FindComponentByClass<USkeletalMeshComponent>();
	const bool bHasSocket = Mesh && Mesh->DoesSocketExist(SpawnSocketName);

	if (bHasSocket)
	{
		SpawnLocation = Mesh->GetSocketLocation(SpawnSocketName);
		SpawnRotation = AvatarActor->GetActorRotation();
	}
	else
	{
		SpawnLocation = AvatarActor->GetActorLocation() + AvatarActor->GetActorRotation().RotateVector(SpawnOffset);
		SpawnRotation = AvatarActor->GetActorRotation();
	}

	// 컨트롤 회전(카메라 방향)이 있으면 발사 방향으로 사용한다.
	if (AController* Controller = AvatarActor->GetInstigatorController())
	{
		SpawnRotation = Controller->GetControlRotation();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASHMagicProjectileActor* Projectile = AvatarActor->GetWorld()->SpawnActor<ASHMagicProjectileActor>(
		ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Projectile)
	{
		Projectile->InitProjectile(
			ActorInfo->AbilitySystemComponent.Get(),
			DamageEffect,
			GetAbilityLevel());
	}
}
