// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAEarthAbilities.h"
#include "ATLACharacter.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "ATLARockProjectile.h"
#include "ATLAEarthSpikeLine.h"
#include "ATLAEarthWall.h"
#include "ATLAEarthArmor.h"
#include "ATLAEarthColumn.h"
#include "ATLAAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* StrikesUpperPath = TEXT("/Game/Bending/Anims/AM_WaterStrikes_Upper.AM_WaterStrikes_Upper");
	const TCHAR* HeavyUpperPath = TEXT("/Game/Bending/Anims/AM_HeavyCast_Upper.AM_HeavyCast_Upper");

	FVector ChestSpawnLocation(const ACharacter* Character)
	{
		return Character->GetActorLocation() + Character->GetControlRotation().Vector() * 80.f + FVector(0.f, 0.f, 40.f);
	}
}

// ---- Rock Jab ----

UATLAAbility_RockJab::UATLAAbility_RockJab()
{
	ElementTag = ATLATags::Element_Earth;
	ProjectileClass = AATLARockProjectile::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Earth_Jab));
	// The jab works airborne too — the rock still comes from the ground below

	CostGameplayEffectClass = UGE_EarthCost_Jab::StaticClass();
	CooldownGameplayEffectClass = UGE_EarthCooldown_Jab::StaticClass();

	// Heavier rhythm than the whip: slower swing, longer beat between taps
	MontageRate = 0.85f;
	CastDelay = 0.27f;
	CastDuration = 0.85f;
	MovementScaleDuringCast = 0.6f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(StrikesUpperPath));

	// Uppercut into a straight punch (measured: hand rises 299cm/s at 1.05,
	// extends forward by 1.25). The rock comes up on the uppercut and is sent
	// on the punch — the show's earthbending shape. Played FAST (1.15x) so the
	// punch snaps instead of pushing; window start = 1.05 - 0.27x1.15.
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Fist_Fight_B_Anim_mixamo_com.MXQ_MX_Fist_Fight_B_Anim_mixamo_com")));
	SequenceStartTimes = { 0.74f };
	MontageRate = 1.15f;
	PunchBeatDelay = 0.20f;   // source-time gap between uppercut (1.05) and punch (1.25)
	CastDuration = 0.7f;

}

FName UATLAAbility_RockJab::GetMontageStartSection() const
{
	return (CastCount % 2 == 0) ? FName(TEXT("Melee01")) : FName(TEXT("Melee02"));
}

void UATLAAbility_RockJab::OnCast()
{
	// Beat one — the uppercut. The rising fist tears a rock out of the ground
	// in front of the bender, where it hangs until the punch sends it. This is
	// the show's earthbending shape: raise it, then strike it.
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && ProjectileClass)
	{
		const FRotator Facing(0.f, Character->GetControlRotation().Yaw, 0.f);
		FVector Ground = Character->GetActorLocation() + Facing.Vector() * 130.f
			- FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.8f);
		// Airborne: earth still comes FROM THE EARTH — trace to the real ground
		// below the aim point and tear the rock out of it
		if (!Character->GetCharacterMovement()->IsMovingOnGround())
		{
			FHitResult Floor;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Character);
			if (Character->GetWorld()->LineTraceSingleByChannel(Floor, Ground + FVector(0.f, 0.f, 100.f),
				Ground - FVector(0.f, 0.f, 6000.f), ECC_Visibility, Params))
			{
				Ground = Floor.ImpactPoint + FVector(0.f, 0.f, 30.f);
			}
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AATLARockProjectile* Rock = Character->GetWorld()->SpawnActor<AATLARockProjectile>(
			ProjectileClass, Ground, Facing, SpawnParams))
		{
			// Masters tear metal, not rock (Toph's discovery)
			if (const AATLACharacter* Bender = Cast<AATLACharacter>(Character); Bender && Bender->HasMetalbending())
			{
				Rock->MakeMetal();
			}
			Rock->HoldInPlace();
			HeldRock = Rock;
		}

		// The rock is loosed on the punch, a fixed beat later in the form
		Character->GetWorldTimerManager().SetTimer(PunchTimer,
			FTimerDelegate::CreateUObject(this, &UATLAAbility_RockJab::PunchHeldRock),
			FMath::Max(PunchBeatDelay / FMath::Max(MontageRate, 0.01f), 0.05f), false);
	}
	CastCount++;
}

