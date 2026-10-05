// Valdorso - Il Registro di Val d'Orso a schermo (v0.1.2, passo 4.2a), nello stile "Oro e brace".
// Scritto da Claude il 05/10/2026.
//
// Il sacerdote scrive il nuovo colono nel registro: una pergamena a sinistra, una pagina per scelta, ognuna aperta
// da una frase della storia del mondo. Pagine: Chi sei (sesso ed età), Fede, Da dove vieni, Cosa facevi, Perché sei
// venuto, Cosa porti, Cosa temi, Il carattere, Il richiamo, Il racconto del sacerdote (ritoccabile), La tua storia
// (facoltativa) e Firma il registro. I dadi del destino danno una risposta a caso alla pagina, o a tutto il registro.
// A ogni cambio di pagina le risposte partono verso il server come bozza: se il gioco si chiude a metà, restano.
// Dopo la firma, nel nero: "Il frammento batte più forte, per un istante. La valle ti ha sentito."
//
// Ritocchi del 05/10 (approvati da Fra): sotto ogni risposta cosa porterà nella valle, il colore dell'elemento accanto a
// ogni fede, il racconto nella pagina della firma, il rombo della pagina che batte col Cuore.
//
// La parte bella (palco con il ritratto, pergamena vera, calligrafia, suoni, ceralacca, luce) è il passo 4.2b:
// per ora a destra c'è solo il frammento che batte.
//
// Tastiera e controller: frecce o croce per scegliere, Invio o A per confermare, Esc o B per tornare indietro,
// Pagina su/giù o dorsali per cambiare pagina.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "ValdorsoRegistro.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"

class FValdorsoStileAccesso;
class SBox;
class SButton;
class SMultiLineEditableTextBox;

DECLARE_DELEGATE_TwoParams(FValdorsoSuSalvaRegistro, const FValdorsoRegistro& /*Risposte*/, bool /*bFirma*/);
DECLARE_DELEGATE_OneParam(FValdorsoSuCambioRisposte, const FValdorsoRegistro& /*Risposte*/);

class VALDORSO_API SValdorsoRegistroColono : public SCompoundWidget
{
	/** (05/10) Il controllo automatico delle schermate (Test/ValdorsoTestSchermate.cpp) può aprire pannelli e pagine. */
	friend struct FValdorsoProvaSchermate;

public:
	SLATE_BEGIN_ARGS(SValdorsoRegistroColono) : _Ritratto(nullptr) {}
		SLATE_ARGUMENT(FString, NomePersonaggio)
		SLATE_ARGUMENT(FValdorsoRegistro, Registro)
		/** Il ritratto del palco (passo 4.2b); se manca, a destra resta il frammento che batte. */
		SLATE_ARGUMENT(UTextureRenderTarget2D*, Ritratto)
		/** Le risposte sono cambiate (per il palco: uomo o donna, colore della fede). */
		SLATE_EVENT(FValdorsoSuCambioRisposte, OnCambia)
		/** Bozza (bFirma falso) o firma: la risposta del server arriva con Esito. */
		SLATE_EVENT(FValdorsoSuSalvaRegistro, OnSalva)
		/** Firmato e finita la dissolvenza: il personaggio può entrare. */
		SLATE_EVENT(FSimpleDelegate, OnFirmato)
		/** "Torna ai personaggi" (la bozza è già partita). */
		SLATE_EVENT(FSimpleDelegate, OnTorna)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** La risposta del server a una bozza o alla firma. */
	void Esito(const FString& Avviso, bool bRiuscito, const FValdorsoRegistro& Salvato);

	/** Il widget a cui dare il fuoco. */
	TSharedPtr<SWidget> FuocoIniziale() const;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	/** Mentre il sacerdote scrive, un tasto o un clic ovunque finisce subito la scrittura. */
	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

	/** Le pagine, nell'ordine. */
	enum class EPagina : uint8
	{
		ChiSei, Fede, Origine, Mestiere, Motivo, Ricordo, Paura, Carattere, Richiamo, Racconto, Storia, Firma, Numero
	};

private:
	void VaiA(EPagina Nuova);
	void Avanti();
	void Indietro();
	void Torna();
	void Ridisegna();
	/** Manda la bozza se è cambiata; se un testo non va, non la manda e restituisce il motivo. */
	FText SalvaBozza();
	void Firma();
	void TiraDadiPagina();
	void TiraDadiTutto();
	void AvviaFinale();
	EActiveTimerReturnType FineFinale(double Ora, float Delta);

	bool PaginaCompleta(EPagina Quale) const;
	/** Il registro come partirebbe alla firma (con il racconto composto se è vuoto). */
	FValdorsoRegistro PerLaFirma() const;
	FText FrasePagina(EPagina Quale) const;
	FText TitoloPagina(EPagina Quale) const;

