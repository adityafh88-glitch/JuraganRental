#include "JRRadio.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "JRBlockout.h"
#include "JRGameMode.h"
#include "JRSettings.h"
#include "JRSound.h"

using namespace JRBlockout;

AJRRadio::AJRRadio()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Music = CreateDefaultSubobject<UAudioComponent>(TEXT("Music"));
	Music->SetupAttachment(Root);
	Music->SetRelativeLocation(FVector(0, 0, 15));
	Music->bAutoActivate = false;
}

void AJRRadio::BeginPlay()
{
	Super::BeginPlay();
	AddProp(this, Root, TEXT("SM_Radio"), FVector::ZeroVector, 0.f);

	// Terdengar jelas di sekitar kasir, sayup-sayup di ujung ruangan.
	USoundAttenuation* Att = NewObject<USoundAttenuation>(this);
	Att->Attenuation.bAttenuate = true;
	Att->Attenuation.bSpatialize = true;
	Att->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	Att->Attenuation.AttenuationShapeExtents = FVector(350.f);
	Att->Attenuation.FalloffDistance = 2200.f;
	Music->AttenuationSettings = Att;
	Music->OnAudioFinished.AddDynamic(this, &AJRRadio::OnTrackFinished);

	for (int32 i = 1; i <= 12; ++i)
	{
		if (USoundBase* S = JRSound::Find(FString::Printf(TEXT("MUS_Radio_%d"), i))) Tracks.Add(S);
	}
	if (Tracks.Num() > 0 && CanPlay()) PlayTrack(FMath::RandRange(0, Tracks.Num() - 1));
}

bool AJRRadio::CanPlay() const
{
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	return JRSettings::Get()->bMusic && !(GM && GM->bMcbTripped);
}

void AJRRadio::PlayTrack(int32 Index)
{
	if (Tracks.Num() == 0) return;
	Track = (Index % Tracks.Num() + Tracks.Num()) % Tracks.Num();
	bSwitching = true;
	Music->SetSound(Tracks[Track]);
	Music->SetVolumeMultiplier(BaseVolume * Duck);
	Music->Play();
	bSwitching = false;
}

void AJRRadio::OnTrackFinished()
{
	// Lagu habis dengan sendirinya: lanjut ke lagu berikutnya (bukan saat dimatikan).
	if (!bSwitching && bOn && CanPlay() && !Music->IsPlaying()) PlayTrack(Track + 1);
}

void AJRRadio::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Tracks.Num() == 0) return;

	const bool bWant = bOn && CanPlay();
	if (!bWant && Music->IsPlaying()) Music->Stop();
	else if (bWant && !Music->IsPlaying()) PlayTrack(Track);	// listrik kembali / musik dinyalakan di Atur

	// Ducking: kecilkan saat ada yang bicara agar suara karakter tetap jelas.
	const float Target = GetWorld()->GetTimeSeconds() < JRSound::VoiceBusyUntil(GetWorld()) ? 0.35f : 1.f;
	Duck = FMath::FInterpTo(Duck, Target, DeltaSeconds, 4.f);
	if (Music->IsPlaying()) Music->SetVolumeMultiplier(BaseVolume * Duck);
}

FJRInteraction AJRRadio::GetInteraction() const
{
	FJRInteraction Out;
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (Tracks.Num() == 0)
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("Radio belum ada lagunya");
	}
	else if (!JRSettings::Get()->bMusic)
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("Musik dimatikan (HP > Atur)");
	}
	else if (GM && GM->bMcbTripped)
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("Listrik mati, radio diam");
	}
	else if (!bOn)
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = TEXT("Nyalakan radio");
	}
	else if (Track < Tracks.Num() - 1)
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = FString::Printf(TEXT("Ganti lagu  (lagu %d dari %d)"), Track + 1, Tracks.Num());
	}
	else
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = FString::Printf(TEXT("Matikan radio  (lagu %d dari %d)"), Track + 1, Tracks.Num());
	}
	return Out;
}

void AJRRadio::Interact()
{
	if (Tracks.Num() == 0 || !CanPlay()) return;
	JRSound::Play2D(this, TEXT("SFX_HP_Tombol"), 0.6f);
	if (!bOn)
	{
		bOn = true;
		PlayTrack(0);
	}
	else if (Track < Tracks.Num() - 1)
	{
		PlayTrack(Track + 1);
	}
	else
	{
		bOn = false;
		Music->Stop();
		Track = 0;
	}
}
