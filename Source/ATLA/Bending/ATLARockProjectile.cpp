// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLARockProjectile.h"
#include "ATLACharacter.h"
#include "NiagaraComponent.h"
#include "ATLAEarthBurst.h"
#include "ATLAGameplayEffects.h"
#include "ATLAGameplayTags.h"
#include "ATLAWaterVisuals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AATLARockProjectile::AATLARockProjectile()
{
	// Straight and chunky — no serpentine weave, minimal arc
	Movement->InitialSpeed = 2600.f;
	Movement->MaxSpeed = 2600.f;
	Movement->ProjectileGravityScale = 0.1f;
	SerpentineAmplitude = 0.f;

	// A boulder, not a cube: a squashed sphere core with lumps fused on at
	// odd angles — the composite silhouette reads as torn rock
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
	Mesh->SetRelativeScale3D(FVector(0.44f, 0.37f, 0.32f));
	Mesh->SetRelativeRotation(FRotator(15.f, 20.f, 30.f));

	static const FVector LumpOffsets[] = {
		FVector(28.f, 18.f, 14.f), FVector(-24.f, 22.f, -12.f),
		FVector(12.f, -30.f, 18.f), FVector(-16.f, -14.f, -24.f) };
	static const FVector LumpScales[] = {
		FVector(0.55f, 0.45f, 0.4f), FVector(0.42f, 0.5f, 0.38f),
		FVector(0.48f, 0.38f, 0.45f), FVector(0.38f, 0.42f, 0.5f) };
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Lump = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Lump%d"), i));
		Lump->SetupAttachment(Mesh);
		Lump->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (SphereMesh.Succeeded())
		{
			Lump->SetStaticMesh(SphereMesh.Object);
		}
		Lump->SetRelativeLocation(LumpOffsets[i]);
		Lump->SetRelativeScale3D(LumpScales[i]);
		Lump->SetRelativeRotation(FRotator(i * 37.f, i * 61.f, i * 23.f));
		Lumps.Add(Lump);
	}

	LaunchGravityScale = 0.1f;   // a thrown rock arcs; matches its normal flight

	// Gravel crumbs, not a water ribbon
	TrailDropScale = 0.06f;
	bUseMeshTrail = true;
	RibbonTrail->SetAsset(nullptr);

	DamageEffect = UGE_EarthDamage_Jab::StaticClass();
	StructureDamage = 22.f;
	ImpactBurstClass = AATLAEarthBurst::StaticClass();
	InitialLifeSpan = 2.5f;
}

void AATLARockProjectile::ApplyMaterials()
{
	// Metal shards render steel-dark; rock renders as earth
	const FLinearColor Steel(0.30f, 0.33f, 0.40f);
	if (bMetal)
	{
		ATLAWaterVisuals::ApplyTintMaterial(Mesh, Steel);
	}
	else
	{
		ATLAWaterVisuals::ApplyEarthMaterial(Mesh);
	}
	for (UStaticMeshComponent* Lump : Lumps)
	{
		bMetal ? ATLAWaterVisuals::ApplyTintMaterial(Lump, Steel) : ATLAWaterVisuals::ApplyEarthMaterial(Lump);
	}
	for (UStaticMeshComponent* Drop : TrailDrops)
	{
		bMetal ? ATLAWaterVisuals::ApplyTintMaterial(Drop, Steel) : ATLAWaterVisuals::ApplyEarthMaterial(Drop);
	}
}

void AATLARockProjectile::MakeMetal()
{
	bMetal = true;

	// A shard, not a lump: stretched core, tighter lumps, no gravity sag
	Mesh->SetRelativeScale3D(FVector(0.62f, 0.26f, 0.22f));
	for (UStaticMeshComponent* Lump : Lumps)
	{
		Lump->SetRelativeScale3D(Lump->GetRelativeScale3D() * 0.55f);
	}
	Movement->InitialSpeed = 3400.f;
	Movement->MaxSpeed = 3400.f;
	Movement->ProjectileGravityScale = 0.f;
	LaunchGravityScale = 0.f;

	DamageEffect = UGE_EarthDamage_Boulder::StaticClass();
	StructureDamage = 60.f;

	ApplyMaterials();
}

void AATLARockProjectile::OnImpactVictim(AActor* Victim, const FHitResult& Hit)
{
	if (AATLACharacter* Thrower = Cast<AATLACharacter>(GetInstigator()))
	{
		Thrower->StokeEarthMastery(0.12f);
	}
}

