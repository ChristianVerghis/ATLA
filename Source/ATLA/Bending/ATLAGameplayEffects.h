// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ATLAGameplayEffects.generated.h"

// ---- Costs (carried water) ----

/** Instant water cost for the water whip (1) */
UCLASS()
class ATLA_API UGE_Cost_WaterWhip : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cost_WaterWhip();
};

/** Instant water cost for ice spears (10) */
UCLASS()
class ATLA_API UGE_Cost_IceSpears : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cost_IceSpears();
};

/** Instant water cost for the ice wall (25) */
UCLASS()
class ATLA_API UGE_Cost_IceWall : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cost_IceWall();
};

/** Instant water cost for octopus form (50) */
UCLASS()
class ATLA_API UGE_Cost_Octopus : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cost_Octopus();
};

/** Chi cost for a dodge (chi regenerates passively — it's the mobility resource) */
UCLASS()
class ATLA_API UGE_Cost_Dodge : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cost_Dodge();
};

// ---- Cooldowns ----

UCLASS()
class ATLA_API UGE_Cooldown_Dodge : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Dodge();
};

UCLASS()
class ATLA_API UGE_Cooldown_WaterWhip : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cooldown_WaterWhip();
};

UCLASS()
class ATLA_API UGE_Cooldown_IceSpears : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cooldown_IceSpears();
};

UCLASS()
class ATLA_API UGE_Cooldown_IceWall : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cooldown_IceWall();
};

UCLASS()
class ATLA_API UGE_Cooldown_Octopus : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Octopus();
};

// ---- Damage ----

/** Health damage dealt by a water whip hit */
UCLASS()
class ATLA_API UGE_Damage_WaterWhip : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Damage_WaterWhip();
};

/** Health damage per ice spear hit */
UCLASS()
class ATLA_API UGE_Damage_IceSpear : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Damage_IceSpear();
};

/** Health damage per octopus tendril lash */
UCLASS()
class ATLA_API UGE_Damage_OctopusLash : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Damage_OctopusLash();
};

// ---- Earth ----

UCLASS() class ATLA_API UGE_EarthCost_Jab : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCost_Jab(); };
UCLASS() class ATLA_API UGE_EarthCost_Boulder : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCost_Boulder(); };
UCLASS() class ATLA_API UGE_EarthCost_Spikes : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCost_Spikes(); };
UCLASS() class ATLA_API UGE_EarthCost_Wall : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCost_Wall(); };
UCLASS() class ATLA_API UGE_EarthCost_WallLaunch : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCost_WallLaunch(); };
UCLASS() class ATLA_API UGE_EarthCost_Armor : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCost_Armor(); };
UCLASS() class ATLA_API UGE_EarthCost_Launch : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCost_Launch(); };
UCLASS() class ATLA_API UGE_EarthCost_Hoist : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCost_Hoist(); };

UCLASS() class ATLA_API UGE_EarthCooldown_Jab : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCooldown_Jab(); };
UCLASS() class ATLA_API UGE_EarthCooldown_Boulder : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCooldown_Boulder(); };
UCLASS() class ATLA_API UGE_EarthCooldown_Spikes : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCooldown_Spikes(); };
UCLASS() class ATLA_API UGE_EarthCooldown_Wall : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCooldown_Wall(); };
UCLASS() class ATLA_API UGE_EarthCooldown_Armor : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCooldown_Armor(); };
UCLASS() class ATLA_API UGE_EarthCooldown_Launch : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCooldown_Launch(); };
UCLASS() class ATLA_API UGE_EarthCooldown_Hoist : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthCooldown_Hoist(); };

/** +6 Earth/s while standing on bendable ground */
UCLASS() class ATLA_API UGE_EarthRegen : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthRegen(); };
/** Root stance channel: +19 Earth/s extra and the Rooted tag (30% DR via attribute hook) */
UCLASS() class ATLA_API UGE_RootBonus : public UGameplayEffect { GENERATED_BODY() public: UGE_RootBonus(); };
/** Earth armor: 12s Armored tag (40% DR via attribute hook) */
UCLASS() class ATLA_API UGE_ArmorBuff : public UGameplayEffect { GENERATED_BODY() public: UGE_ArmorBuff(); };

UCLASS() class ATLA_API UGE_EarthDamage_Jab : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthDamage_Jab(); };
UCLASS() class ATLA_API UGE_EarthDamage_Boulder : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthDamage_Boulder(); };
UCLASS() class ATLA_API UGE_EarthDamage_Spike : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthDamage_Spike(); };
UCLASS() class ATLA_API UGE_EarthDamage_Slab : public UGameplayEffect { GENERATED_BODY() public: UGE_EarthDamage_Slab(); };

