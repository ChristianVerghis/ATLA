// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAEarthBurst.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AATLAEarthBurst::AATLAEarthBurst()
{
	// Real dirt explosion on every earth impact (auto-activates on spawn)
	DirtFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DirtFX"));
	DirtFX->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DirtSystem(TEXT("/Game/NiagaraExamples/FX_Explosions/NS_Dirt_Explosion_Small.NS_Dirt_Explosion_Small"));
	if (DirtSystem.Succeeded())
	{
		DirtFX->SetAsset(DirtSystem.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// No translucent shell for rock — hide it; debris is angular chunks
	Shell->SetRelativeScale3D(FVector(0.01f));
	Shell->SetVisibility(false);

	int32 i = 0;
	for (UStaticMeshComponent* Drop : Droplets)
	{
		if (CubeMesh.Succeeded())
		{
			Drop->SetStaticMesh(CubeMesh.Object);
		}
		Drop->SetRelativeRotation(FRotator(i * 37.f, i * 53.f, i * 21.f));
		++i;
	}
}

void AATLAEarthBurst::ApplyMaterials()
{
	for (UStaticMeshComponent* Drop : Droplets)
	{
		ATLAWaterVisuals::ApplyEarthMaterial(Drop);
	}
}
