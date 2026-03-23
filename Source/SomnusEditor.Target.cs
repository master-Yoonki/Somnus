// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SomnusEditorTarget : TargetRules
{
	public SomnusEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("Somnus");

		// Disable optimizations for easier debugging without DebugGame linker issues
		// (DebugGame config has unresolved engine symbols with installed engine builds)
		bOverrideBuildEnvironment = true;
		AdditionalCompilerArguments = "/Od";  // Disable optimizations (MSVC)
	}
}
