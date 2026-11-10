// Valdorso - Le barre, la barra delle azioni e il velo della caduta: quello che si vede (v0.1.4). Scritto da Claude il
// 08/11/2026.
//
// SValdorsoBarre        quattro barre sottili una sotto l'altra, in basso al centro: salute (brace), stamina (oro),
//                       mana (blu arcano), equilibrio (pergamena), ognuna in una cornice scura con un filo d'oro scuro.
//                       Quando una barra scende resta per un attimo una scia color brace. Ogni barra compare quando
//                       cambia e sparisce sfumando 3 secondi dopo essere tornata piena (ValdorsoBarre::Aggiorna).
//                       Valdorso.Barre.Numeri 1: i numeri accanto alle barre.
// SValdorsoBarraAzioni  sopra le barre, 8 posti con il numero del tasto e una cornice a rombo (la runa vuota); vuoti
//                       sono tenui. Premendo il tasto il posto si illumina; se è vuoto sopra compare "Nessuna azione".
// SValdorsoVeloCaduta   a tutto schermo, quando il proprio colono cade: lo schermo si scurisce e al centro
//                       "Sei caduto. Il Cuore ti riporta al tempio…"; alla rinascita il velo si alza.
// Li mette e li toglie UValdorsoBarreComponent (ValdorsoBarre.h).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateBrush.h"
#include "ValdorsoBarre.h"

class APlayerController;
class UValdorsoBarreComponent;
class FValdorsoCaratteri;

class VALDORSO_API SValdorsoBarre : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoBarre) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Le misure: le barre sono larghe quanto la barra delle azioni; ai lati lo spazio per i numeri. */
	static constexpr float Larghezza = 481.f;
	static constexpr float Margine = 46.f;
	static constexpr float AltezzaBarra = 6.f;
	static constexpr float Passo = 13.f;
	static constexpr int32 Quante = 4;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	EActiveTimerReturnType Anima(double Tempo, float Delta);

	TWeakObjectPtr<APlayerController> Controllore;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FSlateBrush Cornice;
	FSlateBrush Riempimento;
	FValdorsoStatoBarra Stati[Quante];
	float Valori[Quante] = { 0.f, 0.f, 0.f, 0.f };
	float Massimi[Quante] = { 0.f, 0.f, 0.f, 0.f };
	/** Quanto si vede tutto (0 con le schermate aperte o con il colono caduto). */
	float PresenzaSchermo = 0.f;
	/** L'orologio dell'interfaccia (secondi, dall'active timer). */
	double Orologio = 0.0;
	/** (08/11) Quanto è bagnato il colono (ValdorsoBagnato.h) e quanto si vede la goccia a sinistra delle barre. */
	float Bagnato = 0.f;
	float PresenzaGoccia = 0.f;
	/** (10/11) Il fiato sott'acqua (0..1) e quanto si vedono le bolle sopra le barre (solo quando manca un poco). */
	float Fiato = 1.f;
	float PresenzaFiato = 0.f;
};

class VALDORSO_API SValdorsoBarraAzioni : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoBarraAzioni) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(TWeakObjectPtr<UValdorsoBarreComponent>, Barre)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** (08/11, dopo la prima prova di Fra) Più grandi: 54 pixel (erano 40). */
	static constexpr float Lato = 54.f;
	static constexpr float Spazio = 7.f;
	/** Sopra i posti, lo spazio per "Nessuna azione". */
	static constexpr float AltezzaScritta = 22.f;

	/** Il tasto di un posto è stato premuto (bVuoto: sopra compare "Nessuna azione"). */
	void Premi(int32 Indice, bool bVuoto);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	EActiveTimerReturnType Anima(double Tempo, float Delta);

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoBarreComponent> Barre;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FSlateBrush Casella;
	FSlateBrush Pieno;
	/** Quanto è acceso ogni posto (1 appena premuto, poi si spegne). */
	float Lampi[ValdorsoBarre::NumeroAzioni] = {};
	/** Il posto sopra cui si legge "Nessuna azione" e quanto si vede ancora. */
	int32 PostoScritta = INDEX_NONE;
	float Scritta = 0.f;
	float PresenzaSchermo = 0.f;
};

class VALDORSO_API SValdorsoVeloCaduta : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoVeloCaduta) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	EActiveTimerReturnType Anima(double Tempo, float Delta);

	TWeakObjectPtr<APlayerController> Controllore;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FSlateBrush Pieno;
	/** 0 = niente, 1 = schermo scuro con la scritta. */
	float Buio = 0.f;
};