void UATLAAbility_RockJab::PunchHeldRock()
{
	// Beat two — the punch. Whatever the uppercut raised now flies at the
	// crosshair. If the rock died first (lifespan, interruption) just skip it.
	if (!HeldRock.IsValid())
	{
		return;
	}
	const FVector From = HeldRock->GetActorLocation();
	HeldRock->LaunchToward(GetCrosshairAimRotation(From));
	HeldRock.Reset();

	// Weight commitment: the bender steps into the punch when planted, the way
	// Hung Gar drives from the stance — the strike reads firm, not waved
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (Character->GetVelocity().SizeSquared2D() < FMath::Square(80.f) &&
			Character->GetCharacterMovement()->IsMovingOnGround())
		{
			const FVector Fwd = FRotator(0.f, Character->GetControlRotation().Yaw, 0.f).Vector();
			Character->LaunchCharacter(Fwd * 200.f, false, false);
		}
	}
}

// ---- Boulder ----

UATLAAbility_Boulder::UATLAAbility_Boulder()
{
	ElementTag = ATLATags::Element_Earth;
	ProjectileClass = AATLABoulderProjectile::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Earth_Boulder));
	ActivationRequiredTags.AddTag(ATLATags::State_Earth_Grounded);

	CostGameplayEffectClass = UGE_EarthCost_Boulder::StaticClass();
	CooldownGameplayEffectClass = UGE_EarthCooldown_Boulder::StaticClass();

	// The hold IS the charge; this is just the release heave
	MontageRate = 0.8f;
	CastDelay = 0.5f;
	CastDuration = 1.2f;
	MovementScaleDuringCast = 0.0f;
	CastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack")));
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(HeavyUpperPath));
}

void UATLAAbility_Boulder::PlayCastMontage(UAnimMontage* Montage)
{
	// The character's hold already plays the wind-up; if it's mid-swing,
	// let it run rather than restarting with a blend pop
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* Anim = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (Anim && Anim->Montage_IsPlaying(Montage))
	{
		return;
	}
	Super::PlayCastMontage(Montage);
}

void UATLAAbility_Boulder::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && ProjectileClass)
	{
		const FVector SpawnLocation = ChestSpawnLocation(Character) + FVector(0.f, 0.f, 30.f);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character->GetWorld()->SpawnActor<AATLABoulderProjectile>(ProjectileClass, SpawnLocation, GetCrosshairAimRotation(SpawnLocation), SpawnParams);
	}
}

// ---- Earth Spikes ----

UATLAAbility_EarthSpikes::UATLAAbility_EarthSpikes()
{
	ElementTag = ATLATags::Element_Earth;
	SpikeLineClass = AATLAEarthSpikeLine::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Earth_Spikes));
	ActivationRequiredTags.AddTag(ATLATags::State_Earth_Grounded);

	CostGameplayEffectClass = UGE_EarthCost_Spikes::StaticClass();
	CooldownGameplayEffectClass = UGE_EarthCooldown_Spikes::StaticClass();

	// A stomp-and-thrust: the ground does the attacking
	MontageRate = 0.95f;
	CastDelay = 0.4f;
	CastDuration = 1.0f;
	MovementScaleDuringCast = 0.0f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(StrikesUpperPath));

	// Martelo (windowed as a stomp): the knee drives straight up — foot z80
	// at 0.6 with almost no horizontal drift — then ONE hard slam into the
	// ground (vz -282, landing 0.95). Not steps: raise, slam, planted.
	// Spikes erupt on the slam: 0.30 + 0.65 x 1.0 = 0.95 in source time.
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_martelo_2.MXQ_MX_martelo_2")));
	SequenceStartTimes = { 0.30f };
	MontageRate = 1.00f;
	CastDelay = 0.65f;
	CastDuration = 1.0f;

}

void UATLAAbility_EarthSpikes::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority() || !SpikeLineClass)
	{
		return;
	}

	// The line starts just ahead of the bender and runs along the aim yaw
	const FRotator LineDir(0.f, Character->GetControlRotation().Yaw, 0.f);
	const FVector Start = Character->GetActorLocation() + LineDir.Vector() * 180.f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Character->GetWorld()->SpawnActor<AATLAEarthSpikeLine>(SpikeLineClass, Start, LineDir, SpawnParams);
}

