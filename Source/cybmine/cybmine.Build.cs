// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class cybmine : ModuleRules
{
	public cybmine(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"NetCore",
			"CoreOnline",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"AssetRegistry",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"cybmine",
			"cybmine/Variant_Platforming",
			"cybmine/Variant_Platforming/Animation",
			"cybmine/Variant_Combat",
			"cybmine/Variant_Combat/AI",
			"cybmine/Variant_Combat/Animation",
			"cybmine/Variant_Combat/Gameplay",
			"cybmine/Variant_Combat/Interfaces",
			"cybmine/Variant_Combat/UI",
			"cybmine/Variant_SideScrolling",
			"cybmine/Variant_SideScrolling/AI",
			"cybmine/Variant_SideScrolling/Gameplay",
			"cybmine/Variant_SideScrolling/Interfaces",
			"cybmine/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
