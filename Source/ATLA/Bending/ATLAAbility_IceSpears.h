// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLACastAbility.h"
#include "ATLAAbility_IceSpears.generated.h"

class AATLAIceSpear;

/**
 * Ice Spears: a fan of five frozen darts fired where the camera aims.
 * Costs 10 water, 4 second cooldown, 8 damage per spear.
 */
UCLASS()
class ATLA_API UATLAAbility_IceSpears : public UATLACastAbility
{
	GENERATED_BODY()

public:
	UATLAAbility_IceSpears();

protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	TSubclassOf<AATLAIceSpear> SpearClass;

	/** Number of spears in the volley */
	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	int32 SpearCount = 5;

	/** Total horizontal spread of the fan, in degrees */
	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	float FanSpread = 24.f;
};