// ---- Earth Wall (raise / launch) ----

UATLAAbility_EarthWall::UATLAAbility_EarthWall()
{
	ElementTag = ATLATags::Element_Earth;
	WallClass = AATLAEarthWall::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Earth_Wall));
	ActivationRequiredTags.AddTag(ATLATags::State_Earth_Grounded);

	CostGameplayEffectClass = UGE_EarthCost_Wall::StaticClass();
	CooldownGameplayEffectClass = UGE_EarthCooldown_Wall::StaticClass();

	MontageRate = 1.f;
	CastDelay = 0.55f;
	CastDuration = 1.3f;
	MovementScaleDuringCast = 0.0f;
	CastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack")));
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(HeavyUpperPath));
	// Rising double-palm lift (CMU 144_01 @5.5 — pose-sheet verified): both
	// hands heave up together as the wall tears out of the ground
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_2H_Cast_Spell_01.MXQ_MX_Standing_2H_Cast_Spell_01")));
	SequenceStartTimes = { 0.90f };
	MontageRate = 1.00f;
}

void UATLAAbility_EarthWall::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Two forms, one key: raising the wall is the double-palm heave; hurling
	// it is a driving hook punch (fist peak 515cm/s @0.40, extension @0.45).
	// The clip must be chosen BEFORE Super:: plays the montage.
	if (WantsLaunch())
	{
		UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Hook_Anim_mixamo_com.MXQ_MX_Hook_Anim_mixamo_com")));
		SequenceStartTimes = { 0.15f };
		MontageRate = 1.20f;
		CastDelay = 0.25f;
		CastDuration = 0.6f;
	}
	else
	{
		UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_2H_Cast_Spell_01.MXQ_MX_Standing_2H_Cast_Spell_01")));
		SequenceStartTimes = { 0.90f };
		MontageRate = 1.00f;
		CastDelay = 0.55f;
		CastDuration = 1.3f;
	}
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

bool UATLAAbility_EarthWall::WantsLaunch() const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !ActiveWall.IsValid())
	{
		return false;
	}
	const FVector ToWall = ActiveWall->GetActorLocation() - Character->GetActorLocation();
	if (ToWall.Size() > 1500.f)
	{
		return false;
	}
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(
		ToWall.GetSafeNormal2D(), FRotator(0.f, Character->GetControlRotation().Yaw, 0.f).Vector())));
	return Angle <= 35.f;
}

bool UATLAAbility_EarthWall::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// Shoving your standing wall is exempt from the raise cooldown —
	// raise-then-push is the intended combo
	if (WantsLaunch())
	{
		return true;
	}
	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

UGameplayEffect* UATLAAbility_EarthWall::GetCostGameplayEffect() const
{
	// Launching your standing wall is cheap; raising a new one is not
	if (WantsLaunch())
	{
		return UGE_EarthCost_WallLaunch::StaticClass()->GetDefaultObject<UGameplayEffect>();
	}
	return Super::GetCostGameplayEffect();
}

void UATLAAbility_EarthWall::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority() || !WallClass)
	{
		return;
	}

	if (WantsLaunch())
	{
		// Shove the wall away from the bender, along the ground
		const FVector Away = ActiveWall->GetActorLocation() - Character->GetActorLocation();
		ActiveWall->Push(Away.GetSafeNormal2D().Rotation());
		ActiveWall = nullptr;
		return;
	}

	const float Yaw = Character->GetControlRotation().Yaw;
	const FRotator WallRotation(0.f, Yaw, 0.f);
	const float FootZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector SpawnLocation = Character->GetActorLocation() + WallRotation.Vector() * WallDistance;
	SpawnLocation.Z = FootZ;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveWall = Character->GetWorld()->SpawnActor<AATLAEarthWall>(WallClass, SpawnLocation, WallRotation, SpawnParams);
}

// ---- Earth Armor ----

