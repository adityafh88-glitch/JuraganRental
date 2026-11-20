#include "JRBillingWidget.h"

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
#include "Styling/CoreStyle.h"
#include "JRCustomer.h"
#include "JRGameMode.h"
#include "JRLang.h"
#include "JRPhoneWidget.h"
#include "JRStyle.h"
#include "JRStation.h"

namespace JRBillingStyle
{
	// Tampilan layar monitor billing: latar biru tua, teks hijau terang ala aplikasi billing warnet
	const FLinearColor Frame(0.09f, 0.1f, 0.12f, 0.98f);
	const FLinearColor Screen(0.04f, 0.08f, 0.16f);
	const FLinearColor Ink(0.85f, 0.95f, 0.88f);
	const FLinearColor Muted(0.5f, 0.6f, 0.68f);
	const FLinearColor Wait(1.f, 0.78f, 0.3f);
	const FLinearColor Play(0.4f, 0.95f, 0.55f);
	const FLinearColor Bad(0.95f, 0.45f, 0.4f);
	const FLinearColor Btn(0.2f, 0.55f, 0.95f);

	FSlateBrush Rounded(float R)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		B.OutlineSettings.CornerRadii = FVector4(R, R, R, R);
		return B;
	}
}
using namespace JRBillingStyle;

TSharedRef<SWidget> UJRBillingWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildTree();
	return Super::RebuildWidget();
}

UTextBlock* UJRBillingWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Text));
	T->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
	T->SetColorAndOpacity(FSlateColor(Color));
	return T;
}

void UJRBillingWidget::BuildTree()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Size->SetWidthOverride(560.f);
	Size->SetMaxDesiredHeight(620.f);
	UOverlaySlot* S = Root->AddChildToOverlay(Size);
	S->SetHorizontalAlignment(HAlign_Center);
	S->SetVerticalAlignment(VAlign_Center);

	UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Box->SetBrush(Rounded(14.f));
	Box->SetBrushColor(Frame);
	Box->SetPadding(FMargin(16.f));
	Size->AddChild(Box);

	UBorder* Scr = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Scr->SetBrush(Rounded(6.f));
	Scr->SetBrushColor(Screen);
	Scr->SetPadding(FMargin(16.f, 12.f));
	Box->SetContent(Scr);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Scr->SetContent(Col);

	// Judul + tombol tutup
	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Head->AddChildToHorizontalBox(MakeText(JRT(TEXT("JURAGAN BILLING v1.0")), 18, Play, true))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UJRPhoneButton* Close = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
	Close->SetContent(MakeText(TEXT("  X  "), 14, Ink, true));
	JRStyle::Apply(Close, JRStyle::EBtn::Danger, 12.f);
	Close->Bind([this]() { if (OnClose) OnClose(); });
	Head->AddChildToHorizontalBox(Close);
	Col->AddChildToVerticalBox(Head);

	Summary = MakeText(TEXT(""), 13, Muted);
	Col->AddChildToVerticalBox(Summary)->SetPadding(FMargin(0.f, 4.f, 0.f, 10.f));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Scroll->AddChild(List);
	Col->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	Col->AddChildToVerticalBox(MakeText(JRT(TEXT("Esc / E: tutup. Bayaran tetap diterima di meja kasir.")), 11, Muted))->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
}

void UJRBillingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Timer -= InDeltaTime;
	if (Timer <= 0.f)
	{
		Timer = 0.25f;
		Refresh(false);
	}
}

