

using UnrealBuildTool;
using System.Collections.Generic;

public class IsekaiClassTarget : TargetRules
{
	public IsekaiClassTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("IsekaiClass");
		DisableUnityBuildForModules = new string[] { "VRM4ULoader" };
	}
}