UATLAAbility_EarthArmor::UATLAAbility_EarthArmor()
{
	ElementTag = ATLATags::Element_Earth;
	ArmorClass = AATLAEarthArmor::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Earth_Armor));
	ActivationRequiredTags.AddTag(ATLATags::State_Earth_Grounded);

	CostGameplayEffectClass = UGE_EarthCost_Armor::StaticClass();
	CooldownGameplayEffectClass = UGE_EarthCooldown_Armor::StaticClass();

	MontageRate = 0.7f;
	CastDelay = 0.6f;
	CastDuration = 1.4f;
	MovementScaleDuringCast = 0.0f;
	CastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack")));
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(HeavyUpperPath));
	// Down-touch-rise, folding FORWARD not sinking: hands sweep overhead and
	// slam to ground level (hands z32, pelvis 93->53 at 1.3-1.4 - a deep
	// forward fold onto the palms), grip the ground, then the clip itself
	// rises back to standing by 1.9. The cocoon closes during the grip and
	// bursts as the bender comes up armored.
	// Palm-plant: 0.85 + 0.56 x 0.90 = 1.35 in source time.
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_2H_Magic_Area_Attack_01.MXQ_MX_Standing_2H_Magic_Area_Attack_01")));
	SequenceStartTimes = { 0.85f };
	MontageRate = 0.90f;
	CastDelay = 0.56f;
	CastDuration = 1.30f;   // the clip's own rise carries the recovery
}

void UATLAAbility_EarthArmor::OnCast()
{
	ApplyEmpowered();  // the signature form supercharges regular attacks for 10s
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority() || !ArmorClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Character->GetWorld()->SpawnActor<AATLAEarthArmor>(ArmorClass, Character->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

	// The palm-plant answers: rock surges up and encases the whole crouched
	// body, holds while the armor takes, and bursts away as the bender rises
	Character->GetWorld()->SpawnActor<AATLAEarthCocoon>(AATLAEarthCocoon::StaticClass(),
		Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.9f),
		FRotator(0.f, Character->GetActorRotation().Yaw, 0.f), SpawnParams);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_ArmorBuff::StaticClass(), 1.f, ASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

// ---- Earth Launch ----

UATLAAbility_EarthLaunch::UATLAAbility_EarthLaunch()
{
	ElementTag = ATLATags::Element_Earth;
	ColumnClass = AATLAEarthColumn::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Earth_Launch));
	ActivationRequiredTags.AddTag(ATLATags::State_Earth_Grounded);

	CostGameplayEffectClass = UGE_EarthCost_Launch::StaticClass();
	CooldownGameplayEffectClass = UGE_EarthCooldown_Launch::StaticClass();

	// Instant physical launch; the column is the animation
	CastMontage = nullptr;
	UpperBodyCastMontage = nullptr;
	CastDelay = 0.08f;
}

void UATLAAbility_EarthLaunch::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	if (Character->HasAuthority() && ColumnClass)
	{
		const float FootZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character->GetWorld()->SpawnActor<AATLAEarthColumn>(ColumnClass, FVector(Character->GetActorLocation().X, Character->GetActorLocation().Y, FootZ), FRotator::ZeroRotator, SpawnParams);
	}

	const FVector Forward = FRotator(0.f, Character->GetControlRotation().Yaw, 0.f).Vector();
	Character->LaunchCharacter(Forward * 450.f + FVector(0.f, 0.f, 1200.f), true, true);
}

// ---- Boulder Hoist ----

UATLAAbility_BoulderHoist::UATLAAbility_BoulderHoist()
{
	ElementTag = ATLATags::Element_Earth;
	BoulderClass = AATLAHoistedBoulder::StaticClass();
	ThrownClass = AATLABoulderProjectile::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Earth_Hoist));

	CostGameplayEffectClass = UGE_EarthCost_Hoist::StaticClass();
	CooldownGameplayEffectClass = UGE_EarthCooldown_Hoist::StaticClass();
}

