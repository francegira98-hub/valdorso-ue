// Valdorso - La finestra delle storie del Battito (v0.2.4 "PNG veri", blocco 3, 14/11/2026). Scritto da Claude.
// Com'e' nel Design (deciso il 13/11, anteprima Tools/Sognatore/anteprima_finestra_storia.png): al centro dello schermo,
// sulla pergamena come la tessera. In cima il nastro della rarita' con il suo colore e le stelle della difficolta'; poi
// "Il Battito ha intrecciato una storia", il titolo, il filo e il capitolo, la trama, il primo passo, i premi (dalle Rare
// in parte "???"), "Se rifiuti: ...", la scadenza; in fondo Accetta / Ci penso / Rifiuta (tasti 1, 2, 3; Esc = Ci
// penso). Il suono cambia con la rarita' (la campana, piu' forte e due volte per le Epiche e le Leggendarie).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "ValdorsoBattito.h"

class APlayerController;
class FValdorsoStileAccesso;

/** La risposta: l'id della storia e 0 Accetta, 1 Rifiuta, 2 Ci penso. */
DECLARE_DELEGATE_TwoParams(FValdorsoSuRispostaStoria, const FString&, uint8);

class VALDORSO_API SValdorsoFinestraStoria : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoFinestraStoria) {}
		SLATE_ARGUMENT(FValdorsoFinestraStoria, Finestra)
		SLATE_EVENT(FValdorsoSuRispostaStoria, OnRisposta)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TSharedPtr<SWidget> FuocoIniziale() const { return PrimoPulsante; }

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void Rispondi(uint8 Cosa);

	FValdorsoFinestraStoria Dati;
	FValdorsoSuRispostaStoria OnRisposta;
	TSharedPtr<FValdorsoStileAccesso> Stile;
	FSlateBrush Carta;
	FSlateBrush Nastro;
	TSharedPtr<SWidget> PrimoPulsante;
	bool bRisposto = false;
};

namespace ValdorsoFinestraStoria
{
	/** Vero se su questo PC la finestra di una storia e' aperta. */
	VALDORSO_API bool Aperta(const APlayerController* Controllore);

	/** La apre (con il suono della rarita'); alla risposta chiama Risposta e si chiude. */
	VALDORSO_API bool Apri(APlayerController* Controllore, const FValdorsoFinestraStoria& Finestra,
		TFunction<void(const FString&, uint8)> Risposta);

	/** La chiude e rimette il gioco (se non c'e' un'altra schermata). */
	VALDORSO_API void Chiudi(APlayerController* Controllore);
}
