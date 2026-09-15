// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAirActors.h"
#include "NiagaraComponent.h"
#include "ATLAGameplayEffects.h"
#include "ATLAGameplayTags.h"
#include "ATLAWaterVisuals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	void AirApplyDamageTo(AActor* Instigator, AActor* Target, TSubclassOf<UGameplayEffect> Effect)
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

	bool IsDisplacementImmune(AActor* Target)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		return ASC && (ASC->HasMatchingGameplayTag(ATLATags::State_Earth_Rooted) ||
			ASC->HasMatchingGameplayTag(ATLATags::State_Earth_Armored));
	}
}

// ---- Air burst ----

AATLAAirBurst::AATLAAirBurst()
{
	Shell->SetRelativeScale3D(FVector(0.4f));

	// A flat, fast gust — air disperses outward, it doesn't rain down
	DropletGravityZ = 0.f;
	DropletSpeedMin = 460.f;
	DropletSpeedMax = 680.f;
	DropletUpBias = 0.05f;
	DropletZMul = 0.25f;
	DropletShrinkRate = 3.4f;
	DropletShape = FVector(0.18f, 0.18f, 0.07f);  // flattened puffs
}

void AATLAAirBurst::ApplyMaterials()
{
	ATLAWaterVisuals::ApplyAirBandMaterial(Shell);
	for (UStaticMeshComponent* Drop : Droplets)
	{
		ATLAWaterVisuals::ApplyAirBandMaterial(Drop);
	}
}

// ---- Air blast ----

AATLAAirBlastBolt::AATLAAirBlastBolt()
{
	Movement->InitialSpeed = 2800.f;
	Movement->MaxSpeed = 2800.f;
	Movement->ProjectileGravityScale = 0.f;
	SerpentineAmplitude = 10.f;
	SerpentineFrequency = 18.f;

	Mesh->SetRelativeScale3D(FVector(0.5f, 0.42f, 0.42f));
	TrailDropScale = 0.16f;

	// Wind streaks corkscrewing around the flight line
	for (int32 i = 0; i < 3; ++i)
	{
		UStaticMeshComponent* Spiral = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Spiral%d"), i));
		Spiral->SetupAttachment(Collision);
		Spiral->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Spiral->SetRelativeScale3D(FVector(0.42f, 0.1f, 0.1f));
		ATLAWaterVisuals::SetupWaterMesh(Spiral);
		Spirals.Add(Spiral);
	}

	DamageEffect = UGE_AirDamage_Blast::StaticClass();
	StructureDamage = 5.f;
	ImpactBurstClass = AATLAAirBurst::StaticClass();
	InitialLifeSpan = 2.f;

	// Wind streams: the mesh trail (the shared ribbon asset rendered red)
	bUseMeshTrail = true;
}

void AATLAAirBlastBolt::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Corkscrew: the streaks orbit the flight axis
	for (int32 i = 0; i < Spirals.Num(); ++i)
	{
		const float A = Age * 17.f + i * 2.f * PI / FMath::Max(Spirals.Num(), 1);
		Spirals[i]->SetRelativeLocation(FVector(-14.f, FMath::Cos(A) * 30.f, FMath::Sin(A) * 30.f));
		Spirals[i]->SetRelativeRotation(FRotator(0.f, 0.f, FMath::RadiansToDegrees(A)));
	}
}

void AATLAAirBlastBolt::ApplyMaterials()
{
	ATLAWaterVisuals::ApplyAirBandMaterial(Mesh);
	for (UStaticMeshComponent* Drop : TrailDrops)
	{
		ATLAWaterVisuals::ApplyAirBandMaterial(Drop);
	}
	for (UStaticMeshComponent* Spiral : Spirals)
	{
		ATLAWaterVisuals::ApplyAirBandMaterial(Spiral);
	}
}

void AATLAAirBlastBolt::OnImpactVictim(AActor* Victim, const FHitResult& Hit)
{
	if (IsDisplacementImmune(Victim))
	{
		return;
	}
	if (ACharacter* Character = Cast<ACharacter>(Victim))
	{
		const FVector Away = GetVelocity().GetSafeNormal();
		Character->LaunchCharacter(Away * PushSpeed + FVector(0.f, 0.f, 240.f), true, true);
	}
}

// ---- Air swipe ----

AATLAAirSwipeBolt::AATLAAirSwipeBolt()
{
	Movement->InitialSpeed = 2400.f;
	Movement->MaxSpeed = 2400.f;

	// Wide flat crescent
	Mesh->SetRelativeScale3D(FVector(0.42f, 2.1f, 0.24f));
	Collision->SetSphereRadius(70.f);

	DamageEffect = UGE_AirDamage_Swipe::StaticClass();
	PushSpeed = 950.f;
	InitialLifeSpan = 1.2f;
}

// ---- Wind dome ----

