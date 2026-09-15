// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLACastAbility.h"
#include "ATLABendingAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/TimerHandle.h"
#include "ATLAEarthAbilities.generated.h"

class AATLARockProjectile;
class AATLABoulderProjectile;
class AATLAEarthSpikeLine;
class AATLAEarthWall;
class AATLAEarthArmor;
class AATLAEarthColumn;
class AATLAHoistedBoulder;
class AATLAEarthCocoon;

/** Rock Jab: the basic earth projectile. Tap-fired (no autofire); heavier and slower-cadenced than the water whip. */
UCLASS()
class ATLA_API UATLAAbility_RockJab : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_RockJab();
protected:
	virtual void OnCast() override;
	virtual FName GetMontageStartSection() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	TSubclassOf<AATLARockProjectile> ProjectileClass;

	/** Second beat: the punch that sends the rock the uppercut just raised */
	void PunchHeldRock();

	/** Source-time gap between the uppercut and the punch, scaled by MontageRate */
	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	float PunchBeatDelay = 0.22f;

	TWeakObjectPtr<AATLARockProjectile> HeldRock;
	FTimerHandle PunchTimer;

	int32 CastCount = 0;
};

/** Boulder: hold-charged heavy projectile with knockback. Released by the character input after a >=0.6s hold. */
UCLASS()
class ATLA_API UATLAAbility_Boulder : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_Boulder();
protected:
	virtual void OnCast() override;
	virtual void PlayCastMontage(UAnimMontage* Montage) override;

	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	TSubclassOf<AATLABoulderProjectile> ProjectileClass;
};

/** Earth Spikes: a wave of stone spikes erupts along the ground toward the aim. */
UCLASS()
class ATLA_API UATLAAbility_EarthSpikes : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_EarthSpikes();
protected:
	virtual void OnCast() override;
	virtual FName GetMontageStartSection() const override { return FName(TEXT("Melee03")); }

	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	TSubclassOf<AATLAEarthSpikeLine> SpikeLineClass;
};

/** Earth Wall: raise a destructible wall; re-tap while it stands (in range, near crosshair) to hurl it as a slab. */
UCLASS()
class ATLA_API UATLAAbility_EarthWall : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_EarthWall();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnCast() override;
	virtual UGameplayEffect* GetCostGameplayEffect() const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	bool WantsLaunch() const;

	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	TSubclassOf<AATLAEarthWall> WallClass;

	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	float WallDistance = 320.f;

	TWeakObjectPtr<AATLAEarthWall> ActiveWall;
};

/** Earth Armor: 12s of stone plates — 40% DR, grab/knockback immunity, -15% speed. */
UCLASS()
class ATLA_API UATLAAbility_EarthArmor : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_EarthArmor();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	TSubclassOf<AATLAEarthArmor> ArmorClass;
};

/** Earth Launch: a column hurls the bender up and forward. Airborne = disarmed; the rooted element's gamble. */
UCLASS()
class ATLA_API UATLAAbility_EarthLaunch : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_EarthLaunch();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	TSubclassOf<AATLAEarthColumn> ColumnClass;
};

/**
 * Boulder Hoist (hold RMB): a huge boulder tears out of the ground ahead and
 * hovers while the bender braces (rooted — knockback immune, slowed). Release
 * hurls it at the crosshair; releasing before it tears free sinks it back.
 */
UCLASS()
class ATLA_API UATLAAbility_BoulderHoist : public UATLABendingAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_BoulderHoist();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Hovering too long throws automatically — a boulder is heavy */
	void AutoThrow();

	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	TSubclassOf<AATLAHoistedBoulder> BoulderClass;

	UPROPERTY(EditDefaultsOnly, Category = "Earth")
	TSubclassOf<AATLABoulderProjectile> ThrownClass;

	UPROPERTY()
	TObjectPtr<AATLAHoistedBoulder> Hoisted;

	FTimerHandle AutoThrowTimer;
	float SavedSpeed = 0.f;
};

/** Root Stance: immobile neutral-jing channel — fast earth regen, 30% DR, knockback immunity. Held like Draw. */
UCLASS()
class ATLA_API UATLAAbility_RootStance : public UATLABendingAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_RootStance();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Ends the channel automatically once earth is full (like Draw when water fills) */
	void CheckFull();

	FActiveGameplayEffectHandle RootEffectHandle;
	FTimerHandle FullCheckTimer;
};
