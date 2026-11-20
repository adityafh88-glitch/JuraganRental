#include "JRTouchWidget.h"
#include "JRLang.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "JRCharacter.h"
#include "JRGameMode.h"
#include "JRHUD.h"
#include "JRSettings.h"

namespace JRLocal_TouchWidget
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

	/** Tombol bulat tajam ala game HP: isi gelap, cincin tepi berwarna tegas. */
	FButtonStyle RingStyle(const FLinearColor& Ring, float Width)
	{
		FButtonStyle S;
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		B.OutlineSettings.Width = Width;
		B.OutlineSettings.Color = FSlateColor(Ring);
		B.TintColor = FSlateColor(FLinearColor(0.06f, 0.07f, 0.09f, 0.78f));
		S.SetNormal(B);
		B.TintColor = FSlateColor(FLinearColor(0.10f, 0.11f, 0.14f, 0.85f));
		S.SetHovered(B);
		B.TintColor = FSlateColor(Ring * FLinearColor(0.45f, 0.45f, 0.45f, 0.9f));
		S.SetPressed(B);
		S.SetNormalPadding(FMargin(0.f));
		S.SetPressedPadding(FMargin(0.f));
		return S;
	}

	/** Bodi HP jadul: kotak tegak bersudut membulat (bukan lingkaran). */
	FButtonStyle PhoneStyle(const FLinearColor& Color)
	{
		FButtonStyle S = RoundStyle(Color);
		for (FSlateBrush* B : { &S.Normal, &S.Hovered, &S.Pressed })
		{
			B->OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			B->OutlineSettings.CornerRadii = FVector4(12.f, 12.f, 12.f, 12.f);
		}
		S.SetNormalPadding(FMargin(7.f, 8.f));
		S.SetPressedPadding(FMargin(7.f, 9.f, 7.f, 7.f));
		return S;
	}

	FSlateBrush Box(float Radius)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		return B;
	}
}
using namespace JRLocal_TouchWidget;

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

UButton* UJRTouchWidget::MakeRing(const FString& Label, int32 FontSize, const FLinearColor& Ring)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	B->SetStyle(RingStyle(Ring, 4.f));
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Label));
	T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", FontSize));
	T->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.97f, 0.94f)));
	T->SetShadowOffset(FVector2D(1.f, 1.5f));
	T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	T->SetJustification(ETextJustify::Center);
	B->SetContent(T);
	return B;
}

UButton* UJRTouchWidget::MakePhoneButton()
{
	// Tombol berbentuk HP jadul seperti HP "JR-3310" di dalam game: layar LCD hijau dan keypad 3x3.
	UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	B->SetStyle(PhoneStyle(FLinearColor(0.16f, 0.18f, 0.22f, 0.95f)));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	B->SetContent(Col);

	UBorder* Screen = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Screen->SetBrush(Box(4.f));
	Screen->SetBrushColor(FLinearColor(0.42f, 0.55f, 0.24f));
	Screen->SetPadding(FMargin(2.f, 3.f));
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(TEXT("HP")));
	T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	T->SetColorAndOpacity(FSlateColor(FLinearColor(0.03f, 0.06f, 0.02f)));
	T->SetJustification(ETextJustify::Center);
	Screen->SetContent(T);
	Col->AddChildToVerticalBox(Screen)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	for (int32 Row = 0; Row < 3; ++Row)
	{
		UHorizontalBox* Keys = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 K = 0; K < 3; ++K)
		{
			UBorder* Key = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Key->SetBrush(Box(2.f));
			Key->SetBrushColor(FLinearColor(0.62f, 0.65f, 0.70f));
			USizeBox* KS = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			KS->SetHeightOverride(5.f);
			Key->SetContent(KS);
			UHorizontalBoxSlot* HS = Keys->AddChildToHorizontalBox(Key);
			HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HS->SetPadding(FMargin(K == 0 ? 0.f : 3.f, 0.f, 0.f, 0.f));
		}
		Col->AddChildToVerticalBox(Keys)->SetPadding(FMargin(2.f, Row == 0 ? 0.f : 3.f, 2.f, 0.f));
	}
	return B;
}