AATLAWindDome::AATLAWindDome()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Field = CreateDefaultSubobject<USphereComponent>(TEXT("Field"));
	SetRootComponent(Field);
	Field->SetSphereRadius(300.f);
	Field->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	Shell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shell"));
	Shell->SetupAttachment(Field);
	Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ATLAWaterVisuals::SetupWaterMesh(Shell);
	Shell->SetRelativeScale3D(FVector(5.6f));

	// Latitude bands spinning at odd tilts — the visible whirl of the dome
	for (int32 i = 0; i < 3; ++i)
	{
		UStaticMeshComponent* Band = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Band%d"), i));
		Band->SetupAttachment(Field);
		Band->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Band->SetRelativeScale3D(FVector(5.75f, 5.75f, 0.16f));
		ATLAWaterVisuals::SetupWaterMesh(Band);
		Bands.Add(Band);
	}
}

void AATLAWindDome::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(Duration);
	ATLAWaterVisuals::ApplyAirMaterial(Shell);
	for (UStaticMeshComponent* Band : Bands)
	{
		// Faint material — three big bands in the band material white the
		// screen out from inside the dome
		ATLAWaterVisuals::ApplyAirMaterial(Band);
	}

	if (AActor* MyInstigator = GetInstigator())
	{
		AttachToActor(MyInstigator, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(DeflectTimer, this, &AATLAWindDome::DeflectTick, 0.08f, true);
	}
}

void AATLAWindDome::DeflectTick()
{
	// The dome shreds any enemy projectile that enters
	TArray<AActor*> Projectiles;
	Field->GetOverlappingActors(Projectiles, AATLAWaterProjectile::StaticClass());
	for (AActor* Projectile : Projectiles)
	{
		if (Projectile->GetInstigator() != GetInstigator())
		{
			Projectile->Destroy();
		}
	}
}

void AATLAWindDome::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	// Gentle shimmer breathing
	const float Pulse = 5.6f + 0.25f * FMath::Sin(Age * 7.f);
	Shell->SetRelativeScale3D(FVector(Pulse));

	// Bands whirl on tilted axes at staggered speeds
	for (int32 i = 0; i < Bands.Num(); ++i)
	{
		const float Yaw = Age * (140.f + i * 60.f) + i * 120.f;
		Bands[i]->SetRelativeRotation(FRotator(i * 28.f - 28.f, Yaw, i * 20.f));
	}
}

// ---- Cyclone ----

AATLAAirCyclone::AATLAAirCyclone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	for (int32 i = 0; i < 16; ++i)
	{
		UStaticMeshComponent* Wisp = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Wisp%d"), i));
		Wisp->SetupAttachment(RootComponent);
		Wisp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ATLAWaterVisuals::SetupWaterMesh(Wisp);
		Wisps.Add(Wisp);
	}
}

void AATLAAirCyclone::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(Duration);
	for (UStaticMeshComponent* Wisp : Wisps)
	{
		ATLAWaterVisuals::ApplyAirBandMaterial(Wisp);
	}
}

void AATLAAirCyclone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Hand the rider back their normal air control when the spout dies
	if (bRiderBoarded && SavedAirControl >= 0.f)
	{
		if (ACharacter* RiderChar = Cast<ACharacter>(GetInstigator()))
		{
			RiderChar->GetCharacterMovement()->AirControl = SavedAirControl;
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AATLAAirCyclone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	// Funnel visual: stacked rings spinning, wider with height — sized so the
	// crown sits at RideHeight where the caster perches
	const float Rise = RideHeight / FMath::Max(Wisps.Num() - 1, 1);
	for (int32 i = 0; i < Wisps.Num(); ++i)
	{
		const float H = i * Rise;
		const float R = 50.f + i * 19.f;
		const float A = Age * (6.5f - i * 0.18f) + i * 0.9f;
		Wisps[i]->SetRelativeLocation(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, H));
		Wisps[i]->SetRelativeScale3D(FVector(1.9f, 0.5f, 0.42f));
		Wisps[i]->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, 0.f));
	}

	if (!HasAuthority())
	{
		return;
	}

	// The caster rides: inside the column they're carried up to the crown and
	// held there on a soft spring. Air control steers; walking out drops them.
	if (ACharacter* RiderChar = Cast<ACharacter>(GetInstigator()))
	{
		const FVector ToRider = RiderChar->GetActorLocation() - GetActorLocation();
		const bool bInColumn = ToRider.SizeSquared2D() < FMath::Square(Radius * 1.25f)
			&& ToRider.Z > -100.f && ToRider.Z < RideHeight + 250.f;
		if (bInColumn)
		{
			UCharacterMovementComponent* Move = RiderChar->GetCharacterMovement();
			if (Move->MovementMode == MOVE_Walking)
			{
				Move->SetMovementMode(MOVE_Falling);
			}
			if (SavedAirControl < 0.f)
			{
				SavedAirControl = Move->AirControl;
			}
			Move->AirControl = 1.f;
			const float TargetZ = GetActorLocation().Z + RideHeight;
			const float Err = TargetZ - RiderChar->GetActorLocation().Z;
			FVector V = Move->Velocity;
			V.Z = FMath::FInterpTo(V.Z, FMath::Clamp(Err * 3.2f, -260.f, 1050.f), DeltaTime, 8.f);
			// Gentle centering keeps the rider from sliding off the column edge
			V += -ToRider.GetSafeNormal2D() * FMath::Min(ToRider.Size2D() * 1.4f, 260.f) * DeltaTime * 6.f;
			Move->Velocity = V;
			bRiderBoarded = true;
		}
	}

	// Victims get lifted and spun
	TArray<AActor*> Characters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Characters);
	for (AActor* Target : Characters)
	{
		if (Target == GetInstigator() || IsDisplacementImmune(Target))
		{
			continue;
		}
		const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
		if (ToTarget.SizeSquared2D() > FMath::Square(Radius))
		{
			continue;
		}
		if (!Damaged.Contains(Target))
		{
			Damaged.Add(Target);
			AirApplyDamageTo(GetInstigator(), Target, UGE_AirDamage_Cyclone::StaticClass());
		}
		if (ACharacter* Victim = Cast<ACharacter>(Target))
		{
			// Suspend and swirl: upward hold plus tangential shove
			const FVector Tangent = FVector::CrossProduct(FVector::UpVector, ToTarget.GetSafeNormal2D());
			Victim->GetCharacterMovement()->Velocity = Tangent * 420.f + FVector(0.f, 0.f, 260.f);
		}
	}
}

