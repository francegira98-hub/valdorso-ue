// Valdorso - La finestra dello staff (v0.1.4, 10/11/2026). Scritto da Claude, chiesta da Fra: "una finestra tools per lo
// staff con tutto il necessario di quello fatto fino ad ora", con i permessi per ogni cosa (ValdorsoPermessi.h).
//
// F9 la apre (solo a chi ha almeno un permesso, o al capo gilda dei giocatori, che vede solo i gradi). Le schede:
//   Coloni     chi e' nella valle: grado, punti (pronto a salire?), incarichi, Aureus, salute, dove sta; i gradi (lo staff
//              a nome di Mastro Ettore; il capo gilda fino all'Argento a chi ha i punti), i punti, il capo gilda.
//   Prove      a un colono: oggetti, Aureus, cura, armeria, svuota lo zaino, incarichi della Gilda fatti.
//   Bacheche   gli incarichi (appendi, togli, quelli degli abitanti), i proclami e le leggi, la cassa del villaggio.
//   Mondo      ora, tempo, durata del giorno, animali, raccolta, campane, fase del villaggio, sigillo del passo.
//   Giocatori  vai da un colono, chiamalo, portalo (o vai) in un luogo.
//   Registro   gli ultimi eventi (Gilda, gradi, staff, chat, colpi...), con un filtro.
//   Permessi   (solo l'Amministratore) i permessi e il ruolo di ognuno.
// Il PC chiede, il server controlla i permessi a ogni azione (non si fida della finestra), la fa e la annota nel registro
// eventi ("Staff", con chi l'ha fatta), poi rimanda la vista aggiornata.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoGildaDati.h"
#include "ValdorsoStaff.generated.h"

class APlayerController;
class SValdorsoStaff;

/** Un colono come lo vede la finestra dello staff. */
USTRUCT()
struct FValdorsoStaffColono
{
	GENERATED_BODY()

	UPROPERTY()
	FString Nome;

	/** Il nome dell'account (solo per chi ha il permesso Permessi). */
	UPROPERTY()
	FString Account;

	/** 0 Giocatore, 1 Staff, 2 Amministratore. */
	UPROPERTY()
	uint8 Ruolo = 0;

	UPROPERTY()
	int32 Permessi = 0;

	UPROPERTY()
	EValdorsoGrado Grado = EValdorsoGrado::Comune;

	UPROPERTY()
	int32 Punti = 0;

	UPROPERTY()
	bool bPronto = false;

	UPROPERTY()
	bool bCapo = false;

	UPROPERTY()
	int32 Incarichi = 0;

	UPROPERTY()
	int32 Monete = 0;

	UPROPERTY()
	int32 Salute = 0;

	/** Il luogo dove sta (vuoto fuori dai luoghi). */
	UPROPERTY()
	FString Luogo;
};

/** Quello che la finestra mostra (lo manda il server). */
USTRUCT()
struct FValdorsoStaffVista
{
	GENERATED_BODY()

	/** I permessi di chi guarda (ValdorsoPermessi.h) e se e' capo gilda. */
	UPROPERTY()
	int32 Permessi = 0;

	UPROPERTY()
	bool bCapo = false;

	UPROPERTY()
	FString Io;

	UPROPERTY()
	TArray<FValdorsoStaffColono> Coloni;

	UPROPERTY()
	TArray<FValdorsoIncarico> Incarichi;

	UPROPERTY()
	TArray<FValdorsoProclama> Proclami;

	UPROPERTY()
	int32 Cassa = 0;

