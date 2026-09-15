// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ATLAStructureDamageable.generated.h"

UINTERFACE(MinimalAPI)
class UATLAStructureDamageable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Structures (ice/earth walls) have no ability system, so gameplay-effect
 * damage can't touch them. Projectiles call this instead on impact.
 */
class ATLA_API IATLAStructureDamageable
{
	GENERATED_BODY()

public:
	virtual void ApplyStructureDamage(float Amount) = 0;
};
