// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLABenderAI.h"
#include "ATLACharacter.h"
#include "Bending/ATLAAbility_WaterWhip.h"
#include "Bending/ATLAAbility_IceSpears.h"
#include "Bending/ATLAEarthAbilities.h"
#include "Bending/ATLAFireAbilities.h"
#include "Bending/ATLAAirAbilities.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AATLABenderAI::AATLABenderAI()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AATLABenderAI::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	StrafeDir = FMath::RandBool() ? 1.f : -1.f;
	GetWorldTimerManager().SetTimer(CastTimer, this, &AATLABenderAI::CastSomething,
		FMath::FRandRange(CastGapMin, CastGapMax), false);
}

void AATLABenderAI::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(CastTimer);
	Super::OnUnPossess();
}

void AATLABenderAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APawn* Self = GetPawn();
	ACharacter* Target = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!Self || !Target)
	{
		return;
	}

	// Always squared up to the opponent — the control rotation is also the
	// bender's aim (cast abilities trace along it), so aim at the chest
	const FVector ToTarget = (Target->GetActorLocation() + FVector(0.f, 0.f, 20.f)) - Self->GetActorLocation();
	SetControlRotation(ToTarget.Rotation());

	// Hold the fighting band; strafe inside it so it never stands still
	const float Dist = ToTarget.Size2D();
	const FVector Fwd = ToTarget.GetSafeNormal2D();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd);

	StrafePhase += DeltaTime;
	if (StrafePhase > FMath::FRandRange(2.5f, 4.f))
	{
		StrafePhase = 0.f;
		StrafeDir *= -1.f;      // circle the other way now and then
	}

	if (Dist > PreferredRangeMax)
	{
		Self->AddMovementInput(Fwd, 1.f);
	}
	else if (Dist < PreferredRangeMin)
	{
		Self->AddMovementInput(-Fwd, 0.7f);
	}
	else
	{
		Self->AddMovementInput(Side * StrafeDir, 0.55f);
	}
}

void AATLABenderAI::CastSomething()
{
	const AATLACharacter* Bender = Cast<AATLACharacter>(GetPawn());
	UAbilitySystemComponent* ASC = Bender ? Bender->GetAbilitySystemComponent() : nullptr;
	if (ASC)
	{
		TSubclassOf<UGameplayAbility> Jab;
		TArray<TSubclassOf<UGameplayAbility>> Heavies;
		switch (Bender->GetElementLoadout())
		{
		case EATLAElement::Water:
			Jab = UATLAAbility_WaterWhip::StaticClass();
			Heavies = { UATLAAbility_IceSpears::StaticClass() };
			break;
		case EATLAElement::Earth:
			Jab = UATLAAbility_RockJab::StaticClass();
			Heavies = { UATLAAbility_EarthSpikes::StaticClass(), UATLAAbility_Boulder::StaticClass() };
			break;
		case EATLAElement::Fire:
			Jab = UATLAAbility_FireJab::StaticClass();
			Heavies = { UATLAAbility_FireBlast::StaticClass(), UATLAAbility_FireLash::StaticClass() };
			break;
		default:
			Jab = UATLAAbility_AirBlast::StaticClass();
			Heavies = { UATLAAbility_AirSwipe::StaticClass() };
			break;
		}

		const bool bHeavy = Heavies.Num() > 0 && FMath::FRand() < HeavyChance;
		const TSubclassOf<UGameplayAbility> Pick = bHeavy
			? Heavies[FMath::RandRange(0, Heavies.Num() - 1)] : Jab;
		ASC->TryActivateAbilityByClass(Pick);
	}

	GetWorldTimerManager().SetTimer(CastTimer, this, &AATLABenderAI::CastSomething,
		FMath::FRandRange(CastGapMin, CastGapMax), false);
}
