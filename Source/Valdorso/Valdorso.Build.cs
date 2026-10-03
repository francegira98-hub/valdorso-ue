// Copyright Epic Games, Inc. All Rights Reserved.
// Modificato per Valdorso: aggiunto il Gameplay Ability System (GameplayAbilities, GameplayTags, GameplayTasks),
// il Motion Warping e SlateCore (per il menu scritto in C++).

using UnrealBuildTool;

public class Valdorso : ModuleRules
{
    public Valdorso(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "AIModule",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "Slate",
            "SlateCore",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
            "MotionWarping"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicIncludePaths.AddRange(new string[] {
            "Valdorso",
            "Valdorso/Variant_Platforming",
            "Valdorso/Variant_Platforming/Animation",
            "Valdorso/Variant_Combat",
            "Valdorso/Variant_Combat/AI",
            "Valdorso/Variant_Combat/Animation",
            "Valdorso/Variant_Combat/Gameplay",
            "Valdorso/Variant_Combat/Interfaces",
            "Valdorso/Variant_Combat/UI",
            "Valdorso/Variant_SideScrolling",
            "Valdorso/Variant_SideScrolling/AI",
            "Valdorso/Variant_SideScrolling/Gameplay",
            "Valdorso/Variant_SideScrolling/Interfaces",
            "Valdorso/Variant_SideScrolling/UI"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}