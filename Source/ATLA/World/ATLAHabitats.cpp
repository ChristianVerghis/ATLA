// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAHabitats.h"
#include "Bending/ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* SpherePath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* ConePath = TEXT("/Engine/BasicShapes/Cone.Cone");

	const FLinearColor StoneGrey(0.16f, 0.15f, 0.14f);
	const FLinearColor StandsRed(0.28f, 0.06f, 0.05f);
	const FLinearColor DeckGold(0.75f, 0.55f, 0.18f);
	const FLinearColor KoiWhite(0.95f, 0.95f, 0.98f);
	const FLinearColor KoiBlack(0.03f, 0.03f, 0.05f);
	const FLinearColor MoleBrown(0.30f, 0.18f, 0.08f);
	const FLinearColor DragonRed(0.65f, 0.10f, 0.05f);
	const FLinearColor DragonBlue(0.12f, 0.15f, 0.60f);
	const FLinearColor BisonWhite(0.92f, 0.90f, 0.85f);
	const FLinearColor BisonBrown(0.35f, 0.22f, 0.10f);
}

// ---- Coliseum ----

AATLAColiseum::AATLAColiseum()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(CubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(CylinderPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(SpherePath);

	auto Make = [this](UStaticMesh* Mesh, const TCHAR* Name, const FVector& Rel, const FVector& Scale, const FRotator& Rot, bool bCollide) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(RootComponent);
		if (Mesh)
		{
			C->SetStaticMesh(Mesh);
		}
		C->SetRelativeLocation(Rel);
		C->SetRelativeScale3D(Scale);
		C->SetRelativeRotation(Rot);
		C->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		return C;
	};

	// Water basin floor (an ellipse read: squashed cylinder), walkable
	Basin = Make(Cylinder.Object, TEXT("Basin"), FVector(0.f, 0.f, 40.f), FVector(96.f, 78.f, 0.8f), FRotator::ZeroRotator, true);

	// Ring wall + two seating tiers, 24 segments around an ellipse
	int32 Index = 0;
	for (int32 i = 0; i < 24; ++i)
	{
		const float A = i * 2.f * PI / 24.f;
		const float CosA = FMath::Cos(A);
		const float SinA = FMath::Sin(A);
		const float Yaw = FMath::RadiansToDegrees(A) + 90.f;

		// wall
		Stone.Add(Make(Cube.Object, *FString::Printf(TEXT("Wall%d"), Index++),
			FVector(CosA * 4800.f, SinA * 3900.f, 330.f), FVector(1.6f, 13.f, 6.6f), FRotator(0.f, Yaw, 0.f), true));
		// lower stands (tilted slabs behind the wall)
		Stone.Add(Make(Cube.Object, *FString::Printf(TEXT("StandA%d"), Index++),
			FVector(CosA * 5450.f, SinA * 4500.f, 620.f), FVector(6.f, 13.5f, 0.7f), FRotator(0.f, Yaw, 14.f * (CosA >= 0.f ? 1.f : 1.f)), false));
		// upper stands
		Stone.Add(Make(Cube.Object, *FString::Printf(TEXT("StandB%d"), Index++),
			FVector(CosA * 6100.f, SinA * 5100.f, 950.f), FVector(6.f, 14.f, 0.7f), FRotator(0.f, Yaw, 14.f), false));
	}

	// Central platform on stilts (the hex deck)
	Decks.Add(Make(Cylinder.Object, TEXT("CenterDeck"), FVector(0.f, 0.f, 360.f), FVector(16.f, 16.f, 0.7f), FRotator::ZeroRotator, true));
	for (int32 i = 0; i < 4; ++i)
	{
		const float A = i * PI / 2.f + PI / 4.f;
		Stone.Add(Make(Cylinder.Object, *FString::Printf(TEXT("Stilt%d"), i),
			FVector(FMath::Cos(A) * 560.f, FMath::Sin(A) * 560.f, 190.f), FVector(0.9f, 0.9f, 3.2f), FRotator::ZeroRotator, true));
	}

	// Three side platforms with their towers (the reference's judge stands)
	for (int32 i = 0; i < 3; ++i)
	{
		const float A = i * 2.f * PI / 3.f + PI / 6.f;
		const FVector Base(FMath::Cos(A) * 2400.f, FMath::Sin(A) * 1950.f, 0.f);
		Decks.Add(Make(Cube.Object, *FString::Printf(TEXT("SideDeck%d"), i),
			Base + FVector(0.f, 0.f, 430.f), FVector(4.6f, 4.6f, 0.5f), FRotator::ZeroRotator, true));
		Stone.Add(Make(Cube.Object, *FString::Printf(TEXT("SideTowerA%d"), i),
			Base + FVector(150.f, 150.f, 220.f), FVector(0.5f, 0.5f, 4.2f), FRotator::ZeroRotator, true));
		Stone.Add(Make(Cube.Object, *FString::Printf(TEXT("SideTowerB%d"), i),
			Base + FVector(-150.f, -150.f, 220.f), FVector(0.5f, 0.5f, 4.2f), FRotator::ZeroRotator, true));
	}

	// Chandelier: a glowing orb high over the deck
	Lamp = Make(Sphere.Object, TEXT("Lamp"), FVector(0.f, 0.f, 2400.f), FVector(3.4f), FRotator::ZeroRotator, false);
}

