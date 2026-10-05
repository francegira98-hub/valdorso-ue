// Valdorso - La firma vera (vedi il .h).

#include "SValdorsoFirma.h"
#include "ValdorsoRegistro.h"
#include "ValdorsoTemaUI.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"

namespace
{
	/** Quanto deve muoversi la penna (in frazioni del riquadro) prima di segnare un punto nuovo. */
	constexpr float FirmaPassoMinimo = 0.006f;
	/** La penna del controller: quanto riquadro percorre in un secondo con lo stick tutto spinto. */
	constexpr float FirmaVelocitaPenna = 0.55f;
}

void SValdorsoFirma::Construct(const FArguments& InArgs)
{
	Nome = InArgs._Nome;
	FontNome = InArgs._FontNome;
	bModificabile = InArgs._bModificabile;
	Grandezza = InArgs._Grandezza;
	OnCambia = InArgs._OnCambia;
	ImpostaFirma(InArgs._Firma);
}

void SValdorsoFirma::ImpostaFirma(const FString& Nuova)
{
	Tratti.Reset();
	bDisegnando = false;
	bColNome = Nuova == ValdorsoRegistro::FirmaColNome;
	TArray<TArray<FIntPoint>> Letti;
	if (!bColNome && ValdorsoRegistro::LeggiFirma(Nuova, Letti))
	{
		for (const TArray<FIntPoint>& Tratto : Letti)
		{
			TArray<FVector2f>& Nuovo = Tratti.AddDefaulted_GetRef();
			for (const FIntPoint& P : Tratto)
			{
				Nuovo.Add(FVector2f(P.X / 999.f, P.Y / 999.f));
			}
		}
	}
}

FString SValdorsoFirma::Firma() const
{
	if (bColNome)
	{
		return ValdorsoRegistro::FirmaColNome;
	}
	TArray<FString> Parti;
	for (const TArray<FVector2f>& Tratto : Tratti)
	{
		if (Tratto.Num() == 0)
		{
			continue;
		}
		TArray<FString> Punti;
		for (const FVector2f& P : Tratto)
		{
			Punti.Add(FString::Printf(TEXT("%d,%d"),
				FMath::Clamp(FMath::RoundToInt(P.X * 999.f), 0, 999), FMath::Clamp(FMath::RoundToInt(P.Y * 999.f), 0, 999)));
		}
		Parti.Add(FString::Join(Punti, TEXT(" ")));
	}
	return FString::Join(Parti, TEXT(";"));
}

int32 SValdorsoFirma::ContaPunti() const
{
	int32 Totale = 0;
	for (const TArray<FVector2f>& Tratto : Tratti)
	{
		Totale += Tratto.Num();
	}
	return Totale;
}

void SValdorsoFirma::IniziaTratto(const FVector2f& Punto)
{
	if (Tratti.Num() >= ValdorsoRegistro::MassimoTrattiFirma || ContaPunti() >= ValdorsoRegistro::MassimoPuntiFirma)
	{
		return;
	}
	// Chi comincia a disegnare lascia la firma col nome.
	bColNome = false;
	bDisegnando = true;
	Tratti.AddDefaulted();
	AggiungiPunto(Punto, true);
}

void SValdorsoFirma::AggiungiPunto(const FVector2f& Punto, bool bSempre)
{
	if (!bDisegnando || Tratti.Num() == 0 || ContaPunti() >= ValdorsoRegistro::MassimoPuntiFirma)
	{
		return;
	}
	const FVector2f Dentro(FMath::Clamp(Punto.X, 0.f, 1.f), FMath::Clamp(Punto.Y, 0.f, 1.f));
	TArray<FVector2f>& Tratto = Tratti.Last();
	if (bSempre || Tratto.Num() == 0 || FVector2f::Distance(Tratto.Last(), Dentro) >= FirmaPassoMinimo)
	{
		Tratto.Add(Dentro);
	}
}

void SValdorsoFirma::FinisciTratto()
{
	if (!bDisegnando)
	{
		return;
	}
	bDisegnando = false;
	OnCambia.ExecuteIfBound(Firma());
}

