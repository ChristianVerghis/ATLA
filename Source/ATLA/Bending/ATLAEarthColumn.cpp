// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAEarthColumn.h"
#include "ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AATLAEarthColumn::AATLAEarthColumn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Column = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Column"));
	SetRootComponent(Column);
	Column->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Column->SetStaticMesh(CylinderMesh.Object);
	}
}

void AATLAEarthColumn::BeginPlay()
{
	Super::BeginPlay();

	ATLAWaterVisuals::ApplyEarthMaterial(Column);
	SetLifeSpan(2.f);
	GroundZ = GetActorLocation().Z;
	Column->SetWorldScale3D(FVector(1.2f, 1.2f, 0.05f));
}

void AATLAEarthColumn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	// Punch up fast (0.25s), then sink back
	const float T = Age < 0.25f ? Age / 0.25f : FMath::Max(0.f, 1.f - (Age - 0.6f) / 1.2f);
	const float HeightScale = FMath::Max(2.4f * T, 0.05f);
	Column->SetWorldScale3D(FVector(1.2f, 1.2f, HeightScale));
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, GroundZ + HeightScale * 50.f));
}
