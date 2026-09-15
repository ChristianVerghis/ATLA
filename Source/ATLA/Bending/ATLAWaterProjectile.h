// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAWaterProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/**
 * Water whip projectile. The collision sphere flies a clean ballistic path
 * (honest, network-friendly), while the visible water head snakes around it
 * serpentine-style and leaves a trail of shrinking droplets — reads as a
 * flowing stream rather than a ball. Swap for Niagara later without touching
 * gameplay.
 */
UCLASS()
class ATLA_API AATLAWaterProjectile : public AActor
{
	GENERATED_BODY()

public:
	AATLAWaterProjectile();

	virtual void Tick(float DeltaTime) override;

	/** Let this projectile pass through another actor (e.g. sibling spears in a volley) */
	void IgnoreActorForMovement(AActor* Other);

	/** Hover in place (optionally drifting up) until something launches it —
	 *  the earth uppercut tears a rock loose before the punch sends it. */
	void HoldInPlace(float RiseSpeed = 90.f);

	/** Fire a held projectile along Direction at its normal speed */
	void LaunchToward(const FRotator& Direction);

protected:
	virtual void BeginPlay() override;

	/** Applies the element look; ice variants override this */
	virtual void ApplyMaterials();

	/** Extra per-victim impact behavior (e.g. boulder knockback). Server only. */
	virtual void OnImpactVictim(AActor* Victim, const FHitResult& Hit) {}

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Gameplay effect applied to targets with an ability system on hit */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class UGameplayEffect> DamageEffect;

	/** Damage dealt to structures (walls) that implement IATLAStructureDamageable */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float StructureDamage = 15.f;

	/** Impact burst actor (water splash by default; earth debris for rocks) */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class AATLAWaterSplash> ImpactBurstClass;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	UProjectileMovementComponent* Movement;

	/** How far the water head weaves off the flight line, in cm */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Style")
	float SerpentineAmplitude = 26.f;

	/** Weave speed in radians/sec */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Style")
	float SerpentineFrequency = 11.f;

	/** Size of the trail droplets left behind the head */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Style")
	float TrailDropScale = 0.22f;

	/** Distance between stream segments, in cm — smaller = denser, smoother flow */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Style")
	float EmitSpacing = 26.f;

	/** Upward drift of the trail, cm/s — flame licks rise, water falls where it lands */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Style")
	float TrailRiseSpeed = 0.f;

	/** Random scale pulse on the head, 0..1 — fire flickers, water doesn't */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Style")
	float MeshFlickerAmp = 0.f;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TrailDrops;

	int32 NextTrailDrop = 0;
	float Age = 0.f;
	FVector LastEmitLocation = FVector::ZeroVector;
	FVector BaseMeshScale = FVector::OneVector;

	/** Flowing Niagara ribbon behind the projectile (water/air look; fire and
	 *  earth clear the asset and use their own trails) */
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> RibbonTrail;

	/** Subclasses with Niagara trails turn the mesh-drop trail off */
	bool bUseMeshTrail = true;

	/** Gravity restored when a held projectile is launched (rocks arc, water doesn't) */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float LaunchGravityScale = 0.f;

	/** True when the caster had an active signature form at launch — the
	 *  projectile grows (bigger hit sphere + visuals). Damage amp is central
	 *  in the attribute set. */
	bool bEmpowered = false;
};
