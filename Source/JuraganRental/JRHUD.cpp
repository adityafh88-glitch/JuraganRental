#include "JRHUD.h"
#include "JRLang.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "JRCharacter.h"
#include "JRGameMode.h"
#include "JRPhoneWidget.h"
#include "JRChoiceWidget.h"
#include "JRTouchWidget.h"
#include "JRSettings.h"
#include "JRMenuWidget.h"
#include "JRBillingWidget.h"
#include "JRSplashWidget.h"
#include "JRSocialWidgets.h"
#include "JROpening.h"
#include "Kismet/GameplayStatics.h"
#include "JRSound.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/TouchInterface.h"
#include "Misc/CommandLine.h"
#include "Engine/UserInterfaceSettings.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

namespace JRLocal_HUD
{
	const FLinearColor Led(1.f, 0.76f, 0.29f);
	const FLinearColor Ink(0.95f, 0.93f, 0.89f);
	const FLinearColor Muted(0.64f, 0.66f, 0.70f);
	const FLinearColor Panel(0.075f, 0.05f, 0.033f, 0.88f);	// cokelat hangat, senada logo
	const FLinearColor Crt(0.42f, 0.90f, 0.80f);			// tosca untuk ajakan "Ketuk LANJUT"
}
using namespace JRLocal_HUD;

void AJRHUD::BeginPlay()
{
	Super::BeginPlay();
	JRSettings::ApplyGraphics();
	if (PlayerOwner)
	{
		// Mode sentuh: otomatis di Android/iOS, atau paksa di PC dengan -jrtouch.
		bTouchMode = PLATFORM_ANDROID || PLATFORM_IOS || FParse::Param(FCommandLine::Get(), TEXT("jrtouch"));
		if (bTouchMode)
		{
			UInputSettings* Settings = GetMutableDefault<UInputSettings>();
			Settings->bAlwaysShowTouchInterface = true;
			Settings->bUseMouseForTouch = !(PLATFORM_ANDROID || PLATFORM_IOS); // uji di PC pakai mouse
			ApplyTouchSettings();
			ApplyUIScale();
		}

		Menu = CreateWidget<UJRMenuWidget>(PlayerOwner, UJRMenuWidget::StaticClass());
		if (Menu)
		{
			Menu->AddToViewport(30);
			Menu->SetVisibility(ESlateVisibility::Collapsed);
			Menu->OnChanged = [this]() { OnMenuChanged(); };
		}

		CreateGameWidgets();
	}
}

void AJRHUD::CreateGameWidgets()
{
	Phone = CreateWidget<UJRPhoneWidget>(PlayerOwner, UJRPhoneWidget::StaticClass());
	if (Phone)
	{
		Phone->AddToViewport(10);
		Phone->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (bTouchMode)
	{
		Touch = CreateWidget<UJRTouchWidget>(PlayerOwner, UJRTouchWidget::StaticClass());
		if (Touch) Touch->AddToViewport(5);
	}
	Billing = CreateWidget<UJRBillingWidget>(PlayerOwner, UJRBillingWidget::StaticClass());
	if (Billing)
	{
		Billing->AddToViewport(12);
		Billing->SetVisibility(ESlateVisibility::Collapsed);
		Billing->OnClose = [this]() { SetBillingOpen(false); };
	}

	NameWidget = CreateWidget<UJRNameWidget>(PlayerOwner, UJRNameWidget::StaticClass());
	if (NameWidget)
	{
		NameWidget->AddToViewport(25);
		NameWidget->SetVisibility(ESlateVisibility::Collapsed);
		NameWidget->OnDone = [this](const FString& N)
		{
			if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) G->SetRentalName(N);
			NameWidget->SetVisibility(ESlateVisibility::Collapsed);
			bNameShown = false;
			ApplyInputMode();
		};
	}
	MomentCard = CreateWidget<UJRMomentCard>(PlayerOwner, UJRMomentCard::StaticClass());
	if (MomentCard)
	{
		MomentCard->AddToViewport(22);
		MomentCard->SetVisibility(ESlateVisibility::Collapsed);
		MomentCard->OnReport = [this]()
		{
			if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) G->bShowMoment = false;
		};
		MomentCard->OnContinue = [this]()
		{
			if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) { G->bShowMoment = false; G->Confirm(); }
		};
	}

	Choice = CreateWidget<UJRChoiceWidget>(PlayerOwner, UJRChoiceWidget::StaticClass());
	if (Choice)
	{
		Choice->AddToViewport(20);
		Choice->SetVisibility(ESlateVisibility::Collapsed);
		Choice->OnPick = [this](int32 Index)
		{
			if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->ResolveChoice(Index);
			bChoiceShown = false;
			Choice->SetVisibility(ESlateVisibility::Collapsed);
			ApplyInputMode();
		};
	}
}

void AJRHUD::OnLanguageChanged()
{
	// Teks UMG ditulis saat widget dibangun, jadi widget permainan dibuat ulang dengan bahasa baru.
	// Menu (yang sedang terbuka) membangun ulang halamannya sendiri; teks Canvas dan label 3D dibaca tiap frame.
	if (!PlayerOwner) return;
	if (bPhoneOpen) SetPhoneOpen(false);
	if (bBillingOpen) SetBillingOpen(false);
	for (UUserWidget* W : TArray<UUserWidget*>{ Phone, Touch, Billing, NameWidget, MomentCard, Choice }) if (W) W->RemoveFromParent();
	Phone = nullptr; Touch = nullptr; Billing = nullptr; NameWidget = nullptr; MomentCard = nullptr; Choice = nullptr;
	bChoiceShown = false; bMomentShown = false; bNameShown = false;	// dimunculkan lagi otomatis bila masih perlu
	CreateGameWidgets();
	if (Touch) Touch->SyncVisibility();
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->OnLanguageChanged();
	ApplyInputMode();
	UE_LOG(LogTemp, Display, TEXT("JR_LANG ganti english=%d"), JRLang::IsEnglish() ? 1 : 0);
}

void AJRHUD::SyncChoice()
{
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!Choice || !GM) return;
	// Dialog berganti tanpa jeda (mis. kejadian berantai): tampilkan yang baru.
	if (bChoiceShown && GM->Choices.Num() > 0 && GM->Choices[0].Serial != ShownChoiceSerial) bChoiceShown = false;
	if (GM->Choices.Num() > 0 && bBillingOpen) SetBillingOpen(false);	// kejadian penting menutup layar billing
	if (!bChoiceShown && GM->Choices.Num() > 0)
	{
		ShownChoiceSerial = GM->Choices[0].Serial;
		if (bPhoneOpen) SetPhoneOpen(false);
		const FJRChoice& C = GM->Choices[0];
		Choice->Show(C.Title, C.Body, C.Labels, C.bNeutral);
		JRSound::Play2D(this, TEXT("SFX_Notif"), 0.8f);
		Choice->SetVisibility(ESlateVisibility::Visible);
		bChoiceShown = true;
		ApplyInputMode();
	}
	else if (bChoiceShown && GM->Choices.Num() == 0)
	{
		bChoiceShown = false;
		Choice->SetVisibility(ESlateVisibility::Collapsed);
		ApplyInputMode();
	}
}

