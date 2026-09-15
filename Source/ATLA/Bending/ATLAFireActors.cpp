// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAFireActors.h"
#include "ATLACharacter.h"
#include "ATLAGameplayEffects.h"
#include "ATLAGameplayTags.h"
#include "ATLAWaterVisuals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void ApplyDamageTo(AActor* Instigator, AActor* Target, TSubclassOf<UGameplayEffect> Effect)
	{
		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (SourceASC && TargetASC && Effect)
		{
			const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Effect, 1.f, SourceASC->MakeEffectContext());
			if (Spec.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}
	}

	constexpr float WallBottom = -130.f;  // wall spawns with its center 130 above the ground
}

// ---- Fire burst ----

AATLAFireBurst::AATLAFireBurst()
{
	Shell->SetRelativeScale3D(FVector(0.01f));
	Shell->SetVisibility(false);

	// Real detonation from the Fire & Explosion pack (auto-activates on spawn);
	// the rising ember meshes stay as guaranteed backup if the pack is absent
	Detonation = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Detonation"));
	Detonation->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DetonationFX(TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/EXP/NS_Sub_EXP_Small_002.NS_Sub_EXP_Small_002"));
	if (DetonationFX.Succeeded())
	{
		Detonation->SetAsset(DetonationFX.Object);
	}

	// Embers accelerate UP and out — fire climbs, it never splashes
	DropletGravityZ = 540.f;
	DropletSpeedMin = 130.f;
	DropletSpeedMax = 330.f;
	DropletUpBias = 0.85f;
	DropletShrinkRate = 2.7f;
	DropletShape = FVector(0.09f, 0.09f, 0.2f);  // stretched vertical licks
}

void AATLAFireBurst::ApplyMaterials()
{
	for (UStaticMeshComponent* Drop : Droplets)
	{
		ATLAWaterVisuals::ApplyEmberMaterial(Drop);
	}
}

// ---- Fire bolt ----

AATLAFireBolt::AATLAFireBolt()
{
	// Hot and fast; tight jitter rather than a watery weave
	Movement->InitialSpeed = 3000.f;
	Movement->MaxSpeed = 3000.f;
	Movement->ProjectileGravityScale = 0.f;
	SerpentineAmplitude = 5.f;
	SerpentineFrequency = 30.f;

	Mesh->SetRelativeScale3D(FVector(0.42f, 0.26f, 0.26f));
	TrailDropScale = 0.13f;
	TrailRiseSpeed = 120.f;   // the trail licks upward as it fades
	MeshFlickerAmp = 0.45f;   // the head flickers like a real flame

	DamageEffect = UGE_FireDamage_Jab::StaticClass();
	StructureDamage = 20.f;
	ImpactBurstClass = AATLAFireBurst::StaticClass();
	InitialLifeSpan = 2.f;

	// The bolt IS fire now: a real flame rides the collision sphere.
	// The mesh-drop trail retires; the white-hot core mesh stays underneath.
	bUseMeshTrail = false;
	RibbonTrail->SetAsset(nullptr);
	FlameTrail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlameTrail"));
	FlameTrail->SetupAttachment(Collision);
	FlameTrail->SetRelativeScale3D(FVector(0.55f));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FlameFX(TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Small.NS_Fire_Small"));
	if (FlameFX.Succeeded())
	{
		FlameTrail->SetAsset(FlameFX.Object);
	}
	else
	{
		bUseMeshTrail = true;  // pack missing: fall back to the mesh trail
	}
}

void AATLAFireBolt::ApplyMaterials()
{
	ATLAWaterVisuals::ApplyFireCoreMaterial(Mesh);
	for (UStaticMeshComponent* Drop : TrailDrops)
	{
		ATLAWaterVisuals::ApplyEmberMaterial(Drop);
	}
}

// ---- Fire blast ----

AATLAFireBlastBolt::AATLAFireBlastBolt()
{
	Movement->InitialSpeed = 2300.f;
	Movement->MaxSpeed = 2300.f;
	Mesh->SetRelativeScale3D(FVector(0.7f, 0.6f, 0.6f));
	Collision->SetSphereRadius(34.f);

	// White-hot heart inside the orange shell
	Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));
	Core->SetupAttachment(Collision);
	Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Core->SetRelativeScale3D(FVector(0.36f, 0.3f, 0.3f));
	ATLAWaterVisuals::SetupWaterMesh(Core);

	DamageEffect = UGE_FireDamage_Blast::StaticClass();
	StructureDamage = 120.f;
	TrailDropScale = 0.18f;
	MeshFlickerAmp = 0.3f;
}

