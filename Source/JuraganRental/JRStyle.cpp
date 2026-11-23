#include "JRStyle.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

// Bentuk widget rapi (sudut bulat, garis tepi, tab pil, efek tekan) dengan WARNA KHAS JURAGAN RENTAL
// yang senada dengan logo papan nama: panel cokelat hangat bergaris emas, tombol utama oranye-kuning,
// tab aktif tosca (warna dinding garasi); tema LCD hijau untuk HP jadul.
namespace JRStyle
{
	FLinearColor FromHex(uint32 Rgb, float Alpha)
	{
		FLinearColor C = FLinearColor::FromSRGBColor(FColor((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255));
		C.A = Alpha;
		return C;
	}

	FLinearColor Cream() { return FLinearColor(0.085f, 0.058f, 0.04f, 0.97f); }	// panel cokelat tua hangat
	FLinearColor CreamDeep() { return FLinearColor(0.17f, 0.115f, 0.075f); }
	FLinearColor Brown() { return FLinearColor(0.62f, 0.40f, 0.12f); }			// garis tepi emas
	FLinearColor Ink() { return FLinearColor(0.97f, 0.93f, 0.86f); }			// teks krem terang
	FLinearColor Muted() { return FLinearColor(0.70f, 0.63f, 0.54f); }
	FLinearColor Accent() { return FLinearColor(1.f, 0.76f, 0.29f); }			// kuning-emas
	FLinearColor Good() { return FLinearColor(0.36f, 0.79f, 0.54f); }

	namespace
	{
		struct FPal { FLinearColor Fill, Hover, Press, Line, Text; bool bShadow; };
		FPal Palette(EBtn K, ETheme T)
		{
			if (T == ETheme::Lcd)
			{
				// HP jadul: tombol hijau tua di atas layar LCD hijau
				switch (K)
				{
				case EBtn::TabActive: return { FLinearColor(0.02f, 0.04f, 0.01f), FLinearColor(0.03f, 0.06f, 0.02f), FLinearColor(0.01f, 0.02f, 0.f), FLinearColor(0.f, 0.f, 0.f), FLinearColor(0.42f, 0.55f, 0.24f), false };
				case EBtn::Tab:       return { FLinearColor(0.30f, 0.41f, 0.16f), FLinearColor(0.34f, 0.46f, 0.18f), FLinearColor(0.22f, 0.31f, 0.11f), FLinearColor(0.10f, 0.16f, 0.06f), FLinearColor(0.03f, 0.06f, 0.02f), false };
				case EBtn::Danger:    return { FLinearColor(0.75f, 0.2f, 0.17f), FLinearColor(0.82f, 0.25f, 0.2f), FLinearColor(0.6f, 0.15f, 0.12f), FLinearColor(0.45f, 0.1f, 0.08f), FLinearColor::White, true };
				case EBtn::Disabled:  return { FLinearColor(0.55f, 0.68f, 0.36f), FLinearColor(0.55f, 0.68f, 0.36f), FLinearColor(0.55f, 0.68f, 0.36f), FLinearColor(0.36f, 0.47f, 0.2f), FLinearColor(0.10f, 0.17f, 0.06f), false };
				default:              return { FLinearColor(0.10f, 0.16f, 0.06f), FLinearColor(0.14f, 0.21f, 0.08f), FLinearColor(0.05f, 0.09f, 0.03f), FLinearColor(0.03f, 0.06f, 0.02f), FLinearColor(0.42f, 0.55f, 0.24f), false };
				}
			}
			switch (K)
			{
			case EBtn::Primary:   return { FLinearColor(1.f, 0.58f, 0.10f), FLinearColor(1.f, 0.66f, 0.18f), FLinearColor(0.86f, 0.46f, 0.06f), FLinearColor(0.45f, 0.20f, 0.03f), FLinearColor(0.14f, 0.06f, 0.f), false };
			case EBtn::TabActive: return { FLinearColor(0.20f, 0.70f, 0.62f), FLinearColor(0.26f, 0.78f, 0.70f), FLinearColor(0.14f, 0.56f, 0.50f), FLinearColor(0.06f, 0.30f, 0.26f), FLinearColor(0.02f, 0.08f, 0.07f), false };
			case EBtn::Danger:    return { FLinearColor(0.75f, 0.2f, 0.17f), FLinearColor(0.82f, 0.25f, 0.2f), FLinearColor(0.6f, 0.15f, 0.12f), FLinearColor(0.45f, 0.1f, 0.08f), FLinearColor::White, true };
			case EBtn::Disabled:  return { FLinearColor(0.10f, 0.075f, 0.055f), FLinearColor(0.10f, 0.075f, 0.055f), FLinearColor(0.10f, 0.075f, 0.055f), FLinearColor(0.17f, 0.13f, 0.09f), FLinearColor(0.42f, 0.37f, 0.31f), false };
			default:              return { FLinearColor(0.20f, 0.13f, 0.085f), FLinearColor(0.26f, 0.17f, 0.11f), FLinearColor(0.14f, 0.09f, 0.06f), FLinearColor(0.42f, 0.27f, 0.13f), FLinearColor(0.97f, 0.93f, 0.86f), true };	// Secondary, Tab
			}
		}

		FSlateBrush Rounded(const FLinearColor& Fill, const FLinearColor& Line, float Radius, float Width, bool bPill)
		{
			FSlateBrush B;
			B.DrawAs = ESlateBrushDrawType::RoundedBox;
			B.TintColor = FSlateColor(Fill);
			B.OutlineSettings.RoundingType = bPill ? ESlateBrushRoundingType::HalfHeightRadius : ESlateBrushRoundingType::FixedRadius;
			B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
			B.OutlineSettings.Width = Width;
			B.OutlineSettings.Color = FSlateColor(Line);
			return B;
		}
	}

	FButtonStyle Button(EBtn Kind, float Radius, ETheme Theme)
	{
		const FPal P = Palette(Kind, Theme);
		const FPal D = Palette(EBtn::Disabled, Theme);
		const bool bPill = Kind == EBtn::Tab || Kind == EBtn::TabActive;
		FButtonStyle S;
		S.SetNormal(Rounded(P.Fill, P.Line, Radius, 2.f, bPill));
		S.SetHovered(Rounded(P.Hover, P.Line, Radius, 2.f, bPill));
		S.SetPressed(Rounded(P.Press, P.Line, Radius, 2.f, bPill));
		S.SetDisabled(Rounded(D.Fill, D.Line, Radius, 2.f, bPill));
		// Saat ditekan isi tombol turun sedikit (terasa "membal")
		S.SetNormalPadding(FMargin(0.f, 0.f, 0.f, 2.f));
		S.SetPressedPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		return S;
	}

	FLinearColor TextOn(EBtn Kind, ETheme Theme) { return Palette(Kind, Theme).Text; }

	void StyleText(UTextBlock* T, EBtn Kind, ETheme Theme)
	{
		if (!T) return;
		const FPal P = Palette(Kind, Theme);
		T->SetColorAndOpacity(FSlateColor(P.Text));
		T->SetShadowOffset(P.bShadow ? FVector2D(0.f, 1.5f) : FVector2D::ZeroVector);
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, P.bShadow ? 0.45f : 0.f));
	}

	void Apply(UButton* B, EBtn Kind, float Radius, ETheme Theme)
	{
		if (!B) return;
		B->SetStyle(Button(Kind, Radius, Theme));
		B->SetBackgroundColor(FLinearColor::White);
		B->SetColorAndOpacity(FLinearColor::White);
		StyleText(Cast<UTextBlock>(B->GetContent()), Kind, Theme);
	}

	FSlateBrush Panel(float Radius, float Outline)
	{
		return Rounded(Cream(), Brown(), Radius, Outline, false);
	}
}
