#include "JRPhoneWidget.h"
#include "JRLang.h"

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
#include "JRStation.h"
#include "JRSound.h"
#include "JRHUD.h"
#include "JRSettings.h"
#include "GameFramework/PlayerController.h"

namespace JRLocal_PhoneWidget
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
using namespace JRLocal_PhoneWidget;

/* ---------- Tombol ---------- */

void UJRPhoneButton::Bind(TFunction<void()> InAction)
{
	Action = MoveTemp(InAction);
	OnClicked.AddUniqueDynamic(this, &UJRPhoneButton::HandleClicked);
}

void UJRPhoneButton::HandleClicked()
{
	JRSound::Play2D(this, TEXT("SFX_HP_Tombol"), 0.6f);
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
	Head->AddChildToHorizontalBox(HeaderMoney)->SetVerticalAlignment(VAlign_Center);
	// Tombol tutup (wajib untuk layar sentuh: tombol HP di layar tertutup oleh HP ini)
	USizeBox* CloseBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CloseBox->SetWidthOverride(64.f);
	CloseBox->SetHeightOverride(40.f);
	UJRPhoneButton* Close = MakeButton(TEXT("X"), [this]() { ClosePhone(); });
	Close->SetBackgroundColor(FLinearColor(0.75f, 0.2f, 0.17f));
	CloseBox->AddChild(Close);
	UHorizontalBoxSlot* CloseSlot = Head->AddChildToHorizontalBox(CloseBox);
	CloseSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	CloseSlot->SetVerticalAlignment(VAlign_Center);
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
	UTextBlock* Foot = MakeText((PLATFORM_ANDROID || PLATFORM_IOS) ? JRT(TEXT("Ketuk X untuk tutup HP")) : JRT(TEXT("Tab atau X: tutup HP")), 12, FLinearColor(0.55f, 0.58f, 0.62f));
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
	const TCHAR* Names[] = { JRT(TEXT("Toko")), JRT(TEXT("Unit")), JRT(TEXT("Kelola")), JRT(TEXT("Info")), JRT(TEXT("Atur")) };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Names); ++i)
	{
		UJRPhoneButton* B = MakeButton(Names[i], [this, i]() { CurrentTab = i; Refresh(true); }, true, CurrentTab == i);
		// Lima tab: huruf sedikit lebih kecil agar muat di layar HP (juga di layar ponsel sungguhan).
		if (UTextBlock* T = Cast<UTextBlock>(B->GetContent())) T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
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
	if (!bForce && CurrentTab == 3 && BuiltTab == 3)
	{
		FillHariIni(GM);
		return;
	}

	// Tab lain dibangun ulang hanya jika status tombolnya berubah (bukan setiap kali kas bertambah).
	FString Sig = FString::Printf(TEXT("%d|%d|%d|%d|%d|%d|%d|%d%d%d"), CurrentTab, GM->PricePS + GM->PriceVIP * 7, GM->PricePC, GM->CapVA, GM->Era, GM->bPaketMalam ? 1 : 0, GM->Phase == EJRGamePhase::Playing ? 1 : 0,
		GM->HasStaff(EJRStaff::Kasir) ? 1 : 0, GM->HasStaff(EJRStaff::Teknisi) ? 1 : 0, GM->HasStaff(EJRStaff::Koki) ? 1 : 0);
	Sig += FString::Printf(TEXT("|n%d%d"), GM->NetPlan, GM->PendingNetPlan);
	if (CurrentTab == 1)
	{
		// Tab Unit: XP dan status tiap tombol upgrade
		Sig += FString::Printf(TEXT("|x%d"), GM->Xp);
		for (const AJRStation* S : GM->GetStations()) Sig += FString::Printf(TEXT("%d"), S->UpLevel) + GM->UnitUpgradeLock(S);
	}
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
	case 1: FillUnit(GM); break;
	case 2: FillTarif(GM); break;
	case 4: FillAtur(); break;
	default: FillHariIni(GM); break;
	}
}

