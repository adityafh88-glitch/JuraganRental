#include "JRPhoneWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "JRGameMode.h"

namespace
{
	// Palet HP jadul: bodi abu tua, layar LCD hijau.
	const FLinearColor BodyColor = FLinearColor(0.035f, 0.04f, 0.05f, 0.97f);
	const FLinearColor Lcd = FLinearColor(0.42f, 0.55f, 0.24f);
	const FLinearColor LcdInk = FLinearColor(0.03f, 0.06f, 0.02f);
	const FLinearColor LcdMuted = FLinearColor(0.10f, 0.17f, 0.06f);
	const FLinearColor BtnColor = FLinearColor(0.10f, 0.16f, 0.06f);
	const FLinearColor BtnSelected = FLinearColor(0.02f, 0.04f, 0.01f);
	const FLinearColor Led = FLinearColor(1.f, 0.76f, 0.29f);

	FSlateBrush Rounded(float Radius)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		return B;
	}
}

/* ---------- Tombol ---------- */

void UJRPhoneButton::Bind(TFunction<void()> InAction)
{
	Action = MoveTemp(InAction);
	OnClicked.AddUniqueDynamic(this, &UJRPhoneButton::HandleClicked);
}

void UJRPhoneButton::HandleClicked()
{
	if (Action) Action();
}

/* ---------- Susunan widget ---------- */

TSharedRef<SWidget> UJRPhoneWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

UTextBlock* UJRPhoneWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Text));
	T->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetAutoWrapText(true);
	return T;
}

UJRPhoneButton* UJRPhoneWidget::MakeButton(const FString& Label, TFunction<void()> Action, bool bEnabled, bool bSelected)
{
	UJRPhoneButton* B = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
	// Tombol nonaktif digambar sebagai kotak garis terang dengan teks gelap supaya tetap terbaca di layar LCD.
	B->SetBackgroundColor(bSelected ? BtnSelected : (bEnabled ? BtnColor : FLinearColor(0.55f, 0.68f, 0.36f)));
	B->SetIsEnabled(bEnabled);
	UTextBlock* T = MakeText(Label, 13, bEnabled ? Lcd : LcdInk, true);
	T->SetAutoWrapText(false);
	T->SetJustification(ETextJustify::Center);
	B->SetContent(T);
	B->Bind(MoveTemp(Action));
	return B;
}

void UJRPhoneWidget::AddLine(UWidget* Widget, float Top)
{
	UVerticalBoxSlot* S = Content->AddChildToVerticalBox(Widget);
	S->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
}

void UJRPhoneWidget::BuildTree()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Size->SetWidthOverride(400.f);
	Size->SetHeightOverride(660.f);
	UOverlaySlot* RootSlot = Root->AddChildToOverlay(Size);
	RootSlot->SetHorizontalAlignment(HAlign_Right);
	RootSlot->SetVerticalAlignment(VAlign_Center);
	RootSlot->SetPadding(FMargin(0.f, 40.f, 48.f, 0.f));

	// Bodi HP
	UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Body->SetBrush(Rounded(36.f));
	Body->SetBrushColor(BodyColor);
	Body->SetPadding(FMargin(22.f, 20.f));
	Size->AddChild(Body);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Body->SetContent(Col);

	// Kepala HP: merek + kas
	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Brand = MakeText(TEXT("JR-3310"), 14, FLinearColor(0.75f, 0.77f, 0.8f), true);
	Brand->SetAutoWrapText(false);
	Head->AddChildToHorizontalBox(Brand)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HeaderMoney = MakeText(TEXT(""), 14, Led, true);
	HeaderMoney->SetAutoWrapText(false);
	Head->AddChildToHorizontalBox(HeaderMoney);
	Col->AddChildToVerticalBox(Head)->SetPadding(FMargin(4.f, 0.f, 4.f, 10.f));

	// Layar LCD
	UBorder* Screen = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Screen->SetBrush(Rounded(10.f));
	Screen->SetBrushColor(Lcd);
	Screen->SetPadding(FMargin(14.f));
	UVerticalBoxSlot* ScreenSlot = Col->AddChildToVerticalBox(Screen);
	ScreenSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UVerticalBox* ScreenCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Screen->SetContent(ScreenCol);
	TabRow = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ScreenCol->AddChildToVerticalBox(TabRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	ScreenCol->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Scroll->AddChild(Content);

	// Kaki HP: petunjuk
	UTextBlock* Foot = MakeText(TEXT("Tab: tutup HP"), 12, FLinearColor(0.55f, 0.58f, 0.62f));
	Foot->SetJustification(ETextJustify::Center);
	Col->AddChildToVerticalBox(Foot)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
}

/* ---------- Isi layar ---------- */

void UJRPhoneWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshTimer -= InDeltaTime;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = 0.25f;
		Refresh(false);
	}
}

void UJRPhoneWidget::BuildTabs()
{
	TabRow->ClearChildren();
	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const TCHAR* Names[] = { TEXT("Toko"), TEXT("Tarif"), TEXT("Hari ini") };
	for (int32 i = 0; i < 3; ++i)
	{
		UJRPhoneButton* B = MakeButton(Names[i], [this, i]() { CurrentTab = i; Refresh(true); }, true, CurrentTab == i);
		UHorizontalBoxSlot* S = Tabs->AddChildToHorizontalBox(B);
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(i == 0 ? 0.f : 4.f, 0.f, 0.f, 0.f));
	}
	TabRow->AddChildToVerticalBox(Tabs);
}

