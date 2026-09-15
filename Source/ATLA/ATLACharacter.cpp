// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLACharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CharacterTrajectoryComponent.h"
#include "AbilitySystemComponent.h"
#include "ATLAAttributeSet.h"
#include "ATLADrawStream.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "ATLAAbility_WaterWhip.h"
#include "ATLAAbility_DrawWater.h"
#include "ATLAAbility_IceSpears.h"
#include "ATLAAbility_IceWall.h"
#include "ATLAAbility_Octopus.h"
#include "ATLAAbility_Dodge.h"
#include "ATLAEarthAbilities.h"
#include "ATLAFireAbilities.h"
#include "ATLAAirAbilities.h"
#include "ATLAElementAura.h"
#include "ATLAWaterSource.h"
#include "ATLAIceWall.h"
#include "ATLAEarthWall.h"
#include "ATLA.h"

AATLACharacter::AATLACharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Motion matching queries against where the character is about to be, so it
	// needs a trajectory generated every frame regardless of which graph runs.
	TrajectoryComponent = CreateDefaultSubobject<UCharacterTrajectoryComponent>(TEXT("CharacterTrajectory"));

	// GASP's retarget graph picks its IK retargeter by reading the mesh's first
	// component tag (an async asset load on ComponentTags[0]); with no tag it
	// errors out and the pose freezes. This is the same wiring BP_Quinn uses.
	GetMesh()->ComponentTags.Add(FName(TEXT("/Game/Characters/UE5_Mannequins/Rigs/RTG_UEFN_to_UE5_Mannequin.RTG_UEFN_to_UE5_Mannequin")));

	MotionMatchingAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(
		TEXT("/Game/Blueprints/RetargetedCharacters/ABP_GenericRetarget.ABP_GenericRetarget_C")));

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// Benders pivot sharply; 500/s lags the camera noticeably when strafing
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Ability system: replicated, Mixed mode (gameplay effects replicate to owner
	// only; tags and cues replicate to everyone — the standard player setup)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	Attributes = CreateDefaultSubobject<UATLAAttributeSet>(TEXT("Attributes"));

}

void AATLACharacter::BeginPlay()
{
	Super::BeginPlay();

	// Higher, further camera with a touch of lag for weight
	CameraBoom->TargetArmLength = CameraArmLength;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, CameraHeightOffset);
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	// Bending montages must never hijack movement: play their lunges in place
	// so the player keeps full control mid-strike
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}

	// Bender agility: a touch faster than the template baseline
	// Matched to the locomotion blend space's fastest sample (600). Running
	// faster than the animation is authored for means the feet can never keep
	// up with the ground and slide the whole time — the character was pinned
	// past the end of the graph at 620.
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxAcceleration = 2400.f;
	GetCharacterMovement()->JumpZVelocity = 560.f;
	GetCharacterMovement()->AirControl = 0.5f;

	// Earth is positional: poll whether we stand on bendable ground
	GetWorldTimerManager().SetTimer(GroundedTimer, this, &AATLACharacter::UpdateGroundedState, 0.15f, true);

	// Hand the mesh to GASP's motion-matching graph. Done here rather than in
	// the constructor so the class is resolved after content is loaded, and so
	// the flag can be flipped to A/B against the old blend-space locomotion.
	if (bUseMotionMatching)
	{
		if (UClass* MMClass = MotionMatchingAnimClass.LoadSynchronous())
		{
			GetMesh()->SetAnimInstanceClass(MMClass);
			UE_LOG(LogATLA, Log, TEXT("Locomotion: motion matching enabled (%s)"), *MMClass->GetName());
		}
		else
		{
			UE_LOG(LogATLA, Warning, TEXT("Locomotion: motion-matching AnimBP missing; staying on the blend space"));
		}
	}

	// Watchdog for the pose glitches that only show up in real play
	if (IsPlayerControlled())
	{
		GetWorldTimerManager().SetTimer(PoseWatchTimer, this, &AATLACharacter::CheckPoseSanity, 0.05f, true);
	}

	// Every bender's inner fire cools on its own clock (AI benders included)
	GetWorldTimerManager().SetTimer(DriveDecayTimer, this, &AATLACharacter::DecayInnerDrive, 0.1f, true);

	// Niagara pre-warm: the FIRST spawn of a pack system can hitch (or on some
	// Metal drivers, hang) while shaders/PSOs compile. Fire each combat system
	// once, far below the map, so real casts never pay that cost.
	static TWeakObjectPtr<UWorld> WarmedWorld;
	if (IsPlayerControlled() && WarmedWorld.Get() != GetWorld())
	{
		WarmedWorld = GetWorld();
		static const TCHAR* WarmSystems[] = {
			TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Small.NS_Fire_Small"),
			TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Medium.NS_Fire_Medium"),
			TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/EXP/NS_Sub_EXP_Small_002.NS_Sub_EXP_Small_002"),
			TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/EXP/NS_Sub_EXP_Large_001_01.NS_Sub_EXP_Large_001_01"),
			TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/Fire/Loop/NS_Sub_FireTorch_Loop_002.NS_Sub_FireTorch_Loop_002"),
			TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/Fire/Loop/NS_Sub_FireSmall_Loop_001.NS_Sub_FireSmall_Loop_001"),
			TEXT("/Game/NiagaraExamples/FX_Sparks/NS_Spark_Burst.NS_Spark_Burst"),
			TEXT("/Game/NiagaraExamples/FX_Sparks/NS_Spark_Continuous.NS_Spark_Continuous"),
			TEXT("/Game/NiagaraExamples/FX_Explosions/NS_Dirt_Explosion_Small.NS_Dirt_Explosion_Small"),
			TEXT("/Game/NiagaraExamples/FX_Weapons/Trails/NS_SimpleRibbonTrail.NS_SimpleRibbonTrail"),
		};
		const FVector FarBelow(0.f, 0.f, -8000.f);
		for (const TCHAR* Path : WarmSystems)
		{
			if (UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, Path))
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, FarBelow);
			}
		}
	}
}

