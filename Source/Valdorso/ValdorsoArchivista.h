// Valdorso - L'archivista: custodisce account e inviti sul server.
// Scritto da Claude il 03/10/2026 (v0.1.2, passo 2.1).
//
// Dove: Saved/Server/Archivio sul server (mai su GitHub).
//   Account/<Id>.json   un file per account, con .bak1 e .bak2 (le due versioni precedenti)
//   Inviti.json         i codici d'invito (solo la loro impronta), con le stesse copie
//   Registro_accessi.log ogni accesso, errore e azione dell'Amministratrice, con data e indirizzo
//
// Regole principali:
//   - il server tiene solo l'impronta delle password (ValdorsoSicurezza), calcolata fuori dal filo principale;
//   - 5 password sbagliate bloccano l'account (15 min, poi 30, 60... fino a 24 ore);
//   - 20 errori in 10 minuti dallo stesso indirizzo lo bloccano per 30 minuti;
//   - un codice d'invito vale una volta e scade (14 giorni se non si dice altro);
//   - il primo account creato sul server diventa Amministratore.
//
// Tutte le chiamate si fanno sul filo principale del server; le risposte arrivano sempre sul filo principale.
// I comandi della console sono in ValdorsoArchivistaComandi.cpp.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tasks/Pipe.h"
#include "Tasks/Task.h"
#include "Containers/Ticker.h"
#include "ValdorsoArchivista.generated.h"

UENUM(BlueprintType)
enum class EValdorsoRuolo : uint8
{
	Giocatore,
	Staff,
	Amministratore
};

UENUM(BlueprintType)
enum class EValdorsoStatoAccount : uint8
{
	Attivo,
	Sospeso,
	Bandito
};

// ScriptName: per Python il nome deve essere diverso da quello di FValdorsoEsitoAccount (avviso di LogPython).
UENUM(BlueprintType, meta = (ScriptName = "ValdorsoTipoEsitoAccount"))
enum class EValdorsoEsitoAccount : uint8
{
	Ok,
	OkDeveCambiarePassword,
	CredenzialiSbagliate,
	TroppiTentativi,
	AccountSospeso,
	AccountBandito,
	InvitoNonValido,
	NomeNonValido,
	NomeGiaUsato,
	PasswordDebole,
	ServerOccupato,
	ErroreInterno,
	/** Il biglietto del rientro senza password non vale (scaduto, già usato, server riavviato): serve la password. */
	RientroNonValido
};

/** Un account, così come sta nel suo file. Le date sono secondi Unix (UTC). */
USTRUCT()
struct FValdorsoAccount
{
	GENERATED_BODY()

	/** Versione del formato del file: se un giorno cambia, l'archivista sa come aggiornarlo. */
	UPROPERTY()
	int32 VersioneSchema = 1;

	/** Quante volte il file è stato scritto. */
	UPROPERTY()
	int64 Versione = 0;

	UPROPERTY()
	FString Id;

	/** Il nome come l'ha scritto il giocatore. */
	UPROPERTY()
	FString Nome;

	/** Il nome in minuscolo: due account non possono differire solo per le maiuscole. */
	UPROPERTY()
	FString NomeChiave;

	UPROPERTY()
	FString Algoritmo;

	UPROPERTY()
	int32 Iterazioni = 0;

	/** Sale e impronta della password, in Base64. */
	UPROPERTY()
	FString Sale;

	UPROPERTY()
	FString Impronta;

	UPROPERTY()
	EValdorsoRuolo Ruolo = EValdorsoRuolo::Giocatore;

	UPROPERTY()
	EValdorsoStatoAccount Stato = EValdorsoStatoAccount::Attivo;

	UPROPERTY()
	FString MotivoStato;

	UPROPERTY()
	int64 SospesoFino = 0;

	UPROPERTY()
	int32 TentativiFalliti = 0;

	/** Blocchi per troppi tentativi uno dopo l'altro: ognuno dura il doppio del precedente. */
	UPROPERTY()
	int32 BlocchiDiFila = 0;

	UPROPERTY()
	int64 BloccatoFino = 0;

	/** Vero dopo un reset dall'Amministratrice: al prossimo accesso si sceglie una password nuova. */
	UPROPERTY()
	bool bDeveCambiarePassword = false;