void AATLAColiseum::BeginPlay()
{
	Super::BeginPlay();

	ATLAWaterVisuals::ApplyWaterMaterial(Basin);
	for (UStaticMeshComponent* S : Stone)
	{
		const bool bStands = S->GetName().Contains(TEXT("Stand"));
		ATLAWaterVisuals::ApplyTintMaterial(S, bStands ? StandsRed : StoneGrey);
	}
	for (UStaticMeshComponent* D : Decks)
	{
		ATLAWaterVisuals::ApplyTintMaterial(D, DeckGold);
	}
	ATLAWaterVisuals::ApplyFireCoreMaterial(Lamp);
}

FVector AATLAColiseum::GetPlatformTop() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 360.f + 120.f);
}

// ---- Habitat zones ----

AATLAHabitatZone::AATLAHabitatZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

UStaticMeshComponent* AATLAHabitatZone::Piece(const TCHAR* Mesh, const FVector& Rel, const FVector& Scale, const FRotator& Rot, const FLinearColor& Tint, bool bCollide)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetupAttachment(GetRootComponent());
	C->RegisterComponent();
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, Mesh))
	{
		C->SetStaticMesh(M);
	}
	C->SetRelativeLocation(Rel);
	C->SetRelativeScale3D(Scale);
	C->SetRelativeRotation(Rot);
	C->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	ATLAWaterVisuals::ApplyTintMaterial(C, Tint);
	Pieces.Add(C);
	return C;
}

void AATLAHabitatZone::BeginPlay()
{
	Super::BeginPlay();
	BuildZone();
}

