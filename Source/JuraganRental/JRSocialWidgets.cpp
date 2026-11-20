#include "JRSocialWidgets.h"
#include "JRLang.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "JRGameMode.h"
#include "JRPhoneWidget.h"
#include "JRStyle.h"

namespace JRLocal_Social
{
	UTextBlock* Text(UWidgetTree* Tree, const FString& S, int32 Size, const FLinearColor& Color, bool bBold = false, bool bCenter = false)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(S));
		T->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetAutoWrapText(true);
		if (bCenter) T->SetJustification(ETextJustify::Center);
		return T;
	}

	UJRPhoneButton* Btn(UWidgetTree* Tree, const FString& Label, JRStyle::EBtn Kind, int32 Size, TFunction<void()> Action)
	{
		UJRPhoneButton* B = Tree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
		UTextBlock* T = Text(Tree, Label, Size, FLinearColor::White, true, true);
		T->SetAutoWrapText(false);
		if (UButtonSlot* S = Cast<UButtonSlot>(B->SetContent(T))) S->SetPadding(FMargin(14.f, 9.f));
		JRStyle::Apply(B, Kind);
		B->Bind(MoveTemp(Action));
		return B;
	}

	/** Lapisan gelap + kartu di tengah. Mengembalikan kolom isi kartu. */
	UVerticalBox* Card(UWidgetTree* Tree, float Width)
	{
		UOverlay* Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Tree->RootWidget = Root;
		UBorder* Dim = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
		UOverlaySlot* DS = Root->AddChildToOverlay(Dim);
		DS->SetHorizontalAlignment(HAlign_Fill);
		DS->SetVerticalAlignment(VAlign_Fill);
		USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(Width);
		UOverlaySlot* S = Root->AddChildToOverlay(Size);
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
		UBorder* Box = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Box->SetBrush(JRStyle::Panel(22.f));
		Box->SetBrushColor(FLinearColor::White);
		Box->SetPadding(FMargin(28.f, 22.f));
		Size->AddChild(Box);
		UVerticalBox* Col = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Box->SetContent(Col);
		return Col;
	}
}
using namespace JRLocal_Social;

/* ---------- Beri nama rental ---------- */

TSharedRef<SWidget> UJRNameWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildTree();
	return Super::RebuildWidget();
}

void UJRNameWidget::BuildTree()
{
	UVerticalBox* Col = Card(WidgetTree, 560.f);
	Col->AddChildToVerticalBox(Text(WidgetTree, JRT(TEXT("BERI NAMA RENTALMU")), 22, JRStyle::Accent(), true));
	Col->AddChildToVerticalBox(Text(WidgetTree, JRT(TEXT("Nama ini tampil di papan nama depan, laporan harian, Kartu Momen, dan komentar warganet.")), 13, JRStyle::Muted()))
		->SetPadding(FMargin(0.f, 4.f, 0.f, 14.f));

	Input = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	{
		FEditableTextBoxStyle St = Input->WidgetStyle;
		St.SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
		Input->WidgetStyle = St;
	}
	Input->SetHintText(FText::FromString(JRT(TEXT("Nama rental (maks. 20 huruf)"))));
	Input->OnTextChanged.AddDynamic(this, &UJRNameWidget::OnTextChanged);
	Col->AddChildToVerticalBox(Input);

	// Saran nama siap pilih
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	static const TCHAR* Ideas[] = { TEXT("Berkah Jaya"), TEXT("Sumber Rejeki"), TEXT("Bang Jago"), TEXT("Maju Mapan") };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Ideas); ++i)
	{
		const FString Idea = Ideas[i];
		UJRPhoneButton* B = Btn(WidgetTree, Idea, JRStyle::EBtn::Tab, 13, [this, Idea]() { Input->SetText(FText::FromString(Idea)); UpdatePreview(); });
		UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(B);
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(i == 0 ? 0.f : 6.f, 0.f, 0.f, 0.f));
	}
	Col->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	Preview = Text(WidgetTree, TEXT(""), 13, JRStyle::Ink());
	Col->AddChildToVerticalBox(Preview)->SetPadding(FMargin(0.f, 12.f, 0.f, 14.f));

	Col->AddChildToVerticalBox(Btn(WidgetTree, JRT(TEXT("Pasang papan nama")), JRStyle::EBtn::Primary, 17, [this]()
	{
		if (OnDone) OnDone(Input ? Input->GetText().ToString() : FString());
	}));
}

void UJRNameWidget::Open(const FString& Current)
{
	TakeWidget();
	Input->SetText(FText::FromString(Current));
	UpdatePreview();
	SetVisibility(ESlateVisibility::Visible);
}

void UJRNameWidget::OnTextChanged(const FText& T)
{
	if (T.ToString().Len() > 20) Input->SetText(FText::FromString(T.ToString().Left(20)));
	UpdatePreview();
}

void UJRNameWidget::UpdatePreview()
{
	FString N = Input ? Input->GetText().ToString().TrimStartAndEnd() : FString();
	if (N.IsEmpty()) N = TEXT("Juragan");
	Preview->SetText(FText::FromString(JRPrintf(JRT(TEXT("Papan nama: RENTAL PS & WARNET %s")), *N.ToUpper())));
}