void UJRPhoneWidget::FillToko(AJRGameMode* GM)
{
	AddLine(MakeText(JRT(TEXT("TOKO GROSIR")), 15, LcdInk, true), 0.f);
	AddLine(MakeText(JRT(TEXT("Barang diantar kurir besok pagi. Angkat kardusnya di depan pintu.")), 11, LcdMuted), 2.f);
	for (const FJRShopItem& It : GM->GetShopItems())
	{
		AddLine(MakeText(It.Name, 14, LcdInk, true), 14.f);
		AddLine(MakeText(It.Desc, 11, LcdMuted), 2.f);
		const bool bCanPay = GM->Money >= It.Cost;
		FString Label;
		if (!It.Lock.IsEmpty()) Label = It.Lock;
		else if (!bCanPay) Label = JRPrintf(JRT(TEXT("Kas kurang (%s)")), *JRFormatRp(It.Cost));
		else Label = JRPrintf(JRT(TEXT("Beli %s")), *JRFormatRp(It.Cost));
		const FName Id = It.Id;
		AddLine(MakeButton(Label, [this, Id]()
		{
			if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) G->BuyItem(Id);
			Refresh(true);
		}, It.Lock.IsEmpty() && bCanPay), 6.f);
	}
}

void UJRPhoneWidget::FillUnit(AJRGameMode* GM)
{
	const int32 Lv = GM->JuraganLevel();
	AddLine(MakeText(JRPrintf(JRT(TEXT("LEVEL JURAGAN %d")), Lv), 15, LcdInk, true), 0.f);
	AddLine(MakeText(Lv >= AJRGameMode::MaxJuraganLevel ? FString(JRT(TEXT("Level maksimal. Kamu juragan sejati!")))
		: JRPrintf(JRT(TEXT("XP %d / %d")), GM->Xp, AJRGameMode::XpForLevel(Lv + 1)), 13, LcdInk), 2.f);
	AddLine(MakeText(JRT(TEXT("XP dari pelanggan yang bayar, misi, turnamen, dan bisnis sampingan. Naik level membuka barang baru di Toko dan upgrade unit.")), 11, LcdMuted), 2.f);

	AddLine(MakeText(JRT(TEXT("UPGRADE UNIT")), 15, LcdInk, true), 18.f);
	AddLine(MakeText(JRPrintf(JRT(TEXT("Tiap level: tarif +Rp 500/jam (VIP +Rp 2.000), pelanggan lebih puas dan lebih jarang rusak. Perawatan %s per level per hari.")),
		*JRFormatRp(AJRGameMode::MaintPerLevel)), 11, LcdMuted), 2.f);
	TArray<AJRStation*> List;
	for (AJRStation* S : GM->GetStations()) if (S) List.Add(S);
	List.Sort([](const AJRStation& A, const AJRStation& B) { return A.Type != B.Type ? (uint8)A.Type < (uint8)B.Type : A.Index < B.Index; });
	for (AJRStation* S : List)
	{
		AddLine(MakeText(JRPrintf(JRT(TEXT("%s  Lv %d: %s")), *S->DisplayName, S->UpLevel, *AJRGameMode::UnitLevelName(S->Type, S->UpLevel)), 13, LcdInk, true), 12.f);
		const FString Lock = GM->UnitUpgradeLock(S);
		const int32 Next = S->UpLevel + 1;
		if (S->UpLevel < AJRGameMode::MaxUnitLevel)
		{
			AddLine(MakeText(JRPrintf(JRT(TEXT("Berikutnya: %s")), *AJRGameMode::UnitLevelName(S->Type, Next)), 11, LcdMuted), 2.f);
		}
		const FString Label = Lock.IsEmpty() ? JRPrintf(JRT(TEXT("Upgrade ke Lv %d: %s")), Next, *JRFormatRp(AJRGameMode::UnitUpgradeCost(S->Type, Next))) : Lock;
		TWeakObjectPtr<AJRStation> WS(S);
		AddLine(MakeButton(Label, [this, WS]()
		{
			AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>();
			if (G && WS.IsValid()) G->UpgradeUnit(WS.Get());
			Refresh(true);
		}, Lock.IsEmpty()), 6.f);
	}
}

