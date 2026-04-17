// Copyright SH. All Rights Reserved.

#include "Enemy/SHEnemyCharacter.h"
#include "AI/SHEnemyAIController.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraHealthComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"

ASHEnemyCharacter::ASHEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = ASHEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// ASC: OwnerActor = this, 복제 활성화
	AbilitySystemComponent = CreateDefaultSubobject<ULyraAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// LyraHealthComponent: BeginPlay에서 ASC와 연결
	HealthComponent = CreateDefaultSubobject<ULyraHealthComponent>(TEXT("HealthComponent"));
}

UAbilitySystemComponent* ASHEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASHEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 1. ASC 초기화 (OwnerActor = AvatarActor = this)
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// 2. LyraHealthSet 부여
	//    Outer를 this(Actor)로 지정해야 UAttributeSet::GetOwningActor()가 올바르게 동작한다.
	//    AbilitySystemComponent를 Outer로 쓰면 AActor 캐스팅 실패로 SetHealth 등이 무시된다.
	ULyraHealthSet* HealthSet = NewObject<ULyraHealthSet>(this);
	AbilitySystemComponent->AddAttributeSetSubobject(HealthSet);

	// 3. InitHealthEffect 적용: Health / MaxHealth 초기값 설정
	if (InitHealthEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle Spec =
			AbilitySystemComponent->MakeOutgoingSpec(InitHealthEffect, 1.0f, Context);

		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// 4. LyraHealthComponent에 ASC 연결
	HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);

	// 5. 사망 이벤트 바인딩
	HealthComponent->OnDeathStarted.AddDynamic(this, &ASHEnemyCharacter::HandleEnemyDeathStarted);
}

void ASHEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HealthComponent->UninitializeFromAbilitySystem();
	Super::EndPlay(EndPlayReason);
}

void ASHEnemyCharacter::HandleEnemyDeathStarted(AActor* OwningActor)
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle, this, &ASHEnemyCharacter::DestroyEnemy, DestroyDelay, false);
}

void ASHEnemyCharacter::DestroyEnemy()
{
	Destroy();
}
