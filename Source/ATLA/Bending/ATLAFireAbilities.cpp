// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAFireAbilities.h"
#include "ATLACharacter.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "ATLAFireActors.h"
#include "ATLAAttributeSet.h"
#include "ATLAStructureDamageable.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* StrikesUpper = TEXT("/Game/Bending/Anims/AM_WaterStrikes_Upper.AM_WaterStrikes_Upper");
	const TCHAR* HeavyUpper = TEXT("/Game/Bending/Anims/AM_HeavyCast_Upper.AM_HeavyCast_Upper");

	FVector ChestSpawn(const ACharacter* Character)
	{
		return Character->GetActorLocation() + Character->GetControlRotation().Vector() * 80.f + FVector(0.f, 0.f, 40.f);
	}

	// The flame leaves the striking limb: whichever of the two sockets is
	// thrust furthest along the aim at the release moment (punches fire from
	// the fist, kicks from the foot). Chest fallback if the sockets are absent.
	FVector StrikeSpawn(const ACharacter* Character, const TCHAR* SocketA, const TCHAR* SocketB)
	{
		const USkeletalMeshComponent* Mesh = Character->GetMesh();
		if (!Mesh || !Mesh->DoesSocketExist(SocketA) || !Mesh->DoesSocketExist(SocketB))
		{
			return ChestSpawn(Character);
		}
		const FVector Forward = Character->GetControlRotation().Vector();
		const FVector A = Mesh->GetSocketLocation(SocketA);
		const FVector B = Mesh->GetSocketLocation(SocketB);
		FVector Limb = FVector::DotProduct(A, Forward) >= FVector::DotProduct(B, Forward) ? A : B;
		// A planted foot sits at floor level — a bolt born there is inside the
		// ground and dies the same frame (the Q lash "did nothing"). Keep the
		// limb's XY but never spawn below hip height: the lash's 72cm hit
		// sphere needs that much floor clearance.
		Limb.Z = FMath::Max(Limb.Z, Character->GetActorLocation().Z - 10.f);
		// Born just clear of the limb so the bolt doesn't clip the body
		return Limb + Forward * 35.f;
	}
}

// ---- Fire Jab ----

UATLAAbility_FireJab::UATLAAbility_FireJab()
{
	ElementTag = ATLATags::Element_Fire;
	BoltClass = AATLAFireBolt::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Jab));

	CostGameplayEffectClass = UGE_FireCost_Jab::StaticClass();
	CooldownGameplayEffectClass = UGE_FireCooldown_Jab::StaticClass();

	// Fastest form in the game, but the punch still has to read: wind up,
	// extend, recover. Slowed from 1.7x so the pose lands instead of blurring.
	MontageRate = 2.f;
	CastDelay = 0.22f;
	CastDuration = 0.62f;
	MovementScaleDuringCast = 0.55f;  // step into the strike, don't sprint through it
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(StrikesUpper));

	// Mixamo cross punch: a real lunge + thrust (hand peak 558cm/s @1.00,
	// full extension @1.15, pose-verified). Window starts CastDelay x rate
	// before the strike so the fist extends exactly as the bolt leaves.
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Cross_Punch_Anim_mixamo_com.MXQ_MX_Cross_Punch_Anim_mixamo_com")));
	SequenceStartTimes = { 0.78f };
	bUpperBodyWhileMoving = true;   // legs keep running mid-strike; planted = full form
	MontageRate = 1.25f;

}

FName UATLAAbility_FireJab::GetMontageStartSection() const
{
	return (CastCount % 2 == 0) ? FName(TEXT("Melee01")) : FName(TEXT("Melee02"));
}

