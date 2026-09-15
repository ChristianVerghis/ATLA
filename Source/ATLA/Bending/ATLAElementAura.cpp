// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAElementAura.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ATLAWaterVisuals.h"
#include "ATLAGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"

AATLAElementAura::AATLAElementAura()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Bit = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Bit%d"), i));
		Bit->SetupAttachment(RootComponent);
		Bit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ATLAWaterVisuals::SetupWaterMesh(Bit);
		Bits.Add(Bit);
	}
}

void AATLAElementAura::BeginPlay()
{
	Super::BeginPlay();

	for (UStaticMeshComponent* Bit : Bits)
	{
		switch (Element)
		{
		case EATLAElement::Water: ATLAWaterVisuals::ApplyWaterMaterial(Bit); break;
		case EATLAElement::Earth: ATLAWaterVisuals::ApplyEarthMaterial(Bit); break;
		case EATLAElement::Fire:  ATLAWaterVisuals::ApplyFireMaterial(Bit); break;
		case EATLAElement::Air:   ATLAWaterVisuals::ApplyAirMaterial(Bit); break;
		}
	}

	// Fire only wants two flames (the fists); hide the spares
	if (Element == EATLAElement::Fire)
	{
		Bits[2]->SetVisibility(false);
		Bits[3]->SetVisibility(false);
	}

	if (AActor* MyInstigator = GetInstigator())
	{
		AttachToActor(MyInstigator, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		// Fire: real torch flames on the fists (mesh flickers hide entirely)
		if (Element == EATLAElement::Fire)
		{
			if (UNiagaraSystem* Torch = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/Fire/Loop/NS_Sub_FireTorch_Loop_002.NS_Sub_FireTorch_Loop_002")))
			{
				if (const ACharacter* Bender = Cast<ACharacter>(MyInstigator))
				{
					for (const FName Socket : { FName(TEXT("hand_l")), FName(TEXT("hand_r")) })
					{
						if (UNiagaraComponent* Flame = UNiagaraFunctionLibrary::SpawnSystemAttached(
							Torch, Bender->GetMesh(), Socket, FVector::ZeroVector, FRotator::ZeroRotator,
							EAttachLocation::SnapToTarget, false))
						{
							Flame->SetWorldScale3D(FVector(0.22f));
							FlameFX.Add(Flame);
						}
					}
					for (UStaticMeshComponent* Bit : Bits)
					{
						Bit->SetVisibility(false);
					}
				}
			}
		}
	}
}

void AATLAElementAura::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UNiagaraComponent* Flame : FlameFX)
	{
		if (Flame)
		{
			Flame->DestroyComponent();
		}
	}
	FlameFX.Reset();
	Super::EndPlay(EndPlayReason);
}

void AATLAElementAura::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;

	const ACharacter* Bender = Cast<ACharacter>(GetInstigator());

	switch (Element)
	{
	case EATLAElement::Water:
	{
		// Two thin streams orbiting the waist at different heights/phases
		for (int32 i = 0; i < Bits.Num(); ++i)
		{
			const float A = Age * 2.2f + i * (PI / 2.f);
			const float R = 85.f + 8.f * FMath::Sin(Age * 3.f + i);
			Bits[i]->SetRelativeLocation(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 15.f + 22.f * FMath::Sin(Age * 1.7f + i * 1.3f)));
			Bits[i]->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, 0.f));
			Bits[i]->SetRelativeScale3D(FVector(0.55f, 0.12f, 0.12f));
		}
		break;
	}
	case EATLAElement::Earth:
	{
		// Pebbles drifting slowly, bobbing at shoulder height
		for (int32 i = 0; i < Bits.Num(); ++i)
		{
			const float A = Age * 0.9f + i * (2.f * PI / 4.f);
			Bits[i]->SetRelativeLocation(FVector(FMath::Cos(A) * 70.f, FMath::Sin(A) * 70.f, 45.f + 14.f * FMath::Sin(Age * 1.3f + i * 2.f)));
			Bits[i]->SetRelativeRotation(FRotator(Age * 30.f + i * 90.f, Age * 45.f, 0.f));
			Bits[i]->SetRelativeScale3D(FVector(0.1f + 0.03f * (i % 2)));
		}
		break;
	}
	case EATLAElement::Air:
	{
		// Fast faint wisps circling wide
		for (int32 i = 0; i < Bits.Num(); ++i)
		{
			const float A = Age * 4.5f + i * (2.f * PI / 4.f);
			Bits[i]->SetRelativeLocation(FVector(FMath::Cos(A) * 105.f, FMath::Sin(A) * 105.f, 5.f + 45.f * FMath::Sin(Age * 2.4f + i)));
			Bits[i]->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, 20.f));
			Bits[i]->SetRelativeScale3D(FVector(0.7f, 0.1f, 0.08f));
		}
		break;
	}
	case EATLAElement::Fire:
	{
		// Burning fists are the SIGNATURE FORM, not the resting state — a
		// firebender's hands are only alight while Inferno Nova's empowerment
		// is running. Ambient fire shows nothing.
		bool bEmpowered = false;
		if (const UAbilitySystemComponent* ASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator()))
		{
			bEmpowered = ASC->HasMatchingGameplayTag(ATLATags::State_Bending_Empowered);
		}
		// Visibility is the authoritative gate: Niagara keeps reporting IsActive
		// after Deactivate() until the system finishes, so toggling on that
		// alone leaves the flames burning.
		for (UNiagaraComponent* Flame : FlameFX)
		{
			if (!Flame || Flame->IsVisible() == bEmpowered)
			{
				continue;
			}
			Flame->SetVisibility(bEmpowered, true);
			bEmpowered ? Flame->Activate(true) : Flame->Deactivate();
		}
		for (int32 i = 0; i < 2 && i < Bits.Num(); ++i)
		{
			Bits[i]->SetVisibility(bEmpowered && FlameFX.Num() == 0);
		}
		if (!bEmpowered)
		{
			break;
		}

		// Flames flickering at the fists — generated from the bender, not orbiting
		if (Bender && Bender->GetMesh())
		{
			const FName Fists[2] = { TEXT("hand_l"), TEXT("hand_r") };
			for (int32 i = 0; i < 2; ++i)
			{
				const float Flicker = 0.12f + 0.05f * FMath::Sin(Age * 17.f + i * 2.6f) + 0.03f * FMath::Sin(Age * 31.f + i);
				Bits[i]->SetWorldLocation(Bender->GetMesh()->GetSocketLocation(Fists[i]) + FVector(0.f, 0.f, 6.f));
				Bits[i]->SetWorldScale3D(FVector(Flicker, Flicker, Flicker * 1.9f));
			}
		}
		break;
	}
	}
}
