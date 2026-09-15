// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAEarthWall.h"
#include "ATLAEarthBurst.h"
#include "ATLAGameplayEffects.h"
#include "ATLAGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "ATLAWaterVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AATLAEarthWall::AATLAEarthWall()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Wall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall"));
	SetRootComponent(Wall);
	Wall->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Wall->SetStaticMesh(CubeMesh.Object);
	}
}

void AATLAEarthWall::BeginPlay()
{
	Super::BeginPlay();

	ATLAWaterVisuals::ApplyEarthMaterial(Wall);
	SetLifeSpan(Lifetime);
	GroundZ = GetActorLocation().Z;

	Wall->SetWorldScale3D(FVector(WallSize.X / 100.f, WallSize.Y / 100.f, 0.02f));
}

void AATLAEarthWall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	if (!bSliding)
	{
		// Erupt: scale and lift so the base stays planted at ground level
		const float Rise = FMath::Clamp(Age / RiseTime, 0.05f, 1.f);
		const float HeightScale = (WallSize.Z / 100.f) * Rise;
		Wall->SetWorldScale3D(FVector(WallSize.X / 100.f, WallSize.Y / 100.f, HeightScale));
		SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, GroundZ + (HeightScale * 100.f) * 0.5f));
		return;
	}

	// Sliding: shove along the ground, plow through characters, shatter on solids
	if (!HasAuthority())
	{
		return;
	}

	const float Step = SlideSpeed * DeltaTime;
	FHitResult Hit;
	AddActorWorldOffset(SlideDir * Step, true, &Hit);
	SlideDistance += Step;

	if (Hit.bBlockingHit)
	{
		AActor* HitActor = Hit.GetActor();
		ACharacter* Victim = Cast<ACharacter>(HitActor);
		if (Victim && Victim != GetInstigator() && !SlideVictims.Contains(Victim))
		{
			SlideVictims.Add(Victim);

			// Damage through the bender's ability system
			UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim);
			if (SourceASC && TargetASC)
			{
				const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UGE_EarthDamage_Slab::StaticClass(), 1.f, SourceASC->MakeEffectContext());
				if (Spec.IsValid())
				{
					SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
				}
			}

			// Bulldoze them aside unless they're rooted/armored
			const bool bImmune = TargetASC && (TargetASC->HasMatchingGameplayTag(ATLATags::State_Earth_Rooted) ||
				TargetASC->HasMatchingGameplayTag(ATLATags::State_Earth_Armored));
			if (!bImmune)
			{
				Victim->LaunchCharacter(SlideDir * 750.f + FVector(0.f, 0.f, 320.f), true, true);
			}
			// Keep sliding next tick — the victim has been thrown clear
		}
		else if (!Victim)
		{
			// Solid obstacle: deliver structure damage if it's a wall, then break
			if (IATLAStructureDamageable* Structure = Cast<IATLAStructureDamageable>(HitActor))
			{
				Structure->ApplyStructureDamage(300.f);
			}
			Shatter();
			return;
		}
	}

	if (SlideDistance >= MaxSlideDistance)
	{
		Shatter();
	}
}

void AATLAEarthWall::Shatter()
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Right = GetActorRightVector();
	for (float Offset : { -140.f, 0.f, 140.f })
	{
		GetWorld()->SpawnActor<AATLAEarthBurst>(AATLAEarthBurst::StaticClass(), GetActorLocation() + Right * Offset + FVector(0.f, 0.f, 60.f), FRotator::ZeroRotator, SpawnParams);
	}
	Destroy();
}

void AATLAEarthWall::ApplyStructureDamage(float Amount)
{
	if (!HasAuthority())
	{
		return;
	}
	StructureHP -= Amount;
	if (StructureHP <= 0.f)
	{
		Destroy();
	}
}

void AATLAEarthWall::Push(const FRotator& Direction)
{
	if (!HasAuthority() || bSliding)
	{
		return;
	}
	bSliding = true;
	SlideDir = FRotator(0.f, Direction.Yaw, 0.f).Vector();
	// Extend the wall's life so a long slide can finish
	SetLifeSpan(4.f);
}