void UATLAAbility_FireJab::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	// The show's fire jab steps INTO the punch: a short forward lunge when
	// planted (skipped while already running so movement stays the player's)
	if (Character && Character->GetVelocity().SizeSquared2D() < FMath::Square(80.f) &&
		Character->GetCharacterMovement()->IsMovingOnGround())
	{
		const FVector Fwd = FRotator(0.f, Character->GetControlRotation().Yaw, 0.f).Vector();
		Character->LaunchCharacter(Fwd * 240.f, false, false);
	}

	if (Character && Character->HasAuthority() && BoltClass)
	{
		// Punches: the bolt leaves the extended fist. While Empowered the same
		// punch throws the charged blast — Nova upgrades the shot, not the form.
		const FVector SpawnLocation = StrikeSpawn(Character, TEXT("hand_r"), TEXT("hand_l"));
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		const bool bEmpowered = ASC && ASC->HasMatchingGameplayTag(ATLATags::State_Bending_Empowered);
		AATLAFireBolt* Bolt = Character->GetWorld()->SpawnActor<AATLAFireBolt>(
			bEmpowered ? TSubclassOf<AATLAFireBolt>(AATLAFireBlastBolt::StaticClass()) : TSubclassOf<AATLAFireBolt>(BoltClass),
			SpawnLocation, GetCrosshairAimRotation(SpawnLocation), SpawnParams);
		// The flame is the drive made visible: a stoked bender throws bigger fire
		if (Bolt)
		{
			if (const AATLACharacter* Bender = Cast<AATLACharacter>(Character))
			{
				Bolt->SetActorScale3D(FVector(1.f + 0.35f * Bender->GetInnerDrive()));
			}
		}
	}
	CastCount++;
}

// ---- Fire Blast ----

UATLAAbility_FireBlast::UATLAAbility_FireBlast()
{
	ElementTag = ATLATags::Element_Fire;
	BoltClass = AATLAFireBlastBolt::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Blast));

	CostGameplayEffectClass = UGE_FireCost_Blast::StaticClass();
	CooldownGameplayEffectClass = UGE_FireCooldown_Blast::StaticClass();

	MontageRate = 1.2f;
	CastDelay = 0.45f;
	CastDuration = 1.1f;
	MovementScaleDuringCast = 0.0f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(HeavyUpper));
	// Mixamo roundhouse kick (foot peak 600cm/s @0.85, pose-verified) — the
	// charged release comes off the kicking leg
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Roundhouse_Kick_Anim_mixamo_com.MXQ_MX_Roundhouse_Kick_Anim_mixamo_com")));
	SequenceStartTimes = { 0.40f };
	MontageRate = 1.00f;
}

void UATLAAbility_FireBlast::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && BoltClass)
	{
		// The yokogeri release: the fireball explodes off the kicking foot
		const FVector SpawnLocation = StrikeSpawn(Character, TEXT("foot_r"), TEXT("foot_l"));
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character->GetWorld()->SpawnActor<AATLAFireBlastBolt>(BoltClass, SpawnLocation, GetCrosshairAimRotation(SpawnLocation), SpawnParams);
	}
}

// ---- Breath of Fire (held flame stream) ----

UATLAAbility_FireStream::UATLAAbility_FireStream()
{
	ElementTag = ATLATags::Element_Fire;
	TongueClass = AATLAFireStreamBolt::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Stream));

	// Small ignition cost; the real price is the drain while held
	CostGameplayEffectClass = UGE_FireCost_Stream::StaticClass();
}

void UATLAAbility_FireStream::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bChanneling = true;

	if (AATLACharacter* Bender = Cast<AATLACharacter>(Character))
	{
		Bender->EnterBendingStance();
	}

	// One breath at a time: pouring out flame ends any recovery breath
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayTagContainer RecoveryTag(ATLATags::Ability_Fire_Breath);
		ASC->CancelAbilities(&RecoveryTag);
	}

	// Braced against the torrent: slow while the flame pours out
	Character->GetCharacterMovement()->MaxWalkSpeed *= 0.55f;

	// The flame at the lips
	if (UNiagaraSystem* Torch = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/Fire/Loop/NS_Sub_FireTorch_Loop_002.NS_Sub_FireTorch_Loop_002")))
	{
		MouthFlame = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Torch, Character->GetMesh(), TEXT("head"), FVector(0.f, 16.f, 0.f), FRotator::ZeroRotator,
			EAttachLocation::SnapToTargetIncludingScale, false);
		if (MouthFlame)
		{
			MouthFlame->SetWorldScale3D(FVector(0.3f));
		}
	}

	if (Character->HasAuthority())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_FireStreamDrain::StaticClass(), 1.f, ASC->MakeEffectContext());
			if (Spec.IsValid())
			{
				DrainHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	SpawnTongue();
	Character->GetWorldTimerManager().SetTimer(TongueTimer, FTimerDelegate::CreateUObject(this, &UATLAAbility_FireStream::SpawnTongue), 0.09f, true);
}

