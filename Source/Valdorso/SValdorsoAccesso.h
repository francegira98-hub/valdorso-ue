// Valdorso - Le schermate dell'accesso, nello stile "Oro e brace" (Slate, tutto in C++).
// Scritto da Claude il 03/10/2026 (v0.1.2, passo 2.2).
//
//   SValdorsoPrimaDiEntrare  nel menu: schede "Entra" (nome e password) e "Primo ingresso" (codice d'invito,
//                            nome, password due volte). Controlla subito le stesse regole del server, avvisa
//                            se il Bloc Maiusc è acceso, ricorda il nome se si vuole, mostra il motivo
//                            quando si torna dal server (password sbagliata, espulsione, server spento).
//   SValdorsoAnticamera      sul server, prima di avere il personaggio: "Il Cuore ti sta riconoscendo..."
//                            e, dopo un reset dell'Amministratrice, la scelta della password nuova.
//   SValdorsoCodiciRecupero  (04/10) gli 8 codici di recupero appena nati, da scrivere: si vedono una volta sola.
//   In "Prima di entrare", sotto Entra, "Ho perso la password": nome, codice di recupero e password nuova.
//
// Tastiera: Invio conferma, Esc torna indietro, Tab passa al campo dopo. Controller: frecce e croce.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "ValdorsoGameInstance.h"

class SButton;
class SEditableTextBox;
class FValdorsoCaratteri;

DECLARE_DELEGATE_OneParam(FValdorsoSuRichiestaAccesso, const FValdorsoRichiestaAccesso&);
DECLARE_DELEGATE_TwoParams(FValdorsoSuCambioPassword, const FString& /*Attuale*/, const FString& /*Nuova*/);

/** Pennelli, stili e pezzi comuni alle schermate dell'accesso. Vive finché vive la schermata che lo usa. */
class VALDORSO_API FValdorsoStileAccesso
{
public:
	FValdorsoStileAccesso();

	TSharedRef<SButton> Pulsante(TAttribute<FText> Testo, FSimpleDelegate Azione, float Dimensione = 24.f, TAttribute<bool> Abilitato = true);
	TSharedRef<SEditableTextBox> Campo(const FText& Suggerimento, TAttribute<bool> Nascosto, FOnTextCommitted SuInvio);
	/** (05/10) Il colore si può cambiare: sulla pergamena del Registro etichette e separatori sono d'inchiostro. */
	TSharedRef<SWidget> Etichetta(const FText& Testo, TOptional<FLinearColor> Colore = TOptional<FLinearColor>());
	TSharedRef<SWidget> Separatore(float Larghezza, TOptional<FLinearColor> Colore = TOptional<FLinearColor>());
	TSharedRef<SWidget> Diamante(float Lato, TAttribute<FSlateColor> Colore);

	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FButtonStyle StilePulsante;
	FEditableTextBoxStyle StileCampo;
	FSlateBrush Pieno;
	FSlateBrush SfondoPannello;
	FSlateBrush SfondoCampo;
	FSlateBrush SfondoCampoAttivo;
	FSlateBrush Velo;
};

/** "Prima di entrare": il pannello dell'accesso nel menu. */
class VALDORSO_API SValdorsoPrimaDiEntrare : public SCompoundWidget
{
	/** (05/10) Il controllo automatico delle schermate (Test/ValdorsoTestSchermate.cpp) può aprire pannelli e pagine. */
	friend struct FValdorsoProvaSchermate;

public:
	SLATE_BEGIN_ARGS(SValdorsoPrimaDiEntrare) : _bPrimoIngresso(false) {}
		SLATE_ARGUMENT(FString, NomeIniziale)
		SLATE_ARGUMENT(FText, Messaggio)
		SLATE_ARGUMENT(bool, bPrimoIngresso)
		SLATE_EVENT(FValdorsoSuRichiestaAccesso, OnRichiesta)
		SLATE_EVENT(FSimpleDelegate, OnIndietro)
		SLATE_EVENT(FSimpleDelegate, OnProvaLocale)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Un messaggio sotto i campi: rosso brace se è un errore, pergamena se è un avviso. */
	void MostraMessaggio(const FText& Testo, bool bErrore = true);

