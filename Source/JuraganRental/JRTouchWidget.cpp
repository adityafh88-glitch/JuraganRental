#include "JRTouchWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "JRCharacter.h"
#include "JRGameMode.h"
#include "JRHUD.h"

namespace
{
	FButtonStyle RoundStyle(const FLinearColor& Color)
	{
		FButtonStyle S;
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		B.TintColor = FSlateColor(Color);
		S.SetNormal(B);
		B.TintColor = FSlateColor(Color * 1.15f);
		S.SetHovered(B);
		B.TintColor = FSlateColor(Color * 0.75f);
		S.SetPressed(B);
		S.SetNormalPadding(FMargin(0.f));
		S.SetPressedPadding(FMargin(0.f));
		return S;
	}
}

TSharedRef<SWidget> UJRTouchWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

UButton* UJRTouchWidget::MakeRound(const FString& Label, float Size, const FLinearColor& Color, const FLinearColor& TextColor)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	B->SetStyle(RoundStyle(Color));
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Label));
	T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", FMath::RoundToInt(Size * 0.26f)));
	T->SetColorAndOpacity(FSlateColor(TextColor));
	T->SetJustification(ETextJustify::Center);
	B->SetContent(T);
	return B;
}

void UJRTouchWidget::BuildTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	auto Place = [Canvas](UWidget* W, const FVector2D& OffsetFromRightBottom, float Size)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(1.f, 1.f));
		S->SetAlignment(FVector2D(1.f, 1.f));
		S->SetPosition(-OffsetFromRightBottom);
		S->SetSize(FVector2D(Size, Size));
	};

	// Di atas joystick kanan (joystick bawaan berada di pojok kanan bawah).
	UButton* E = MakeRound(TEXT("E"), 130.f, FLinearColor(1.f, 0.76f, 0.29f, 0.92f), FLinearColor(0.1f, 0.07f, 0.f));
	E->OnPressed.AddDynamic(this, &UJRTouchWidget::OnInteractPressed);
	E->OnReleased.AddDynamic(this, &UJRTouchWidget::OnInteractReleased);
	Place(E, FVector2D(60.f, 330.f), 130.f);

	UButton* Phone = MakeRound(TEXT("HP"), 96.f, FLinearColor(0.37f, 0.70f, 1.f, 0.9f), FLinearColor(0.03f, 0.07f, 0.12f));
	Phone->OnClicked.AddDynamic(this, &UJRTouchWidget::OnPhone);
	Place(Phone, FVector2D(220.f, 380.f), 96.f);

	NextButton = MakeRound(TEXT("Lanjut"), 120.f, FLinearColor(0.36f, 0.79f, 0.54f, 0.95f), FLinearColor(0.02f, 0.1f, 0.05f));
	NextButton->OnClicked.AddDynamic(this, &UJRTouchWidget::OnNext);
	Place(NextButton, FVector2D(60.f, 180.f), 120.f);
}

void UJRTouchWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const AJRGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr;
	const bool bShowNext = GM && GM->Phase != EJRGamePhase::Playing;
	const ESlateVisibility Want = bShowNext ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (NextButton && NextButton->GetVisibility() != Want) NextButton->SetVisibility(Want);
}

void UJRTouchWidget::OnInteractPressed()
{
	if (AJRCharacter* C = Cast<AJRCharacter>(GetOwningPlayerPawn())) C->InteractStarted();
}

void UJRTouchWidget::OnInteractReleased()
{
	if (AJRCharacter* C = Cast<AJRCharacter>(GetOwningPlayerPawn())) C->InteractCompleted();
}

void UJRTouchWidget::OnPhone()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AJRHUD* Hud = Cast<AJRHUD>(PC->GetHUD())) Hud->TogglePhone();
	}
}

void UJRTouchWidget::OnNext()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->Confirm();
}
