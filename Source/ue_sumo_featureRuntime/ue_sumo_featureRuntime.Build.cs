/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnrealBuildTool;

public class ue_sumo_featureRuntime : ModuleRules
{
	public ue_sumo_featureRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		string sumoPath = Path.Combine(ModuleDirectory, "ThirdParty", "sumo");
		string sumoLibPath = Path.Combine(sumoPath, "Lib");
		string sumoIncludePath = Path.Combine(sumoPath, "Include");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string DLLDestinationDirectory = Target.Type == TargetType.Editor
				? Path.Combine("$(PluginDir)", "Binaries", "Win64")
				: "$(TargetOutputDir)";
			string SumoCLib = Path.Combine(sumoLibPath, "libsumoc.lib");
			if (!File.Exists(SumoCLib))
			{
				throw new BuildException("SUMO C bindings library not found: " + SumoCLib);
			}
			PublicAdditionalLibraries.Add(SumoCLib);

			foreach (string DllSourcePath in Directory.EnumerateFiles(sumoLibPath, "*.dll"))
			{
				string DLLDestinationPath = Path.Combine(DLLDestinationDirectory, Path.GetFileName(DllSourcePath));
				RuntimeDependencies.Add(DLLDestinationPath, DllSourcePath, StagedFileType.NonUFS);
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string SumoSharedLibrary = Path.Combine(sumoLibPath, "libsumoc.so");
			PublicAdditionalLibraries.Add(SumoSharedLibrary);

			// Place the libraries beside the packaged executable. Unreal includes
			// ${ORIGIN} in its Linux RPATH, so the loader can resolve these files
			// without container-specific absolute paths or LD_LIBRARY_PATH.
			string[] RuntimeLibraries =
			{
				"libsumoc.so",
				"libxerces-c-3.2.so",
				"libz.so.1",
				"libicuuc.so.70",
				"libicudata.so.70"
			};
			foreach (string RuntimeLibrary in RuntimeLibraries)
			{
				RuntimeDependencies.Add(Path.Combine("$(TargetOutputDir)", RuntimeLibrary),
					Path.Combine(sumoLibPath, RuntimeLibrary), StagedFileType.NonUFS);
			}
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
				"GameFeatures",
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


		string SourceDir =
			Path.Combine(
				PluginDirectory,
				"Resources",
				"Sumo"
			);

		string DestinationDir =
			Path.Combine(
				Target.ProjectFile.Directory.FullName,
				"Simulation",
				"SUMO"
			);

		CopyDirectoryIfMissing(
			SourceDir,
			DestinationDir
		);

		string SourceIni = Path.Combine(
			PluginDirectory,
			"Resources",
			"GemstarFeature.ini"
		);

		string TargetIni = Path.Combine(
			Target.ProjectFile.Directory.FullName,
			"Simulation",
			"DefaultGemstar.ini"
		);
		MergeMissingIniSections(
			SourceIni,
			TargetIni
		);
	}

	private static void CopyDirectoryIfMissing(
		string SourceDir,
		string DestinationDir)
	{
		if (!Directory.Exists(SourceDir))
		{
			return;
		}

		Directory.CreateDirectory(DestinationDir);

		foreach (string SourceFile in Directory.GetFiles(
			SourceDir,
			"*",
			SearchOption.AllDirectories))
		{
			string RelativePath =
				Path.GetRelativePath(
					SourceDir,
					SourceFile
				);

			string DestinationFile =
				Path.Combine(
					DestinationDir,
					RelativePath
				);

			string DestinationFileDir =
				Path.GetDirectoryName(
					DestinationFile
				);

			if (!Directory.Exists(DestinationFileDir))
			{
				Directory.CreateDirectory(
					DestinationFileDir
				);
			}

			// Important:
			// Never overwrite user-modified files.
			if (!File.Exists(DestinationFile))
			{
				File.Copy(
					SourceFile,
					DestinationFile,
					false
				);
			}
		}
	}


