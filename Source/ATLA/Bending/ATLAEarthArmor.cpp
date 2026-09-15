// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAEarthArmor.h"
#include "ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	// Limb segments: an ellipsoid stretched between two bones — placed at the
	// midpoint and oriented along the limb, so left/right mirroring can't
	// misplace anything. Girth = the clay coat's thickness around that limb.
	struct FSegmentDef
	{
		FName BoneA;
		FName BoneB;
		float Girth;
	};
	const FSegmentDef SegmentDefs[] = {
		{ TEXT("thigh_l"), TEXT("calf_l"), 0.24f },
		{ TEXT("calf_l"), TEXT("foot_l"), 0.20f },
		{ TEXT("thigh_r"), TEXT("calf_r"), 0.24f },
		{ TEXT("calf_r"), TEXT("foot_r"), 0.20f },
		{ TEXT("upperarm_l"), TEXT("lowerarm_l"), 0.19f },
		{ TEXT("lowerarm_l"), TEXT("hand_l"), 0.16f },
		{ TEXT("upperarm_r"), TEXT("lowerarm_r"), 0.19f },
		{ TEXT("lowerarm_r"), TEXT("hand_r"), 0.16f },
		{ TEXT("spine_01"), TEXT("spine_03"), 0.40f },
		{ TEXT("spine_03"), TEXT("spine_05"), 0.44f },
	};

	// Single-bone blobs: rounded clay lumps at joints and extremities
	struct FBlobDef
	{
		FName Bone;
		FVector Scale;
	};
	const FBlobDef BlobDefs[] = {
		{ TEXT("head"), FVector(0.30f, 0.32f, 0.34f) },
		{ TEXT("neck_01"), FVector(0.18f, 0.20f, 0.18f) },
		{ TEXT("pelvis"), FVector(0.34f, 0.44f, 0.32f) },
		{ TEXT("clavicle_l"), FVector(0.20f, 0.24f, 0.20f) },
		{ TEXT("clavicle_r"), FVector(0.20f, 0.24f, 0.20f) },
		{ TEXT("hand_l"), FVector(0.17f, 0.15f, 0.13f) },
		{ TEXT("hand_r"), FVector(0.17f, 0.15f, 0.13f) },
		{ TEXT("foot_l"), FVector(0.22f, 0.14f, 0.12f) },
		{ TEXT("foot_r"), FVector(0.22f, 0.14f, 0.12f) },
	};
	constexpr int32 NumSegments = UE_ARRAY_COUNT(SegmentDefs);
	constexpr int32 NumBlobs = UE_ARRAY_COUNT(BlobDefs);
}

AATLAEarthArmor::AATLAEarthArmor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	// Rounded clay: everything is an ellipsoid (engine sphere, scaled)
	for (int32 i = 0; i < NumSegments + NumBlobs; ++i)
	{
		UStaticMeshComponent* Plate = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Plate%d"), i));
		Plate->SetupAttachment(RootComponent);
		Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Plate->SetUsingAbsoluteLocation(true);
		Plate->SetUsingAbsoluteRotation(true);
		Plate->SetUsingAbsoluteScale(true);
		ATLAWaterVisuals::SetupWaterMesh(Plate);
		Plates.Add(Plate);
	}
}

void AATLAEarthArmor::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(Duration);

	for (UStaticMeshComponent* Plate : Plates)
	{
		ATLAWaterVisuals::ApplyEarthMaterial(Plate);
	}

	if (ACharacter* Bender = Cast<ACharacter>(GetInstigator()))
	{
		AttachToActor(Bender, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SavedWalkSpeed = Bender->GetCharacterMovement()->MaxWalkSpeed;
		Bender->GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed * MoveSpeedMultiplier;
	}
}

void AATLAEarthArmor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACharacter* Bender = Cast<ACharacter>(GetInstigator()))
	{
		if (SavedWalkSpeed > 0.f)
		{
			Bender->GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed;
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AATLAEarthArmor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const ACharacter* Bender = Cast<ACharacter>(GetInstigator());
	if (!Bender || !Bender->GetMesh())
	{
		return;
	}
	const USkeletalMeshComponent* Mesh = Bender->GetMesh();

	// Limb segments: midpoint between bone pairs, stretched along the limb
	for (int32 i = 0; i < NumSegments; ++i)
	{
		const FSegmentDef& Def = SegmentDefs[i];
		const FVector A = Mesh->GetSocketLocation(Def.BoneA);
		const FVector B = Mesh->GetSocketLocation(Def.BoneB);
		const FVector Dir = B - A;
		const float Len = FMath::Max(Dir.Size(), 1.f);

		Plates[i]->SetWorldLocation((A + B) * 0.5f);
		Plates[i]->SetWorldRotation(Dir.Rotation());
		Plates[i]->SetWorldScale3D(FVector(Len / 100.f * 0.72f, Def.Girth, Def.Girth));
	}

	// Blobs: rounded lumps riding single bones
	for (int32 i = 0; i < NumBlobs; ++i)
	{
		const FBlobDef& Def = BlobDefs[i];
		const FTransform BoneTM = Mesh->GetSocketTransform(Def.Bone);
		Plates[NumSegments + i]->SetWorldLocation(BoneTM.GetLocation());
		Plates[NumSegments + i]->SetWorldRotation(BoneTM.GetRotation().Rotator());
		Plates[NumSegments + i]->SetWorldScale3D(Def.Scale);
	}
}
