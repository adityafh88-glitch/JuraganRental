// Rekaman video promosi (Threads/IG) dengan "sutradara" otomatis.
// Jalankan: -jrpromo=N -jrcapture -jrindo -benchmark -fps=30 -nosound -ExecCmds="r.SetRes 1280x720w, r.AntiAliasingMethod 4"
// Frame disimpan ke Saved/Capture/promoN_00000.png; isyarat suara ke Saved/Capture/promoN_cues.txt ("frame nama").
// Suara ditempel belakangan oleh Tools/make_promo.ps1 (rekaman berjalan tanpa audio, langkah waktu tetap).

#include "JRGameMode.h"
#include "JRLang.h"
#include "JRCustomer.h"
#include "JRStation.h"
#include "JRFixtures.h"
#include "JRHUD.h"
#include "JRSettings.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "HighResScreenshot.h"
#include "Engine/Engine.h"

namespace JRLocal_Promo
{
	constexpr int32 TBocil = 0, TSma = 1, TMahasiswa = 2, TKantoran = 3, TGamer = 4, TBapak = 5;
	float Smooth(float A) { A = FMath::Clamp(A, 0.f, 1.f); return A * A * (3.f - 2.f * A); }
}
using namespace JRLocal_Promo;

void AJRGameMode::PromoSetup()
{
	// Kondisi awal tiap skenario (sebelum dunia dibangun).
	Hour = 16.f;
	bNeedName = false;
	switch (PromoId)
	{
	case 2: Era = 1; CapVA = 1300; OwnedPS = 4; OwnedPC = 4; bPS2 = false; Money = 250000.f; Rep = 35.f; break;	// MCB jeglek
	case 3: Era = 1; CapVA = 2200; OwnedPS = 4; OwnedPC = 2; Money = 180000.f; Rep = 40.f; break;				// tiga momen
	case 4: Era = 1; CapVA = 1300; OwnedPS = 4; OwnedPC = 4; break;												// tur tiga era
	case 5: Era = 1; CapVA = 2200; OwnedPS = 4; OwnedPC = 2; Money = 320000.f; Rep = 48.f;						// Kartu Momen
		RentalName = JRT(TEXT("Sejam Lagi")); break;
	case 7: Era = 2; CapVA = 4400; OwnedPS = 6; OwnedPC = 4; bPS2 = true; Money = 900000.f; Rep = 58.f; Hour = 15.9f;	// turnamen PS
		RentalName = JRT(TEXT("Sejam Lagi")); break;
	case 8: Era = 2; CapVA = 4400; OwnedPS = 6; OwnedPC = 4; bPS2 = true; Money = 750000.f; Rep = 55.f; Hour = 22.1f;	// Pak RT datang
		RentalName = JRT(TEXT("Sejam Lagi")); break;
	case 6: Era = 1; CapVA = 4400; OwnedPS = 4; OwnedPC = 4; Money = 450000.f; Rep = 52.f; bPS2 = true;		// foto tiga era
		RentalName = JRT(TEXT("Sejam Lagi")); Decor = { TEXT("poster"), TEXT("tanaman"), TEXT("kipas"), TEXT("neon") }; break;
	default: break;
	}
}

void AJRGameMode::PromoCue(const FString& Sound)
{
	if (bPromoRec) PromoCues.Add(FString::Printf(TEXT("%d %s"), PromoFrame, *Sound));
}

