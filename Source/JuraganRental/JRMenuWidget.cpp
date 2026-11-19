#include "JRMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "JRGameMode.h"
#include "JRPhoneWidget.h"
#include "JRSettings.h"
#include "JRSound.h"

namespace JRMenuStyle
{
	const FLinearColor Card(0.10f, 0.12f, 0.15f, 0.97f);
	const FLinearColor Ink(0.95f, 0.93f, 0.89f);
	const FLinearColor Muted(0.62f, 0.65f, 0.70f);
	const FLinearColor Led(1.f, 0.76f, 0.29f);
	const FLinearColor Crt(0.37f, 0.70f, 1.f);
	const FLinearColor CrtInk(0.03f, 0.07f, 0.12f);
	const FLinearColor Alt(0.18f, 0.21f, 0.26f);
	const FLinearColor Off(0.13f, 0.14f, 0.17f);

	constexpr bool bMobile = PLATFORM_ANDROID || PLATFORM_IOS;

	FSlateBrush RoundedBrush(float Radius)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		return B;
	}

	UGameUserSettings* Gfx() { return GEngine ? GEngine->GetGameUserSettings() : nullptr; }

	void ApplyGfx()
	{
		if (UGameUserSettings* S = Gfx())
		{
			S->ApplySettings(false);
			S->SaveSettings();
		}
	}

	FString OnOff(bool b) { return b ? TEXT("Nyala") : TEXT("Mati"); }
}
using namespace JRMenuStyle;

TSharedRef<SWidget> UJRMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UJRMenuWidget::BuildTree()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);

	CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CardSize->SetWidthOverride(460.f);
	UOverlaySlot* S = Root->AddChildToOverlay(CardSize);
	S->SetHorizontalAlignment(HAlign_Center);
	S->SetVerticalAlignment(VAlign_Center);

	UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Box->SetBrush(RoundedBrush(18.f));
	Box->SetBrushColor(Card);
	Box->SetPadding(FMargin(32.f, 28.f));
	CardSize->AddChild(Box);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->SetContent(Col);

	TitleText = MakeText(TEXT(""), 30, Led, true);
	TitleText->SetJustification(ETextJustify::Center);
	Col->AddChildToVerticalBox(TitleText);
	SubtitleText = MakeText(TEXT(""), 14, Muted);
	SubtitleText->SetJustification(ETextJustify::Center);
	Col->AddChildToVerticalBox(SubtitleText)->SetPadding(FMargin(0.f, 4.f, 0.f, 20.f));

	// Isi bisa digulir (halaman pengaturan cukup panjang di layar 720p).
	USizeBox* MaxH = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	MaxH->SetMaxDesiredHeight(560.f);
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	MaxH->AddChild(Scroll);
	Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Scroll->AddChild(Content);
	Col->AddChildToVerticalBox(MaxH);
}

UTextBlock* UJRMenuWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Text));
	T->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetAutoWrapText(true);
	return T;
}

void UJRMenuWidget::Begin(EJRMenuPage NewPage, const FString& Title, const FString& Subtitle, float Width)
{
	TakeWidget(); // pastikan pohon widget sudah dibangun
	if (NewPage == EJRMenuPage::Main || NewPage == EJRMenuPage::Pause) ReturnPage = NewPage;
	Page = NewPage;
	CardSize->SetWidthOverride(Width);
	TitleText->SetText(FText::FromString(Title));
	SubtitleText->SetText(FText::FromString(Subtitle));
	SubtitleText->SetVisibility(Subtitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	Content->ClearChildren();
	SetVisibility(ESlateVisibility::Visible);
	if (OnChanged) OnChanged();
}

UJRPhoneButton* UJRMenuWidget::AddButton(const FString& Label, TFunction<void()> Action, bool bPrimary, bool bEnabled)
{
	UJRPhoneButton* B = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
	B->SetBackgroundColor(!bEnabled ? Off : bPrimary ? Crt : Alt);
	B->SetIsEnabled(bEnabled);
	UTextBlock* T = MakeText(Label, 17, !bEnabled ? Muted : bPrimary ? CrtInk : Ink, true);
	T->SetAutoWrapText(false);
	T->SetJustification(ETextJustify::Center);
	if (UButtonSlot* BS = Cast<UButtonSlot>(B->SetContent(T))) BS->SetPadding(FMargin(16.f, 11.f));
	B->Bind(MoveTemp(Action));
	UVerticalBoxSlot* S = Content->AddChildToVerticalBox(B);
	S->SetPadding(FMargin(0.f, Content->GetChildrenCount() > 1 ? 10.f : 0.f, 0.f, 0.f));
	return B;
}

void UJRMenuWidget::AddSection(const FString& Title)
{
	UTextBlock* T = MakeText(Title, 15, Led, true);
	Content->AddChildToVerticalBox(T)->SetPadding(FMargin(0.f, Content->GetChildrenCount() > 1 ? 18.f : 0.f, 0.f, 4.f));
}

void UJRMenuWidget::AddRow(const FString& Label, const FString& Value, TFunction<void()> Action)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* L = MakeText(Label, 15, Ink);
	UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(L);
	LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LS->SetVerticalAlignment(VAlign_Center);

	USizeBox* W = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	W->SetWidthOverride(250.f);
	UJRPhoneButton* B = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
	B->SetBackgroundColor(Alt);
	UTextBlock* T = MakeText(FString::Printf(TEXT("<  %s  >"), *Value), 15, Ink, true);
	T->SetAutoWrapText(false);
	T->SetJustification(ETextJustify::Center);
	if (UButtonSlot* BS = Cast<UButtonSlot>(B->SetContent(T))) BS->SetPadding(FMargin(10.f, 7.f));
	B->Bind([this, Action]() { Action(); ShowSettings(); });
	W->AddChild(B);
	Row->AddChildToHorizontalBox(W)->SetVerticalAlignment(VAlign_Center);
	Content->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
}

