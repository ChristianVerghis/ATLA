// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLACastAbility.h"
#include "ATLABendingAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/TimerHandle.h"
#include "ATLAAirAbilities.generated.h"

class AATLAAirBlastBolt;
class AATLAAirSwipeBolt;
class AATLAWindDome;
class AATLAAirCyclone;
class AATLAAirScooterBall;

/** Air Blast: palm-thrust push projectile. */
UCLASS()
class ATLA_API UATLAAbility_AirBlast : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_AirBlast();
protected:
	virtual void OnCast() override;
	virtual FName GetMontageStartSection() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Air")
	TSubclassOf<AATLAAirBlastBolt> BoltClass;

	int32 CastCount = 0;
};

/** Air Swipe: wide crescent, heavy knockback. */
UCLASS()
class ATLA_API UATLAAbility_AirSwipe : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_AirSwipe();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Air")
	TSubclassOf<AATLAAirSwipeBolt> SwipeClass;
};

/** Wind Dome: mobile projectile-shredding shield. */
UCLASS()
class ATLA_API UATLAAbility_WindDome : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_WindDome();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Air")
	TSubclassOf<AATLAWindDome> DomeClass;
};

/** Cyclone: a tornado at the aim point that lifts and spins victims. */
UCLASS()
class ATLA_API UATLAAbility_AirCyclone : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_AirCyclone();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Air")
	TSubclassOf<AATLAAirCyclone> CycloneClass;

	UPROPERTY(EditDefaultsOnly, Category = "Air")
	float MaxRange = 2200.f;
};

/** Air Scooter: a burst of riding speed with big air control. */
UCLASS()
class ATLA_API UATLAAbility_AirScooter : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_AirScooter();
protected:
	virtual void OnCast() override;

	UFUNCTION()
	void EndScooter();

	UPROPERTY(EditDefaultsOnly, Category = "Air")
	float SpeedMultiplier = 1.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Air")
	float Duration = 4.f;

	UPROPERTY()
	TObjectPtr<AATLAAirScooterBall> Ball;

	FTimerHandle ScooterTimer;
	float SavedSpeed = 0.f;
};

/** Updraft: hold to glide — falling slows to a feather drop while chi drains. */
UCLASS()
class ATLA_API UATLAAbility_Updraft : public UATLABendingAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_Updraft();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	void GlideTick();

	FActiveGameplayEffectHandle DrainHandle;
	FTimerHandle GlideTimer;
	float SavedGravity = 1.f;
};
