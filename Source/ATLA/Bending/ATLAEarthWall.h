// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAStructureDamageable.h"
#include "ATLAEarthWall.generated.h"

class UStaticMeshComponent;

/**
 * Earth wall: shorter and wider than the ice wall, destructible (300 HP)
 * rather than melting, and launchable — the owner can convert it into a
 * hurled slab. Neutral jing: the defence stores an attack.
 */
UCLASS()
class ATLA_API AATLAEarthWall : public AActor, public IATLAStructureDamageable
{
	GENERATED_BODY()

public:
	AATLAEarthWall();

	virtual void Tick(float DeltaTime) override;
	virtual void ApplyStructureDamage(float Amount) override;

	/** Shove the wall: it slides along the ground in this direction, damaging
	 *  and knocking aside characters, until it hits something solid and shatters. */
	void Push(const FRotator& Direction);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Earth Wall")
	UStaticMeshComponent* Wall;

	/** thickness (along aim), width (across aim), height */
	UPROPERTY(EditDefaultsOnly, Category = "Earth Wall")
	FVector WallSize = FVector(60.f, 400.f, 280.f);

	UPROPERTY(EditDefaultsOnly, Category = "Earth Wall")
	float RiseTime = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Earth Wall")
	float Lifetime = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Earth Wall")
	float StructureHP = 300.f;

	/** Slide speed once pushed, cm/s */
	UPROPERTY(EditDefaultsOnly, Category = "Earth Wall")
	float SlideSpeed = 1400.f;

	/** Max slide distance before the wall crumbles on its own */
	UPROPERTY(EditDefaultsOnly, Category = "Earth Wall")
	float MaxSlideDistance = 3500.f;

	void Shatter();

	float Age = 0.f;
	float GroundZ = 0.f;
	bool bSliding = false;
	FVector SlideDir = FVector::ZeroVector;
	float SlideDistance = 0.f;
	TSet<TWeakObjectPtr<AActor>> SlideVictims;
};
