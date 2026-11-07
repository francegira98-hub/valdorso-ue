// Valdorso - La schermata d'avvio (07/11/2026, v0.1.3, scritta da Claude mentre Fra dormiva).
//
// È la prima cosa che si vede aprendo il gioco, prima del menu. Tre momenti, ognuno si salta con un tasto
// (tastiera, mouse o controller):
//   1. Il logo: sul nero un rombo di brace che batte con il Cuore (due colpi e una pausa), poi VALDORSO in oro
//      e "Anno 312 della Corona". Si sente un battito. Dopo 4 secondi va avanti da solo.
//   2. Prima di partire: due righe oneste (la versione in costruzione) e l'avviso per la salute (luci che
//      lampeggiano). Dopo 10 secondi va avanti da solo.
//   3. Il titolo sopra la scena del menu (il nero si apre): VALDORSO grande al centro e "Premi un tasto per
//      entrare", che pulsa. Qui aspetta il giocatore; poi sfuma e compare il menu.
// Con "Meno movimenti" (Impostazioni, Accessibilità) niente pulsazioni: le scritte restano ferme.
// Si vede una volta per avvio del gioco (lo ricorda l'istanza del gioco); il comando Valdorso.Avvio 0 la toglie.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"

#include "Components/AudioComponent.h"

class FValdorsoCaratteri;

class VALDORSO_API SValdorsoAvvio : public SCompoundWidget
{
	/** Il controllo automatico delle schermate (Test/ValdorsoTestSchermate.cpp) passa da un momento all'altro. */
	friend struct FValdorsoProvaSchermate;

public:
	enum class EMomento : uint8
	{
		Logo,
		Avviso,
		Titolo,
		Uscita
	};

	SLATE_BEGIN_ARGS(SValdorsoAvvio) {}
		/** Alla fine (dopo la sfumatura del titolo): il controllore toglie l'avvio e mostra il menu. */
		SLATE_EVENT(FSimpleDelegate, OnFine)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SValdorsoAvvio() override;

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	EMomento Momento() const { return Adesso; }

	/** Quanto dura il logo e quanto aspetta l'avviso prima di andare avanti da solo (secondi). */
	static constexpr float DurataLogo = 4.2f;
	static constexpr float DurataAvviso = 10.f;
	/** Prima di questi secondi un tasto non salta (si eviterebbe di saltare tutto con un solo colpo). */
	static constexpr float AttesaMinima = 0.6f;
	static constexpr float DurataUscita = 0.7f;

private:
	void VaiA(EMomento Nuovo);
	void Avanza();
	EActiveTimerReturnType Anima(double Tempo, float Delta);

	/** Secondi dall'inizio del momento di adesso. */
	float Trascorso() const;
	/** Quanto si vede ogni parte (0..1): il nero, il logo, l'avviso, il titolo. */
	float OpacitaNero() const;
	float OpacitaLogo() const;
	float OpacitaAvviso() const;
	float OpacitaTitolo() const;
	/** Il battito del Cuore (0..1), fermo a metà con "Meno movimenti". */
	float Pulsa() const;
	TSharedRef<SWidget> CreaSeparatore(float Larghezza);

	FSimpleDelegate OnFine;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FSlateBrush Pieno;
	FSlateBrush Vuoto;
	EMomento Adesso = EMomento::Logo;
	double InizioMomento = 0.0;
	bool bMenoMovimenti = false;
	bool bFinito = false;
	TStrongObjectPtr<UAudioComponent> Battito;
};
