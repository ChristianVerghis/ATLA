// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAEarthSpikeLine.h"
#include "ATLAGameplayEffects.h"
#include "ATLAGameplayTags.h"
#include "ATLAWaterVisuals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 NumSpikes = 6;
}

AATLAEarthSpikeLine::AATLAEarthSpikeLine()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

	for (int32 i = 0; i < NumSpikes; ++i)
	{
		UStaticMeshComponent* Spike = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Spike%d"), i));
		Spike->SetupAttachment(RootComponent);
		Spike->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Spike->SetUsingAbsoluteLocation(true);
		Spike->SetUsingAbsoluteRotation(true);
		Spike->SetUsingAbsoluteScale(true);
		Spike->SetVisibility(false);
		if (ConeMesh.Succeeded())
		{
			Spike->SetStaticMesh(ConeMesh.Object);
		}
		// Slight lean per spike so the line reads jagged, not manufactured
		Spike->SetWorldRotation(FRotator((i % 2 == 0) ? 6.f : -5.f, i * 47.f, (i % 3 == 0) ? -4.f : 5.f));
		Spikes.Add(Spike);
		EruptTimes.Add(-1.f);
	}

	SetLifeSpan(2.4f);
}

void AATLAEarthSpikeLine::BeginPlay()
{
	Super::BeginPlay();

	for (UStaticMeshComponent* Spike : Spikes)
	{
		ATLAWaterVisuals::ApplyEarthMaterial(Spike);
	}

	// Lay out the line along our facing, snapping each spike to the floor
	const FVector Dir = GetActorForwardVector().GetSafeNormal2D();
	for (int32 i = 0; i < NumSpikes; ++i)
	{
		const FVector Probe = GetActorLocation() + Dir * (Spacing * i) + FVector(0.f, 0.f, 100.f);
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(GetInstigator());
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Probe, Probe - FVector(0.f, 0.f, 500.f), ECC_Visibility, Params);
		SpikeBases.Add(bHit ? Hit.ImpactPoint : GetActorLocation() + Dir * (Spacing * i));
	}

	EruptNext();
	GetWorldTimerManager().SetTimer(EruptTimer, this, &AATLAEarthSpikeLine::EruptNext, EruptInterval, true);
}

void AATLAEarthSpikeLine::EruptNext()
{
	if (NextSpike >= NumSpikes)
	{
		GetWorldTimerManager().ClearTimer(EruptTimer);
		return;
	}

	const int32 i = NextSpike++;
	EruptTimes[i] = Age;
	Spikes[i]->SetVisibility(true);
	Spikes[i]->SetWorldLocation(SpikeBases[i]);

	// Damage + pop-up anyone standing over this spike (server only)
	if (!HasAuthority())
	{
		return;
	}
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());

	TArray<AActor*> Characters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Characters);
	for (AActor* Target : Characters)
	{
		if (Target == GetInstigator() || Victims.Contains(Target))
		{
			continue;
		}
		if (FVector::DistSquared2D(Target->GetActorLocation(), SpikeBases[i]) > FMath::Square(HitRadius))
		{
			continue;
		}
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!TargetASC)
		{
			continue;
		}
		Victims.Add(Target);

		if (SourceASC)
		{
			const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UGE_EarthDamage_Spike::StaticClass(), 1.f, SourceASC->MakeEffectContext());
			if (Spec.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}

		const bool bImmune = TargetASC->HasMatchingGameplayTag(ATLATags::State_Earth_Rooted) ||
			TargetASC->HasMatchingGameplayTag(ATLATags::State_Earth_Armored);
		if (!bImmune)
		{
			if (ACharacter* Victim = Cast<ACharacter>(Target))
			{
				Victim->LaunchCharacter(FVector(0.f, 0.f, PopUpSpeed), false, true);
			}
		}
	}
}

void AATLAEarthSpikeLine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	for (int32 i = 0; i < Spikes.Num(); ++i)
	{
		if (EruptTimes[i] < 0.f)
		{
			continue;
		}
		const float SpikeAge = Age - EruptTimes[i];

		// Burst up over 0.12s, hold, then sink after 1.1s
		float H;
		if (SpikeAge < 0.12f)
		{
			H = SpikeAge / 0.12f;
		}
		else if (SpikeAge < 1.1f)
		{
			H = 1.f;
		}
		else
		{
			H = FMath::Max(0.02f, 1.f - (SpikeAge - 1.1f) / 0.5f);
		}

		Spikes[i]->SetWorldScale3D(FVector(0.55f, 0.55f, 1.7f * H));
		Spikes[i]->SetWorldLocation(SpikeBases[i] + FVector(0.f, 0.f, 85.f * H * 0.5f));
	}
}
