// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class hIK : ModuleRules
{
	public hIK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				//"$(EIGEN)\\",
				"$(LIB_HIK_INC)\\"
			}
			);

		PublicAdditionalLibraries.AddRange(
			new string [] {
				"$(LIB_HIK_BIN)\\lib_hIK.lib"
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"AnimGraphRuntime",
				"AnimationCore"
			});



		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...
			}
			);
		//PrivatePCHHeaderFile = "stdafx.h";
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
