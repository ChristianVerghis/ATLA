// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLABendingAbility.h"
#include "Engine/TimerHandle.h"
#include "ATLAAbility_Dodge.generated.h"

class UAnimMontage;

/**
 * Dodge: a quick burst of movement in the direction being held (backward if
 * none). Dodging again within the chain window turns the second dodge into a
 * full dodge roll — longer, faster, with the dash animation. Costs chi.
 */
UCLASS()
class ATLA_API UATLAAbility_Dodge : public UATLABendingAbility
{
	GENERATED_BODY()

public:
	UATLAAbility_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void FinishDodge();

	/** Burst speed of a single dodge, cm/s */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeSpeed = 1100.f;

	/** Burst speed of a chained dodge roll, cm/s */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float RollSpeed = 1700.f;

	/** Second dodge within this window (seconds) becomes a roll */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float ChainWindow = 0.6f;

	/** Roll animation (the platforming dash) */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TSoftObjectPtr<UAnimMontage> RollMontage;

	/** Preferred: a real acrobatic clip window played full-body (dynamic montage) */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TSoftObjectPtr<UAnimSequenceBase> RollSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float RollSequenceStart = 0.f;

	float LastDodgeEndTime = -10.f;
	FTimerHandle FinishTimer;
	bool bWasRoll = false;
};
