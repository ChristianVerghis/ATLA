// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLACastAbility.h"
#include "ATLACharacter.h"
#include "ATLADrawStream.h"
#include "ATLAWaterProjectile.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

UATLACastAbility::UATLACastAbility()
{
	// Borrowed melee swing until we have real bending animations
	CastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Anims/AM_ComboAttack.AM_ComboAttack")));
}

void UATLACastAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Every cast puts the bender in the aim-facing combat stance
	if (AATLACharacter* Bender = Cast<AATLACharacter>(Avatar))
	{
		Bender->EnterBendingStance();

		// The element is visibly drawn from its source into the casting hand
		// for the whole wind-up (water from pools, earth from the ground, air
		// from the ring around the body; fire self-generates — no draw)
		AATLADrawStream::SpawnForCast(Bender, ElementTag, CastDelay + 0.25f);
	}

	// The ability stays active through the cast so the montage keeps playing
	// (GAS stops an ability's montage when it ends). Preference order:
	// real clip windows (dynamic montage) > upper-body montage > full-body.
	bPlayedMontage = false;
	bCastFired = false;
	bUpperBodyMontageActive = false;

	if (UAnimSequenceBase* Sequence = UpperBodyCastSequence.LoadSynchronous())
	{
		const ACharacter* Character = Cast<ACharacter>(Avatar);
		// If another form left the mesh in single-node mode, take it back first —
		// montages can't play on the single-node instance
		if (Character && Character->GetMesh() &&
			Character->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationSingleNode)
		{
			Character->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		}
		UAnimInstance* Anim = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
		if (Anim)
		{
			const float Start = SequenceStartTimes.Num() > 0
				? SequenceStartTimes[SequenceCastCount % SequenceStartTimes.Num()] : 0.f;
			SequenceCastCount++;
			// Planted: the full body performs the technique (kicks can't read
			// on an arms-only layer). Moving: layer over locomotion instead.
			// Fast blend-in — with the old 0.2s blend on sub-half-second casts
			// the pose never reached full weight and every strike looked like
			// the generic stance.
			// How the clip reaches the body:
			//  - LMB strikes (bUpperBodyWhileMoving): ALWAYS the UpperBody slot.
			//    Legs stay locomotion-driven (running or idle) — the foot-IK
			//    control rig in ABP_Unarmed pins feet to the floor, so letting
			//    a clip drive the legs through the montage path just drags the
			//    thighs inward over glued feet (the "weird inwards" glitch).
			//  - Planted forms (MovementScale == 0): raw single-node playback
			//    on the mesh. Full body, faithful, and it BYPASSES the foot-IK
			//    rig entirely — stomps lift, kicks kick, all-fours drops. The
			//    anim blueprint is restored when the form ends.
			//  - Semi-mobile forms: full-body montage (arm-led clips; the
			//    pinned feet read as a braced stance).
			if (bUpperBodyWhileMoving)
			{
				DynamicCastMontage = Anim->PlaySlotAnimationAsDynamicMontage(
					Sequence, TEXT("UpperBody"), 0.15f, 0.35f, MontageRate, 1, -1.f, Start);
				if (DynamicCastMontage)
				{
					bPlayedMontage = true;
					bUpperBodyMontageActive = true;
				}
			}
			else if (MovementScaleDuringCast <= KINDA_SMALL_NUMBER)
			{
				USkeletalMeshComponent* Mesh = Character->GetMesh();
				Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				Mesh->PlayAnimation(Sequence, false);
				if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance())
				{
					Single->SetPosition(Start, false);
					Single->SetPlayRate(MontageRate);
				}
				bSingleNodePlayback = true;
				bPlayedMontage = true;
				bUpperBodyMontageActive = false;
			}
			else
			{
				DynamicCastMontage = Anim->PlaySlotAnimationAsDynamicMontage(
					Sequence, TEXT("DefaultSlot"), 0.15f, 0.35f, MontageRate, 1, -1.f, Start);
				if (DynamicCastMontage)
				{
					bPlayedMontage = true;
					bUpperBodyMontageActive = false;  // full body owns the form
				}
			}
		}
	}

	if (!bPlayedMontage)
	{
		UAnimMontage* Montage = UpperBodyCastMontage.LoadSynchronous();
		if (Montage)
		{
			bUpperBodyMontageActive = true;
		}
		else
		{
			Montage = CastMontage.LoadSynchronous();
		}

		if (Montage)
		{
			ActiveCastMontage = Montage;
			PlayCastMontage(Montage);
			bPlayedMontage = true;
		}
	}

	// The form owns the body while it plays: heavy techniques root the bender
	// until the animation finishes (mobility abilities leave the scale at 1).
	// Speed is scaled MULTIPLICATIVELY and undone by the inverse — never
	// captured and restored absolutely, or overlapping modifiers (the jet's
	// 2.6x boost, breath's 0.55x) get clobbered and the bender keeps a
	// permanently wrong walk speed.
	if (MovementScaleDuringCast < 1.f && !bMovementScaled && !bMovementRooted)
	{
		if (ACharacter* Character = Cast<ACharacter>(Avatar))
		{
			if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
			{
				if (MovementScaleDuringCast <= KINDA_SMALL_NUMBER)
				{
					// Fully rooted: planted on the spot, no speed maths at all
					if (Move->IsMovingOnGround())
					{
						Move->StopMovementImmediately();
						Move->SetMovementMode(MOVE_None);
						bMovementRooted = true;
					}
				}
				else
				{
					Move->MaxWalkSpeed *= MovementScaleDuringCast;
					bMovementScaled = true;
				}
			}
		}
	}

	if (bPlayedMontage)
	{
		Avatar->GetWorldTimerManager().SetTimer(FinishTimer, FTimerDelegate::CreateUObject(this, &UATLACastAbility::FinishCast), CastDuration, false);
	}

	Avatar->GetWorldTimerManager().SetTimer(CastTimer, FTimerDelegate::CreateUObject(this, &UATLACastAbility::HandleCastMoment), CastDelay, false);
}