	UPROPERTY()
	TArray<FString> Registro;
};

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoStaffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoStaffComponent();

	static UValdorsoStaffComponent* Di(const APlayerController* Controllore);

	/** Solo sul server: i permessi veri di quel giocatore (dall'account; nell'editor con l'accesso libero tutti). */
	static int32 PermessiDi(const APlayerController* Controllore);

	// --- Dal PC ----------------------------------------------------------------------------------------

	/** F9: chiede la vista (se non si hanno permessi il server non risponde, e la finestra non si apre). */
	UFUNCTION(Server, Reliable)
	void ServerApri(const FString& FiltroRegistro);

	/** Un'azione della finestra (vedi ValdorsoStaff.cpp, Fai): Azione, a chi (nome del personaggio), con che valore. */
	UFUNCTION(Server, Reliable)
	void ServerAzione(const FString& Azione, const FString& Chi, const FString& Valore);

	// --- Sul PC ----------------------------------------------------------------------------------------

	UFUNCTION(Client, Reliable)
	void ClientVista(const FValdorsoStaffVista& Nuova);

	UFUNCTION(Client, Reliable)
	void ClientEsito(const FString& Testo);

	// --- (13/11) Il collaudo della valle --------------------------------------------------------------------
	// Alla grande prova del 12/11 la E non funzionava: il server non riconosceva le porte e i PNG che il gioco gli
	// indicava (nomi diversi tra server dall'editor e gioco pacchettizzato). Il collaudo lo scopre in pochi secondi,
	// senza andare in giro: il PC manda al server, a gruppi, tutte le cose che si usano con la E che vede; il server
	// risponde per ognuna se l'ha riconosciuta e che cosa direbbe (la scritta della E). Il resoconto va in
	// Saved/Claude/collaudo.txt del gioco. Si lancia dalla finestra dello staff (F9, scheda Mondo) o con Valdorso.Collaudo.

	/** PC: comincia il collaudo (solo chi ha il permesso Mondo; il server ricontrolla). */
	void Collauda();

	/** Un gruppo di cose da riconoscere: gli attori (come li vede il PC) e i loro nomi, per il resoconto. */
	UFUNCTION(Server, Reliable)
	void ServerCollaudo(const TArray<AActor*>& Attori, const TArray<FString>& Nomi, bool bUltimo);

	/** La risposta del server, una riga per cosa; Riconosciute e Perse contano questo gruppo. */
	UFUNCTION(Client, Reliable)
	void ClientCollaudo(const TArray<FString>& Righe, int32 Riconosciute, int32 Perse, bool bUltimo);

	const FValdorsoStaffVista& Vista() const { return VistaArrivata; }
	const FString& Esito() const { return UltimoEsito; }
	int32 Cambiamenti() const { return NumeroCambiamenti; }

	/** Il filtro del registro scritto nella finestra (per gli aggiornamenti). */
	FString Filtro;

	/** PC: F9 premuto, si aspetta la vista per aprire la finestra. */
	bool bApertura = false;

	/** La finestra aperta (solo sul PC). */
	TSharedPtr<SValdorsoStaff> Schermata;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Server: la vista per questo giocatore (solo quello che i suoi permessi gli lasciano vedere). */
	FValdorsoStaffVista Componi(const FString& FiltroRegistro) const;
	/** Server: fa l'azione; restituisce cosa dire. */
	FString Fai(const FString& Azione, const FString& Chi, const FString& Valore);
	/** Server: il PlayerController del colono con quel nome (nullptr se non c'e'). */
	APlayerController* Colono(const FString& Nome) const;
	FString NomeMio() const;

	FValdorsoStaffVista VistaArrivata;

	/** (13/11) PC: il collaudo in corso: le cose ancora da mandare (un gruppo alla volta, il prossimo quando arriva la
	 *  risposta, così la rete non si intasa), le righe arrivate e i conti. */
	TArray<TWeakObjectPtr<AActor>> CollaudoDaMandare;
	TArray<FString> CollaudoNomiDaMandare;
	TArray<FString> CollaudoRighe;
	int32 CollaudoRiconosciute = 0;
	int32 CollaudoPerse = 0;
	bool bCollaudoInCorso = false;
	/** Quando è arrivata l'ultima risposta (se il server non risponde più, dopo 15 secondi si può ricominciare). */
	double CollaudoUltimaRisposta = 0.0;

	/** PC: manda al server il prossimo gruppo del collaudo. */
	void CollaudoProssimo();
	FString UltimoEsito;
	int32 NumeroCambiamenti = 0;
	double UltimaRichiesta = 0.0;
};

namespace ValdorsoStaffSchermo
{
	VALDORSO_API bool Aperta(const APlayerController* Controllore);
	/** Apre la finestra con la vista arrivata (la chiama ClientVista dopo F9). */
	VALDORSO_API bool Apri(APlayerController* Controllore);
	/** Il tasto F9: apre (chiedendo la vista al server) o chiude. */
	VALDORSO_API void Alterna(APlayerController* Controllore);
	VALDORSO_API void Chiudi(APlayerController* Controllore);
}
