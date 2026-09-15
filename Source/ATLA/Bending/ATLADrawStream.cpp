// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLADrawStream.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ATLAGameplayTags.h"
#include "ATLAWaterSource.h"
#include "ATLAWaterSplash.h"
#include "ATLAWaterVisuals.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 NumGlobules = 12;
	constexpr float TravelTime = 0.3f;    // seconds for one globule to reach the hand
	constexpr float Stagger = 0.055f;     // launch offset between globules

	FVector HandLocation(const ACharacter* Bender)
	{
		if (const USkeletalMeshComponent* Mesh = Bender ? Bender->GetMesh() : nullptr)
		{
			if (Mesh->DoesSocketExist(TEXT("hand_r")))
			{
				return Mesh->GetSocketLocation(TEXT("hand_r"));
			}
		}
		return Bender ? Bender->GetActorLocation() + FVector(0.f, 0.f, 40.f) : FVector::ZeroVector;
	}

	FVector Bezier(const FVector& A, const FVector& B, const FVector& C, float T)
	{
		const float U = 1.f - T;
		return A * (U * U) + B * (2.f * U * T) + C * (T * T);
	}
}

// ---- Draw stream ----

AATLADrawStream::AATLADrawStream()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	for (int32 i = 0; i < NumGlobules; ++i)
	{
		UStaticMeshComponent* Globule = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Globule%d"), i));
		Globule->SetupAttachment(RootComponent);
		Globule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Globule->SetUsingAbsoluteLocation(true);
		Globule->SetUsingAbsoluteRotation(true);
		Globule->SetUsingAbsoluteScale(true);
		Globule->SetVisibility(false);
		// Meshes are swapped per style in BeginPlay; default to spheres
		if (SphereMesh.Succeeded())
		{
			Globule->SetStaticMesh(SphereMesh.Object);
		}
		if (CubeMesh.Succeeded() && (i % 2 == 0))
		{
			// Half the pool alternates cube meshes so earth reads as broken chunks;
			// water and air swap back to spheres in BeginPlay
			Globule->SetStaticMesh(CubeMesh.Object);
		}
		Globules.Add(Globule);
	}
	SphereMeshAsset = SphereMesh.Succeeded() ? SphereMesh.Object : nullptr;
}

void AATLADrawStream::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(FMath::Max(Duration, 0.25f) + 0.1f);

	// (No ribbon on the draw arc: the example ribbon asset renders red and
	// read as fire inside water/air draws. The globule stream carries it.)

	for (int32 i = 0; i < Globules.Num(); ++i)
	{
		// Only earth keeps the broken-chunk cubes; a water stream is round
		if (Style != StyleEarth && SphereMeshAsset)
		{
			Globules[i]->SetStaticMesh(SphereMeshAsset);
		}
		switch (Style)
		{
		case StyleWater: ATLAWaterVisuals::ApplyWaterMaterial(Globules[i]); break;
		case StyleEarth: ATLAWaterVisuals::ApplyEarthMaterial(Globules[i]); break;
		default:         ATLAWaterVisuals::ApplyAirBandMaterial(Globules[i]); break;
		}
		TumbleRates.Add(FRotator(FMath::FRandRange(-320.f, 320.f), FMath::FRandRange(-320.f, 320.f), 0.f));

		// Scatter the origins so the stream has body
		if (Starts.IsValidIndex(i))
		{
			continue;  // air pre-filled a ring
		}
		Starts.Add(FVector(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-8.f, 8.f)));
	}
}

