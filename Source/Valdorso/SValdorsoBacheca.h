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

	/**
	 * (07/11) Le righe scritte sulla pergamena, una sotto l'altra con il separatore in mezzo: le righe "rune:" con le
	 * rune disegnate, quelle che cominciano con la minuscola unite alla frase di prima. Le usa anche il taccuino di
	 * viaggio (le iscrizioni rilette e il quaderno delle rune). StileRighe deve vivere quanto i widget che ne escono.
	 * (Non si chiama Stile: nasconderebbe il membro Stile, e con gli avvisi C4458 trattati come errori non compila.)
	 */
	static TSharedRef<SWidget> ScriviRighe(const TArray<FString>& Avvisi, FValdorsoStileAccesso& StileRighe);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TSharedPtr<FValdorsoStileAccesso> Stile;
	FSimpleDelegate OnChiudi;
	TSharedPtr<SWidget> PulsanteChiudi;
	FSlateBrush Carta;
};
