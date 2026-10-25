using UnrealBuildTool;
using System.Collections.Generic;

public class AstoniaUE5Target : TargetRules
{
	public AstoniaUE5Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "AstoniaCore", "AstoniaGame" });

	}
}
