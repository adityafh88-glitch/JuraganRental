#include "JRSplashWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "JRSound.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace JRLocal_Splash
{
	// Membesar melewati ukuran akhir lalu kembali (efek pop/membal). P dari 0 sampai 1.
	float EaseOutBack(float P)
	{
		P = FMath::Clamp(P, 0.f, 1.f);
		const float C1 = 2.2f, C3 = C1 + 1.f;
		return 1.f + C3 * FMath::Pow(P - 1.f, 3.f) + C1 * FMath::Pow(P - 1.f, 2.f);
	}

	// Jadwal (detik)
	constexpr float TileStart = 0.35f, TileGap = 0.24f, PopTime = 0.42f;
	constexpr float GamedevStart = 1.2f;
	constexpr float KahFadeStart = 2.5f, KahFadeEnd = 2.85f;
	constexpr float TitleStart = 3.0f, TitlePop = 0.6f;
	constexpr float FadeStart = 5.0f, FadeEnd = 5.45f;
}
using namespace JRLocal_Splash;

TSharedRef<SWidget> UJRSplashWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildTree();
	return Super::RebuildWidget();
}

UImage* UJRSplashWidget::MakeImage(const TCHAR* Asset, const FVector2D& Size)
{
	UImage* I = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, Asset)) I->SetBrushFromTexture(Tex);
	// Ukuran gambar diatur lewat brush (bawaan brush 32x32 piksel).
	FSlateBrush B = I->GetBrush();
	B.ImageSize = Size;
	I->SetBrush(B);
	I->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	I->SetRenderScale(FVector2D::ZeroVector);	// muncul lewat animasi
	return I;
}

void UJRSplashWidget::BuildTree()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Bg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Bg->SetBrushColor(FLinearColor(0.0056f, 0.0065f, 0.0080f, 1.f));	// #111317
	UOverlaySlot* BgSlot = Root->AddChildToOverlay(Bg);
	BgSlot->SetHorizontalAlignment(HAlign_Fill);
	BgSlot->SetVerticalAlignment(VAlign_Fill);

	// KAH Gamedev
	UVerticalBox* Kah = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	static const TCHAR* TileAssets[] = { TEXT("/Game/UI/Logo/KAH_K.KAH_K"), TEXT("/Game/UI/Logo/KAH_A.KAH_A"), TEXT("/Game/UI/Logo/KAH_H.KAH_H") };
	for (int32 i = 0; i < 3; ++i)
	{
		UImage* T = MakeImage(TileAssets[i], FVector2D(150.f, 150.f));
		Tiles.Add(T);
		Row->AddChildToHorizontalBox(T)->SetPadding(FMargin(i == 0 ? 0.f : 14.f, 0.f, 0.f, 0.f));
	}
	Kah->AddChildToVerticalBox(Row)->SetHorizontalAlignment(HAlign_Center);
	Gamedev = MakeImage(TEXT("/Game/UI/Logo/KAH_Gamedev.KAH_Gamedev"), FVector2D(360.f, 101.f));
	UVerticalBoxSlot* GS = Kah->AddChildToVerticalBox(Gamedev);
	GS->SetHorizontalAlignment(HAlign_Center);
	GS->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	KahGroup = Kah;
	UOverlaySlot* KS = Root->AddChildToOverlay(Kah);
	KS->SetHorizontalAlignment(HAlign_Center);
	KS->SetVerticalAlignment(VAlign_Center);

	// Logo judul
	Title = MakeImage(TEXT("/Game/UI/Logo/Logo_Judul.Logo_Judul"), FVector2D(800.f, 450.f));
	UOverlaySlot* TS = Root->AddChildToOverlay(Title);
	TS->SetHorizontalAlignment(HAlign_Center);
	TS->SetVerticalAlignment(VAlign_Center);

	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);
}

void UJRSplashWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bDone) return;
	Time += FMath::Min(InDeltaTime, 0.05f);	// jeda loading pertama tidak melompati animasi
	// Uji: -jrsplashat=1.0 membekukan animasi di detik tertentu (untuk dipotret).
	static float FreezeAt = -1.f;
	static const bool bFreeze = FParse::Value(FCommandLine::Get(), TEXT("jrsplashat="), FreezeAt);
	if (bFreeze) Time = FreezeAt;

	auto Once = [this](int32 Bit, const TCHAR* Sfx, float Volume)
	{
		if (SoundsPlayed & (1 << Bit)) return;
		SoundsPlayed |= 1 << Bit;
		JRSound::Play2D(this, Sfx, Volume);
	};
	if (Time > 0.05f) Once(0, TEXT("SFX_TV_Nyala"), 0.5f);

	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		const float Start = TileStart + i * TileGap;
		if (Time >= Start) Once(1 + i, TEXT("SFX_HP_Tombol"), 0.9f);
		const float S = Time < Start ? 0.f : EaseOutBack((Time - Start) / PopTime);
		Tiles[i]->SetRenderScale(FVector2D(S, S));
	}
	if (Time >= GamedevStart) Once(4, TEXT("SFX_Notif"), 0.6f);
	const float GSc = Time < GamedevStart ? 0.f : EaseOutBack((Time - GamedevStart) / PopTime);
	Gamedev->SetRenderScale(FVector2D(GSc, GSc));
	KahGroup->SetRenderOpacity(1.f - FMath::Clamp((Time - KahFadeStart) / (KahFadeEnd - KahFadeStart), 0.f, 1.f));

	// Logo judul membal masuk dan bergoyang sebentar seperti papan yang baru dipasang.
	if (Time >= TitleStart) Once(5, TEXT("SFX_Pasang"), 0.9f);
	const float P = Time < TitleStart ? 0.f : (Time - TitleStart) / TitlePop;
	const float TSc = EaseOutBack(P);
	Title->SetRenderScale(FVector2D(TSc, TSc));
	const float Wobble = Time < TitleStart ? 0.f : FMath::Sin((Time - TitleStart) * 11.f) * 5.f * FMath::Exp(-(Time - TitleStart) * 2.5f);
	Title->SetRenderTransformAngle(Wobble);

	SetRenderOpacity(1.f - FMath::Clamp((Time - FadeStart) / (FadeEnd - FadeStart), 0.f, 1.f));
	if (Time >= FadeEnd) Finish();
}

FReply UJRSplashWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Finish();
	return FReply::Handled();
}

FReply UJRSplashWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	Finish();
	return FReply::Handled();
}

void UJRSplashWidget::Finish()
{
	if (bDone) return;
	bDone = true;
	RemoveFromParent();
	if (OnFinished) OnFinished();
}