void AATLACharacter::CheckPoseSanity()
{
	USkeletalMeshComponent* Mesh = GetMesh();
	if (!Mesh || !GetCapsuleComponent())
	{
		return;
	}

	const float FloorZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Pelvis = Mesh->GetSocketLocation(TEXT("pelvis"));
	const FRotator PelvisRot = Mesh->GetSocketRotation(TEXT("pelvis"));
	const FVector FootR = Mesh->GetSocketLocation(TEXT("foot_r"));
	const FVector FootL = Mesh->GetSocketLocation(TEXT("foot_l"));

	const float LowestFoot = FMath::Min(FootR.Z, FootL.Z) - FloorZ;
	const float LongestLeg = FMath::Max((FootR - Pelvis).Size(), (FootL - Pelvis).Size());
	// Upright reads ~175-180 on this rig; a horizontal body drops toward 90.
	// Measure deviation from upright, not from an assumed axis.
	const float PelvisTip = 180.f - FMath::Abs(FRotator::NormalizeAxis(PelvisRot.Pitch));

	// Legs folding into the torso is the signature of a broken pose: a healthy
	// leg spans ~88cm from the pelvis, and anything under 45 is a collapse.
	const bool bBroken = LowestFoot < -12.f || LongestLeg > 115.f || LongestLeg < 45.f ||
		PelvisTip > 45.f || (Pelvis.Z - FloorZ) < 60.f;
	if (!bBroken)
	{
		return;
	}

	// Throttle: one complaint per second is plenty to identify the culprit
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastPoseComplaintTime < 1.f)
	{
		return;
	}
	LastPoseComplaintTime = Now;

	FString MontageName(TEXT("none"));
	if (const UAnimInstance* Anim = Mesh->GetAnimInstance())
	{
		if (const UAnimMontage* Active = Anim->GetCurrentActiveMontage())
		{
			MontageName = Active->GetName();
			if (Active->SlotAnimTracks.Num() > 0)
			{
				MontageName += FString::Printf(TEXT(" slot=%s"), *Active->SlotAnimTracks[0].SlotName.ToString());
			}
		}
	}

	static const TCHAR* ElementNames[] = { TEXT("Water"), TEXT("Earth"), TEXT("Fire"), TEXT("Air") };
	UE_LOG(LogATLA, Warning,
		TEXT("POSEWATCH element=%s foot=%+.0f legLen=%.0f pelvisTip=%.0f pelvisZ=%.0f mode=%d vel=%.0f montage=%s"),
		ElementNames[static_cast<int32>(ElementLoadout)], LowestFoot, LongestLeg, PelvisTip,
		Pelvis.Z - FloorZ, static_cast<int32>(GetCharacterMovement()->MovementMode),
		GetVelocity().Size(), *MontageName);
}