/* ---------- Kartu Momen ---------- */

TSharedRef<SWidget> UJRMomentCard::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildTree();
	return Super::RebuildWidget();
}

void UJRMomentCard::BuildTree()
{
	Body = Card(WidgetTree, 540.f);
}

void UJRMomentCard::Refresh()
{
	TakeWidget();
	const AJRGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr;
	if (!GM || !Body) return;
	Body->ClearChildren();
	const FJRDayStats& St = GM->GetStats();

	// Kepala: label kecil + nama rental besar + hari/era
	Body->AddChildToVerticalBox(Text(WidgetTree, JRT(TEXT("KARTU MOMEN")), 12, JRStyle::Muted(), true, true));
	Body->AddChildToVerticalBox(Text(WidgetTree, GM->RentalName.ToUpper(), 28, JRStyle::Accent(), true, true));
	const FString EraName = GM->Era >= 3 ? JRT(TEXT("Game Center")) : GM->Era == 2 ? JRT(TEXT("Ruko Kecil")) : JRT(TEXT("Garasi"));
	Body->AddChildToVerticalBox(Text(WidgetTree, JRPrintf(JRT(TEXT("Hari ke-%d  -  %s  -  Level Juragan %d")), GM->Day, *EraName, GM->JuraganLevel()), 13, JRStyle::Muted(), false, true));

	// Bintang hari ini
	FString Stars;
	for (int32 i = 0; i < 5; ++i) Stars += i < GM->MomentStars() ? TEXT("*") : TEXT("-");
	UTextBlock* StarT = Text(WidgetTree, FString::Printf(TEXT("[ %s ]"), *Stars), 30, FLinearColor(1.f, 0.76f, 0.29f), true, true);
	Body->AddChildToVerticalBox(StarT)->SetPadding(FMargin(0.f, 8.f, 0.f, 6.f));

	// Tiga angka utama dalam kotak
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	auto Stat = [&](const FString& Big, const FString& Small, int32 Index)
	{
		UBorder* B = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		B->SetBrush(JRStyle::Panel(12.f, 2.f));
		B->SetBrushColor(FLinearColor(1.6f, 1.6f, 1.6f, 1.f));	// sedikit lebih terang dari kartu
		B->SetPadding(FMargin(8.f, 8.f));
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		V->AddChildToVerticalBox(Text(WidgetTree, Big, 18, JRStyle::Ink(), true, true));
		V->AddChildToVerticalBox(Text(WidgetTree, Small, 11, JRStyle::Muted(), false, true));
		B->SetContent(V);
		UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(B);
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(Index == 0 ? 0.f : 8.f, 0.f, 0.f, 0.f));
	};
	Stat(FString::Printf(TEXT("%d"), St.Customers), JRT(TEXT("pelanggan")), 0);
	Stat(JRFormatRp(GM->LastNet), JRT(TEXT("untung bersih")), 1);
	Stat(FString::Printf(TEXT("%dx"), St.Upsells), JRT(TEXT("\"nambah sejam\"")), 2);
	Body->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 4.f, 0.f, 10.f));

	// Momen hari ini (paling banyak 2) dan komentar warganet terbaru
	int32 Shown = 0;
	for (EJRMoment M : St.Moments)
	{
		if (Shown++ >= 2) break;
		Body->AddChildToVerticalBox(Text(WidgetTree, FString(TEXT("- ")) + AJRGameMode::MomentText(M), 14, JRStyle::Ink()))->SetPadding(FMargin(0.f, 2.f));
	}
	if (GM->Posts.Num() > 0 && GM->Posts[0].Day == GM->Day)
	{
		const FJRPost& P = GM->Posts[0];
		Body->AddChildToVerticalBox(Text(WidgetTree, FString::Printf(TEXT("%s: \"%s\""), *P.Author, *P.Text), 13, JRStyle::Muted()))
			->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
	Body->AddChildToVerticalBox(Text(WidgetTree, JRT(TEXT("#JuraganRental  -  KAH Gamedev")), 12, JRStyle::Accent(), true, true))
		->SetPadding(FMargin(0.f, 12.f, 0.f, 2.f));
	Body->AddChildToVerticalBox(Text(WidgetTree, JRT(TEXT("Screenshot kartu ini dan bagikan ke Threads atau Instagram!")), 11, JRStyle::Muted(), false, true));

	// Tombol
	UHorizontalBox* Btns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* A = Btns->AddChildToHorizontalBox(Btn(WidgetTree, JRT(TEXT("Laporan lengkap")), JRStyle::EBtn::Secondary, 15, [this]() { if (OnReport) OnReport(); }));
	A->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UHorizontalBoxSlot* B = Btns->AddChildToHorizontalBox(Btn(WidgetTree, JRT(TEXT("Buka toko besok")), JRStyle::EBtn::Primary, 15, [this]() { if (OnContinue) OnContinue(); }));
	B->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	B->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	Body->AddChildToVerticalBox(Btns)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	SetVisibility(ESlateVisibility::Visible);
}
