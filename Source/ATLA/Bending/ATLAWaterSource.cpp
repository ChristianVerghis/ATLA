// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAWaterSource.h"
#include "ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AATLAWaterSource::AATLAWaterSource()
{
	PrimaryActorTick.bCanEverTick = false;

	Pool = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pool"));
	SetRootComponent(Pool);
	Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pool->SetRelativeScale3D(FVector(6.f, 6.f, 0.12f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Pool->SetStaticMesh(CylinderMesh.Object);
	}
}

void AATLAWaterSource::BeginPlay()
{
	Super::BeginPlay();

	ATLAWaterVisuals::ApplyWaterMaterial(Pool);
}

AATLAWaterSource* AATLAWaterSource::FindSourceInRange(UWorld* World, const FVector& Location)
{
	AATLAWaterSource* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<AATLAWaterSource> It(World); It; ++It)
	{
		const float DistSq = FVector::DistSquared(It->GetActorLocation(), Location);
		if (DistSq <= FMath::Square(It->DrawRange) && DistSq < NearestDistSq)
		{
			Nearest = *It;
			NearestDistSq = DistSq;
		}
	}
	return Nearest;
}
