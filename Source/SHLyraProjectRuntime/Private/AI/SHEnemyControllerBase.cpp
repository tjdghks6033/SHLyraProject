// Copyright SH. All Rights Reserved.

#include "AI/SHEnemyControllerBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

ASHEnemyControllerBase::ASHEnemyControllerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Lyra 봇 흐름에 편입되기 위해 PlayerState 보유. 언포세스 시 AI 로직이 유지되도록.
	bWantsPlayerState = true;
	bStopAILogicOnUnposses = false;

	SHPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("SHPerceptionComponent"));

	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius                  = SightRadius;
	SightConfig->LoseSightRadius              = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngle;
	SightConfig->SetMaxAge(5.0f);

	// 팀 시스템(ILyraTeamAgentInterface)이 붙은 상태에서는 플레이어(Team 0)와 적(Team 10)의
	// TeamId가 다르므로 Lyra 솔버가 Hostile로 판정한다. bDetectEnemies 하나로 플레이어 감지 성립.
	SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals   = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	SHPerceptionComponent->ConfigureSense(*SightConfig);
	SHPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void ASHEnemyControllerBase::BeginPlay()
{
	Super::BeginPlay();

	// 컴포넌트가 완전히 초기화된 이후에 바인딩.
	SHPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this, &ASHEnemyControllerBase::OnTargetPerceptionUpdated);
}

// ---------------------------------------------------------------------------
// ILyraTeamAgentInterface — 팀 ID는 PlayerState가 보유, 컨트롤러는 위임·전파 담당
// (Lyra ALyraPlayerBotController의 패턴을 재현)
// ---------------------------------------------------------------------------

void ASHEnemyControllerBase::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// PlayerState가 팀 소유권을 갖기 때문에 컨트롤러에서 직접 설정하는 것은 차단.
	// 팀 변경은 LyraTeamSubsystem::ChangeTeamForActor(PlayerState, ...) 경로를 사용해야 한다.
	UE_LOG(LogTemp, Warning,
		TEXT("SetGenericTeamId on enemy controller (%s) is ignored — team is owned by PlayerState."),
		*GetPathNameSafe(this));
}

FGenericTeamId ASHEnemyControllerBase::GetGenericTeamId() const
{
	if (const ILyraTeamAgentInterface* PSWithTeamInterface = Cast<ILyraTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnLyraTeamIndexChangedDelegate* ASHEnemyControllerBase::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

ETeamAttitude::Type ASHEnemyControllerBase::GetTeamAttitudeTowards(const AActor& Other) const
{
	// Other가 Pawn이면 그 Controller의 ILyraTeamAgentInterface를 통해 팀 ID 비교.
	if (const APawn* OtherPawn = Cast<APawn>(&Other))
	{
		if (const ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(OtherPawn->GetController()))
		{
			const FGenericTeamId OtherTeamID = TeamAgent->GetGenericTeamId();
			return (OtherTeamID.GetId() != GetGenericTeamId().GetId())
				? ETeamAttitude::Hostile
				: ETeamAttitude::Friendly;
		}
	}
	return ETeamAttitude::Neutral;
}

// ---------------------------------------------------------------------------
// PlayerState 변경 흐름 — 팀 바인딩 재설정
// ---------------------------------------------------------------------------

void ASHEnemyControllerBase::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ASHEnemyControllerBase::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void ASHEnemyControllerBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void ASHEnemyControllerBase::BroadcastOnPlayerStateChanged()
{
	// 이전 PlayerState의 팀 델리게이트 해제.
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (ILyraTeamAgentInterface* PrevTeamIF = Cast<ILyraTeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamID = PrevTeamIF->GetGenericTeamId();
			PrevTeamIF->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// 새 PlayerState의 팀 델리게이트 구독.
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (ILyraTeamAgentInterface* NewTeamIF = Cast<ILyraTeamAgentInterface>(PlayerState))
		{
			NewTeamID = NewTeamIF->GetGenericTeamId();
			NewTeamIF->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// 팀이 바뀌었으면 우리 자신의 델리게이트도 방출. (ConditionalBroadcastTeamChanged는 LYRAGAME_API로 export됨)
	ILyraTeamAgentInterface::ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);

	LastSeenPlayerState = PlayerState;
}

void ASHEnemyControllerBase::OnPlayerStateChangedTeam(UObject* /*TeamAgent*/, int32 OldTeam, int32 NewTeam)
{
	ILyraTeamAgentInterface::ConditionalBroadcastTeamChanged(
		this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void ASHEnemyControllerBase::OnUnPossess()
{
	// 언포세스 시 ASC의 avatar가 구 폰을 계속 가리키지 않도록 정리. (Lyra 봇 컨트롤러 동일 패턴)
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}

	Super::OnUnPossess();
}

// ---------------------------------------------------------------------------
// Perception
// ---------------------------------------------------------------------------

void ASHEnemyControllerBase::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Actor == GetPawn() || !Actor->IsA<APawn>())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		if (!CurrentTarget)
		{
			CurrentTarget = Actor;
			return;
		}

		const APawn* MyPawn = GetPawn();
		if (!MyPawn)
		{
			return;
		}

		const float CurrentDist = FVector::Dist(MyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
		const float NewDist     = FVector::Dist(MyPawn->GetActorLocation(), Actor->GetActorLocation());

		if (NewDist < CurrentDist)
		{
			CurrentTarget = Actor;
		}
	}
	else
	{
		if (CurrentTarget == Actor)
		{
			CurrentTarget = nullptr;
			StopMovement();
			ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}

float ASHEnemyControllerBase::GetDistanceToTarget() const
{
	const APawn* MyPawn = GetPawn();
	if (!CurrentTarget || !MyPawn)
	{
		return MAX_FLT;
	}
	return FVector::Dist(MyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
}
