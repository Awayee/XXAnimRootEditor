// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class XXAnimRootEditor : ModuleRules
{
	public XXAnimRootEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
			}
        );
        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"AnimGraph",
				"ContentBrowser",
				"AssetRegistry",
			}
		);
	}
}
