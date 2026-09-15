// Copyright Epic Games, Inc. All Rights Reserved.


#include "ATLAPlayerController.h"
#include "ATLACharacter.h"
#include "AI/ATLABenderAI.h"
#include "World/ATLAHabitats.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "Bending/ATLAAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "ATLA.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AATLAPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogATLA, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AATLAPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Sparring toggle rides the legacy binding path — no IMC edit needed
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AATLAPlayerController::ToggleSparringPartner);
	InputComponent->BindKey(EKeys::N, IE_Pressed, this, &AATLAPlayerController::ToggleColiseum);

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AATLAPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}


void AATLAPlayerController::ToggleSparringPartner()
{
	// Dismiss
	if (SparringPartner.IsValid())
	{
		if (AController* Brain = SparringPartner->GetController())
		{
			Brain->Destroy();
		}
		SparringPartner->Destroy();
		SparringPartner = nullptr;
		return;
	}

	// Summon: ahead of wherever the player is standing right now, facing them
	APawn* Me = GetPawn();
	if (!Me)
	{
		return;
	}
	UClass* PawnClass = LoadClass<APawn>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C"));
	if (!PawnClass)
	{
		return;
	}
	const FVector Fwd = FRotator(0.f, GetControlRotation().Yaw, 0.f).Vector();
	const FVector SpawnLoc = Me->GetActorLocation() + Fwd * 800.f + FVector(0.f, 0.f, 20.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	APawn* Enemy = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnLoc, (-Fwd).Rotation(), SpawnParams);
	if (!Enemy)
	{
		return;
	}
	if (AATLACharacter* Bender = Cast<AATLACharacter>(Enemy))
	{
		static const EATLAElement Elements[] = {
			EATLAElement::Water, EATLAElement::Earth, EATLAElement::Fire, EATLAElement::Air };
		Bender->SetElementLoadout(Elements[FMath::RandRange(0, 3)]);
		// SetElementLoadout teleports to the element's home square — sparring
		// happens HERE, on whatever square the player picked
		Bender->SetActorLocation(SpawnLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}
	if (AATLABenderAI* Brain = GetWorld()->SpawnActor<AATLABenderAI>(AATLABenderAI::StaticClass()))
	{
		Brain->Possess(Enemy);
	}
	SparringPartner = Enemy;

	// The duel is on: first bender to fall loses. On the arena it's the show's
	// pro-bending minigame; anywhere else the same rule keeps sparring honest.
	bDuelSettled = false;
	GetWorldTimerManager().SetTimer(DuelTimer, this, &AATLAPlayerController::CheckDuel, 0.25f, true);
	if (GEngine)
	{
		bool bAtArena = false;
		if (TActorIterator<AATLAColiseum> It(GetWorld()); It)
		{
			bAtArena = FVector::Dist2D(Me->GetActorLocation(), It->GetActorLocation()) < 7000.f;
		}
		GEngine->AddOnScreenDebugMessage(7, 4.f, FColor::Yellow,
			bAtArena ? TEXT("DUEL! First bender to fall loses the arena.")
			         : TEXT("Sparring partner summoned - first to fall loses."));
	}
}

void AATLAPlayerController::CheckDuel()
{
	if (bDuelSettled)
	{
		return;
	}
	const AATLACharacter* Me = Cast<AATLACharacter>(GetPawn());
	if (!Me)
	{
		return;
	}
	if (!SparringPartner.IsValid())
	{
		GetWorldTimerManager().ClearTimer(DuelTimer);
		return;
	}
	const AATLACharacter* Foe = Cast<AATLACharacter>(SparringPartner.Get());
	const float MyHealth = Me->GetHealth();
	const float FoeHealth = Foe ? Foe->GetHealth() : 1.f;
	if (MyHealth > 0.f && FoeHealth > 0.f)
	{
		return;
	}

	// Settled: announce, knock the loser down, and let the moment breathe
	// before the cleanup pass heals and clears the field
	bDuelSettled = true;
	GetWorldTimerManager().ClearTimer(DuelTimer);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(7, 6.f, FoeHealth <= 0.f ? FColor::Green : FColor::Red,
			FoeHealth <= 0.f ? TEXT("VICTORY - your opponent falls!") : TEXT("DEFEAT - you fall. Press B to rematch."));
	}

	// The brain stops fighting immediately either way
	if (AController* Brain = SparringPartner->GetController())
	{
		Brain->Destroy();
	}

	if (FoeHealth <= 0.f)
	{
		KnockDown(Cast<ACharacter>(SparringPartner.Get()));
	}
	if (MyHealth <= 0.f)
	{
		ACharacter* MeChar = Cast<ACharacter>(GetPawn());
		if (MeChar && MeChar->GetMesh())
		{
			SavedMeshLocation = MeChar->GetMesh()->GetRelativeLocation();
			SavedMeshRotation = MeChar->GetMesh()->GetRelativeRotation();
		}
		KnockDown(MeChar);
		bPlayerKnockedDown = true;
	}

	GetWorldTimerManager().SetTimer(DuelCleanupTimer, this, &AATLAPlayerController::FinishDuelCleanup, 2.6f, false);
}

