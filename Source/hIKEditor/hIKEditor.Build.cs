// Copyright (c) Mathew Wang 2021

using UnrealBuildTool;

public class hIKEditor : ModuleRules
{
	public hIKEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"hIK",
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"UnrealEd"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"EditorStyle",
				"AnimGraphRuntime",
				"AnimGraph",
				"BlueprintGraph",
				"PropertyEditor",
				"Slate",
				"SlateCore"
			}
			);

		// PrivatePCHHeaderFile = "hIKEditor.h";

		// PublicIncludePaths.AddRange(new string[] { "hIKEditor/Public", "hIKEditor/Public/GraphNodes" });

		// PrivateIncludePaths.AddRange(new string[] { "hIKEditor/Private", "hIKEditor/Private/GraphNodes" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