int32 SValdorsoFirma::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Misura = FVector2f(AllottedGeometry.GetLocalSize());
	const FLinearColor Inchiostro = ValdorsoTema::Inchiostro() * InWidgetStyle.GetColorAndOpacityTint();
	const FLinearColor Riga = ValdorsoTema::InchiostroTenue().CopyWithNewOpacity(0.45f) * InWidgetStyle.GetColorAndOpacityTint();

	// La riga su cui si firma, a tre quarti dell'altezza, con una piccola croce all'inizio (come nei registri).
	const float AltezzaRiga = Misura.Y * 0.78f;
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ FVector2f(Misura.X * 0.04f, AltezzaRiga), FVector2f(Misura.X * 0.96f, AltezzaRiga) },
		ESlateDrawEffect::None, Riga, true, 1.f);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ FVector2f(Misura.X * 0.04f - 6.f, AltezzaRiga - 14.f), FVector2f(Misura.X * 0.04f + 6.f, AltezzaRiga - 2.f) },
		ESlateDrawEffect::None, Riga, true, 1.f);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ FVector2f(Misura.X * 0.04f + 6.f, AltezzaRiga - 14.f), FVector2f(Misura.X * 0.04f - 6.f, AltezzaRiga - 2.f) },
		ESlateDrawEffect::None, Riga, true, 1.f);

	if (bColNome && FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
	{
		// La firma col nome: il nome in calligrafia, appoggiato sulla riga.
		const TSharedRef<FSlateFontMeasure> Misuratore = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const FVector2f Testo = FVector2f(Misuratore->Measure(Nome, FontNome));
		const FVector2f Posizione(FMath::Max(Misura.X * 0.08f, (Misura.X - Testo.X) * 0.5f), AltezzaRiga - Testo.Y * 0.82f);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(Testo), FSlateLayoutTransform(1.f, FVector2D(Posizione))),
			Nome, FontNome, ESlateDrawEffect::None, Inchiostro);
	}

	// I tratti di penna.
	for (const TArray<FVector2f>& Tratto : Tratti)
	{
		TArray<FVector2f> Punti;
		Punti.Reserve(Tratto.Num() + 1);
		for (const FVector2f& P : Tratto)
		{
			Punti.Add(FVector2f(P.X * Misura.X, P.Y * Misura.Y));
		}
		if (Punti.Num() == 1)
		{
			// Un punto solo (un puntino sulla i): un trattino piccolissimo.
			Punti.Add(Punti[0] + FVector2f(1.5f, 0.5f));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Punti,
			ESlateDrawEffect::None, Inchiostro, true, 2.6f);
	}

	// La penna del controller, quando si usa lo stick.
	if (bModificabile && bPennaVista && HasKeyboardFocus())
	{
		const FVector2f Centro(Penna.X * Misura.X, Penna.Y * Misura.Y);
		const FLinearColor Colore = (bPennaGiu ? ValdorsoTema::Rubrica() : ValdorsoTema::InchiostroTenue()) * InWidgetStyle.GetColorAndOpacityTint();
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>{ Centro + FVector2f(-7.f, 0.f), Centro + FVector2f(7.f, 0.f) }, ESlateDrawEffect::None, Colore, true, 1.5f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>{ Centro + FVector2f(0.f, -7.f), Centro + FVector2f(0.f, 7.f) }, ESlateDrawEffect::None, Colore, true, 1.5f);
	}
	return LayerId + 2;
}

FReply SValdorsoFirma::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bModificabile || !IsEnabled() || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPennaVista = false;
	const FVector2f Locale = FVector2f(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
	const FVector2f Misura = FVector2f(MyGeometry.GetLocalSize());
	IniziaTratto(FVector2f(Locale.X / FMath::Max(Misura.X, 1.f), Locale.Y / FMath::Max(Misura.Y, 1.f)));
	return FReply::Handled().CaptureMouse(AsShared());
}

FReply SValdorsoFirma::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bDisegnando || !HasMouseCapture())
	{
		return FReply::Unhandled();
	}
	const FVector2f Locale = FVector2f(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
	const FVector2f Misura = FVector2f(MyGeometry.GetLocalSize());
	AggiungiPunto(FVector2f(Locale.X / FMath::Max(Misura.X, 1.f), Locale.Y / FMath::Max(Misura.Y, 1.f)));
	return FReply::Handled();
}

FReply SValdorsoFirma::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !HasMouseCapture())
	{
		return FReply::Unhandled();
	}
	FinisciTratto();
	return FReply::Handled().ReleaseMouseCapture();
}

FCursorReply SValdorsoFirma::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// Sul riquadro la freccia diventa una croce sottile: si capisce che lì si scrive.
	return bModificabile ? FCursorReply::Cursor(EMouseCursor::Crosshairs) : FCursorReply::Unhandled();
}

FReply SValdorsoFirma::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (bModificabile && IsEnabled() && InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (!InKeyEvent.IsRepeat() && !bPennaGiu)
		{
			bPennaGiu = true;
			bPennaVista = true;
			IniziaTratto(Penna);
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SValdorsoFirma::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Bottom && bPennaGiu)
	{
		bPennaGiu = false;
		FinisciTratto();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SValdorsoFirma::OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	if (!bModificabile)
	{
		return FReply::Unhandled();
	}
	const FKey Chiave = InAnalogInputEvent.GetKey();
	const float Valore = InAnalogInputEvent.GetAnalogValue();
	const float Pulito = FMath::Abs(Valore) < 0.15f ? 0.f : Valore;   // lo stick fermo non è mai a zero preciso
	if (Chiave == EKeys::Gamepad_LeftX)
	{
		Spinta.X = Pulito;
	}
	else if (Chiave == EKeys::Gamepad_LeftY)
	{
		Spinta.Y = -Pulito;   // stick in su = penna in su (sullo schermo la y cresce verso il basso)
	}
	else
	{
		return FReply::Unhandled();
	}
	if (!Spinta.IsNearlyZero())
	{
		bPennaVista = true;
	}
	return FReply::Handled();
}

void SValdorsoFirma::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SLeafWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (!bModificabile || Spinta.IsNearlyZero() || !HasKeyboardFocus())
	{
		return;
	}
	// La penna va più piano di quanto si sposta lo sguardo: il riquadro è largo quattro volte l'altezza.
	const FVector2f Misura = FVector2f(AllottedGeometry.GetLocalSize());
	const float Rapporto = Misura.Y > 1.f ? Misura.X / Misura.Y : 4.f;
	Penna.X = FMath::Clamp(Penna.X + Spinta.X * FirmaVelocitaPenna * InDeltaTime, 0.f, 1.f);
	Penna.Y = FMath::Clamp(Penna.Y + Spinta.Y * FirmaVelocitaPenna * Rapporto * InDeltaTime, 0.f, 1.f);
	if (bPennaGiu)
	{
		AggiungiPunto(Penna);
	}
}

void SValdorsoFirma::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	SLeafWidget::OnFocusLost(InFocusEvent);
	Spinta = FVector2f::ZeroVector;
	if (bPennaGiu)
	{
		bPennaGiu = false;
		FinisciTratto();
	}
}

void SValdorsoFirma::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	SLeafWidget::OnMouseCaptureLost(CaptureLostEvent);
	if (!bPennaGiu)
	{
		FinisciTratto();
	}
}