void AATLAPlayerController::KnockDown(ACharacter* Loser)
{
	if (!Loser || !Loser->GetMesh())
	{
		return;
	}
	Loser->GetCharacterMovement()->DisableMovement();
	Loser->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* Mesh = Loser->GetMesh();
	Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
	Mesh->SetSimulatePhysics(true);
	// A last shove so the fall reads as a hit, not a faint
	Mesh->AddImpulse(FVector(0.f, 0.f, 12000.f) + Loser->GetActorForwardVector() * -22000.f, NAME_None, true);
}

void AATLAPlayerController::FinishDuelCleanup()
{
	AATLACharacter* Me = Cast<AATLACharacter>(GetPawn());

	// Stand the player back up if they were the one on the floor
	if (bPlayerKnockedDown && Me && Me->GetMesh())
	{
		USkeletalMeshComponent* Mesh = Me->GetMesh();
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));
		Mesh->AttachToComponent(Me->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Mesh->SetRelativeLocation(SavedMeshLocation);
		Mesh->SetRelativeRotation(SavedMeshRotation);
		Me->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Me->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		bPlayerKnockedDown = false;
	}

	if (Me)
	{
		if (UAbilitySystemComponent* ASC = Me->GetAbilitySystemComponent())
		{
			ASC->ApplyModToAttribute(UATLAAttributeSet::GetHealthAttribute(), EGameplayModOp::Override,
				ASC->GetNumericAttribute(UATLAAttributeSet::GetMaxHealthAttribute()));
		}
	}

	if (SparringPartner.IsValid())
	{
		SparringPartner->Destroy();
	}
	SparringPartner = nullptr;
}


void AATLAPlayerController::ToggleColiseum()
{
	AATLACharacter* Me = Cast<AATLACharacter>(GetPawn());
	if (!Me)
	{
		return;
	}
	AATLAColiseum* Arena = nullptr;
	if (TActorIterator<AATLAColiseum> It(GetWorld()); It)
	{
		Arena = *It;
	}
	if (!Arena)
	{
		return;
	}
	// Near the arena -> go home; anywhere else -> step onto the platform
	const bool bAtArena = FVector::Dist2D(Me->GetActorLocation(), Arena->GetActorLocation()) < 7000.f;
	const FVector Destination = bAtArena
		? AATLACharacter::GetZoneAnchor(Me->GetElementLoadout())
		: Arena->GetPlatformTop();
	Me->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);

	// The sparring partner follows the fight
	if (SparringPartner.IsValid())
	{
		SparringPartner->SetActorLocation(Destination + FVector(600.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
}


void AATLAPlayerController::DebugSettleDuel()
{
	if (const AATLACharacter* Foe = Cast<AATLACharacter>(SparringPartner.Get()))
	{
		if (UAbilitySystemComponent* ASC = Foe->GetAbilitySystemComponent())
		{
			ASC->ApplyModToAttribute(UATLAAttributeSet::GetHealthAttribute(), EGameplayModOp::Override, 0.f);
		}
	}
}
