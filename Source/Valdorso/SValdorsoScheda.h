// Valdorso - La scheda del personaggio: le abilita' (scheletro della v0.1.5, 10/11/2026). Scritto da Claude. Vedi
// ValdorsoCrescita.h e ValdorsoAbilitaDati.h.
//
// Tasto K. Le abilita' divise per famiglia (il corpo, le armi, il mondo, i mestieri, lo spirito): il nome (quello
// dell'evoluzione), la barra da 0 a 100 con le soglie, i punti, e la freccia (Sale, Ferma, Cala: un clic la cambia). Ferma
// a una soglia si vede quanto e' carica. In alto il totale sul tetto di 700. Passando sopra a una riga: cosa fa e cosa si
// sblocca alla prossima soglia. Quelle che arrivano piu' avanti (i mestieri, la magia) si vedono in grigio.
// Si chiude con K o Esc.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "ValdorsoAbilitaDati.h"

class APlayerController;
class FValdorsoCaratteri;
class SBox;
class UValdorsoCrescitaComponent;

class VALDORSO_API SValdorsoScheda : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoScheda) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Rifa' il contenuto (le abilita' sono cambiate). */
	void Rifai();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedRef<SWidget> Riga(const FValdorsoAbilitaTipo& Tipo, const FValdorsoAbilitaValore* Valore);
	TSharedRef<SWidget> Barra(float Punti, float Carica, bool bInArrivo);
	FText TestoTotale() const;

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoCrescitaComponent> Crescita;
	FSimpleDelegate OnChiudi;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	TSharedPtr<SBox> Contenuto;
	FSlateBrush Carta;
	FSlateBrush Velo;
	FSlateBrush Sfondo;
	FSlateBrush Pieno;
	FSlateBrush PienoGrigio;
	FSlateBrush Caricato;
	FSlateBrush Tacca;
	int32 Visti = -1;
};

namespace ValdorsoSchedaSchermo
{
	/** Vero se su questo PC la scheda e' aperta. */
	VALDORSO_API bool Aperta(const APlayerController* Controllore);

	/** La apre (se non c'e' altro aperto) o la chiude. */
	VALDORSO_API void Alterna(APlayerController* Controllore);

	/** La chiude e torna al gioco. */
	VALDORSO_API void Chiudi(APlayerController* Controllore);
}
