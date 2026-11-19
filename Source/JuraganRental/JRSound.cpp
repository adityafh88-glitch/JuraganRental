#include "JRSound.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

namespace JRSound
{
	static TMap<FString, TWeakObjectPtr<USoundBase>> Cache;
	static TSet<FString> Missing;

	static USoundAttenuation* Attenuation()
	{
		static TWeakObjectPtr<USoundAttenuation> Att;
		if (!Att.IsValid())
		{
			USoundAttenuation* A = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("JR_Attenuation"));
			A->AddToRoot();
			A->Attenuation.bAttenuate = true;
			A->Attenuation.bSpatialize = true;
			A->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
			A->Attenuation.AttenuationShapeExtents = FVector(250.f);
			A->Attenuation.FalloffDistance = 1600.f;
			Att = A;
		}
		return Att.Get();
	}

	static USoundAttenuation* NearAttenuation()
	{
		static TWeakObjectPtr<USoundAttenuation> Att;
		if (!Att.IsValid())
		{
			USoundAttenuation* A = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("JR_AttenuationNear"));
			A->AddToRoot();
			A->Attenuation.bAttenuate = true;
			A->Attenuation.bSpatialize = true;
			A->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
			A->Attenuation.AttenuationShapeExtents = FVector(120.f);
			A->Attenuation.FalloffDistance = 480.f;
			Att = A;
		}
		return Att.Get();
	}

	USoundBase* Find(const FString& Name)
	{
		if (Missing.Contains(Name)) return nullptr;
		if (TWeakObjectPtr<USoundBase>* Hit = Cache.Find(Name))
		{
			if (Hit->IsValid()) return Hit->Get();
		}
		const FString Folder = Name.StartsWith(TEXT("VO_")) ? TEXT("/Game/Audio/Voice/") : TEXT("/Game/Audio/");
		const FString Path = Folder + Name + TEXT(".") + Name;
		USoundBase* S = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet));
		if (S) Cache.Add(Name, S);
		else Missing.Add(Name);
		return S;
	}

	void Play2D(const UObject* Context, const TCHAR* Name, float Volume)
	{
		if (USoundBase* S = Find(Name)) UGameplayStatics::PlaySound2D(Context, S, Volume);
	}

	void PlayAt(const UObject* Context, const TCHAR* Name, const FVector& Location, float Volume)
	{
		if (USoundBase* S = Find(Name)) UGameplayStatics::PlaySoundAtLocation(Context, S, Location, Volume, 1.f, 0.f, Attenuation());
	}

	void PlayNear(const UObject* Context, const TCHAR* Name, const FVector& Location, float Volume)
	{
		if (USoundBase* S = Find(Name)) UGameplayStatics::PlaySoundAtLocation(Context, S, Location, Volume, FMath::FRandRange(0.95f, 1.05f), 0.f, NearAttenuation());
	}

	bool Voice(const UObject* Context, const FString& Char, const TCHAR* Situasi, const FVector& Location)
	{
		TArray<USoundBase*> Options;
		for (int32 i = 1; i <= 3; ++i)
		{
			if (USoundBase* S = Find(FString::Printf(TEXT("VO_%s_%s_%d"), *Char, Situasi, i))) Options.Add(S);
		}
		if (Options.Num() == 0) return false;
		// Nada per karakter: Bocil disuarakan pengisi dewasa, nadanya dinaikkan agar terdengar lebih muda.
		const float Pitch = Char == TEXT("Bocil") ? 1.15f : 1.f;
		UGameplayStatics::PlaySoundAtLocation(Context, Options[FMath::RandRange(0, Options.Num() - 1)], Location + FVector(0, 0, 150), 1.f, Pitch, 0.f, Attenuation());
		return true;
	}

	UAudioComponent* MakeLoop(USceneComponent* Parent, const TCHAR* Name, float Volume, bool b3D)
	{
		USoundBase* S = Find(Name);
		if (!S || !Parent) return nullptr;
		UAudioComponent* A = NewObject<UAudioComponent>(Parent->GetOwner());
		A->SetupAttachment(Parent);
		A->SetSound(S);
		A->SetVolumeMultiplier(Volume);
		A->bAutoActivate = false;
		if (b3D) A->AttenuationSettings = Attenuation();
		else A->bAllowSpatialization = false;
		A->RegisterComponent();
		return A;
	}
}