	UPROPERTY()
	int64 CreatoIl = 0;

	UPROPERTY()
	int64 UltimoAccesso = 0;

	UPROPERTY()
	int64 PasswordCambiataIl = 0;

	/** L'indizio dell'invito usato (per esempio VALD-7K3P) e chi l'aveva creato. */
	UPROPERTY()
	FString InvitoUsato;

	UPROPERTY()
	FString InvitatoDa;

	/** Codici di recupero: le impronte di quelli non ancora usati, il sale e quando sono stati creati. */
	UPROPERTY()
	TArray<FString> CodiciRecupero;

	UPROPERTY()
	FString SaleRecupero;

	UPROPERTY()
	int64 CodiciCreatiIl = 0;

	/** Predisposti per la registrazione con l'email (roadmap): per ora restano vuoti. */
	UPROPERTY()
	FString Email;

	UPROPERTY()
	bool bEmailVerificata = false;

	/** Gli Id dei personaggi (fino a 3, passo 3 della v0.1.2). */
	UPROPERTY()
	TArray<FString> Personaggi;

	/** Note dello staff, con data e autore. I giocatori non le vedono. */
	UPROPERTY()
	TArray<FString> NoteStaff;
};

/** Un codice d'invito. Il codice vero non si salva: solo la sua impronta e l'inizio (l'indizio). */
USTRUCT()
struct FValdorsoInvito
{
	GENERATED_BODY()

	UPROPERTY()
	FString ImprontaCodice;

	UPROPERTY()
	FString Indizio;

	UPROPERTY()
	FString CreatoDa;

	UPROPERTY()
	int64 CreatoIl = 0;

	UPROPERTY()
	int64 ScadeIl = 0;

	UPROPERTY()
	FString UsatoDa;

	UPROPERTY()
	int64 UsatoIl = 0;

	UPROPERTY()
	bool bRevocato = false;

	UPROPERTY()
	FString Nota;
};

/** Il file Inviti.json. */
USTRUCT()
struct FValdorsoArchivioInviti
{
	GENERATED_BODY()

	UPROPERTY()
	int32 VersioneSchema = 1;

	UPROPERTY()
	int64 Versione = 0;

	UPROPERTY()
	TArray<FValdorsoInvito> Inviti;
};

/** La risposta dell'archivista a un giocatore. Il messaggio è già pronto da mostrare. */
USTRUCT(BlueprintType)
struct FValdorsoEsitoAccount
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Valdorso|Account")
	EValdorsoEsitoAccount Esito = EValdorsoEsitoAccount::ErroreInterno;

	UPROPERTY(BlueprintReadOnly, Category = "Valdorso|Account")
	FString Messaggio;

	UPROPERTY(BlueprintReadOnly, Category = "Valdorso|Account")
	FString AccountId;

	UPROPERTY(BlueprintReadOnly, Category = "Valdorso|Account")
	FString Nome;

	UPROPERTY(BlueprintReadOnly, Category = "Valdorso|Account")
	EValdorsoRuolo Ruolo = EValdorsoRuolo::Giocatore;

	/** Solo quando ne nascono di nuovi (primo ingresso, account senza codici, codici finiti): da mostrare una volta. */
	UPROPERTY()
	TArray<FString> CodiciRecupero;

	/** Quanti codici di recupero restano (dopo un recupero), -1 se non serve dirlo. */
	UPROPERTY()
	int32 CodiciRimasti = -1;

	bool Riuscito() const
	{
		return Esito == EValdorsoEsitoAccount::Ok || Esito == EValdorsoEsitoAccount::OkDeveCambiarePassword;
	}
};

