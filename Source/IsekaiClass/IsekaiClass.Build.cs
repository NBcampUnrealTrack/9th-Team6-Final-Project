using UnrealBuildTool;

public class IsekaiClass : ModuleRules
{
    public IsekaiClass(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",
            "SlateCore",
            "GameplayTags",
            "GameplayAbilities",
            "GameplayTasks",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "NavigationSystem",
            "Niagara",
            "AIModule"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
            PrivateDependencyModuleNames.Add("UMGEditor");
        }
    }
}
