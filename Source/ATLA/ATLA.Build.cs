// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ATLA : ModuleRules
{
	public ATLA(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"Niagara",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			// Motion matching (GASP): the trajectory component feeds the
			// pose-search query that drives the locomotion graph
			"MotionTrajectory"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ATLA",
			"ATLA/Bending",
			"ATLA/Variant_Platforming",
			"ATLA/Variant_Platforming/Animation",
			"ATLA/Variant_Combat",
			"ATLA/Variant_Combat/AI",
			"ATLA/Variant_Combat/Animation",
			"ATLA/Variant_Combat/Gameplay",
			"ATLA/Variant_Combat/Interfaces",
			"ATLA/Variant_Combat/UI",
			"ATLA/Variant_SideScrolling",
			"ATLA/Variant_SideScrolling/AI",
			"ATLA/Variant_SideScrolling/Gameplay",
			"ATLA/Variant_SideScrolling/Interfaces",
			"ATLA/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
