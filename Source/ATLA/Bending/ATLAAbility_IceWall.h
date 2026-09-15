// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLACastAbility.h"
#include "ATLAAbility_IceWall.generated.h"

class AATLAIceWall;

/**
 * Ice Wall: raises a wall of ice from the ground in front of the bender.
 * Physically blocks projectiles and movement for 8 seconds.
 * Costs 25 water, 8 second cooldown.
 */
UCLASS()
class ATLA_API UATLAAbility_IceWall : public UATLACastAbility
{
	GENERATED_BODY()

public:
	UATLAAbility_IceWall();

protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	TSubclassOf<AATLAIceWall> WallClass;

	/** How far in front of the bender the wall rises, in cm */
	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	float WallDistance = 320.f;
};
