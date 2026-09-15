// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAGameplayTags.h"

namespace ATLATags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Water, "Element.Water", "Waterbending element");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Earth, "Element.Earth", "Earthbending element");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Fire, "Element.Fire", "Firebending element");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Air, "Element.Air", "Airbending element");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Water_Pull, "Ability.Water.Pull", "Draw water from a source toward the bender");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Water_Whip, "Ability.Water.Whip", "Offensive water whip strike");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Water_Shield, "Ability.Water.Shield", "Defensive water barrier");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Water_IceSpears, "Ability.Water.IceSpears", "Volley of frozen darts");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Water_IceWall, "Ability.Water.IceWall", "Raise a defensive wall of ice from the ground");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Water_Octopus, "Ability.Water.Octopus", "Octopus form: rotating water tendrils lash nearby enemies");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Movement_Dodge, "Ability.Movement.Dodge", "Quick dash; doubled within the window becomes a dodge roll");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Movement_Dodge, "Cooldown.Movement.Dodge", "Dodge is on cooldown");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Earth_Jab, "Ability.Earth.Jab", "Rock jab: basic earth projectile");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Earth_Boulder, "Ability.Earth.Boulder", "Charged boulder: heavy knockback projectile");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Earth_Spikes, "Ability.Earth.Spikes", "Erupting spike line along the ground");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Earth_Wall, "Ability.Earth.Wall", "Raise (or re-tap: launch) an earth wall");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Earth_Armor, "Ability.Earth.Armor", "Earth armor: damage reduction + grab immunity form");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Earth_Launch, "Ability.Earth.Launch", "Earth column launch: vertical mobility");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Earth_Root, "Ability.Earth.Root", "Root stance: immobile channel, fast regen, damage reduction");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Earth_Hoist, "Ability.Earth.Hoist", "Boulder hoist: raise a huge boulder from the ground, throw on release");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Earth_Jab, "Cooldown.Earth.Jab", "Rock jab cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Earth_Boulder, "Cooldown.Earth.Boulder", "Boulder cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Earth_Spikes, "Cooldown.Earth.Spikes", "Spike line cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Earth_Wall, "Cooldown.Earth.Wall", "Earth wall cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Earth_Armor, "Cooldown.Earth.Armor", "Earth armor cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Earth_Launch, "Cooldown.Earth.Launch", "Earth launch cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Earth_Hoist, "Cooldown.Earth.Hoist", "Boulder hoist cooldown");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Jab, "Ability.Fire.Jab", "Rapid flame bolt");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Blast, "Ability.Fire.Blast", "Charged explosive fire blast");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Arc, "Ability.Fire.Arc", "Sweeping fan of flame bolts");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Lash, "Ability.Fire.Lash", "Fire lash: a sweeping crescent of flame");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Lightning, "Ability.Fire.Lightning", "Lightning: cold-blooded fire — charge and release a bolt");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Wall, "Ability.Fire.Wall", "Wall of flame: burns crossers, destroys projectiles");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Nova, "Ability.Fire.Nova", "Radial inferno burst around the bender");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Jet, "Ability.Fire.Jet", "Jet propulsion dash");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Breath, "Ability.Fire.Breath", "Recovery breath: channel to surge chi recovery");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire_Stream, "Ability.Fire.Stream", "Breath of fire: held flame stream from the mouth");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Fire_Jab, "Cooldown.Fire.Jab", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Fire_Blast, "Cooldown.Fire.Blast", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Fire_Arc, "Cooldown.Fire.Arc", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Fire_Wall, "Cooldown.Fire.Wall", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Fire_Nova, "Cooldown.Fire.Nova", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Fire_Jet, "Cooldown.Fire.Jet", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Fire_Lightning, "Cooldown.Fire.Lightning", "");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Air_Blast, "Ability.Air.Blast", "Palm-thrust air projectile: light damage, heavy push");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Air_Swipe, "Ability.Air.Swipe", "Wide crescent of compressed air");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Air_Shield, "Ability.Air.Shield", "Wind dome that destroys incoming projectiles");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Air_Cyclone, "Ability.Air.Cyclone", "Tornado that lifts and spins enemies");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Air_Scooter, "Ability.Air.Scooter", "Air scooter: burst of riding speed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Air_Updraft, "Ability.Air.Updraft", "Updraft glide: hold to fall gently");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Air_Blast, "Cooldown.Air.Blast", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Air_Swipe, "Cooldown.Air.Swipe", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Air_Shield, "Cooldown.Air.Shield", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Air_Cyclone, "Cooldown.Air.Cyclone", "");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Air_Scooter, "Cooldown.Air.Scooter", "");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Bending_Empowered, "State.Bending.Empowered", "Signature form active: regular attacks amplified (bigger, harder)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Fire_Stoked, "State.Fire.Stoked", "Inner drive above half: flames burn hotter (+20% fire damage)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Fire_Blazing, "State.Fire.Blazing", "Inner drive near full: the inner fire rages (+35% fire damage)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Earth_Grounded, "State.Earth.Grounded", "Standing on bendable earth; required by every earth ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Earth_Rooted, "State.Earth.Rooted", "Root stance active: damage reduction + knockback/grab immunity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Earth_Armored, "State.Earth.Armored", "Earth armor active: damage reduction + knockback/grab immunity");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Water_Whip, "Cooldown.Water.Whip", "Water whip is on cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Water_IceSpears, "Cooldown.Water.IceSpears", "Ice spears are on cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Water_IceWall, "Cooldown.Water.IceWall", "Ice wall is on cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Water_Octopus, "Cooldown.Water.Octopus", "Octopus form is on cooldown");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combo_Window_Open, "Combo.Window.Open", "This bender is channeling and accepting a combo partner");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combo_Channeling, "Combo.Channeling", "This bender is locked in a channel as part of a combined technique");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Bending, "State.Bending", "Character is currently performing a bending ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Character is dead; blocks all ability activation");
}
