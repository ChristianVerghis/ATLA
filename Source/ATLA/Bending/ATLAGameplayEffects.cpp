// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAGameplayEffects.h"
#include "ATLAAttributeSet.h"
#include "ATLAGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace
{
	FGameplayModifierInfo MakeModifier(const FGameplayAttribute& Attribute, float Magnitude)
	{
		FGameplayModifierInfo Info;
		Info.Attribute = Attribute;
		Info.ModifierOp = EGameplayModOp::Additive;
		Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Magnitude));
		return Info;
	}
}

// Instant water cost. GAS refuses activation when the cost can't be paid.
static void SetupWaterCost(UGameplayEffect* Effect, float Amount)
{
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	Effect->Modifiers.Add(MakeModifier(UATLAAttributeSet::GetWaterAttribute(), -Amount));
}

// Duration + granted-tag setup for a cooldown effect. The caller (a
// UGameplayEffect subclass constructor) must create the component with
// CreateDefaultSubobject and add it to its protected GEComponents itself.
static void SetupCooldown(UGameplayEffect* Effect, UTargetTagsGameplayEffectComponent* TargetTags, float Seconds, const FGameplayTag& CooldownTag)
{
	Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Seconds));

	FInheritedTagContainer TagChanges;
	TagChanges.Added.AddTag(CooldownTag);
	TargetTags->SetAndApplyTargetTagChanges(TagChanges);
}

static void SetupHealthDamage(UGameplayEffect* Effect, float Amount)
{
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	Effect->Modifiers.Add(MakeModifier(UATLAAttributeSet::GetHealthAttribute(), -Amount));
}

UGE_Cost_WaterWhip::UGE_Cost_WaterWhip() { SetupWaterCost(this, 1.f); }
UGE_Cost_IceSpears::UGE_Cost_IceSpears() { SetupWaterCost(this, 10.f); }
UGE_Cost_IceWall::UGE_Cost_IceWall() { SetupWaterCost(this, 25.f); }
UGE_Cost_Octopus::UGE_Cost_Octopus() { SetupWaterCost(this, 50.f); }

UGE_Cost_Dodge::UGE_Cost_Dodge()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeModifier(UATLAAttributeSet::GetChiAttribute(), -12.f));
}

UGE_Cooldown_Dodge::UGE_Cooldown_Dodge()
{
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	SetupCooldown(this, TargetTags, 0.35f, ATLATags::Cooldown_Movement_Dodge);
}

UGE_Cooldown_WaterWhip::UGE_Cooldown_WaterWhip()
{
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	SetupCooldown(this, TargetTags, 0.25f, ATLATags::Cooldown_Water_Whip);
}

UGE_Cooldown_IceSpears::UGE_Cooldown_IceSpears()
{
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	SetupCooldown(this, TargetTags, 1.f, ATLATags::Cooldown_Water_IceSpears);
}

UGE_Cooldown_IceWall::UGE_Cooldown_IceWall()
{
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	SetupCooldown(this, TargetTags, 1.f, ATLATags::Cooldown_Water_IceWall);
}

UGE_Cooldown_Octopus::UGE_Cooldown_Octopus()
{
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	SetupCooldown(this, TargetTags, 10.f, ATLATags::Cooldown_Water_Octopus);
}

UGE_Damage_WaterWhip::UGE_Damage_WaterWhip() { SetupHealthDamage(this, 15.f); }
UGE_Damage_IceSpear::UGE_Damage_IceSpear() { SetupHealthDamage(this, 8.f); }
UGE_Damage_OctopusLash::UGE_Damage_OctopusLash() { SetupHealthDamage(this, 10.f); }

static void SetupEarthCost(UGameplayEffect* Effect, float Amount)
{
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	Effect->Modifiers.Add(MakeModifier(UATLAAttributeSet::GetEarthAttribute(), -Amount));
}

UGE_EarthCost_Jab::UGE_EarthCost_Jab() { SetupEarthCost(this, 2.f); }
UGE_EarthCost_Boulder::UGE_EarthCost_Boulder() { SetupEarthCost(this, 18.f); }
UGE_EarthCost_Spikes::UGE_EarthCost_Spikes() { SetupEarthCost(this, 12.f); }
UGE_EarthCost_Wall::UGE_EarthCost_Wall() { SetupEarthCost(this, 25.f); }
UGE_EarthCost_WallLaunch::UGE_EarthCost_WallLaunch() { SetupEarthCost(this, 5.f); }
UGE_EarthCost_Armor::UGE_EarthCost_Armor() { SetupEarthCost(this, 50.f); }
UGE_EarthCost_Hoist::UGE_EarthCost_Hoist() { SetupEarthCost(this, 30.f); }