void UATLACastAbility::PlayCastMontage(UAnimMontage* Montage)
{
	// A full-body montage would freeze the legs mid-run, so skip it while
	// sprinting. Upper-body variants layer over locomotion and always play.
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		if (!bUpperBodyMontageActive && Avatar->GetVelocity().SizeSquared2D() > FMath::Square(80.f))
		{
			return;
		}
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, MontageRate, GetMontageStartSection());
	MontageTask->OnCompleted.AddDynamic(this, &UATLACastAbility::FinishCast);
	MontageTask->OnInterrupted.AddDynamic(this, &UATLACastAbility::FinishCast);
	MontageTask->OnCancelled.AddDynamic(this, &UATLACastAbility::FinishCast);
	MontageTask->ReadyForActivation();
}

FRotator UATLACastAbility::GetCrosshairAimRotation(const FVector& SpawnLocation) const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return FRotator::ZeroRotator;
	}

	const AController* Controller = Character->GetController();
	if (!Controller)
	{
		return Character->GetActorRotation();
	}

	// Trace from the camera through the screen-center dot; aim the projectile
	// from its spawn point at whatever the trace finds, so shots land exactly
	// under the crosshair. Pawn-aware sphere sweep — pawn capsules IGNORE
	// ECC_Visibility, so a channel trace phases through characters and aims
	// at the ground far behind them (off-hand spawns then never converge).
	FVector CamLoc;
	FRotator CamRot;
	Controller->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector TraceEnd = CamLoc + CamRot.Vector() * 10000.f;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// SweepMulti + skip penetrating starts: when the camera sits inside level
	// geometry (arena pillars), the single-sweep "hit" is the camera itself
	// and every shot would aim backward at the bender.
	TArray<FHitResult> Hits;
	Character->GetWorld()->SweepMultiByObjectType(Hits, CamLoc, TraceEnd, FQuat::Identity,
		ObjParams, FCollisionShape::MakeSphere(30.f), Params);
	FVector AimPoint = TraceEnd;
	for (const FHitResult& Hit : Hits)
	{
		if (Hit.bStartPenetrating)
		{
			continue;
		}
		// The bender's own projectiles in flight sit exactly on the crosshair
		// line; aiming at them makes follow-up shots feed back into the stream
		// and scatter. Skip them — aim at what's beyond.
		const AActor* HitActor = Hit.GetActor();
		if (HitActor && Cast<AATLAWaterProjectile>(HitActor) && HitActor->GetInstigator() == Character)
		{
			continue;
		}
		AimPoint = Hit.ImpactPoint;
		break;
	}

	return (AimPoint - SpawnLocation).Rotation();
}

