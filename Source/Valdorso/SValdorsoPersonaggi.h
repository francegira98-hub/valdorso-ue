// Valdorso - La scelta del personaggio (v0.1.2, passo 3), nello stile "Oro e brace". Scritto da Claude il 04/10/2026.
//
// Dopo l'accesso: "Il Registro dei coloni" con i propri personaggi (fino a 3). Per ognuno: Entra o Cancella
// (cancellare chiede di riscrivere il nome). Nei posti liberi si crea un personaggio nuovo scrivendo il nome:
// per ora solo il nome, il Registro di Val d'Orso (passo 4) aggiungerà aspetto, fede, età e domande del sacerdote.
// Il server controlla tutto (regole del nome, nomi già presi o riservati, somiglianza con lo staff) e risponde
// con l'elenco aggiornato e il motivo.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ValdorsoArchivista.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/TextureRenderTarget2D.h"

class FValdorsoStileAccesso;
class SVerticalBox;
class SEditableTextBox;

DECLARE_DELEGATE_OneParam(FValdorsoSuPersonaggio, const FString& /*Id o Nome*/);
DECLARE_DELEGATE_TwoParams(FValdorsoSuCancellaPersonaggio, const FString& /*Id*/, const FString& /*Conferma*/);
DECLARE_DELEGATE_OneParam(FValdorsoSuMostraPersonaggio, const FValdorsoPersonaggioBreve& /*Personaggio*/);

class VALDORSO_API SValdorsoSceltaPersonaggio : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoSceltaPersonaggio) : _Ritratto(nullptr) {}
		/** (05/10) Il ritratto del palco: a destra si vede chi è selezionato. */
		SLATE_ARGUMENT(UTextureRenderTarget2D*, Ritratto)
		/** È cambiato chi è selezionato (mouse sopra o fuoco): il palco lo mostra. */
		SLATE_EVENT(FValdorsoSuMostraPersonaggio, OnMostra)
		SLATE_EVENT(FValdorsoSuPersonaggio, OnScegli)
		SLATE_EVENT(FValdorsoSuPersonaggio, OnCrea)
		SLATE_EVENT(FValdorsoSuCancellaPersonaggio, OnCancella)
		SLATE_EVENT(FSimpleDelegate, OnEsci)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Il server ha mandato l'elenco (e un messaggio, se c'è): si ridisegnano le righe. */
	void Aggiorna(const TArray<FValdorsoPersonaggioBreve>& Nuovi, const FText& Testo, bool bComeErrore);

	/** Mentre il server risponde: pulsanti spenti, il messaggio pulsa. */
	void Attendi(const FText& Testo);

	/** Il widget a cui dare il fuoco. */
	TSharedPtr<SWidget> FuocoIniziale() const;

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void Ridisegna();
	TSharedRef<SWidget> RigaPersonaggio(const FValdorsoPersonaggioBreve& Personaggio);
	TSharedRef<SWidget> RigaNuovo();
	TSharedRef<SWidget> RigaLibera();
	void Crea();

	TSharedPtr<FValdorsoStileAccesso> Stile;
	FValdorsoSuPersonaggio OnScegli;
	FValdorsoSuPersonaggio OnCrea;
	FValdorsoSuCancellaPersonaggio OnCancella;
	FSimpleDelegate OnEsci;
	FValdorsoSuMostraPersonaggio OnMostra;

	TStrongObjectPtr<UTextureRenderTarget2D> RitrattoVivo;
	FSlateBrush PennelloRitratto;
	/** Le righe dei personaggi (per sapere quale ha il mouse sopra o il fuoco) e chi si sta mostrando. */
	TArray<TPair<TSharedPtr<SWidget>, FString>> RigheVive;
	FString IdMostrato;
	FString NomeMostrato;

	TArray<FValdorsoPersonaggioBreve> Elenco;
	TSharedPtr<SVerticalBox> Righe;
	TSharedPtr<SEditableTextBox> CampoNome;
	TSharedPtr<SEditableTextBox> CampoConferma;
	TSharedPtr<SWidget> PrimoPulsante;

	/** L'Id del personaggio che si sta cancellando (vuoto = nessuno). */
	FString IdDaCancellare;
	/** Il nome scritto nel campo prima di ridisegnare. */
	FText ScrittoPrima;
	FText Messaggio;
	bool bErrore = false;
	bool bInAttesa = false;
};
