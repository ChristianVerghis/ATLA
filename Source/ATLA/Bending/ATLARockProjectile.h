// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLAWaterProjectile.h"
#include "ATLARockProjectile.generated.h"

/**
 * Rock jab projectile: a chunk of earth torn from the ground. Straight,
 * chunky, hits harder and slower than the water whip.
 */
UCLASS()
class ATLA_API AATLARockProjectile : public AATLAWaterProjectile
{
	GENERATED_BODY()

public:
	AATLARockProjectile();

	/** Metal shard mode (the metalbending unlock): faster, harder, steel-dark */
	void MakeMetal();

protected:
	virtual void ApplyMaterials() override;

	/** Landed earth hits deepen the thrower's earth mastery */
	virtual void OnImpactVictim(AActor* Victim, const FHitResult& Hit) override;

	bool bMetal = false;

	/** Lumps fused onto the core sphere — the irregular boulder silhouette */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Lumps;
};

/**
 * Boulder: charged heavy projectile with knockback. Also used (scaled up)
 * as the hurled slab from a launched earth wall.
 */
UCLASS()
class ATLA_API AATLABoulderProjectile : public AATLARockProjectile
{
	GENERATED_BODY()

public:
	AATLABoulderProjectile();

protected:
	virtual void OnImpactVictim(AActor* Victim, const FHitResult& Hit) override;

	/** Horizontal knockback speed applied to victims (rooted/armored are immune) */
	UPROPERTY(EditDefaultsOnly, Category = "Boulder")
	float KnockbackSpeed = 600.f;
};

/**
 * The hoisted boulder: a huge rock that heaves itself out of the ground in
 * front of a bracing earthbender and hovers, waiting to be hurled. Spawned by
 * the Boulder Hoist channel; purely visual until the throw converts it into
 * an AATLABoulderProjectile.
 */
UCLASS()
class ATLA_API AATLAHoistedBoulder : public AActor
{
	GENERATED_BODY()

public:
	AATLAHoistedBoulder();

	virtual void Tick(float DeltaTime) override;

	/** True once the rock has fully torn free of the ground and can be thrown */
	bool IsRisen() const { return Age >= RiseTime * 0.65f; }

	/** Where the rock itself currently floats (the throw starts here) */
	FVector GetRockLocation() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	UStaticMeshComponent* Rock;

	/** Small debris shed while the boulder tears loose */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Debris;

	/** Lumps fused onto the hoisted rock — boulder silhouette */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> RockLumps;

	UPROPERTY(EditDefaultsOnly, Category = "Hoist")
	float RiseTime = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "Hoist")
	float HoverHeight = 190.f;

	float Age = 0.f;
};



/**
 * Earth Armor cocoon: at the palm-plant, slabs of rock surge up out of the
 * ground and close around the crouched bender's whole body, hold a beat, and
 * burst away as they rise, armored. Purely cosmetic — the armor effect is GAS.
 */
UCLASS()
class ATLA_API AATLAEarthCocoon : public AActor
{
	GENERATED_BODY()
public:
	AATLAEarthCocoon();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** The shell slabs ringing the body (two stacked rings + a cap) */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Slabs;

	/** Per-slab resting offset once closed */
	TArray<FVector> ClosedOffsets;

	/** Seconds for the shell to surge up and close */
	UPROPERTY(EditDefaultsOnly, Category = "Cocoon")
	float CloseTime = 0.22f;

	/** Seconds fully closed before bursting away */
	UPROPERTY(EditDefaultsOnly, Category = "Cocoon")
	float HoldTime = 0.55f;

	/** Slab fling velocities after the burst */
	TArray<FVector> BurstVel;

	bool bBurst = false;
	float Age = 0.f;
};
