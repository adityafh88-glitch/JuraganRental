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
#include "Kismet/GameplayStatics.h"
#include "JRSound.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/TouchInterface.h"
#include "Misc/CommandLine.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

namespace JRLocal_HUD
{
	const FLinearColor Led(1.f, 0.76f, 0.29f);
	const FLinearColor Ink(0.95f, 0.93f, 0.89f);
	const FLinearColor Muted(0.64f, 0.66f, 0.70f);
	const FLinearColor Panel(0.07f, 0.08f, 0.10f, 0.85f);
	const FLinearColor Crt(0.37f, 0.70f, 1.f);
}
using namespace JRLocal_HUD;

void AJRHUD::BeginPlay()
{
	Super::BeginPlay();
	if (PlayerOwner)
	{
		Phone = CreateWidget<UJRPhoneWidget>(PlayerOwner, UJRPhoneWidget::StaticClass());
		if (Phone)
		{
			Phone->AddToViewport(10);
			Phone->SetVisibility(ESlateVisibility::Collapsed);
		}
		// Mode sentuh: otomatis di Android/iOS, atau paksa di PC dengan -jrtouch.
		bTouchMode = PLATFORM_ANDROID || PLATFORM_IOS || FParse::Param(FCommandLine::Get(), TEXT("jrtouch"));
		if (bTouchMode)
		{
			UInputSettings* Settings = GetMutableDefault<UInputSettings>();
			Settings->bAlwaysShowTouchInterface = true;
			Settings->bUseMouseForTouch = !(PLATFORM_ANDROID || PLATFORM_IOS); // uji di PC pakai mouse
			ApplyTouchSettings();
			Touch = CreateWidget<UJRTouchWidget>(PlayerOwner, UJRTouchWidget::StaticClass());
			if (Touch) Touch->AddToViewport(5);
		}

		Menu = CreateWidget<UJRMenuWidget>(PlayerOwner, UJRMenuWidget::StaticClass());
		if (Menu)
		{
			Menu->AddToViewport(30);
			Menu->SetVisibility(ESlateVisibility::Collapsed);
			Menu->OnChanged = [this]() { OnMenuChanged(); };
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
}

void AJRHUD::SyncChoice()
{
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!Choice || !GM) return;
	// Dialog berganti tanpa jeda (mis. kejadian berantai): tampilkan yang baru.
	if (bChoiceShown && GM->Choices.Num() > 0 && GM->Choices[0].Serial != ShownChoiceSerial) bChoiceShown = false;
	if (!bChoiceShown && GM->Choices.Num() > 0)
	{
		ShownChoiceSerial = GM->Choices[0].Serial;
		if (bPhoneOpen) SetPhoneOpen(false);
		const FJRChoice& C = GM->Choices[0];
		Choice->Show(C.Title, C.Body, C.Labels);
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
	return bPhoneOpen || bChoiceShown || (Menu && Menu->IsOpen());
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
	if (Menu->IsOpen()) { Menu->Back(); return; }
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
	for (FTouchInputControl& C : Sticks->Controls)
	{
		// Joystick lihat bawaan Unreal terbalik untuk kamera orang pertama: geser ke atas harus lihat ke atas.
		if (C.MainInputKey == EKeys::Gamepad_RightX) C.InputScale.Y = -C.InputScale.Y;
	}
	PlayerOwner->ActivateTouchInterface(Sticks);
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

void AJRHUD::DrawHUD()
{
	Super::DrawHUD();
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!GM || !Canvas) return;

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
	UFont* Font = GEngine->GetMediumFont();
	const float W = Canvas->ClipX, H = Canvas->ClipY;

	// Status atas
	const int32 Hr = FMath::FloorToInt(GM->Hour);
	const int32 Mn = FMath::FloorToInt((GM->Hour - Hr) * 60.f);
	const FString Power = GM->bMcbTripped ? FString(JRT(TEXT("MCB TURUN"))) : JRPrintf(JRT(TEXT("%d/%d VA")), GM->PowerVA, GM->CapVA);
	const FString Top = JRPrintf(JRT(TEXT("%s  Hari %d   %02d:%02d   Kas %s   Reputasi %d   Daya %s")),
		*GM->Layout().Name, GM->Day, Hr, Mn, *JRFormatRp(GM->Money), FMath::RoundToInt(GM->Rep), *Power);
	DrawRect(Panel, 0.f, 0.f, W, 40.f);
	DrawText(Top, Led, 16.f, 9.f, Font, 1.25f);

	if (GM->Phase == EJRGamePhase::Intro)
	{
		DrawPanel(JRT(TEXT("Juragan Rental")), {
			JRT(TEXT("Kamu baru membuka rental PS di garasi rumah: dua TV tabung, satu PC warnet,")),
			JRT(TEXT("dan MCB 1.300 VA yang gampang turun.")),
			TEXT(""),
			bTouchMode ? JRT(TEXT("Joystick kiri: jalan    Joystick kanan: lihat    Tombol aksi: interaksi    HP: belanja, tarif    II: jeda"))
				: JRT(TEXT("WASD: jalan    Mouse: lihat    E: interaksi (tahan untuk memperbaiki)    Tab: HP    Esc: menu jeda")),
			TEXT(""),
			JRT(TEXT("Lihat ke TV yang ditunggu pelanggan lalu tekan E untuk menyalakan.")),
			JRT(TEXT("Pelanggan selesai main akan antre di kasir dekat pintu. Jangan biarkan kasir kosong.")),
			JRT(TEXT("Target: Rp 600.000 dan reputasi 40 untuk membuka Gerbang 1.")),
		}, bTouchMode ? JRT(TEXT("Ketuk LANJUT untuk buka toko")) : JRT(TEXT("Tekan ENTER untuk buka toko")));
		return;
	}
	if (GM->Phase == EJRGamePhase::Report)
	{
		if (!bPhoneOpen)
		{
			TArray<FString> Lines = GM->ReportLines;
			Lines.Add(TEXT(""));
			Lines.Add(bTouchMode ? JRT(TEXT("Ketuk tombol HP untuk belanja barang dan atur tarif.")) : JRT(TEXT("Tekan TAB untuk buka HP: belanja barang dan atur tarif.")));
			DrawPanel(JRPrintf(JRT(TEXT("Hari %d selesai")), GM->Day), Lines,
				bTouchMode ? JRPrintf(JRT(TEXT("Ketuk LANJUT untuk buka toko hari %d")), GM->Day + 1) : JRPrintf(JRT(TEXT("Tekan ENTER untuk buka toko hari %d")), GM->Day + 1));
		}
		return;
	}

	// Titik bidik dan petunjuk HP
	if (!bPhoneOpen)
	{
		DrawRect(FLinearColor(1, 1, 1, 0.8f), W * 0.5f - 2.f, H * 0.5f - 2.f, 4.f, 4.f);
	}
	if (!bTouchMode)
	{
		DrawRect(Panel, 12.f, H - 44.f, 150.f, 32.f);
		DrawText(bPhoneOpen ? JRT(TEXT("[Tab] Tutup HP")) : JRT(TEXT("[Tab] Buka HP")), Led, 24.f, H - 38.f, Font, 1.1f);
	}

	// Panel tutorial di kiri atas
	if (GM->IsTutorial() && GM->Phase == EJRGamePhase::Playing && !bPhoneOpen)
	{
		FString Title, Body;
		GM->TutorialText(bTouchMode, Title, Body);
		const float PX = bTouchMode ? 100.f : 16.f, PY = 52.f, PW = FMath::Min(520.f, W * 0.42f);
		// Bungkus teks sederhana per kata agar muat di panel
		TArray<FString> Words, Lines;
		Body.ParseIntoArray(Words, TEXT(" "));
		FString Cur;
		for (const FString& Wd : Words)
		{
			const FString Try = Cur.IsEmpty() ? Wd : Cur + TEXT(" ") + Wd;
			float TW = 0.f, TH = 0.f;
			GetTextSize(Try, TW, TH, Font, 1.1f);
			if (TW > PW - 28.f && !Cur.IsEmpty()) { Lines.Add(Cur); Cur = Wd; } else Cur = Try;
		}
		if (!Cur.IsEmpty()) Lines.Add(Cur);
		const float LineH = 24.f;
		DrawRect(Panel, PX, PY, PW, 46.f + LineH * Lines.Num() + 26.f);
		DrawRect(FLinearColor(0.2f, 1.f, 0.35f), PX, PY, 5.f, 46.f + LineH * Lines.Num() + 26.f);
		DrawText(Title, FLinearColor(0.2f, 1.f, 0.35f), PX + 16.f, PY + 10.f, Font, 1.15f);
		for (int32 i = 0; i < Lines.Num(); ++i) DrawText(Lines[i], Ink, PX + 16.f, PY + 40.f + i * LineH, Font, 1.1f);
		DrawText(JRT(TEXT("Lewati tutorial: menu jeda (Esc)")), Muted, PX + 16.f, PY + 44.f + LineH * Lines.Num(), Font, 0.9f);
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
			GetTextSize(Text, TW, TH, Font, 1.3f);
			const float X = (W - TW) * 0.5f, Y = H * 0.62f;
			DrawRect(Panel, X - 14.f, Y - 8.f, TW + 28.f, TH + (I.Kind == EJRInteractKind::Hold ? 24.f : 16.f));
			DrawText(Text, I.Kind == EJRInteractKind::Info ? Muted : Ink, X, Y, Font, 1.3f);
			if (I.Kind == EJRInteractKind::Hold && I.HoldTime > 0.f)
			{
				const float Pct = FMath::Clamp(Ch->HoldProgress / I.HoldTime, 0.f, 1.f);
				DrawRect(FLinearColor(0.15f, 0.17f, 0.21f), X, Y + TH + 4.f, TW, 5.f);
				DrawRect(Crt, X, Y + TH + 4.f, TW * Pct, 5.f);
			}
		}
	}

	// Notifikasi kanan atas
	float Y = 56.f;
	for (const FJRToast& T : GM->Toasts)
	{
		float TW = 0.f, TH = 0.f;
		GetTextSize(T.Text, TW, TH, Font, 1.f);
		const float X = W - TW - 32.f;
		DrawRect(Panel, X - 10.f, Y - 5.f, TW + 20.f, TH + 10.f);
		DrawRect(T.Color, X - 10.f, Y - 5.f, 3.f, TH + 10.f);
		DrawText(T.Text, Ink, X, Y, Font, 1.f);
		Y += TH + 16.f;
	}
}

void AJRHUD::DrawPanel(const FString& Title, const TArray<FString>& Lines, const FString& Footer)
{
	UFont* Font = GEngine->GetMediumFont();
	UFont* Big = GEngine->GetLargeFont();
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float PW = FMath::Min(900.f, W - 40.f);
	const float LineH = 26.f;
	const float PH = 110.f + Lines.Num() * LineH;
	const float X = (W - PW) * 0.5f, Y = FMath::Max(50.f, (H - PH) * 0.5f);

	DrawRect(FLinearColor(0, 0, 0, 0.55f), 0.f, 40.f, W, H - 40.f);
	DrawRect(Panel, X, Y, PW, PH);
	DrawText(Title, Ink, X + 24.f, Y + 18.f, Big, 1.f);
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		const bool bHeader = Lines[i].StartsWith(JRT(TEXT("Tekan TAB"))) || Lines[i].StartsWith(JRT(TEXT("Ketuk tombol HP")));
		DrawText(Lines[i], bHeader ? Led : Ink, X + 24.f, Y + 60.f + i * LineH, Font, 1.15f);
	}
	DrawText(Footer, Crt, X + 24.f, Y + PH - 36.f, Font, 1.2f);
}
