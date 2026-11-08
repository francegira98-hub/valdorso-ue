// Valdorso - Il controllore del menu principale: mostra il menu "Oro e brace", guarda la scena
// dalla telecamera con l'etichetta MenuCamera (che ondeggia piano), e porta nella valle o chiude il gioco.
//
// (07/11/2026) Prima del menu, una volta per avvio, la schermata d'avvio (SValdorsoAvvio: logo, avviso, "premi un
// tasto"); nel menu "Impostazioni" apre le impostazioni vere e in basso a destra c'è lo stato del mondo chiesto al
// server (ValdorsoStatoMondo), richiesto di nuovo ogni 30 secondi.
//
// L'ingresso nella valle (v0.1.2, passo 2.5, 04/10/2026): prima il volo, poi il collegamento.
//   - Ogni volta: il volo corto verso il frammento (2,4 s), il nero, e solo allora parte il collegamento al server.
//   - Al primo ingresso (con l'invito): il volo lungo lungo la curva AValdorsoVoloIngresso (circa 7 s),
//     con "La valle ti accoglie" e il Cuore che batte più forte; si salta con Esc.
//   Sul nero, mentre ci si collega: "In cammino verso la valle...". Se il server non risponde si torna al menu
//   con il motivo (lo fa l'istanza del gioco, ValdorsoGameInstance).
//
// (08/11, v0.1.3) Il menu nella valle vera al tramonto (ValdorsoMenuValle.h): in Lvl_Menu, se c'è la valle, questo
// controllore mette il nero e la apre senza server (AValdorsoMenuValleGameMode); di là lo stesso controllore mostra
// l'avvio e il menu, e la telecamera fa il giro lento sopra il villaggio. "Entra" e il volo lungo partono da lì.
// Se la valle manca o non si apre, tutto resta come prima, nella scena di Lvl_Menu.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ValdorsoGameInstance.h"
#include "ValdorsoMenuValle.h"
#include "ValdorsoMenuController.generated.h"

class SValdorsoMenuPrincipale;
class SValdorsoAvvio;
class SValdorsoImpostazioni;
class SValdorsoPrimaDiEntrare;
class SValdorsoVolo;
class AValdorsoVoloIngresso;
class UCameraComponent;

UCLASS()
class VALDORSO_API AValdorsoMenuController : public APlayerController
{
	GENERATED_BODY()

public:
	AValdorsoMenuController();

	/** Il livello che si apre con "Entra nella valle" (per ora la mappa di prova; dalla v0.1.3 la valle). */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	FName LivelloValle = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");

	/** (08/11) La valle vera che si vede dietro il menu (si apre senza server; se manca resta la scena di Lvl_Menu). */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	FName LivelloMenuValle = TEXT("/Game/Prove/L_Valle");

	/** (08/11) L'apertura dell'obiettivo della telecamera del giro nella valle, in gradi. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	float SguardoGiro = 55.f;

	/** Quanto dura il volo verso il frammento prima di entrare nella valle, in secondi. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	float DurataTransizione = 2.4f;

	/** Al primo ingresso il Cuore batte più forte: a ogni colpo lo sguardo si stringe di questi gradi. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	float ColpoSulloSguardo = 1.6f;

	/** Quanto ondeggia la telecamera, in centimetri. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	float Ondeggio = 14.f;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	// --- (08/11) Il menu nella valle vera al tramonto ----------------------------------------------
	/** L'avvio (una volta per avvio del gioco) oppure subito il menu; poi i messaggi di ritorno dal server. */
	void Comincia();
	/** In Lvl_Menu: vero se si può andare nella valle (comando acceso, la valle c'è, non è già andata male, niente rientro). */
	bool ValleDisponibile() const;
	/** In Lvl_Menu: il nero e la riga "La valle si sveglia...", poi (dopo un attimo, perché si vedano) la valle. */
	void VaiNellaValle();
	void AvanzaVersoLaValle(float DeltaSeconds);
	/** Apre la valle senza server, con la modalità di gioco del menu. */
	void ApriLaValle();
	/** In Lvl_Menu, se la valle non si è aperta: si toglie il nero e si va avanti con la scena di sempre. */
	void RinunciaAllaValle();
	/** Nella valle: la telecamera del giro (nasce qui) e il nero finché la valle intorno non è arrivata. */
	void PreparaGiro();
	void AvanzaGiro(float DeltaSeconds);

	/** Vero se questo menu sta nella valle vera (aperta con AValdorsoMenuValleGameMode). */
	bool bNellaValle = false;
	bool bVersoLaValle = false;
	bool bValleChiesta = false;
	float AttesaValle = 0.f;

