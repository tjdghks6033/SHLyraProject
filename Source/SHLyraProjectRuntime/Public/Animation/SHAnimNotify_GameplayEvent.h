// Copyright SH. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"

#include "SHAnimNotify_GameplayEvent.generated.h"

/**
 * USHAnimNotify_GameplayEvent
 *
 * 몽타주 특정 프레임에 GAS GameplayEvent 를 발화하는 범용 AnimNotify.
 * 에디터에서 EventTag 만 변경하면 어떤 몽타주에서도 재사용 가능하다.
 *
 * 사용법:
 *   몽타주 Notifies 트랙 → 우클릭 → Add Notify → SHAnimNotify_GameplayEvent
 *   → Details 패널에서 Event Tag 설정 (예: Event.SH.AerialCombo.Launch)
 */
UCLASS(meta = (DisplayName = "SH Gameplay Event"))
class SHLYRAPROJECTRUNTIME_API USHAnimNotify_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	// 발화할 GameplayEvent 태그. 몽타주마다 다른 태그로 설정한다.
	UPROPERTY(EditAnywhere, Category = "SH|Gameplay Event")
	FGameplayTag EventTag;
};
