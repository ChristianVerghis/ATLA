// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAStructureDamageable.h"
#include "ATLAIceWall.generated.h"

class UStaticMeshComponent;

/**
 * A wall of ice raised from the ground. Physically blocks projectiles and
 * characters, erupts upward over a fraction of a second, and melts away
 * when its lifetime expires.
 */
UCLASS()
class ATLA_API AATLAIceWall : public AActor, public IATLAStructureDamageable
{
	GENERATED_BODY()

public:
	AATLAIceWall();

	virtual void Tick(float DeltaTime) override;

	virtual void ApplyStructureDamage(float Amount) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Ice Wall")
	UStaticMeshComponent* Wall;

	/** Final size in cm: thickness (along aim), width (across aim), height */
	UPROPERTY(EditDefaultsOnly, Category = "Ice Wall")
	FVector WallSize = FVector(60.f, 520.f, 260.f);

	/** Seconds to fully erupt from the ground */
	UPROPERTY(EditDefaultsOnly, Category = "Ice Wall")
	float RiseTime = 0.35f;

	/** Seconds the wall stands before melting */
	UPROPERTY(EditDefaultsOnly, Category = "Ice Wall")
	float Lifetime = 8.f;

	/** Hit points before the wall shatters (siege damage from boulders etc.) */
	UPROPERTY(EditDefaultsOnly, Category = "Ice Wall")
	float StructureHP = 200.f;

	float Age = 0.f;
	float GroundZ = 0.f;
};
