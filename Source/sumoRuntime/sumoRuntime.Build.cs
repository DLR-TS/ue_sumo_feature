// Copyright Epic Games, Inc. All Rights Reserved.

using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

public class sumoRuntime : ModuleRules
{
	public sumoRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		string sumoPath = Path.Combine(ModuleDirectory, "ThirdParty", "sumo");
        string sumoLibPath = Path.Combine(sumoPath, "Lib");
        string sumoIncludePath = Path.Combine(sumoPath, "Include");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string DLLName = "sumo_c_wrapper.dll";
			string DLLSourcePath = Path.Combine(sumoLibPath, DLLName);
			string DLLDestinationPath = Path.Combine("$(PluginDir)", "Binaries", "Win64", DLLName);

			RuntimeDependencies.Add(DLLDestinationPath, DLLSourcePath, StagedFileType.NonUFS);
			System.Console.WriteLine($"Copying {DLLName} from {DLLSourcePath} to {DLLDestinationPath}");

			// PublicAdditionalLibraries.Add(Path.Combine(sumoLibPath, "sumoInterface.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(sumoLibPath, "sumo_c_wrapper.lib"));

			//dependencies, only copy
			string[] dependencyList = new string[] {
			"libsumocpp.dll",
			"fox-16.dll",
			"iconv-2.dll",
			"intl-8.dll",
			"libbz2.dll",
			"libcrypto-3-x64.dll",
			"libcurl.dll",
			"libpng16.dll",
			"libssh2.dll",
			"libssl-3-x64.dll",
			"proj_9.dll",
			"sqlite3.dll",
			"tiff.dll",
			"xerces-c_3_3.dll",
			"zlib.dll"
			};
			foreach (string name in dependencyList) {
				string dependendySource = Path.Combine(sumoLibPath, name);
				string dependencyDestination = Path.Combine("$(PluginDir)", "Binaries", "Win64", name);
				RuntimeDependencies.Add(dependencyDestination, dependendySource, StagedFileType.NonUFS);
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			// sumo_c_wrapper shared library (built from source)
			PublicAdditionalLibraries.Add(Path.Combine(sumoLibPath, "libsumo_c_wrapper.so"));
			RuntimeDependencies.Add(
				Path.Combine("$(PluginDir)", "Binaries", "Linux", "libsumo_c_wrapper.so"),
				Path.Combine(sumoLibPath, "libsumo_c_wrapper.so"),
				StagedFileType.NonUFS);

			// libtracicpp.so is a transitive dependency loaded at runtime via libsumo_c_wrapper.so
		}
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				sumoIncludePath
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GEMSTAR_Interfaces"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Projects",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
