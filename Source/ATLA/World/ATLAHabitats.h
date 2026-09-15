// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAHabitats.generated.h"

class UStaticMeshComponent;

/**
 * Pro-bending coliseum (the multi-element option): an elliptical ring of
 * stands around a water basin, with a raised central platform on stilts and
 * three side platforms — the Korra arena silhouette, built from primitives.
 * Press N to travel between it and your element's home square.
 */
UCLASS()
class ATLA_API AATLAColiseum : public AActor
{
	GENERATED_BODY()
public:
	AATLAColiseum();

	/** Where travelers arrive: the center of the raised platform */
	UFUNCTION(BlueprintPure, Category = "Coliseum")
	FVector GetPlatformTop() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Stone;    // walls, stands, stilts

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Basin;            // the water floor

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Decks;    // center + side platforms

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Lamp;             // chandelier glow
};

/**
 * The habitats' original benders, as ambient silhouettes: the koi Tui and La
 * circling their pool (water learned from the Moon's push and pull), a
 * badgermole by the rocks, two dragons flying their helix, and a sky bison
 * drifting overhead. Purely cosmetic composites, one actor per habitat.
 */
UENUM()
enum class EATLACreatureStyle : uint8
{
	KoiPair,
	Badgermole,
	Dragons,
	SkyBison,
};

/**
 * Element habitat dressing: expands each home square into a real biome ring
 * (radius ~4000). Water gets the most detail — the spirit-oasis pond, ice
 * floes, snow mounds, and an ice-wall crescent; earth gets pillars, boulders
 * and a cave arch; fire gets obsidian pillars with braziers and banners; air
 * gets spires, floating cloud discs and a meditation circle.
 */
UCLASS()
class ATLA_API AATLAHabitatZone : public AActor
{
	GENERATED_BODY()
public:
	AATLAHabitatZone();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Habitat")
	uint8 Element = 0;   // EATLAElement value

protected:
	virtual void BeginPlay() override;
	void BuildZone();

	UStaticMeshComponent* Piece(const TCHAR* Mesh, const FVector& Rel, const FVector& Scale, const FRotator& Rot, const FLinearColor& Tint, bool bCollide);

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Pieces;

	/** Parts that drift (cloud discs, floes) */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Drifters;

	TArray<FVector> DriftHomes;
	float Age = 0.f;
};

UCLASS()
class ATLA_API AATLAHabitatCreature : public AActor
{
	GENERATED_BODY()
public:
	AATLAHabitatCreature();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Creature")
	EATLACreatureStyle Style = EATLACreatureStyle::KoiPair;

protected:
	virtual void BeginPlay() override;

	/** Builds the composite for the chosen style (runtime — components are dynamic) */
	void BuildBody();

	UStaticMeshComponent* AddPart(const TCHAR* Mesh, const FVector& Rel, const FVector& Scale, const FRotator& Rot, USceneComponent* Parent);

	/** Movable roots: koi/dragon/bison bodies that travel their paths */
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Movers;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	/** Dragon spine segments per dragon (animated as trailing chains) */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> DragonA;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> DragonB;

	float Age = 0.f;
};
