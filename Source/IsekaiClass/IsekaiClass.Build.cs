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
            "AnimGraphRuntime",
            "DeveloperSettings",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "CommonUI",
            "CommonInput",
            "MoviePlayer",
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
            PrivateDependencyModuleNames.Add("AssetRegistry");
            PrivateDependencyModuleNames.Add("AnimationBlueprintLibrary");
            PrivateDependencyModuleNames.Add("RenderCore");
            PrivateDependencyModuleNames.Add("RHI");
            PrivateDependencyModuleNames.Add("VRM4U");
            PrivateDependencyModuleNames.AddRange(new string[] {"KawaiiPhysics", "KawaiiPhysicsEd", "AnimGraph", "BlueprintGraph", "KismetCompiler"});
        }
    }
}
