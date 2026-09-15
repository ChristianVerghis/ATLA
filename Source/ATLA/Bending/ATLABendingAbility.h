// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ATLABendingAbility.generated.h"

/**
 * Base class for all bending abilities.
 * Subclass in Blueprint per technique (water whip, rock throw, ...);
 * the element tag drives combo pairing between players.
 */
UCLASS(Abstract)
class ATLA_API UATLABendingAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UATLABendingAbility();

	/** Which element this technique belongs to (Element.Water, Element.Fire, ...) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bending")
	FGameplayTag ElementTag;

	/** If true, holding this ability opens a combo window other players can join */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bending|Combo")
	bool bOpensComboWindow = false;

	/** Signature forms call this on cast: 10s of State.Bending.Empowered —
	 *  all damage dealt +50% and regular attacks grow. Server-side. */
	void ApplyEmpowered();

	/** Max distance to a partner for a combined technique, in cm */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bending|Combo", meta = (EditCondition = "bOpensComboWindow"))
	float ComboPartnerRange = 1500.f;
};