UCLASS()
class VALDORSO_API UValdorsoArchivista : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	using FRisposta = TFunction<void(const FValdorsoEsitoAccount&)>;

	/** L'archivista del server di questo mondo (nullo su un client). */
	static UValdorsoArchivista* Di(const UObject* Contesto);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Giocatori ---------------------------------------------------------------------------------

	/** Primo ingresso: crea l'account con un codice d'invito. Indirizzo = da dove arriva la richiesta. */
	void CreaAccount(const FString& CodiceInvito, const FString& Nome, const FString& Password, const FString& Indirizzo, FRisposta Risposta);

	/** Accesso con nome e password. */
	void Accedi(const FString& Nome, const FString& Password, const FString& Indirizzo, FRisposta Risposta);

	/**
	 * Il giocatore ha scritto i codici di recupero appena mostrati: da adesso valgono (e si salvano).
	 * Finché non conferma restano in attesa, solo in memoria: se il collegamento cade prima, al prossimo accesso
	 * ne riceve di nuovi (così non succede mai di avere codici salvati che nessuno ha visto).
	 */
	void ConfermaCodiciRecupero(const FString& AccountId);

	/** Password dimenticata: nome, un codice di recupero e la password nuova. Il codice si consuma. */
	void RecuperaConCodice(const FString& Nome, const FString& Codice, const FString& Nuova, const FString& Indirizzo, FRisposta Risposta);

	/** Cambio della password (anche quella temporanea dopo un reset). */
	void CambiaPassword(const FString& Nome, const FString& Attuale, const FString& Nuova, const FString& Indirizzo, FRisposta Risposta);

	// --- Rientro senza password (v0.1.2) -----------------------------------------------------------
	// Chi è entrato riceve un biglietto: 32 byte casuali, che il gioco tiene solo in memoria. Se il collegamento
	// si interrompe, entro 15 minuti il gioco rientra da solo mostrando il biglietto. Il server tiene solo
	// l'impronta del biglietto, e solo in memoria: un riavvio del server li annulla tutti (si rientra con la password).
	// Un biglietto vale una volta; a ogni rientro se ne dà uno nuovo. Un account ha al massimo un biglietto.

	/** Un biglietto nuovo per l'account (annulla quello vecchio). Vale finché si è collegati, poi 15 minuti. Vuoto se non riesce. */
	FString CreaBiglietto(const FString& AccountId);

	/** Rientro con il biglietto: risponde subito (niente calcoli lenti). */
	void RientraConBiglietto(const FString& Biglietto, const FString& Indirizzo, FRisposta Risposta);

	/** Annulla il biglietto dell'account (password reimpostata, sospensione, bando). */
	void AnnullaBiglietto(const FString& AccountId);

	// --- Collegamenti (un solo collegamento per account, usato dal passo 2.3) ----------------------

	/** Segna l'account come collegato. Falso se lo era già. */
	bool SegnaCollegato(const FString& AccountId);
	void SegnaScollegato(const FString& AccountId);
	bool ECollegato(const FString& AccountId) const { return Collegati.Contains(AccountId); }

	// --- Amministrazione (rispondono con un testo per la console) ----------------------------------

	FString CreaInvito(int32 Giorni, const FString& Autore, const FString& Nota, FString& OutCodice);
	FString ElencoInviti() const;
	FString RevocaInvito(const FString& Indizio, const FString& Autore);

	FString ElencoAccount() const;
	FString InfoAccount(const FString& Nome) const;
	FString ImpostaRuolo(const FString& Nome, const FString& Ruolo, const FString& Autore);
	FString ReimpostaPassword(const FString& Nome, const FString& Autore);
	FString Sospendi(const FString& Nome, int32 Ore, const FString& Motivo, const FString& Autore);
	FString Banna(const FString& Nome, const FString& Motivo, const FString& Autore);
	FString Riattiva(const FString& Nome, const FString& Autore);
	FString Sblocca(const FString& Nome, const FString& Autore);
	FString AggiungiNota(const FString& Nome, const FString& Nota, const FString& Autore);

	// --- Privacy (v0.1.2) ---------------------------------------------------------------------------

	/**
	 * Cancella un account per sempre: il suo file con le copie, il biglietto del rientro; negli inviti, negli
	 * altri account e nel registro il nome diventa "[account cancellato]". Conferma = il nome riscritto.
	 * Un Amministratore non si cancella (prima si cambia il ruolo). Chi chiama scollega prima il giocatore.
	 */
	FString CancellaAccount(const FString& Nome, const FString& Conferma, const FString& Autore);

	/** Oscura gli indirizzi IP più vecchi di 30 giorni nel registro (all'avvio e poi ogni 6 ore, da solo). */
	void PulisciRegistro();

	int32 NumeroAccount() const { return Account.Num(); }

	/** L'Id dell'account con questo nome (vuoto se non c'è). */
	FString IdDi(const FString& Nome) const;

