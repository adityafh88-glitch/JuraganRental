#include "JROpening.h"
#include "JRLang.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "JRBlockout.h"
#include "JRSound.h"

using namespace JRBlockout;

namespace JRLocal_Opening
{
	struct FCamKey { float T; FVector Loc; FVector LookAt; };

	// Rolling door: lebar 212 cm menutup lubang pintu 200 x 220 cm, tepat di luar dinding depan.
	constexpr float ShutW = 212.f, ShutH = 222.f;
	constexpr float DoorUp0 = 7.6f, DoorUp1 = 9.8f;

	float Smooth(float A) { A = FMath::Clamp(A, 0.f, 1.f); return A * A * (3.f - 2.f * A); }
}
using namespace JRLocal_Opening;

AJROpening::AJROpening()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(Root);
	Cam->SetFieldOfView(78.f);
}

void AJROpening::Start(float HalfX, float DoorY)
{
	HX = HalfX;
	Door = DoorY;
	// Rolling door tertutup (abu logam) dengan alur-alur mendatar.
	const float X = -(HX + 22.f);
	Shutter = AddPart(this, Root, EShape::Cube, FVector(4, ShutW, ShutH), FVector(X, Door, ShutH / 2), Hex(0x8d949c), false);
	for (float Z = 18.f; Z < ShutH; Z += 18.f)
	{
		Grooves.Add(AddPart(this, Root, EShape::Cube, FVector(2, ShutW - 6.f, 3), FVector(X - 3.f, Door, Z), Hex(0x5f666e), false));
	}
	Grooves.Add(AddPart(this, Root, EShape::Cube, FVector(4, 40, 6), FVector(X - 4.f, Door, 14.f), Hex(0x3a3f45), false));	// pegangan
	PlaceCamera();
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTargetWithBlend(this, 0.f);
}

void AJROpening::PlaceCamera()
{
	const FVector D(-(HX + 20.f), Door, 0.f);	// muka pintu dari luar
	const FCamKey Keys[] = {
		{ 0.0f,  D + FVector(-950.f, -650.f, 240.f), D + FVector(0.f, 0.f, 170.f) },	// seberang jalan, agak ke kiri
		{ 4.0f,  D + FVector(-720.f, -180.f, 200.f), D + FVector(0.f, 0.f, 140.f) },
		{ 7.4f,  D + FVector(-330.f, 0.f, 170.f),    D + FVector(0.f, 0.f, 125.f) },	// tepat di depan rolling door
		{ 9.8f,  D + FVector(-300.f, 0.f, 168.f),    D + FVector(0.f, 0.f, 120.f) },
		{ 11.6f, D + FVector(140.f, -40.f, 160.f),   FVector(HX - 80.f, -500.f, 110.f) },	// masuk garasi
		{ 14.5f, FVector(HX - 520.f, -380.f, 150.f), FVector(HX - 80.f, -680.f, 85.f) },	// dekat deretan TV tabung
	};
	const int32 N = UE_ARRAY_COUNT(Keys);
	int32 i = 0;
	while (i < N - 2 && Time > Keys[i + 1].T) ++i;
	const float A = Smooth((Time - Keys[i].T) / FMath::Max(Keys[i + 1].T - Keys[i].T, 0.01f));
	const FVector Loc = FMath::Lerp(Keys[i].Loc, Keys[i + 1].Loc, A);
	const FVector Look = FMath::Lerp(Keys[i].LookAt, Keys[i + 1].LookAt, A);
	Cam->SetWorldLocationAndRotation(Loc, (Look - Loc).Rotation());
}

void AJROpening::UpdateShutter()
{
	if (!Shutter) return;
	// Gulungan naik: tinggi berkurang dari bawah, tepi atas tetap.
	const float H = ShutH * (1.f - Smooth((Time - DoorUp0) / (DoorUp1 - DoorUp0)));
	const float Bottom = ShutH - H;
	Shutter->SetVisibility(H > 1.f);
	FVector S = Shutter->GetRelativeScale3D();
	S.Z = FMath::Max(H, 0.01f) / 100.f;
	Shutter->SetRelativeScale3D(S);
	FVector L = Shutter->GetRelativeLocation();
	L.Z = Bottom + H / 2.f;
	Shutter->SetRelativeLocation(L);
	for (UStaticMeshComponent* G : Grooves) if (G) G->SetVisibility(G->GetRelativeLocation().Z > Bottom + 2.f);
}