void UATLAAbility_FireStream::SpawnTongue()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	// The breath gutters out when the chi runs dry
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UATLAAttributeSet* Attributes = ASC ? ASC->GetSet<UATLAAttributeSet>() : nullptr;
	if (Attributes && Attributes->GetChi() < 3.f && IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
		return;
	}

	if (!Character->HasAuthority() || !TongueClass)
	{
		return;
	}

	const USkeletalMeshComponent* Mesh = Character->GetMesh();
	const FVector Mouth = (Mesh && Mesh->DoesSocketExist(TEXT("head")))
		? Mesh->GetSocketLocation(TEXT("head"))
		: ChestSpawn(Character);
	FRotator Aim = GetCrosshairAimRotation(Mouth);
	Aim.Yaw += FMath::FRandRange(-2.5f, 2.5f);
	Aim.Pitch += FMath::FRandRange(-1.8f, 1.8f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Character->GetWorld()->SpawnActor<AATLAFireStreamBolt>(TongueClass, Mouth + Aim.Vector() * 35.f, Aim, SpawnParams);
}

void UATLAAbility_FireStream::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()); Character && bChanneling)
	{
		Character->GetWorldTimerManager().ClearTimer(TongueTimer);
		Character->GetCharacterMovement()->MaxWalkSpeed /= 0.55f;
	}
	bChanneling = false;

	if (DrainHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(DrainHandle);
		}
		DrainHandle.Invalidate();
	}

	if (MouthFlame)
	{
		MouthFlame->DestroyComponent();
		MouthFlame = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---- Fire Lash ----

UATLAAbility_FireLash::UATLAAbility_FireLash()
{
	ElementTag = ATLATags::Element_Fire;
	LashClass = AATLAFireLashBolt::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Lash));

	// Same cost/cooldown slot the arc occupied
	CostGameplayEffectClass = UGE_FireCost_Arc::StaticClass();
	CooldownGameplayEffectClass = UGE_FireCooldown_Arc::StaticClass();

	// Mid-weight: a beat of wind-up before the crescent sweeps
	MontageRate = 1.6f;
	CastDelay = 0.4f;
	CastDuration = 1.0f;
	MovementScaleDuringCast = 0.0f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(StrikesUpper));

	// Low sweeping kick (foot peak 474cm/s @0.65) — a horizontal crescent.
	// A spinning heel kick reads better but yaws the body ~90 degrees off
	// the aim, which fights the crosshair; the sweep keeps facing intact.
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Leg_Sweep_Anim_mixamo_com.MXQ_MX_Leg_Sweep_Anim_mixamo_com")));
	SequenceStartTimes = { 0.05f };
	MontageRate = 1.50f;
}

void UATLAAbility_FireLash::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && LashClass)
	{
		// The roundhouse release: the crescent sweeps off the kicking foot
		const FVector SpawnLocation = StrikeSpawn(Character, TEXT("foot_r"), TEXT("foot_l"));
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character->GetWorld()->SpawnActor<AATLAFireLashBolt>(LashClass, SpawnLocation, GetCrosshairAimRotation(SpawnLocation), SpawnParams);
	}
}

// ---- Lightning ----

UATLAAbility_Lightning::UATLAAbility_Lightning()
{
	ElementTag = ATLATags::Element_Fire;
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Lightning));

	CostGameplayEffectClass = UGE_FireCost_Lightning::StaticClass();
	CooldownGameplayEffectClass = UGE_FireCooldown_Lightning::StaticClass();
}

