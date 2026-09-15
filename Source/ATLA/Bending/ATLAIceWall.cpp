// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAIceWall.h"
#include "ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AATLAIceWall::AATLAIceWall()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Wall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall"));
	SetRootComponent(Wall);
	Wall->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Wall->SetStaticMesh(CubeMesh.Object);
	}
}

void AATLAIceWall::BeginPlay()
{
	Super::BeginPlay();

	ATLAWaterVisuals::ApplyIceMaterial(Wall);
	SetLifeSpan(Lifetime);
	GroundZ = GetActorLocation().Z;

	// Start flat in the ground; Tick raises it
	Wall->SetWorldScale3D(FVector(WallSize.X / 100.f, WallSize.Y / 100.f, 0.02f));
}

void AATLAIceWall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	// Erupt: scale and lift so the base stays planted at ground level
	const float Rise = FMath::Clamp(Age / RiseTime, 0.05f, 1.f);
	const float HeightScale = (WallSize.Z / 100.f) * Rise;
	Wall->SetWorldScale3D(FVector(WallSize.X / 100.f, WallSize.Y / 100.f, HeightScale));
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, GroundZ + (HeightScale * 100.f) * 0.5f));

	// Melt: sink back down in the final second of life
	const float Remaining = Lifetime - Age;
	if (Remaining < 1.f && Remaining > 0.f)
	{
		const float Melt = FMath::Max(Remaining, 0.05f);
		Wall->SetWorldScale3D(FVector(WallSize.X / 100.f, WallSize.Y / 100.f, HeightScale * Melt));
		SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, GroundZ + (HeightScale * Melt * 100.f) * 0.5f));
	}
}

void AATLAIceWall::ApplyStructureDamage(float Amount)
{
	if (!HasAuthority())
	{
		return;
	}
	StructureHP -= Amount;
	if (StructureHP <= 0.f)
	{
		Destroy();
	}
}