void AATLAFireBlastBolt::ApplyMaterials()
{
	ATLAWaterVisuals::ApplyFireMaterial(Mesh);
	ATLAWaterVisuals::ApplyFireCoreMaterial(Core);
	for (UStaticMeshComponent* Drop : TrailDrops)
	{
		ATLAWaterVisuals::ApplyEmberMaterial(Drop);
	}
}

void AATLAFireBolt::OnImpactVictim(AActor* Victim, const FHitResult& Hit)
{
	// Fire feeds on victory: every landed flame stokes the inner drive
	if (AATLACharacter* Thrower = Cast<AATLACharacter>(GetInstigator()))
	{
		Thrower->StokeInnerDrive(0.22f);
	}
}

void AATLAFireBlastBolt::OnImpactVictim(AActor* Victim, const FHitResult& Hit)
{
	Super::OnImpactVictim(Victim, Hit);

	// The direct hit already took full damage via DamageEffect; splash the area
	TArray<AActor*> Characters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Characters);
	for (AActor* Target : Characters)
	{
		if (Target == GetInstigator() || Target == Victim)
		{
			continue;
		}
		// Empowered blasts also detonate wider
		const float Radius = ExplosionRadius * (bEmpowered ? 1.45f : 1.f);
		if (FVector::DistSquared(Target->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius))
		{
			ApplyDamageTo(GetInstigator(), Target, UGE_FireDamage_Arc::StaticClass());
		}
	}
}

// ---- Fire stream tongue ----

AATLAFireStreamBolt::AATLAFireStreamBolt()
{
	// Short reach, lively weave: many of these in flight read as one stream
	Movement->InitialSpeed = 2000.f;
	Movement->MaxSpeed = 2000.f;
	Mesh->SetRelativeScale3D(FVector(0.3f, 0.18f, 0.18f));
	Collision->SetSphereRadius(22.f);  // a flame lick is forgiving, not a needle
	SerpentineAmplitude = 12.f;
	SerpentineFrequency = 24.f;
	TrailDropScale = 0.09f;
	FlameTrail->SetRelativeScale3D(FVector(0.4f));

	DamageEffect = UGE_FireDamage_Stream::StaticClass();
	StructureDamage = 5.f;
	InitialLifeSpan = 0.6f;
}

// ---- Fire lash ----

AATLAFireLashBolt::AATLAFireLashBolt()
{
	Movement->InitialSpeed = 2500.f;
	Movement->MaxSpeed = 2500.f;

	// A wide flat burning crescent that sweeps through a line of enemies
	Mesh->SetRelativeScale3D(FVector(0.5f, 2.2f, 0.24f));
	Collision->SetSphereRadius(72.f);
	SerpentineAmplitude = 0.f;
	TrailDropScale = 0.2f;
	MeshFlickerAmp = 0.3f;

	DamageEffect = UGE_FireDamage_Lash::StaticClass();
	StructureDamage = 45.f;
	InitialLifeSpan = 1.2f;
}

// ---- Lightning bolt visual ----

AATLALightningBolt::AATLALightningBolt()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 i = 0; i < 14; ++i)
	{
		UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Seg%d"), i));
		Segment->SetupAttachment(RootComponent);
		Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Segment->SetUsingAbsoluteLocation(true);
		Segment->SetUsingAbsoluteRotation(true);
		Segment->SetUsingAbsoluteScale(true);
		if (CubeMesh.Succeeded())
		{
			Segment->SetStaticMesh(CubeMesh.Object);
		}
		Segments.Add(Segment);
	}

	SetLifeSpan(0.35f);
}

