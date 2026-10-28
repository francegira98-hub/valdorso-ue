// Valdorso - Volto Valdorso, parte che gira nel gioco. Scritto da Claude il 17/10/2026.

using UnrealBuildTool;

public class VoltoValdorso : ModuleRules
{
	public VoltoValdorso(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"AnimGraphRuntime"
		});
	}
}
