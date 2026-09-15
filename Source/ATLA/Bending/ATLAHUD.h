// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ATLAHUD.generated.h"

/**
 * Minimal canvas HUD: health bar and carried-water bar.
 * Placeholder until a proper UMG interface exists.
 */
UCLASS()
class ATLA_API AATLAHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& FillColor, const FString& Label);
};
