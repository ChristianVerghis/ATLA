// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLACastAbility.h"
#include "ATLAAbility_WaterWhip.generated.h"

class AATLAWaterProjectile;

/**
 * Water Whip: the bender sweeps an arm and launches a snaking water stream
 * where the camera is aiming. Costs 1 water, 2 second cooldown, 15 damage.
 */
UCLASS()
class ATLA_API UATLAAbility_WaterWhip : public UATLACastAbility
{
	GENERATED_BODY()

public:
	UATLAAbility_WaterWhip();

protected:
	virtual void OnCast() override;
	virtual FName GetMontageStartSection() const override;

	/** Flow chain: successive casts continue one montage across strike sections
	 *  instead of restarting it — smooth back-and-forth while spamming */
	virtual void PlayCastMontage(UAnimMontage* Montage) override;

	/** Blends the flow montage out once the player stops casting */
	void StopFlowMontage();

	/** Projectile class to spawn; defaults to the C++ placeholder, override in Blueprint for fancier visuals */
	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	TSubclassOf<AATLAWaterProjectile> ProjectileClass;

	/** Montage sections cycled between successive casts — alternating arms */
	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	TArray<FName> StrikeSections;

	/** How far off-center each throw spawns, alternating sides with the arms */
	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	float HandOffset = 22.f;

	int32 CastCount = 0;
	FTimerHandle FlowStopTimer;
};
