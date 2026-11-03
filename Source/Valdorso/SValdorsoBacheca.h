// Valdorso - La bacheca letta (v0.1.3): gli avvisi su una pergamena. Scritto da Claude il 03/11/2026.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class FValdorsoStileAccesso;

class VALDORSO_API SValdorsoBacheca : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoBacheca) {}
		SLATE_ARGUMENT(FString, Titolo)
		SLATE_ARGUMENT(TArray<FString>, Avvisi)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TSharedPtr<SWidget> FuocoIniziale() const { return PulsanteChiudi; }

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TSharedPtr<FValdorsoStileAccesso> Stile;
	FSimpleDelegate OnChiudi;
	TSharedPtr<SWidget> PulsanteChiudi;
	FSlateBrush Carta;
};
