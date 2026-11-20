#include "JRSettings.h"

#include "Kismet/GameplayStatics.h"
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
}
