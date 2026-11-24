// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JuraganRental : ModuleRules
{
	public JuraganRental(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Tiap file dikompilasi sendiri: warna/fungsi lokal bernama sama di beberapa file tidak bentrok
		// (pengelompokan unity build berbeda antara Development dan Shipping).
		bUseUnity = false;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "ApplicationCore", "RHI" });

		// Android: iklan berhadiah AdMob (Java lewat UPL) + JNI dari JRAds.cpp
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.Add("Launch");
			string Upl = System.IO.Path.Combine(ModuleDirectory, "JuraganRental_UPL.xml");
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Upl);
		}	// RHI: hitung draw call untuk uji performa (-jrperf)	// input device mapper (uji geser layar)

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
