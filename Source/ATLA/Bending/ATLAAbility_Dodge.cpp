// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAbility_Dodge.h"
#include "ATLAGameplayTags.h"
#include "ATLAGameplayEffects.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UATLAAbility_Dodge::UATLAAbility_Dodge()
{
	SetAssetTags(FGameplayTagContainer(ATLATags::Ability_Movement_Dodge));

	CostGameplayEffectClass = UGE_Cost_Dodge::StaticClass();
	CooldownGameplayEffectClass = UGE_Cooldown_Dodge::StaticClass();

	RollMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Variant_Platforming/Anims/AM_Dash.AM_Dash")));

	// Real cartwheel (CMU 87_05) — the acrobatic bender roll
	RollSequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Anims/CMU_Manny/MNY_87_05.MNY_87_05")));
	RollSequenceStart = 0.5f;
}

void UATLAAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Dodge along the held movement input; with no input, hop backward
	FVector Direction = Character->GetCharacterMovement()->GetLastInputVector().GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		Direction = -Character->GetActorForwardVector().GetSafeNormal2D();
	}

	// A second dodge inside the chain window becomes a roll
	const float Now = Character->GetWorld()->GetTimeSeconds();
	bWasRoll = (Now - LastDodgeEndTime) <= ChainWindow;

	const float Speed = bWasRoll ? RollSpeed : DodgeSpeed;
	Character->LaunchCharacter(Direction * Speed + FVector(0.f, 0.f, bWasRoll ? 220.f : 140.f), true, true);

	if (bWasRoll)
	{
		// Real acrobatic clip preferred (full-body dynamic window); the
		// platforming dash montage stays as the fallback
		UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
		UAnimSequenceBase* Sequence = RollSequence.LoadSynchronous();
		if (Anim && Sequence)
		{
			Anim->PlaySlotAnimationAsDynamicMontage(Sequence, TEXT("DefaultSlot"), 0.1f, 0.2f, 1.25f, 1, -1.f, RollSequenceStart);
		}
		else if (UAnimMontage* Montage = RollMontage.LoadSynchronous())
		{
			UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, Montage, 1.15f, NAME_None, /*bStopWhenAbilityEnds*/ false);
			MontageTask->ReadyForActivation();
		}
	}

	Character->GetWorldTimerManager().SetTimer(FinishTimer, FTimerDelegate::CreateUObject(this, &UATLAAbility_Dodge::FinishDodge), bWasRoll ? 0.55f : 0.3f, false);
}

void UATLAAbility_Dodge::FinishDodge()
{
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		// Rolls don't chain into further rolls; only a fresh dodge restarts the window
		LastDodgeEndTime = bWasRoll ? -10.f : Avatar->GetWorld()->GetTimeSeconds();
	}
	if (IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UATLAAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Avatar->GetWorldTimerManager().ClearTimer(FinishTimer);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
