// Valdorso - La polvere nella luce (vedi il .h).

#include "SValdorsoPolvere.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"

namespace
{
	/** Il raggio: entra in alto a destra e scende verso il ritratto (coordinate da 0 a 1 dello schermo). */
	const FVector2f PolvereRaggioInizio(0.97f, -0.10f);
	const FVector2f PolvereRaggioFine(0.62f, 1.10f);
	/** Mezza larghezza del raggio, in frazioni della larghezza dello schermo. */
	constexpr float PolvereRaggioLargo = 0.11f;
}

void SValdorsoPolvere::Construct(const FArguments& InArgs)
{
	Caso.Initialize(static_cast<int32>(FPlatformTime::Cycles()));
	Pennello = FSlateRoundedBoxBrush(FLinearColor::White, 100.f);

	Granelli.SetNum(FMath::Max(0, InArgs._Numero));
	for (FGranello& Granello : Granelli)
	{
		Rinasci(Granello, true);
	}

	SetVisibility(EVisibility::HitTestInvisible);
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoPolvere::Aggiorna));
}

void SValdorsoPolvere::Rinasci(FGranello& Granello, bool bAllInizio)
{
	// Metà dei granelli nascono vicino al raggio (lì si vedono), gli altri ovunque nella metà destra.
	if (Caso.FRand() < 0.55f)
	{
		const float Lungo = Caso.FRand();
		const FVector2f SulRaggio = PolvereRaggioInizio + (PolvereRaggioFine - PolvereRaggioInizio) * Lungo;
		Granello.Posizione = SulRaggio + FVector2f(Caso.FRandRange(-1.f, 1.f) * PolvereRaggioLargo * 1.3f, 0.f);
	}
	else
	{
		Granello.Posizione = FVector2f(Caso.FRandRange(0.45f, 1.f), Caso.FRand());
	}
	// Quasi ferma: scende pianissimo e ondeggia.
	Granello.Deriva = FVector2f(Caso.FRandRange(-0.006f, 0.006f), Caso.FRandRange(0.002f, 0.010f));
	Granello.Fase = Caso.FRandRange(0.f, 2.f * PI);
	Granello.Dimensione = Caso.FRandRange(1.0f, 2.6f);
	Granello.Vita = Caso.FRandRange(14.f, 30.f);
	Granello.Eta = bAllInizio ? Caso.FRandRange(0.f, Granello.Vita) : 0.f;
}

float SValdorsoPolvere::NelRaggio(const FVector2f& Punto)
{
	// Distanza dalla retta del raggio (in orizzontale, perché il raggio è quasi verticale).
	const FVector2f Asse = PolvereRaggioFine - PolvereRaggioInizio;
	const float T = FMath::Clamp((Punto.Y - PolvereRaggioInizio.Y) / Asse.Y, 0.f, 1.f);
	const float XAsse = PolvereRaggioInizio.X + Asse.X * T;
	const float D = (Punto.X - XAsse) / PolvereRaggioLargo;
	// Il raggio si allarga un poco e si fa più tenue scendendo.
	return FMath::Exp(-D * D * 1.6f) * (1.f - 0.35f * T);
}

EActiveTimerReturnType SValdorsoPolvere::Aggiorna(double Tempo, float Delta)
{
	Delta = FMath::Min(Delta, 0.1f);
	for (FGranello& Granello : Granelli)
	{
		Granello.Eta += Delta;
		Granello.Posizione += Granello.Deriva * Delta;
		Granello.Posizione.X += FMath::Sin(Granello.Eta * 0.7f + Granello.Fase) * 0.004f * Delta;
		Granello.Posizione.Y += FMath::Cos(Granello.Eta * 0.5f + Granello.Fase) * 0.003f * Delta;
		if (Granello.Eta > Granello.Vita || Granello.Posizione.Y > 1.05f || Granello.Posizione.X < 0.f || Granello.Posizione.X > 1.02f)
		{
			Rinasci(Granello, false);
		}
	}
	return EActiveTimerReturnType::Continue;
}

int32 SValdorsoPolvere::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Misura = FVector2f(AllottedGeometry.GetLocalSize());
	const FLinearColor Tinta = InWidgetStyle.GetColorAndOpacityTint();
	const float Respiro = 0.85f + 0.15f * ValdorsoTema::Battito(FPlatformTime::Seconds());
	const FLinearColor Luce = FMath::Lerp(ValdorsoTema::Pergamena(), ValdorsoTema::OroChiaro(), 0.4f);

	// Il raggio: tanti chiarori grandi e tenuissimi in fila lungo l'asse (sembra una lama di luce nella penombra).
	const int32 Passi = 16;
	for (int32 i = 0; i < Passi; ++i)
	{
		const float T = (i + 0.5f) / Passi;
		const FVector2f Centro = (PolvereRaggioInizio + (PolvereRaggioFine - PolvereRaggioInizio) * T) * Misura;
		const float Lato = Misura.X * PolvereRaggioLargo * (2.2f + 0.6f * T);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(Lato, Lato), FSlateLayoutTransform(Centro - FVector2f(Lato * 0.5f, Lato * 0.5f))),
			&Pennello, ESlateDrawEffect::None, Luce.CopyWithNewOpacity(0.018f * (1.f - 0.4f * T) * Respiro) * Tinta);
	}

	// I granelli.
	for (const FGranello& Granello : Granelli)
	{
		const float Entrata = FMath::Clamp(Granello.Eta / 3.f, 0.f, 1.f);
		const float Uscita = FMath::Clamp((Granello.Vita - Granello.Eta) / 3.f, 0.f, 1.f);
		const float Luccichio = 0.75f + 0.25f * FMath::Sin(Granello.Eta * 2.3f + Granello.Fase);
		const float Raggio = NelRaggio(Granello.Posizione);
		const float Alfa = Entrata * Uscita * Luccichio * (0.06f + 0.62f * Raggio) * Respiro;
		if (Alfa <= 0.01f)
		{
			continue;
		}
		const FVector2f Centro = Granello.Posizione * Misura;
		const float Lato = Granello.Dimensione * (1.f + 0.5f * Raggio);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(Lato, Lato), FSlateLayoutTransform(Centro - FVector2f(Lato * 0.5f, Lato * 0.5f))),
			&Pennello, ESlateDrawEffect::None, Luce.CopyWithNewOpacity(Alfa) * Tinta);
		if (Raggio > 0.4f)
		{
			// Nel raggio: un piccolo alone intorno al granello.
			const float Alone = Lato * 4.f;
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(Alone, Alone), FSlateLayoutTransform(Centro - FVector2f(Alone * 0.5f, Alone * 0.5f))),
				&Pennello, ESlateDrawEffect::None, Luce.CopyWithNewOpacity(Alfa * 0.12f) * Tinta);
		}
	}
	return LayerId + 1;
}
