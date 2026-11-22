#include "JRChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "JRPhoneWidget.h"
#include "JRStyle.h"
#include "Components/ButtonSlot.h"

namespace JRLocal_ChoiceWidget
{
	const FLinearColor Card(0.10f, 0.12f, 0.15f, 0.97f);
	const FLinearColor Ink(0.95f, 0.93f, 0.89f);
	const FLinearColor Led(1.f, 0.76f, 0.29f);
	const FLinearColor Crt(0.37f, 0.70f, 1.f);
	const FLinearColor CrtInk(0.03f, 0.07f, 0.12f);
	const FLinearColor Alt(0.18f, 0.21f, 0.26f);

	FSlateBrush RoundedBrush(float Radius)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		return B;
	}
}
using namespace JRLocal_ChoiceWidget;

TSharedRef<SWidget> UJRChoiceWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UJRChoiceWidget::BuildTree()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Latar gelap transparan di belakang kartu
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Size->SetWidthOverride(580.f);
	UOverlaySlot* S = Root->AddChildToOverlay(Size);
	S->SetHorizontalAlignment(HAlign_Center);
	S->SetVerticalAlignment(VAlign_Center);

	UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Box->SetBrush(JRStyle::Panel(22.f));
	Box->SetBrushColor(FLinearColor::White);
	Box->SetPadding(FMargin(30.f, 26.f));
	Size->AddChild(Box);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->SetContent(Col);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 24));
	TitleText->SetColorAndOpacity(FSlateColor(Led));
	Col->AddChildToVerticalBox(TitleText);

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BodyText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
	BodyText->SetColorAndOpacity(FSlateColor(Ink));
	BodyText->SetAutoWrapText(true);
	Col->AddChildToVerticalBox(BodyText)->SetPadding(FMargin(0.f, 12.f, 0.f, 18.f));

	Buttons = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Col->AddChildToVerticalBox(Buttons);
}

void UJRChoiceWidget::Show(const FString& Title, const FString& Body, const TArray<FString>& Labels, bool bNeutral)
{
	TakeWidget(); // pastikan pohon widget sudah dibangun
	TitleText->SetText(FText::FromString(Title));
	BodyText->SetText(FText::FromString(Body));
	Buttons->ClearChildren();
	for (int32 i = 0; i < Labels.Num(); ++i)
	{
		UJRPhoneButton* B = WidgetTree->ConstructWidget<UJRPhoneButton>(UJRPhoneButton::StaticClass());
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(Labels[i]));
		T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
		T->SetColorAndOpacity(FSlateColor(i == 0 && !bNeutral ? CrtInk : Ink));
		T->SetJustification(ETextJustify::Center);
		T->SetAutoWrapText(true);	// label kartu rencana bisa panjang (terutama bahasa Inggris)
		if (UButtonSlot* BSl = Cast<UButtonSlot>(B->SetContent(T))) BSl->SetPadding(FMargin(16.f, 11.f));
		JRStyle::Apply(B, i == 0 && !bNeutral ? JRStyle::EBtn::Primary : JRStyle::EBtn::Secondary);
		B->Bind([this, i]() { if (OnPick) OnPick(i); });
		UVerticalBoxSlot* BS = Buttons->AddChildToVerticalBox(B);
		BS->SetPadding(FMargin(0.f, i == 0 ? 0.f : 8.f, 0.f, 0.f));
	}
}