void AATLACharacter::UpdateGroundedState()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// MOVE_None = planted (root stance); rooting is the deepest grounding there is
	bool bGrounded = GetCharacterMovement()->IsMovingOnGround() ||
		GetCharacterMovement()->MovementMode == MOVE_None;
	if (bGrounded)
	{
		// v1 rule: any walkable surface counts as earth except water pools and walls
		const AActor* Floor = GetCharacterMovement()->CurrentFloor.HitResult.GetActor();
		if (Floor && (Floor->IsA<AATLAWaterSource>() || Floor->IsA<AATLAIceWall>() || Floor->IsA<AATLAEarthWall>()))
		{
			bGrounded = false;
		}
	}

	AbilitySystemComponent->SetLooseGameplayTagCount(ATLATags::State_Earth_Grounded, bGrounded ? 1 : 0);

	// Near a pool, water is effectively limitless — the bender pulls straight
	// from the source. The carried reserve only matters away from water.
	bNearWaterSource = AATLAWaterSource::FindSourceInRange(GetWorld(), GetActorLocation()) != nullptr;
	if (HasAuthority() && bNearWaterSource)
	{
		if (const UATLAAttributeSet* AttrSet = AbilitySystemComponent->GetSet<UATLAAttributeSet>())
		{
			AbilitySystemComponent->SetNumericAttributeBase(UATLAAttributeSet::GetWaterAttribute(), AttrSet->GetMaxWater());
		}
	}

	// Earth trickles back only while grounded (server-authoritative)
	if (HasAuthority())
	{
		if (bGrounded && !EarthRegenHandle.IsValid())
		{
			const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(UGE_EarthRegen::StaticClass(), 1.f, AbilitySystemComponent->MakeEffectContext());
			if (Spec.IsValid())
			{
				EarthRegenHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
		else if (!bGrounded && EarthRegenHandle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(EarthRegenHandle);
			EarthRegenHandle.Invalidate();
		}
	}
}

void AATLACharacter::GrantElementKit()
{
	if (!AbilitySystemComponent || !HasAuthority())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : ElementKitHandles)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}
	ElementKitHandles.Reset();

	TArray<TSubclassOf<UGameplayAbility>> Kit;
	switch (ElementLoadout)
	{
	case EATLAElement::Water:
		Kit = { UATLAAbility_WaterWhip::StaticClass(), UATLAAbility_DrawWater::StaticClass(),
		        UATLAAbility_IceSpears::StaticClass(), UATLAAbility_IceWall::StaticClass(),
		        UATLAAbility_Octopus::StaticClass() };
		break;
	case EATLAElement::Earth:
		Kit = { UATLAAbility_RockJab::StaticClass(), UATLAAbility_Boulder::StaticClass(),
		        UATLAAbility_EarthSpikes::StaticClass(), UATLAAbility_EarthWall::StaticClass(),
		        UATLAAbility_EarthArmor::StaticClass(), UATLAAbility_EarthLaunch::StaticClass(),
		        UATLAAbility_BoulderHoist::StaticClass() };
		break;
	case EATLAElement::Fire:
		Kit = { UATLAAbility_FireJab::StaticClass(), UATLAAbility_FireBlast::StaticClass(),
		        UATLAAbility_FireStream::StaticClass(), UATLAAbility_FireLash::StaticClass(),
		        UATLAAbility_FireWall::StaticClass(), UATLAAbility_FireNova::StaticClass(),
		        UATLAAbility_FireJet::StaticClass(), UATLAAbility_FireBreath::StaticClass(),
		        UATLAAbility_Lightning::StaticClass() };
		break;
	case EATLAElement::Air:
		Kit = { UATLAAbility_AirBlast::StaticClass(), UATLAAbility_AirSwipe::StaticClass(),
		        UATLAAbility_WindDome::StaticClass(), UATLAAbility_AirCyclone::StaticClass(),
		        UATLAAbility_AirScooter::StaticClass(), UATLAAbility_Updraft::StaticClass() };
		break;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Kit)
	{
		ElementKitHandles.Add(AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this)));
	}

	// Element movement passives: airbenders are lighter than everyone else —
	// higher leaps plus a mid-air double jump off a cushion of air
	if (ElementLoadout == EATLAElement::Air)
	{
		GetCharacterMovement()->JumpZVelocity = 860.f;
		GetCharacterMovement()->AirControl = 0.9f;
		JumpMaxCount = 2;
	}
	else
	{
		GetCharacterMovement()->JumpZVelocity = 560.f;
		GetCharacterMovement()->AirControl = 0.5f;
		JumpMaxCount = 1;
	}

	SpawnAura();
}