// ---- Fire (runs on chi) ----
UCLASS() class ATLA_API UGE_FireCost_Jab : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCost_Jab(); };
UCLASS() class ATLA_API UGE_FireCost_Blast : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCost_Blast(); };
UCLASS() class ATLA_API UGE_FireCost_Arc : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCost_Arc(); };
UCLASS() class ATLA_API UGE_FireCost_Wall : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCost_Wall(); };
UCLASS() class ATLA_API UGE_FireCost_Nova : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCost_Nova(); };
UCLASS() class ATLA_API UGE_FireCost_Jet : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCost_Jet(); };
UCLASS() class ATLA_API UGE_FireCost_Lightning : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCost_Lightning(); };
UCLASS() class ATLA_API UGE_FireCost_Stream : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCost_Stream(); };
UCLASS() class ATLA_API UGE_FireStreamDrain : public UGameplayEffect { GENERATED_BODY() public: UGE_FireStreamDrain(); };
UCLASS() class ATLA_API UGE_FireCooldown_Jab : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCooldown_Jab(); };
UCLASS() class ATLA_API UGE_FireCooldown_Blast : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCooldown_Blast(); };
UCLASS() class ATLA_API UGE_FireCooldown_Arc : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCooldown_Arc(); };
UCLASS() class ATLA_API UGE_FireCooldown_Wall : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCooldown_Wall(); };
UCLASS() class ATLA_API UGE_FireCooldown_Nova : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCooldown_Nova(); };
UCLASS() class ATLA_API UGE_FireCooldown_Jet : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCooldown_Jet(); };
UCLASS() class ATLA_API UGE_FireCooldown_Lightning : public UGameplayEffect { GENERATED_BODY() public: UGE_FireCooldown_Lightning(); };
UCLASS() class ATLA_API UGE_FireDamage_Jab : public UGameplayEffect { GENERATED_BODY() public: UGE_FireDamage_Jab(); };
UCLASS() class ATLA_API UGE_FireDamage_Blast : public UGameplayEffect { GENERATED_BODY() public: UGE_FireDamage_Blast(); };
UCLASS() class ATLA_API UGE_FireDamage_Arc : public UGameplayEffect { GENERATED_BODY() public: UGE_FireDamage_Arc(); };
UCLASS() class ATLA_API UGE_FireDamage_Burn : public UGameplayEffect { GENERATED_BODY() public: UGE_FireDamage_Burn(); };
UCLASS() class ATLA_API UGE_FireDamage_Nova : public UGameplayEffect { GENERATED_BODY() public: UGE_FireDamage_Nova(); };
UCLASS() class ATLA_API UGE_FireDamage_Lash : public UGameplayEffect { GENERATED_BODY() public: UGE_FireDamage_Lash(); };
UCLASS() class ATLA_API UGE_FireDamage_Lightning : public UGameplayEffect { GENERATED_BODY() public: UGE_FireDamage_Lightning(); };
UCLASS() class ATLA_API UGE_FireDamage_Stream : public UGameplayEffect { GENERATED_BODY() public: UGE_FireDamage_Stream(); };
/** Breath of fire: strong chi recovery while channeling */
UCLASS() class ATLA_API UGE_FireBreath : public UGameplayEffect { GENERATED_BODY() public: UGE_FireBreath(); };
/** 10s of signature-form empowerment: all damage dealt +50%, projectiles grow */
UCLASS() class ATLA_API UGE_Empowered : public UGameplayEffect { GENERATED_BODY() public: UGE_Empowered(); };

// ---- Air (runs on chi; damage is light, displacement is the weapon) ----
UCLASS() class ATLA_API UGE_AirCost_Blast : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCost_Blast(); };
UCLASS() class ATLA_API UGE_AirCost_Swipe : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCost_Swipe(); };
UCLASS() class ATLA_API UGE_AirCost_Shield : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCost_Shield(); };
UCLASS() class ATLA_API UGE_AirCost_Cyclone : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCost_Cyclone(); };
UCLASS() class ATLA_API UGE_AirCost_Scooter : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCost_Scooter(); };
UCLASS() class ATLA_API UGE_AirCooldown_Blast : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCooldown_Blast(); };
UCLASS() class ATLA_API UGE_AirCooldown_Swipe : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCooldown_Swipe(); };
UCLASS() class ATLA_API UGE_AirCooldown_Shield : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCooldown_Shield(); };
UCLASS() class ATLA_API UGE_AirCooldown_Cyclone : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCooldown_Cyclone(); };
UCLASS() class ATLA_API UGE_AirCooldown_Scooter : public UGameplayEffect { GENERATED_BODY() public: UGE_AirCooldown_Scooter(); };
UCLASS() class ATLA_API UGE_AirDamage_Blast : public UGameplayEffect { GENERATED_BODY() public: UGE_AirDamage_Blast(); };
UCLASS() class ATLA_API UGE_AirDamage_Swipe : public UGameplayEffect { GENERATED_BODY() public: UGE_AirDamage_Swipe(); };
UCLASS() class ATLA_API UGE_AirDamage_Cyclone : public UGameplayEffect { GENERATED_BODY() public: UGE_AirDamage_Cyclone(); };
/** Updraft glide: chi drain while held */
UCLASS() class ATLA_API UGE_AirUpdraftDrain : public UGameplayEffect { GENERATED_BODY() public: UGE_AirUpdraftDrain(); };

// ---- Utility ----

/** Passive chi regeneration, applied to every bender on possession */
UCLASS()
class ATLA_API UGE_ChiRegen : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ChiRegen();
};

/** Channeled water intake while drawing from a source (applied for the duration of the draw) */
UCLASS()
class ATLA_API UGE_DrawWater : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_DrawWater();
};