bool AJRHUD::IsUIOpen() const
{
	return bPhoneOpen || bChoiceShown || bBillingOpen || bNameShown || bMomentShown || (Menu && Menu->IsOpen());
}

void AJRHUD::OnMenuChanged()
{
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	// Menu jeda menghentikan waktu; menu utama tidak (dunia jadi latar yang hidup).
	const bool bPause = Menu && Menu->IsOpen() && GM && GM->Phase != EJRGamePhase::Menu && Menu->GetPage() != EJRMenuPage::DemoEnd;
	if (UGameplayStatics::IsGamePaused(this) != bPause) UGameplayStatics::SetGamePaused(this, bPause);
	if (Menu && Menu->IsOpen() && bPhoneOpen) SetPhoneOpen(false);
	ApplyInputMode();
}

void AJRHUD::TogglePause()
{
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!Menu || !GM) return;
	if (bPhotoMode) { SetPhotoMode(false); return; }	// Esc / tombol jeda = keluar mode foto
	if (Menu->IsOpen()) { Menu->Back(); return; }
	if (bBillingOpen) { SetBillingOpen(false); return; }
	if (bPhoneOpen) { SetPhoneOpen(false); return; }
	if (bChoiceShown || GM->Phase == EJRGamePhase::Menu) return;
	Menu->ShowPause();
}

void AJRHUD::ApplyInputMode()
{
	if (!PlayerOwner) return;
	if (IsUIOpen() || bTouchMode)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerOwner->SetInputMode(Mode);
		PlayerOwner->bShowMouseCursor = true;
	}
	else
	{
		PlayerOwner->SetInputMode(FInputModeGameOnly());
		PlayerOwner->bShowMouseCursor = false;
	}
}

void AJRHUD::ApplyTouchSettings()
{
	if (!bTouchMode || !PlayerOwner) return;
	UTouchInterface* Base = LoadObject<UTouchInterface>(nullptr, TEXT("/Engine/MobileResources/HUD/DefaultVirtualJoysticks.DefaultVirtualJoysticks"));
	if (!Base) return;
	UTouchInterface* Sticks = DuplicateObject<UTouchInterface>(Base, this);
	const bool bFixed = JRSettings::Get()->bJoystickFixed;
	Sticks->bPreventRecenter = bFixed;			// tetap: joystick tidak pindah ke posisi jari
	Sticks->InactiveOpacity = bFixed ? 0.35f : 0.f;	// tetap: selalu terlihat samar
	if (bPhotoMode) { Sticks->ActiveOpacity = 0.f; Sticks->InactiveOpacity = 0.f; }	// mode foto: joystick tak terlihat
	// Joystick kanan dibuang: menoleh dengan menggeser jari di sisi kanan layar (UJRLookPad).
	Sticks->Controls.RemoveAll([](const FTouchInputControl& C) { return C.MainInputKey == EKeys::Gamepad_RightX; });
	for (FTouchInputControl& C : Sticks->Controls)
	{
		// Ukuran joystick dari pengaturan (nilai <= 1 relatif ke layar, jadi jangan sampai lewat 1)
		const float F = FMath::Clamp(JRSettings::Get()->JoystickSize, 0.5f, 1.3f);
		auto Scale = [F](FVector2D& V) { const bool bRel = V.X <= 1.f && V.Y <= 1.f; V *= F; if (bRel) { V.X = FMath::Min(V.X, 1.f); V.Y = FMath::Min(V.Y, 1.f); } };
		Scale(C.VisualSize); Scale(C.ThumbSize); Scale(C.InteractionSize);
	}
	PlayerOwner->ActivateTouchInterface(Sticks);
	if (!bStickShown) PlayerOwner->SetVirtualJoystickVisibility(false);	// tetap tersembunyi di panel pembuka/laporan
}

void AJRHUD::SetPhotoMode(bool bOn)
{
	if (bPhotoMode == bOn) return;
	if (bOn)
	{
		if (bPhoneOpen) SetPhoneOpen(false);
		if (bBillingOpen) SetBillingOpen(false);
	}
	bPhotoMode = bOn;
	PhotoHint = bOn ? 4.f : 0.f;
	ApplyTouchSettings();
	if (Touch) Touch->SyncVisibility();
	UE_LOG(LogTemp, Display, TEXT("JR_FOTO mode=%d"), bOn ? 1 : 0);
}

void AJRHUD::TakePhoto()
{
	const FString Dir = FPaths::Combine(FPlatformProcess::UserDir(), TEXT("Juragan Rental"), TEXT("Foto"));
	IFileManager::Get().MakeDirectory(*Dir, true);
	const FString File = FPaths::Combine(Dir, FDateTime::Now().ToString(TEXT("JuraganRental_%Y%m%d_%H%M%S.png")));
	FScreenshotRequest::RequestScreenshot(File, false, false);	// tanpa widget UMG
	PhotoHideFrames = 3;	// HUD Canvas juga disembunyikan sampai foto diambil
	PendingPhoto = File;
	JRSound::Play2D(this, TEXT("SFX_HP_Tombol"), 0.6f);
}

void AJRHUD::ApplyUIScale()
{
	// Skala UMG dihitung ulang tiap frame dari ApplicationScale, jadi perubahan langsung terlihat.
	GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale = bTouchMode ? FMath::Clamp(JRSettings::Get()->UIScale, 1.f, 2.2f) : 1.f;
}

void AJRHUD::OpenNameDialog()
{
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!NameWidget || !GM || bNameShown) return;
	if (bPhoneOpen) SetPhoneOpen(false);
	NameWidget->Open(GM->RentalName);
	bNameShown = true;
	ApplyInputMode();
}

void AJRHUD::SetBillingOpen(bool bOpen)
{
	if (!Billing || bBillingOpen == bOpen) return;
	if (bOpen && (bChoiceShown || (Menu && Menu->IsOpen()))) return;
	if (bOpen && bPhoneOpen) SetPhoneOpen(false);
	bBillingOpen = bOpen;
	Billing->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bOpen) Billing->Refresh(true);
	JRSound::Play2D(this, bOpen ? TEXT("SFX_HP_Buka") : TEXT("SFX_HP_Tombol"), 0.5f);
	ApplyInputMode();
}

void AJRHUD::TogglePhone()
{
	SetPhoneOpen(!bPhoneOpen);
}