void AATLACharacter::SpawnAura()
{
	if (Aura)
	{
		Aura->Destroy();
		Aura = nullptr;
	}
	if (!GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Aura = GetWorld()->SpawnActorDeferred<AATLAElementAura>(AATLAElementAura::StaticClass(), GetActorTransform(), this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Aura)
	{
		Aura->Element = ElementLoadout;
		Aura->FinishSpawning(GetActorTransform());
	}
}

FVector AATLACharacter::GetZoneAnchor(EATLAElement Element)
{
	switch (Element)
	{
	case EATLAElement::Water: return FVector(0.f, 0.f, 320.f);
	case EATLAElement::Earth: return FVector(6000.f, 0.f, 260.f);
	case EATLAElement::Fire:  return FVector(0.f, 6000.f, 260.f);
	case EATLAElement::Air:   return FVector(6000.f, 6000.f, 260.f);
	}
	return FVector::ZeroVector;
}

void AATLACharacter::SetElementLoadout(EATLAElement NewElement)
{
	if (ElementLoadout == NewElement)
	{
		return;
	}
	ElementLoadout = NewElement;
	InnerDrive = 0.f;      // the inner fire is stoked in battle, not carried between elements
	SyncDriveTags();
	GrantElementKit();

	// Travel to the element's home ground
	SetActorLocation(GetZoneAnchor(ElementLoadout), false, nullptr, ETeleportType::TeleportPhysics);

	if (GEngine && IsLocallyControlled())
	{
		static const TCHAR* Names[] = { TEXT("WATER"), TEXT("EARTH"), TEXT("FIRE"), TEXT("AIR") };
		GEngine->AddOnScreenDebugMessage(3, 2.f, FColor::Green,
			FString::Printf(TEXT("Element: %s"), Names[static_cast<int32>(ElementLoadout)]));
	}
}

void AATLACharacter::SwitchElement()
{
	SetElementLoadout(static_cast<EATLAElement>((static_cast<int32>(ElementLoadout) + 1) % 4));
}

void AATLACharacter::EnterBendingStance()
{
	// Strafe-style combat movement: the character rotates smoothly toward the
	// camera aim (RotationRate-limited) instead of facing the run direction
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetWorldTimerManager().SetTimer(StanceTimer, this, &AATLACharacter::ExitBendingStance, 1.6f, false);
}

void AATLACharacter::ExitBendingStance()
{
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

UAbilitySystemComponent* AATLACharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AATLACharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitAbilitySystem();

	// Grant abilities on the server only; GAS replicates the specs to the owning client
	if (HasAuthority())
	{
		// Shared kit (element-agnostic) plus any Blueprint extras
		TArray<TSubclassOf<UGameplayAbility>> ToGrant = DefaultAbilities;
		ToGrant.AddUnique(UATLAAbility_Dodge::StaticClass());

		for (const TSubclassOf<UGameplayAbility>& AbilityClass : ToGrant)
		{
			if (AbilityClass && !AbilitySystemComponent->FindAbilitySpecFromClass(AbilityClass))
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
			}
		}

		// The element-specific kit (re-granted on element switch)
		GrantElementKit();

		// Passive chi regeneration for every bender
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		const FGameplayEffectSpecHandle RegenSpec = AbilitySystemComponent->MakeOutgoingSpec(UGE_ChiRegen::StaticClass(), 1.f, Context);
		if (RegenSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*RegenSpec.Data.Get());
		}
	}
}

void AATLACharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilitySystem();
}

void AATLACharacter::InitAbilitySystem()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

bool AATLACharacter::ActivateAbilitiesByTag(FGameplayTagContainer AbilityTags)
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags);
	}
	return false;
}

float AATLACharacter::GetChi() const
{
	return Attributes ? Attributes->GetChi() : 0.f;
}

float AATLACharacter::GetWater() const
{
	return Attributes ? Attributes->GetWater() : 0.f;
}

float AATLACharacter::GetEarth() const
{
	return Attributes ? Attributes->GetEarth() : 0.f;
}

float AATLACharacter::GetHealth() const
{
	return Attributes ? Attributes->GetHealth() : 0.f;
}

void AATLACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AATLACharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AATLACharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AATLACharacter::Look);

		// Bending. Fall back to loading the default action directly so the binding
		// never silently vanishes if the Blueprint loses its property override.
		if (!BendPrimaryAction)
		{
			BendPrimaryAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_BendPrimary.IA_BendPrimary"));
		}
		if (BendPrimaryAction)
		{
			// Water: Triggered = hold-to-autofire. Earth: press/release = tap
			// jab vs hold-charged boulder. Handlers no-op for the other element.
			EnhancedInputComponent->BindAction(BendPrimaryAction, ETriggerEvent::Triggered, this, &AATLACharacter::BendPrimary);
			EnhancedInputComponent->BindAction(BendPrimaryAction, ETriggerEvent::Started, this, &AATLACharacter::BendPrimaryPressed);
			EnhancedInputComponent->BindAction(BendPrimaryAction, ETriggerEvent::Completed, this, &AATLACharacter::BendPrimaryReleased);
		}
		else
		{
			UE_LOG(LogATLA, Warning, TEXT("BendPrimaryAction not set and IA_BendPrimary not found; bending input disabled"));
		}

		if (!BendDrawAction)
		{
			BendDrawAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_BendDraw.IA_BendDraw"));
		}
		if (BendDrawAction)
		{
			EnhancedInputComponent->BindAction(BendDrawAction, ETriggerEvent::Started, this, &AATLACharacter::BendDrawStart);
			EnhancedInputComponent->BindAction(BendDrawAction, ETriggerEvent::Completed, this, &AATLACharacter::BendDrawStop);
		}
		else
		{
			UE_LOG(LogATLA, Warning, TEXT("BendDrawAction not set and IA_BendDraw not found; draw input disabled"));
		}

		if (!BendIceSpearsAction)
		{
			BendIceSpearsAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_IceSpears.IA_IceSpears"));
		}
		if (BendIceSpearsAction)
		{
			EnhancedInputComponent->BindAction(BendIceSpearsAction, ETriggerEvent::Started, this, &AATLACharacter::BendSecondary);
		}

		if (!BendIceWallAction)
		{
			BendIceWallAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_IceWall.IA_IceWall"));
		}
		if (BendIceWallAction)
		{
			EnhancedInputComponent->BindAction(BendIceWallAction, ETriggerEvent::Started, this, &AATLACharacter::BendStructure);
		}

		if (!BendOctopusAction)
		{
			BendOctopusAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_Octopus.IA_Octopus"));
		}
		if (BendOctopusAction)
		{
			EnhancedInputComponent->BindAction(BendOctopusAction, ETriggerEvent::Started, this, &AATLACharacter::BendSignature);
		}

		if (!DodgeAction)
		{
			DodgeAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_Dodge.IA_Dodge"));
		}
		if (DodgeAction)
		{
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AATLACharacter::Dodge);
		}

		if (!EarthLaunchAction)
		{
			EarthLaunchAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_EarthLaunch.IA_EarthLaunch"));
		}
		if (EarthLaunchAction)
		{
			EnhancedInputComponent->BindAction(EarthLaunchAction, ETriggerEvent::Started, this, &AATLACharacter::EarthLaunch);
			EnhancedInputComponent->BindAction(EarthLaunchAction, ETriggerEvent::Completed, this, &AATLACharacter::EarthLaunchReleased);
		}

		if (!SwitchElementAction)
		{
			SwitchElementAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_SwitchElement.IA_SwitchElement"));
		}
		if (SwitchElementAction)
		{
			EnhancedInputComponent->BindAction(SwitchElementAction, ETriggerEvent::Started, this, &AATLACharacter::SwitchElement);
		}
	}
	else
	{
		UE_LOG(LogATLA, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AATLACharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AATLACharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AATLACharacter::TryBend(const FGameplayTag& AbilityTag, float WaterCost)
{
	const bool bActivated = ActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));

	// Refusal hints (successful casts enter the bending stance from the ability itself)
	if (!bActivated && GEngine && IsLocallyControlled() && Attributes)
	{
		if (ElementLoadout == EATLAElement::Earth && AbilitySystemComponent && !AbilitySystemComponent->HasMatchingGameplayTag(ATLATags::State_Earth_Grounded))
		{
			GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Orange, TEXT("Earthbending needs solid ground beneath you"));
		}
		else if (ElementLoadout == EATLAElement::Earth && Attributes->GetEarth() < WaterCost)
		{
			GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Orange,
				FString::Printf(TEXT("Not enough earth (need %.0f) — stand on bendable ground to recharge"), WaterCost));
		}
		else if (ElementLoadout == EATLAElement::Water && Attributes->GetWater() < WaterCost)
		{
			GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Orange,
				FString::Printf(TEXT("Not enough water (need %.0f) — hold Right Mouse near a pool to draw"), WaterCost));
		}
		else if ((ElementLoadout == EATLAElement::Fire || ElementLoadout == EATLAElement::Air) && Attributes->GetChi() < WaterCost)
		{
			GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Orange,
				ElementLoadout == EATLAElement::Fire
					? TEXT("Not enough chi — tap Right Mouse: recovery breath")
					: TEXT("Not enough chi — it recovers on its own; ease off"));
		}
	}
}

