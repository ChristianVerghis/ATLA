// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAirAbilities.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "ATLAAirActors.h"
#include "ATLAAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* AirStrikesUpper = TEXT("/Game/Bending/Anims/AM_WaterStrikes_Upper.AM_WaterStrikes_Upper");
	const TCHAR* AirHeavyUpper = TEXT("/Game/Bending/Anims/AM_HeavyCast_Upper.AM_HeavyCast_Upper");

	FVector AirChestSpawn(const ACharacter* Character)
	{
		return Character->GetActorLocation() + Character->GetControlRotation().Vector() * 80.f + FVector(0.f, 0.f, 40.f);
	}
}

// ---- Air Blast ----

UATLAAbility_AirBlast::UATLAAbility_AirBlast()
{
	ElementTag = ATLATags::Element_Air;
	BoltClass = AATLAAirBlastBolt::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Air_Blast));

	CostGameplayEffectClass = UGE_AirCost_Blast::StaticClass();
	CooldownGameplayEffectClass = UGE_AirCooldown_Blast::StaticClass();

	// Light, circular, quick — Baguazhang palms
	MontageRate = 2.f;
	CastDelay = 0.16f;
	CastDuration = 0.45f;
	MovementScaleDuringCast = 0.7f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(AirStrikesUpper));

	// Quick one-handed push (hand peak 611cm/s @0.60) — the airbender's
	// palm strike, fast enough for hold-to-autofire
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_1H_Magic_Attack_02.MXQ_MX_Standing_1H_Magic_Attack_02")));
	SequenceStartTimes = { 0.38f };
	bUpperBodyWhileMoving = true;   // legs keep running mid-strike; planted = full form
	MontageRate = 1.35f;

}

FName UATLAAbility_AirBlast::GetMontageStartSection() const
{
	return (CastCount % 2 == 0) ? FName(TEXT("Melee02")) : FName(TEXT("Melee01"));
}

void UATLAAbility_AirBlast::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && BoltClass)
	{
		const FVector SpawnLocation = AirChestSpawn(Character);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character->GetWorld()->SpawnActor<AATLAAirBlastBolt>(BoltClass, SpawnLocation, GetCrosshairAimRotation(SpawnLocation), SpawnParams);
	}
	CastCount++;
}

// ---- Air Swipe ----

UATLAAbility_AirSwipe::UATLAAbility_AirSwipe()
{
	ElementTag = ATLATags::Element_Air;
	SwipeClass = AATLAAirSwipeBolt::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Air_Swipe));

	CostGameplayEffectClass = UGE_AirCost_Swipe::StaticClass();
	CooldownGameplayEffectClass = UGE_AirCooldown_Swipe::StaticClass();

	MontageRate = 1.6f;
	CastDelay = 0.35f;
	CastDuration = 0.95f;
	MovementScaleDuringCast = 0.0f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(AirStrikesUpper));

	// Capoeira armada spin-kick (hand 826 / foot 597 @0.90-0.70) — the
	// sweeping circular swipe, closest live style to Baguazhang
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_armada.MXQ_MX_armada")));
	SequenceStartTimes = { 0.48f };
	MontageRate = 1.20f;

}

void UATLAAbility_AirSwipe::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && SwipeClass)
	{
		const FVector SpawnLocation = AirChestSpawn(Character);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character->GetWorld()->SpawnActor<AATLAAirSwipeBolt>(SwipeClass, SpawnLocation, GetCrosshairAimRotation(SpawnLocation), SpawnParams);
	}
}

// ---- Wind Dome ----

UATLAAbility_WindDome::UATLAAbility_WindDome()
{
	ElementTag = ATLATags::Element_Air;
	DomeClass = AATLAWindDome::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Air_Shield));

	CostGameplayEffectClass = UGE_AirCost_Shield::StaticClass();
	CooldownGameplayEffectClass = UGE_AirCooldown_Shield::StaticClass();

	MontageRate = 1.5f;
	CastDelay = 0.45f;
	CastDuration = 1.1f;
	MovementScaleDuringCast = 0.0f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(AirHeavyUpper));
	// Capoeira-style fancy footwork (CMU 85_04) — spinning up the dome
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_2H_Magic_Attack_04.MXQ_MX_Standing_2H_Magic_Attack_04")));
	SequenceStartTimes = { 0.50f };
	MontageRate = 1.00f;
}

void UATLAAbility_WindDome::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && DomeClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character->GetWorld()->SpawnActor<AATLAWindDome>(DomeClass, Character->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	}
}

// ---- Cyclone ----