	private static void MergeMissingIniSections(
		string SourceIni,
		string TargetIni)
	{
		if (!File.Exists(SourceIni))
		{
			return;
		}

		string TargetDir = Path.GetDirectoryName(TargetIni);

		if (!Directory.Exists(TargetDir))
		{
			Directory.CreateDirectory(TargetDir);
		}

		if (!File.Exists(TargetIni))
		{
			File.WriteAllText(TargetIni, "", Encoding.UTF8);
		}

		string SourceText = File.ReadAllText(SourceIni, Encoding.UTF8);
		string TargetText = File.ReadAllText(TargetIni, Encoding.UTF8);

		Dictionary<string, string> SourceSections =
			ParseIniSections(SourceText);

		HashSet<string> ExistingTargetSections =
			ParseIniSectionNames(TargetText);

		StringBuilder Output = new StringBuilder(TargetText);

		bool Changed = false;

		foreach (KeyValuePair<string, string> Section in SourceSections)
		{
			string SectionName = Section.Key;

			if (ExistingTargetSections.Contains(SectionName))
			{
				continue;
			}

			if (Output.Length > 0 && !Output.ToString().EndsWith("\n"))
			{
				Output.AppendLine();
			}

			Output.AppendLine();
			Output.Append(Section.Value.TrimEnd());
			Output.AppendLine();

			Changed = true;
		}

		if (Changed)
		{
			File.WriteAllText(
				TargetIni,
				Output.ToString(),
				Encoding.UTF8
			);
		}
	}

	private static Dictionary<string, string> ParseIniSections(
	string IniText)
	{
		Dictionary<string, string> Sections =
			new Dictionary<string, string>();

		using StringReader Reader = new StringReader(IniText);

		string CurrentSectionName = null;
		StringBuilder CurrentSectionText = null;
		string Line;

		while ((Line = Reader.ReadLine()) != null)
		{
			string Trimmed = Line.Trim();

			bool IsNormalSection = Trimmed.StartsWith("[");
			bool IsCommentedSection = Trimmed.StartsWith(";[");

			if ((IsNormalSection || IsCommentedSection)
				&& Trimmed.EndsWith("]"))
			{
				if (CurrentSectionName != null &&
					CurrentSectionText != null)
				{
					Sections[CurrentSectionName] =
						CurrentSectionText.ToString();
				}

				int SectionNameStart = IsCommentedSection ? 2 : 1;

				CurrentSectionName = Trimmed.Substring(
					SectionNameStart,
					Trimmed.Length - SectionNameStart - 1
				);

				CurrentSectionText = new StringBuilder();
				CurrentSectionText.AppendLine(Line);
			}
			else if (CurrentSectionText != null)
			{
				CurrentSectionText.AppendLine(Line);
			}
		}

		if (CurrentSectionName != null &&
			CurrentSectionText != null)
		{
			Sections[CurrentSectionName] =
				CurrentSectionText.ToString();
		}

		return Sections;
	}

	private static HashSet<string> ParseIniSectionNames(
	string IniText)
	{
		HashSet<string> SectionNames =
			new HashSet<string>();

		using StringReader Reader = new StringReader(IniText);

		string Line;

		while ((Line = Reader.ReadLine()) != null)
		{
			string Trimmed = Line.Trim();

			bool IsNormalSection = Trimmed.StartsWith("[");
			bool IsCommentedSection = Trimmed.StartsWith(";[");

			if ((IsNormalSection || IsCommentedSection)
				&& Trimmed.EndsWith("]"))
			{
				int SectionNameStart = IsCommentedSection ? 2 : 1;

				string SectionName = Trimmed.Substring(
					SectionNameStart,
					Trimmed.Length - SectionNameStart - 1
				);

				SectionNames.Add(SectionName);
			}
		}

		return SectionNames;
	}

}