void AATLALightningBolt::SetEndpoints(const FVector& Start, const FVector& End)
{
	// Jittered polyline: each joint strays off the straight path, the strays
	// shrinking to zero at both ends so the bolt stays anchored
	const int32 N = Segments.Num();
	TArray<FVector> Joints;
	Joints.Add(Start);
	for (int32 i = 1; i < N; ++i)
	{
		const float T = static_cast<float>(i) / N;
		const float Stray = 60.f * FMath::Sin(T * PI);
		Joints.Add(FMath::Lerp(Start, End, T) + FMath::VRand() * FMath::FRandRange(0.f, Stray));
	}
	Joints.Add(End);

	for (int32 i = 0; i < N; ++i)
	{
		ATLAWaterVisuals::ApplyLightningMaterial(Segments[i]);
		const FVector A = Joints[i];
		const FVector B = Joints[i + 1];
		Segments[i]->SetWorldLocation((A + B) * 0.5f);
		Segments[i]->SetWorldRotation((B - A).Rotation());
		Segments[i]->SetWorldScale3D(FVector((B - A).Size() / 100.f, 0.045f, 0.045f));
	}
}

void AATLALightningBolt::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	// Strobe as it dies
	const bool bVisible = FMath::Fmod(Age, 0.08f) < 0.055f;
	for (UStaticMeshComponent* Segment : Segments)
	{
		Segment->SetVisibility(bVisible);
	}
}

// ---- Fire wall ----

AATLAFireWall::AATLAFireWall()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	SetRootComponent(Zone);
	Zone->SetBoxExtent(FVector(40.f, 260.f, 140.f));
	Zone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// A line of licking cone tongues, each with a white-hot core nested inside
	for (int32 i = 0; i < 7; ++i)
	{
		UStaticMeshComponent* Flame = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Flame%d"), i));
		Flame->SetupAttachment(Zone);
		Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ATLAWaterVisuals::SetupConeMesh(Flame);
		Flame->SetRelativeLocation(FVector(0.f, -240.f + i * 80.f, WallBottom + 100.f));
		Flames.Add(Flame);

		UStaticMeshComponent* FlameCore = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Core%d"), i));
		FlameCore->SetupAttachment(Zone);
		FlameCore->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ATLAWaterVisuals::SetupConeMesh(FlameCore);
		FlameCore->SetRelativeLocation(FVector(0.f, -240.f + i * 80.f, WallBottom + 65.f));
		FlameCores.Add(FlameCore);
	}

	// Sparks that cycle up out of the fire
	for (int32 i = 0; i < 8; ++i)
	{
		UStaticMeshComponent* Ember = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Ember%d"), i));
		Ember->SetupAttachment(Zone);
		Ember->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ATLAWaterVisuals::SetupWaterMesh(Ember);
		Embers.Add(Ember);
	}

	// Real fire along the wall: three medium Niagara flames; the cone
	// tongues become the fallback when the pack is absent
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> WallFlameFX(TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Medium.NS_Fire_Medium"));
	if (WallFlameFX.Succeeded())
	{
		bNiagaraWall = true;
		for (int32 i = 0; i < 3; ++i)
		{
			UNiagaraComponent* Flame = CreateDefaultSubobject<UNiagaraComponent>(*FString::Printf(TEXT("FlameFX%d"), i));
			Flame->SetupAttachment(Zone);
			Flame->SetAsset(WallFlameFX.Object);
			Flame->SetRelativeLocation(FVector(0.f, -170.f + i * 170.f, WallBottom));
			Flame->SetRelativeScale3D(FVector(1.6f));
			FlameFX.Add(Flame);
		}
	}
}

void AATLAFireWall::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(Lifetime);
	for (UStaticMeshComponent* Flame : Flames)
	{
		ATLAWaterVisuals::ApplyFireMaterial(Flame);
		Flame->SetVisibility(!bNiagaraWall);
	}
	for (UStaticMeshComponent* FlameCore : FlameCores)
	{
		ATLAWaterVisuals::ApplyFireCoreMaterial(FlameCore);
		FlameCore->SetVisibility(!bNiagaraWall);
	}
	for (UStaticMeshComponent* Ember : Embers)
	{
		ATLAWaterVisuals::ApplyEmberMaterial(Ember);
	}
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(BurnTimer, this, &AATLAFireWall::BurnTick, 0.5f, true);
	}
}

