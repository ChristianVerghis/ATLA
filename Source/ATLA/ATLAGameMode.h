// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ATLAGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AATLAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	AATLAGameMode();

protected:
	virtual void BeginPlay() override;

	/** Spawn a bending AI opponent near the player start (testable enemy) */
	UPROPERTY(EditDefaultsOnly, Category = "Enemies")
	bool bSpawnEnemyBender = false;   // B key summons/dismisses one instead

	/** Pawn class for the enemy (defaults to the player's character BP so it has a mesh) */
	UPROPERTY(EditDefaultsOnly, Category = "Enemies")
	TSoftClassPtr<APawn> EnemyPawnClass;
};