void AATLACharacter::BendPrimary()
{
	// This fires EVERY FRAME while the button is held. Without pacing, each
	// frame starts another cast montage on top of the last; the overlapping
	// blends shred the pose (legs collapsing into the body). Earthbending
	// never showed it because it casts once, on release.
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAutoFireTime < AutoFireInterval)
	{
		return;
	}

	// Hold-to-autofire elements; earth and fire use tap/hold press-release
	if (ElementLoadout == EATLAElement::Water)
	{
		LastAutoFireTime = Now;
		TryBend(ATLATags::Ability_Water_Whip, 1.f);
	}
	else if (ElementLoadout == EATLAElement::Air)
	{
		LastAutoFireTime = Now;
		TryBend(ATLATags::Ability_Air_Blast, 5.f);
	}
}

void AATLACharacter::BendPrimaryPressed()
{
	if (ElementLoadout == EATLAElement::Earth)
	{
		PrimaryPressTime = GetWorld()->GetTimeSeconds();
		bPrimaryHeld = true;
		// If the hold commits (not just a tap), show the boulder wind-up
		GetWorldTimerManager().SetTimer(ChargeAnimTimer, this, &AATLACharacter::StartChargeAnim, 0.22f, false);
	}
	else if (ElementLoadout == EATLAElement::Fire)
	{
		bPrimaryHeld = true;
		// The shot leaves on the press — no release-wait. (While Empowered the
		// jab itself upgrades its bolt to a blast; the punch anim stays.)
		TryBend(ATLATags::Ability_Fire_Jab, 4.f);
		// Keep holding and the breath of fire ignites
		GetWorldTimerManager().SetTimer(FireStreamTimer, this, &AATLACharacter::StartFireStream, 0.4f, false);
	}
}

void AATLACharacter::StartFireStream()
{
	if (bPrimaryHeld && ElementLoadout == EATLAElement::Fire)
	{
		TryBend(ATLATags::Ability_Fire_Stream, 5.f);
	}
}

void AATLACharacter::StartChargeAnim()
{
	if (!bPrimaryHeld || ElementLoadout != EATLAElement::Earth)
	{
		return;
	}
	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* Charge = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Bending/Anims/AM_HeavyCast_Upper.AM_HeavyCast_Upper"));
	if (Anim && Charge && !Anim->Montage_IsPlaying(Charge))
	{
		// Slow-motion wind-up: the heave releases when the boulder fires
		Anim->Montage_Play(Charge, 0.45f);
	}

	// The charge itself is visible: a growing orb of the element at the hand
	if (!ChargeGlow)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ChargeGlow = GetWorld()->SpawnActor<AATLAChargeGlow>(AATLAChargeGlow::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
		if (ChargeGlow)
		{
			ChargeGlow->AttachToHand(this, ATLATags::Element_Earth);
		}
	}

	// Earth's charge also visibly gathers ground into the hand
	if (ElementLoadout == EATLAElement::Earth)
	{
		AATLADrawStream::SpawnForCast(this, ATLATags::Element_Earth, 1.f);
	}
}

void AATLACharacter::BendPrimaryReleased()
{
	if (ElementLoadout == EATLAElement::Fire)
	{
		// The shot already left on the press; lifting the button just closes the breath
		bPrimaryHeld = false;
		GetWorldTimerManager().ClearTimer(FireStreamTimer);
		if (AbilitySystemComponent)
		{
			FGameplayTagContainer StreamTag(ATLATags::Ability_Fire_Stream);
			AbilitySystemComponent->CancelAbilities(&StreamTag);
		}
		return;
	}

	if (ElementLoadout != EATLAElement::Earth)
	{
		return;
	}
	bPrimaryHeld = false;
	GetWorldTimerManager().ClearTimer(ChargeAnimTimer);
	if (ChargeGlow)
	{
		ChargeGlow->Destroy();
		ChargeGlow = nullptr;
	}

	const float Held = GetWorld()->GetTimeSeconds() - PrimaryPressTime;
	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* Charge = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Bending/Anims/AM_HeavyCast_Upper.AM_HeavyCast_Upper"));

	if (Held >= 0.6f)
	{
		// The boulder continues the already-playing wind-up at full speed
		if (Anim && Charge && Anim->Montage_IsPlaying(Charge))
		{
			Anim->Montage_SetPlayRate(Charge, 1.1f);
		}
		TryBend(ATLATags::Ability_Earth_Boulder, 18.f);
	}
	else
	{
		// Just a tap: drop any wind-up that started and jab instead
		if (Anim && Charge && Anim->Montage_IsPlaying(Charge))
		{
			Anim->Montage_Stop(0.12f, Charge);
		}
		TryBend(ATLATags::Ability_Earth_Jab, 2.f);
	}
}

