// Valdorso - La mappa di carta (v0.1.3, tasto M). Scritto da Claude il 06/11/2026, preparato mentre Fra dormiva.
//
// La pergamena della valle (T_MappaValle, fatta da Tools/Generatori/mappa.py e importata da crea_mappa.py) su un velo
// scuro. Niente puntino "sei qui": la mappa è di carta, ci si orienta con quello che si vede (deciso nel Design).
// Rotella del mouse: avvicina e allontana; tenendo premuto il tasto sinistro si sposta il foglio.
// M, Esc, E o il tasto B del controller: si chiude.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"

class FValdorsoStileAccesso;
class UTexture2D;

class VALDORSO_API SValdorsoMappa : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoMappa) {}
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Il percorso della texture della mappa. */
	static const TCHAR* PercorsoTexture;

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TSharedPtr<FValdorsoStileAccesso> Stile;
	FSimpleDelegate OnChiudi;
	TStrongObjectPtr<UTexture2D> Texture;
	FSlateBrush Foglio;

	/** Quanto è vicina (1 = tutta la mappa nello schermo) e di quanto è spostata (pixel dello schermo). */
	float Zoom = 1.f;
	FVector2D Spostamento = FVector2D::ZeroVector;
	bool bTrascina = false;

	TOptional<FSlateRenderTransform> Trasformazione() const;
};
