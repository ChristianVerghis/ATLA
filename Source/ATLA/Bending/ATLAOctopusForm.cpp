// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAOctopusForm.h"
#include "ATLAGameplayEffects.h"
#include "ATLAWaterSplash.h"
#include "ATLAWaterVisuals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace
{
	constexpr int32 NumTendrils = 8;
	constexpr int32 SegmentsPerTendril = 10;
	constexpr int32 GrabTendril = 0;
	constexpr int32 LashTendril = 4; // opposite side from the grab arm
}

AATLAOctopusForm::AATLAOctopusForm()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	for (int32 t = 0; t < NumTendrils; ++t)
	{
		for (int32 s = 0; s < SegmentsPerTendril; ++s)
		{
			UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Tendril%d_Seg%d"), t, s));
			Segment->SetupAttachment(RootComponent);
			Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ATLAWaterVisuals::SetupWaterMesh(Segment);
			Segments.Add(Segment);
		}
	}

	UpdateTendrils();
}

void AATLAOctopusForm::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AATLAOctopusForm, GrabbedTarget);
	DOREPLIFETIME(AATLAOctopusForm, LashTarget);
}

void AATLAOctopusForm::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(Duration);

	for (UStaticMeshComponent* Segment : Segments)
	{
		ATLAWaterVisuals::ApplyWaterMaterial(Segment);
	}

	if (AActor* MyInstigator = GetInstigator())
	{
		AttachToActor(MyInstigator, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(LashTimer, this, &AATLAOctopusForm::LashNearbyTargets, LashInterval, true);
	}
}

void AATLAOctopusForm::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Never leave a victim frozen in the air when the form expires
	if (HasAuthority() && GrabbedTarget)
	{
		ReleaseGrabbedTarget(true);
	}
	Super::EndPlay(EndPlayReason);
}

void AATLAOctopusForm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	// The whole form slowly swirls around the bender
	AddActorLocalRotation(FRotator(0.f, 40.f * DeltaTime, 0.f));

	if (HasAuthority())
	{
		const float Now = GetWorld()->GetTimeSeconds();

		if (GrabbedTarget)
		{
			// The grab arm's hold point swings with the form's rotation,
			// so the victim gets carried around the bender
			const FVector HoldPoint = GetActorLocation() + GetActorRotation().Vector() * 280.f + FVector(0.f, 0.f, 170.f);
			const FVector NewLocation = FMath::VInterpTo(GrabbedTarget->GetActorLocation(), HoldPoint, DeltaTime, 7.f);
			GrabbedTarget->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

			if (Now - GrabStartTime >= GrabHoldTime)
			{
				ReleaseGrabbedTarget(true);
			}
		}

		if (LashTarget && Now >= LashVisualEndTime)
		{
			LashTarget = nullptr;
		}
	}

	UpdateTendrils();
}

void AATLAOctopusForm::UpdateTendrils()
{
	const FTransform ToLocal = GetActorTransform();

	for (int32 t = 0; t < NumTendrils; ++t)
	{
		// Grab arm reaches for its victim; lash arm snaps toward the last strike
		AActor* ReachTarget = nullptr;
		if (t == GrabTendril && GrabbedTarget)
		{
			ReachTarget = GrabbedTarget;
		}
		else if (t == LashTendril && LashTarget && LashTarget != GrabbedTarget)
		{
			ReachTarget = LashTarget;
		}

		// Build the arm's curve as a point chain, then skin it with overlapping
		// segments oriented along the curve — reads as one smooth, flaring tube
		FVector Points[SegmentsPerTendril + 1];

		if (ReachTarget)
		{
			// The arm stretches along an arc to its victim
			const FVector RelTarget = ToLocal.InverseTransformPosition(ReachTarget->GetActorLocation() + FVector(0.f, 0.f, 30.f));
			for (int32 s = 0; s <= SegmentsPerTendril; ++s)
			{
				const float Along = s / float(SegmentsPerTendril);
				const float Arc = FMath::Sin(Along * PI) * 55.f;
				Points[s] = RelTarget * Along + FVector(0.f, 0.f, Arc);
			}
		}
		else
		{
			const float Yaw = t * (360.f / NumTendrils);
			const FVector Outward = FRotator(0.f, Yaw, 0.f).Vector();
			const FVector Side = FVector::CrossProduct(Outward, FVector::UpVector);

			for (int32 s = 0; s <= SegmentsPerTendril; ++s)
			{
				// Rising, out-curling arc; a wave travels base to tip and the
				// tip flares the most
				const float Along = s / float(SegmentsPerTendril);
				const float Radius = 70.f + s * 27.f;
				const float Height = 8.f + Along * Along * 150.f;
				const float Wave = FMath::Sin(Age * 5.f + t * 0.8f + s * 0.55f);
				Points[s] = Outward * Radius + Side * (Wave * 30.f * Along) + FVector(0.f, 0.f, Height + Wave * 14.f * Along);
			}
		}

		for (int32 s = 0; s < SegmentsPerTendril; ++s)
		{
			UStaticMeshComponent* Segment = Segments[t * SegmentsPerTendril + s];
			const FVector Dir = Points[s + 1] - Points[s];
			const float Len = FMath::Max(Dir.Size(), 1.f);

			// Sphere stretched to bridge to the next point with generous
			// overlap; girth tapers from waist-thick to a fine tip
			const float Girth = FMath::Lerp(0.55f, 0.1f, s / float(SegmentsPerTendril - 1));
			Segment->SetRelativeLocation((Points[s] + Points[s + 1]) * 0.5f);
			Segment->SetRelativeRotation(Dir.Rotation());
			Segment->SetRelativeScale3D(FVector(Len / 100.f * 1.6f, Girth, Girth));
		}
	}
}

void AATLAOctopusForm::LashNearbyTargets()
{
	AActor* MyInstigator = GetInstigator();
	if (!MyInstigator)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyInstigator);
	if (!SourceASC)
	{
		return;
	}

	TArray<AActor*> Characters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Characters);

	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	for (AActor* Target : Characters)
	{
		if (Target == MyInstigator)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Target->GetActorLocation(), GetActorLocation());
		if (DistSq > FMath::Square(LashRange))
		{
			continue;
		}

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
		{
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UGE_Damage_OctopusLash::StaticClass(), 1.f, Context);
			if (Spec.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<AATLAWaterSplash>(AATLAWaterSplash::StaticClass(), Target->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

			if (DistSq < NearestDistSq)
			{
				Nearest = Target;
				NearestDistSq = DistSq;
			}
		}
	}

	if (Nearest)
	{
		// The lash arm visibly snaps toward the victim we just struck
		LashTarget = Nearest;
		LashVisualEndTime = GetWorld()->GetTimeSeconds() + 0.35f;

		// Seize one victim at a time if they're deep inside our reach
		if (!GrabbedTarget && NearestDistSq <= FMath::Square(GrabRange))
		{
			GrabbedTarget = Nearest;
			GrabStartTime = GetWorld()->GetTimeSeconds();
			if (ACharacter* Victim = Cast<ACharacter>(Nearest))
			{
				Victim->GetCharacterMovement()->StopMovementImmediately();
				Victim->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			}
		}
	}
}

void AATLAOctopusForm::ReleaseGrabbedTarget(bool bThrow)
{
	if (ACharacter* Victim = Cast<ACharacter>(GrabbedTarget))
	{
		Victim->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		if (bThrow)
		{
			const FVector Away = (Victim->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			Victim->LaunchCharacter(Away * ThrowSpeed + FVector(0.f, 0.f, 450.f), true, true);
		}
	}
	GrabbedTarget = nullptr;
}
