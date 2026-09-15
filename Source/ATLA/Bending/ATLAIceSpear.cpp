// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAIceSpear.h"
#include "ATLAGameplayEffects.h"
#include "ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AATLAIceSpear::AATLAIceSpear()
{
	// Ice flies fast and dead straight — no weave, no arc
	Movement->InitialSpeed = 3000.f;
	Movement->MaxSpeed = 3000.f;
	Movement->ProjectileGravityScale = 0.f;
	SerpentineAmplitude = 0.f;

	// A real icicle: cone mesh, apex pointing along the flight direction
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(ConeMesh.Object);
	}
	Mesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Mesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 1.25f));

	// Sparse frost dust instead of the whip's fat water trail
	TrailDropScale = 0.07f;

	DamageEffect = UGE_Damage_IceSpear::StaticClass();
	StructureDamage = 8.f;
	InitialLifeSpan = 2.f;
}

void AATLAIceSpear::ApplyMaterials()
{
	ATLAWaterVisuals::ApplyIceMaterial(Mesh);
	for (UStaticMeshComponent* Drop : TrailDrops)
	{
		ATLAWaterVisuals::ApplyIceMaterial(Drop);
	}
}
