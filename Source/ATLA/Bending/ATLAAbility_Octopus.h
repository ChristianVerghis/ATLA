// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLACastAbility.h"
#include "ATLAAbility_Octopus.generated.h"

class AATLAOctopusForm;

/**
 * Octopus Form: the bender surrounds themself with eight rotating water
 * tendrils that lash nearby enemies for 12 seconds.
 * Costs 50 water, 25 second cooldown.
 */
UCLASS()
class ATLA_API UATLAAbility_Octopus : public UATLACastAbility
{
	GENERATED_BODY()

public:
	UATLAAbility_Octopus();

protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Bending")
	TSubclassOf<AATLAOctopusForm> FormClass;
};
