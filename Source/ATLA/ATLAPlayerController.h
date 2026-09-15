// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ATLAPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class ACharacter;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AATLAPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	/** B: summon/dismiss the AI sparring partner near the player (any square) */
	UFUNCTION(BlueprintCallable, Category = "Sparring")
	void ToggleSparringPartner();

	/** Console 'DebugSettleDuel': zeroes the partner's health (duel-flow test) */
	UFUNCTION(Exec)
	void DebugSettleDuel();

	/** N: travel between the coliseum and the element's home square */
	void ToggleColiseum();

	/** Arena duel: while a partner exists, first health to hit zero loses */
	void CheckDuel();

	/** Ragdoll the loser where they stand (the knockdown moment) */
	void KnockDown(ACharacter* Loser);

	/** After the knockdown pause: heal, un-ragdoll the player, clear the field */
	void FinishDuelCleanup();

	FTimerHandle DuelTimer;
	FTimerHandle DuelCleanupTimer;
	bool bDuelSettled = false;
	bool bPlayerKnockedDown = false;
	FVector SavedMeshLocation = FVector::ZeroVector;
	FRotator SavedMeshRotation = FRotator::ZeroRotator;

	TWeakObjectPtr<APawn> SparringPartner;

public:
	/** The current duel opponent, if one stands (HUD draws their meter) */
	UFUNCTION(BlueprintPure, Category = "Sparring")
	APawn* GetSparringPartner() const { return SparringPartner.Get(); }

};
