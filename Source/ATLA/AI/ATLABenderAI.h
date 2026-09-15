// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Engine/TimerHandle.h"
#include "ATLABenderAI.generated.h"

class UGameplayAbility;

/**
 * A sparring opponent that actually bends: keeps a fighting range from the
 * player, strafes, and throws its element's techniques on a human-ish rhythm.
 * Deliberately simple — a testable enemy, not a boss: no prediction, no
 * dodging, generous cast gaps.
 */
UCLASS()
class ATLA_API AATLABenderAI : public AAIController
{
	GENERATED_BODY()

public:
	AATLABenderAI();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Throw a technique: mostly the element's jab, sometimes a heavy form */
	void CastSomething();

	/** Preferred fighting band (outside = close in, inside = back off) */
	UPROPERTY(EditDefaultsOnly, Category = "Bender AI")
	float PreferredRangeMin = 550.f;

	UPROPERTY(EditDefaultsOnly, Category = "Bender AI")
	float PreferredRangeMax = 1100.f;

	/** Seconds between casts (random in range — a rhythm, not a metronome) */
	UPROPERTY(EditDefaultsOnly, Category = "Bender AI")
	float CastGapMin = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Bender AI")
	float CastGapMax = 2.9f;

	/** Chance a cast is the heavy form instead of the jab */
	UPROPERTY(EditDefaultsOnly, Category = "Bender AI")
	float HeavyChance = 0.3f;

	FTimerHandle CastTimer;
	float StrafePhase = 0.f;
	float StrafeDir = 1.f;
};