void AJRGameMode::TickPromo(float DeltaSeconds)
{
	const float Dt = DeltaSeconds;
	const float T0 = PromoT;
	PromoT += Dt;
	auto At = [&](float T) { return PromoT >= T && T0 < T; };
	UWorld* W = GetWorld();
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr;
	const FJRLayout& L = Layout();

	// Kamera bebas + pemain disembunyikan
	if (!PromoCam && PC)
	{
		FActorSpawnParameters SP;
		PromoCam = W->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, SP);
		PromoCam->GetCameraComponent()->SetConstraintAspectRatio(false);
		PromoCam->GetCameraComponent()->SetFieldOfView(74.f);
	}
	if (PC && PromoCam && PC->GetViewTarget() != PromoCam) PC->SetViewTarget(PromoCam);
	if (ACharacter* Pw = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr)
	{
		Pw->SetActorHiddenInGame(true);
		Pw->SetActorEnableCollision(false);
		Pw->GetCharacterMovement()->DisableMovement();
		Pw->SetActorLocation(FVector(-(L.HalfX + 3000.f), 0.f, 2000.f));	// jauh dari interaksi apa pun
	}
	auto Cam = [&](const FVector& Loc, const FVector& Look, float Fov = 74.f)
	{
		if (!PromoCam) return;
		PromoCam->SetActorLocationAndRotation(Loc, (Look - Loc).Rotation());
		PromoCam->GetCameraComponent()->SetFieldOfView(Fov);
	};
	auto CamMove = [&](float Ta, float Tb, const FVector& A, const FVector& B, const FVector& LA, const FVector& LB, float Fov = 74.f)
	{
		const float K = Smooth((PromoT - Ta) / FMath::Max(Tb - Ta, 0.01f));
		Cam(FMath::Lerp(A, B, K), FMath::Lerp(LA, LB, K), Fov);
	};
	auto Unit = [&](EJRUnitType Ty, int32 I) -> AJRStation*
	{
		for (AJRStation* S : Stations) if (S && S->Type == Ty && S->Index == I) return S;
		return nullptr;
	};
	auto ShowHud = [&](bool bShow) { if (Hud) Hud->bShowHUD = bShow; };
	JRSettings::Get()->bMissionsCompact = true;	// panel misi diringkas agar gambar lega (tidak disimpan)
	auto Rec = [&](bool bOn) { bPromoRec = bOn; };
	auto Finish = [&]()
	{
		bPromoRec = false;
		FFileHelper::SaveStringArrayToFile(PromoCues, *(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Capture/promo%d_cues.txt"), PromoId)));
		UE_LOG(LogTemp, Display, TEXT("JR_PROMO done id=%d frames=%d cues=%d"), PromoId, PromoFrame, PromoCues.Num());
		FGenericPlatformMisc::RequestExit(false);
	};
	auto Hold = [&](float Secs) { PromoHeld = MoveTemp(Choices); Choices.Reset(); PromoHeldUntil = PromoT + Secs; };
	if (PromoHeldUntil > 0.f && PromoT >= PromoHeldUntil) { Choices = MoveTemp(PromoHeld); PromoHeld.Reset(); PromoHeldUntil = -1.f; }
	auto Seat = [&](EJRUnitType Ty, int32 I, int32 Type) { return PromoSeat(Unit(Ty, I), Type); };
	auto On = [&](EJRUnitType Ty, int32 I, const TCHAR* Sfx)
	{
		if (PromoStart(Unit(Ty, I)) && Sfx) PromoCue(Sfx);
	};

	// V = waktu video (detik) bila rekaman dimulai pada PromoT = Rs
	switch (PromoId)
	{
	// ---------------- Video 2: daya lewat 1.300 VA, MCB jeglek ----------------
	case 2:
	{
		constexpr float Rs = 3.f;
		const FVector C0(-60.f, -120.f, 235.f), LookTv(600.f, -430.f, 70.f), LookPc(150.f, 930.f, 70.f);
		const FVector M = L.Mcb();
		if (At(0.2f)) { for (int32 i = 0; i < 4; ++i) Seat(EJRUnitType::PS, i, i == 0 ? TBocil : i == 3 ? TBapak : TSma); for (int32 i = 0; i < 4; ++i) Seat(EJRUnitType::PC, i, i % 2 ? TGamer : TMahasiswa); }
		if (PromoT < Rs + 2.4f) CamMove(Rs, Rs + 2.4f, C0, C0 + FVector(40.f, 0.f, 0.f), LookTv, LookTv, 70.f);
		else if (PromoT < Rs + 6.5f) CamMove(Rs + 2.4f, Rs + 3.0f, C0 + FVector(40.f, 0.f, 0.f), C0 + FVector(40.f, 0.f, 0.f), LookTv, LookPc, 70.f);
		else Cam(M + FVector(-70.f, 175.f, 25.f), M + FVector(0.f, 0.f, -5.f), 55.f);
		if (At(Rs)) { Toasts.Reset(); Rec(true); }
		if (At(Rs + 0.3f)) On(EJRUnitType::PS, 0, TEXT("SFX_Konsol_Boot"));
		if (At(Rs + 0.9f)) On(EJRUnitType::PS, 1, TEXT("SFX_Konsol_Boot"));
		if (At(Rs + 1.5f)) On(EJRUnitType::PS, 2, TEXT("SFX_Konsol_Boot"));
		if (At(Rs + 2.1f)) On(EJRUnitType::PS, 3, TEXT("SFX_Konsol_Boot"));
		if (At(Rs + 2.8f)) On(EJRUnitType::PC, 0, TEXT("SFX_TV_Nyala"));
		if (At(Rs + 3.5f)) On(EJRUnitType::PC, 1, TEXT("SFX_TV_Nyala"));
		if (At(Rs + 4.2f)) { On(EJRUnitType::PC, 2, TEXT("SFX_TV_Nyala")); ComputePower(); }
		if (At(Rs + 4.25f) && bMcbTripped) PromoCue(TEXT("SFX_MCB_Turun"));
		if (bMcbTripped && Room) Room->SetLightsOn(false);
		if (At(Rs + 8.0f)) { if (AJRStation* P3 = Unit(EJRUnitType::PC, 2)) if (P3->Customer) EndSession(P3->Customer); ResetMcb(); PromoCue(TEXT("SFX_MCB_Naik")); if (Room) Room->SetLightsOn(true); }
		if (At(Rs + 10.6f)) Finish();
		break;
	}
	// ---------------- Video 3: nambah sejam, emak datang, rage quit ----------------
	case 3:
	{
		constexpr float Rs = 3.f;
		if (At(0.2f)) { Seat(EJRUnitType::PS, 0, TBocil); Seat(EJRUnitType::PS, 1, TSma); Seat(EJRUnitType::PS, 2, TBapak); Seat(EJRUnitType::PC, 0, TMahasiswa); Seat(EJRUnitType::PC, 1, TGamer); }
		if (At(1.0f)) { for (int32 i = 0; i < 3; ++i) On(EJRUnitType::PS, i, nullptr); On(EJRUnitType::PC, 0, nullptr); On(EJRUnitType::PC, 1, nullptr); }
		AJRStation* S1 = Unit(EJRUnitType::PS, 0);
		AJRStation* S2 = Unit(EJRUnitType::PS, 1);
		AJRStation* S3 = Unit(EJRUnitType::PS, 2);
		const FVector P1 = S1 ? S1->GetActorLocation() : FVector::ZeroVector;
		const FVector P2 = S2 ? S2->GetActorLocation() : FVector::ZeroVector;
		const FVector P3 = S3 ? S3->GetActorLocation() : FVector::ZeroVector;
		// Momen A (V 0-4): "NAMBAH SEJAM?"
		if (PromoT < Rs + 4.f) CamMove(Rs, Rs + 4.f, P2 + FVector(-420.f, 250.f, 150.f), P2 + FVector(-360.f, 200.f, 135.f), P2 + FVector(-120.f, 0.f, 70.f), P2 + FVector(-120.f, 0.f, 75.f), 66.f);
		if (At(Rs)) { Toasts.Reset(); Rec(true); }
		if (At(Rs + 0.4f) && S2 && S2->Customer)
		{
			AJRCustomer* C = S2->Customer;
			C->bAskExtend = true; C->bAskedExtend = true; C->ExtendLeft = 99.f;
			AddToast(JRPrintf(JRT(TEXT("%s di %s: \"Bang, nambah sejam!\"")), *C->DisplayName, *S2->DisplayName), FLinearColor(1.f, 0.76f, 0.29f));
			PromoCue(TEXT("SFX_Notif"));
		}
		if (At(Rs + 2.9f) && S2) { ExtendSession(S2); PromoCue(TEXT("SFX_Kaching")); }
		// Momen B (V 4-8): Emak datang
		const FVector DoorIn(-(L.HalfX - 90.f), L.DoorY, 110.f);
		if (PromoT >= Rs + 4.f && PromoT < Rs + 8.f) CamMove(Rs + 4.f, Rs + 8.f, DoorIn + FVector(620.f, -560.f, 80.f), DoorIn + FVector(520.f, -470.f, 70.f), DoorIn, DoorIn + FVector(0.f, 0.f, 5.f), 62.f);
		if (At(Rs + 4.f)) Toasts.Reset();
		if (At(Rs + 4.3f) && S1 && S1->Customer) { EmakEvent(S1->Customer); Hold(1.7f); PromoCue(TEXT("VO_Emak_Datang_1")); }
		// Momen C (V 8-12.5): rage quit banting stik
		if (PromoT >= Rs + 8.f) CamMove(Rs + 8.f, Rs + 12.5f, P3 + FVector(-430.f, 280.f, 150.f), P3 + FVector(-380.f, 230.f, 140.f), P3 + FVector(-140.f, 0.f, 70.f), P3 + FVector(-140.f, 0.f, 70.f), 66.f);
		if (At(Rs + 8.f)) { Choices.Reset(); PromoHeld.Reset(); PromoHeldUntil = -1.f; Toasts.Reset(); }
		if (At(Rs + 8.4f) && S3 && S3->Customer) { RageQuitEvent(S3->Customer); Hold(1.7f); PromoCue(TEXT("SFX_Stik_Banting")); PromoCue(TEXT("VO_Bapak_Rage_1")); }
		if (At(Rs + 12.5f)) Finish();
		break;
	}
	// ---------------- Video 4: tiga tampak depan, naik ke VIP Lounge, TV 1 berkedip ----------------
	case 4:
	{
		ShowHud(false);
		const FVector Front(-1980.f, 560.f, 430.f), FrontLook(-700.f, 600.f, 420.f);
		constexpr float FrontFov = 88.f;
		// Era 1
		if (At(0.2f)) { Seat(EJRUnitType::PS, 0, TSma); Seat(EJRUnitType::PC, 0, TMahasiswa); }
		if (PromoT < 4.5f) CamMove(3.f, 4.4f, Front, Front + FVector(40.f, 0.f, 0.f), FrontLook, FrontLook, FrontFov);
		if (At(3.f)) Rec(true);
		if (At(4.4f)) { Rec(false); MoveToEra(2); Toasts.Reset(); }
		// Era 2
		if (PromoT >= 4.4f && PromoT < 8.f) CamMove(6.5f, 7.9f, Front, Front + FVector(40.f, 0.f, 0.f), FrontLook, FrontLook, FrontFov);
		if (At(6.5f)) Rec(true);
		if (At(7.9f))
		{
			Rec(false);
			OwnedPS = 6; OwnedPC = 5; bPS2 = true;
			MoveToEra(3);
			while (Count(EJRUnitType::PS) < 6) SpawnStation(EJRUnitType::PS);
			while (Count(EJRUnitType::PC) < 5) SpawnStation(EJRUnitType::PC);
			while (Count(EJRUnitType::VIP) < 3) SpawnStation(EJRUnitType::VIP);
			Toasts.Reset();
		}
		if (At(8.2f))
		{
			for (int32 i = 1; i < 6; ++i) Seat(EJRUnitType::PS, i, i % 3 == 0 ? TBapak : i % 2 ? TSma : TBocil);
			for (int32 i = 0; i < 5; ++i) Seat(EJRUnitType::PC, i, i % 2 ? TGamer : TMahasiswa);
			for (int32 i = 0; i < 3; ++i) Seat(EJRUnitType::VIP, i, i == 1 ? TKantoran : TGamer);
		}
		if (At(9.0f)) { for (int32 i = 1; i < 6; ++i) On(EJRUnitType::PS, i, nullptr); for (int32 i = 0; i < 5; ++i) On(EJRUnitType::PC, i, nullptr); for (int32 i = 0; i < 3; ++i) On(EJRUnitType::VIP, i, nullptr); }
		// Era 3 tampak depan, lalu masuk dan naik tangga
		const float E3 = 11.f;	// mulai rekam Era 3
		if (PromoT >= 7.9f && PromoT < E3 + 1.6f) CamMove(E3, E3 + 1.6f, Front, Front + FVector(60.f, 0.f, 0.f), FrontLook, FrontLook, FrontFov);
		if (At(E3)) Rec(true);
		const float FH = L.FloorH;
		const FVector P0(-760.f, 380.f, 175.f), P1(-120.f, -1140.f, 175.f), P2(780.f, -1170.f, FH + 175.f), P3(300.f, -520.f, FH + 185.f);
		const FVector Stair(60.f, -1220.f, 140.f), Up(880.f, -1220.f, FH + 150.f), Lounge(894.f, 120.f, FH + 200.f);
		if (PromoT >= E3 + 1.6f && PromoT < E3 + 3.4f) CamMove(E3 + 1.6f, E3 + 3.4f, P0, P1, Stair, Stair + FVector(300.f, 0.f, 60.f));
		else if (PromoT >= E3 + 3.4f && PromoT < E3 + 5.6f)
		{
			// Naik tangga: pandangan lurus ke depan searah anak tangga, lalu menoleh ke lounge di puncak
			const float K = Smooth((PromoT - (E3 + 3.4f)) / 2.2f);
			const FVector Pos = FMath::Lerp(P1, P2, K);
			const FVector Ahead = Pos + FVector(400.f, 0.f, 110.f * (1.f - K));
			Cam(Pos, FMath::Lerp(Ahead, Up + FVector(0.f, 700.f, 0.f), FMath::Clamp((K - 0.7f) / 0.3f, 0.f, 1.f)));
		}
		else if (PromoT >= E3 + 5.6f && PromoT < E3 + 7.8f) CamMove(E3 + 5.6f, E3 + 7.8f, P2, P3, Up + FVector(0.f, 700.f, 0.f), Lounge);
		// TV 1 berkedip (1,2 dtk)
		AJRStation* Tv1 = Unit(EJRUnitType::PS, 0);
		if (PromoT >= E3 + 7.8f && Tv1)
		{
			const FVector P = Tv1->GetActorLocation();
			Cam(P + FVector(-330.f, 120.f, 120.f), P + FVector(0.f, 0.f, 95.f), 58.f);
		}
		if (At(E3 + 7.8f) && Tv1) { Tv1->bHaunted = true; FlickerTime = 1.2f; PromoCue(TEXT("SFX_Misteri")); }
		if (At(E3 + 9.0f)) Finish();
		break;
	}
	// ---------------- Video 5: Kartu Momen dengan nama rental lucu ----------------
	case 5:
	{
		const FVector Wide(-520.f, -600.f, 250.f), WideLook(430.f, 150.f, 60.f);
		Cam(Wide, WideLook);
		if (At(0.2f)) { for (int32 i = 0; i < 4; ++i) Seat(EJRUnitType::PS, i, i == 0 ? TBocil : TSma); Seat(EJRUnitType::PC, 0, TMahasiswa); Seat(EJRUnitType::PC, 1, TGamer); }
		if (At(1.0f))
		{
			for (int32 i = 0; i < 4; ++i) On(EJRUnitType::PS, i, nullptr);
			On(EJRUnitType::PC, 0, nullptr); On(EJRUnitType::PC, 1, nullptr);
			// Isi hari yang "seru" untuk Kartu Momen
			Stats.Customers = 27; Stats.Income = 186000.f; Stats.Upsells = 4;
			AddMoment(EJRMoment::Upsell); AddMoment(EJRMoment::Emak); AddMoment(EJRMoment::Trip); AddMoment(EJRMoment::RageQuit);
		}
		if (At(2.0f)) { Toasts.Reset(); Choices.Reset(); EndDay(); }
		if (At(4.0f)) { Choices.Reset(); Rec(true); }
		if (At(9.2f)) Finish();
		break;
	}
	// ---------------- Video 7: turnamen PS akhir pekan (Era 2) ----------------
	case 7:
	{
		constexpr float Rs = 3.f;
		AJRStation* T1 = Unit(EJRUnitType::PS, 0);
		AJRStation* T2 = Unit(EJRUnitType::PS, 1);
		AJRStation* T4 = Unit(EJRUnitType::PS, 3);
		const FVector P1 = T1 ? T1->GetActorLocation() : FVector::ZeroVector;
		const FVector P2 = T2 ? T2->GetActorLocation() : FVector::ZeroVector;
		const FVector P4 = T4 ? T4->GetActorLocation() : FVector::ZeroVector;
		if (At(0.2f))
		{
			for (int32 i = 0; i < 6; ++i) Seat(EJRUnitType::PS, i, i % 3 == 0 ? TBocil : i % 3 == 1 ? TSma : TGamer);
			for (int32 i = 0; i < 4; ++i) Seat(EJRUnitType::PC, i, i % 2 ? TGamer : TMahasiswa);
		}
		if (At(1.0f)) { for (int32 i = 0; i < 4; ++i) On(EJRUnitType::PC, i, nullptr); }
		if (PromoT < Rs) Toasts.Reset();
		// V 0-4: deretan TV, turnamen dimulai, peserta mendaftar satu per satu
		if (PromoT < Rs + 4.2f) CamMove(Rs, Rs + 4.2f, P4 + FVector(-520.f, 420.f, 190.f), P1 + FVector(-520.f, -260.f, 175.f), P2 + FVector(-60.f, -120.f, 60.f), P2 + FVector(-60.f, 60.f, 70.f), 72.f);
		if (At(Rs)) { Toasts.Reset(); Rec(true); bTourAccepted = true; Hour = 15.995f; }
		if (At(Rs + 0.25f)) PromoCue(TEXT("SFX_TV_Gol"));
		for (int32 i = 0; i < 6; ++i)
		{
			if (At(Rs + 0.9f + i * 0.5f)) { On(EJRUnitType::PS, i, i == 0 ? TEXT("SFX_Konsol_Boot") : TEXT("SFX_Stik_Klik_1")); }
		}
		// V 4.2-8: close-up dua peserta bertanding, tendangan dan gol
		if (PromoT >= Rs + 4.2f && PromoT < Rs + 8.f)
			CamMove(Rs + 4.2f, Rs + 8.f, P1 + FVector(-330.f, 230.f, 140.f), P1 + FVector(-280.f, 190.f, 128.f), P1 + FVector(-100.f, 160.f, 80.f), P1 + FVector(-100.f, 160.f, 85.f), 62.f);
		if (At(Rs + 4.5f)) { Toasts.Reset(); PromoCue(TEXT("SFX_Stik_Klik_2")); }
		if (At(Rs + 5.3f)) PromoCue(TEXT("SFX_TV_Tendang"));
		if (At(Rs + 6.1f)) PromoCue(TEXT("SFX_Stik_Klik_3"));
		if (At(Rs + 6.6f)) { PromoCue(TEXT("SFX_TV_Tendang")); }
		if (At(Rs + 7.0f)) { PromoCue(TEXT("SFX_TV_Gol")); PromoCue(TEXT("VO_SMA_Rage_2")); }
		// V 8-13.5: pukul 20.00, final adu penalti, hadiah diserahkan
		if (PromoT >= Rs + 8.f) CamMove(Rs + 8.f, Rs + 13.5f, P4 + FVector(-700.f, 380.f, 230.f), P4 + FVector(-640.f, 330.f, 220.f), P2 + FVector(-40.f, -60.f, 60.f), P2 + FVector(-40.f, -40.f, 65.f), 74.f);
		if (At(Rs + 8.f)) { Toasts.Reset(); Hour = 20.f; }
		if (At(Rs + 8.1f)) PromoCue(TEXT("SFX_TV_Gol"));
		if (At(Rs + 11.3f) && Choices.Num() > 0) { ResolveChoice(0); PromoCue(TEXT("SFX_Kaching")); }
		if (At(Rs + 13.5f)) Finish();
		break;
	}
	// ---------------- Video 8: jam 10 malam, Pak RT datang menegur (Era 2) ----------------
	case 8:
	{
		constexpr float Rs = 3.f;
		AJRStation* T2 = Unit(EJRUnitType::PS, 1);
		AJRStation* T3 = Unit(EJRUnitType::PS, 2);
		const FVector P2 = T2 ? T2->GetActorLocation() : FVector::ZeroVector;
		const FVector P3 = T3 ? T3->GetActorLocation() : FVector::ZeroVector;
		const FVector DoorIn(-(L.HalfX - 90.f), L.DoorY, 110.f);
		if (At(0.2f))
		{
			for (int32 i = 0; i < 6; ++i) Seat(EJRUnitType::PS, i, i % 2 ? TSma : TGamer);
			for (int32 i = 0; i < 4; ++i) Seat(EJRUnitType::PC, i, i % 2 ? TGamer : TMahasiswa);
		}
		if (At(1.0f)) { for (int32 i = 0; i < 6; ++i) On(EJRUnitType::PS, i, nullptr); for (int32 i = 0; i < 4; ++i) On(EJRUnitType::PC, i, nullptr); }
		if (PromoT < Rs) Toasts.Reset();
		// V 0-3.5: rental ramai dan berisik jam sepuluh malam
		if (PromoT < Rs + 3.5f) CamMove(Rs, Rs + 3.5f, P3 + FVector(-430.f, 280.f, 150.f), P3 + FVector(-380.f, 230.f, 140.f), P2 + FVector(-120.f, 0.f, 70.f), P2 + FVector(-120.f, 0.f, 75.f), 66.f);
		if (At(Rs)) { Toasts.Reset(); Rec(true); PromoCue(TEXT("SFX_TV_Tendang")); }
		if (At(Rs + 0.6f)) PromoCue(TEXT("VO_SMA_Rage_1"));
		if (At(Rs + 1.6f)) PromoCue(TEXT("SFX_TV_Gol"));
		if (At(Rs + 2.3f)) PromoCue(TEXT("VO_Bapak_Rage_1"));
		// V 3.5-8: Pak RT masuk dari pintu, dialog teguran
		if (PromoT >= Rs + 3.5f && PromoT < Rs + 8.f) CamMove(Rs + 3.5f, Rs + 8.f, DoorIn + FVector(620.f, -560.f, 80.f), DoorIn + FVector(500.f, -450.f, 60.f), DoorIn, DoorIn + FVector(0.f, 0.f, 10.f), 60.f);
		if (At(Rs + 3.5f)) Toasts.Reset();
		if (At(Rs + 3.8f)) { PakRtEvent(); Hold(1.8f); PromoCue(TEXT("SFX_Bel_Pintu")); PromoCue(TEXT("VO_PakRT_Datang_1")); }
		// V 8-12: kasih kopi gratis, Pak RT pulang
		if (PromoT >= Rs + 8.f) CamMove(Rs + 8.f, Rs + 12.f, DoorIn + FVector(560.f, -500.f, 70.f), DoorIn + FVector(620.f, -540.f, 90.f), DoorIn, DoorIn + FVector(-60.f, 0.f, 10.f), 64.f);
		if (At(Rs + 8.f) && Choices.Num() > 0) { ResolveChoice(1); PromoCue(TEXT("VO_PakRT_Kopi_1")); PromoCue(TEXT("SFX_Kaching")); }
		if (At(Rs + 12.f)) Finish();
		break;
	}
	// ---------------- Foto 6: screenshot bersih tiga era untuk postingan (1920x1080) ----------------
	case 6:
	{
		auto Shot = [&](float T, const TCHAR* Name, bool bUI)
		{
			if (!At(T)) return;
			const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Capture/foto_%s.png"), Name));
			if (bUI) FScreenshotRequest::RequestScreenshot(File, true, false);
			else
			{
				// Tanpa HUD: render ulang 1920x1080 walau jendela lebih kecil (layar laptop)
				GetHighResScreenshotConfig().FilenameOverride = File;
				if (PC) PC->ConsoleCommand(TEXT("HighResShot 1920x1080"));
			}
			UE_LOG(LogTemp, Display, TEXT("JR_FOTO_SHOT %s era=%d"), Name, Era);
		};
		auto Wide = [&](float Fov = 72.f)
		{
			const FJRLayout& Ly = Layout();
			// Dari sudut dekat pintu, sedikit menunduk ke deretan TV dan PC (lantai kosong di depan dipotong)
			Cam(FVector(-Ly.HalfX * 0.25f, -Ly.HalfY * 0.45f, 175.f), FVector(Ly.HalfX * 0.75f, Ly.HalfY * 0.55f, 85.f), Fov);
		};
		auto SeatAll = [&]()
		{
			for (int32 i = 0; i < Count(EJRUnitType::PS); ++i) Seat(EJRUnitType::PS, i, i % 4 == 0 ? TBocil : i % 4 == 3 ? TBapak : TSma);
			for (int32 i = 0; i < Count(EJRUnitType::PC); ++i) Seat(EJRUnitType::PC, i, i % 2 ? TGamer : TMahasiswa);
			for (int32 i = 0; i < Count(EJRUnitType::VIP); ++i) Seat(EJRUnitType::VIP, i, i == 1 ? TKantoran : TGamer);
		};
		auto OnAll = [&]()
		{
			for (int32 i = 0; i < Count(EJRUnitType::PS); ++i) On(EJRUnitType::PS, i, nullptr);
			for (int32 i = 0; i < Count(EJRUnitType::PC); ++i) On(EJRUnitType::PC, i, nullptr);
			for (int32 i = 0; i < Count(EJRUnitType::VIP); ++i) On(EJRUnitType::VIP, i, nullptr);
		};
		// Tampak depan: mundur sesuai lebar gedung agar papan nama tidak terpotong
		auto FrontCam = [&]()
		{
			const FJRLayout& Ly = Layout();
			// Seberang jalan (lebih mundur lagi tertutup atap rumah kampung); gedung lebar = lensa lebih lebar
			const float Fov = FMath::Clamp(Ly.HalfY / 1000.f * 82.f, 82.f, 104.f);
			Cam(FVector(-(Ly.HalfX + 1280.f), 0.f, 400.f), FVector(-Ly.HalfX, 0.f, 400.f), Fov);
		};
		const bool bHudShot = PromoT >= 11.5f && PromoT < 14.f;
		ShowHud(bHudShot);
		if (At(0.2f)) SeatAll();
		if (At(1.0f)) OnAll();
		if (PromoT < 1.5f) Toasts.Reset();
		// Era 1: garasi
		if (PromoT < 4.f) Wide();
		Shot(3.8f, TEXT("01_garasi_dalam"), false);
		if (PromoT >= 4.f && PromoT < 6.5f)
		{
			if (AJRStation* S = Unit(EJRUnitType::PS, 1)) { const FVector P = S->GetActorLocation(); Cam(P + FVector(-420.f, 250.f, 150.f), P + FVector(-120.f, 0.f, 70.f), 66.f); }
		}
		Shot(6.3f, TEXT("02_main_ps"), false);
		if (PromoT >= 6.5f && PromoT < 9.f)
		{
			// Dari belakang pemain PC 2: monitor, kursi, dan deretan bilik
			if (AJRStation* S = Unit(EJRUnitType::PC, 1)) { const FVector P = S->GetActorLocation(); Cam(P + FVector(-260.f, -360.f, 165.f), P + FVector(120.f, 40.f, 80.f), 64.f); }
		}
		Shot(8.8f, TEXT("03_warnet"), false);
		if (PromoT >= 9.f && PromoT < 11.5f) FrontCam();
		Shot(11.3f, TEXT("04_garasi_depan"), false);
		if (bHudShot) { Wide(); if (At(11.6f)) { Toasts.Reset(); AddToast(JRPrintf(JRT(TEXT("Siap, Bang! %s main sejam lagi (+%s)")), TEXT("Raka"), *JRFormatRp(2000.f)), FLinearColor(0.36f, 0.85f, 0.56f)); } }
		Shot(13.8f, TEXT("05_gameplay_hud"), true);
		// Era 2: ruko kecil
		if (At(14.f)) { OwnedPS = 6; OwnedPC = 5; bPS2 = true; MoveToEra(2); while (Count(EJRUnitType::PS) < 6) SpawnStation(EJRUnitType::PS); while (Count(EJRUnitType::PC) < 5) SpawnStation(EJRUnitType::PC); Toasts.Reset(); }
		if (At(14.3f)) SeatAll();
		if (At(15.0f)) OnAll();
		if (PromoT >= 14.f && PromoT < 18.f) Wide();
		Shot(17.8f, TEXT("06_ruko_dalam"), false);
		if (PromoT >= 18.f && PromoT < 20.5f) FrontCam();
		Shot(20.3f, TEXT("07_ruko_depan"), false);
		// Era 3: game center 2 lantai + VIP Lounge
		if (At(20.5f))
		{
			OwnedPS = 8; OwnedPC = 7; MoveToEra(3);
			while (Count(EJRUnitType::PS) < 8) SpawnStation(EJRUnitType::PS);
			while (Count(EJRUnitType::PC) < 7) SpawnStation(EJRUnitType::PC);
			while (Count(EJRUnitType::VIP) < 4) SpawnStation(EJRUnitType::VIP);
			CapVA = 99999;	// foto: semua unit menyala tanpa MCB turun
			Toasts.Reset();
		}
		if (At(20.8f)) SeatAll();
		if (At(21.5f)) { OnAll(); ComputePower(); if (bMcbTripped) ResetMcb(); }
		if (PromoT >= 20.5f && PromoT < 24.5f) Wide(84.f);
		Shot(24.3f, TEXT("08_gamecenter_dalam"), false);
		{
			const float FH = Layout().FloorH;
			if (PromoT >= 24.5f && PromoT < 27.f)
			{
				// Booth VIP dari samping belakang pemain: sofa, TV besar
				if (AJRStation* S = Unit(EJRUnitType::VIP, 1)) { const FVector P = S->GetActorLocation(); Cam(P + FVector(-480.f, -330.f, 190.f), P + FVector(0.f, 60.f, 100.f), 70.f); }
				else Cam(FVector(300.f, -520.f, FH + 185.f), FVector(894.f, 120.f, FH + 200.f), 80.f);
			}
		}
		Shot(26.8f, TEXT("09_vip_lounge"), false);
		if (PromoT >= 27.f && PromoT < 29.5f) FrontCam();
		Shot(29.3f, TEXT("10_gamecenter_depan"), false);
		if (At(30.f)) { UE_LOG(LogTemp, Display, TEXT("JR_PROMO done id=6")); FGenericPlatformMisc::RequestExit(false); }
		break;
	}
	default: break;
	}

	if (bPromoRec)
	{
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Capture/promo%d_%05d.png"), PromoId, PromoFrame++));
		FScreenshotRequest::RequestScreenshot(File, PromoId != 4, false);
	}
}