// ---- Air scooter ball ----

AATLAAirScooterBall::AATLAAirScooterBall()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	CoreSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreSphere"));
	CoreSphere->SetupAttachment(RootComponent);
	CoreSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoreSphere->SetRelativeScale3D(FVector(0.8f));
	ATLAWaterVisuals::SetupWaterMesh(CoreSphere);

	for (int32 i = 0; i < 3; ++i)
	{
		UStaticMeshComponent* Band = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Band%d"), i));
		Band->SetupAttachment(RootComponent);
		Band->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Band->SetRelativeScale3D(FVector(0.86f, 0.86f, 0.16f));
		ATLAWaterVisuals::SetupWaterMesh(Band);
		Bands.Add(Band);
	}
}

void AATLAAirScooterBall::BeginPlay()
{
	Super::BeginPlay();

	// Safety net if the ability dies without cleaning us up
	SetLifeSpan(6.f);

	ATLAWaterVisuals::ApplyAirMaterial(CoreSphere);
	for (UStaticMeshComponent* Band : Bands)
	{
		ATLAWaterVisuals::ApplyAirBandMaterial(Band);
	}
}

void AATLAAirScooterBall::AttachToRider(ACharacter* InRider)
{
	if (!InRider)
	{
		return;
	}
	Rider = InRider;

	// Ball radius ~42: center 48 below the capsule center, resting on the ground
	AttachToComponent(InRider->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(FVector(0.f, 0.f, -48.f));

	// Lift the rider's mesh so their feet sit on top of the ball
	if (USkeletalMeshComponent* RiderMesh = InRider->GetMesh())
	{
		SavedMeshLocation = RiderMesh->GetRelativeLocation();
		SavedMeshRotation = RiderMesh->GetRelativeRotation();
		RiderMesh->SetRelativeLocation(FVector(SavedMeshLocation.X, SavedMeshLocation.Y, -6.f));
		bAdjustedRider = true;
	}
}

void AATLAAirScooterBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	// The wind bands roll like a spinning ball
	Bands[0]->SetRelativeRotation(FRotator(FMath::Fmod(Age * 640.f, 360.f), 0.f, 0.f));
	Bands[1]->SetRelativeRotation(FRotator(FMath::Fmod(Age * 520.f, 360.f), 60.f, 0.f));
	Bands[2]->SetRelativeRotation(FRotator(0.f, FMath::Fmod(Age * 460.f, 360.f), 75.f));

	// ...while the rider pirouettes on top, show-style
	if (bAdjustedRider && Rider.IsValid())
	{
		if (USkeletalMeshComponent* RiderMesh = Rider->GetMesh())
		{
			RiderMesh->SetRelativeRotation(FRotator(0.f, SavedMeshRotation.Yaw + Age * 420.f, 0.f));
		}
	}
}

void AATLAAirScooterBall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Set the rider back on the ground exactly as they were
	if (bAdjustedRider && Rider.IsValid())
	{
		if (USkeletalMeshComponent* RiderMesh = Rider->GetMesh())
		{
			RiderMesh->SetRelativeLocation(SavedMeshLocation);
			RiderMesh->SetRelativeRotation(SavedMeshRotation);
		}
	}
	Super::EndPlay(EndPlayReason);
}
