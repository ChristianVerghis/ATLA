// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAEarthSpikeLine.generated.h"

class UStaticMeshComponent;

/**
 * A wave of stone spikes erupting from the ground in a line toward the aim —
 * the classic earthbending fissure attack. Each spike bursts up in sequence,
 * damaging and popping up characters caught above it, then the line sinks away.
 */
UCLASS()
class ATLA_API AATLAEarthSpikeLine : public AActor
{
	GENERATED_BODY()

public:
	AATLAEarthSpikeLine();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	void EruptNext();

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Spikes;

	/** Distance between spikes along the line, in cm */
	UPROPERTY(EditDefaultsOnly, Category = "Spikes")
	float Spacing = 160.f;

	/** Seconds between successive eruptions */
	UPROPERTY(EditDefaultsOnly, Category = "Spikes")
	float EruptInterval = 0.07f;

	/** Damage radius around each spike as it erupts */
	UPROPERTY(EditDefaultsOnly, Category = "Spikes")
	float HitRadius = 150.f;

	/** Upward pop applied to victims */
	UPROPERTY(EditDefaultsOnly, Category = "Spikes")
	float PopUpSpeed = 540.f;

	TArray<FVector> SpikeBases;
	TArray<float> EruptTimes;
	TSet<TWeakObjectPtr<AActor>> Victims;
	FTimerHandle EruptTimer;
	int32 NextSpike = 0;
	float Age = 0.f;
};
