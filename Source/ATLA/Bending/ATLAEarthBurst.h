// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLAWaterSplash.h"
#include "ATLAEarthBurst.generated.h"

/**
 * Rock impact debris: chunks of stone scatter under heavy gravity —
 * no watery shell, no glow, just shrapnel and dust.
 */
UCLASS()
class ATLA_API AATLAEarthBurst : public AATLAWaterSplash
{
	GENERATED_BODY()

public:
	AATLAEarthBurst();

protected:
	virtual void ApplyMaterials() override;

	/** Real dirt kick-up (pack asset; debris meshes remain as backup) */
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> DirtFX;
};
