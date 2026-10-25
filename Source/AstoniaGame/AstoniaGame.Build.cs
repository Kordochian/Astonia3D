using UnrealBuildTool;

// Actors, components and input for the 3D game. Rules live in AstoniaCore.
public class AstoniaGame : ModuleRules
{
	public AstoniaGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"UMG",
			"Slate",
			"SlateCore",
			"HairStrandsCore",
			"AstoniaCore"
		});
	}
}
