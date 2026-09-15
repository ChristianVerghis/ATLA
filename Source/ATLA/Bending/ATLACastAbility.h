// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLABendingAbility.h"
#include "Engine/TimerHandle.h"
#include "ATLACastAbility.generated.h"

class UAnimMontage;

/**
 * Base for bending techniques with a cast animation: plays a montage, fires
 * OnCast() partway through (the release moment), and ends after a recovery
 * window so the montage blends out instead of playing its full length.
 */
UCLASS(Abstract)
class ATLA_API UATLACastAbility : public UATLABendingAbility
{
	GENERATED_BODY()

public:
	UATLACastAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** The release moment: spawn projectiles/walls/forms here. Runs on client and server; guard authority-only work. */
	virtual void OnCast() {}

	/** Montage section to start from; override to vary strikes between casts */
	virtual FName GetMontageStartSection() const { return NAME_None; }

	/** Plays the cast animation. Default: GAS montage task that stops with the
	 *  ability. Override for custom montage handling (e.g. the whip's flow chain). */
	virtual void PlayCastMontage(UAnimMontage* Montage);

	/** Rotation from SpawnLocation toward whatever is under the crosshair (camera-center trace) */
	FRotator GetCrosshairAimRotation(const FVector& SpawnLocation) const;

	/** Ends the ability (recovery elapsed or montage finished) */
	UFUNCTION()
	void FinishCast();

	/** Cast animation (replicates to other players via GAS) */
	UPROPERTY(EditDefaultsOnly, Category = "Bending|Cast")
	TSoftObjectPtr<UAnimMontage> CastMontage;

	/** Optional upper-body-slot variant. When set and loadable it is preferred
	 *  and plays even while running (legs stay with locomotion — requires the
	 *  AnimBP to have an 'UpperBody' layered slot). */
	UPROPERTY(EditDefaultsOnly, Category = "Bending|Cast")
	TSoftObjectPtr<UAnimMontage> UpperBodyCastMontage;

	/** Preferred over montages once real clips exist: this raw AnimSequence
	 *  plays as a dynamic montage in the UpperBody slot — no montage asset
	 *  needed. Start times rotate per cast (windows into the source form). */
	UPROPERTY(EditDefaultsOnly, Category = "Bending|Cast")
	TSoftObjectPtr<UAnimSequenceBase> UpperBodyCastSequence;

	/** Per-cast start offsets into the sequence, cycled like strike sections */
	UPROPERTY(EditDefaultsOnly, Category = "Bending|Cast")
	TArray<float> SequenceStartTimes;

	/** The dynamic montage created for the current sequence cast */
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> DynamicCastMontage;

	int32 SequenceCastCount = 0;

	/** True while the current cast is using the upper-body montage variant */
	bool bUpperBodyMontageActive = false;

	/** The montage actually chosen for the current cast */
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveCastMontage;

	/** Seconds into the montage when OnCast fires */
	UPROPERTY(EditDefaultsOnly, Category = "Bending|Cast")
	float CastDelay = 0.22f;

	/** Total cast time before the ability ends and the montage blends out */
	UPROPERTY(EditDefaultsOnly, Category = "Bending|Cast")
	float CastDuration = 0.9f;

	/** Montage play rate — crank above 1 for snappier bending forms */
	UPROPERTY(EditDefaultsOnly, Category = "Bending|Cast")
	float MontageRate = 1.f;

	/**
	 * How much the bender can still move while performing the form, 0..1.
	 * Bending is the pose: heavy techniques root you until the form finishes
	 * (0 = planted), light ones only slow you. Mobility abilities leave it 1.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Bending|Cast", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MovementScaleDuringCast = 1.f;

	/**
	 * Mobile casts (LMB strikes): while actually moving, play the clip on the
	 * UpperBody slot so the legs keep the run cycle. Planted casts stay full-body.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Bending|Cast")
	bool bUpperBodyWhileMoving = false;

private:
	/** True while this cast is scaling movement (guards double-apply) */
	bool bMovementScaled = false;
	/** True while this cast has the bender rooted via MOVE_None */
	bool bMovementRooted = false;
	/** True while the mesh is in raw single-node playback (planted forms) */
	bool bSingleNodePlayback = false;

protected:

private:
	void HandleCastMoment();

	FTimerHandle CastTimer;
	FTimerHandle FinishTimer;
	bool bPlayedMontage = false;
	bool bCastFired = false;
};