/* ---------- Halaman ---------- */

void UJRMenuWidget::ShowMain()
{
	Begin(EJRMenuPage::Main, TEXT("JURAGAN RENTAL"), TEXT("Rental PS & Warnet Simulator"), 460.f);
	AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	const FString Summary = AJRGameMode::SaveSummary();
	AddButton(Summary.IsEmpty() ? FString(TEXT("Lanjutkan")) : FString::Printf(TEXT("Lanjutkan  (%s)"), *Summary), [this, GM]()
	{
		if (GM && GM->LoadProgress()) Close();
	}, true, !Summary.IsEmpty());
	AddButton(TEXT("Main baru"), [this, GM, Summary]()
	{
		if (Summary.IsEmpty())
		{
			if (GM) GM->BeginNewGame();
			Close();
			return;
		}
		ShowConfirm(TEXT("Mulai dari awal?"),
			FString::Printf(TEXT("Simpanan lama (%s) akan ditimpa saat hari pertama selesai."), *Summary),
			TEXT("Ya, mulai baru"), [this, GM]() { if (GM) GM->BeginNewGame(); Close(); });
	}, Summary.IsEmpty());
	AddButton(TEXT("Pengaturan"), [this]() { ShowSettings(); });
	if (!bMobile)
	{
		AddButton(TEXT("Keluar"), [this]()
		{
			UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
		});
	}
	UTextBlock* Hint = MakeText(TEXT("Permainan tersimpan otomatis setiap toko tutup."), 12, Muted);
	Hint->SetJustification(ETextJustify::Center);
	Content->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
}

void UJRMenuWidget::ShowPause()
{
	Begin(EJRMenuPage::Pause, TEXT("Jeda"), TEXT(""), 420.f);
	AddButton(TEXT("Lanjut main"), [this]() { Close(); }, true);
	AddButton(TEXT("Pengaturan"), [this]() { ShowSettings(); });

	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	const bool bSaved = GM && GM->Phase == EJRGamePhase::Report;
	const FString Warn = bSaved
		? FString(TEXT("Progres sampai hari ini sudah tersimpan."))
		: FString(TEXT("Progres hari ini belum tersimpan. Game tersimpan otomatis setiap toko tutup (pukul 24.00)."));
	AddButton(TEXT("Keluar ke menu utama"), [this, Warn]()
	{
		ShowConfirm(TEXT("Keluar ke menu utama?"), Warn, TEXT("Ya, keluar"), [this]()
		{
			UGameplayStatics::SetGamePaused(this, false);
			UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
		});
	});
	if (!bMobile)
	{
		AddButton(TEXT("Keluar dari game"), [this, Warn]()
		{
			ShowConfirm(TEXT("Keluar dari game?"), Warn, TEXT("Ya, keluar"), [this]()
			{
				UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
			});
		});
	}
}

void UJRMenuWidget::ShowConfirm(const FString& Title, const FString& Body, const FString& Yes, TFunction<void()> OnYes)
{
	Begin(EJRMenuPage::Confirm, Title, TEXT(""), 480.f);
	UTextBlock* T = MakeText(Body, 15, Ink);
	Content->AddChildToVerticalBox(T)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	AddButton(Yes, MoveTemp(OnYes), true);
	AddButton(TEXT("Batal"), [this]() { Back(); });
}

