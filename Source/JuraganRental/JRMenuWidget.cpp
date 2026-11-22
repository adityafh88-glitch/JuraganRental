#include "JRMenuWidget.h"
#include "JRLang.h"

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
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Styling/CoreStyle.h"
#include "JRGameMode.h"
#include "JRHUD.h"
#include "JRPhoneWidget.h"
#include "JRSettings.h"
#include "JRStyle.h"
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

	FString OnOff(bool b) { return b ? JRT(TEXT("Nyala")) : JRT(TEXT("Mati")); }
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
	Box->SetBrush(JRStyle::Panel(20.f));
	Box->SetBrushColor(FLinearColor::White);
	Box->SetPadding(FMargin(32.f, 28.f));
	CardSize->AddChild(Box);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->SetContent(Col);

	// Logo judul di menu utama (halaman lain memakai judul teks)
	LogoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Logo/Logo_Judul.Logo_Judul"))) LogoImage->SetBrushFromTexture(Tex);
	{
		FSlateBrush B = LogoImage->GetBrush();
		B.ImageSize = FVector2D(320.f, 180.f);	// bawaan brush 32x32
		LogoImage->SetBrush(B);
	}
	Col->AddChildToVerticalBox(LogoImage)->SetHorizontalAlignment(HAlign_Center);

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
	const bool bLogo = NewPage == EJRMenuPage::Main && LogoImage && LogoImage->GetBrush().GetResourceObject();
	if (LogoImage) LogoImage->SetVisibility(bLogo ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	TitleText->SetVisibility(bLogo ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	SubtitleText->SetText(FText::FromString(Subtitle));
	SubtitleText->SetVisibility(Subtitle.IsEmpty() || bLogo ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);	// logo sudah memuat subjudul
	Content->ClearChildren();
	SetVisibility(ESlateVisibility::Visible);
	if (OnChanged) OnChanged();
}

UJRPhoneButton* UJRMenuWidget::AddButton(const FString& Label, TFunction<void()> Action, bool bPrimary, bool bEnabled)
{
	UJRPhoneButton* B = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
	B->SetIsEnabled(bEnabled);
	UTextBlock* T = MakeText(Label, 17, Ink, true);
	T->SetAutoWrapText(false);
	T->SetJustification(ETextJustify::Center);
	if (UButtonSlot* BS = Cast<UButtonSlot>(B->SetContent(T))) BS->SetPadding(FMargin(16.f, 11.f));
	JRStyle::Apply(B, !bEnabled ? JRStyle::EBtn::Disabled : bPrimary ? JRStyle::EBtn::Primary : JRStyle::EBtn::Secondary);
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
	UTextBlock* T = MakeText(FString::Printf(TEXT("<  %s  >"), *Value), 15, Ink, true);
	T->SetAutoWrapText(false);
	T->SetJustification(ETextJustify::Center);
	if (UButtonSlot* BS = Cast<UButtonSlot>(B->SetContent(T))) BS->SetPadding(FMargin(10.f, 7.f));
	JRStyle::Apply(B, JRStyle::EBtn::Secondary, 12.f);
	B->Bind([this, Action]() { Action(); ShowSettings(); });
	W->AddChild(B);
	Row->AddChildToHorizontalBox(W)->SetVerticalAlignment(VAlign_Center);
	Content->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
}

/* ---------- Halaman ---------- */

void UJRMenuWidget::ShowMain()
{
	Begin(EJRMenuPage::Main, JRT(TEXT("JURAGAN RENTAL")), JRT(TEXT("Rental PS & Warnet Simulator")), 460.f);
	AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	const FString Summary = AJRGameMode::SaveSummary();
	AddButton(Summary.IsEmpty() ? FString(JRT(TEXT("Lanjutkan"))) : JRPrintf(JRT(TEXT("Lanjutkan\n%s")), *Summary), [this, GM]()
	{
		if (GM && GM->LoadProgress()) Close();
	}, true, !Summary.IsEmpty());
	AddButton(JRT(TEXT("Main baru")), [this, GM, Summary]()
	{
		if (Summary.IsEmpty())
		{
			if (GM) GM->BeginNewGame();
			Close();
			return;
		}
		ShowConfirm(JRT(TEXT("Mulai dari awal?")),
			JRPrintf(JRT(TEXT("Simpanan lama (%s) akan ditimpa begitu permainan baru tersimpan otomatis.")), *Summary),
			JRT(TEXT("Ya, mulai baru")), [this, GM]() { if (GM) GM->BeginNewGame(); Close(); });
	}, Summary.IsEmpty());
	AddButton(JRT(TEXT("Pengaturan")), [this]() { ShowSettings(); });
	if (!bMobile)
	{
		AddButton(JRT(TEXT("Keluar")), [this]()
		{
			UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
		});
	}
	UTextBlock* Hint = MakeText(JRT(TEXT("Permainan tersimpan otomatis tiap 2 jam dalam game, saat toko tutup, dan saat keluar.")), 12, Muted);
	Hint->SetJustification(ETextJustify::Center);
	Content->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
}

void UJRMenuWidget::ShowPause()
{
	Begin(EJRMenuPage::Pause, JRT(TEXT("Jeda")), TEXT(""), 420.f);
	AddButton(JRT(TEXT("Lanjut main")), [this]() { Close(); }, true);
	AddButton(JRT(TEXT("Pengaturan")), [this]() { ShowSettings(); });

	AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (GM && GM->IsTutorial())
	{
		AddButton(JRT(TEXT("Lewati tutorial")), [this, GM]() { GM->SkipTutorial(); Close(); });
	}
	if (GM && GM->Phase == EJRGamePhase::Playing)
	{
		AddButton(JRT(TEXT("Mode foto (sembunyikan HUD)")), [this]()
		{
			AJRHUD* Hud = GetOwningPlayer() ? Cast<AJRHUD>(GetOwningPlayer()->GetHUD()) : nullptr;
			Close();
			if (Hud) Hud->SetPhotoMode(true);
		});
	}
	const FString Warn = JRT(TEXT("Permainan disimpan dulu, lalu kamu bisa lanjut dari jam yang sama lewat Lanjutkan."));
	AddButton(JRT(TEXT("Keluar ke menu utama")), [this, Warn, GM]()
	{
		ShowConfirm(JRT(TEXT("Keluar ke menu utama?")), Warn, JRT(TEXT("Ya, keluar")), [this, GM]()
		{
			if (GM) GM->SaveNow();
			UGameplayStatics::SetGamePaused(this, false);
			UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
		});
	});
	if (!bMobile)
	{
		AddButton(JRT(TEXT("Keluar dari game")), [this, Warn, GM]()
		{
			ShowConfirm(JRT(TEXT("Keluar dari game?")), Warn, JRT(TEXT("Ya, keluar")), [this, GM]()
			{
				if (GM) GM->SaveNow();
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
	AddButton(JRT(TEXT("Batal")), [this]() { Back(); });
}

void UJRMenuWidget::ShowDemoEnd()
{
	Begin(EJRMenuPage::DemoEnd, JRT(TEXT("Terima kasih sudah main!")), JRT(TEXT("Akhir demo Juragan Rental")), 520.f);
	AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (GM)
	{
		for (const FString& L : GM->DemoSummary)
		{
			Content->AddChildToVerticalBox(MakeText(L, 15, Ink))->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}
	}
	UTextBlock* Ask = MakeText(JRT(TEXT("Suka? Bantu kami dengan menambahkan Juragan Rental ke wishlist Steam-mu.")), 15, Led, true);
	Content->AddChildToVerticalBox(Ask)->SetPadding(FMargin(0.f, 8.f, 0.f, 16.f));
	if (GM && !GM->DemoStoreURL.IsEmpty())
	{
		const FString Url = GM->DemoStoreURL;
		AddButton(TEXT("Wishlist di Steam"), [Url]() { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); }, true);
	}
	AddButton(JRT(TEXT("Kembali ke menu utama")), [this]()
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
	});
	if (!bMobile)
	{
		AddButton(JRT(TEXT("Keluar")), [this]() { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); });
	}
}

void UJRMenuWidget::ShowSettings()
{
	Begin(EJRMenuPage::Settings, JRT(TEXT("Pengaturan")), TEXT(""), 620.f);
	UJRSettingsSave* S = JRSettings::Get();

	if (!bMobile)
	{
		if (UGameUserSettings* G = Gfx())
		{
			AddSection(JRT(TEXT("GRAFIS")));
			const EWindowMode::Type Mode = G->GetFullscreenMode();
			AddRow(JRT(TEXT("Mode layar")),
				Mode == EWindowMode::Fullscreen ? JRT(TEXT("Layar penuh")) : Mode == EWindowMode::WindowedFullscreen ? JRT(TEXT("Penuh tanpa bingkai")) : JRT(TEXT("Jendela")),
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
			AddRow(JRT(TEXT("Resolusi")), FString::Printf(TEXT("%d x %d"), Cur.X, Cur.Y), [G, Res, Cur]()
			{
				int32 Next = 0;
				for (int32 i = 0; i < Res.Num(); ++i) if (Res[i] == Cur) { Next = (i + 1) % Res.Num(); break; }
				G->SetScreenResolution(Res[Next]);
				ApplyGfx();
			});

			const TCHAR* Levels[] = { JRT(TEXT("Rendah")), JRT(TEXT("Sedang")), JRT(TEXT("Tinggi")), JRT(TEXT("Ultra")) };
			const int32 Q = G->GetOverallScalabilityLevel();
			AddRow(JRT(TEXT("Kualitas grafis")), Q >= 0 && Q <= 3 ? Levels[Q] : JRT(TEXT("Kustom")), [G, Q]()
			{
				G->SetOverallScalabilityLevel(Q < 0 || Q >= 3 ? 0 : Q + 1);
				ApplyGfx();
			});
			AddRow(TEXT("VSync"), OnOff(G->IsVSyncEnabled()), [G]() { G->SetVSyncEnabled(!G->IsVSyncEnabled()); ApplyGfx(); });

			const float Fps = G->GetFrameRateLimit();
			AddRow(JRT(TEXT("Batas FPS")), Fps <= 0.f ? FString(JRT(TEXT("Tanpa batas"))) : FString::Printf(TEXT("%d"), FMath::RoundToInt(Fps)), [G, Fps]()
			{
				G->SetFrameRateLimit(Fps <= 0.f ? 30.f : Fps < 45.f ? 60.f : Fps < 90.f ? 120.f : 0.f);
				ApplyGfx();
			});
		}
	}

	if (bMobile || FParse::Param(FCommandLine::Get(), TEXT("jrgfx")))
	{
		AddSection(JRT(TEXT("GRAFIS")));
		AddRow(JRT(TEXT("Kualitas grafis")), JRSettings::GfxPresetName(JRSettings::GfxPreset()), [S]()
		{
			S->GfxPreset = (JRSettings::GfxPreset() + 1) % 3;
			JRSettings::Save();
			JRSettings::ApplyGraphics();
		});
		AddRow(JRT(TEXT("Batas FPS")), S->MaxFps <= 30 ? FString(TEXT("30")) : FString(TEXT("60")), [S]()
		{
			S->MaxFps = S->MaxFps <= 30 ? 60 : 30;
			JRSettings::Save();
			JRSettings::ApplyGraphics();
		});
		UTextBlock* Note = MakeText(JRT(TEXT("Kalau game terasa patah-patah atau HP panas, pilih Hemat dan batas 30 FPS.")), 12, Muted);
		Content->AddChildToVerticalBox(Note)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	AddRow(JRT(TEXT("Tampilkan FPS")), OnOff(S->bShowFps), [S]() { S->bShowFps = !S->bShowFps; JRSettings::Save(); });

	AddSection(JRT(TEXT("SUARA")));
	AddRow(JRT(TEXT("Efek suara")), OnOff(S->bSfx), [S]() { S->bSfx = !S->bSfx; JRSettings::Save(); JRSound::ApplySettings(); });
	AddRow(JRT(TEXT("Suara karakter")), OnOff(S->bVoice), [S]() { S->bVoice = !S->bVoice; JRSettings::Save(); });
	AddRow(JRT(TEXT("Musik radio")), OnOff(S->bMusic), [S]() { S->bMusic = !S->bMusic; JRSettings::Save(); });

	AddSection(JRT(TEXT("KONTROL")));
	AddRow(bMobile ? JRT(TEXT("Kepekaan geser layar")) : JRT(TEXT("Kepekaan mouse")), FString::Printf(TEXT("%.2gx"), S->LookSpeed), [S]()
	{
		static const float Steps[] = { 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f };
		int32 Next = 0;
		for (int32 i = 0; i < UE_ARRAY_COUNT(Steps); ++i) if (FMath::IsNearlyEqual(Steps[i], S->LookSpeed)) { Next = (i + 1) % UE_ARRAY_COUNT(Steps); break; }
		S->LookSpeed = Steps[Next];
		JRSettings::Save();
	});
	AddRow(JRT(TEXT("Arah lihat atas/bawah")), S->bInvertLook ? JRT(TEXT("Dibalik")) : JRT(TEXT("Normal")), [S]() { S->bInvertLook = !S->bInvertLook; JRSettings::Save(); });
	AddRow(JRT(TEXT("Daftar misi")), S->bMissionsCompact ? JRT(TEXT("Ringkas")) : JRT(TEXT("Lengkap")), [S]() { S->bMissionsCompact = !S->bMissionsCompact; JRSettings::Save(); });
	AddRow(JRT(TEXT("Notifikasi")), S->bToastsHidden ? JRT(TEXT("Disembunyikan")) : JRT(TEXT("Ditampilkan")), [S]() { S->bToastsHidden = !S->bToastsHidden; JRSettings::Save(); });
	AJRHUD* Hud = GetOwningPlayer() ? Cast<AJRHUD>(GetOwningPlayer()->GetHUD()) : nullptr;
	if (bMobile || (Hud && Hud->IsTouchMode()))
	{
		AddRow(JRT(TEXT("Ukuran tampilan")), JRSettings::UIScaleName(S->UIScale), [S, Hud]()
		{
			S->UIScale = JRSettings::NextUIScale(S->UIScale);
			JRSettings::Save();
			if (Hud) Hud->ApplyUIScale();
		});
		AddRow(JRT(TEXT("Ukuran joystick")), JRSettings::JoystickSizeName(S->JoystickSize), [S, Hud]()
		{
			S->JoystickSize = JRSettings::NextJoystickSize(S->JoystickSize);
			JRSettings::Save();
			if (Hud) Hud->ApplyTouchSettings();
		});
	}

	// Sengaja dua bahasa agar mudah ditemukan apa pun bahasa yang aktif.
	AddSection(JRT(TEXT("BAHASA / LANGUAGE")));
	{
		const TCHAR* Names[] = { JRT(TEXT("Otomatis / Auto")), JRT(TEXT("Indonesia")), JRT(TEXT("English")) };
		AddRow(JRT(TEXT("Bahasa / Language")), Names[FMath::Clamp(S->Language, 0, 2)], [S, Hud]()
		{
			S->Language = (S->Language + 1) % 3;
			JRSettings::Save();
			JRLang::Reset();	// langsung berlaku: halaman ini dibangun ulang setelah aksi, HUD membangun ulang widget lain
			if (Hud) Hud->OnLanguageChanged();
		});
	}

	UJRPhoneButton* B = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
	UTextBlock* T = MakeText(JRT(TEXT("Kembali")), 17, Ink, true);
	T->SetJustification(ETextJustify::Center);
	if (UButtonSlot* BS = Cast<UButtonSlot>(B->SetContent(T))) BS->SetPadding(FMargin(16.f, 11.f));
	JRStyle::Apply(B, JRStyle::EBtn::Primary);
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
		return false;	// menu utama dan akhir demo tidak bisa ditutup dengan Esc
	}
}