void UJRPhoneWidget::Refresh(bool bForce)
{
	AJRGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr;
	if (!GM || !Content) return;

	HeaderMoney->SetText(FText::FromString(JRFormatRp(GM->Money)));

	// Tab Hari ini: cukup perbarui teksnya (angka berubah terus, jangan bangun ulang tombol).
	if (!bForce && CurrentTab == 2 && BuiltTab == 2)
	{
		FillHariIni(GM);
		return;
	}

	// Tab lain dibangun ulang hanya jika status tombolnya berubah (bukan setiap kali kas bertambah).
	FString Sig = FString::Printf(TEXT("%d|%d|%d|%d"), CurrentTab, GM->PricePS, GM->PricePC, GM->CapVA);
	for (const FJRShopItem& It : GM->GetShopItems())
	{
		Sig += It.Lock + (GM->Money >= It.Cost ? TEXT("1") : TEXT("0"));
	}
	if (!bForce && Sig == LastSignature) return;
	LastSignature = Sig;

	if (bForce || BuiltTab != CurrentTab) BuildTabs();
	BuiltTab = CurrentTab;
	Content->ClearChildren();
	InfoLines.Reset();
	switch (CurrentTab)
	{
	case 0: FillToko(GM); break;
	case 1: FillTarif(GM); break;
	default: FillHariIni(GM); break;
	}
}

void UJRPhoneWidget::FillToko(AJRGameMode* GM)
{
	AddLine(MakeText(TEXT("TOKO GROSIR"), 15, LcdInk, true), 0.f);
	AddLine(MakeText(TEXT("Barang langsung dipasang di garasi."), 11, LcdMuted), 2.f);
	for (const FJRShopItem& It : GM->GetShopItems())
	{
		AddLine(MakeText(It.Name, 14, LcdInk, true), 14.f);
		AddLine(MakeText(It.Desc, 11, LcdMuted), 2.f);
		const bool bCanPay = GM->Money >= It.Cost;
		FString Label;
		if (!It.Lock.IsEmpty()) Label = It.Lock;
		else if (!bCanPay) Label = FString::Printf(TEXT("Kas kurang (%s)"), *JRFormatRp(It.Cost));
		else Label = FString::Printf(TEXT("Beli %s"), *JRFormatRp(It.Cost));
		const FName Id = It.Id;
		AddLine(MakeButton(Label, [this, Id]()
		{
			if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) G->BuyItem(Id);
			Refresh(true);
		}, It.Lock.IsEmpty() && bCanPay), 6.f);
	}
}

void UJRPhoneWidget::FillTarif(AJRGameMode* GM)
{
	AddLine(MakeText(TEXT("TARIF PER JAM"), 15, LcdInk, true), 0.f);
	AddLine(MakeText(TEXT("Lebih murah: lebih ramai dan pelanggan lebih puas. Lebih mahal: untung per jam naik."), 11, LcdMuted), 2.f);
	for (EJRUnitType Type : { EJRUnitType::PS, EJRUnitType::PC })
	{
		const FString Name = Type == EJRUnitType::PS ? TEXT("Rental PS") : TEXT("Warnet");
		AddLine(MakeText(FString::Printf(TEXT("%s: %s/jam"), *Name, *JRFormatRp(GM->PriceFor(Type))), 14, LcdInk, true), 16.f);
		AddLine(MakeText(FString::Printf(TEXT("Harga wajar %s"), *JRFormatRp(GM->FairFor(Type))), 11, LcdMuted), 2.f);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 Delta : { -500, 500 })
		{
			UJRPhoneButton* B = MakeButton(Delta < 0 ? TEXT("- Rp 500") : TEXT("+ Rp 500"), [this, Type, Delta]()
			{
				if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) G->AdjustPrice(Type, Delta);
				Refresh(true);
			});
			UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(B);
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			S->SetPadding(FMargin(Delta < 0 ? 0.f : 6.f, 0.f, 0.f, 0.f));
		}
		AddLine(Row, 6.f);
	}
}

void UJRPhoneWidget::FillHariIni(AJRGameMode* GM)
{
	const FJRDayStats& S = GM->GetStats();
	const FString Lines[] = {
		FString::Printf(TEXT("HARI %d"), GM->Day),
		FString::Printf(TEXT("Pelanggan datang: %d"), S.Customers),
		FString::Printf(TEXT("Pemasukan: %s"), *JRFormatRp(S.Income)),
		FString::Printf(TEXT("Belanja bahan kantin: %s"), *JRFormatRp(S.Stock)),
		FString::Printf(TEXT("Listrik sejauh ini: %s"), *JRFormatRp(S.Elec)),
		FString::Printf(TEXT("Kabur tanpa bayar: %s"), *JRFormatRp(S.Kabur)),
		FString::Printf(TEXT("Reputasi: %d"), FMath::RoundToInt(GM->Rep)),
		FString::Printf(TEXT("Daya: %d / %d VA"), GM->PowerVA, GM->CapVA),
		TEXT("Target Gerbang 1: Rp 1.500.000 dan reputasi 40."),
	};
	const int32 N = UE_ARRAY_COUNT(Lines);
	if (InfoLines.Num() != N)
	{
		InfoLines.Reset();
		for (int32 i = 0; i < N; ++i)
		{
			UTextBlock* T = MakeText(Lines[i], i == 0 ? 15 : 13, LcdInk, i == 0);
			AddLine(T, i == 0 ? 0.f : (i == N - 1 ? 18.f : 8.f));
			InfoLines.Add(T);
		}
		return;
	}
	for (int32 i = 0; i < N; ++i) InfoLines[i]->SetText(FText::FromString(Lines[i]));
}
