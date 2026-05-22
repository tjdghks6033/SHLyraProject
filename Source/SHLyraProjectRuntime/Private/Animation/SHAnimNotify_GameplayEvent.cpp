// Copyright SH. All Rights Reserved.

#include "Animation/SHAnimNotify_GameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"

void USHAnimNotify_GameplayEvent::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!EventTag.IsValid() || !MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.Instigator = Owner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
}

FString USHAnimNotify_GameplayEvent::GetNotifyName_Implementation() const
{
	return EventTag.IsValid()
		? FString::Printf(TEXT("SH GE: %s"), *EventTag.GetTagName().ToString())
		: TEXT("SH Gameplay Event");
}
