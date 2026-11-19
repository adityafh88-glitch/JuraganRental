#include "JRSettings.h"

#include "Kismet/GameplayStatics.h"

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
}