	TSharedRef<SWidget> ContenutoPagina();
	TSharedRef<SWidget> Scelte(ValdorsoRegistro::EDomanda Quale, bool bCompatte);
	TSharedRef<SWidget> PaginaChiSei();
	TSharedRef<SWidget> PaginaCarattere();
	TSharedRef<SWidget> PaginaTesto(bool bRacconto);
	TSharedRef<SWidget> PaginaFirma();
	TSharedRef<SWidget> Destra();
	TSharedRef<SWidget> Sigillo();
	/** (05/10) Il racconto nella pagina della firma, con il capolettera rosso. */
	TSharedRef<SWidget> Capolettera(const FString& Racconto);
	/** La calligrafia del sacerdote: il racconto si scrive lettera per lettera (la prima volta). */
	void IniziaScrittura();
	void FinisciScrittura();
	EActiveTimerReturnType Scrivi(double Ora, float Delta);
	/** La pagina firmata, con il sigillo, salvata come immagine tra gli screenshot. */
	EActiveTimerReturnType FotoPagina(double Ora, float Delta);
	/** Una risposta: rombo e testo. Simbolo trasparente = rombo d'oro; altrimenti il colore del simbolo (le fedi). */
	TSharedRef<SButton> PulsanteVoce(const FText& Scritta, TFunction<bool()> Scelta, TFunction<void()> Azione, float Dimensione,
		const FText& Aiuto = FText::GetEmpty(), FLinearColor Simbolo = FLinearColor::Transparent);
	void CambiaEta(int32 Di);
	void MostraMessaggio(const FText& Scritta, bool bComeErrore);

	TSharedPtr<FValdorsoStileAccesso> Stile;
	FButtonStyle StileVoce;
	FButtonStyle StileVoceScelta;

	/** (05/10) La pergamena vera: texture con i bordi bruciati (Content/UI/Registro/T_Pergamena), tenuta viva qui. */
	TStrongObjectPtr<UTexture2D> TexturePergamena;
	FSlateBrush PennelloPergamena;
	/** I campi di testo sulla pergamena: niente fondo scuro, solo un filo d'inchiostro. */
	FEditableTextBoxStyle StileCampoPergamena;
	/** (05/10) L'inchiostro del sacerdote appena scritto (fresco, lucido) e mentre si asciuga. */
	FEditableTextBoxStyle StileCampoFresco;
	FEditableTextBoxStyle StileCampoMezzo;
	/** (05/10) Il cuoio dei pulsanti del Registro (Content/UI/Registro/T_Cuoio). */
	TStrongObjectPtr<UTexture2D> TextureCuoio;
	FSlateBrush PennelloCapolettera;

	FValdorsoSuSalvaRegistro OnSalva;
	FValdorsoSuCambioRisposte OnCambia;
	FSimpleDelegate OnFirmato;
	FSimpleDelegate OnTorna;

	FString Nome;
	FValdorsoRegistro Bozza;
	/** L'ultima bozza mandata al server (per non rimandare risposte uguali). */
	FValdorsoRegistro Inviata;
	bool bInviataValida = false;
	/** Il giocatore ha ritoccato il racconto: non si riscrive da solo quando cambiano le risposte. */
	bool bRaccontoToccato = false;

	EPagina Pagina = EPagina::ChiSei;
	TSharedPtr<SBox> Corpo;
	TSharedPtr<SWidget> PrimoFuoco;
	TSharedPtr<SWidget> PulsanteAvanti;
	/** (05/10) Lo stesso pulsante come SButton: nella pagina della firma diventa rosso ceralacca. */
	TSharedPtr<SButton> BottoneProssimo;
	FButtonStyle StileFirma;
	TSharedPtr<SMultiLineEditableTextBox> CampoTesto;
	FRandomStream Dadi;

	FText Messaggio;
	bool bErrore = false;
	bool bInFirma = false;
	bool bFinale = false;
	/** "Torna ai personaggi" è partito: tutto spento finché arriva l'elenco. */
	bool bInUscita = false;

	/** Il ritratto (tenuto vivo dalla schermata finché si vede) e il suo pennello. */
	TStrongObjectPtr<UTextureRenderTarget2D> RitrattoVivo;
	FSlateBrush PennelloRitratto;
	FSlateBrush PennelloSigillo;
	FSlateBrush PennelloSigilloBordo;

	bool bScrivendo = false;
	/** Il messaggio di adesso è quello della scrittura (si toglie alla fine; gli altri restano). */
	bool bMessaggioScrittura = false;
	bool bRaccontoScritto = false;
	float Scritte = 0.f;
	TSharedPtr<FActiveTimerHandle> TimerScrittura;
	double InizioFinale = 0.0;
};