void AATLADrawStream::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;
	if (!Caster.IsValid())
	{
		return;
	}

	const FVector Hand = HandLocation(Caster.Get());
	const FVector Origin = GetActorLocation();

	// Water rides as one connected chain (droplets fill the whole arc at once);
	// earth and air keep the staggered convoy
	const float StyleStagger = (Style == StyleWater) ? (TravelTime / NumGlobules) : Stagger;

	for (int32 i = 0; i < Globules.Num(); ++i)
	{
		const float T = (Age - i * StyleStagger) / TravelTime;
		if (T < 0.f)
		{
			continue;
		}
		// Globules cycle source-to-hand for the stream's whole life
		const float F = FMath::Frac(T);
		const FVector Start = (Style == StyleAir) ? Starts[i] : Origin + Starts[i];

		// Arc up out of the source; earth heaves higher (it is being ripped out)
		const float ArcHeight = (Style == StyleEarth) ? 140.f : (Style == StyleWater ? 110.f : 50.f);
		const FVector Mid = (Start + Hand) * 0.5f + FVector(0.f, 0.f, ArcHeight);

		FVector Pos = Bezier(Start, Mid, Hand, F);
		const FVector Tangent = (2.f * (1.f - F) * (Mid - Start) + 2.f * F * (Hand - Mid)).GetSafeNormal();

		if (Style == StyleWater)
		{
			// A living stream, not beads: each droplet is stretched along the
			// flow and the whole chain undulates side to side as it travels
			const FVector Side = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
			Pos += Side * 10.f * FMath::Sin(F * 9.f - Age * 14.f)
			     + FVector(0.f, 0.f, 5.f * FMath::Sin(F * 13.f - Age * 11.f));
			Globules[i]->SetWorldRotation(FRotationMatrix::MakeFromX(Tangent).Rotator());
			// Long overlapping capsule-ish drops fuse into a ribbon of water;
			// thick mid-stream, tapering into the pool and the hand
			const float Body = 0.7f + 0.5f * FMath::Sin(F * PI);
			Globules[i]->SetWorldScale3D(FVector(0.55f, 0.13f, 0.13f) * Body);
		}
		else
		{
			// Shrink into the hand as the matter is absorbed into the technique
			const float BaseScale = (Style == StyleEarth) ? 0.16f : 0.18f;
			const FVector Shape = (Style == StyleAir) ? FVector(1.4f, 1.f, 0.5f) : FVector(1.f);
			Globules[i]->SetWorldScale3D(Shape * BaseScale * (1.f - F * 0.45f));
		}
		Globules[i]->SetWorldLocation(Pos);
		Globules[i]->SetVisibility(true);

		if (Style == StyleEarth)
		{
			Globules[i]->SetWorldRotation(FRotator(TumbleRates[i].Pitch * Age, TumbleRates[i].Yaw * Age, 0.f));
		}
	}

	// The pool churns where the stream tears out of it, and the hand catches
	// with a small splash — spaced out so it reads as agitation, not spam
	if (Style == StyleWater && Age - LastSplashTime > 0.5f && GetWorld())
	{
		LastSplashTime = Age;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AATLAWaterSplash>(AATLAWaterSplash::StaticClass(), Origin, FRotator::ZeroRotator, SpawnParams);
	}
}

void AATLADrawStream::SpawnForCast(ACharacter* Caster, const FGameplayTag& ElementTag, float Duration)
{
	if (!Caster || !Caster->HasAuthority() || !Caster->GetWorld())
	{
		return;
	}

	UWorld* World = Caster->GetWorld();
	const FVector Loc = Caster->GetActorLocation();
	const FVector Fwd = Caster->GetActorForwardVector();

	int32 Style;
	FVector Source;
	TArray<FVector> AirRing;

	if (ElementTag == ATLATags::Element_Water)
	{
		// Pull from the nearest pool — search generously; this is the read of
		// "the water came from somewhere", not the draw-inventory economy
		AATLAWaterSource* Nearest = nullptr;
		float BestDistSq = FMath::Square(2600.f);
		for (TActorIterator<AATLAWaterSource> It(World); It; ++It)
		{
			const float DistSq = FVector::DistSquared(It->GetActorLocation(), Loc);
			if (DistSq < BestDistSq)
			{
				Nearest = *It;
				BestDistSq = DistSq;
			}
		}
		if (!Nearest)
		{
			return;  // no pool anywhere near: nothing to visibly draw
		}
		// A point on the pool surface, on the bender's side
		const FVector ToBender = (Loc - Nearest->GetActorLocation()).GetSafeNormal2D();
		Source = Nearest->GetActorLocation() + ToBender * 240.f + FVector(0.f, 0.f, 14.f);
		Style = AATLADrawStream::StyleWater;
	}
	else if (ElementTag == ATLATags::Element_Earth)
	{
		// Rip chunks out of the ground just ahead of the bender
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Caster);
		const FVector Probe = Loc + Fwd * 170.f;
		if (World->LineTraceSingleByChannel(Hit, Probe + FVector(0.f, 0.f, 60.f), Probe - FVector(0.f, 0.f, 400.f), ECC_Visibility, Params))
		{
			Source = Hit.ImpactPoint;
		}
		else
		{
			Source = Probe - FVector(0.f, 0.f, 90.f);
		}
		Style = AATLADrawStream::StyleEarth;
	}
	else if (ElementTag == ATLATags::Element_Air)
	{
		// Air converges from a ring around the body
		Source = Loc;
		for (int32 i = 0; i < NumGlobules; ++i)
		{
			const float Angle = i * 2.f * PI / NumGlobules;
			AirRing.Add(Loc + FVector(FMath::Cos(Angle) * 230.f, FMath::Sin(Angle) * 230.f, FMath::FRandRange(-20.f, 110.f)));
		}
		Style = AATLADrawStream::StyleAir;
	}
	else
	{
		return;  // fire is self-generated — no draw
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Caster;
	SpawnParams.Instigator = Caster;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AATLADrawStream* Stream = World->SpawnActorDeferred<AATLADrawStream>(AATLADrawStream::StaticClass(), FTransform(Source), Caster, Caster, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Stream)
	{
		Stream->Caster = Caster;
		Stream->Style = Style;
		Stream->Duration = FMath::Max(Duration, 0.3f);
		Stream->Starts = MoveTemp(AirRing);  // empty for water/earth → scatter fills in
		Stream->FinishSpawning(FTransform(Source));
	}
}