void UJRPhoneWidget::FillTarif(AJRGameMode* GM)
{
	AddLine(MakeText(JRT(TEXT("TARIF PER JAM")), 15, LcdInk, true), 0.f);
	AddLine(MakeText(JRT(TEXT("Lebih murah: lebih ramai dan pelanggan lebih puas. Lebih mahal: untung per jam naik.")), 11, LcdMuted), 2.f);
	TArray<EJRUnitType> Types = { EJRUnitType::PS, EJRUnitType::PC };
	if (GM->Era >= 3) Types.Add(EJRUnitType::VIP);
	for (EJRUnitType Type : Types)
	{
		const FString Name = Type == EJRUnitType::PS ? JRT(TEXT("Rental PS")) : Type == EJRUnitType::VIP ? JRT(TEXT("Booth VIP")) : JRT(TEXT("Warnet"));
		AddLine(MakeText(JRPrintf(JRT(TEXT("%s: %s/jam")), *Name, *JRFormatRp(GM->PriceFor(Type))), 14, LcdInk, true), 16.f);
		AddLine(MakeText(JRPrintf(JRT(TEXT("Harga wajar %s")), *JRFormatRp(GM->FairFor(Type))), 11, LcdMuted), 2.f);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 Delta : { -500, 500 })
		{
			UJRPhoneButton* B = MakeButton(Delta < 0 ? JRT(TEXT("- Rp 500")) : JRT(TEXT("+ Rp 500")), [this, Type, Delta]()
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
	if (GM->Era >= 2)
	{
		AddLine(MakeText(JRT(TEXT("Paket malam warnet")), 14, LcdInk, true), 18.f);
		AddLine(MakeText(JRT(TEXT("Setelah 21.00 gamer bayar Rp 10.000 flat dan main sampai tutup.")), 11, LcdMuted), 2.f);
		AddLine(MakeButton(GM->bPaketMalam ? JRT(TEXT("Matikan paket malam")) : JRT(TEXT("Aktifkan paket malam")), [this]()
		{
			if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) G->TogglePaketMalam();
			Refresh(true);
		}), 6.f);
	}
	FillInternet(GM);
	FillStaff(GM);
}

void UJRPhoneWidget::FillInternet(AJRGameMode* GM)
{
	if (GM->Era < 2) return;
	AddLine(MakeText(JRT(TEXT("PROVIDER INTERNET")), 15, LcdInk, true), 22.f);
	AddLine(MakeText(JRT(TEXT("Ganti provider berlaku mulai besok pagi.")), 11, LcdMuted), 2.f);
	for (int32 Plan = 0; Plan < 3; ++Plan)
	{
		const bool bNow = GM->NetPlan == Plan;
		const bool bNext = GM->PendingNetPlan == Plan;
		AddLine(MakeText(JRPrintf(JRT(TEXT("%s  (%s/hari)%s")), AJRGameMode::NetName(Plan), *JRFormatRp(AJRGameMode::NetCost(Plan)),
			bNow ? JRT(TEXT("  - DIPAKAI")) : bNext ? JRT(TEXT("  - MULAI BESOK")) : TEXT("")), 14, LcdInk, true), 12.f);
		AddLine(MakeText(AJRGameMode::NetDesc(Plan), 11, LcdMuted), 2.f);
		if (!bNow && !bNext)
		{
			AddLine(MakeButton(JRT(TEXT("Pilih")), [this, Plan]()
			{
				if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) G->ChooseNetPlan(Plan);
				Refresh(true);
			}), 6.f);
		}
	}
}

