// Valdorso - Il menu principale "Oro e brace", scritto in C++ (Slate): niente da costruire a mano nell'editor.
// Titolo VALDORSO che pulsa con il battito del Cuore, il motto, i pulsanti, le braci che salgono,
// una voce della locanda diversa a ogni avvio e la versione. Si usa anche con il controller (frecce e croce).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"

class SButton;
class FValdorsoCaratteri;

/** Le braci che salgono lente sullo schermo, disegnate da Slate. */
class VALDORSO_API SValdorsoBraci : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoBraci) : _Numero(70) {}
		SLATE_ARGUMENT(int32, Numero)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }

private:
	struct FBrace
	{
		FVector2f Posizione = FVector2f::ZeroVector;   // da 0 a 1 sullo schermo
		float Salita = 0.05f;                           // schermi al secondo
		float Ondeggio = 0.01f;
		float Fase = 0.f;
		float Dimensione = 3.f;                         // pixel
		float Eta = 0.f;
		float Vita = 8.f;
		FLinearColor Colore = FLinearColor::White;
	};

	void Rinasci(FBrace& Brace, bool bAllInizio);
	EActiveTimerReturnType Aggiorna(double Tempo, float Delta);

	TArray<FBrace> Braci;
	FSlateBrush Pennello;
	FRandomStream Caso;
};

/** Una sfumatura scura a sinistra e in basso, per leggere bene i testi sopra la scena. */
class VALDORSO_API SValdorsoSfumatura : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoSfumatura) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs) {}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }
};

/** Il menu principale. */
class VALDORSO_API SValdorsoMenuPrincipale : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoMenuPrincipale) {}
		SLATE_EVENT(FSimpleDelegate, OnEntra)
		SLATE_EVENT(FSimpleDelegate, OnEsci)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Il primo pulsante ("Entra nella valle"), per dargli il fuoco del controller. */
	TSharedPtr<SWidget> GetPrimoPulsante() const { return PrimoPulsante; }

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	TSharedRef<SButton> CreaPulsante(const FText& Testo, FSimpleDelegate Azione, float Dimensione = 28.f);
	TSharedRef<SWidget> CreaSeparatore(float Larghezza);
	TSharedRef<SWidget> CreaDiamante(float Lato, const FLinearColor& Colore);

	void MostraPannello(const FText& Titolo, const FText& Testo);
	void ChiudiPannello();
	void MostraImpostazioni();
	void MostraRiconoscimenti();
	bool PannelloAperto() const { return VisibilitaPannello == EVisibility::Visible; }

	EActiveTimerReturnType Anima(double Tempo, float Delta);

	FSimpleDelegate OnEntra;
	FSimpleDelegate OnEsci;

	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FButtonStyle StilePulsante;
	FSlateBrush Pieno;
	FSlateBrush SfondoPannello;

	TSharedPtr<SWidget> PrimoPulsante;
	TSharedPtr<SWidget> PulsanteChiudi;

	EVisibility VisibilitaPannello = EVisibility::Collapsed;
	FText TitoloPannello;
	FText TestoPannello;

	double Inizio = 0.0;
};
