// Valdorso - La polvere nella luce (passo 4.2b, seconda parte). Scritto da Claude il 06/10/2026.
//
// Granelli di polvere che fluttuano piano davanti al palco del Registro: quasi invisibili nell'ombra, si accendono
// quando attraversano il raggio di luce che scende dall'alto a destra sul ritratto (come in una chiesa di pietra).
// Il raggio stesso è un chiarore leggerissimo. Tutto respira appena con il battito del Cuore.
// Disegnato da Slate (niente Niagara): non costa quasi niente e si vede uguale in ogni schermata.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateBrush.h"
#include "Math/RandomStream.h"

class VALDORSO_API SValdorsoPolvere : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoPolvere) : _Numero(110) {}
		SLATE_ARGUMENT(int32, Numero)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }

private:
	struct FGranello
	{
		FVector2f Posizione = FVector2f::ZeroVector;   // da 0 a 1 sullo schermo
		FVector2f Deriva = FVector2f::ZeroVector;      // schermi al secondo
		float Fase = 0.f;
		float Dimensione = 1.5f;                       // pixel
		float Eta = 0.f;
		float Vita = 20.f;
	};

	void Rinasci(FGranello& Granello, bool bAllInizio);
	EActiveTimerReturnType Aggiorna(double Tempo, float Delta);
	/** Quanto un punto (da 0 a 1) sta dentro il raggio di luce: 0 nell'ombra, 1 al centro del raggio. */
	static float NelRaggio(const FVector2f& Punto);

	TArray<FGranello> Granelli;
	FSlateBrush Pennello;
	FRandomStream Caso;
};
