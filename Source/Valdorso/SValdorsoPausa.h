// Valdorso - La pausa (v0.1.2, passo 6, prima parte), nello stile "Oro e brace". Scritto da Claude il 02/11/2026.
//
// Esc (nell'editor F10, perché Esc ferma il Play; sul controller Start) apre "Una sosta": Riprendi, Impostazioni,
// Torna ai personaggi, Torna al menu, Esci dal gioco. Valdorso è un MMO: il mondo non si ferma, la pausa è solo
// per chi la apre. Per uscire il colono resta nella valle 20 secondi (lo decide il server): se in quel tempo viene
// colpito o spostato, l'uscita si annulla. Così nessuno sparisce a metà di uno scontro.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FValdorsoStileAccesso;

/** Dove si va uscendo dalla valle (viaggia in rete come numero). */
enum class EValdorsoUscita : uint8
{
	Personaggi = 0,
	Menu = 1,
	Gioco = 2,
};

DECLARE_DELEGATE_OneParam(FValdorsoSuUscita, EValdorsoUscita /*Dove*/);

class VALDORSO_API SValdorsoPausa : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoPausa) : _bConPersonaggi(true) {}
		/** Falso per chi gioca senza account (in locale o nell'editor): niente "Torna ai personaggi". */
		SLATE_ARGUMENT(bool, bConPersonaggi)
		SLATE_EVENT(FSimpleDelegate, OnRiprendi)
		SLATE_EVENT(FSimpleDelegate, OnImpostazioni)
		SLATE_EVENT(FValdorsoSuUscita, OnEsci)
		SLATE_EVENT(FSimpleDelegate, OnAnnulla)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Il server ha accettato l'uscita: si conta alla rovescia (i secondi li decide il server). */
	void MostraConto(EValdorsoUscita Dove, int32 Secondi);

	/** L'uscita è annullata (colpito, spostato o annullata dal giocatore): si torna ai pulsanti, con il motivo. */
	void Annullata(const FText& Motivo);

	/** Vero mentre si conta alla rovescia (o si aspetta la risposta del server). */
	bool InUscita() const { return bInUscita; }

	/** Il widget a cui dare il fuoco. */
	TSharedPtr<SWidget> FuocoIniziale() const { return PrimoPulsante; }

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	void Esci(EValdorsoUscita Dove);
	FText TestoConto() const;

	TSharedPtr<FValdorsoStileAccesso> Stile;
	FSimpleDelegate OnRiprendi;
	FSimpleDelegate OnImpostazioni;
	FValdorsoSuUscita OnEsci;
	FSimpleDelegate OnAnnulla;
	TSharedPtr<SWidget> PrimoPulsante;

	bool bInUscita = false;
	/** Vero quando il server ha risposto e si conta (prima si aspetta). */
	bool bConta = false;
	EValdorsoUscita DoveVa = EValdorsoUscita::Menu;
	double FineConto = 0.0;
	FText Messaggio;
	bool bErrore = false;
};