UGE_EarthCost_Launch::UGE_EarthCost_Launch()
{
	// Dual cost: 5 earth + 14 chi (the vertical gamble)
	SetupEarthCost(this, 5.f);
	Modifiers.Add(MakeModifier(UATLAAttributeSet::GetChiAttribute(), -14.f));
}

#define ATLA_EARTH_COOLDOWN(ClassName, Seconds, Tag) 	ClassName::ClassName() 	{ 		UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags")); 		GEComponents.Add(TargetTags); 		SetupCooldown(this, TargetTags, Seconds, Tag); 	}

ATLA_EARTH_COOLDOWN(UGE_EarthCooldown_Jab, 0.45f, ATLATags::Cooldown_Earth_Jab)
ATLA_EARTH_COOLDOWN(UGE_EarthCooldown_Boulder, 3.f, ATLATags::Cooldown_Earth_Boulder)
ATLA_EARTH_COOLDOWN(UGE_EarthCooldown_Spikes, 3.f, ATLATags::Cooldown_Earth_Spikes)
ATLA_EARTH_COOLDOWN(UGE_EarthCooldown_Wall, 4.f, ATLATags::Cooldown_Earth_Wall)
ATLA_EARTH_COOLDOWN(UGE_EarthCooldown_Armor, 15.f, ATLATags::Cooldown_Earth_Armor)
ATLA_EARTH_COOLDOWN(UGE_EarthCooldown_Launch, 2.5f, ATLATags::Cooldown_Earth_Launch)
ATLA_EARTH_COOLDOWN(UGE_EarthCooldown_Hoist, 6.f, ATLATags::Cooldown_Earth_Hoist)

UGE_EarthRegen::UGE_EarthRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.5f);
	bExecutePeriodicEffectOnApplication = false;
	Modifiers.Add(MakeModifier(UATLAAttributeSet::GetEarthAttribute(), 3.f));
}

UGE_RootBonus::UGE_RootBonus()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.5f);
	bExecutePeriodicEffectOnApplication = true;
	Modifiers.Add(MakeModifier(UATLAAttributeSet::GetEarthAttribute(), 9.5f));

	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	FInheritedTagContainer TagChanges;
	TagChanges.Added.AddTag(ATLATags::State_Earth_Rooted);
	TargetTags->SetAndApplyTargetTagChanges(TagChanges);
}

UGE_ArmorBuff::UGE_ArmorBuff()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(12.f));

	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	FInheritedTagContainer TagChanges;
	TagChanges.Added.AddTag(ATLATags::State_Earth_Armored);
	TargetTags->SetAndApplyTargetTagChanges(TagChanges);
}

UGE_EarthDamage_Jab::UGE_EarthDamage_Jab() { SetupHealthDamage(this, 22.f); }
UGE_EarthDamage_Boulder::UGE_EarthDamage_Boulder() { SetupHealthDamage(this, 40.f); }
UGE_EarthDamage_Spike::UGE_EarthDamage_Spike() { SetupHealthDamage(this, 25.f); }
UGE_EarthDamage_Slab::UGE_EarthDamage_Slab() { SetupHealthDamage(this, 35.f); }

static void SetupChiCost(UGameplayEffect* Effect, float Amount)
{
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	Effect->Modifiers.Add(MakeModifier(UATLAAttributeSet::GetChiAttribute(), -Amount));
}

UGE_FireCost_Jab::UGE_FireCost_Jab() { SetupChiCost(this, 4.f); }
UGE_FireCost_Blast::UGE_FireCost_Blast() { SetupChiCost(this, 15.f); }
UGE_FireCost_Arc::UGE_FireCost_Arc() { SetupChiCost(this, 12.f); }
UGE_FireCost_Wall::UGE_FireCost_Wall() { SetupChiCost(this, 25.f); }
UGE_FireCost_Nova::UGE_FireCost_Nova() { SetupChiCost(this, 45.f); }
UGE_FireCost_Jet::UGE_FireCost_Jet() { SetupChiCost(this, 8.f); }
UGE_FireCost_Lightning::UGE_FireCost_Lightning() { SetupChiCost(this, 35.f); }
UGE_FireCost_Stream::UGE_FireCost_Stream() { SetupChiCost(this, 5.f); }

// Breath of fire burns chi for as long as the stream is held
UGE_FireStreamDrain::UGE_FireStreamDrain()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.25f);
	bExecutePeriodicEffectOnApplication = true;
	Modifiers.Add(MakeModifier(UATLAAttributeSet::GetChiAttribute(), -3.f));
}

