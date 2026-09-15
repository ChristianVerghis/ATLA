// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;

/** Shared helpers for the placeholder water/ice look (engine primitives + generated materials). */
namespace ATLAWaterVisuals
{
	/** Assigns the engine sphere mesh. Only call from a constructor. */
	void SetupWaterMesh(UStaticMeshComponent* Mesh);

	/** Assigns the engine cone mesh (flame tongues). Only call from a constructor. */
	void SetupConeMesh(UStaticMeshComponent* Mesh);

	/** Applies /Game/Bending/VFX/M_Water if it exists, otherwise tints the basic material blue. Call at runtime (BeginPlay). */
	void ApplyWaterMaterial(UStaticMeshComponent* Mesh);

	/** Applies /Game/Bending/VFX/M_Ice if it exists, otherwise tints the basic material pale blue. Call at runtime (BeginPlay). */
	void ApplyIceMaterial(UStaticMeshComponent* Mesh);

	/** Applies /Game/Bending/VFX/M_Earth if it exists, otherwise tints the basic material brown. Call at runtime (BeginPlay). */
	void ApplyEarthMaterial(UStaticMeshComponent* Mesh);

	/** Applies /Game/Bending/VFX/M_Fire (emissive orange). Call at runtime (BeginPlay). */
	void ApplyFireMaterial(UStaticMeshComponent* Mesh);

	/** Applies /Game/Bending/VFX/M_Air (faint translucent white). Call at runtime (BeginPlay). */
	void ApplyAirMaterial(UStaticMeshComponent* Mesh);

	/** Applies /Game/Bending/VFX/M_FireCore (white-hot emissive) — the inner heart of a flame. */
	void ApplyFireCoreMaterial(UStaticMeshComponent* Mesh);

	/** Applies /Game/Bending/VFX/M_Ember (bright orange emissive) — sparks and trail licks. */
	void ApplyEmberMaterial(UStaticMeshComponent* Mesh);

	/** Applies /Game/Bending/VFX/M_AirBand (clearly visible pale swirl) — active air techniques. */
	void ApplyAirBandMaterial(UStaticMeshComponent* Mesh);

	/** Applies /Game/Bending/VFX/M_Lightning (blinding blue-white) — the cold-blooded fire. */
	void ApplyLightningMaterial(UStaticMeshComponent* Mesh);

	/** Tints the basic shape material an arbitrary color (habitat dressing/creatures). */
	void ApplyTintMaterial(UStaticMeshComponent* Mesh, const FLinearColor& Tint);
}
