// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLABendingAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/TimerHandle.h"
#include "ATLAAbility_DrawWater.generated.h"

class AATLAWaterSource;

/**
 * Channeled draw: while held near a water source, water streams into the
 * bender's carried inventory. Ends on release, when full, or when the bender
 * moves out of range. No cost, no cooldown.
 */
UCLASS()
class ATLA_API UATLAAbility_DrawWater : public UATLABendingAbility
{
	GENERATED_BODY()

public:
	UATLAAbility_DrawWater();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Periodic check: still in range? full? Also emits the draw visual. */
	void TickDraw();

	/** Seconds between draw-state checks */
	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	float CheckInterval = 0.25f;

	UPROPERTY()
	TObjectPtr<AATLAWaterSource> CurrentSource;

	FActiveGameplayEffectHandle DrawEffectHandle;
	FTimerHandle CheckTimer;
	float VisualAccumulator = 0.f;
};
