// Valdorso - La schermata delle impostazioni (v0.1.2, passo 6, seconda parte). Scritto da Claude il 02/11/2026.
//
// Si apre dalla pausa. Ogni voce si cambia con le frecce (anche da tastiera e controller) e vale subito: non serve
// un "Salva". I dati e il modo in cui si applicano stanno in ValdorsoImpostazioni.h.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ValdorsoImpostazioni.h"

class FValdorsoStileAccesso;
class SVerticalBox;

class VALDORSO_API SValdorsoImpostazioni : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoImpostazioni) {}
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TSharedPtr<SWidget> FuocoIniziale() const { return PrimoPulsante; }

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	/** Una voce: il nome a sinistra, il valore tra due frecce a destra. Passo riceve -1 o +1. */
	void AggiungiVoce(const TSharedRef<SVerticalBox>& Dove, const FText& Nome, TFunction<FText()> Valore, TFunction<void(int32)> Passo);
	void AggiungiTitolo(const TSharedRef<SVerticalBox>& Dove, const FText& Titolo);
	void Applica();

	TSharedPtr<FValdorsoStileAccesso> Stile;
	FSimpleDelegate OnChiudi;
	TSharedPtr<SWidget> PrimoPulsante;
	FValdorsoImpostazioni Bozza;
};