void UJRBillingWidget::Refresh(bool bForce)
{
	AJRGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr;
	if (!GM || !List) return;

	// Status tiap unit; daftar dibangun ulang hanya jika ada status yang berubah (bukan tiap detik)
	struct FRow { AJRStation* S; int32 Kind; FString Text; };	// Kind: 0 kosong, 1 menunggu, 2 main, 3 rusak, 4 listrik mati
	TArray<FRow> Rows;
	int32 Waiting = 0, Playing = 0;
	FString Sig;
	TArray<AJRStation*> Sorted;
	for (AJRStation* S : GM->GetStations()) if (S) Sorted.Add(S);
	Sorted.Sort([](const AJRStation& A, const AJRStation& B) { return A.Type != B.Type ? (uint8)A.Type < (uint8)B.Type : A.Index < B.Index; });
	for (AJRStation* S : Sorted)
	{
		if (!S) continue;
		FRow R{ S, 0, FString() };
		AJRCustomer* C = S->Customer;
		if (S->bBroken) { R.Kind = 3; R.Text = S->bVirus ? JRT(TEXT("VIRUS")) : JRT(TEXT("RUSAK")); }
		else if (C && C->State == EJRCustomerState::WaitStart) { R.Kind = 1; R.Text = JRPrintf(JRT(TEXT("Menunggu: %s")), *C->DisplayName); ++Waiting; }
		else if (C && C->State == EJRCustomerState::Playing)
		{
			++Playing;
			if (GM->bMcbTripped) { R.Kind = 4; R.Text = JRT(TEXT("Listrik mati")); }
			else
			{
				const int32 Min = FMath::Max(0, FMath::CeilToInt(C->HoursLeft * 60.f));
				R.Kind = 2; R.Text = JRPrintf(JRT(TEXT("Main: %s  (sisa %d:%02d)")), *C->DisplayName, Min / 60, Min % 60);
			}
		}
		else if (C) { R.Kind = 0; R.Text = JRT(TEXT("Pelanggan sedang jalan")); }
		else R.Text = JRT(TEXT("Kosong"));
		Sig += FString::Printf(TEXT("%d%d;"), R.Kind, C ? 1 : 0);
		Rows.Add(R);
	}
	Summary->SetText(FText::FromString(JRPrintf(JRT(TEXT("%d unit  -  %d menunggu  -  %d sedang main")), Rows.Num(), Waiting, Playing)));
	if (!bForce && Sig == LastSig)
	{
		// Teks sisa waktu tetap diperbarui tanpa membangun ulang tombol
		for (int32 i = 0; i < Rows.Num() && i < List->GetChildrenCount() - (Waiting > 1 ? 1 : 0); ++i)
		{
			if (UHorizontalBox* H = Cast<UHorizontalBox>(List->GetChildAt(i + (Waiting > 1 ? 1 : 0))))
			{
				if (UTextBlock* T = Cast<UTextBlock>(H->GetChildAt(1))) T->SetText(FText::FromString(Rows[i].Text));
			}
		}
		return;
	}
	LastSig = Sig;
	List->ClearChildren();

	if (Waiting > 1)
	{
		UJRPhoneButton* All = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
		All->SetBackgroundColor(FLinearColor::White);	// gaya diterapkan setelah isi dipasang
		if (UButtonSlot* BS = Cast<UButtonSlot>(All->SetContent(MakeText(JRPrintf(JRT(TEXT("Nyalakan semua yang menunggu (%d)")), Waiting), 15, FLinearColor::White, true))))
		{
			BS->SetPadding(FMargin(10.f, 8.f));
		}
		JRStyle::Apply(All, JRStyle::EBtn::Primary, 12.f);
		All->Bind([this]()
		{
			if (AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>())
			{
				for (AJRStation* S : G->GetStations())
					if (S && !S->bBroken && S->Customer && S->Customer->State == EJRCustomerState::WaitStart) G->StartSession(S);
			}
			Refresh(true);
		});
		List->AddChildToVerticalBox(All)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	for (const FRow& R : Rows)
	{
		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		USizeBox* NameBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		NameBox->SetWidthOverride(110.f);
		NameBox->AddChild(MakeText(R.S->DisplayName, 14, Ink, true));
		H->AddChildToHorizontalBox(NameBox)->SetVerticalAlignment(VAlign_Center);
		const FLinearColor C = R.Kind == 1 ? Wait : R.Kind == 2 ? Play : R.Kind >= 3 ? Bad : Muted;
		UHorizontalBoxSlot* TS = H->AddChildToHorizontalBox(MakeText(R.Text, 13, C, R.Kind == 1));
		TS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TS->SetVerticalAlignment(VAlign_Center);
		if (R.Kind == 1)
		{
			TWeakObjectPtr<AJRStation> WS(R.S);
			UJRPhoneButton* B = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
			B->SetBackgroundColor(FLinearColor::White);
			if (UButtonSlot* BS = Cast<UButtonSlot>(B->SetContent(MakeText(JRPrintf(JRT(TEXT("Nyalakan  %s/jam")), *JRFormatRp(GM->UnitPrice(R.S))), 13, FLinearColor::White, true))))
			{
				BS->SetPadding(FMargin(10.f, 5.f));
			}
			JRStyle::Apply(B, JRStyle::EBtn::Primary, 12.f);
			B->Bind([this, WS]()
			{
				AJRGameMode* G = GetWorld()->GetAuthGameMode<AJRGameMode>();
				if (G && WS.IsValid() && WS->Customer && WS->Customer->State == EJRCustomerState::WaitStart) G->StartSession(WS.Get());
				Refresh(true);
			});
			H->AddChildToHorizontalBox(B)->SetVerticalAlignment(VAlign_Center);
		}
		List->AddChildToVerticalBox(H)->SetPadding(FMargin(0.f, 3.f));
	}
}