	/** Mentre si parte verso il server: campi e pulsanti spenti, il messaggio pulsa. */
	void Attendi(const FText& Testo);

	/** Il primo campo da riempire, per dargli il fuoco. */
	TSharedPtr<SWidget> CampoIniziale() const;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	void Invia();
	void ScegliScheda(bool bPrimo);
	void ScegliRecupero(bool bSi);
	bool Controlla(FValdorsoRichiestaAccesso& Out, FText& OutErrore) const;
	void SuInvio(const FText& Testo, ETextCommit::Type Tipo);
	TSharedRef<SWidget> Scheda(const FText& Testo, bool bPrimo);
	TSharedRef<SWidget> Riga(const FText& Titolo, const TSharedRef<SWidget>& Contenuto);

	TSharedPtr<FValdorsoStileAccesso> Stile;
	FValdorsoSuRichiestaAccesso OnRichiesta;
	FSimpleDelegate OnIndietro;
	FSimpleDelegate OnProvaLocale;

	TSharedPtr<SEditableTextBox> NomeEntra;
	TSharedPtr<SEditableTextBox> PasswordEntra;
	TSharedPtr<SEditableTextBox> Codice;
	TSharedPtr<SEditableTextBox> NomeNuovo;
	TSharedPtr<SEditableTextBox> PasswordNuova;
	TSharedPtr<SEditableTextBox> PasswordRipetuta;
	TSharedPtr<SEditableTextBox> NomeRecupero;
	TSharedPtr<SEditableTextBox> CodiceRecupero;
	TSharedPtr<SEditableTextBox> PasswordRecupero;
	TSharedPtr<SEditableTextBox> PasswordRecuperoRipetuta;

	bool bPrimoIngresso = false;
	bool bRecupero = false;
	bool bMostraPassword = false;
	bool bRicordaNome = true;
	bool bInAttesa = false;
	bool bErrore = true;
	FText Messaggio;
};

/** L'anticamera sul server: attesa e, se serve, la password nuova. */
class VALDORSO_API SValdorsoAnticamera : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoAnticamera) {}
		SLATE_EVENT(FValdorsoSuCambioPassword, OnCambia)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void MostraAttesa(const FText& Testo);
	void MostraCambioPassword(const FText& Testo, bool bErrore);

	TSharedPtr<SWidget> CampoIniziale() const;

private:
	void Invia();
	void SuInvio(const FText& Testo, ETextCommit::Type Tipo);

	TSharedPtr<FValdorsoStileAccesso> Stile;
	FValdorsoSuCambioPassword OnCambia;

	TSharedPtr<SEditableTextBox> Attuale;
	TSharedPtr<SEditableTextBox> Nuova;
	TSharedPtr<SEditableTextBox> Ripetuta;

	bool bCambio = false;
	bool bInAttesa = true;
	bool bErrore = false;
	FText Messaggio;
};

/** I codici di recupero appena nati: da scrivere su carta. Si chiude solo dopo aver detto di averli scritti. */
class VALDORSO_API SValdorsoCodiciRecupero : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoCodiciRecupero) {}
		SLATE_ARGUMENT(TArray<FString>, Codici)
		SLATE_EVENT(FSimpleDelegate, OnFatto)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Il primo pulsante, per dargli il fuoco. */
	TSharedPtr<SWidget> PulsanteIniziale() const { return Conferma; }

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TSharedPtr<FValdorsoStileAccesso> Stile;
	TArray<FString> Codici;
	FSimpleDelegate OnFatto;
	TSharedPtr<SWidget> Conferma;
	bool bScritti = false;
	bool bCopiati = false;
};