void AATLAFireWall::BurnTick()
{
	// Burn characters standing in the flames
	TArray<AActor*> Overlapping;
	Zone->GetOverlappingActors(Overlapping, ACharacter::StaticClass());
	for (AActor* Target : Overlapping)
	{
		if (Target != GetInstigator())
		{
			ApplyDamageTo(GetInstigator(), Target, UGE_FireDamage_Burn::StaticClass());
		}
	}

	// Incinerate enemy projectiles crossing the wall
	TArray<AActor*> Projectiles;
	Zone->GetOverlappingActors(Projectiles, AATLAWaterProjectile::StaticClass());
	for (AActor* Projectile : Projectiles)
	{
		if (Projectile->GetInstigator() != GetInstigator())
		{
			Projectile->Destroy();
		}
	}
}

void AATLAFireWall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Fire is not free-standing: it lives on its maker's chi. Maker gone
	// (or downed) -> the wall gutters out.
	if (const AATLACharacter* Maker = Cast<AATLACharacter>(GetInstigator());
		!GetInstigator() || (Maker && Maker->GetHealth() <= 0.f))
	{
		Destroy();
		return;
	}

	Age += DeltaTime;

	// Each tongue licks on two beat frequencies plus per-frame shiver;
	// the cone stretches from a rooted base, never floating off the ground
	for (int32 i = 0; i < Flames.Num(); ++i)
	{
		const float Pulse = 0.9f + 0.28f * FMath::Sin(Age * 8.f + i * 1.7f) + 0.16f * FMath::Sin(Age * 21.f + i * 3.1f) + 0.06f * FMath::FRand();
		const float H = 2.3f * Pulse;                 // cone height in engine units of 100
		Flames[i]->SetRelativeScale3D(FVector(0.75f, 0.95f, H));
		Flames[i]->SetRelativeLocation(FVector(0.f, -240.f + i * 80.f, WallBottom + H * 50.f));
		Flames[i]->SetRelativeRotation(FRotator(FMath::Sin(Age * 5.f + i) * 6.f, 0.f, FMath::Sin(Age * 4.f + i * 2.2f) * 5.f));

		const float CoreH = H * 0.62f;
		FlameCores[i]->SetRelativeScale3D(FVector(0.38f, 0.5f, CoreH));
		FlameCores[i]->SetRelativeLocation(FVector(0.f, -240.f + i * 80.f, WallBottom + CoreH * 50.f));
	}

	// Sparks rise out of the tongues, shrinking as they climb
	for (int32 i = 0; i < Embers.Num(); ++i)
	{
		const float Speed = 170.f + i * 23.f;
		const float Cycle = FMath::Fmod(Age * Speed + i * 47.f, 320.f);
		const float Fade = 1.f - Cycle / 320.f;
		Embers[i]->SetRelativeLocation(FVector(
			FMath::Sin(Age * 3.f + i * 1.3f) * 18.f,
			-240.f + i * 68.f + FMath::Sin(Age * 2.f + i) * 22.f,
			WallBottom + 60.f + Cycle));
		Embers[i]->SetRelativeScale3D(FVector(0.03f + 0.06f * Fade));
	}
}

// ---- Fire nova ----

AATLAFireNova::AATLAFireNova()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	SetRootComponent(Ring);
	Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ATLAWaterVisuals::SetupWaterMesh(Ring);
	Ring->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.25f));

	// Flash and gouts must not inherit the ring's growing scale
	Flash = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flash"));
	Flash->SetupAttachment(Ring);
	Flash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flash->SetUsingAbsoluteScale(true);
	Flash->SetUsingAbsoluteLocation(true);
	Flash->SetRelativeScale3D(FVector(1.2f, 1.2f, 1.8f));
	ATLAWaterVisuals::SetupWaterMesh(Flash);

	for (int32 i = 0; i < 12; ++i)
	{
		UStaticMeshComponent* Wisp = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Wisp%d"), i));
		Wisp->SetupAttachment(Ring);
		Wisp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wisp->SetUsingAbsoluteScale(true);
		Wisp->SetUsingAbsoluteLocation(true);
		ATLAWaterVisuals::SetupWaterMesh(Wisp);
		Wisps.Add(Wisp);
	}

	// The real detonation: a large one-shot explosion; flash column and
	// gout meshes become the fallback
	Explosion = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Explosion"));
	Explosion->SetupAttachment(Ring);
	Explosion->SetUsingAbsoluteScale(true);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NovaFX(TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/EXP/NS_Sub_EXP_Large_001_01.NS_Sub_EXP_Large_001_01"));
	if (NovaFX.Succeeded())
	{
		bNiagaraNova = true;
		Explosion->SetAsset(NovaFX.Object);
	}

	SetLifeSpan(1.6f);
}

