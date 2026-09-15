// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLAWaterProjectile.h"
#include "ATLAWaterSplash.h"
#include "GameFramework/Actor.h"
class UNiagaraComponent;

#include "ATLAFireActors.generated.h"

/** Fire impact: a flash of embers that rise and lick upward — fire climbs, it never splashes. */
UCLASS()
class ATLA_API AATLAFireBurst : public AATLAWaterSplash
{
	GENERATED_BODY()
public:
	AATLAFireBurst();
protected:
	virtual void ApplyMaterials() override;

	/** One-shot Niagara detonation (pack asset; ember meshes remain as backup) */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> Detonation;
};

/** Fire jab bolt: fast, hot, straight — a white-hot core trailing rising flame licks. */
UCLASS()
class ATLA_API AATLAFireBolt : public AATLAWaterProjectile
{
	GENERATED_BODY()
public:
	AATLAFireBolt();
protected:
	virtual void ApplyMaterials() override;

	/** A landed hit stokes the thrower's inner drive */
	virtual void OnImpactVictim(AActor* Victim, const FHitResult& Hit) override;

	/** Real flame riding the bolt (Niagara, replaces the mesh-drop trail) */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> FlameTrail;
};

/** Charged fire blast: slower fireball that explodes in a radius on impact. */
UCLASS()
class ATLA_API AATLAFireBlastBolt : public AATLAFireBolt
{
	GENERATED_BODY()
public:
	AATLAFireBlastBolt();
protected:
	virtual void ApplyMaterials() override;
	virtual void OnImpactVictim(AActor* Victim, const FHitResult& Hit) override;

	/** White-hot heart inside the orange fireball */
	UPROPERTY()
	UStaticMeshComponent* Core;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float ExplosionRadius = 260.f;
};

/** Breath-of-fire tongue: one short-lived lick of the held flame stream. */
UCLASS()
class ATLA_API AATLAFireStreamBolt : public AATLAFireBolt
{
	GENERATED_BODY()
public:
	AATLAFireStreamBolt();
};

/** Fire lash: a wide sweeping crescent of flame — Zuko's whip-arc. */
UCLASS()
class ATLA_API AATLAFireLashBolt : public AATLAFireBolt
{
	GENERATED_BODY()
public:
	AATLAFireLashBolt();
};

/**
 * Lightning: the visual bolt of the cold-blooded fire. A jittered chain of
 * white-hot segments from the fingertips to the strike point, gone in a
 * flash. Damage is applied by the ability (hitscan); this is pure light.
 */
UCLASS()
class ATLA_API AATLALightningBolt : public AActor
{
	GENERATED_BODY()
public:
	AATLALightningBolt();

	/** Lay the segments along a jittered polyline from Start to End. */
	void SetEndpoints(const FVector& Start, const FVector& End);

	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Segments;

	float Age = 0.f;
};

/** Jeong Jeong's wall of flame: burns characters crossing it, destroys projectiles. */
UCLASS()
class ATLA_API AATLAFireWall : public AActor
{
	GENERATED_BODY()
public:
	AATLAFireWall();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

	void BurnTick();

	UPROPERTY(VisibleAnywhere, Category = "Fire Wall")
	class UBoxComponent* Zone;

	/** Outer orange flame tongues (cones) */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Flames;

	/** White-hot inner cores nested in the tongues */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> FlameCores;

	/** Sparks cycling up out of the fire */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Embers;

	/** Real Niagara flames along the wall (cone tongues hide when present) */
	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> FlameFX;

	bool bNiagaraWall = false;

	UPROPERTY(EditDefaultsOnly, Category = "Fire Wall")
	float Lifetime = 8.f;

	FTimerHandle BurnTimer;
	float Age = 0.f;
};

/** Inferno nova: an expanding ring that damages and knocks back everything around the bender once. */
UCLASS()
class ATLA_API AATLAFireNova : public AActor
{
	GENERATED_BODY()
public:
	AATLAFireNova();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	UStaticMeshComponent* Ring;

	/** Bright central flash column at detonation */
	UPROPERTY()
	UStaticMeshComponent* Flash;

	/** Real Niagara detonation (flash/wisps hide when present) */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> Explosion;

	bool bNiagaraNova = false;

	/** Flame gouts thrown radially out of the blast */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Wisps;

	UPROPERTY(EditDefaultsOnly, Category = "Nova")
	float Radius = 420.f;

	float Age = 0.f;
};