UATLAAbility_AirCyclone::UATLAAbility_AirCyclone()
{
	ElementTag = ATLATags::Element_Air;
	CycloneClass = AATLAAirCyclone::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Air_Cyclone));

	CostGameplayEffectClass = UGE_AirCost_Cyclone::StaticClass();
	CooldownGameplayEffectClass = UGE_AirCooldown_Cyclone::StaticClass();

	MontageRate = 1.2f;
	CastDelay = 0.7f;
	CastDuration = 1.6f;
	MovementScaleDuringCast = 0.0f;
	CastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack")));
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(AirHeavyUpper));
	// Jump twist (CMU 85_01) — summoning the cyclone
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_2H_Magic_Area_Attack_02.MXQ_MX_Standing_2H_Magic_Area_Attack_02")));
	SequenceStartTimes = { 1.10f };
	MontageRate = 1.00f;
}

void UATLAAbility_AirCyclone::OnCast()
{
	ApplyEmpowered();  // the signature form supercharges regular attacks for 10s
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority() || !CycloneClass)
	{
		return;
	}

	// Place the funnel where the crosshair looks, clamped to range, on the ground
	FVector CamLoc;
	FRotator CamRot;
	if (const AController* Controller = Character->GetController())
	{
		Controller->GetPlayerViewPoint(CamLoc, CamRot);
	}
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	FVector Target = CamLoc + CamRot.Vector() * MaxRange;
	if (Character->GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, Target, ECC_Visibility, Params))
	{
		Target = Hit.ImpactPoint;
	}

	// Aiming near your own feet (or steeply down) summons the spout under you — the ride
	const float AimPitch = FRotator::NormalizeAxis(Character->GetControlRotation().Pitch);
	if (AimPitch < -50.f ||
		FVector::DistSquared2D(Target, Character->GetActorLocation()) < FMath::Square(450.f))
	{
		Target = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetSimpleCollisionHalfHeight());
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Character->GetWorld()->SpawnActor<AATLAAirCyclone>(CycloneClass, Target, FRotator::ZeroRotator, SpawnParams);
}

// ---- Air Scooter ----

UATLAAbility_AirScooter::UATLAAbility_AirScooter()
{
	ElementTag = ATLATags::Element_Air;
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Air_Scooter));

	CostGameplayEffectClass = UGE_AirCost_Scooter::StaticClass();
	CooldownGameplayEffectClass = UGE_AirCooldown_Scooter::StaticClass();

	CastMontage = nullptr;
	UpperBodyCastMontage = nullptr;
	CastDelay = 0.05f;
}

void UATLAAbility_AirScooter::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	SavedSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeed = SavedSpeed * SpeedMultiplier;
	Character->GetCharacterMovement()->MaxAcceleration = 4800.f;

	// The ball itself: ride it show-style
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ball = Character->GetWorld()->SpawnActor<AATLAAirScooterBall>(AATLAAirScooterBall::StaticClass(), Character->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	if (Ball)
	{
		Ball->AttachToRider(Character);
	}

	Character->GetWorldTimerManager().SetTimer(ScooterTimer, FTimerDelegate::CreateUObject(this, &UATLAAbility_AirScooter::EndScooter), Duration, false);
}

void UATLAAbility_AirScooter::EndScooter()
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (SavedSpeed > 0.f)
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = SavedSpeed;
			Character->GetCharacterMovement()->MaxAcceleration = 2400.f;
		}
	}
	if (Ball)
	{
		Ball->Destroy();  // EndPlay sets the rider's mesh back down
		Ball = nullptr;
	}
}

// ---- Updraft ----

UATLAAbility_Updraft::UATLAAbility_Updraft()
{
	ElementTag = ATLATags::Element_Air;
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Air_Updraft));
}

void UATLAAbility_Updraft::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Feather fall while held
	SavedGravity = Character->GetCharacterMovement()->GravityScale;
	Character->GetCharacterMovement()->GravityScale = 0.15f;

	if (Character->HasAuthority())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_AirUpdraftDrain::StaticClass(), 1.f, ASC->MakeEffectContext());
			if (Spec.IsValid())
			{
				DrainHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	Character->GetWorldTimerManager().SetTimer(GlideTimer, FTimerDelegate::CreateUObject(this, &UATLAAbility_Updraft::GlideTick), 0.1f, true);
}

void UATLAAbility_Updraft::GlideTick()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UATLAAttributeSet* Attributes = ASC ? ASC->GetSet<UATLAAttributeSet>() : nullptr;

	// Cap fall speed to a drift
	if (Character && Character->GetCharacterMovement()->Velocity.Z < -220.f)
	{
		Character->GetCharacterMovement()->Velocity.Z = -220.f;
	}

	// Out of chi: the wind lets go
	if (Attributes && Attributes->GetChi() <= 1.f && IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UATLAAbility_Updraft::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->GetWorldTimerManager().ClearTimer(GlideTimer);
		Character->GetCharacterMovement()->GravityScale = SavedGravity > 0.f ? SavedGravity : 1.f;
	}

	if (DrainHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(DrainHandle);
		}
		DrainHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