void UJRTouchWidget::BuildTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	// Area geser untuk menoleh: sisi kanan layar di bawah bilah status. Ditambahkan paling dulu
	// supaya tombol-tombol di atasnya tetap menerima ketukan.
	LookPad = CreateWidget<UJRLookPad>(GetOwningPlayer(), UJRLookPad::StaticClass());
	if (LookPad)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(LookPad);
		S->SetAnchors(FAnchors(0.42f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f, 44.f, 0.f, 0.f));
	}

	// Tombol utama di pojok kanan bawah, jangkauan jempol kanan.
	auto PlaceThumb = [Canvas](UWidget* W, float Size)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(1.f, 1.f));
		S->SetAlignment(FVector2D(1.f, 1.f));
		S->SetPosition(FVector2D(-40.f, -40.f));
		S->SetSize(FVector2D(Size, Size));
	};
	auto PlaceTopLeft = [Canvas](UWidget* W, float X, float Size)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(X, 56.f));
		S->SetSize(FVector2D(Size, Size));
	};

	ActionButton = MakeRing(JRT(TEXT("AKSI")), 14, FLinearColor(1.f, 0.76f, 0.29f));
	ActionLabel = Cast<UTextBlock>(ActionButton->GetContent());
	// Langsung bereaksi saat jari menyentuh (bawaan UButton menunggu jari diangkat dan batal bila jari bergeser sedikit).
	ActionButton->SetTouchMethod(EButtonTouchMethod::Down);
	ActionButton->SetClickMethod(EButtonClickMethod::MouseDown);
	ActionButton->SetPressMethod(EButtonPressMethod::ButtonPress);
	ActionButton->OnPressed.AddDynamic(this, &UJRTouchWidget::OnInteractPressed);
	ActionButton->OnReleased.AddDynamic(this, &UJRTouchWidget::OnInteractReleased);
	PlaceThumb(ActionButton, 108.f);

	// LANJUT di tempat yang sama dengan AKSI (tidak pernah tampil bersamaan).
	NextButton = MakeRing(JRT(TEXT("Lanjut")), 17, FLinearColor(0.36f, 0.85f, 0.56f));
	NextButton->OnClicked.AddDynamic(this, &UJRTouchWidget::OnNext);
	PlaceThumb(NextButton, 108.f);

	// Tombol yang jarang dipakai di kiri atas, di bawah bilah status: jeda lalu HP.
	UButton* Pause = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Pause->SetStyle(RingStyle(FLinearColor(0.85f, 0.87f, 0.9f), 3.f));
	{
		// Ikon jeda: dua batang putih (lebih tajam daripada huruf "II")
		UHorizontalBox* Bars = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 i = 0; i < 2; ++i)
		{
			UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Bar->SetBrush(Box(1.5f));
			Bar->SetBrushColor(FLinearColor(0.98f, 0.97f, 0.94f));
			USizeBox* BS = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			BS->SetWidthOverride(5.f);
			BS->SetHeightOverride(18.f);
			Bar->SetContent(BS);
			Bars->AddChildToHorizontalBox(Bar)->SetPadding(FMargin(i == 0 ? 0.f : 6.f, 0.f, 0.f, 0.f));
		}
		Pause->SetContent(Bars);
		if (UButtonSlot* PS = Cast<UButtonSlot>(Pause->GetContentSlot()))
		{
			PS->SetHorizontalAlignment(HAlign_Center);
			PS->SetVerticalAlignment(VAlign_Center);
		}
	}
	Pause->OnClicked.AddDynamic(this, &UJRTouchWidget::OnPause);
	PlaceTopLeft(Pause, 24.f, 52.f);
	PauseButton = Pause;

	// Ketuk judul panel misi (digambar HUD di x=180, y=50) untuk membuka/menciutkan daftar misi.
	MissionToggle = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	{
		FButtonStyle Clear;
		FSlateBrush None;
		None.DrawAs = ESlateBrushDrawType::NoDrawType;
		Clear.SetNormal(None); Clear.SetHovered(None); Clear.SetPressed(None);
		MissionToggle->SetStyle(Clear);
		MissionToggle->OnClicked.AddDynamic(this, &UJRTouchWidget::OnMissions);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(MissionToggle);
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(176.f, 46.f));
		S->SetSize(FVector2D(260.f, 38.f));
	}

	UButton* Phone = MakePhoneButton();
	PhoneButton = Phone;
	Phone->OnClicked.AddDynamic(this, &UJRTouchWidget::OnPhone);
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Phone);
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(92.f, 48.f));
		S->SetSize(FVector2D(54.f, 82.f));	// tegak seperti HP
	}
}

void UJRTouchWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SyncVisibility();

	// Label tombol aksi = kata kerja dari petunjuk ("Nyalakan PS 1" -> NYALAKAN, "Tahan E: restart" -> TAHAN / RESTART).
	FString Label = JRT(TEXT("AKSI"));
	bool bActive = false;
	if (const AJRCharacter* C = Cast<AJRCharacter>(GetOwningPlayerPawn()))
	{
		const FJRInteraction& I = C->CurrentInteraction;
		if (I.Kind == EJRInteractKind::Press || I.Kind == EJRInteractKind::Hold)
		{
			bActive = true;
			FString Prompt = I.Prompt;
			FString Left, Right;
			if (Prompt.Split(TEXT(":"), &Left, &Right) && Left.StartsWith(JRT(TEXT("Tahan"))))
			{
				Right.TrimStartInline();
				Right.Split(TEXT(" "), &Right, nullptr);
				Label = FString(JRT(TEXT("TAHAN"))) + TEXT("\n") + Right.ToUpper();
			}
			else
			{
				Prompt.Split(TEXT(" "), &Left, nullptr);
				Label = (Left.IsEmpty() ? Prompt : Left).ToUpper();
			}
		}
	}
	if (Label != ShownLabel && ActionLabel)
	{
		ShownLabel = Label;
		ActionLabel->SetText(FText::FromString(Label));
	}
	if (ActionButton) ActionButton->SetRenderOpacity(bActive ? 1.f : 0.6f);
}

void UJRTouchWidget::SyncVisibility()
{
	const AJRGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr;
	const AJRHUD* Hud = GetOwningPlayer() ? Cast<AJRHUD>(GetOwningPlayer()->GetHUD()) : nullptr;
	// HP terbuka juga menyembunyikan tombol: HP menutupi sisi kanan layar dan punya tombol X sendiri.
	const bool bHideAll = !GM || GM->Phase == EJRGamePhase::Menu || (Hud && Hud->IsUIOpen());
	const ESlateVisibility Root = bHideAll ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible;
	if (GetVisibility() != Root) SetVisibility(Root);
	const bool bShowNext = GM && GM->Phase != EJRGamePhase::Playing;
	const ESlateVisibility Want = bShowNext ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (NextButton && NextButton->GetVisibility() != Want) NextButton->SetVisibility(Want);
	// Saat adegan pembuka, tombol LANJUT berfungsi sebagai LEWATI.
	if (UTextBlock* NL = NextButton ? Cast<UTextBlock>(NextButton->GetContent()) : nullptr)
	{
		const FString Want2 = (GM && GM->IsOpening()) ? JRT(TEXT("Lewati")) : JRT(TEXT("Lanjut"));
		if (NL->GetText().ToString() != Want2) NL->SetText(FText::FromString(Want2));
	}
	const ESlateVisibility WantMis = (GM && GM->Phase == EJRGamePhase::Playing && !GM->IsTutorial()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (MissionToggle && MissionToggle->GetVisibility() != WantMis) MissionToggle->SetVisibility(WantMis);
	// Adegan pembuka: hanya tombol LEWATI yang tampil.
	const bool bOpening = GM && GM->IsOpening();
	for (UButton* B : { PauseButton.Get(), PhoneButton.Get() })
	{
		const ESlateVisibility V = bOpening ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
		if (B && B->GetVisibility() != V) B->SetVisibility(V);
	}
	const ESlateVisibility WantAction = bShowNext ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
	if (ActionButton && ActionButton->GetVisibility() != WantAction) ActionButton->SetVisibility(WantAction);
}

TSharedRef<SWidget> UJRLookPad::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// Lapisan transparan yang bisa disentuh (widget kosong tidak menangkap sentuhan).
		UBorder* Surface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Surface"));
		Surface->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
		Surface->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Surface;
	}
	SetVisibility(ESlateVisibility::Visible);
	return Super::RebuildWidget();
}

FReply UJRLookPad::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (Finger >= 0) return FReply::Unhandled();
	Finger = InGestureEvent.GetPointerIndex();
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UJRLookPad::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (InGestureEvent.GetPointerIndex() != Finger) return FReply::Unhandled();
	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn) return FReply::Handled();
	// Geser dalam satuan UI (sudah disesuaikan ukuran layar): satu lebar layar kira-kira satu putaran.
	const FVector2D D = InGestureEvent.GetCursorDelta() / FMath::Max(InGeometry.Scale, 0.01f);
	const UJRSettingsSave* S = JRSettings::Get();
	const float K = 0.075f * S->LookSpeed;
	Pawn->AddControllerYawInput(D.X * K);
	Pawn->AddControllerPitchInput((S->bInvertLook ? -D.Y : D.Y) * K);
	return FReply::Handled();
}

FReply UJRLookPad::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (InGestureEvent.GetPointerIndex() != Finger) return FReply::Unhandled();
	Finger = -1;
	return FReply::Handled().ReleaseMouseCapture();
}

void UJRTouchWidget::OnMissions()
{
	UJRSettingsSave* S = JRSettings::Get();
	S->bMissionsCompact = !S->bMissionsCompact;
	JRSettings::Save();
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

void UJRTouchWidget::OnPause()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AJRHUD* Hud = Cast<AJRHUD>(PC->GetHUD())) Hud->TogglePause();
	}
}

void UJRTouchWidget::OnNext()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->Confirm();
}
