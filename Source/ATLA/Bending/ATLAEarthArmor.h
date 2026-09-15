// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATLAEarthArmor.generated.h"

class UStaticMeshComponent;

/**
 * Earth armor: stone plates locked to the bender's body for 12 seconds.
 * The buff itself (40% DR, grab/knockback immunity via the Armored tag)
 * is a gameplay effect; this actor is the visual plus the -15% move speed.
 */
UCLASS()
class ATLA_API AATLAEarthArmor : public AActor
{
	GENERATED_BODY()

public:
	AATLAEarthArmor();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Plates;

	UPROPERTY(EditDefaultsOnly, Category = "Armor")
	float Duration = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Armor")
	float MoveSpeedMultiplier = 0.85f;

	TArray<FName> PlateBones;
	float SavedWalkSpeed = 0.f;
};
