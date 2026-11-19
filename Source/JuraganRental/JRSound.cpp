#include "JRSound.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "TimerManager.h"

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
			A->Attenuation.AttenuationShapeExtents = FVector(200.f);
			A->Attenuation.FalloffDistance = 700.f;
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
		USoundBase* S = Find(Name);
		static int32 Logged = 0;
		if (Logged < 12)
		{
			Logged++;
			UE_LOG(LogTemp, Display, TEXT("JR_SND %s %s"), Name, S ? TEXT("ok") : TEXT("TIDAK ADA"));
		}
		if (S) UGameplayStatics::PlaySoundAtLocation(Context, S, Location, Volume, FMath::FRandRange(0.95f, 1.05f), 0.f, NearAttenuation());
	}

	// Kapan "mulut" bebas lagi (waktu dunia). Disimpan per dunia agar tidak terbawa antar sesi main.
	static TWeakObjectPtr<UWorld> VoiceWorld;
	static float VoiceFreeAt = 0.f;
	static constexpr float VoiceGap = 0.5f;

	bool Voice(const UObject* Context, const FString& Char, const TCHAR* Situasi, const FVector& Location, bool bPriority)
	{
		UWorld* World = Context ? Context->GetWorld() : nullptr;
		if (!World) return false;
		if (VoiceWorld.Get() != World) { VoiceWorld = World; VoiceFreeAt = 0.f; }
		const float Now = World->GetTimeSeconds();
		const bool bBusy = Now < VoiceFreeAt;
		if (bBusy && !bPriority) return false;

		TArray<USoundBase*> Options;
		for (int32 i = 1; i <= 3; ++i)
		{
			if (USoundBase* S = Find(FString::Printf(TEXT("VO_%s_%s_%d"), *Char, Situasi, i))) Options.Add(S);
		}
		if (Options.Num() == 0) return false;
		// Nada per karakter: Bocil disuarakan pengisi dewasa, nadanya dinaikkan agar terdengar lebih muda.
		const float Pitch = Char == TEXT("Bocil") ? 1.15f : 1.f;
		USoundBase* Pick = Options[FMath::RandRange(0, Options.Num() - 1)];
		const FVector At = Location + FVector(0, 0, 150);
		const float Delay = bBusy ? VoiceFreeAt - Now : 0.f;
		VoiceFreeAt = Now + Delay + Pick->GetDuration() / Pitch + VoiceGap;
		if (Delay <= 0.f)
		{
			UGameplayStatics::PlaySoundAtLocation(World, Pick, At, 1.f, Pitch, 0.f, Attenuation());
		}
		else
		{
			// Giliran bicara: putar setelah suara sebelumnya selesai.
			TWeakObjectPtr<UWorld> WeakWorld = World;
			TWeakObjectPtr<USoundBase> WeakSound = Pick;
			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakWorld, WeakSound, At, Pitch]()
			{
				if (WeakWorld.IsValid() && WeakSound.IsValid())
				{
					UGameplayStatics::PlaySoundAtLocation(WeakWorld.Get(), WeakSound.Get(), At, 1.f, Pitch, 0.f, Attenuation());
				}
			}), Delay, false);
		}
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