ATLA_EARTH_COOLDOWN(UGE_FireCooldown_Jab, 0.3f, ATLATags::Cooldown_Fire_Jab)
ATLA_EARTH_COOLDOWN(UGE_FireCooldown_Blast, 2.5f, ATLATags::Cooldown_Fire_Blast)
ATLA_EARTH_COOLDOWN(UGE_FireCooldown_Arc, 1.5f, ATLATags::Cooldown_Fire_Arc)
ATLA_EARTH_COOLDOWN(UGE_FireCooldown_Wall, 8.f, ATLATags::Cooldown_Fire_Wall)
ATLA_EARTH_COOLDOWN(UGE_FireCooldown_Nova, 12.f, ATLATags::Cooldown_Fire_Nova)
ATLA_EARTH_COOLDOWN(UGE_FireCooldown_Jet, 0.8f, ATLATags::Cooldown_Fire_Jet)
ATLA_EARTH_COOLDOWN(UGE_FireCooldown_Lightning, 10.f, ATLATags::Cooldown_Fire_Lightning)
ATLA_EARTH_COOLDOWN(UGE_Empowered, 10.f, ATLATags::State_Bending_Empowered)

UGE_FireDamage_Jab::UGE_FireDamage_Jab() { SetupHealthDamage(this, 18.f); }
UGE_FireDamage_Blast::UGE_FireDamage_Blast() { SetupHealthDamage(this, 35.f); }
UGE_FireDamage_Arc::UGE_FireDamage_Arc() { SetupHealthDamage(this, 12.f); }
UGE_FireDamage_Lash::UGE_FireDamage_Lash() { SetupHealthDamage(this, 22.f); }
UGE_FireDamage_Lightning::UGE_FireDamage_Lightning() { SetupHealthDamage(this, 50.f); }
UGE_FireDamage_Burn::UGE_FireDamage_Burn() { SetupHealthDamage(this, 10.f); }
UGE_FireDamage_Nova::UGE_FireDamage_Nova() { SetupHealthDamage(this, 30.f); }
UGE_FireDamage_Stream::UGE_FireDamage_Stream() { SetupHealthDamage(this, 7.f); }

UGE_FireBreath::UGE_FireBreath()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.5f);
	bExecutePeriodicEffectOnApplication = true;
	Modifiers.Add(MakeModifier(UATLAAttributeSet::GetChiAttribute(), 6.f));
}

UGE_AirCost_Blast::UGE_AirCost_Blast() { SetupChiCost(this, 5.f); }
UGE_AirCost_Swipe::UGE_AirCost_Swipe() { SetupChiCost(this, 10.f); }
UGE_AirCost_Shield::UGE_AirCost_Shield() { SetupChiCost(this, 20.f); }
UGE_AirCost_Cyclone::UGE_AirCost_Cyclone() { SetupChiCost(this, 40.f); }
UGE_AirCost_Scooter::UGE_AirCost_Scooter() { SetupChiCost(this, 20.f); }

ATLA_EARTH_COOLDOWN(UGE_AirCooldown_Blast, 0.35f, ATLATags::Cooldown_Air_Blast)
ATLA_EARTH_COOLDOWN(UGE_AirCooldown_Swipe, 1.2f, ATLATags::Cooldown_Air_Swipe)
ATLA_EARTH_COOLDOWN(UGE_AirCooldown_Shield, 6.f, ATLATags::Cooldown_Air_Shield)
ATLA_EARTH_COOLDOWN(UGE_AirCooldown_Cyclone, 12.f, ATLATags::Cooldown_Air_Cyclone)
ATLA_EARTH_COOLDOWN(UGE_AirCooldown_Scooter, 6.f, ATLATags::Cooldown_Air_Scooter)

UGE_AirDamage_Blast::UGE_AirDamage_Blast() { SetupHealthDamage(this, 8.f); }
UGE_AirDamage_Swipe::UGE_AirDamage_Swipe() { SetupHealthDamage(this, 12.f); }
UGE_AirDamage_Cyclone::UGE_AirDamage_Cyclone() { SetupHealthDamage(this, 15.f); }

UGE_AirUpdraftDrain::UGE_AirUpdraftDrain()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.5f);
	bExecutePeriodicEffectOnApplication = true;
	Modifiers.Add(MakeModifier(UATLAAttributeSet::GetChiAttribute(), -4.f));
}

UGE_ChiRegen::UGE_ChiRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.5f);
	bExecutePeriodicEffectOnApplication = false;
	Modifiers.Add(MakeModifier(UATLAAttributeSet::GetChiAttribute(), 4.f));
}

UGE_DrawWater::UGE_DrawWater()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(0.2f);
	bExecutePeriodicEffectOnApplication = true;
	Modifiers.Add(MakeModifier(UATLAAttributeSet::GetWaterAttribute(), 8.f));
}