void AJRHUD::SetPhoneOpen(bool bOpen)
{
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (bOpen && (!GM || GM->Phase == EJRGamePhase::Intro || GM->Phase == EJRGamePhase::Menu || bChoiceShown || (Menu && Menu->IsOpen()))) return;
	if (!Phone || !PlayerOwner) return;
	bPhoneOpen = bOpen;
	Phone->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (bOpen) Phone->Refresh(true);
	if (bOpen) if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) G->TutorialAdvance(AJRGameMode::TutPhone);
	JRSound::Play2D(this, bOpen ? TEXT("SFX_HP_Buka") : TEXT("SFX_HP_Tombol"), 0.6f);
	ApplyInputMode();
}

void AJRHUD::PostRender()
{
	// Engine mengecilkan kanvas HUD ke area aman layar (poni/kamera HP), sehingga bar atas dan panel
	// tidak sampai tepi kiri. HUD ini menggambar sampai tepi layar; tombol sentuh UMG tetap pada tempatnya.
	if (Canvas) Canvas->PopSafeZoneTransform();
	Super::PostRender();
	if (Canvas) Canvas->ApplySafeZoneTransform();	// dikembalikan agar Pop milik engine tetap seimbang
}

void AJRHUD::DrawHUD()
{
	Super::DrawHUD();
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!GM || !Canvas) return;
	if (Touch) Touch->SyncVisibility();	// juga memunculkan lagi tombol sentuh setelah menu utama ditutup
	if (bTouchMode && PlayerOwner)
	{
		// Joystick tidak menutupi panel pembuka/laporan harian (di fase itu pemain tidak berjalan).
		const bool bStick = GM->Phase == EJRGamePhase::Playing && !GM->IsOpening();
		if (bStick != bStickShown) { bStickShown = bStick; PlayerOwner->SetVirtualJoystickVisibility(bStick); }
	}

	// -jrtouchflow: ulangi alur di HP (menu utama -> Main baru) lalu catat apakah tombol sentuh muncul.
	static const bool bTouchFlow = FParse::Param(FCommandLine::Get(), TEXT("jrtouchflow"));
	if (bTouchFlow)
	{
		static float FlowTime = 0.f;
		FlowTime += GetWorld()->GetDeltaSeconds();
		AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>();
		static int32 Step = 0;
		if (Step == 0 && FlowTime > 3.f && G->Phase == EJRGamePhase::Menu && Menu && Menu->IsOpen())
		{
			UE_LOG(LogTemp, Display, TEXT("JR_TOUCHFLOW menu touchVisible=%d"), Touch && Touch->IsVisible() ? 1 : 0);
			G->BeginNewGame();
			Menu->Close();
			Step = 1;
		}
		else if (Step == 1 && FlowTime > 5.f)
		{
			UE_LOG(LogTemp, Display, TEXT("JR_TOUCHFLOW intro phase=%d touchVisible=%d uis=%.2f"), (int32)G->Phase, Touch && Touch->IsVisible() ? 1 : 0, UIS);
			G->Confirm();	// sama dengan tombol LANJUT
			Step = 2;
		}
		else if (Step == 2 && FlowTime > 7.f)
		{
			UE_LOG(LogTemp, Display, TEXT("JR_TOUCHFLOW playing phase=%d touchVisible=%d"), (int32)G->Phase, Touch && Touch->IsVisible() ? 1 : 0);
			Step = 3;
		}
		else if (Step >= 3 && Step < 9 && FlowTime > 7.5f + (Step - 3) * 0.1f)
		{
			// Simulasi jari menggeser sisi kanan layar 300 piksel ke kanan, lalu cek kamera ikut berputar.
			TSharedPtr<SViewport> VP = GEngine->GameViewport ? GEngine->GameViewport->GetGameViewportWidget() : nullptr;
			TSharedPtr<SWindow> Win = GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
			if (VP.IsValid() && Win.IsValid())
			{
				const FGeometry Geo = VP->GetCachedGeometry();
				const FVector2D Size = Geo.GetLocalSize();
				const FVector2D P = Geo.LocalToAbsolute(FVector2D(Size.X * 0.7f + (Step - 3) * 60.f, Size.Y * 0.5f));
				FSlateApplication& App = FSlateApplication::Get();
				// Pengguna dan perangkat input yang sah (ID kosong memicu assert UserIndex >= 0 di Slate).
				IPlatformInputDeviceMapper& Mapper = IPlatformInputDeviceMapper::Get();
				const FPlatformUserId U = Mapper.GetPrimaryPlatformUser();
				const FInputDeviceId D = Mapper.GetDefaultInputDevice();
				if (Step == 3) { LookYaw0 = PlayerOwner->GetControlRotation().Yaw; App.OnTouchStarted(Win->GetNativeWindow(), P, 1.f, 3, U, D); }
				else if (Step < 8) App.OnTouchMoved(P, 1.f, 3, U, D);
				else
				{
					App.OnTouchEnded(P, 3, U, D);
					const float Turned = FMath::FindDeltaAngleDegrees(LookYaw0, PlayerOwner->GetControlRotation().Yaw);
					UE_LOG(LogTemp, Display, TEXT("JR_TOUCHFLOW swipe yawDelta=%.1f"), Turned);
				}
			}
			else UE_LOG(LogTemp, Display, TEXT("JR_TOUCHFLOW swipe noviewport vp=%d win=%d"), VP.IsValid() ? 1 : 0, Win.IsValid() ? 1 : 0);
			Step++;
		}
		else if (Step >= 9 && Step < 20 && FlowTime > 9.f + (Step - 9) * 0.1f)
		{
			// Uji jari "nyangkut": jari 4 menyentuh, HP dibuka-tutup sebelum jari diangkat,
			// lalu jari 5 menggeser. Kamera harus tetap ikut berputar.
			TSharedPtr<SViewport> VP = GEngine->GameViewport ? GEngine->GameViewport->GetGameViewportWidget() : nullptr;
			TSharedPtr<SWindow> Win = GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
			if (VP.IsValid() && Win.IsValid())
			{
				const FGeometry Geo = VP->GetCachedGeometry();
				const FVector2D Size = Geo.GetLocalSize();
				FSlateApplication& App = FSlateApplication::Get();
				IPlatformInputDeviceMapper& Mapper = IPlatformInputDeviceMapper::Get();
				const FPlatformUserId U = Mapper.GetPrimaryPlatformUser();
				const FInputDeviceId D = Mapper.GetDefaultInputDevice();
				const FVector2D P0 = Geo.LocalToAbsolute(FVector2D(Size.X * 0.6f, Size.Y * 0.55f));
				if (Step == 9) App.OnTouchStarted(Win->GetNativeWindow(), P0, 1.f, 4, U, D);
				else if (Step == 10) SetPhoneOpen(true);
				else if (Step == 11) SetPhoneOpen(false);
				else
				{
					const FVector2D P = Geo.LocalToAbsolute(FVector2D(Size.X * 0.7f + (Step - 12) * 60.f, Size.Y * 0.5f));
					if (Step == 12) { LookYaw0 = PlayerOwner->GetControlRotation().Yaw; App.OnTouchStarted(Win->GetNativeWindow(), P, 1.f, 5, U, D); }
					else if (Step < 17) App.OnTouchMoved(P, 1.f, 5, U, D);
					else if (Step == 17)
					{
						App.OnTouchEnded(P, 5, U, D);
						UE_LOG(LogTemp, Display, TEXT("JR_TOUCHFLOW stuckfinger yawDelta=%.1f"), FMath::FindDeltaAngleDegrees(LookYaw0, PlayerOwner->GetControlRotation().Yaw));
					}
					else if (Step == 18 && Touch)
					{
						// Uji AKSI "nyangkut": AKSI disentuh, HP dibuka-tutup sebelum jari diangkat, lalu AKSI diketuk lagi.
						const FVector2D A = Touch->ActionCenterAbs();
						const int32 Before = Touch->ActionPresses;
						App.OnTouchStarted(Win->GetNativeWindow(), A, 1.f, 6, U, D);
						SetPhoneOpen(true);
						SetPhoneOpen(false);
						Touch->SyncVisibility();
						App.OnTouchStarted(Win->GetNativeWindow(), A, 1.f, 7, U, D);
						App.OnTouchEnded(A, 7, U, D);
						UE_LOG(LogTemp, Display, TEXT("JR_TOUCHFLOW aksi presses=%d (harus 2)"), Touch->ActionPresses - Before);
					}
					else if (Step == 19) FGenericPlatformMisc::RequestExit(false);
				}
			}
			Step++;
		}
	}

	// Layar pembuka sekali saat game dibuka (dilewati di uji otomatis yang memotret menu).
	if (!bSplashChecked && GM->Phase == EJRGamePhase::Menu)
	{
		bSplashChecked = true;
		const TCHAR* Cmd = FCommandLine::Get();
		const bool bSkip = FParse::Param(Cmd, TEXT("jrmenutest")) || FParse::Param(Cmd, TEXT("jrtouchflow")) || FParse::Param(Cmd, TEXT("jrnosplash"));
		if (!bSkip && PlayerOwner)
		{
			Splash = CreateWidget<UJRSplashWidget>(PlayerOwner, UJRSplashWidget::StaticClass());
			if (Splash)
			{
				Splash->AddToViewport(60);
				Splash->OnFinished = [this]() { Splash = nullptr; };
			}
		}
	}

	// Menu utama: tampilkan menunya saja, tanpa status toko.
	if (GM->Phase == EJRGamePhase::Menu)
	{
		if (Menu && !Menu->IsOpen()) Menu->ShowMain();
		return;
	}

	if (GM->bDemoOver && Menu && Menu->GetPage() != EJRMenuPage::DemoEnd)
	{
		if (bPhoneOpen) SetPhoneOpen(false);
		Menu->ShowDemoEnd();
	}

	SyncChoice();
	// Uji -jrnametest: buka dialog nama saat bermain (untuk dipotret)
	static const bool bNameTest = FParse::Param(FCommandLine::Get(), TEXT("jrnametest"));
	if (bNameTest && GM->Phase == EJRGamePhase::Playing && !bNameShown && GetWorld()->GetTimeSeconds() > 4.f) { static bool bOnce = false; if (!bOnce) { bOnce = true; OpenNameDialog(); } }
	// Main baru: minta nama rental setelah opening selesai
	if (!bNameShown && GM->bNeedName && !GM->IsOpening() && GM->Phase == EJRGamePhase::Intro && NameWidget && !(Menu && Menu->IsOpen()))
	{
		NameWidget->Open(GM->RentalName);
		bNameShown = true;
		ApplyInputMode();
	}
	// Kartu Momen di layar laporan hari yang baru selesai
	{
		const bool bWant = GM->Phase == EJRGamePhase::Report && GM->bShowMoment && !bPhoneOpen && !bChoiceShown && !(Menu && Menu->IsOpen()) && !GM->bDemoOver;
		if (MomentCard && bWant != bMomentShown)
		{
			if (bWant) MomentCard->Refresh(); else MomentCard->SetVisibility(ESlateVisibility::Collapsed);
			bMomentShown = bWant;
			ApplyInputMode();
		}
	}
	UFont* Font = GEngine->GetMediumFont();
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	// Mode sentuh: ikuti skala UMG (resolusi layar x ukuran tampilan) agar teks HUD sama besar dengan HP/menu.
	UIS = bTouchMode ? FMath::Max(0.5f, GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(FMath::RoundToInt(W), FMath::RoundToInt(H)))) : 1.f;
	const float S = UIS;
	const float Dt = GetWorld()->GetDeltaSeconds();

	// F12: HUD disembunyikan beberapa frame sampai foto selesai diambil, lalu beri tahu lokasinya.
	if (PhotoHideFrames > 0)
	{
		if (--PhotoHideFrames == 0 && !PendingPhoto.IsEmpty())
		{
			if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>())
				G->AddToast(JRPrintf(JRT(TEXT("Foto tersimpan di Dokumen > Juragan Rental > Foto (%s)")), *FPaths::GetCleanFilename(PendingPhoto)), FLinearColor(0.36f, 0.85f, 0.56f));
			PendingPhoto.Reset();
		}
		return;
	}
	// Uji -jrlangtest: ganti bahasa ke Inggris di tengah permainan (tanpa menyimpan pengaturan ke disk).
	static const bool bLangTest = FParse::Param(FCommandLine::Get(), TEXT("jrlangtest"));
	if (bLangTest && GetWorld()->GetTimeSeconds() > 4.f)
	{
		static bool bOnce = false;
		if (!bOnce)
		{
			bOnce = true;
			const FString Before = JRT(TEXT("Pengaturan"));
			JRSettings::Get()->Language = JRLang::IsEnglish() ? 1 : 2;	// balik dari bahasa yang sedang aktif
			JRLang::Reset();
			OnLanguageChanged();
			UE_LOG(LogTemp, Display, TEXT("JR_LANGTEST sebelum=%s sesudah=%s phone=%d"), *Before, JRT(TEXT("Pengaturan")), Phone ? 1 : 0);
			return;	// widget baru, gambar di frame berikutnya
		}
	}
	// Uji -jrfototest: masuk mode foto setelah 6 detik bermain (untuk dipotret).
	static const bool bFotoTest = FParse::Param(FCommandLine::Get(), TEXT("jrfototest"));
	if (bFotoTest && !bPhotoMode && GM->Phase == EJRGamePhase::Playing && GetWorld()->GetTimeSeconds() > 6.f) { static bool bOnce = false; if (!bOnce) { bOnce = true; SetPhotoMode(true); } }
	// Mode foto: layar bersih untuk bahan postingan, hanya petunjuk singkat di awal.
	if (bPhotoMode)
	{
		if (bChoiceShown || GM->Phase != EJRGamePhase::Playing) { SetPhotoMode(false); }
		else
		{
			PhotoHint = FMath::Max(0.f, PhotoHint - Dt);
			if (PhotoHint > 0.f)
			{
				const FString Hint = bTouchMode
					? FString(JRT(TEXT("MODE FOTO: pakai screenshot / rekam layar HP. Ketuk pojok kiri atas untuk kembali.")))
					: FString(JRT(TEXT("MODE FOTO: F12 simpan foto, Win+Alt+R rekam video. Esc untuk kembali.")));
				float TW = 0.f, TH = 0.f;
				GetTextSize(Hint, TW, TH, Font, 1.05f * S);
				const float Fit = FMath::Min(1.05f * S, (W * 0.9f) / FMath::Max(1.f, TW / (1.05f * S)));
				GetTextSize(Hint, TW, TH, Font, Fit);
				const float A = FMath::Clamp(PhotoHint, 0.f, 1.f);
				DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f * A), (W - TW) * 0.5f - 14.f * S, H * 0.42f - 8.f * S, TW + 28.f * S, TH + 16.f * S);
				DrawText(Hint, FLinearColor(1.f, 1.f, 1.f, A), (W - TW) * 0.5f, H * 0.42f, Font, Fit);
			}
			return;
		}
	}
	// Konfeti: naik level, impian terwujud, ending
	{
		const float T = GetWorld()->GetTimeSeconds() - GM->CelebrateAt;
		if (T >= 0.f && T < 2.8f)
		{
			static const FLinearColor Cols[] = { FLinearColor(1.f, 0.58f, 0.1f), FLinearColor(1.f, 0.85f, 0.2f), FLinearColor(0.2f, 0.75f, 0.65f), FLinearColor(0.95f, 0.3f, 0.3f), FLinearColor(0.35f, 0.55f, 1.f), FLinearColor(0.97f, 0.95f, 0.9f) };
			const float A = FMath::Clamp((2.8f - T) / 0.6f, 0.f, 1.f);
			const int32 Seed = FMath::RoundToInt(GM->CelebrateAt * 100.f);
			for (int32 i = 0; i < 90; ++i)
			{
				FRandomStream R(Seed + i * 7919);
				const float X0 = R.FRandRange(0.f, W), Speed = R.FRandRange(0.35f, 0.75f) * H, Delay = R.FRandRange(0.f, 0.6f);
				const float Tt = T - Delay;
				if (Tt < 0.f) continue;
				const float Y = -20.f * S + Tt * Speed;
				if (Y > H) continue;
				const float X = X0 + FMath::Sin(Tt * R.FRandRange(3.f, 7.f) + i) * 30.f * S;
				const float Flip = FMath::Abs(FMath::Sin(Tt * R.FRandRange(6.f, 12.f) + i));	// berputar: lebar berubah-ubah
				FLinearColor C = Cols[i % UE_ARRAY_COUNT(Cols)];
				C.A = A;
				DrawRect(C, X, Y, (3.f + 7.f * Flip) * S, 12.f * S);
			}
		}
	}
	// Penguji: angka FPS di pojok kanan bawah (Pengaturan > Tampilkan FPS).
	if (Dt > 0.f) FpsSmooth = FMath::Lerp(FpsSmooth, 1.f / Dt, 0.05f);
	if (JRSettings::Get()->bShowFps)
	{
		const FString Fps = JRPrintf(JRT(TEXT("%d FPS")), FMath::RoundToInt(FpsSmooth));
		const FLinearColor C = FpsSmooth >= 50.f ? FLinearColor(0.4f, 1.f, 0.5f) : FpsSmooth >= 28.f ? FLinearColor(1.f, 0.85f, 0.3f) : FLinearColor(1.f, 0.4f, 0.35f);
		DrawText(Fps, C, W * 0.5f - 30.f * S, H - 28.f * S, Font, 1.f * S);
	}
	// Tanda kecil setiap kali permainan tersimpan otomatis.
	if (GM->SaveFlash > 0.f && GM->Phase == EJRGamePhase::Playing)
	{
		const FString Saved = JRT(TEXT("Tersimpan otomatis"));
		float TW = 0.f, TH = 0.f;
		GetTextSize(Saved, TW, TH, Font, 0.95f * S);
		const float A = FMath::Clamp(GM->SaveFlash, 0.f, 1.f);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f * A), (W - TW) * 0.5f - 10.f * S, 46.f * S, TW + 20.f * S, TH + 8.f * S);
		DrawText(Saved, FLinearColor(0.6f, 1.f, 0.7f, A), (W - TW) * 0.5f, 50.f * S, Font, 0.95f * S);
	}

	// Adegan pembuka: layar bergaris hitam ala film + teks narasi, tanpa HUD lain.
	if (const AJROpening* Op = GM->GetOpening())
	{
		static const bool bCapture = FParse::Param(FCommandLine::Get(), TEXT("jrcapture"));
		if (bCapture) return;	// rekaman promosi: gambar bersih tanpa pita hitam dan teks
		const float Bar = H * 0.11f;
		DrawRect(FLinearColor::Black, 0.f, 0.f, W, Bar);
		DrawRect(FLinearColor::Black, 0.f, H - Bar, W, Bar);
		const FString Cap = Op->Caption();
		if (!Cap.IsEmpty())
		{
			float TW = 0.f, TH = 0.f;
			GetTextSize(Cap, TW, TH, Font, 1.f);
			const float TS = FMath::Min(1.45f * S, (W * 0.86f) / FMath::Max(1.f, TW));	// teks panjang diperkecil agar muat
			GetTextSize(Cap, TW, TH, Font, TS);
			FLinearColor C = FLinearColor::White;	// di atas pita gelap
			C.A = Op->CaptionAlpha();
			const float CY = H - Bar - TH - 22.f * S;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f * C.A), (W - TW) * 0.5f - 18.f * S, CY - 8.f * S, TW + 36.f * S, TH + 16.f * S);	// pita gelap agar terbaca
			DrawText(Cap, C, (W - TW) * 0.5f, CY, Font, TS);
		}
		const FString Skip = bTouchMode ? JRT(TEXT("Ketuk LEWATI untuk melewati")) : JRT(TEXT("ENTER: lewati"));
		float SW = 0.f, SH = 0.f;
		GetTextSize(Skip, SW, SH, Font, 0.9f * S);
		DrawText(Skip, Muted, bTouchMode ? 24.f * S : W - SW - 24.f * S, H - Bar * 0.5f - SH * 0.5f, Font, 0.9f * S);
		const float F = Op->Fade();
		if (F > 0.01f) DrawRect(FLinearColor(0.f, 0.f, 0.f, F), 0.f, 0.f, W, H);
		return;
	}

	// Status atas
	const int32 Hr = FMath::FloorToInt(GM->Hour);
	const int32 Mn = FMath::FloorToInt((GM->Hour - Hr) * 60.f);
	const FString Power = GM->bMcbTripped ? FString(JRT(TEXT("MCB TURUN"))) : JRPrintf(JRT(TEXT("%d/%d VA")), GM->PowerVA, GM->CapVA);
	const FString Top = JRPrintf(JRT(TEXT("%s  Hari %d   %02d:%02d   Kas %s   Reputasi %d   Daya %s")),
		JRT(*GM->Layout().Name), GM->Day, Hr, Mn, *JRFormatRp(GM->Money), FMath::RoundToInt(GM->Rep), *Power)
		+ JRPrintf(JRT(TEXT("   Lv %d")), GM->JuraganLevel());
	{
		// Bar atas: perkecil teks bila layar sempit agar tidak terpotong
		float TW = 0.f, TH = 0.f;
		GetTextSize(Top, TW, TH, Font, 1.25f * S);
		const float Fit = FMath::Min(1.f, (W - 32.f * S) / FMath::Max(TW, 1.f));
		DrawRect(Panel, 0.f, 0.f, W, 40.f * S);
		DrawText(Top, Led, 16.f * S, 9.f * S, Font, 1.25f * S * Fit);
	}

	if (GM->Phase == EJRGamePhase::Intro)
	{
		DrawPanel(TEXT("Juragan Rental"), {
			JRT(TEXT("Kamu baru membuka rental PS di garasi rumah: dua TV tabung, satu PC warnet,")),
			JRT(TEXT("dan MCB 1.300 VA yang gampang turun.")),
			TEXT(""),
			bTouchMode ? JRT(TEXT("Joystick kiri: jalan    Geser layar kanan: lihat sekitar    Tombol aksi: interaksi    HP: belanja, tarif    II: jeda"))
				: JRT(TEXT("WASD: jalan    Mouse: lihat    E: interaksi (tahan untuk memperbaiki)    Tab: HP    Esc: menu jeda")),
			TEXT(""),
			bTouchMode ? JRT(TEXT("Arahkan titik tengah layar ke TV yang ditunggu pelanggan lalu ketuk tombol aksi.")) : JRT(TEXT("Lihat ke TV yang ditunggu pelanggan lalu tekan E untuk menyalakan.")),
			JRT(TEXT("Pelanggan selesai main akan antre di kasir dekat pintu. Jangan biarkan kasir kosong.")),
			JRT(TEXT("Target: Rp 600.000 dan reputasi 40 untuk membuka Gerbang 1.")),
		}, bTouchMode ? JRT(TEXT("Ketuk LANJUT untuk buka toko")) : JRT(TEXT("Tekan ENTER untuk buka toko")));
		return;
	}
	if (GM->Phase == EJRGamePhase::Report)
	{
		if (!bPhoneOpen && !bMomentShown)
		{
			TArray<FString> Lines = GM->ReportLines;
			Lines.Add(TEXT(""));
			Lines.Add(bTouchMode ? JRT(TEXT("Ketuk tombol HP untuk belanja barang dan atur tarif.")) : JRT(TEXT("Tekan TAB untuk buka HP: belanja barang dan atur tarif.")));
			DrawPanel(JRPrintf(JRT(TEXT("%s  -  Hari %d selesai")), *GM->RentalName, GM->Day), Lines,
				bTouchMode ? JRPrintf(JRT(TEXT("Ketuk LANJUT untuk buka toko hari %d")), GM->Day + 1) : JRPrintf(JRT(TEXT("Tekan ENTER untuk buka toko hari %d")), GM->Day + 1));
		}
		return;
	}

	// Titik bidik dan petunjuk HP
	if (!bPhoneOpen)
	{
		DrawRect(FLinearColor(1, 1, 1, 0.8f), W * 0.5f - 2.f * S, H * 0.5f - 2.f * S, 4.f * S, 4.f * S);
	}
	if (!bTouchMode)
	{
		DrawRect(Panel, 12.f, H - 44.f, 150.f, 32.f);
		DrawText(bPhoneOpen ? JRT(TEXT("[Tab] Tutup HP")) : JRT(TEXT("[Tab] Buka HP")), Led, 24.f, H - 38.f, Font, 1.1f);
	}

	// Spanduk turnamen
	if (GM->bTourRunning && GM->Phase == EJRGamePhase::Playing)
	{
		const FString Banner = JRPrintf(JRT(TEXT("TURNAMEN BERLANGSUNG  16.00-20.00   Peserta %d/%d")), GM->TourPlayers, AJRGameMode::TourMax);
		float TW = 0.f, TH = 0.f;
		GetTextSize(Banner, TW, TH, Font, 1.2f * S);
		DrawRect(FLinearColor(0.75f, 0.12f, 0.1f, 0.9f), (W - TW) * 0.5f - 16.f * S, 46.f * S, TW + 32.f * S, TH + 12.f * S);
		DrawText(Banner, FLinearColor::White, (W - TW) * 0.5f, 52.f * S, Font, 1.2f * S);
	}

	// Daftar misi di kiri atas (saat tutorial tidak tampil)
	if (!GM->IsTutorial() && GM->Phase == EJRGamePhase::Playing && !bPhoneOpen && GM->Missions.Num() > 0)
	{
		const float MS = S;
		const float PX = (bTouchMode ? 180.f : 16.f) * S, PY = 50.f * S;	// sentuh: di kanan tombol II dan HP
		const float LineH = 22.f * MS;
		// Lebar panel mengikuti teks terpanjang (bahasa Inggris/Indonesia beda panjang)
		float PW = 380.f * MS;
		for (const FJRMission& M : GM->Missions)
		{
			float TW = 0.f, TH = 0.f;
			GetTextSize(GM->MissionText(M) + JRT(TEXT("  (Minggu) 300/300")), TW, TH, Font, 0.95f * MS);
			PW = FMath::Max(PW, TW + 24.f * MS);
		}
		PW = FMath::Min(PW, W * 0.40f);	// sisakan tempat untuk notifikasi di kanan
		// Ringkas: satu baris "MISI 1/4 selesai [+]" agar tidak menutupi layar; ketuk (atau atur di HP) untuk membuka.
		const bool bCompact = JRSettings::Get()->bMissionsCompact;
		int32 DoneCount = 0;
		for (const FJRMission& M : GM->Missions) DoneCount += M.bDone ? 1 : 0;
		const FString Head = JRPrintf(JRT(TEXT("MISI  %d/%d selesai")), DoneCount, GM->Missions.Num()) + (bCompact ? TEXT("  [+]") : TEXT("  [-]"));
		if (bCompact)
		{
			float TW = 0.f, TH = 0.f;
			GetTextSize(Head, TW, TH, Font, 1.f * MS);
			DrawRect(Panel, PX, PY, TW + 24.f * MS, 30.f * MS);
			DrawText(Head, Led, PX + 12.f * MS, PY + 6.f * MS, Font, 1.f * MS);
		}
		else
		{
			DrawRect(Panel, PX, PY, PW, 30.f * MS + LineH * GM->Missions.Num());
			DrawText(Head, Led, PX + 12.f * MS, PY + 6.f * MS, Font, 1.f * MS);
		}
		for (int32 i = 0; i < (bCompact ? 0 : GM->Missions.Num()); ++i)
		{
			const FJRMission& M = GM->Missions[i];
			FString Prog;
			if (M.bDone) Prog = TEXT("[v]");
			else if (M.Kind == EJRMission::NoRun || M.Kind == EJRMission::Happy) Prog = JRT(TEXT("[tutup]"));
			else
			{
				int32 Today = 0;
				const FJRDayStats& St = GM->GetStats();
				switch (M.Kind)
				{
				case EJRMission::Serve: Today = St.Paid; break;
				case EJRMission::Mie: Today = St.MieSold; break;
				case EJRMission::Income: Today = FMath::RoundToInt(St.Income); break;
				case EJRMission::Start: Today = St.Started; break;
				case EJRMission::Repair: Today = St.Repairs; break;
				case EJRMission::Upsell: Today = St.Upsells; break;
				case EJRMission::ExactChange: Today = St.ExactChange; break;
				default: break;
				}
				const int32 Cur = M.bWeekly ? M.Progress + Today : Today;
				Prog = M.Kind == EJRMission::Income ? FString::Printf(TEXT("%d%%"), FMath::Min(100, Cur * 100 / FMath::Max(1, M.Target))) : FString::Printf(TEXT("%d/%d"), FMath::Min(Cur, M.Target), M.Target);
			}
			const FString Line = FString::Printf(TEXT("%s%s  %s"), M.bWeekly ? JRT(TEXT("(Minggu) ")) : TEXT(""), *GM->MissionText(M), *Prog);
			DrawText(Line, M.bDone ? Muted : Ink, PX + 12.f * MS, PY + 28.f * MS + i * LineH, Font, 0.95f * MS);
		}
		// Kabar pagi dan rencana hari ini, satu baris di bawah panel misi
		if (GM->News != EJRNews::None || GM->Plan != EJRPlan::None)
		{
			FString Today = GM->News != EJRNews::None ? AJRGameMode::NewsTitle(GM->News) : FString();
			if (GM->Plan != EJRPlan::None) Today += (Today.IsEmpty() ? FString() : FString(TEXT("  |  "))) + AJRGameMode::PlanTitle(GM->Plan);
			const float TY = PY + (bCompact ? 30.f * MS : 30.f * MS + LineH * GM->Missions.Num()) + 4.f * MS;
			float TW = 0.f, TH = 0.f;
			GetTextSize(Today, TW, TH, Font, 0.9f * MS);
			DrawRect(Panel, PX, TY, TW + 24.f * MS, TH + 8.f * MS);
			DrawText(Today, Led, PX + 12.f * MS, TY + 4.f * MS, Font, 0.9f * MS);
		}
	}

	// Panel tutorial di kiri atas
	if (GM->IsTutorial() && GM->Phase == EJRGamePhase::Playing && !bPhoneOpen)
	{
		FString Title, Body;
		GM->TutorialText(bTouchMode, Title, Body);
		const float PX = (bTouchMode ? 180.f : 16.f) * S, PY = 52.f * S, PW = FMath::Min(520.f * S, W * 0.42f);
		// Bungkus teks sederhana per kata agar muat di panel
		TArray<FString> Words, Lines;
		Body.ParseIntoArray(Words, TEXT(" "));
		FString Cur;
		for (const FString& Wd : Words)
		{
			const FString Try = Cur.IsEmpty() ? Wd : Cur + TEXT(" ") + Wd;
			float TW = 0.f, TH = 0.f;
			GetTextSize(Try, TW, TH, Font, 1.1f * S);
			if (TW > PW - 28.f * S && !Cur.IsEmpty()) { Lines.Add(Cur); Cur = Wd; } else Cur = Try;
		}
		if (!Cur.IsEmpty()) Lines.Add(Cur);
		const float LineH = 24.f * S;
		DrawRect(Panel, PX, PY, PW, 72.f * S + LineH * Lines.Num());
		DrawRect(FLinearColor(0.2f, 1.f, 0.35f), PX, PY, 5.f * S, 72.f * S + LineH * Lines.Num());
		DrawText(Title, FLinearColor(0.2f, 1.f, 0.35f), PX + 16.f * S, PY + 10.f * S, Font, 1.15f * S);
		for (int32 i = 0; i < Lines.Num(); ++i) DrawText(Lines[i], Ink, PX + 16.f * S, PY + 40.f * S + i * LineH, Font, 1.1f * S);
		DrawText(bTouchMode ? JRT(TEXT("Lewati tutorial: tombol II")) : JRT(TEXT("Lewati tutorial: menu jeda (Esc)")), Muted, PX + 16.f * S, PY + 44.f * S + LineH * Lines.Num(), Font, 0.9f * S);
	}

	// Petunjuk interaksi
	if (const AJRCharacter* Ch = bPhoneOpen ? nullptr : Cast<AJRCharacter>(GetOwningPawn()))
	{
		const FJRInteraction& I = Ch->CurrentInteraction;
		if (I.Kind != EJRInteractKind::None)
		{
			FString Text = I.Prompt;
			if (bTouchMode) Text.ReplaceInline(JRT(TEXT("Tahan E:")), JRT(TEXT("Tahan tombol:")));
			else if (I.Kind == EJRInteractKind::Press) Text = TEXT("[E] ") + Text;
			float TW = 0.f, TH = 0.f;
			// Petunjuk panjang diperkecil agar muat di layar (sisakan ruang untuk joystick)
			GetTextSize(Text, TW, TH, Font, 1.3f * S);
			const float MaxW = W * (bTouchMode ? 0.62f : 0.9f);
			const float TS = 1.3f * S * FMath::Min(1.f, MaxW / FMath::Max(TW, 1.f));
			GetTextSize(Text, TW, TH, Font, TS);
			const float X = (W - TW) * 0.5f, Y = H * 0.62f;
			DrawRect(Panel, X - 14.f * S, Y - 8.f * S, TW + 28.f * S, TH + (I.Kind == EJRInteractKind::Hold ? 24.f : 16.f) * S);
			DrawText(Text, I.Kind == EJRInteractKind::Info ? Muted : Ink, X, Y, Font, TS);
			if (I.Kind == EJRInteractKind::Hold && I.HoldTime > 0.f)
			{
				const float Pct = FMath::Clamp(Ch->HoldProgress / I.HoldTime, 0.f, 1.f);
				DrawRect(FLinearColor(0.15f, 0.17f, 0.21f), X, Y + TH + 4.f * S, TW, 5.f * S);
				DrawRect(Crt, X, Y + TH + 4.f * S, TW * Pct, 5.f * S);
			}
		}
	}

	// Notifikasi kanan atas
	// Mode sentuh: paling banyak 3 notifikasi dan lebar dibatasi agar tidak menutupi layar kecil.
	const float MS = S;
	float Y = 56.f * S;
	// Layar sentuh: label NOTIFIKASI di kanan atas; diketuk untuk menyembunyikan/memunculkan notifikasi.
	const bool bToastsHidden = JRSettings::Get()->bToastsHidden;
	if (bTouchMode && GM->Phase == EJRGamePhase::Playing)
	{
		const FString Head = bToastsHidden ? JRPrintf(JRT(TEXT("NOTIFIKASI (%d)  [+]")), GM->Toasts.Num()) : FString(JRT(TEXT("NOTIFIKASI  [-]")));
		float HW = 0.f, HH = 0.f;
		GetTextSize(Head, HW, HH, Font, 0.9f * MS);
		const float HX = W - HW - 32.f * MS;
		DrawRect(Panel, HX - 10.f * MS, Y - 5.f * MS, HW + 20.f * MS, HH + 10.f * MS);
		DrawText(Head, Led, HX, Y, Font, 0.9f * MS);
		Y += HH + 16.f * MS;
	}
	if (bToastsHidden) return;
	int32 Shown = 0;
	for (const FJRToast& T : GM->Toasts)
	{
		if (bTouchMode && Shown++ >= 3) break;
		float TW = 0.f, TH = 0.f;
		GetTextSize(T.Text, TW, TH, Font, 1.f * MS);
		const float TS = MS * FMath::Min(1.f, W * 0.40f / FMath::Max(TW, 1.f));
		GetTextSize(T.Text, TW, TH, Font, TS);
		const float X = W - TW - 32.f * MS;
		DrawRect(Panel, X - 10.f * MS, Y - 5.f * MS, TW + 20.f * MS, TH + 10.f * MS);
		DrawRect(T.Color, X - 10.f * MS, Y - 5.f * MS, 3.f * MS, TH + 10.f * MS);
		DrawText(T.Text, Ink, X, Y, Font, TS);
		Y += TH + 16.f * MS;
	}
}

