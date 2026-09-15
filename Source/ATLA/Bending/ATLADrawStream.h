// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ATLADrawStream.generated.h"

class ACharacter;
class UStaticMeshComponent;
class UStaticMesh;

/**
 * The source-draw: bending visibly pulls its element from the world into the
 * bender's casting hand. Water arcs out of the nearest pool, earth rips out of
 * the ground, air converges from a ring around the body. Fire spawns nothing —
 * it is self-generated from the breath. Purely cosmetic; the resource economy
 * is unchanged.
 */
UCLASS()
class ATLA_API AATLADrawStream : public AActor
{
	GENERATED_BODY()

public:
	AATLADrawStream();

	virtual void Tick(float DeltaTime) override;

	/** Spawn the element-appropriate draw for a cast wind-up (server only, no-op for fire). */
	static void SpawnForCast(ACharacter* Caster, const FGameplayTag& ElementTag, float Duration);

protected:
	virtual void BeginPlay() override;

	enum EStyle { StyleWater, StyleEarth, StyleAir };

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Globules;

	/** Flowing ribbon riding the lead globule (water/air styles) */
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> StreamRibbon;

	TWeakObjectPtr<ACharacter> Caster;
	TArray<FVector> Starts;      // per-globule origin points
	TArray<FRotator> TumbleRates; // earth chunks tumble as they fly
	int32 Style = StyleWater;
	float Duration = 0.4f;
	float Age = 0.f;
	float LastSplashTime = -1.f;

	/** Kept so BeginPlay can swap water/air globules back to spheres */
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMeshAsset;

	friend class UATLACastAbility;
};

/**
 * A growing orb of the held element at the casting hand while a charged
 * technique winds up. Spawned by the character on hold, destroyed on release.
 */
UCLASS()
class ATLA_API AATLAChargeGlow : public AActor
{
	GENERATED_BODY()

public:
	AATLAChargeGlow();

	virtual void Tick(float DeltaTime) override;

	/** Attach to the bender's casting hand and pick the element look. */
	void AttachToHand(ACharacter* Bender, const FGameplayTag& ElementTag);

protected:
	UPROPERTY()
	UStaticMeshComponent* Orb;

	UPROPERTY()
	UStaticMeshComponent* Core;

	bool bFlicker = false;
	float Age = 0.f;
};
