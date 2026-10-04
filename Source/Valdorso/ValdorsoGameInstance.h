// Valdorso - L'istanza del gioco: vive per tutta la partita, anche quando si cambia livello.
// Scritto da Claude il 03/10/2026 (v0.1.2, passo 2.3).
//
// Due compiti:
//   1. La connessione cifrata.
//      Sul server: tiene la chiave privata RSA (Saved/Server/Chiavi, creata da sola al primo avvio)
//      e apre la chiave di sessione che arriva da ogni client.
//      Sul client: inventa una chiave di sessione casuale, la chiude con la chiave pubblica del server
//      e la manda come EncryptionToken; quando il server risponde, Unreal cifra tutti i pacchetti (AES-GCM).
//   2. La richiesta d'accesso: nome, password o codice d'invito restano qui solo il tempo di arrivare
//      nell'anticamera del server; poi il controllore del giocatore li prende, li manda e li cancella.
//
// La chiave pubblica per i giocatori sta in Config/DefaultGame.ini (ChiavePubblicaServer). Nelle versioni
// di sviluppo, se manca, si legge Saved/Server/Chiavi/server_pubblica.pem (server e client sullo stesso PC).

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ValdorsoGameInstance.generated.h"

class UNetDriver;

/** Che cosa vuole fare il giocatore quando arriva al server. */
enum class EValdorsoModoAccesso : uint8
{
	Entra,
	PrimoIngresso,
	/** Rientro senza password dopo un collegamento interrotto (con il biglietto del server). */
	Rientro,
	/** Password dimenticata: nome, codice di recupero e password nuova. */
	Recupero
};

struct FValdorsoRichiestaAccesso
{
	EValdorsoModoAccesso Modo = EValdorsoModoAccesso::Entra;
	FString Nome;
	FString Password;
	FString CodiceInvito;
	FString Biglietto;
	FString CodiceRecupero;
};

UCLASS(Config = Game)
class VALDORSO_API UValdorsoGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Indirizzo del server (host:porta). */
	UPROPERTY(Config)
	FString IndirizzoServer = TEXT("127.0.0.1:7777");

	/** La chiave pubblica del server: il testo del file server_pubblica.pem senza le righe BEGIN/END, tutto su una riga. */
	UPROPERTY(Config)
	FString ChiavePubblicaServer;

	/** Nell'editor (Gioca come client) si entra senza cifratura e senza password, come "Prova1", "Prova2"... Mai fuori dall'editor. */
	UPROPERTY(Config)
	bool bAccessoLiberoNellEditor = true;

	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void ReceivedNetworkEncryptionToken(const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate) override;
	virtual void ReceivedNetworkEncryptionAck(const FOnEncryptionKeyResponse& Delegate) override;

	// --- Client ------------------------------------------------------------------------------------

	/**
	 * Controlla, senza collegarsi, che il viaggio verso il server possa partire (gioco pronto, chiave pubblica presente).
	 * Il menu lo chiede prima del volo (passo 2.5), così un errore si vede subito e non dopo il nero.
	 */
	bool PuoiCollegarti(FString& OutErrore, const FString& Indirizzo = FString()) const;

	/** Si collega al server con la connessione cifrata e porta con sé la richiesta. Falso (con il motivo) se non può partire. */
	bool Collegati(const FValdorsoRichiestaAccesso& Richiesta, FString& OutErrore, const FString& Indirizzo = FString());

	/** Il controllore del giocatore prende la richiesta (una volta sola: poi qui non resta niente). */
	bool PrendiRichiesta(FValdorsoRichiestaAccesso& Out);

	/** Un messaggio da mostrare quando si torna al menu (per esempio "Nome o password sbagliati."). */
	void RicordaMessaggio(const FString& Messaggio);
	FString PrendiMessaggio();

	/** L'ultimo nome usato, per non doverlo riscrivere. */
	FString UltimoNome;

	// --- Rientro senza password (v0.1.2) -----------------------------------------------------------

	/** Il server ha dato il biglietto del rientro (solo in memoria: chiudendo il gioco sparisce). */
	void RicordaBiglietto(const FString& Biglietto, const FString& Nome);

	/** Via il biglietto (espulsione, uscita voluta, rientro rifiutato). */
	void DimenticaBiglietto();

	/**
	 * Il menu chiede se rientrare da solo: vero se c'è un biglietto, se il collegamento si è interrotto
	 * (non un'espulsione) e se non si è già provato 3 volte di fila. Ogni chiamata vera conta un tentativo.
	 */
	bool PrendiRientro(FValdorsoRichiestaAccesso& Out);

	/** Il nome di chi può rientrare (per le scritte), vuoto se nessuno. */
	FString NomeRientro() const { return Biglietto.IsEmpty() ? FString() : NomeBiglietto; }

	/** Chiamata quando si è entrati davvero: i tentativi di rientro ripartono da zero. */
	void Entrato() { TentativiRientro = 0; bCollegamentoInterrotto = false; }

	// --- Server ------------------------------------------------------------------------------------

	/** Carica la chiave privata del server; se non c'è, crea la coppia e la salva. */
	bool PreparaChiaveServer();

private:
	FString CartellaChiavi() const;
	bool ChiavePubblica(FString& OutPem) const;
	void SuErroreDiRete(UWorld* Mondo, UNetDriver* Driver, ENetworkFailure::Type Tipo, const FString& Errore);

	FString Biglietto;
	FString NomeBiglietto;
	int32 TentativiRientro = 0;
	bool bCollegamentoInterrotto = false;

	TArray<uint8> ChiaveSessione;
	TOptional<FValdorsoRichiestaAccesso> RichiestaInSospeso;
	FString MessaggioInSospeso;
	FString ChiavePrivataPem;
	FDelegateHandle ManigliaErroreDiRete;
};