void AATLACharacter::BendSecondary()
{
	switch (ElementLoadout)
	{
	case EATLAElement::Water: TryBend(ATLATags::Ability_Water_IceSpears, 10.f); break;
	case EATLAElement::Earth: TryBend(ATLATags::Ability_Earth_Spikes, 12.f); break;
	case EATLAElement::Fire:  TryBend(ATLATags::Ability_Fire_Lash, 12.f); break;
	case EATLAElement::Air:   TryBend(ATLATags::Ability_Air_Swipe, 10.f); break;
	}
}

void AATLACharacter::BendStructure()
{
	switch (ElementLoadout)
	{
	case EATLAElement::Water: TryBend(ATLATags::Ability_Water_IceWall, 25.f); break;
	case EATLAElement::Earth: TryBend(ATLATags::Ability_Earth_Wall, 5.f); break;
	case EATLAElement::Fire:  TryBend(ATLATags::Ability_Fire_Wall, 25.f); break;
	case EATLAElement::Air:   TryBend(ATLATags::Ability_Air_Shield, 20.f); break;
	}
}

void AATLACharacter::BendSignature()
{
	switch (ElementLoadout)
	{
	case EATLAElement::Water: TryBend(ATLATags::Ability_Water_Octopus, 50.f); break;
	case EATLAElement::Earth: TryBend(ATLATags::Ability_Earth_Armor, 50.f); break;
	case EATLAElement::Fire:  TryBend(ATLATags::Ability_Fire_Nova, 45.f); break;
	case EATLAElement::Air:   TryBend(ATLATags::Ability_Air_Cyclone, 40.f); break;
	}
}

void AATLACharacter::EarthLaunch()
{
	switch (ElementLoadout)
	{
	case EATLAElement::Earth: TryBend(ATLATags::Ability_Earth_Launch, 5.f); break;
	case EATLAElement::Fire:  TryBend(ATLATags::Ability_Fire_Jet, 8.f); break;
	case EATLAElement::Air:   TryBend(ATLATags::Ability_Air_Scooter, 20.f); break;
	default: break;
	}
}

void AATLACharacter::EarthLaunchReleased()
{
	// The jet burns only while held
	if (ElementLoadout == EATLAElement::Fire && AbilitySystemComponent)
	{
		FGameplayTagContainer JetTag(ATLATags::Ability_Fire_Jet);
		AbilitySystemComponent->CancelAbilities(&JetTag);
	}
}

void AATLACharacter::Dodge()
{
	ActivateAbilitiesByTag(FGameplayTagContainer(ATLATags::Ability_Movement_Dodge));
}

void AATLACharacter::BendDrawStart()
{
	switch (ElementLoadout)
	{
	case EATLAElement::Water: ActivateAbilitiesByTag(FGameplayTagContainer(ATLATags::Ability_Water_Pull)); break;
	case EATLAElement::Earth: TryBend(ATLATags::Ability_Earth_Hoist, 30.f); break;
	case EATLAElement::Fire:
		// Tap-vs-hold: tap toggles the breath channel, holding charges lightning
		RMBPressTime = GetWorld()->GetTimeSeconds();
		bRMBHeld = true;
		GetWorldTimerManager().SetTimer(LightningGlowTimer, this, &AATLACharacter::StartLightningCharge, 0.35f, false);
		break;
	case EATLAElement::Air:   ActivateAbilitiesByTag(FGameplayTagContainer(ATLATags::Ability_Air_Updraft)); break;
	}
}

void AATLACharacter::StartLightningCharge()
{
	if (!bRMBHeld || ElementLoadout != EATLAElement::Fire)
	{
		return;
	}

	// Azula plants herself: the charge crackles at the fingertips, walking slows
	if (!LightningGlow)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		LightningGlow = GetWorld()->SpawnActor<AATLAChargeGlow>(AATLAChargeGlow::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
		if (LightningGlow)
		{
			LightningGlow->AttachToHand(this, ATLATags::Ability_Fire_Lightning);
		}
	}
	if (!bLightningSlowed)
	{
		GetCharacterMovement()->MaxWalkSpeed *= 0.45f;
		bLightningSlowed = true;
	}

	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* Charge = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Bending/Anims/AM_HeavyCast_Upper.AM_HeavyCast_Upper"));
	if (Anim && Charge && !Anim->Montage_IsPlaying(Charge))
	{
		Anim->Montage_Play(Charge, 0.35f);
	}
}