void UATLACastAbility::HandleCastMoment()
{
	bCastFired = true;
	OnCast();

	// Without a montage there is no recovery to wait for
	if (!bPlayedMontage)
	{
		FinishCast();
	}
}

void UATLACastAbility::FinishCast()
{
	if (IsActive())
	{
		// The technique must fire exactly once even if the montage ends early
		// (cold-loaded animations can finish instantly on first play) — the
		// committed cost has been paid, so deliver the effect before ending.
		if (!bCastFired)
		{
			bCastFired = true;
			OnCast();
		}
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UATLACastAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Avatar->GetWorldTimerManager().ClearTimer(CastTimer);
		Avatar->GetWorldTimerManager().ClearTimer(FinishTimer);

		// Hand the body back to the player — inverse of whatever was applied
		if (bMovementScaled || bMovementRooted)
		{
			if (const ACharacter* Character = Cast<ACharacter>(Avatar))
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					if (bMovementScaled && MovementScaleDuringCast > KINDA_SMALL_NUMBER)
					{
						Move->MaxWalkSpeed /= MovementScaleDuringCast;
					}
					if (bMovementRooted && Move->MovementMode == MOVE_None)
					{
						Move->SetMovementMode(MOVE_Walking);
					}
				}
			}
			bMovementScaled = false;
			bMovementRooted = false;
		}

		// Single-node forms hand the mesh back to the anim blueprint. Restoring
		// alone hard-cuts to the locomotion pose (the "leg snap" at the end of a
		// stomp) — so the same clip is re-played as a blended montage from the
		// exact frame the single node froze on and immediately blended out: the
		// pose carries over and eases back into locomotion.
		if (bSingleNodePlayback)
		{
			if (const ACharacter* Character = Cast<ACharacter>(Avatar))
			{
				if (USkeletalMeshComponent* Mesh = Character->GetMesh())
				{
					UAnimSequenceBase* Sequence = nullptr;
					float Pos = 0.f;
					if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance())
					{
						Sequence = Cast<UAnimSequenceBase>(Single->GetCurrentAsset());
						Pos = Single->GetCurrentTime();
					}
					Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
					if (Sequence)
					{
						if (UAnimInstance* Anim = Mesh->GetAnimInstance())
						{
							if (UAnimMontage* Bridge = Anim->PlaySlotAnimationAsDynamicMontage(
								Sequence, TEXT("DefaultSlot"), 0.f, 0.35f, MontageRate, 1, -1.f, Pos))
							{
								Anim->Montage_Stop(0.35f, Bridge);
							}
						}
					}
				}
			}
			bSingleNodePlayback = false;
		}

		// Dynamic clip windows blend out with the cast (they aren't GAS-owned)
		if (DynamicCastMontage)
		{
			const ACharacter* Character = Cast<ACharacter>(Avatar);
			UAnimInstance* Anim = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
			if (Anim && Anim->Montage_IsPlaying(DynamicCastMontage))
			{
				Anim->Montage_Stop(0.25f, DynamicCastMontage);
			}
			DynamicCastMontage = nullptr;
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