private:
	/** Un calcolo lento da fare fuori dal filo principale; restituisce il seguito da eseguire sul filo principale. */
	using FCalcolo = TFunction<TFunction<void()>()>;

	struct FContoIndirizzo
	{
		int32 Errori = 0;
		int64 Inizio = 0;
		int64 BloccatoFino = 0;
	};

	// Caricamento e scrittura
	void CaricaTutto();
	void SalvaAccount(FValdorsoAccount& Dati);
	void SalvaInviti();
	void Scrivi(const FString& Percorso, const FString& Testo);
	void Annota(const FString& Riga);

	// Calcoli in coda
	bool Accoda(FCalcolo Calcolo);
	void Avvia(FCalcolo Calcolo);
	void CalcoloFinito();

	// Seguiti sul filo principale
	void ConcludiAccesso(const FString& Chiave, const FString& Indirizzo, bool bGiusta,
		const FString& NuovoSale, const FString& NuovaImpronta, int32 NuoveIterazioni, FRisposta Risposta);
	void ConcludiCreazione(const FString& Nome, const FString& ImprontaCodice, const FString& Indirizzo, bool bCalcoloOk,
		const FString& Sale, const FString& Impronta, int32 Iterazioni, FRisposta Risposta);
	void ConcludiCambio(const FString& Chiave, const FString& Indirizzo, bool bGiusta,
		const FString& Sale, const FString& Impronta, int32 Iterazioni, FRisposta Risposta);

	// Tentativi
	bool IndirizzoBloccato(const FString& Indirizzo, int64 Ora, int64& OutSecondiRimasti) const;
	void ErroreDaIndirizzo(const FString& Indirizzo, int64 Ora);
	void ErroreSullAccount(FValdorsoAccount& Dati, const FString& Indirizzo, int64 Ora);

	/** 8 codici nuovi per l'account, in attesa di conferma: restituisce i codici da mostrare, nella forma XXXX-XXXX-XXXX-XXXX. */
	TArray<FString> CreaCodiciRecupero(const FValdorsoAccount& Dati);

	/** Codici mostrati e non ancora confermati: account -> sale e impronte. */
	struct FCodiciInAttesa
	{
		FString Sale;
		TArray<FString> Impronte;
	};
	TMap<FString, FCodiciInAttesa> CodiciInAttesa;

	FValdorsoInvito* TrovaInvitoValido(const FString& ImprontaCodice, int64 Ora);
	FValdorsoAccount* TrovaAccount(const FString& Nome);
	const FValdorsoAccount* TrovaAccount(const FString& Nome) const;

	FString Cartella;
	FString CartellaAccount;
	FString FileInviti;
	FString FileRegistro;

	/** Gli account, per nome in minuscolo. */
	TMap<FString, FValdorsoAccount> Account;
	FValdorsoArchivioInviti Inviti;

	/** Nomi e inviti prenotati mentre si calcola l'impronta di un account nuovo. */
	TSet<FString> NomiInCreazione;
	TSet<FString> InvitiInUso;

	TMap<FString, FContoIndirizzo> ContiIndirizzi;
	TSet<FString> Collegati;

	/** I biglietti del rientro: impronta SHA-256 del biglietto -> account e scadenza (0 = finché è collegato). */
	struct FBiglietto
	{
		FString AccountId;
		int64 ScadeIl = 0;
	};
	TMap<FString, FBiglietto> Biglietti;

	TArray<FCalcolo> InAttesa;
	int32 InCorso = 0;

	/** Il filo delle scritture: un file alla volta, nell'ordine in cui sono chieste. */
	TUniquePtr<UE::Tasks::FPipe> Scrittore;
	UE::Tasks::FTask UltimaScrittura;

	/** Sale per i calcoli finti sui nomi che non esistono. */
	TArray<uint8> SaleFinto;

	/** Il richiamo ogni 6 ore per PulisciRegistro. */
	FTSTicker::FDelegateHandle ManigliaPulizia;
};
