using UnrealBuildTool;
using System.Collections.Generic;

public class AstoniaUE5EditorTarget : TargetRules
{
	public AstoniaUE5EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "AstoniaCore", "AstoniaGame" });
		ExtraModuleNames.Add("AstoniaEditor");
	}
}
