// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/TimerHandle.h"
#include "ATLACharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAbilitySystemComponent;
class UATLAAttributeSet;
class UGameplayAbility;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** Which element's kit this bender wields */
UENUM(BlueprintType)
enum class EATLAElement : uint8
{
	Water,
	Earth,
	Fire,
	Air
};

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AATLACharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Ability system component driving all bending abilities. Lives on the
	 *  character (not PlayerState) — acceptable while there is no respawn flow. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	/** Health + chi attributes */
	UPROPERTY()
	UATLAAttributeSet* Attributes;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Primary bending Input Action (water whip) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BendPrimaryAction;

	/** Draw-water Input Action (hold near a source to refill) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BendDrawAction;

	/** Ice spears Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BendIceSpearsAction;

	/** Ice wall Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BendIceWallAction;

	/** Octopus form Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BendOctopusAction;

	/** Dodge Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DodgeAction;

	/** Earth launch Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* EarthLaunchAction;

	/** Element switch Input Action (demo/testing) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SwitchElementAction;

public:

	/** Constructor */
	AATLACharacter();

protected:

	/**
	 * Motion-matched locomotion (Game Animation Sample). When on, the mesh runs
	 * GASP's retarget graph — the motion-matching pose is generated on the UEFN
	 * skeleton and retargeted onto ours each frame — instead of the template
	 * blend space.
	 *
	 * OFF by default: the graph loads and runs, but GASP feeds it gait/stance/
	 * movement state from its own character through Blueprint interfaces
	 * (BPI_SandboxCharacter_Pawn / _ABP) that this C++ character doesn't
	 * implement, so it currently holds a static pose. Finishing that hand-off
	 * is the remaining work — see Design/anim-windows.md.
	 */
	UPROPERTY(EditAnywhere, Category="Animation")
	bool bUseMotionMatching = false;

	/** GASP's retarget wrapper graph; drives the mesh when motion matching is on */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSoftClassPtr<UAnimInstance> MotionMatchingAnimClass;

	/** Predicted path the pose search matches against — no trajectory, no matching */
	UPROPERTY(VisibleAnywhere, Category="Animation")
	TObjectPtr<class UCharacterTrajectoryComponent> TrajectoryComponent;

	/** Camera boom length; applied at BeginPlay so Blueprint stale values can't shadow it */
	UPROPERTY(EditAnywhere, Category="Camera")
	float CameraArmLength = 520.f;

	/** Raises the camera pivot above the character for a higher vantage
	 *  (high enough that the crosshair floats clear above the head) */
	UPROPERTY(EditAnywhere, Category="Camera")
	float CameraHeightOffset = 140.f;

	virtual void BeginPlay() override;


	/** Bending abilities granted on possession (set in the character Blueprint) */
	UPROPERTY(EditDefaultsOnly, Category="Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Server: initialize the ability system and grant default abilities */
	virtual void PossessedBy(AController* NewController) override;

	/** Client: re-initialize ability actor info once state has replicated */
	virtual void OnRep_PlayerState() override;

	/** Points the ability system at this character as both owner and avatar */
	void InitAbilitySystem();

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for primary bending input */
	void BendPrimary();

	/** Draw-water input pressed/released */
	void BendDrawStart();
	void BendDrawStop();

	/** Technique inputs — routed by the current element loadout */
	void BendSecondary();   // Q: ice spears / shard volley
	void BendStructure();   // E: ice wall / earth wall
	void BendSignature();   // R: octopus form / earth armor

	/** Earth primary is tap-vs-hold: tap = jab, hold >= threshold = boulder.
	 *  Fire primary shoots on press; holding ignites the breath-of-fire stream. */
	void BendPrimaryPressed();
	void BendPrimaryReleased();

	/** Fire LMB held past the threshold: the breath of fire ignites */
	void StartFireStream();

	/** Dodge input */
	void Dodge();

	/** Earth launch input (F); for fire, press starts the jet glide */
	void EarthLaunch();

	/** F released: the fire jet cuts out */
	void EarthLaunchReleased();

	/** Cycle element loadout (Tab) */
	void SwitchElement();

	/** Re-grants the ability kit for the current element */
	void GrantElementKit();

	/** Spawns the ambient element aura (destroying the previous one) */
	void SpawnAura();

	/** Themed zone anchor for each element (Tab teleports between them) */
public:
	static FVector GetZoneAnchor(EATLAElement Element);
protected:

	UPROPERTY()
	TObjectPtr<class AATLAElementAura> Aura;

	/** Growing element orb at the hand while a charged technique is held */
	UPROPERTY()
	TObjectPtr<class AATLAChargeGlow> ChargeGlow;

	/** Grounded check: standing on bendable earth maintains the grounded tag + regen */
	void UpdateGroundedState();

	/**
	 * Pose watchdog: samples the skeleton a few times a second and logs the
	 * full context (montage, slot, element, movement mode) the moment the body
	 * goes wrong — pelvis tipping over, legs through the floor, limbs stretched.
	 * Catches glitches during real play that scripted probes can't reproduce.
	 */
	void CheckPoseSanity();

	FTimerHandle PoseWatchTimer;
	float LastPoseComplaintTime = 0.f;

	/** Held-LMB autofire is an every-frame input; this paces it to one cast per
	 *  form so montages can't stack and shred the pose */
	float LastAutoFireTime = 0.f;

	UPROPERTY(EditAnywhere, Category = "Bending")
	float AutoFireInterval = 0.4f;

	/** Element loadout for this bender (Tab cycles at runtime) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Abilities")
	EATLAElement ElementLoadout = EATLAElement::Water;

public:
	/**
	 * Inner Drive (fire only), 0..1: firebending is generated from within, so
	 * the inner fire waxes with aggression — landing fire hits and taking
	 * blows stoke it, disengaging cools it. Crossing 0.5 grants State.Fire.
	 * Stoked (+20% fire damage), 0.85 grants Blazing (+35%); flames scale with
	 * it visually. Reset on element switch.
	 */
	UFUNCTION(BlueprintCallable, Category = "Fire")
	void StokeInnerDrive(float Amount);

	UFUNCTION(BlueprintPure, Category = "Fire")
	float GetInnerDrive() const { return InnerDrive; }

	/**
	 * Earth mastery, 0..1: earthbending is learned by listening — every landed
	 * earth hit deepens it. At 1.0 metalbending unlocks permanently (Toph's
	 * discovery): rock jabs become metal shards — faster, harder, darker.
	 * Mastery survives element switches; an unlock is not a mood.
	 */
	UFUNCTION(BlueprintCallable, Category = "Earth")
	void StokeEarthMastery(float Amount);

	UFUNCTION(BlueprintPure, Category = "Earth")
	float GetEarthMastery() const { return EarthMastery; }

	UFUNCTION(BlueprintPure, Category = "Earth")
	bool HasMetalbending() const { return bMetalUnlocked; }

	/** World seconds when this bender last took damage (lightning's calm gate) */
	float GetLastDamagedTime() const { return LastDamagedTime; }

	UFUNCTION(BlueprintCallable, Category = "Fire")
	void NoteDamageTaken();

protected:
	/** Cools the inner fire on a 0.1s cadence (timer — independent of actor tick settings) */
	void DecayInnerDrive();

	FTimerHandle DriveDecayTimer;

	float InnerDrive = 0.f;
	float EarthMastery = 0.f;
	bool bMetalUnlocked = false;
	float LastStokeTime = -1000.f;
	float LastDamagedTime = -1000.f;

	/** Keeps the Stoked/Blazing loose tags in step with the drive value */
	void SyncDriveTags();

	/** Starts the boulder wind-up animation once a hold is committed */
	void StartChargeAnim();

	/** Fire RMB hold: the lightning charge becomes visible (crackling glow, slowed walk) */
	void StartLightningCharge();

	/** True if any granted ability with this tag is currently active */
	bool IsAbilityActive(const struct FGameplayTag& AbilityTag) const;

public:
	/** True while standing within bending range of a water pool (water flows freely) */
	bool IsNearWaterSource() const { return bNearWaterSource; }

protected:

	TArray<FGameplayAbilitySpecHandle> ElementKitHandles;
	FActiveGameplayEffectHandle EarthRegenHandle;
	FTimerHandle GroundedTimer;
	FTimerHandle ChargeAnimTimer;
	FTimerHandle FireStreamTimer;
	float PrimaryPressTime = 0.f;
	bool bPrimaryHeld = false;

	UPROPERTY()
	TObjectPtr<class AATLAChargeGlow> LightningGlow;

	FTimerHandle LightningGlowTimer;
	float RMBPressTime = 0.f;
	bool bRMBHeld = false;
	bool bLightningSlowed = false;
	bool bNearWaterSource = false;

	/** Shared activation-with-hint helper */
	void TryBend(const struct FGameplayTag& AbilityTag, float WaterCost);

	void ExitBendingStance();

	FTimerHandle StanceTimer;

public:

	/** Combat stance: strafe movement, character faces the camera aim.
	 *  Called by bending abilities on every successful cast; relaxes after a quiet period. */
	void EnterBendingStance();

protected:

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	/** Try to activate any granted ability matching the given tags (wire to input in Blueprint) */
	UFUNCTION(BlueprintCallable, Category="Abilities")
	bool ActivateAbilitiesByTag(FGameplayTagContainer AbilityTags);

	/** Current chi, for HUD display */
	UFUNCTION(BlueprintPure, Category="Attributes")
	float GetChi() const;

	/** Current carried water */
	UFUNCTION(BlueprintPure, Category="Attributes")
	float GetWater() const;

	/** Current positional earth */
	UFUNCTION(BlueprintPure, Category="Attributes")
	float GetEarth() const;

	/** Current element loadout */
	UFUNCTION(BlueprintPure, Category="Abilities")
	EATLAElement GetElementLoadout() const { return ElementLoadout; }

	/** Set the loadout and re-grant abilities (also used by test automation) */
	UFUNCTION(BlueprintCallable, Category="Abilities")
	void SetElementLoadout(EATLAElement NewElement);

	/** Current health, for HUD display */
	UFUNCTION(BlueprintPure, Category="Attributes")
	float GetHealth() const;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

