// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAEarthColumn.generated.h"

class UStaticMeshComponent;

/** Short-lived rock column that erupts under an earth-launching bender. Cosmetic. */
UCLASS()
class ATLA_API AATLAEarthColumn : public AActor
{
	GENERATED_BODY()

public:
	AATLAEarthColumn();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Column")
	UStaticMeshComponent* Column;

	float Age = 0.f;
	float GroundZ = 0.f;
};
