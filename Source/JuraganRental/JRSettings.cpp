#include "JRSettings.h"

#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "Scalability.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "JRLang.h"

namespace JRSettings
{
	static const TCHAR* Slot = TEXT("JRSettings");

	UJRSettingsSave* Get()
	{
		static TWeakObjectPtr<UJRSettingsSave> Current;
		if (!Current.IsValid())
		{
			UJRSettingsSave* S = Cast<UJRSettingsSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
			if (!S) S = Cast<UJRSettingsSave>(UGameplayStatics::CreateSaveGameObject(UJRSettingsSave::StaticClass()));
			S->AddToRoot();
			Current = S;
		}
		return Current.Get();
	}

	void Save()
	{
		UGameplayStatics::SaveGameToSlot(Get(), Slot, 0);
	}

	static const float UISteps[] = { 1.2f, 1.5f, 1.8f, 2.1f };

	float NextUIScale(float Current)
	{
		for (int32 i = 0; i < UE_ARRAY_COUNT(UISteps); ++i)
		{
			if (FMath::IsNearlyEqual(UISteps[i], Current, 0.05f)) return UISteps[(i + 1) % UE_ARRAY_COUNT(UISteps)];
		}
		return 1.5f;
	}

	static const float JoySteps[] = { 0.7f, 0.85f, 1.f, 1.15f };

	float NextJoystickSize(float Current)
	{
		for (int32 i = 0; i < UE_ARRAY_COUNT(JoySteps); ++i)
		{
			if (FMath::IsNearlyEqual(JoySteps[i], Current, 0.05f)) return JoySteps[(i + 1) % UE_ARRAY_COUNT(JoySteps)];
		}
		return 0.85f;
	}

	FString JoystickSizeName(float Size)
	{
		return Size < 0.78f ? JRT(TEXT("Kecil")) : Size < 0.93f ? JRT(TEXT("Sedang")) : Size < 1.08f ? JRT(TEXT("Besar")) : JRT(TEXT("Sangat besar"));
	}

	FString UIScaleName(float Scale)
	{
		return Scale < 1.35f ? JRT(TEXT("Kecil")) : Scale < 1.65f ? JRT(TEXT("Sedang")) : Scale < 1.95f ? JRT(TEXT("Besar")) : JRT(TEXT("Sangat besar"));
	}

	int32 GfxPreset()
	{
		const int32 P = Get()->GfxPreset;
		return P >= 0 && P <= 2 ? P : 1;
	}

	FString GfxPresetName(int32 Preset)
	{
		switch (Preset)
		{
		case 0: return JRT(TEXT("Hemat (paling lancar)"));
		case 2: return JRT(TEXT("Bagus (HP kencang)"));
		default: return JRT(TEXT("Seimbang"));
		}
	}

	void ApplyGraphics()
	{
		// PC tetap memakai menu grafis Windows; -jrgfx menguji preset HP di PC.
		const bool bMobile = PLATFORM_ANDROID || PLATFORM_IOS || FParse::Param(FCommandLine::Get(), TEXT("jrgfx"));
		if (!bMobile) return;
		// Penyebab utama patah-patah di HP adalah GPU, bukan RAM: layar HP sekarang 1080p ke atas
		// dan setiap piksel diterangi banyak lampu. Gambar 3D dirender lebih kecil lalu diperbesar
		// (teks dan tombol tetap tajam), bayangan dan efek dikurangi sesuai preset.
		const int32 P = GfxPreset();
		static const int32 Quality[] = { 0, 1, 2 };		// tingkat scalability bawaan Unreal
		static const float Percent[] = { 55.f, 72.f, 90.f };	// persen resolusi 3D
		Scalability::FQualityLevels Q = Scalability::GetQualityLevels();
		Q.SetFromSingleQualityLevel(Quality[P]);
		Scalability::SetQualityLevels(Q);
		auto SetCVar = [](const TCHAR* Name, float V)
		{
			if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(Name)) C->Set(V, ECVF_SetByGameSetting);
		};
		SetCVar(TEXT("r.ScreenPercentage"), Percent[P]);
		SetCVar(TEXT("r.BloomQuality"), P == 0 ? 0.f : 3.f);
		SetCVar(TEXT("r.Shadow.MaxCSMResolution"), P == 0 ? 512.f : P == 1 ? 1024.f : 2048.f);
		SetCVar(TEXT("r.Shadow.DistanceScale"), P == 0 ? 0.5f : P == 1 ? 0.75f : 1.f);
		SetCVar(TEXT("t.MaxFPS"), (float)(Get()->MaxFps <= 30 ? 30 : 60));
		UE_LOG(LogTemp, Display, TEXT("JR_GFX preset=%d persen=%.0f fps=%d"), P, Percent[P], Get()->MaxFps);
	}
}