void AATLAHabitatZone::BuildZone()
{
	const FLinearColor Ice(0.75f, 0.88f, 0.98f);
	const FLinearColor Snow(0.93f, 0.95f, 0.99f);
	const FLinearColor WaterBlue(0.06f, 0.25f, 0.55f);
	const FLinearColor Rock(0.32f, 0.24f, 0.14f);
	const FLinearColor RockDark(0.2f, 0.15f, 0.09f);
	const FLinearColor Obsidian(0.08f, 0.06f, 0.06f);
	const FLinearColor Banner(0.55f, 0.08f, 0.05f);
	const FLinearColor AirWhite(0.92f, 0.94f, 0.99f);
	const FLinearColor TempleTan(0.78f, 0.7f, 0.55f);

	FRandomStream Rand(GetUniqueID());

	switch (Element)
	{
	case 0: // Water: the most detailed — spirit oasis + floes + ice crescent
	{
		// The oasis pond: a broad shallow water disc where the koi circle
		Piece(CylinderPath, FVector(900.f, 900.f, -6.f), FVector(24.f, 24.f, 0.3f), FRotator::ZeroRotator, WaterBlue, true);
		// grass ring around the pond (the oasis's green lip)
		Piece(CylinderPath, FVector(900.f, 900.f, -14.f), FVector(28.f, 28.f, 0.25f), FRotator::ZeroRotator, FLinearColor(0.2f, 0.4f, 0.15f), true);
		// Torii-style oasis gate
		Piece(CubePath, FVector(560.f, 560.f, 150.f), FVector(0.35f, 0.35f, 3.2f), FRotator::ZeroRotator, TempleTan, true);
		Piece(CubePath, FVector(760.f, 360.f, 150.f), FVector(0.35f, 0.35f, 3.2f), FRotator::ZeroRotator, TempleTan, true);
		Piece(CubePath, FVector(660.f, 460.f, 330.f), FVector(0.4f, 4.4f, 0.35f), FRotator(0.f, -45.f, 0.f), TempleTan, false);
		// Crescent of ice wall panels sheltering the oasis (the polar city read)
		for (int32 i = 0; i < 9; ++i)
		{
			const float A = PI * 0.55f + i * 0.16f;
			Piece(CubePath, FVector(FMath::Cos(A) * 3400.f, FMath::Sin(A) * 3400.f, 260.f),
				FVector(1.2f, 8.5f, 6.f), FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, 0.f), Ice, true);
		}
		// Drifting ice floes on the big pools
		for (int32 i = 0; i < 10; ++i)
		{
			const float A = Rand.FRandRange(0.f, 2.f * PI);
			const float R = Rand.FRandRange(1200.f, 3200.f);
			UStaticMeshComponent* Floe = Piece(CylinderPath,
				FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 6.f),
				FVector(Rand.FRandRange(2.2f, 5.f), Rand.FRandRange(2.2f, 5.f), 0.25f),
				FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), Ice, true);
			Drifters.Add(Floe);
			DriftHomes.Add(Floe->GetRelativeLocation());
		}
		// Snow mounds
		for (int32 i = 0; i < 8; ++i)
		{
			const float A = Rand.FRandRange(0.f, 2.f * PI);
			const float R = Rand.FRandRange(2000.f, 3800.f);
			Piece(SpherePath, FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, -60.f),
				FVector(Rand.FRandRange(3.f, 7.f), Rand.FRandRange(3.f, 7.f), Rand.FRandRange(1.2f, 2.f)),
				FRotator::ZeroRotator, Snow, true);
		}
		break;
	}
	case 1: // Earth: pillar ring, boulders, cave arch
	{
		for (int32 i = 0; i < 10; ++i)
		{
			const float A = i * 2.f * PI / 10.f + Rand.FRandRange(-0.1f, 0.1f);
			const float R = Rand.FRandRange(2600.f, 3900.f);
			Piece(CylinderPath, FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, Rand.FRandRange(200.f, 700.f)),
				FVector(Rand.FRandRange(2.f, 4.f), Rand.FRandRange(2.f, 4.f), Rand.FRandRange(8.f, 20.f)),
				FRotator(Rand.FRandRange(-6.f, 6.f), 0.f, Rand.FRandRange(-6.f, 6.f)), Rock, true);
		}
		for (int32 i = 0; i < 12; ++i)
		{
			const float A = Rand.FRandRange(0.f, 2.f * PI);
			const float R = Rand.FRandRange(1200.f, 3600.f);
			Piece(SpherePath, FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 40.f),
				FVector(Rand.FRandRange(1.5f, 4.5f), Rand.FRandRange(1.5f, 4.f), Rand.FRandRange(1.2f, 3.f)),
				FRotator(Rand.FRandRange(0.f, 30.f), Rand.FRandRange(0.f, 360.f), 0.f), RockDark, true);
		}
		// The badgermole cave: two leaning slabs and a lintel
		Piece(CubePath, FVector(1500.f, 1000.f, 260.f), FVector(1.5f, 6.f, 6.f), FRotator(0.f, 20.f, 14.f), RockDark, true);
		Piece(CubePath, FVector(1300.f, 1500.f, 260.f), FVector(1.5f, 6.f, 6.f), FRotator(0.f, 20.f, -14.f), RockDark, true);
		Piece(CubePath, FVector(1400.f, 1250.f, 560.f), FVector(2.f, 7.f, 1.4f), FRotator(0.f, 20.f, 0.f), Rock, true);
		break;
	}
	case 2: // Fire: obsidian pillars, braziers, banners
	{
		for (int32 i = 0; i < 8; ++i)
		{
			const float A = i * 2.f * PI / 8.f;
			const float R = 3000.f;
			const FVector Base(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f);
			Piece(CubePath, Base + FVector(0.f, 0.f, 450.f), FVector(1.4f, 1.4f, 9.f), FRotator::ZeroRotator, Obsidian, true);
			// brazier flame on top (ember-tinted cone, flickered in Tick)
			UStaticMeshComponent* Flame = Piece(ConePath, Base + FVector(0.f, 0.f, 960.f), FVector(1.2f, 1.2f, 1.8f),
				FRotator::ZeroRotator, FLinearColor(1.f, 0.45f, 0.05f), false);
			Drifters.Add(Flame);
			DriftHomes.Add(Flame->GetRelativeLocation());
			// hanging banner
			Piece(CubePath, Base + FVector(0.f, 0.f, 620.f) - Base.GetSafeNormal2D() * 90.f,
				FVector(0.08f, 1.6f, 4.f), FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, 0.f), Banner, false);
		}
		break;
	}
	default: // Air: temple spires, floating cloud discs, meditation circle
	{
		for (int32 i = 0; i < 5; ++i)
		{
			const float A = i * 2.f * PI / 5.f;
			const FVector Base(FMath::Cos(A) * 3200.f, FMath::Sin(A) * 3200.f, 0.f);
			Piece(CylinderPath, Base + FVector(0.f, 0.f, 500.f), FVector(2.4f, 2.4f, 10.f), FRotator::ZeroRotator, TempleTan, true);
			Piece(ConePath, Base + FVector(0.f, 0.f, 1120.f), FVector(3.2f, 3.2f, 2.6f), FRotator::ZeroRotator, FLinearColor(0.25f, 0.45f, 0.75f), false);
		}
		for (int32 i = 0; i < 9; ++i)
		{
			const float A = Rand.FRandRange(0.f, 2.f * PI);
			const float R = Rand.FRandRange(1000.f, 3600.f);
			UStaticMeshComponent* Cloud = Piece(CylinderPath,
				FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, Rand.FRandRange(500.f, 1500.f)),
				FVector(Rand.FRandRange(2.5f, 5.f), Rand.FRandRange(2.f, 4.f), 0.4f),
				FRotator::ZeroRotator, AirWhite, true);   // walkable cloud steps
			Drifters.Add(Cloud);
			DriftHomes.Add(Cloud->GetRelativeLocation());
		}
		// Meditation circle of low stones
		for (int32 i = 0; i < 6; ++i)
		{
			const float A = i * PI / 3.f;
			Piece(CylinderPath, FVector(FMath::Cos(A) * 500.f, FMath::Sin(A) * 500.f, 25.f),
				FVector(0.8f, 0.8f, 0.5f), FRotator::ZeroRotator, TempleTan, true);
		}
		break;
	}
	}
}

void AATLAHabitatZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	// Drifters breathe: floes bob, brazier flames flicker, clouds wander
	for (int32 i = 0; i < Drifters.Num() && i < DriftHomes.Num(); ++i)
	{
		if (!Drifters[i]) continue;
		if (Element == 2)
		{
			const float F = 1.f + 0.18f * FMath::Sin(Age * 9.f + i * 1.7f) + 0.08f * FMath::Sin(Age * 23.f + i);
			Drifters[i]->SetRelativeScale3D(FVector(1.2f * F, 1.2f * F, 1.8f * F));
		}
		else
		{
			Drifters[i]->SetRelativeLocation(DriftHomes[i] + FVector(
				40.f * FMath::Sin(Age * 0.15f + i * 1.3f),
				40.f * FMath::Cos(Age * 0.12f + i * 0.9f),
				(Element == 3 ? 25.f : 6.f) * FMath::Sin(Age * 0.5f + i)));
		}
	}
}

// ---- Habitat creatures ----

AATLAHabitatCreature::AATLAHabitatCreature()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

UStaticMeshComponent* AATLAHabitatCreature::AddPart(const TCHAR* Mesh, const FVector& Rel, const FVector& Scale, const FRotator& Rot, USceneComponent* Parent)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetupAttachment(Parent ? Parent : GetRootComponent());
	C->RegisterComponent();
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, Mesh))
	{
		C->SetStaticMesh(M);
	}
	C->SetRelativeLocation(Rel);
	C->SetRelativeScale3D(Scale);
	C->SetRelativeRotation(Rot);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Parts.Add(C);
	return C;
}

