// Copyright SH. All Rights Reserved.

#pragma once

#include "GameplayEffectExecutionCalculation.h"

#include "SHDamageExecution.generated.h"

/**
 * USHDamageExecution
 *
 * SHLyraProject 전용 데미지 실행 계산. ULyraDamageExecution의 로직을 기반으로 하되,
 * 상태이상에 따른 데미지 배율을 ExecutionCalculation 내부에서 처리한다.
 *
 * [설계 원칙]
 * 데미지 배율 조건(빙결 등)은 어빌리티·발사체가 아닌 이 클래스에서만 판단한다.
 * 어빌리티/발사체 코드는 "GE를 적용한다"는 책임만 가지며, 피해 대상의 상태를 알 필요 없다.
 *
 * [상태이상 배율]
 * - Status.SH.Frozen : 데미지 × 2 (빙결 취약 상태)
 *
 * [사용처]
 * GE_SHMeleeDamage, GE_SHIceBoltDamage, GE_SHFireballDamage 등
 * SHLyraProject의 모든 데미지 GE ExecutionClass를 이 클래스로 지정한다.
 */
UCLASS()
class SHLYRAPROJECTRUNTIME_API USHDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:

	USHDamageExecution();

protected:

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
