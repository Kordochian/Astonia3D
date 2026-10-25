using UnrealBuildTool;

// Editor-only tools: importers that turn the original zone files into Unreal data and levels.
public class AstoniaEditor : ModuleRules
{
	public AstoniaEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AstoniaCore" });
	}
}
