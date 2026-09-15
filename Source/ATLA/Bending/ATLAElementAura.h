// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLACharacter.h"
#include "ATLAElementAura.generated.h"

class UStaticMeshComponent;

/**
 * Idle ambient bending: the bender's element visibly moves with them.
 * Water: two thin streams orbit the waist. Earth: three pebbles float and
 * bob. Air: fast pale wisps circle. Fire: flames flicker at the fists —
 * generated, not gathered. Purely cosmetic; spawned per loadout.
 */
UCLASS()
class ATLA_API AATLAElementAura : public AActor
{
	GENERATED_BODY()

public:
	AATLAElementAura();

	virtual void Tick(float DeltaTime) override;

	/** Which element look this aura wears (set before/at spawn) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura")
	EATLAElement Element = EATLAElement::Water;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Bits;

	/** Real Niagara fist flames (fire element; mesh bits hide when present) */
	UPROPERTY()
	TArray<TObjectPtr<class UNiagaraComponent>> FlameFX;

	float Age = 0.f;
};