void UJRMenuWidget::ShowSettings()
{
	Begin(EJRMenuPage::Settings, TEXT("Pengaturan"), TEXT(""), 620.f);
	UJRSettingsSave* S = JRSettings::Get();

	if (!bMobile)
	{
		if (UGameUserSettings* G = Gfx())
		{
			AddSection(TEXT("GRAFIS"));
			const EWindowMode::Type Mode = G->GetFullscreenMode();
			AddRow(TEXT("Mode layar"),
				Mode == EWindowMode::Fullscreen ? TEXT("Layar penuh") : Mode == EWindowMode::WindowedFullscreen ? TEXT("Penuh tanpa bingkai") : TEXT("Jendela"),
				[G, Mode]()
				{
					G->SetFullscreenMode(Mode == EWindowMode::Fullscreen ? EWindowMode::WindowedFullscreen
						: Mode == EWindowMode::WindowedFullscreen ? EWindowMode::Windowed : EWindowMode::Fullscreen);
					if (G->GetFullscreenMode() != EWindowMode::Windowed) G->SetScreenResolution(G->GetDesktopResolution());
					ApplyGfx();
				});

			TArray<FIntPoint> Res;
			UKismetSystemLibrary::GetSupportedFullscreenResolutions(Res);
			const FIntPoint Desk = G->GetDesktopResolution();
			Res.RemoveAll([Desk](const FIntPoint& P) { return P.X < 1024 || P.X > Desk.X || P.Y > Desk.Y; });
			if (Res.Num() == 0) Res = { FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080) };
			const FIntPoint Cur = G->GetScreenResolution();
			AddRow(TEXT("Resolusi"), FString::Printf(TEXT("%d x %d"), Cur.X, Cur.Y), [G, Res, Cur]()
			{
				int32 Next = 0;
				for (int32 i = 0; i < Res.Num(); ++i) if (Res[i] == Cur) { Next = (i + 1) % Res.Num(); break; }
				G->SetScreenResolution(Res[Next]);
				ApplyGfx();
			});

			static const TCHAR* Levels[] = { TEXT("Rendah"), TEXT("Sedang"), TEXT("Tinggi"), TEXT("Ultra") };
			const int32 Q = G->GetOverallScalabilityLevel();
			AddRow(TEXT("Kualitas grafis"), Q >= 0 && Q <= 3 ? Levels[Q] : TEXT("Kustom"), [G, Q]()
			{
				G->SetOverallScalabilityLevel(Q < 0 || Q >= 3 ? 0 : Q + 1);
				ApplyGfx();
			});
			AddRow(TEXT("VSync"), OnOff(G->IsVSyncEnabled()), [G]() { G->SetVSyncEnabled(!G->IsVSyncEnabled()); ApplyGfx(); });

			const float Fps = G->GetFrameRateLimit();
			AddRow(TEXT("Batas FPS"), Fps <= 0.f ? FString(TEXT("Tanpa batas")) : FString::Printf(TEXT("%d"), FMath::RoundToInt(Fps)), [G, Fps]()
			{
				G->SetFrameRateLimit(Fps <= 0.f ? 30.f : Fps < 45.f ? 60.f : Fps < 90.f ? 120.f : 0.f);
				ApplyGfx();
			});
		}
	}

	AddSection(TEXT("SUARA"));
	AddRow(TEXT("Efek suara"), OnOff(S->bSfx), [S]() { S->bSfx = !S->bSfx; JRSettings::Save(); JRSound::ApplySettings(); });
	AddRow(TEXT("Suara karakter"), OnOff(S->bVoice), [S]() { S->bVoice = !S->bVoice; JRSettings::Save(); });
	AddRow(TEXT("Musik radio"), OnOff(S->bMusic), [S]() { S->bMusic = !S->bMusic; JRSettings::Save(); });

	AddSection(TEXT("KONTROL"));
	AddRow(bMobile ? TEXT("Kepekaan joystick lihat") : TEXT("Kepekaan mouse"), FString::Printf(TEXT("%.2gx"), S->LookSpeed), [S]()
	{
		static const float Steps[] = { 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f };
		int32 Next = 0;
		for (int32 i = 0; i < UE_ARRAY_COUNT(Steps); ++i) if (FMath::IsNearlyEqual(Steps[i], S->LookSpeed)) { Next = (i + 1) % UE_ARRAY_COUNT(Steps); break; }
		S->LookSpeed = Steps[Next];
		JRSettings::Save();
	});
	AddRow(TEXT("Arah lihat atas/bawah"), S->bInvertLook ? TEXT("Dibalik") : TEXT("Normal"), [S]() { S->bInvertLook = !S->bInvertLook; JRSettings::Save(); });

	UJRPhoneButton* B = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
	B->SetBackgroundColor(Crt);
	UTextBlock* T = MakeText(TEXT("Kembali"), 17, CrtInk, true);
	T->SetJustification(ETextJustify::Center);
	if (UButtonSlot* BS = Cast<UButtonSlot>(B->SetContent(T))) BS->SetPadding(FMargin(16.f, 11.f));
	B->Bind([this]() { Back(); });
	Content->AddChildToVerticalBox(B)->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
}

void UJRMenuWidget::Close()
{
	Page = EJRMenuPage::None;
	SetVisibility(ESlateVisibility::Collapsed);
	if (OnChanged) OnChanged();
}

bool UJRMenuWidget::Back()
{
	switch (Page)
	{
	case EJRMenuPage::Settings:
	case EJRMenuPage::Confirm:
		if (ReturnPage == EJRMenuPage::Main) ShowMain(); else ShowPause();
		return false;
	case EJRMenuPage::Pause:
		Close();
		return true;
	default:
		return false;	// menu utama tidak bisa ditutup dengan Esc
	}
}
