// Valdorso - Bussola e taccuino di viaggio: quello che si vede (v0.1.3). Scritto da Claude il 07/11/2026.
//
// SValdorsoBussola   la striscia in alto al centro: N, NE, E, SE, S, SO, O, NO che scorrono con lo sguardo, una
//                    tacca ogni 15 gradi e, sotto, i segni dei luoghi già scoperti; il luogo davanti a sé ha il nome e
//                    la distanza. Compare e sparisce sfumando (nascosta con le schermate aperte e nella modalità foto).
// SValdorsoTaccuino  la pergamena del taccuino (tasto J): le schede Luoghi, Iscrizioni e Rune.
// Li apre e li chiude UValdorsoTaccuinoComponent (ValdorsoTaccuino.h).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class APlayerController;
class UValdorsoTaccuinoComponent;
class FValdorsoCaratteri;
class FValdorsoStileAccesso;
class SBox;

class VALDORSO_API SValdorsoBussola : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoBussola) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(TWeakObjectPtr<UValdorsoTaccuinoComponent>, Taccuino)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Quanto è larga la striscia (pixel) e quanti gradi mostra. */
	static constexpr float Larghezza = 420.f;
	static constexpr float Campo = 180.f;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	EActiveTimerReturnType Anima(double Tempo, float Delta);
	/** Vero quando la bussola deve vedersi (il colono c'è e non c'è niente di aperto sopra). */
	bool DaMostrare() const;
	/** (09/11) Ai lati della striscia: l'ora canonica e l'ora (a sinistra), il termometro (a destra). */
	void DisegnaLati(const FGeometry& Geometria, FSlateWindowElementList& Elementi, int32 Strato, float Alfa) const;
	/** Il cielo del livello (per l'ora), cercato una volta. */
	mutable TWeakObjectPtr<class AValdorsoCielo> Cielo;
	mutable bool bCieloCercato = false;

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoTaccuinoComponent> Taccuino;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FSlateBrush Pieno;
	FSlateBrush Fascia;
	/** 0 = nascosta, 1 = tutta: sfuma in un quarto di secondo. */
	float Presenza = 0.f;
};

class VALDORSO_API SValdorsoTaccuino : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoTaccuino) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(TWeakObjectPtr<UValdorsoTaccuinoComponent>, Taccuino)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	/** Le schede: 0 Luoghi, 1 Iscrizioni, 2 Rune. */
	void MostraScheda(int32 Nuova);
	/** Rifà la pagina della scheda aperta (quando si cambia scheda o arriva qualcosa dal server). */
	void Rifai();
	TSharedRef<SWidget> PaginaLuoghi();
	TSharedRef<SWidget> PaginaIscrizioni();
	TSharedRef<SWidget> PaginaRune();
	TSharedRef<SWidget> Linguetta(int32 Indice, const FText& Testo);
	/** Su o giù nell'elenco delle iscrizioni (frecce e croce del controller). */
	void ScorriIscrizioni(int32 Verso);
	EActiveTimerReturnType Controlla(double Tempo, float Delta);

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoTaccuinoComponent> Taccuino;
	FSimpleDelegate OnChiudi;
	TSharedPtr<FValdorsoStileAccesso> Stile;
	TSharedPtr<SBox> Contenuto;
	FSlateBrush Carta;
	FSlateBrush Trasparente;
	FButtonStyle StileLinguetta;
	int32 Scheda = 0;
	/** L'iscrizione aperta (indice nel taccuino); INDEX_NONE = l'ultima letta. */
	int32 IscrizioneScelta = INDEX_NONE;
	int32 CambiamentiVisti = -1;
	/** Il testo dell'iscrizione aperta non è ancora arrivato dal server. */
	bool bAttendeTesto = false;
};