bool UATLAAbility_Lightning::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	// "It requires peace of mind": no lightning while rattled — the bender
	// must not have been struck within the calm window
	if (const AATLACharacter* Bender = ActorInfo ? Cast<AATLACharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		const UWorld* World = Bender->GetWorld();
		if (World && World->GetTimeSeconds() - Bender->GetLastDamagedTime() < CalmWindow)
		{
			return false;
		}
	}
	return true;
}

void UATLAAbility_Lightning::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Character->HasAuthority())
	{
		// Hitscan along the crosshair — lightning does not travel, it arrives
		FVector CamLoc;
		FRotator CamRot;
		if (const AController* Controller = Character->GetController())
		{
			Controller->GetPlayerViewPoint(CamLoc, CamRot);
		}
		// A fat sweep against pawns + world, not a visibility ray — pawn
		// capsules ignore ECC_Visibility, so a channel trace phases straight
		// through characters. Near-misses at the crosshair still connect.
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);
		FCollisionObjectQueryParams ObjParams;
		ObjParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		const FVector TraceEnd = CamLoc + CamRot.Vector() * MaxRange;
		// Multi-sweep + skip penetrating starts (camera inside level geometry
		// would otherwise strike the bender's own position)
		TArray<FHitResult> Hits;
		Character->GetWorld()->SweepMultiByObjectType(Hits, CamLoc, TraceEnd, FQuat::Identity,
			ObjParams, FCollisionShape::MakeSphere(45.f), Params);
		bool bHit = false;
		for (const FHitResult& H : Hits)
		{
			if (H.bStartPenetrating)
			{
				continue;
			}
			// Don't strike the bender's own projectiles riding the crosshair line
			if (H.GetActor() && Cast<AATLAWaterProjectile>(H.GetActor()) && H.GetActor()->GetInstigator() == Character)
			{
				continue;
			}
			Hit = H;
			bHit = true;
			break;
		}
		const FVector Strike = bHit ? Hit.ImpactPoint : TraceEnd;

		if (bHit && Hit.GetActor())
		{
			if (IATLAStructureDamageable* Structure = Cast<IATLAStructureDamageable>(Hit.GetActor()))
			{
				Structure->ApplyStructureDamage(300.f);
			}
			UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hit.GetActor());
			if (SourceASC && TargetASC)
			{
				const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UGE_FireDamage_Lightning::StaticClass(), 1.f, SourceASC->MakeEffectContext());
				if (Spec.IsValid())
				{
					SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
				}
			}
		}

		// The bolt itself: fingertips to strike point, plus an impact flare
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector Fingertips = ChestSpawn(Character);
		if (AATLALightningBolt* Bolt = Character->GetWorld()->SpawnActor<AATLALightningBolt>(AATLALightningBolt::StaticClass(), Fingertips, FRotator::ZeroRotator, SpawnParams))
		{
			Bolt->SetEndpoints(Fingertips, Strike);
		}
		Character->GetWorld()->SpawnActor<AATLAFireBurst>(AATLAFireBurst::StaticClass(), Strike, FRotator::ZeroRotator, SpawnParams);

		// Electric shrapnel at the strike point
		if (UNiagaraSystem* Sparks = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/NiagaraExamples/FX_Sparks/NS_Spark_Burst.NS_Spark_Burst")))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(Character->GetWorld(), Sparks, Strike);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---- Fire Arc ----

UATLAAbility_FireArc::UATLAAbility_FireArc()
{
	ElementTag = ATLATags::Element_Fire;
	BoltClass = AATLAFireBolt::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Arc));

	CostGameplayEffectClass = UGE_FireCost_Arc::StaticClass();
	CooldownGameplayEffectClass = UGE_FireCooldown_Arc::StaticClass();

	MontageRate = 1.6f;
	CastDelay = 0.15f;
	CastDuration = 0.45f;
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(StrikesUpper));
}