void AJROpening::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bFinished) return;
	Time += FMath::Min(DeltaSeconds, 0.05f);
	// Uji: -jropeningat=detik membekukan adegan (untuk dipotret)
	static float FreezeAt = -1.f;
	static const bool bFreeze = FParse::Value(FCommandLine::Get(), TEXT("jropeningat="), FreezeAt);
	if (bFreeze) Time = FreezeAt;

	auto Once = [this](int32 Bit, float At, const TCHAR* Sfx, float Vol, const FVector* Where)
	{
		if (Time < At || (SoundsPlayed & (1 << Bit))) return;
		SoundsPlayed |= 1 << Bit;
		if (Where) JRSound::PlayAt(this, Sfx, *Where, Vol); else JRSound::Play2D(this, Sfx, Vol);
	};
	const FVector Street(-(HX + 600.f), Door - 300.f, 80.f);
	const FVector DoorPos(-(HX + 20.f), Door, 110.f);
	const FVector Tv(HX - 80.f, -550.f, 100.f);
	Once(0, 0.8f, TEXT("SFX_Motor_Lewat"), 1.f, &Street);
	Once(1, DoorUp0, TEXT("SFX_Kardus"), 0.9f, &DoorPos);
	Once(2, DoorUp0 + 0.9f, TEXT("SFX_Kardus"), 0.7f, &DoorPos);
	Once(3, DoorUp1, TEXT("SFX_Pasang"), 0.9f, &DoorPos);
	Once(4, 11.9f, TEXT("SFX_TV_Nyala"), 0.8f, &Tv);
	Once(5, 12.4f, TEXT("SFX_Konsol_Boot"), 0.8f, &Tv);

	PlaceCamera();
	UpdateShutter();
	// -jrcapture (dengan -jropening -benchmark -fps=30): simpan setiap frame viewport tanpa HUD ke Saved/Capture,
	// untuk bahan video promosi. Game keluar sendiri setelah adegan selesai.
	static const bool bCapture = FParse::Param(FCommandLine::Get(), TEXT("jrcapture"));
	if (bCapture && GEngine)
	{
		static int32 Frame = 0;
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Capture/op_%05d.png"), Frame++));
		FScreenshotRequest::RequestScreenshot(File, false, false);	// tangkap viewport apa adanya: riwayat Lumen/TSR utuh, tidak berbintik
		if (Time >= Duration) { UE_LOG(LogTemp, Display, TEXT("JR_CAPTURE frames=%d"), Frame); FGenericPlatformMisc::RequestExit(false); }
	}
	if (!bFreeze && !bCapture && Time >= Duration) Finish();
}

FString AJROpening::Caption() const
{
	if (Time < 0.6f) return FString();
	if (Time < 4.0f) return JRT(TEXT("Pagi di sebuah gang kampung..."));
	if (Time < 7.5f) return JRT(TEXT("Emak: \"Daripada garasinya nganggur, bikin rental PS aja sana!\""));
	if (Time < 10.8f) return JRT(TEXT("Modal awal: dua TV tabung, satu PC bekas, dan MCB 1.300 VA."));
	return JRT(TEXT("Mimpi kecil dimulai: jadi JURAGAN RENTAL!"));
}

float AJROpening::CaptionAlpha() const
{
	// Tiap teks muncul pelan (0,4 dtk) setelah berganti.
	static const float Starts[] = { 0.6f, 4.0f, 7.5f, 10.8f };
	float Since = 99.f;
	for (float S : Starts) if (Time >= S) Since = Time - S;
	return FMath::Clamp(Since / 0.4f, 0.f, 1.f);
}

float AJROpening::Fade() const
{
	return FMath::Max(1.f - Time / 0.8f, Smooth((Time - (Duration - 0.7f)) / 0.7f));
}

void AJROpening::Finish()
{
	if (bFinished) return;
	bFinished = true;
	UE_LOG(LogTemp, Display, TEXT("JR_OPENING selesai t=%.1f"), Time);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* P = PC->GetPawn()) PC->SetViewTargetWithBlend(P, 0.f);
	}
	Destroy();
}