	FValdorsoGiroMenu GiroValle;
	float DistanzaGiro = 0.f;
	/** Quanto è salita la telecamera sopra il giro perché sotto c'era qualcosa (sfuma piano). */
	float AlzataGiro = 0.f;
	/** Dove guarda adesso la telecamera del giro ("Entra" ci vola incontro). */
	FVector BersaglioGiro = FVector::ZeroVector;
	bool bValleVisibile = false;
	bool bSuoloTrovato = false;
	float AttesaVista = 0.f;
	float SuoloDa = -1.f;

	// --- (07/11) L'avvio, le impostazioni e lo stato del mondo ------------------------------------
	void MostraAvvio();
	void FineAvvio();
	/** Il menu sopra la scena, con il fuoco sul primo pulsante; poi i messaggi di ritorno dal server. */
	void CreaMenu();
	void ControllaRitorno();
	void ApriImpostazioni();
	void ChiudiImpostazioni();
	void ChiediStatoMondo();
	TSharedPtr<SValdorsoAvvio> Avvio;
	TSharedPtr<SValdorsoImpostazioni> Impostazioni;
	float AttesaStato = 0.f;
	bool bStatoInViaggio = false;
	/** L'ultima risposta (arriva anche durante l'avvio, prima che il menu ci sia). */
	FText UltimaRigaStato;
	FText UltimaNotiziaStato;

	void EntraNellaValle();
	void Esci();

	// "Prima di entrare" (v0.1.2, passo 2.2): il pannello dell'accesso sopra il menu.
	void MostraAccesso(const FText& Messaggio);
	void ChiudiAccesso();
	void SuRichiestaAccesso(const FValdorsoRichiestaAccesso& Richiesta);
	void ProvaLocale();
	void Arriva();

	/** Rientro senza password (v0.1.2): dopo un collegamento interrotto, il menu riparte da solo verso il server. */
	void PreparaRientro(const FValdorsoRichiestaAccesso& Rientro, const FString& Messaggio);
	void AvanzaRientro(float DeltaSeconds);
	void AnnullaRientro();
	void TogliScritte();
	TOptional<FValdorsoRichiestaAccesso> RientroInAttesa;
	FString MessaggioRientro;
	float AttesaRientro = 0.f;

	/** Se il collegamento non può partire (dopo il nero): si riapre il menu con il motivo. */
	void TornaAlMenu(const FString& Motivo);

	TSharedPtr<SValdorsoPrimaDiEntrare> Accesso;

	/** Vero: alla fine del volo ci si collega al server; falso: si apre la mappa di prova in locale. */
	bool bVersoIlServer = false;

	/** La richiesta aspetta qui la fine del volo; poi va all'istanza del gioco e qui si cancella. */
	TOptional<FValdorsoRichiestaAccesso> RichiestaDaMandare;

	TSharedPtr<SValdorsoMenuPrincipale> Menu;
	TWeakObjectPtr<AActor> Telecamera;
	FVector PosizioneBase = FVector::ZeroVector;
	FRotator RotazioneBase = FRotator::ZeroRotator;
	float Tempo = 0.f;

	/** "Entra nella valle": la telecamera vola verso il frammento, il menu sfuma, tutto diventa nero. */
	void AvanzaTransizione(float DeltaSeconds);
	/** Menu e, se c'è ancora, il pannello "Prima di entrare" sfumano insieme all'inizio del volo (ritocco del 05/10). */
	void SfumaSopra(float Opacita);

	/** Il volo lungo del primo ingresso, lungo la curva. */
	void AvanzaVoloLungo(float DeltaSeconds);
	void PreparaVoloLungo();
	void SaltaVolo();

	/** Dopo il nero, mentre ci si collega: la riga "In cammino verso la valle...". */
	void AvanzaCollegamento(float DeltaSeconds);

	TSharedPtr<SValdorsoVolo> Scritte;
	TWeakObjectPtr<AValdorsoVoloIngresso> Volo;
	TWeakObjectPtr<UCameraComponent> Obiettivo;
	float SguardoBase = 50.f;
	bool bVoloLungo = false;
	bool bSaltato = false;
	float AttesaSalto = 0.f;
	float TempoCollegamento = 0.f;

	bool bInTransizione = false;
	bool bBuio = false;
	bool bLivelloAperto = false;
	float TempoTransizione = 0.f;
	FVector Partenza = FVector::ZeroVector;
	FQuat GiroPartenza = FQuat::Identity;
	FVector Arrivo = FVector::ZeroVector;
	FQuat GiroArrivo = FQuat::Identity;
};