void AATLAHabitatCreature::BeginPlay()
{
	Super::BeginPlay();
	BuildBody();
}

void AATLAHabitatCreature::BuildBody()
{
	auto Mover = [this](const TCHAR* Name) -> USceneComponent*
	{
		USceneComponent* M = NewObject<USceneComponent>(this, Name);
		M->SetupAttachment(RootComponent);
		M->RegisterComponent();
		Movers.Add(M);
		return M;
	};

	switch (Style)
	{
	case EATLACreatureStyle::KoiPair:
	{
		// Tui and La: the push and the pull, forever circling
		for (int32 k = 0; k < 2; ++k)
		{
			USceneComponent* Fish = Mover(*FString::Printf(TEXT("Koi%d"), k));
			const FLinearColor Body = (k == 0) ? KoiWhite : KoiBlack;
			const FLinearColor Spot = (k == 0) ? KoiBlack : KoiWhite;
			UStaticMeshComponent* Torso = AddPart(SpherePath, FVector::ZeroVector, FVector(2.6f, 0.9f, 0.8f), FRotator::ZeroRotator, Fish);
			ATLAWaterVisuals::ApplyTintMaterial(Torso, Body);
			UStaticMeshComponent* Tail = AddPart(ConePath, FVector(-150.f, 0.f, 0.f), FVector(0.7f, 0.25f, 0.9f), FRotator(0.f, 0.f, 90.f), Fish);
			ATLAWaterVisuals::ApplyTintMaterial(Tail, Body);
			UStaticMeshComponent* Eye = AddPart(SpherePath, FVector(70.f, 0.f, 28.f), FVector(0.5f, 0.5f, 0.3f), FRotator::ZeroRotator, Fish);
			ATLAWaterVisuals::ApplyTintMaterial(Eye, Spot);
		}
		break;
	}
	case EATLACreatureStyle::Badgermole:
	{
		USceneComponent* Mole = Mover(TEXT("Mole"));
		UStaticMeshComponent* Body = AddPart(SpherePath, FVector(0.f, 0.f, 210.f), FVector(6.5f, 4.6f, 4.2f), FRotator::ZeroRotator, Mole);
		ATLAWaterVisuals::ApplyTintMaterial(Body, MoleBrown);
		UStaticMeshComponent* Head = AddPart(SpherePath, FVector(340.f, 0.f, 150.f), FVector(2.8f, 2.2f, 2.0f), FRotator::ZeroRotator, Mole);
		ATLAWaterVisuals::ApplyTintMaterial(Head, MoleBrown);
		UStaticMeshComponent* Stripe = AddPart(SpherePath, FVector(430.f, 0.f, 190.f), FVector(1.5f, 0.6f, 0.9f), FRotator::ZeroRotator, Mole);
		ATLAWaterVisuals::ApplyTintMaterial(Stripe, KoiWhite);
		for (int32 i = 0; i < 4; ++i)
		{
			UStaticMeshComponent* Claw = AddPart(ConePath, FVector(430.f, -70.f + i * 46.f, 20.f), FVector(0.25f, 0.18f, 0.8f), FRotator(-80.f, 0.f, 0.f), Mole);
			ATLAWaterVisuals::ApplyTintMaterial(Claw, KoiWhite);
		}
		break;
	}
	case EATLACreatureStyle::Dragons:
	{
		// Two dragons flying their entwined circle (Ran and Shaw): long tapered
		// serpents with a ridge of dorsal spikes, cream bellies, horned heads
		// with whiskers, and four tucked legs
		for (int32 k = 0; k < 2; ++k)
		{
			const FLinearColor Body = (k == 0) ? DragonRed : DragonBlue;
			const FLinearColor Belly(0.85f, 0.78f, 0.62f);
			TArray<TObjectPtr<UStaticMeshComponent>>& Spine = (k == 0) ? DragonA : DragonB;
			for (int32 i = 0; i < 18; ++i)
			{
				const float Taper = 1.45f - i * 0.066f;   // bigger beasts, sharper taper
				UStaticMeshComponent* Seg = AddPart(SpherePath, FVector::ZeroVector,
					FVector(1.5f * Taper, 0.85f * Taper, 0.8f * Taper), FRotator::ZeroRotator, RootComponent);
				ATLAWaterVisuals::ApplyTintMaterial(Seg, Body);
				Spine.Add(Seg);

				// ride-along details parent to their segment: dorsal spike + belly
				UStaticMeshComponent* Spike = AddPart(ConePath, FVector(0.f, 0.f, 34.f * Taper),
					FVector(0.16f, 0.3f, 0.5f) * Taper, FRotator::ZeroRotator, Seg);
				ATLAWaterVisuals::ApplyTintMaterial(Spike, Belly);
				UStaticMeshComponent* BellyPatch = AddPart(SpherePath, FVector(0.f, 0.f, -18.f * Taper),
					FVector(0.75f, 0.55f, 0.35f), FRotator::ZeroRotator, Seg);
				ATLAWaterVisuals::ApplyTintMaterial(BellyPatch, Belly);

				if (i == 0)
				{
					// The head rides the lead segment: horns, snout, whiskers, eyes
					UStaticMeshComponent* Snout = AddPart(SpherePath, FVector(55.f, 0.f, -4.f), FVector(0.8f, 0.5f, 0.4f), FRotator::ZeroRotator, Seg);
					ATLAWaterVisuals::ApplyTintMaterial(Snout, Body);
					for (int32 h = 0; h < 2; ++h)
					{
						const float Side = (h == 0) ? 1.f : -1.f;
						UStaticMeshComponent* Horn = AddPart(ConePath, FVector(-12.f, Side * 22.f, 36.f),
							FVector(0.14f, 0.14f, 0.55f), FRotator(-35.f, 0.f, Side * 18.f), Seg);
						ATLAWaterVisuals::ApplyTintMaterial(Horn, Belly);
						UStaticMeshComponent* Whisker = AddPart(CylinderPath, FVector(58.f, Side * 16.f, 2.f),
							FVector(0.05f, 0.05f, 1.1f), FRotator(80.f, Side * 25.f, 0.f), Seg);
						ATLAWaterVisuals::ApplyTintMaterial(Whisker, Belly);
						UStaticMeshComponent* Eye = AddPart(SpherePath, FVector(28.f, Side * 20.f, 16.f),
							FVector(0.16f), FRotator::ZeroRotator, Seg);
						ATLAWaterVisuals::ApplyEmberMaterial(Eye);   // burning eyes
						// gold frill fanning behind the head
						UStaticMeshComponent* Frill = AddPart(ConePath, FVector(-38.f, Side * 26.f, 22.f),
							FVector(0.12f, 0.35f, 0.7f), FRotator(-25.f, Side * 55.f, 0.f), Seg);
						ATLAWaterVisuals::ApplyTintMaterial(Frill, FLinearColor(0.85f, 0.65f, 0.15f));
					}
					// a tongue of flame at the jaw, flickered by the flight code
					UStaticMeshComponent* Breath = AddPart(ConePath, FVector(95.f, 0.f, -8.f),
						FVector(0.3f, 0.3f, 0.8f), FRotator(-90.f, 0.f, 0.f), Seg);
					ATLAWaterVisuals::ApplyFireMaterial(Breath);
				}
				if (i == 3 || i == 8)
				{
					// Two pairs of small tucked legs along the body
					for (int32 l = 0; l < 2; ++l)
					{
						const float Side = (l == 0) ? 1.f : -1.f;
						UStaticMeshComponent* Leg = AddPart(CylinderPath, FVector(0.f, Side * 34.f, -26.f),
							FVector(0.16f, 0.16f, 0.6f), FRotator(20.f, 0.f, Side * 30.f), Seg);
						ATLAWaterVisuals::ApplyTintMaterial(Leg, Body);
					}
				}
			}
		}
		break;
	}
	case EATLACreatureStyle::SkyBison:
	{
		// Appa: massive fluffy body, six dangling legs, the flat beaver tail,
		// the brown arrow running head-to-tail, curved horns, a real face
		USceneComponent* Bison = Mover(TEXT("Bison"));
		UStaticMeshComponent* Body = AddPart(SpherePath, FVector::ZeroVector, FVector(8.2f, 5.2f, 4.0f), FRotator::ZeroRotator, Bison);
		ATLAWaterVisuals::ApplyTintMaterial(Body, BisonWhite);
		// underbelly fluff (brown, like the show's underside)
		UStaticMeshComponent* Under = AddPart(SpherePath, FVector(0.f, 0.f, -90.f), FVector(7.6f, 4.6f, 2.6f), FRotator::ZeroRotator, Bison);
		ATLAWaterVisuals::ApplyTintMaterial(Under, BisonBrown);

		// Head: a broad flat face at the front
		UStaticMeshComponent* Head = AddPart(SpherePath, FVector(430.f, 0.f, 60.f), FVector(3.4f, 3.8f, 2.4f), FRotator::ZeroRotator, Bison);
		ATLAWaterVisuals::ApplyTintMaterial(Head, BisonWhite);
		for (int32 e = 0; e < 2; ++e)
		{
			const float Side = (e == 0) ? 1.f : -1.f;
			UStaticMeshComponent* Eye = AddPart(SpherePath, FVector(560.f, Side * 90.f, 90.f), FVector(0.28f), FRotator::ZeroRotator, Bison);
			ATLAWaterVisuals::ApplyTintMaterial(Eye, KoiBlack);
			// Curved horn: two cones chained at an angle
			UStaticMeshComponent* HornA = AddPart(ConePath, FVector(430.f, Side * 170.f, 150.f),
				FVector(0.45f, 0.45f, 0.9f), FRotator(55.f, Side * 20.f, 0.f), Bison);
			ATLAWaterVisuals::ApplyTintMaterial(HornA, BisonBrown);
			UStaticMeshComponent* HornB = AddPart(ConePath, FVector(505.f, Side * 205.f, 205.f),
				FVector(0.28f, 0.28f, 0.6f), FRotator(85.f, Side * 30.f, 0.f), Bison);
			ATLAWaterVisuals::ApplyTintMaterial(HornB, BisonBrown);
		}
		// Mouth line
		UStaticMeshComponent* Mouth = AddPart(CubePath, FVector(590.f, 0.f, -10.f), FVector(0.15f, 2.4f, 0.06f), FRotator::ZeroRotator, Bison);
		ATLAWaterVisuals::ApplyTintMaterial(Mouth, KoiBlack);

		// The arrow: shaft down the spine, triangular head on the forehead
		UStaticMeshComponent* Shaft = AddPart(CubePath, FVector(-60.f, 0.f, 195.f), FVector(7.2f, 0.8f, 0.08f), FRotator::ZeroRotator, Bison);
		ATLAWaterVisuals::ApplyTintMaterial(Shaft, BisonBrown);
		UStaticMeshComponent* ArrowHead = AddPart(ConePath, FVector(470.f, 0.f, 150.f), FVector(1.5f, 1.9f, 0.5f), FRotator(0.f, 0.f, 90.f), Bison);
		ATLAWaterVisuals::ApplyTintMaterial(ArrowHead, BisonBrown);

		// Six legs, brown-striped feet
		for (int32 i = 0; i < 6; ++i)
		{
			const float Side = (i % 2 == 0) ? 1.f : -1.f;
			UStaticMeshComponent* Leg = AddPart(CylinderPath,
				FVector(-260.f + (i / 2) * 260.f, Side * 230.f, -170.f), FVector(0.8f, 0.8f, 1.4f), FRotator::ZeroRotator, Bison);
			ATLAWaterVisuals::ApplyTintMaterial(Leg, BisonWhite);
			UStaticMeshComponent* Foot = AddPart(CylinderPath,
				FVector(-260.f + (i / 2) * 260.f, Side * 230.f, -238.f), FVector(0.85f, 0.85f, 0.25f), FRotator::ZeroRotator, Bison);
			ATLAWaterVisuals::ApplyTintMaterial(Foot, BisonBrown);
		}

		// The flat beaver tail with its brown stripe
		UStaticMeshComponent* Tail = AddPart(CubePath, FVector(-500.f, 0.f, 30.f), FVector(3.4f, 4.2f, 0.28f), FRotator::ZeroRotator, Bison);
		ATLAWaterVisuals::ApplyTintMaterial(Tail, BisonWhite);
		UStaticMeshComponent* TailTip = AddPart(CubePath, FVector(-650.f, 0.f, 32.f), FVector(0.9f, 4.2f, 0.3f), FRotator::ZeroRotator, Bison);
		ATLAWaterVisuals::ApplyTintMaterial(TailTip, BisonBrown);
		break;
	}
	}
}

