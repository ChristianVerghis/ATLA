// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAOctopusForm.generated.h"

class UStaticMeshComponent;

/**
 * Octopus form: eight segmented water tendrils arc out from the bender,
 * undulating with a wave that travels down each arm. Nearby enemies are
 * automatically lashed (a tendril visibly snaps out to strike), and one
 * victim at a time can be seized, hoisted, swung around, and thrown —
 * as Katara does in the show.
 */
UCLASS()
class ATLA_API AATLAOctopusForm : public AActor
{
	GENERATED_BODY()

public:
	AATLAOctopusForm();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void LashNearbyTargets();
	void UpdateTendrils();
	void ReleaseGrabbedTarget(bool bThrow);

	/** Flat array: tendril t, segment s lives at [t * SegmentsPerTendril + s] */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Segments;

	/** Victim currently held by the grab arm (replicated so clients render the arm) */
	UPROPERTY(Replicated)
	TObjectPtr<AActor> GrabbedTarget;

	/** Victim currently being visibly lashed (replicated for the strike visual) */
	UPROPERTY(Replicated)
	TObjectPtr<AActor> LashTarget;

	/** Reach of the tendril lashes, in cm */
	UPROPERTY(EditDefaultsOnly, Category = "Octopus")
	float LashRange = 420.f;

	/** Seconds between lash ticks */
	UPROPERTY(EditDefaultsOnly, Category = "Octopus")
	float LashInterval = 0.6f;

	/** Victims inside this range can be seized */
	UPROPERTY(EditDefaultsOnly, Category = "Octopus")
	float GrabRange = 330.f;

	/** Seconds a victim is held before being thrown */
	UPROPERTY(EditDefaultsOnly, Category = "Octopus")
	float GrabHoldTime = 1.6f;

	/** Throw speed on release, cm/s */
	UPROPERTY(EditDefaultsOnly, Category = "Octopus")
	float ThrowSpeed = 950.f;

	/** Seconds the form lasts */
	UPROPERTY(EditDefaultsOnly, Category = "Octopus")
	float Duration = 10.f;

	FTimerHandle LashTimer;
	float Age = 0.f;
	float GrabStartTime = 0.f;
	float LashVisualEndTime = 0.f;
};
