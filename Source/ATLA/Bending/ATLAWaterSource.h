// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAWaterSource.generated.h"

class UStaticMeshComponent;

/**
 * A pool of bendable water placed in the level. Benders within DrawRange can
 * channel water from it into their carried inventory (hold the draw input).
 */
UCLASS()
class ATLA_API AATLAWaterSource : public AActor
{
	GENERATED_BODY()

public:
	AATLAWaterSource();

	/** How close a bender must be to draw from this pool, in cm */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Source")
	float DrawRange = 600.f;

	/** Finds the nearest source within draw range of the given location, or nullptr */
	static AATLAWaterSource* FindSourceInRange(UWorld* World, const FVector& Location);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Water Source")
	UStaticMeshComponent* Pool;
};