void UJRPhoneWidget::ClosePhone()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AJRHUD* Hud = Cast<AJRHUD>(PC->GetHUD())) Hud->SetPhoneOpen(false);
	}
}

void UJRPhoneWidget::FillAtur()
{
	UJRSettingsSave* S = JRSettings::Get();
	AddLine(MakeText(TEXT("PENGATURAN"), 15, LcdInk, true), 0.f);

	auto Toggle = [this](const FString& Title, const FString& Desc, const FString& Label, TFunction<void()> Change)
	{
		AddLine(MakeText(Title, 14, LcdInk, true), 16.f);
		AddLine(MakeText(Desc, 11, LcdMuted), 2.f);
		AddLine(MakeButton(Label, [this, Change]()
		{
			Change();
			JRSettings::Save();
			Refresh(true);
		}), 6.f);
	};

	Toggle(JRT(TEXT("Efek suara")), JRT(TEXT("Bunyi unit, kasir, kompor, motor lewat.")),
		S->bSfx ? JRT(TEXT("NYALA (ketuk untuk matikan)")) : JRT(TEXT("MATI (ketuk untuk nyalakan)")),
		[S]() { S->bSfx = !S->bSfx; JRSound::ApplySettings(); });
	Toggle(JRT(TEXT("Suara karakter")), JRT(TEXT("Celetukan pelanggan, Emak, dan Pak RT.")),
		S->bVoice ? JRT(TEXT("NYALA (ketuk untuk matikan)")) : JRT(TEXT("MATI (ketuk untuk nyalakan)")),
		[S]() { S->bVoice = !S->bVoice; });
	Toggle(JRT(TEXT("Musik radio")), JRT(TEXT("Lagu dari radio di meja kasir. Tekan radionya untuk ganti lagu.")),
		S->bMusic ? JRT(TEXT("NYALA (ketuk untuk matikan)")) : JRT(TEXT("MATI (ketuk untuk nyalakan)")),
		[S]() { S->bMusic = !S->bMusic; });
	Toggle(JRT(TEXT("Joystick layar")), JRT(TEXT("Tetap: joystick diam di pojok dan selalu terlihat. Ikut jari: muncul di tempat jari menyentuh.")),
		S->bJoystickFixed ? JRT(TEXT("TETAP (ketuk untuk ikut jari)")) : JRT(TEXT("IKUT JARI (ketuk untuk tetap)")),
		[this, S]()
		{
			S->bJoystickFixed = !S->bJoystickFixed;
			if (APlayerController* PC = GetOwningPlayer())
			{
				if (AJRHUD* Hud = Cast<AJRHUD>(PC->GetHUD())) Hud->ApplyTouchSettings();
			}
		});
	Toggle(JRT(TEXT("Arah lihat atas/bawah")), JRT(TEXT("Dibalik: geser ke atas untuk melihat ke bawah.")),
		S->bInvertLook ? JRT(TEXT("DIBALIK (ketuk untuk normal)")) : JRT(TEXT("NORMAL (ketuk untuk dibalik)")),
		[S]() { S->bInvertLook = !S->bInvertLook; });
	Toggle(JRT(TEXT("Daftar misi")), JRT(TEXT("Ringkas: hanya satu baris di pojok kiri atas agar layar lebih lega.")),
		S->bMissionsCompact ? JRT(TEXT("RINGKAS (ketuk untuk lengkap)")) : JRT(TEXT("LENGKAP (ketuk untuk ringkas)")),
		[S]() { S->bMissionsCompact = !S->bMissionsCompact; });
	AJRHUD* Hud = GetOwningPlayer() ? Cast<AJRHUD>(GetOwningPlayer()->GetHUD()) : nullptr;
	if (Hud && Hud->IsTouchMode())
	{
		Toggle(JRT(TEXT("Ukuran tampilan")), JRT(TEXT("Besarkan teks dan tombol bila terlalu kecil di layar HP-mu.")),
			JRPrintf(JRT(TEXT("%s (ketuk untuk ganti)")), *JRSettings::UIScaleName(S->UIScale)),
			[S, Hud]() { S->UIScale = JRSettings::NextUIScale(S->UIScale); Hud->ApplyUIScale(); });
		Toggle(JRT(TEXT("Ukuran joystick")), JRT(TEXT("Perkecil joystick bila terlalu memenuhi layar, perbesar bila sulit dikendalikan.")),
			JRPrintf(JRT(TEXT("%s (ketuk untuk ganti)")), *JRSettings::JoystickSizeName(S->JoystickSize)),
			[S, Hud]() { S->JoystickSize = JRSettings::NextJoystickSize(S->JoystickSize); Hud->ApplyTouchSettings(); });
	}

}

