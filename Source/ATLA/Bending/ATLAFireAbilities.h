// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ATLACastAbility.h"
#include "ATLABendingAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/TimerHandle.h"
#include "ATLAFireAbilities.generated.h"

class AATLAFireBolt;
class AATLAFireBlastBolt;
class AATLAFireLashBolt;
class AATLAFireStreamBolt;
class AATLAFireWall;
class AATLAFireNova;

/** Fire Jab: rapid flame bolt — the fastest basic attack in the game. */
UCLASS()
class ATLA_API UATLAAbility_FireJab : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_FireJab();
protected:
	virtual void OnCast() override;
	virtual FName GetMontageStartSection() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TSubclassOf<AATLAFireBolt> BoltClass;

	int32 CastCount = 0;
};

/** Fire Blast: charged explosive ball (LMB hold-release). */
UCLASS()
class ATLA_API UATLAAbility_FireBlast : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_FireBlast();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TSubclassOf<AATLAFireBlastBolt> BoltClass;
};

/**
 * Breath of Fire: the held flame stream (LMB hold). A torrent of short-lived
 * flame tongues pours from the mouth along the crosshair, draining chi until
 * the button lifts or the chi runs dry.
 */
UCLASS()
class ATLA_API UATLAAbility_FireStream : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_FireStream();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** One tongue of the stream; repeats on a timer while held */
	void SpawnTongue();

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TSubclassOf<AATLAFireStreamBolt> TongueClass;

	/** Flame at the lips while the breath pours out */
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> MouthFlame;

	FActiveGameplayEffectHandle DrainHandle;
	FTimerHandle TongueTimer;

	/** True only after a successful commit — gates the EndAbility teardown */
	bool bChanneling = false;
};

/** Fire Lash: a wide sweeping crescent of flame — Zuko's whip-arc (Q). */
UCLASS()
class ATLA_API UATLAAbility_FireLash : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_FireLash();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TSubclassOf<AATLAFireLashBolt> LashClass;
};

class UNiagaraSystem;

/**
 * Lightning: the cold-blooded fire (RMB hold-release). The character input
 * handles the charge (glow + slow); this ability is the release — an instant
 * hitscan bolt at the crosshair for devastating damage.
 */
UCLASS()
class ATLA_API UATLAAbility_Lightning : public UATLABendingAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_Lightning();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Lightning is the cold-blooded fire: it demands complete calm. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float MaxRange = 8000.f;

	/** Seconds of not being hit required for the peace of mind to form */
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float CalmWindow = 4.f;
};

/** Fire Arc: a sweeping horizontal fan of three bolts. */
UCLASS()
class ATLA_API UATLAAbility_FireArc : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_FireArc();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TSubclassOf<AATLAFireBolt> BoltClass;
};

/** Wall of Flame: area denial that burns crossers and incinerates projectiles. */
UCLASS()
class ATLA_API UATLAAbility_FireWall : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_FireWall();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TSubclassOf<AATLAFireWall> WallClass;
};

/** Inferno Nova: radial burst around the bender. */
UCLASS()
class ATLA_API UATLAAbility_FireNova : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_FireNova();
protected:
	virtual void OnCast() override;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TSubclassOf<AATLAFireNova> NovaClass;
};

/**
 * Fire Jet: Azula's jet propulsion. Hold to blast flame plumes backward and
 * zip along the ground at boosted speed (uncapped for now); jumps float on
 * the thrust for a little air. Release and the burners cut out.
 */
UCLASS()
class ATLA_API UATLAAbility_FireJet : public UATLACastAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_FireJet();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Exhaust flame asset for the jet */
	UPROPERTY()
	TObjectPtr<UNiagaraSystem> ExhaustFX;

	/** The pair of thrust plumes blasting back from the feet while boosting */
	UPROPERTY()
	TArray<TObjectPtr<class UNiagaraComponent>> JetFlames;

	/** Kata stance sequence ridden (near-frozen) while the jet burns */
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> GlideStance;

	/** Near-frozen kata stance held while riding the jet (no run cycle) */
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> GlideMontage;

	/** True only after a successful commit — gates the EndAbility teardown */
	bool bGliding = false;

	/** Holds the capsule at hover height above the traced floor while jetting */
	FTimerHandle HoverTimer;

	/** Keeps the foot plumes pointed back-and-down as the bender turns */
	FTimerHandle PlumeAimTimer;

	/** Per-plume exhaust direction in bender space, parallel to JetFlames */
	TArray<FVector> JetPlumeDirs;
};

/** Breath of Fire: immobile channel that surges chi recovery. */
UCLASS()
class ATLA_API UATLAAbility_FireBreath : public UATLABendingAbility
{
	GENERATED_BODY()
public:
	UATLAAbility_FireBreath();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	void CheckFull();

	/** Small flame at the mouth while channeling */
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> BreathFlame;

	FActiveGameplayEffectHandle BreathHandle;
	FTimerHandle FullCheckTimer;
};