void AJRHUD::DrawPanel(const FString& Title, const TArray<FString>& Lines, const FString& Footer)
{
	UFont* Font = GEngine->GetMediumFont();
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	// Mode sentuh: panel memakai hampir seluruh lebar layar (kiri: tombol jeda/HP, kanan: tombol LANJUT),
	// baris panjang dibungkus, bukan diperkecil, agar tetap terbaca di HP.
	const float Left = bTouchMode ? 16.f * UIS : 20.f;	// sentuh: mulai di bawah tombol jeda/HP, jadi bisa mentok kiri
	const float Top = bTouchMode ? 150.f * UIS : 50.f * UIS;
	const float Right = bTouchMode ? 175.f * UIS : 20.f;
	const float MaxPW = W - Left - Right;
	float S = UIS;
	TArray<FString> Wrapped;
	auto Wrap = [&](float Scale)
	{
		Wrapped.Reset();
		const float Max = MaxPW - 48.f * Scale;
		for (const FString& L : Lines)
		{
			float TW = 0.f, TH = 0.f;
			GetTextSize(L, TW, TH, Font, 1.15f * Scale);
			if (TW <= Max) { Wrapped.Add(L); continue; }
			TArray<FString> Words;
			L.ParseIntoArray(Words, TEXT(" "), false);	// false: spasi ganda antar petunjuk tetap terjaga
			FString Cur;
			for (const FString& Wd : Words)
			{
				const FString Try = Cur.IsEmpty() ? Wd : Cur + TEXT(" ") + Wd;
				GetTextSize(Try, TW, TH, Font, 1.15f * Scale);
				if (TW > Max && !Cur.IsEmpty()) { Wrapped.Add(Cur); Cur = Wd; } else Cur = Try;
			}
			if (!Cur.IsEmpty()) Wrapped.Add(Cur);
		}
	};
	// Laporan harian bisa panjang (Era 3): kecilkan hanya bila tidak muat tinggi layar.
	const float Avail = H - Top - 16.f * UIS;
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		Wrap(S);
		const float Need = (110.f + Wrapped.Num() * 26.f) * S;
		if (Need <= Avail) break;
		S *= FMath::Max(0.6f, Avail / Need);
	}
	float Widest = 0.f;
	for (const FString& L : Wrapped)
	{
		float TW = 0.f, TH = 0.f;
		GetTextSize(L, TW, TH, Font, 1.15f * S);
		Widest = FMath::Max(Widest, TW);
	}
	const float PW = bTouchMode ? MaxPW : FMath::Min(FMath::Max(900.f * S, Widest + 48.f * S), MaxPW);
	const float LineH = 26.f * S;
	const float PH = 110.f * S + Wrapped.Num() * LineH;
	const float X = bTouchMode ? Left : (W - PW) * 0.5f;
	const float Y = FMath::Max(Top, (H - PH) * 0.5f);

	DrawRect(FLinearColor(0, 0, 0, 0.55f), 0.f, 40.f * UIS, W, H - 40.f * UIS);
	DrawRect(Panel, X, Y, PW, PH);
	DrawText(Title, Led, X + 24.f * S, Y + 14.f * S, Font, 1.6f * S);
	for (int32 i = 0; i < Wrapped.Num(); ++i)
	{
		const bool bHeader = Wrapped[i].StartsWith(JRT(TEXT("Tekan TAB"))) || Wrapped[i].StartsWith(JRT(TEXT("Ketuk tombol HP")));
		DrawText(Wrapped[i], bHeader ? Led : Ink, X + 24.f * S, Y + 60.f * S + i * LineH, Font, 1.15f * S);
	}
	DrawText(Footer, Crt, X + 24.f * S, Y + PH - 36.f * S, Font, 1.2f * S);
}