void AATLAFireNova::BeginPlay()
{
	Super::BeginPlay();

	ATLAWaterVisuals::ApplyFireMaterial(Ring);
	ATLAWaterVisuals::ApplyFireCoreMaterial(Flash);
	for (UStaticMeshComponent* Wisp : Wisps)
	{
		ATLAWaterVisuals::ApplyEmberMaterial(Wisp);
	}

	// With the real Niagara detonation the giant expanding disc reads as a
	// yellow pancake flooding the arena — hide it; the radial gouts stay to
	// carry the expanding-wave read.
	if (bNiagaraNova)
	{
		Ring->SetVisibility(false);
	}

	if (!HasAuthority())
	{
		return;
	}

	// One-shot radial damage + knockback
	TArray<AActor*> Characters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Characters);
	for (AActor* Target : Characters)
	{
		if (Target == GetInstigator())
		{
			continue;
		}
		if (FVector::DistSquared(Target->GetActorLocation(), GetActorLocation()) > FMath::Square(Radius))
		{
			continue;
		}
		ApplyDamageTo(GetInstigator(), Target, UGE_FireDamage_Nova::StaticClass());

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		const bool bImmune = TargetASC && (TargetASC->HasMatchingGameplayTag(ATLATags::State_Earth_Rooted) ||
			TargetASC->HasMatchingGameplayTag(ATLATags::State_Earth_Armored));
		if (!bImmune)
		{
			if (ACharacter* Victim = Cast<ACharacter>(Target))
			{
				const FVector Away = (Victim->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
				Victim->LaunchCharacter(Away * 850.f + FVector(0.f, 0.f, 320.f), true, true);
			}
		}
	}
}

void AATLAFireNova::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (const AATLACharacter* Maker = Cast<AATLACharacter>(GetInstigator());
		!GetInstigator() || (Maker && Maker->GetHealth() <= 0.f))
	{
		Destroy();
		return;
	}

	Age += DeltaTime;

	// Expanding, thinning flame ring
	const float R = FMath::Lerp(0.5f, Radius / 50.f, FMath::Min(Age / 0.45f, 1.f));
	Ring->SetRelativeScale3D(FVector(R, R, FMath::Max(0.3f * (1.f - Age), 0.02f)));

	// The white-hot column flashes up and burns out fast (fallback only)
	if (bNiagaraNova)
	{
		Flash->SetVisibility(false);
	}
	else if (Age < 0.28f)
	{
		const float F = Age / 0.28f;
		Flash->SetWorldLocation(GetActorLocation());
		Flash->SetWorldScale3D(FVector(1.2f + F * 1.6f, 1.2f + F * 1.6f, 1.8f + F * 3.2f));
	}
	else
	{
		Flash->SetVisibility(false);
	}

	// Flame gouts thrown out of the blast, rising as they fly — they carry
	// the expanding damage-radius read even over the Niagara detonation
	const float Reach = FMath::Min(Age / 0.45f, 1.f) * Radius;
	for (int32 i = 0; i < Wisps.Num(); ++i)
	{
		const float Angle = i * 2.f * PI / Wisps.Num();
		const FVector Dir(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Wisps[i]->SetWorldLocation(GetActorLocation() + Dir * Reach + FVector(0.f, 0.f, 30.f + Age * 300.f));
		// Stretched vertical licks, not floating orbs
		const float WispScale = FMath::Max(0.3f * (1.f - Age), 0.02f);
		Wisps[i]->SetWorldScale3D(FVector(WispScale * 0.55f, WispScale * 0.55f, WispScale * 2.2f));
	}
}
