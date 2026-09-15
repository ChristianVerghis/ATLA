// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAWaterSplash.generated.h"

class UStaticMeshComponent;

/**
 * Short-lived water impact burst: droplets scatter outward under gravity
 * while a translucent shell expands and pops. Purely cosmetic.
 */
UCLASS()
class ATLA_API AATLAWaterSplash : public AActor
{
	GENERATED_BODY()

public:
	AATLAWaterSplash();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** Element look; earth burst overrides */
	virtual void ApplyMaterials();

	UPROPERTY()
	UStaticMeshComponent* Shell;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Droplets;

	// Burst style knobs — element bursts retune these in their constructors.
	float DropletGravityZ = -980.f;   // positive makes embers rise (fire)
	float DropletSpeedMin = 220.f;
	float DropletSpeedMax = 420.f;
	float DropletUpBias = 0.3f;       // 0 = pure radial scatter, 1 = mostly upward
	float DropletZMul = 1.f;          // < 1 flattens the scatter into a ground gust (air)
	float DropletShrinkRate = 2.f;
	FVector DropletShape = FVector(0.12f);

	TArray<FVector> DropletVelocities;
	float Age = 0.f;
};
