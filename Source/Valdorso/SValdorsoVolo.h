// Valdorso - Le scritte sopra il volo verso la valle (v0.1.2, passo 2.5). Scritto da Claude il 04/10/2026.
//
// Al centro "La valle ti accoglie" (solo al primo ingresso), che batte con il Cuore; sotto, sul nero,
// "In cammino verso la valle..." mentre il gioco si collega al server; in basso a destra "Esc  salta"
// quando il volo si può saltare. Il controllore del menu decide quanto si vede ogni scritta.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FValdorsoCaratteri;

class VALDORSO_API SValdorsoVolo : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoVolo) {}
		SLATE_EVENT(FSimpleDelegate, OnSalta)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Quanto si vede "La valle ti accoglie" (da 0 a 1). */
	void ImpostaTitolo(float Opacita) { OpacitaTitolo = FMath::Clamp(Opacita, 0.f, 1.f); }

	/** La riga piccola sotto (vuota = nascosta). */
	void ImpostaRiga(const FText& Testo, float Opacita) { Riga = Testo; OpacitaRiga = FMath::Clamp(Opacita, 0.f, 1.f); }

	/** Se vero, Esc (o B / Start sul controller) salta il volo e si mostra l'aiuto in basso. */
	void ImpostaSaltabile(bool bSi) { bSaltabile = bSi; }

	/** Il testo dell'aiuto in basso a destra (di serie "ESC  SALTA"). */
	void ImpostaAiuto(const FText& Testo) { Aiuto = Testo; }

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FSimpleDelegate OnSalta;

	float OpacitaTitolo = 0.f;
	float OpacitaRiga = 0.f;
	FText Riga;
	FText Aiuto;
	bool bSaltabile = false;
	double InizioAiuto = -1.0;
};