void UATLAAbility_BoulderHoist::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Braced: rooted against knockback, slowed to a heave-walk
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(ATLATags::State_Earth_Rooted);
	}
	SavedSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeed = SavedSpeed * 0.4f;

	// The wind-up heave plays for the whole hoist
	if (UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
	{
		if (UAnimMontage* Heave = LoadObject<UAnimMontage>(nullptr, HeavyUpperPath))
		{
			Anim->Montage_Play(Heave, 0.4f);
		}
	}

	if (Character->HasAuthority() && BoulderClass)
	{
		// Tear the boulder out of the ground ahead of the bender
		const FVector Probe = Character->GetActorLocation() + Character->GetActorForwardVector() * 300.f;
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);
		FVector GroundPoint = Probe - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		if (Character->GetWorld()->LineTraceSingleByChannel(Hit, Probe + FVector(0.f, 0.f, 80.f), Probe - FVector(0.f, 0.f, 500.f), ECC_Visibility, Params))
		{
			GroundPoint = Hit.ImpactPoint;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Hoisted = Character->GetWorld()->SpawnActor<AATLAHoistedBoulder>(BoulderClass, GroundPoint, FRotator(0.f, Character->GetActorRotation().Yaw, 0.f), SpawnParams);
	}

	// A boulder is heavy: hover too long and it flies on its own
	Character->GetWorldTimerManager().SetTimer(AutoThrowTimer, FTimerDelegate::CreateUObject(this, &UATLAAbility_BoulderHoist::AutoThrow), 4.f, false);
}

void UATLAAbility_BoulderHoist::AutoThrow()
{
	if (IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UATLAAbility_BoulderHoist::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (Character)
	{
		Character->GetWorldTimerManager().ClearTimer(AutoThrowTimer);
		if (SavedSpeed > 0.f)
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = SavedSpeed;
			SavedSpeed = 0.f;
		}
		if (UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (UAnimMontage* Heave = LoadObject<UAnimMontage>(nullptr, HeavyUpperPath))
			{
				if (Anim->Montage_IsPlaying(Heave))
				{
					// Blend straight out — leaving the slow heave playing locked
					// the bender in the pose after the throw
					Anim->Montage_Stop(0.2f, Heave);
				}
			}
		}
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(ATLATags::State_Earth_Rooted);
	}

	// Releasing the hold IS the throw (both manual release and auto-throw land here)
	if (Hoisted)
	{
		if (Character && Character->HasAuthority() && Hoisted->IsRisen() && ThrownClass)
		{
			const FVector From = Hoisted->GetRockLocation();

			// Hurl at whatever the crosshair looks at
			FVector CamLoc;
			FRotator CamRot;
			if (const AController* Controller = Character->GetController())
			{
				Controller->GetPlayerViewPoint(CamLoc, CamRot);
			}
			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Character);
			Params.AddIgnoredActor(Hoisted);
			const FVector TraceEnd = CamLoc + CamRot.Vector() * 10000.f;
			const bool bHit = Character->GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_Visibility, Params);
			const FRotator Aim = ((bHit ? Hit.ImpactPoint : TraceEnd) - From).Rotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Character;
			SpawnParams.Instigator = Character;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AATLABoulderProjectile* Thrown = Character->GetWorld()->SpawnActor<AATLABoulderProjectile>(ThrownClass, From, Aim, SpawnParams))
			{
				Thrown->SetActorScale3D(FVector(1.7f));  // this one was HOISTED
			}
		}
		Hoisted->Destroy();  // released too early: the ground swallows it back
		Hoisted = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---- Root Stance ----

UATLAAbility_RootStance::UATLAAbility_RootStance()
{
	ElementTag = ATLATags::Element_Earth;
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Earth_Root));
	ActivationRequiredTags.AddTag(ATLATags::State_Earth_Grounded);
}

void UATLAAbility_RootStance::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Plant: no movement while channeling
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->GetCharacterMovement()->DisableMovement();

	if (Character->HasAuthority())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_RootBonus::StaticClass(), 1.f, ASC->MakeEffectContext());
			if (Spec.IsValid())
			{
				RootEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	Character->GetWorldTimerManager().SetTimer(FullCheckTimer, FTimerDelegate::CreateUObject(this, &UATLAAbility_RootStance::CheckFull), 0.25f, true);
}

void UATLAAbility_RootStance::CheckFull()
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UATLAAttributeSet* Attributes = ASC ? ASC->GetSet<UATLAAttributeSet>() : nullptr;
	if (Attributes && Attributes->GetEarth() >= Attributes->GetMaxEarth() && IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UATLAAbility_RootStance::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->GetWorldTimerManager().ClearTimer(FullCheckTimer);
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	if (RootEffectHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(RootEffectHandle);
		}
		RootEffectHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