void AATLACharacter::BendDrawStop()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (ElementLoadout == EATLAElement::Fire)
	{
		const float Held = GetWorld()->GetTimeSeconds() - RMBPressTime;
		bRMBHeld = false;
		GetWorldTimerManager().ClearTimer(LightningGlowTimer);
		if (LightningGlow)
		{
			LightningGlow->Destroy();
			LightningGlow = nullptr;
		}
		if (bLightningSlowed)
		{
			GetCharacterMovement()->MaxWalkSpeed /= 0.45f;
			bLightningSlowed = false;
		}
		if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			if (UAnimMontage* Charge = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Bending/Anims/AM_HeavyCast_Upper.AM_HeavyCast_Upper")))
			{
				if (Anim->Montage_IsPlaying(Charge))
				{
					Anim->Montage_Stop(0.15f, Charge);
				}
			}
		}

		if (Held >= 1.2f)
		{
			// The charge is complete: release the bolt
			TryBend(ATLATags::Ability_Fire_Lightning, 35.f);
		}
		else if (Held < 0.5f)
		{
			// A tap toggles the breath channel on and off
			if (IsAbilityActive(ATLATags::Ability_Fire_Breath))
			{
				FGameplayTagContainer BreathTag(ATLATags::Ability_Fire_Breath);
				AbilitySystemComponent->CancelAbilities(&BreathTag);
			}
			else
			{
				TryBend(ATLATags::Ability_Fire_Breath, 1.f);
			}
		}
		// Between 0.5s and 1.2s: an aborted charge — the lightning fizzles, no cost
		return;
	}

	FGameplayTagContainer ChannelTags;
	ChannelTags.AddTag(ATLATags::Ability_Water_Pull);
	ChannelTags.AddTag(ATLATags::Ability_Earth_Root);
	ChannelTags.AddTag(ATLATags::Ability_Earth_Hoist);
	ChannelTags.AddTag(ATLATags::Ability_Air_Updraft);
	AbilitySystemComponent->CancelAbilities(&ChannelTags);
}

bool AATLACharacter::IsAbilityActive(const FGameplayTag& AbilityTag) const
{
	if (!AbilitySystemComponent)
	{
		return false;
	}
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->GetAssetTags().HasTag(AbilityTag))
		{
			return true;
		}
	}
	return false;
}

void AATLACharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AATLACharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AATLACharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AATLACharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}


// ---- Inner Drive (fire) ----

void AATLACharacter::StokeInnerDrive(float Amount)
{
	if (ElementLoadout != EATLAElement::Fire)
	{
		return;
	}
	InnerDrive = FMath::Clamp(InnerDrive + Amount, 0.f, 1.f);
	LastStokeTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	SyncDriveTags();
}

void AATLACharacter::StokeEarthMastery(float Amount)
{
	if (bMetalUnlocked)
	{
		return;
	}
	EarthMastery = FMath::Clamp(EarthMastery + Amount, 0.f, 1.f);
	if (EarthMastery >= 1.f)
	{
		bMetalUnlocked = true;
		if (GEngine && IsPlayerControlled())
		{
			GEngine->AddOnScreenDebugMessage(8, 6.f, FColor::Silver,
				TEXT("METALBENDING UNLOCKED - your jabs now tear metal, not rock."));
		}
	}
}

void AATLACharacter::NoteDamageTaken()
{
	LastDamagedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	// Pain feeds the fire too — the show's firebending runs on adversity
	StokeInnerDrive(0.12f);
}

void AATLACharacter::SyncDriveTags()
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->SetLooseGameplayTagCount(ATLATags::State_Fire_Stoked, InnerDrive >= 0.5f ? 1 : 0);
	AbilitySystemComponent->SetLooseGameplayTagCount(ATLATags::State_Fire_Blazing, InnerDrive >= 0.85f ? 1 : 0);
}

void AATLACharacter::DecayInnerDrive()
{
	// The inner fire cools when the fight does: hold for 2.5s, then fade
	if (InnerDrive > 0.f && GetWorld() &&
		GetWorld()->GetTimeSeconds() - LastStokeTime > 2.5f)
	{
		InnerDrive = FMath::Max(InnerDrive - 0.08f * 0.1f, 0.f);
		SyncDriveTags();
	}
}
