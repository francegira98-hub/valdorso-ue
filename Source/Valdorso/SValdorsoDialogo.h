// Valdorso - Il dialogo con un PNG sulla pergamena (v0.1.3). Scritto da Claude il 07/11/2026.
//
// In cima il nome di chi parla, sotto le sue battute (i gesti, tra parentesi nei testi, in corsivo; le righe "rune:"
// con le rune disegnate), poi le risposte numerate. Si risponde con il mouse, con i tasti 1-4 o con il controller
// (frecce e croce). Esc (o il cerchio sul controller) chiude il dialogo. All'ultima battuta c'è solo "Congedati".
// Le battute le decide il server (ValdorsoDialoghi.h, AValdorsoPNG): la schermata le mostra e basta.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "ValdorsoDialoghi.h"

class FValdorsoStileAccesso;
class SBox;

/** La risposta scelta (il numero, da 0) o INDEX_NONE: il colono se ne va. */
DECLARE_DELEGATE_OneParam(FValdorsoSuSceltaDialogo, int32);

class VALDORSO_API SValdorsoDialogo : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoDialogo) {}
		SLATE_ARGUMENT(FValdorsoBattuta, Battuta)
		SLATE_EVENT(FValdorsoSuSceltaDialogo, OnScelta)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Una battuta nuova (dallo stesso PNG): la pergamena si riscrive. */
	void Mostra(const FValdorsoBattuta& Battuta);

	/** La prima risposta (o "Congedati"), per dargli il fuoco. */
	TSharedPtr<SWidget> FuocoIniziale() const { return PrimoPulsante; }

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	/** Il colono sceglie (o se ne va con INDEX_NONE). Dopo una risposta si aspetta la battuta dopo dal server. */
	void Scegli(int32 Indice);

	TSharedRef<SWidget> PulsanteRisposta(int32 Indice, const FString& Testo);

	TSharedPtr<FValdorsoStileAccesso> Stile;
	FValdorsoSuSceltaDialogo OnScelta;
	FSlateBrush Carta;
	FSlateBrush VeloLeggero;
	TSharedPtr<SBox> Contenuto;
	TSharedPtr<SWidget> PrimoPulsante;
	int32 NumeroScelte = 0;
	bool bFine = false;
	/** Vero dopo una risposta, finché il server non manda la battuta dopo: i pulsanti si spengono. */
	bool bInAttesa = false;
};
