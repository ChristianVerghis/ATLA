// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLAWaterProjectile.h"
#include "ATLAWaterSplash.h"
#include "GameFramework/Actor.h"
#include "ATLAAirActors.generated.h"

class ACharacter;

/** Air impact: a flat, fast gust of pale puffs blowing outward — no splash, just displaced air. */
UCLASS()
class ATLA_API AATLAAirBurst : public AATLAWaterSplash
{
	GENERATED_BODY()
public:
	AATLAAirBurst();
protected:
	virtual void ApplyMaterials() override;
};

/** Air blast: a palm-thrust ball of compressed air — light damage, heavy push. */
UCLASS()
class ATLA_API AATLAAirBlastBolt : public AATLAWaterProjectile
{
	GENERATED_BODY()
public:
	AATLAAirBlastBolt();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void ApplyMaterials() override;
	virtual void OnImpactVictim(AActor* Victim, const FHitResult& Hit) override;

	/** Wind streaks spiraling around the flight line — this is what makes air readable */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Spirals;

	UPROPERTY(EditDefaultsOnly, Category = "Air")
	float PushSpeed = 700.f;
};

/** Air swipe: a wide flat crescent — the anti-crowd knockback. */
UCLASS()
class ATLA_API AATLAAirSwipeBolt : public AATLAAirBlastBolt
{
	GENERATED_BODY()
public:
	AATLAAirSwipeBolt();
};

/** Wind dome: destroys incoming enemy projectiles for its duration. */
UCLASS()
class ATLA_API AATLAWindDome : public AActor
{
	GENERATED_BODY()
public:
	AATLAWindDome();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
	void DeflectTick();

	UPROPERTY(VisibleAnywhere, Category = "Dome")
	class USphereComponent* Field;

	UPROPERTY()
	UStaticMeshComponent* Shell;

	/** Rotating wind bands that make the dome visibly spin */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Bands;

	UPROPERTY(EditDefaultsOnly, Category = "Dome")
	float Duration = 4.f;

	FTimerHandle DeflectTimer;
	float Age = 0.f;
};

/**
 * Cyclone: a vortex that lifts, spins, and lightly grinds victims — and a
 * rideable spout for its caster: step in (or cast it at your feet) and the
 * column carries you to its crown, where you can hover, steer, or leap off
 * into a glide. Enemies still get tossed; only the instigator rides.
 */
UCLASS()
class ATLA_API AATLAAirCyclone : public AActor
{
	GENERATED_BODY()
public:
	AATLAAirCyclone();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Wisps;

	UPROPERTY(EditDefaultsOnly, Category = "Cyclone")
	float Radius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cyclone")
	float Duration = 6.f;

	/** How high above the base the caster is carried and held */
	UPROPERTY(EditDefaultsOnly, Category = "Cyclone")
	float RideHeight = 780.f;

	TSet<TWeakObjectPtr<AActor>> Damaged;
	bool bRiderBoarded = false;
	float SavedAirControl = -1.f;
	float Age = 0.f;
};

/**
 * The air scooter ball: a spinning sphere of wind under the bender's feet,
 * ridden show-style — the rider is lifted onto it and pirouettes on top
 * while the wind bands roll beneath. Cosmetic; the speed comes from the ability.
 */
UCLASS()
class ATLA_API AATLAAirScooterBall : public AActor
{
	GENERATED_BODY()
public:
	AATLAAirScooterBall();
	virtual void Tick(float DeltaTime) override;

	/** Attach under the rider's capsule and lift their mesh onto the ball. */
	void AttachToRider(ACharacter* InRider);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	UStaticMeshComponent* CoreSphere;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Bands;

	TWeakObjectPtr<ACharacter> Rider;
	FVector SavedMeshLocation = FVector::ZeroVector;
	FRotator SavedMeshRotation = FRotator::ZeroRotator;
	bool bAdjustedRider = false;
	float Age = 0.f;
};