void UATLAAbility_FireArc::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority() || !BoltClass)
	{
		return;
	}
	const FVector SpawnLocation = ChestSpawn(Character);
	const FRotator Aim = GetCrosshairAimRotation(SpawnLocation);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<AATLAFireBolt*> Fan;
	for (float YawOffset : { -14.f, 0.f, 14.f })
	{
		const FRotator BoltRot = Aim + FRotator(0.f, YawOffset, 0.f);
		if (AATLAFireBolt* Bolt = Character->GetWorld()->SpawnActor<AATLAFireBolt>(BoltClass, SpawnLocation + BoltRot.Vector() * 30.f, BoltRot, SpawnParams))
		{
			// Arc bolts hit lighter than the jab
			Fan.Add(Bolt);
		}
	}
	for (AATLAFireBolt* A : Fan)
	{
		for (AATLAFireBolt* B : Fan)
		{
			if (A != B)
			{
				A->IgnoreActorForMovement(B);
			}
		}
	}
}

// ---- Wall of Flame ----

UATLAAbility_FireWall::UATLAAbility_FireWall()
{
	ElementTag = ATLATags::Element_Fire;
	WallClass = AATLAFireWall::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Wall));

	CostGameplayEffectClass = UGE_FireCost_Wall::StaticClass();
	CooldownGameplayEffectClass = UGE_FireCooldown_Wall::StaticClass();

	// Heavy: the wall takes a deliberate beat to raise
	MontageRate = 1.3f;
	CastDelay = 0.7f;
	CastDuration = 1.5f;
	MovementScaleDuringCast = 0.0f;
	CastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack")));
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(HeavyUpper));
	// Two-handed overhead raise (both hands to z=197 @1.45, pose-verified) —
	// the bender heaves the wall of flame up
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_2H_Cast_Spell_01.MXQ_MX_Standing_2H_Cast_Spell_01")));
	SequenceStartTimes = { 0.75f };
	MontageRate = 1.00f;
}

void UATLAAbility_FireWall::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority() || !WallClass)
	{
		return;
	}
	const float Yaw = Character->GetControlRotation().Yaw;
	const FRotator WallRot(0.f, Yaw, 0.f);
	const float FootZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector SpawnLocation = Character->GetActorLocation() + WallRot.Vector() * 300.f;
	SpawnLocation.Z = FootZ + 130.f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Character->GetWorld()->SpawnActor<AATLAFireWall>(WallClass, SpawnLocation, WallRot, SpawnParams);
}

// ---- Inferno Nova ----

UATLAAbility_FireNova::UATLAAbility_FireNova()
{
	ElementTag = ATLATags::Element_Fire;
	NovaClass = AATLAFireNova::StaticClass();
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Nova));

	CostGameplayEffectClass = UGE_FireCost_Nova::StaticClass();
	CooldownGameplayEffectClass = UGE_FireCooldown_Nova::StaticClass();

	// The ultimate: a long, deliberate kata build into the detonation
	MontageRate = 1.1f;
	CastDelay = 0.85f;
	CastDuration = 1.8f;
	MovementScaleDuringCast = 0.0f;
	CastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack")));
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(HeavyUpper));
	// Two-handed area burst (hand peak 932cm/s @1.25) — the nova detonation
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_2H_Magic_Area_Attack_01.MXQ_MX_Standing_2H_Magic_Area_Attack_01")));
	SequenceStartTimes = { 0.40f };
	MontageRate = 1.00f;
}

void UATLAAbility_FireNova::OnCast()
{
	ApplyEmpowered();  // the signature form supercharges regular attacks for 10s
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && NovaClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character->GetWorld()->SpawnActor<AATLAFireNova>(NovaClass, Character->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	}
}

// ---- Fire Jet ----