// ---- Charge glow ----

AATLAChargeGlow::AATLAChargeGlow()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Orb = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Orb"));
	Orb->SetupAttachment(RootComponent);
	Orb->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ATLAWaterVisuals::SetupWaterMesh(Orb);
	Orb->SetRelativeScale3D(FVector(0.08f));

	Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));
	Core->SetupAttachment(RootComponent);
	Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ATLAWaterVisuals::SetupWaterMesh(Core);
	Core->SetRelativeScale3D(FVector(0.04f));
}

void AATLAChargeGlow::AttachToHand(ACharacter* Bender, const FGameplayTag& ElementTag)
{
	if (!Bender || !Bender->GetMesh())
	{
		return;
	}

	AttachToComponent(Bender->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("hand_r"));
	SetActorRelativeLocation(FVector::ZeroVector);

	if (ElementTag == ATLATags::Ability_Fire_Lightning)
	{
		// Crackling pre-bolt charge: blinding, jittery, with real sparks
		ATLAWaterVisuals::ApplyLightningMaterial(Orb);
		ATLAWaterVisuals::ApplyLightningMaterial(Core);
		bFlicker = true;
		if (UNiagaraSystem* Crackle = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/NiagaraExamples/FX_Sparks/NS_Spark_Continuous.NS_Spark_Continuous")))
		{
			if (UNiagaraComponent* Sparks = UNiagaraFunctionLibrary::SpawnSystemAttached(
				Crackle, Orb, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget, false))
			{
				Sparks->SetWorldScale3D(FVector(0.3f));
			}
		}
	}
	else if (ElementTag == ATLATags::Element_Fire)
	{
		ATLAWaterVisuals::ApplyFireMaterial(Orb);
		ATLAWaterVisuals::ApplyFireCoreMaterial(Core);
		bFlicker = true;
	}
	else if (ElementTag == ATLATags::Element_Earth)
	{
		ATLAWaterVisuals::ApplyEarthMaterial(Orb);
		ATLAWaterVisuals::ApplyEarthMaterial(Core);
	}
	else
	{
		ATLAWaterVisuals::ApplyWaterMaterial(Orb);
		ATLAWaterVisuals::ApplyIceMaterial(Core);
	}
}

void AATLAChargeGlow::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	// Grow toward full charge, breathing; fire also shivers
	float Scale = 0.08f + 0.34f * FMath::Min(Age / 1.1f, 1.f);
	Scale *= 1.f + 0.07f * FMath::Sin(Age * 16.f) + (bFlicker ? 0.08f * (FMath::FRand() - 0.5f) : 0.f);
	Orb->SetRelativeScale3D(FVector(Scale));
	Core->SetRelativeScale3D(FVector(Scale * 0.45f));
}
