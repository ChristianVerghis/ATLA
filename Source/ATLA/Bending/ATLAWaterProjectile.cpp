// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAWaterProjectile.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "ATLAGameplayEffects.h"
#include "ATLAGameplayTags.h"
#include "ATLAWaterSplash.h"
#include "ATLAWaterVisuals.h"
#include "ATLAStructureDamageable.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 NumTrailDrops = 32;
}

AATLAWaterProjectile::AATLAWaterProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
	InitialLifeSpan = 3.f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(16.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->OnComponentHit.AddDynamic(this, &AATLAWaterProjectile::OnHit);
	SetRootComponent(Collision);

	// The visible water head: elongated along the flight direction
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.55f, 0.3f, 0.3f));
	ATLAWaterVisuals::SetupWaterMesh(Mesh);

	// Trail droplets: world-locked ring buffer dropped along the path
	for (int32 i = 0; i < NumTrailDrops; ++i)
	{
		UStaticMeshComponent* Drop = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("TrailDrop%d"), i));
		Drop->SetupAttachment(Collision);
		Drop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Drop->SetUsingAbsoluteLocation(true);
		Drop->SetUsingAbsoluteRotation(true);
		Drop->SetUsingAbsoluteScale(true);
		Drop->SetVisibility(false);
		ATLAWaterVisuals::SetupWaterMesh(Drop);
		TrailDrops.Add(Drop);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 2200.f;
	Movement->MaxSpeed = 2200.f;
	Movement->ProjectileGravityScale = 0.15f;
	Movement->bRotationFollowsVelocity = true;

	DamageEffect = UGE_Damage_WaterWhip::StaticClass();
	ImpactBurstClass = AATLAWaterSplash::StaticClass();

	// The whip flows as velocity-stretched mesh segments. (The example ribbon
	// asset renders RED with no exposed tint — it read as fire flashes inside
	// water bending — so ribbons stay off until a properly colored one exists.)
	RibbonTrail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RibbonTrail"));
	RibbonTrail->SetupAttachment(Collision);
}

void AATLAWaterProjectile::BeginPlay()
{
	Super::BeginPlay();

	ApplyMaterials();

	LastEmitLocation = Mesh->GetComponentLocation();
	BaseMeshScale = Mesh->GetRelativeScale3D();

	// Never collide with the bender who cast it
	if (AActor* MyInstigator = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(MyInstigator, true);

		// Siblings from the same bender must ignore each other PHYSICALLY,
		// not just skip damage in OnHit — a blocked projectile has already
		// been stopped by physics before OnHit runs, and one stopped bolt
		// mid-air corks every following one (rapid streams pile into an
		// invisible wall at the muzzle).
		TArray<AActor*> Siblings;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATLAWaterProjectile::StaticClass(), Siblings);
		for (AActor* Sibling : Siblings)
		{
			if (Sibling != this && Sibling->GetInstigator() == MyInstigator)
			{
				IgnoreActorForMovement(Sibling);
				CastChecked<AATLAWaterProjectile>(Sibling)->IgnoreActorForMovement(this);
			}
		}

		// A live signature form supercharges regular attacks: the projectile
		// grows — bigger hit sphere, bigger read (damage amp is central in
		// the attribute set via the captured Empowered source tag)
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyInstigator))
		{
			if (ASC->HasMatchingGameplayTag(ATLATags::State_Bending_Empowered))
			{
				bEmpowered = true;
				SetActorScale3D(GetActorScale3D() * 1.45f);
			}
		}
	}
}

void AATLAWaterProjectile::ApplyMaterials()
{
	ATLAWaterVisuals::ApplyWaterMaterial(Mesh);
	for (UStaticMeshComponent* Drop : TrailDrops)
	{
		ATLAWaterVisuals::ApplyWaterMaterial(Drop);
	}
}

void AATLAWaterProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	// The head weaves around the true flight path — pure visual, collision stays honest
	const float Phase = Age * SerpentineFrequency;
	Mesh->SetRelativeLocation(FVector(
		0.f,
		FMath::Sin(Phase) * SerpentineAmplitude,
		FMath::Cos(Phase * 0.6f) * SerpentineAmplitude * 0.45f));

	if (MeshFlickerAmp > 0.f)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale * (1.f + MeshFlickerAmp * (FMath::FRand() - 0.4f)));
	}

	// Lay stream segments at fixed distances along the flown path (not once per
	// frame) so the trail stays gapless at any speed or framerate. Segments are
	// stretched along the flight direction; overlapping, they read as one flow.
	if (!bUseMeshTrail)
	{
		return;
	}
	const FVector HeadPos = Mesh->GetComponentLocation();
	FVector ToHead = HeadPos - LastEmitLocation;
	while (ToHead.Size() >= EmitSpacing)
	{
		LastEmitLocation += ToHead.GetSafeNormal() * EmitSpacing;

		UStaticMeshComponent* Drop = TrailDrops[NextTrailDrop];
		NextTrailDrop = (NextTrailDrop + 1) % TrailDrops.Num();
		Drop->SetWorldLocation(LastEmitLocation);
		Drop->SetWorldRotation(GetActorRotation());
		Drop->SetWorldScale3D(FVector(TrailDropScale * 3.f, TrailDropScale * 0.85f, TrailDropScale * 0.85f));
		Drop->SetVisibility(true);

		ToHead = HeadPos - LastEmitLocation;
	}

	// All droplets shrink toward nothing (flame trails also lick upward)
	for (UStaticMeshComponent* Trail : TrailDrops)
	{
		if (Trail->IsVisible())
		{
			const FVector Scale = Trail->GetComponentScale() * (1.f - DeltaTime * 3.f);
			if (Scale.X < 0.02f)
			{
				Trail->SetVisibility(false);
			}
			else
			{
				Trail->SetWorldScale3D(Scale);
				if (TrailRiseSpeed > 0.f)
				{
					Trail->AddWorldOffset(FVector(0.f, 0.f, TrailRiseSpeed * DeltaTime));
				}
			}
		}
	}
}

void AATLAWaterProjectile::HoldInPlace(float RiseSpeed)
{
	// Torn loose but not thrown yet: drifts up out of the ground, no gravity,
	// and can't hurt anything until it's launched
	Movement->Velocity = FVector(0.f, 0.f, RiseSpeed);
	Movement->ProjectileGravityScale = 0.f;
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AATLAWaterProjectile::LaunchToward(const FRotator& Direction)
{
	SetActorRotation(Direction);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Movement->Velocity = Direction.Vector() * Movement->InitialSpeed;
	Movement->ProjectileGravityScale = LaunchGravityScale;
	Movement->UpdateComponentVelocity();
}

void AATLAWaterProjectile::IgnoreActorForMovement(AActor* Other)
{
	if (Other)
	{
		Collision->IgnoreActorWhenMoving(Other, true);
	}
}

void AATLAWaterProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Friendly projectiles from the same bender pass through each other
	if (Cast<AATLAWaterProjectile>(OtherActor) && OtherActor->GetInstigator() == GetInstigator())
	{
		return;
	}

	if (HasAuthority() && OtherActor && OtherActor != GetInstigator())
	{
		if (IATLAStructureDamageable* Structure = Cast<IATLAStructureDamageable>(OtherActor))
		{
			Structure->ApplyStructureDamage(StructureDamage);
		}

		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);

		if (SourceASC && TargetASC && DamageEffect)
		{
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddHitResult(Hit);
			const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);
			if (Spec.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}

		if (TargetASC)
		{
			OnImpactVictim(OtherActor, Hit);
		}

		// Impact burst replicates to all clients
		if (ImpactBurstClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<AATLAWaterSplash>(ImpactBurstClass, Hit.ImpactPoint + Hit.ImpactNormal * 8.f, FRotator::ZeroRotator, SpawnParams);
		}
	}

	Destroy();
}