UATLAAbility_FireJet::UATLAAbility_FireJet()
{
	ElementTag = ATLATags::Element_Fire;
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Jet));

	CostGameplayEffectClass = UGE_FireCost_Jet::StaticClass();
	CooldownGameplayEffectClass = UGE_FireCooldown_Jet::StaticClass();

	CastMontage = nullptr;
	UpperBodyCastMontage = nullptr;
	CastDelay = 0.05f;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> TorchFX(TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/Fire/Loop/NS_Sub_FireTorch_Loop_002.NS_Sub_FireTorch_Loop_002"));
	if (TorchFX.Succeeded())
	{
		ExhaustFX = TorchFX.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> StanceSeq(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_Sprint_Forward_Anim_mixamo_com.MXQ_MX_Standing_Sprint_Forward_Anim_mixamo_com"));
	if (StanceSeq.Succeeded())
	{
		GlideStance = StanceSeq.Object;
	}
}

void UATLAAbility_FireJet::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bGliding = true;

	// Azula's propulsion: the thrust actually carries the bender — flying
	// movement, held just off the floor by the hover spring, screaming forward.
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Move->SetMovementMode(MOVE_Flying);
	Move->MaxFlySpeed = 1650.f;
	Move->MaxAcceleration = 6000.f;
	Move->BrakingDecelerationFlying = 1400.f;

	// Ignition kick along the aim
	const FVector Fwd = FRotator(0.f, Character->GetControlRotation().Yaw, 0.f).Vector();
	Move->Velocity += Fwd * 700.f;

	// Hover spring: trace the floor and hold the capsule HoverHeight above it,
	// riding terrain like the show's ground glide. No floor below = fall gently.
	Character->GetWorldTimerManager().SetTimer(HoverTimer, FTimerDelegate::CreateWeakLambda(Character, [Character]()
	{
		UCharacterMovementComponent* M = Character->GetCharacterMovement();
		if (!M || M->MovementMode != MOVE_Flying)
		{
			return;
		}
		const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector Loc = Character->GetActorLocation();
		FHitResult Floor;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);
		const bool bFloor = Character->GetWorld()->LineTraceSingleByChannel(
			Floor, Loc, Loc - FVector(0.f, 0.f, HalfHeight + 400.f), ECC_Visibility, Params);
		const float TargetZ = bFloor ? (Floor.ImpactPoint.Z + HalfHeight + 45.f) : (Loc.Z - 30.f);
		// Critically-damped-ish spring on vertical velocity only
		M->Velocity.Z = FMath::FInterpTo(M->Velocity.Z, FMath::Clamp((TargetZ - Loc.Z) * 6.f, -700.f, 700.f), 0.02f, 12.f);
	}), 0.02f, true);

	// Ignition burst at the soles
	if (Character->HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.8f);
		Character->GetWorld()->SpawnActor<AATLAFireBurst>(AATLAFireBurst::StaticClass(), Feet, FRotator::ZeroRotator, SpawnParams);
	}

	// Riding the thrust: a slowed sprint stride — body pitched forward, arms
	// trailing, legs back — matches the show's ground-glide silhouette
	if (GlideStance)
	{
		if (UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			GlideMontage = Anim->PlaySlotAnimationAsDynamicMontage(
				GlideStance, TEXT("DefaultSlot"), 0.25f, 0.3f, 0.05f, 999, -1.f, 0.35f);
		}
	}

	// The whole wake from the reference: main plumes off both soles, side
	// exhausts fanning out from the feet, and hand jets trailing off the arms.
	// All anchored to the bones so they follow the stride.
	if (ExhaustFX && Character->GetMesh())
	{
		struct FPlumeDef { FName Socket; FVector Dir; float Scale; };
		const FPlumeDef Defs[] = {
			{ TEXT("foot_l"), FVector(-1.f,  0.f,  -0.4f),  1.15f },   // main thrust
			{ TEXT("foot_r"), FVector(-1.f,  0.f,  -0.4f),  1.15f },
			{ TEXT("foot_l"), FVector(-0.7f, -0.65f, -0.2f), 0.7f },   // side fans
			{ TEXT("foot_r"), FVector(-0.7f,  0.65f, -0.2f), 0.7f },
			{ TEXT("hand_l"), FVector(-1.f, -0.25f,  0.05f), 0.6f },   // arm jets
			{ TEXT("hand_r"), FVector(-1.f,  0.25f,  0.05f), 0.6f },
		};
		JetPlumeDirs.Reset();
		for (const FPlumeDef& Def : Defs)
		{
			UNiagaraComponent* Plume = UNiagaraFunctionLibrary::SpawnSystemAttached(
				ExhaustFX, Character->GetMesh(),
				Character->GetMesh()->DoesSocketExist(Def.Socket) ? Def.Socket : NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget, /*bAutoDestroy*/ false);
			if (Plume)
			{
				Plume->SetUsingAbsoluteScale(true);
				Plume->SetUsingAbsoluteRotation(true);   // aimed in bender space each tick, below
				Plume->SetWorldScale3D(FVector(Def.Scale));
				JetFlames.Add(Plume);
				JetPlumeDirs.Add(Def.Dir.GetSafeNormal());
			}
		}
		// Orient every plume each tick so its exhaust direction tracks the facing
		Character->GetWorldTimerManager().SetTimer(PlumeAimTimer, FTimerDelegate::CreateWeakLambda(Character, [Character, this]()
		{
			const FQuat Yaw = FRotator(0.f, Character->GetActorRotation().Yaw, 0.f).Quaternion();
			for (int32 i = 0; i < JetFlames.Num() && i < JetPlumeDirs.Num(); ++i)
			{
				if (JetFlames[i])
				{
					JetFlames[i]->SetWorldRotation((Yaw * FRotationMatrix::MakeFromZ(JetPlumeDirs[i]).ToQuat()).Rotator());
				}
			}
		}), 0.03f, true);
	}
}