void AATLAHabitatCreature::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	switch (Style)
	{
	case EATLACreatureStyle::KoiPair:
	{
		// Opposite points of the same circle — the tide's push and pull
		for (int32 k = 0; k < Movers.Num(); ++k)
		{
			const float A = Age * 0.55f + k * PI;
			const FVector P(FMath::Cos(A) * 330.f, FMath::Sin(A) * 330.f, 10.f + 14.f * FMath::Sin(Age * 1.4f + k));
			Movers[k]->SetRelativeLocation(P);
			Movers[k]->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, 12.f * FMath::Sin(Age * 2.f + k)));
		}
		break;
	}
	case EATLACreatureStyle::Badgermole:
	{
		if (Movers.Num() > 0)
		{
			// Rooting and digging in place: heavy sway, head dipping to the ground
			Movers[0]->SetRelativeLocation(FVector(0.f, 0.f, 18.f * FMath::Abs(FMath::Sin(Age * 0.8f))));
			Movers[0]->SetRelativeRotation(FRotator(-6.f * FMath::Sin(Age * 0.8f), 10.f * FMath::Sin(Age * 0.23f), 0.f));
		}
		break;
	}
	case EATLACreatureStyle::Dragons:
	{
		// Each spine segment trails the flight path: an undulating banked
		// circle, the two dragons opposite each other like the reference
		auto Fly = [this](TArray<TObjectPtr<UStaticMeshComponent>>& Spine, float Phase)
		{
			for (int32 i = 0; i < Spine.Num(); ++i)
			{
				const float A = Age * 0.55f + Phase - i * 0.115f;
				const float Rise = 760.f + 240.f * FMath::Sin(A * 2.f) + 60.f * FMath::Sin(A * 5.f);
				const FVector P(FMath::Cos(A) * 950.f, FMath::Sin(A) * 950.f, Rise);
				Spine[i]->SetRelativeLocation(P);
				// face along the path: yaw follows the tangent, pitch follows the
				// climb, roll banks into the turn
				const float ClimbDeg = FMath::RadiansToDegrees(FMath::Atan2(
					480.f * FMath::Cos(A * 2.f) + 300.f * FMath::Cos(A * 5.f), 950.f)) * 0.55f;
				Spine[i]->SetRelativeRotation(FRotator(ClimbDeg, FMath::RadiansToDegrees(A) + 90.f, 22.f));
			}
		};
		Fly(DragonA, 0.f);
		Fly(DragonB, PI);
		break;
	}
	case EATLACreatureStyle::SkyBison:
	{
		if (Movers.Num() > 0)
		{
			const float A = Age * 0.12f;
			Movers[0]->SetRelativeLocation(FVector(FMath::Cos(A) * 1700.f, FMath::Sin(A) * 1700.f,
				980.f + 60.f * FMath::Sin(Age * 0.5f)));
			Movers[0]->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, 6.f * FMath::Sin(Age * 0.4f)));
		}
		break;
	}
	}
}
