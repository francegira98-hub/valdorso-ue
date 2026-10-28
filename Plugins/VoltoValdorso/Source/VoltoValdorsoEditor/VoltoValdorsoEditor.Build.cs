// Valdorso - Volto Valdorso, parte dell'editor (il nodo nel grafo). Scritto da Claude il 17/10/2026.

using UnrealBuildTool;

public class VoltoValdorsoEditor : ModuleRules
{
	public VoltoValdorsoEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"AnimGraph",
			"AnimGraphRuntime",
			"BlueprintGraph",
			"VoltoValdorso"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd"
		});
	}
}