void UJRPhoneWidget::FillStaff(AJRGameMode* GM)
{
	AddLine(MakeText(TEXT("KARYAWAN"), 15, LcdInk, true), 22.f);
	if (GM->Era < 2)
	{
		AddLine(MakeText(JRT(TEXT("Karyawan bisa direkrut setelah pindah ke ruko.")), 11, LcdMuted), 2.f);
		return;
	}
	AddLine(MakeText(JRT(TEXT("Gaji dipotong setiap toko tutup.")), 11, LcdMuted), 2.f);
	for (int32 i = 0; i < (int32)EJRStaff::Count; ++i)
	{
		const EJRStaff S = (EJRStaff)i;
		const bool bHired = GM->HasStaff(S);
		AddLine(MakeText(JRPrintf(JRT(TEXT("%s  (%s/hari)%s")), AJRGameMode::StaffName(S), *JRFormatRp(AJRGameMode::StaffWage(S)), bHired ? JRT(TEXT("  - BEKERJA")) : TEXT("")), 14, LcdInk, true), 12.f);
		AddLine(MakeText(AJRGameMode::StaffDesc(S), 11, LcdMuted), 2.f);
		AddLine(MakeButton(bHired ? JRT(TEXT("Berhentikan")) : JRT(TEXT("Rekrut")), [this, S]()
		{
			if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>()) G->ToggleStaff(S);
			Refresh(true);
		}), 6.f);
	}
}

void UJRPhoneWidget::FillHariIni(AJRGameMode* GM)
{
	const FJRDayStats& S = GM->GetStats();
	const FString Lines[] = {
		JRPrintf(JRT(TEXT("HARI %d")), GM->Day),
		JRPrintf(JRT(TEXT("Pelanggan datang: %d")), S.Customers),
		JRPrintf(JRT(TEXT("Pemasukan: %s")), *JRFormatRp(S.Income)),
		JRPrintf(JRT(TEXT("Belanja bahan kantin: %s")), *JRFormatRp(S.Stock)),
		JRPrintf(JRT(TEXT("Listrik sejauh ini: %s")), *JRFormatRp(S.Elec)),
		JRPrintf(JRT(TEXT("Kabur tanpa bayar: %s")), *JRFormatRp(S.Kabur)),
		JRPrintf(JRT(TEXT("Piutang di buku utang: %s")), *JRFormatRp(GM->TotalDebt())),
		JRPrintf(JRT(TEXT("Reputasi: %d")), FMath::RoundToInt(GM->Rep)),
		JRPrintf(JRT(TEXT("Level Juragan %d  (XP %d)")), GM->JuraganLevel(), GM->Xp),
		JRPrintf(JRT(TEXT("Daya: %d / %d VA")), GM->PowerVA, GM->CapVA),
		GM->Era == 1 ? JRT(TEXT("Target Gerbang 1: Rp 600.000 dan reputasi 40.")) : JRT(TEXT("Target Gerbang 2: Rp 3 juta, reputasi 60, 10 unit.")),
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