AATLABoulderProjectile::AATLABoulderProjectile()
{
	Movement->InitialSpeed = 2000.f;
	Movement->MaxSpeed = 2000.f;
	Movement->ProjectileGravityScale = 0.25f;

	Mesh->SetRelativeScale3D(FVector(1.2f, 1.05f, 1.1f));
	Collision->SetSphereRadius(50.f);

	DamageEffect = UGE_EarthDamage_Boulder::StaticClass();
	StructureDamage = 150.f;
	KnockbackSpeed = 600.f;
	InitialLifeSpan = 3.f;
}

void AATLABoulderProjectile::OnImpactVictim(AActor* Victim, const FHitResult& Hit)
{
	Super::OnImpactVictim(Victim, Hit);   // heavy hits still deepen mastery

	// Rooted and armored earthbenders don't get moved
	if (UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
	{
		if (VictimASC->HasMatchingGameplayTag(ATLATags::State_Earth_Rooted) ||
			VictimASC->HasMatchingGameplayTag(ATLATags::State_Earth_Armored))
		{
			return;
		}
	}

	if (ACharacter* Character = Cast<ACharacter>(Victim))
	{
		const FVector Away = GetVelocity().GetSafeNormal2D();
		Character->LaunchCharacter(Away * KnockbackSpeed + FVector(0.f, 0.f, 260.f), true, true);
	}
}

// ---- Hoisted boulder ----

AATLAHoistedBoulder::AATLAHoistedBoulder()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root"))); // sits at the ground rip point

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// Boulder silhouette: squashed sphere core plus fused lumps, not a cube
	Rock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rock"));
	Rock->SetupAttachment(RootComponent);
	Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (SphereMesh.Succeeded())
	{
		Rock->SetStaticMesh(SphereMesh.Object);
	}
	Rock->SetRelativeScale3D(FVector(2.5f, 2.1f, 1.8f));
	Rock->SetRelativeRotation(FRotator(14.f, 25.f, 20.f));
	Rock->SetRelativeLocation(FVector(0.f, 0.f, -160.f)); // buried; the rise animates it out

	static const FVector LumpOffsets[] = {
		FVector(30.f, 20.f, 16.f), FVector(-26.f, 24.f, -14.f),
		FVector(14.f, -32.f, 20.f), FVector(-18.f, -16.f, -26.f) };
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Lump = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("RockLump%d"), i));
		Lump->SetupAttachment(Rock);
		Lump->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (SphereMesh.Succeeded())
		{
			Lump->SetStaticMesh(SphereMesh.Object);
		}
		Lump->SetRelativeLocation(LumpOffsets[i]);
		Lump->SetRelativeScale3D(FVector(0.5f - i * 0.04f, 0.42f + i * 0.03f, 0.45f));
		Lump->SetRelativeRotation(FRotator(i * 37.f, i * 61.f, i * 23.f));
		RockLumps.Add(Lump);
	}

	for (int32 i = 0; i < 6; ++i)
	{
		UStaticMeshComponent* Chip = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Chip%d"), i));
		Chip->SetupAttachment(RootComponent);
		Chip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (SphereMesh.Succeeded())
		{
			Chip->SetStaticMesh(SphereMesh.Object);
		}
		Chip->SetRelativeScale3D(FVector(0.16f, 0.13f, 0.11f));
		Debris.Add(Chip);
	}
}

void AATLAHoistedBoulder::BeginPlay()
{
	Super::BeginPlay();

	// Safety: if the channel somehow dies without cleanup, don't hover forever
	SetLifeSpan(8.f);

	ATLAWaterVisuals::ApplyEarthMaterial(Rock);
	for (UStaticMeshComponent* Lump : RockLumps)
	{
		ATLAWaterVisuals::ApplyEarthMaterial(Lump);
	}
	for (UStaticMeshComponent* Chip : Debris)
	{
		ATLAWaterVisuals::ApplyEarthMaterial(Chip);
	}

	// Debris kick as the ground breaks open
	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AATLAEarthBurst>(AATLAEarthBurst::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
}

FVector AATLAHoistedBoulder::GetRockLocation() const
{
	return Rock->GetComponentLocation();
}

void AATLAHoistedBoulder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	// Tear free of the ground, then hover with a heavy bob and a slow turn
	const float T = FMath::Min(Age / RiseTime, 1.f);
	const float Eased = 1.f - FMath::Square(1.f - T); // fast burst, slowing to the hover
	const float Bob = (T >= 1.f) ? 9.f * FMath::Sin((Age - RiseTime) * 3.2f) : 0.f;
	Rock->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(-160.f, HoverHeight, Eased) + Bob));
	Rock->SetRelativeRotation(FRotator(14.f, 25.f + Age * 24.f, 20.f));

	// Loosened chips orbit up along with it and rain back down
	for (int32 i = 0; i < Debris.Num(); ++i)
	{
		const float A = Age * 2.4f + i * 1.05f;
		const float R = 95.f + 18.f * FMath::Sin(A * 0.7f);
		Debris[i]->SetRelativeLocation(FVector(
			FMath::Cos(A) * R,
			FMath::Sin(A) * R,
			FMath::Lerp(-30.f, HoverHeight * 0.55f, Eased) + 26.f * FMath::Sin(A * 1.6f)));
	}
}


