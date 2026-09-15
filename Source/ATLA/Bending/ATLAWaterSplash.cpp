// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAWaterSplash.h"
#include "ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	constexpr int32 NumDroplets = 8;
	constexpr float ShellPopTime = 0.35f;
}

AATLAWaterSplash::AATLAWaterSplash()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	InitialLifeSpan = 0.8f;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Shell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shell"));
	Shell->SetupAttachment(RootComponent);
	Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shell->SetRelativeScale3D(FVector(0.3f));
	ATLAWaterVisuals::SetupWaterMesh(Shell);

	for (int32 i = 0; i < NumDroplets; ++i)
	{
		UStaticMeshComponent* Drop = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Droplet%d"), i));
		Drop->SetupAttachment(RootComponent);
		Drop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Drop->SetUsingAbsoluteLocation(true);
		Drop->SetUsingAbsoluteScale(true);
		Drop->SetRelativeScale3D(FVector(0.12f));
		ATLAWaterVisuals::SetupWaterMesh(Drop);
		Droplets.Add(Drop);
	}
}

void AATLAWaterSplash::BeginPlay()
{
	Super::BeginPlay();

	ApplyMaterials();

	for (UStaticMeshComponent* Drop : Droplets)
	{
		Drop->SetWorldLocation(GetActorLocation());
		Drop->SetWorldScale3D(DropletShape);

		// Scatter into the upper hemisphere, shaped by the element's knobs
		FVector Dir = FMath::VRand();
		Dir.Z = (FMath::Abs(Dir.Z) + DropletUpBias) * DropletZMul;
		DropletVelocities.Add(Dir.GetSafeNormal() * FMath::FRandRange(DropletSpeedMin, DropletSpeedMax));
	}
}

void AATLAWaterSplash::ApplyMaterials()
{
	ATLAWaterVisuals::ApplyWaterMaterial(Shell);
	for (UStaticMeshComponent* Drop : Droplets)
	{
		ATLAWaterVisuals::ApplyWaterMaterial(Drop);
	}
}

void AATLAWaterSplash::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	// Shell expands fast then pops
	if (Age < ShellPopTime)
	{
		Shell->SetRelativeScale3D(Shell->GetRelativeScale3D() + FVector(DeltaTime * 5.f));
	}
	else
	{
		Shell->SetVisibility(false);
	}

	// Droplets fly out and shrink away; gravity sign is the element's
	// signature (water falls, embers rise, air just disperses)
	for (int32 i = 0; i < Droplets.Num(); ++i)
	{
		DropletVelocities[i].Z += DropletGravityZ * DeltaTime;
		Droplets[i]->AddWorldOffset(DropletVelocities[i] * DeltaTime);
		Droplets[i]->SetWorldScale3D(Droplets[i]->GetComponentScale() * (1.f - DeltaTime * DropletShrinkRate));
	}
}