void UATLAAbility_FireJet::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && bGliding)
	{
		Character->GetWorldTimerManager().ClearTimer(HoverTimer);
		Character->GetWorldTimerManager().ClearTimer(PlumeAimTimer);
		UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		if (Move->MovementMode == MOVE_Flying)
		{
			Move->SetMovementMode(MOVE_Falling);   // burners cut; drop the last few inches
		}
		Move->MaxAcceleration = 2400.f;
	}
	bGliding = false;

	if (Character && GlideMontage)
	{
		if (UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (Anim->Montage_IsPlaying(GlideMontage))
			{
				Anim->Montage_Stop(0.25f, GlideMontage);
			}
		}
		GlideMontage = nullptr;
	}

	for (UNiagaraComponent* Plume : JetFlames)
	{
		if (Plume)
		{
			Plume->DestroyComponent();
		}
	}
	JetFlames.Empty();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---- Breath of Fire ----

UATLAAbility_FireBreath::UATLAAbility_FireBreath()
{
	ElementTag = ATLATags::Element_Fire;
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Fire_Breath));
}

void UATLAAbility_FireBreath::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Rooted in breath: slow to a walk while channeling
	Character->GetCharacterMovement()->MaxWalkSpeed *= 0.35f;

	// The breath is visible: a small flame at the mouth
	if (UNiagaraSystem* Torch = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Fire_EXP_Vol01_Free/Niagara/Fire/Loop/NS_Sub_FireSmall_Loop_001.NS_Sub_FireSmall_Loop_001")))
	{
		BreathFlame = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Torch, Character->GetMesh(), TEXT("head"), FVector(0.f, 14.f, 0.f), FRotator::ZeroRotator,
			EAttachLocation::SnapToTargetIncludingScale, false);
		if (BreathFlame)
		{
			BreathFlame->SetWorldScale3D(FVector(0.28f));
		}
	}

	if (Character->HasAuthority())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_FireBreath::StaticClass(), 1.f, ASC->MakeEffectContext());
			if (Spec.IsValid())
			{
				BreathHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	Character->GetWorldTimerManager().SetTimer(FullCheckTimer, FTimerDelegate::CreateUObject(this, &UATLAAbility_FireBreath::CheckFull), 0.25f, true);
}

void UATLAAbility_FireBreath::CheckFull()
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UATLAAttributeSet* Attributes = ASC ? ASC->GetSet<UATLAAttributeSet>() : nullptr;
	if (Attributes && Attributes->GetChi() >= Attributes->GetMaxChi() && IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UATLAAbility_FireBreath::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->GetWorldTimerManager().ClearTimer(FullCheckTimer);
		Character->GetCharacterMovement()->MaxWalkSpeed /= 0.35f;
	}

	if (BreathHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(BreathHandle);
		}
		BreathHandle.Invalidate();
	}

	if (BreathFlame)
	{
		BreathFlame->DestroyComponent();
		BreathFlame = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