// ---- Earth Armor cocoon ----

AATLAEarthCocoon::AATLAEarthCocoon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// Two rings of slabs (low and high) plus a cap: enough coverage to read
	// as "the earth swallowed them" while the bender stays glimpsable
	int32 Index = 0;
	for (int32 Ring = 0; Ring < 2; ++Ring)
	{
		for (int32 i = 0; i < 6; ++i, ++Index)
		{
			UStaticMeshComponent* Slab = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Slab%d"), Index));
			Slab->SetupAttachment(RootComponent);
			Slab->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (SphereMesh.Succeeded())
			{
				Slab->SetStaticMesh(SphereMesh.Object);
			}
			const float A = i * (2.f * PI / 6.f) + Ring * (PI / 6.f);
			const float R = 62.f - Ring * 10.f;
			ClosedOffsets.Add(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 35.f + Ring * 70.f));
			Slab->SetRelativeScale3D(FVector(0.55f, 0.42f, 0.75f));
			Slab->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(A), 12.f - Ring * 24.f));
			Slabs.Add(Slab);
		}
	}
	UStaticMeshComponent* Cap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cap"));
	Cap->SetupAttachment(RootComponent);
	Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (SphereMesh.Succeeded())
	{
		Cap->SetStaticMesh(SphereMesh.Object);
	}
	ClosedOffsets.Add(FVector(0.f, 0.f, 150.f));
	Cap->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.35f));
	Slabs.Add(Cap);

	InitialLifeSpan = 3.f;
}

void AATLAEarthCocoon::BeginPlay()
{
	Super::BeginPlay();

	for (int32 i = 0; i < Slabs.Num(); ++i)
	{
		ATLAWaterVisuals::ApplyEarthMaterial(Slabs[i]);
		// Buried: each slab starts under the ground below its closed spot
		Slabs[i]->SetRelativeLocation(ClosedOffsets[i] - FVector(0.f, 0.f, 190.f));
		BurstVel.Add((ClosedOffsets[i].GetSafeNormal2D() * FMath::FRandRange(420.f, 640.f))
			+ FVector(0.f, 0.f, FMath::FRandRange(180.f, 320.f)));
	}

	// The ground breaks as the shell tears out of it
	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AATLAEarthBurst>(AATLAEarthBurst::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
}

void AATLAEarthCocoon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	if (!bBurst)
	{
		const float T = FMath::Min(Age / CloseTime, 1.f);
		const float Eased = 1.f - FMath::Square(1.f - T);   // surge, then settle
		for (int32 i = 0; i < Slabs.Num(); ++i)
		{
			Slabs[i]->SetRelativeLocation(FMath::Lerp(ClosedOffsets[i] - FVector(0.f, 0.f, 190.f), ClosedOffsets[i], Eased));
		}
		if (Age >= CloseTime + HoldTime)
		{
			bBurst = true;
		}
	}
	else
	{
		// Shattering away as the armored bender rises
		for (int32 i = 0; i < Slabs.Num(); ++i)
		{
			BurstVel[i].Z -= 980.f * DeltaTime;
			Slabs[i]->AddRelativeLocation(BurstVel[i] * DeltaTime);
			Slabs[i]->AddRelativeRotation(FRotator(240.f * DeltaTime, 180.f * DeltaTime, 0.f));
			Slabs[i]->SetRelativeScale3D(Slabs[i]->GetRelativeScale3D() * FMath::Max(1.f - 1.4f * DeltaTime, 0.f));
		}
		if (Age >= CloseTime + HoldTime + 0.8f)
		{
			Destroy();
		}
	}
}
