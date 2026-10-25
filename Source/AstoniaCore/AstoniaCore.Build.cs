using UnrealBuildTool;

// Game rules and data formats ported from the Astonia 3.5 server. Keep this module free of
// actors and rendering so the rules can be unit-tested and later run on a dedicated server.
public class AstoniaCore : ModuleRules
{
	public AstoniaCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
