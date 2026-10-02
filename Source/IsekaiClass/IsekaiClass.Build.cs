// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class IsekaiClass : ModuleRules
{
	public IsekaiClass(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
<<<<<<< HEAD
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG" });
=======
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore" });
>>>>>>> origin/dev

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

	}
}
