#include "JRHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "JRCharacter.h"
#include "JRGameMode.h"
#include "JRPhoneWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor Led(1.f, 0.76f, 0.29f);
	const FLinearColor Ink(0.95f, 0.93f, 0.89f);
	const FLinearColor Muted(0.64f, 0.66f, 0.70f);
	const FLinearColor Panel(0.07f, 0.08f, 0.10f, 0.85f);
	const FLinearColor Crt(0.37f, 0.70f, 1.f);
}

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
	}
}

void AJRHUD::TogglePhone()
{
	SetPhoneOpen(!bPhoneOpen);
}

void AJRHUD::SetPhoneOpen(bool bOpen)
{
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (bOpen && (!GM || GM->Phase == EJRGamePhase::Intro)) return;
	if (!Phone || !PlayerOwner) return;
	bPhoneOpen = bOpen;
	Phone->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (bOpen)
	{
		Phone->Refresh(true);
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

void AJRHUD::DrawHUD()
{
	Super::DrawHUD();
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!GM || !Canvas) return;

	UFont* Font = GEngine->GetMediumFont();
	const float W = Canvas->ClipX, H = Canvas->ClipY;

	// Status atas
	const int32 Hr = FMath::FloorToInt(GM->Hour);
	const int32 Mn = FMath::FloorToInt((GM->Hour - Hr) * 60.f);
	const FString Power = GM->bMcbTripped ? FString(TEXT("MCB TURUN")) : FString::Printf(TEXT("%d/%d VA"), GM->PowerVA, GM->CapVA);
	const FString Top = FString::Printf(TEXT("GARASI  Hari %d   %02d:%02d   Kas %s   Reputasi %d   Daya %s"),
		GM->Day, Hr, Mn, *JRFormatRp(GM->Money), FMath::RoundToInt(GM->Rep), *Power);
	DrawRect(Panel, 0.f, 0.f, W, 40.f);
	DrawText(Top, Led, 16.f, 9.f, Font, 1.25f);

	if (GM->Phase == EJRGamePhase::Intro)
	{
		DrawPanel(TEXT("Juragan Rental"), {
			TEXT("Kamu baru membuka rental PS di garasi rumah: dua TV tabung, satu PC warnet,"),
			TEXT("dan MCB 1.300 VA yang gampang turun."),
			TEXT(""),
			TEXT("WASD: jalan    Mouse: lihat    E: interaksi (tahan untuk memperbaiki)    Tab: HP (belanja, tarif)"),
			TEXT(""),
			TEXT("Lihat ke TV yang ditunggu pelanggan lalu tekan E untuk menyalakan."),
			TEXT("Pelanggan selesai main akan antre di kasir dekat pintu. Jangan biarkan kasir kosong."),
			TEXT("Target: Rp 1.500.000 dan reputasi 40 untuk membuka Gerbang 1."),
		}, TEXT("Tekan ENTER untuk buka toko"));
		return;
	}
	if (GM->Phase == EJRGamePhase::Report)
	{
		if (!bPhoneOpen)
		{
			TArray<FString> Lines = GM->ReportLines;
			Lines.Add(TEXT(""));
			Lines.Add(TEXT("Tekan TAB untuk buka HP: belanja barang dan atur tarif."));
			DrawPanel(FString::Printf(TEXT("Hari %d selesai"), GM->Day), Lines, FString::Printf(TEXT("Tekan ENTER untuk buka toko hari %d"), GM->Day + 1));
		}
		return;
	}

	// Titik bidik dan petunjuk HP
	if (!bPhoneOpen)
	{
		DrawRect(FLinearColor(1, 1, 1, 0.8f), W * 0.5f - 2.f, H * 0.5f - 2.f, 4.f, 4.f);
	}
	DrawRect(Panel, 12.f, H - 44.f, 150.f, 32.f);
	DrawText(bPhoneOpen ? TEXT("[Tab] Tutup HP") : TEXT("[Tab] Buka HP"), Led, 24.f, H - 38.f, Font, 1.1f);

	// Petunjuk interaksi
	if (const AJRCharacter* Ch = bPhoneOpen ? nullptr : Cast<AJRCharacter>(GetOwningPawn()))
	{
		const FJRInteraction& I = Ch->CurrentInteraction;
		if (I.Kind != EJRInteractKind::None)
		{
			const FString Text = I.Kind == EJRInteractKind::Info ? I.Prompt : (I.Kind == EJRInteractKind::Press ? TEXT("[E] ") + I.Prompt : I.Prompt);
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
		const bool bHeader = Lines[i].StartsWith(TEXT("Tekan TAB"));
		DrawText(Lines[i], bHeader ? Led : Ink, X + 24.f, Y + 60.f + i * LineH, Font, 1.15f);
	}
	DrawText(Footer, Crt, X + 24.f, Y + PH - 36.f, Font, 1.2f);
}
