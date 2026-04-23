// Copyright SH. All Rights Reserved.

#include "Enemy/SHEnemyBase.h"
#include "TimerManager.h"

ASHEnemyBase::ASHEnemyBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// ALyraCharacter가 PawnExtensionComponent / HealthComponent / CameraComponent를 이미 서브오브젝트로 생성한다.
	// 적 공통 설정이 필요하면 여기에 추가.
}

void ASHEnemyBase::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();

	// ASC 초기화 완료 — 자식 클래스가 Attribute 초기값 세팅이나 어빌리티 바인딩 훅으로 활용 가능.
}

void ASHEnemyBase::OnDeathFinished(AActor* OwningActor)
{
	// ALyraCharacter 기본 구현은 SetTimerForNextTick → DestroyDueToDeath로 즉시 파괴한다.
	// 적은 시체 연출(애니메이션, VFX) 시간을 확보하기 위해 DestroyDelay만큼 지연한다.
	// 이동/충돌 비활성화는 OnDeathStarted에서 부모가 이미 처리했으므로 여기서 중복하지 않는다.

	GetWorldTimerManager().SetTimer(
		DestroyTimerHandle,
		this,
		&ASHEnemyBase::HandleDestroyAfterDelay,
		DestroyDelay,
		false);
}

void ASHEnemyBase::HandleDestroyAfterDelay()
{
	// Lyra 정식 파괴 경로 — K2_OnDeathFinished(BP 이벤트) 발화 + UninitAndDestroy(ASC uninit + Destroy) 실행.
	DestroyDueToDeath();
}
