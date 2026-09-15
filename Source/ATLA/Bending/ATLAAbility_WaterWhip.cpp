// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAbility_WaterWhip.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "ATLAWaterProjectile.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

UATLAAbility_WaterWhip::UATLAAbility_WaterWhip()
{
	ElementTag = ATLATags::Element_Water;
	ProjectileClass = AATLAWaterProjectile::StaticClass();

	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Water_Whip));

	CostGameplayEffectClass = UGE_Cost_WaterWhip::StaticClass();
	CooldownGameplayEffectClass = UGE_Cooldown_WaterWhip::StaticClass();

	// Rapid-fire with a flowing strike chain: successive casts ping-pong one
	// montage between its two equal-length strikes (left/right arms) instead
	// of restarting it. Measured section length is 0.91s; at 1.55x each strike
	// lasts ~0.59s, always outlasting the 0.45s recast cadence, so the next
	// strike is queued before the boundary and the flow never breaks.
	MontageRate = 1.55f;
	CastDelay = 0.2f;
	CastDuration = 0.6f;
	MovementScaleDuringCast = 0.6f;
	StrikeSections = { TEXT("Melee01"), TEXT("Melee02") };

	// Preferred once the AnimBP has an UpperBody layered slot: strikes play
	// over running legs (asset generated from AM_ComboAttack with re-slotted track)
	UpperBodyCastMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Bending/Anims/AM_WaterStrikes_Upper.AM_WaterStrikes_Upper")));

	// Real Tai Chi (CMU mocap retargeted to the game skeleton): each cast
	// plays a different window of the form — waterbending's actual style.
	// Falls back to the montage flow chain if the clip is missing.
	// Flowing one-handed pushes (hand peaks 596cm/s @0.95, 546 @0.80).
	// Measured in-game at ~0-9 degrees of body yaw: Attack_01 reads similar on
	// paper but swings the torso 143 degrees off the crosshair, which fights
	// the aim — always confirm a clip's yaw in the gauntlet, not just on paper.
	UpperBodyCastSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/Mixamo/MXQ_MX_Standing_1H_Magic_Attack_03.MXQ_MX_Standing_1H_Magic_Attack_03")));
	// Firmness pass: start each strike just before its measured hand-speed peak
	// (0.95 / 0.80) so the swing opens mid-acceleration instead of in wind-up,
	// and run it hotter — same snap treatment the earth punches got.
	SequenceStartTimes = { 0.80f, 0.66f };
	MontageRate = 1.35f;
	bUpperBodyWhileMoving = true;   // legs keep running mid-strike; planted = full form
}

void UATLAAbility_WaterWhip::PlayCastMontage(UAnimMontage* Montage)
{
	// Played directly on the anim instance, NOT through GAS: an ability-owned
	// montage gets stopped the moment the ability ends (0.45s), which is what
	// made spamming look choppy. The flow montage outlives each cast.
	// TODO(multiplayer): sim proxies won't see these swings; replicate later.
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* Anim = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim)
	{
		return;
	}

	// Full-body strikes freeze the legs, so they're skipped while running.
	// Upper-body variants layer over locomotion and flow at any speed.
	if (!bUpperBodyMontageActive && Character->GetVelocity().SizeSquared2D() > FMath::Square(80.f))
	{
		if (Anim->Montage_IsPlaying(Montage))
		{
			Anim->Montage_Stop(0.2f, Montage);
		}
		return;
	}

	if (Anim->Montage_IsPlaying(Montage))
	{
		// Mid-flow: queue the next strike at the current section's boundary —
		// no restart, no blend pop
		Anim->Montage_SetNextSection(Anim->Montage_GetCurrentSection(Montage), GetMontageStartSection(), Montage);
	}
	else
	{
		Character->PlayAnimMontage(Montage, MontageRate, GetMontageStartSection());
	}

	// Each cast pushes the blend-out further away; when the player stops
	// casting, the current strike finishes and the montage fades naturally
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Avatar->GetWorldTimerManager().SetTimer(FlowStopTimer, FTimerDelegate::CreateUObject(this, &UATLAAbility_WaterWhip::StopFlowMontage), CastDuration + 0.15f, false);
	}
}

void UATLAAbility_WaterWhip::StopFlowMontage()
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* Anim = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (Anim && ActiveCastMontage && Anim->Montage_IsPlaying(ActiveCastMontage))
	{
		Anim->Montage_Stop(0.3f, ActiveCastMontage);
	}
}

FName UATLAAbility_WaterWhip::GetMontageStartSection() const
{
	return StrikeSections.Num() > 0 ? StrikeSections[CastCount % StrikeSections.Num()] : NAME_None;
}

void UATLAAbility_WaterWhip::OnCast()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && ProjectileClass)
	{
		// Alternate the throw between hands to match the alternating strikes
		const FRotator ControlRotation = Character->GetControlRotation();
		const float Side = (CastCount % 2 == 0) ? 1.f : -1.f;
		const FVector HandSide = FRotationMatrix(FRotator(0.f, ControlRotation.Yaw, 0.f)).GetUnitAxis(EAxis::Y) * HandOffset * Side;
		const FVector SpawnLocation = Character->GetActorLocation() + ControlRotation.Vector() * 80.f + HandSide + FVector(0.f, 0.f, 40.f);
		const FRotator AimRotation = GetCrosshairAimRotation(SpawnLocation);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		Character->GetWorld()->SpawnActor<AATLAWaterProjectile>(ProjectileClass, SpawnLocation, AimRotation, SpawnParams);
	}

	CastCount++;
}
