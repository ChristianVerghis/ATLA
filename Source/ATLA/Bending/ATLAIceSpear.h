// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLAWaterProjectile.h"
#include "ATLAIceSpear.generated.h"

/**
 * A frozen dart: faster and straighter than the water whip, pale ice look.
 * Fired in fans of five by the ice spears technique.
 */
UCLASS()
class ATLA_API AATLAIceSpear : public AATLAWaterProjectile
{
	GENERATED_BODY()

public:
	AATLAIceSpear();

protected:
	virtual void ApplyMaterials() override;
};
