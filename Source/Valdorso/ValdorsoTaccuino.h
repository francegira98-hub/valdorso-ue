// Valdorso - Bussola e taccuino di viaggio (v0.1.3). Scritto da Claude il 07/11/2026.
//
// Come funziona:
// - UValdorsoTaccuinoComponent sta sul PlayerController (uno per giocatore). Il taccuino vero lo tiene il server:
//   ogni secondo guarda dov'è il colono e, se è entrato nel raggio di un luogo che non conosceva, lo annota
//   ("Luogo scoperto: Il Mulino", l'avviso in alto) e salva il personaggio. Quando il colono legge una bacheca o
//   un'iscrizione (AValdorsoBacheca::Interagisci, sul server) il testo si ricopia nel taccuino: niente doppioni, al
//   massimo 200 iscrizioni (via le più vecchie). Il client non decide niente: chiede e mostra.
// - Al giocatore arrivano (solo a lui, COND_OwnerOnly) i luoghi scoperti e i titoli delle iscrizioni: sono pochi
//   byte e servono alla bussola. Il testo di un'iscrizione arriva solo quando lo si apre nel taccuino (una richiesta
//   al server, poi resta in memoria finché il taccuino non cambia): così 200 iscrizioni non pesano sulla rete.
// - Il taccuino si salva con il personaggio (FValdorsoPersonaggio::Taccuino): il PlayerController lo dà al componente
//   quando il personaggio entra (Carica) e lo riprende quando salva (Dati). (08/11) Con il taccuino viaggia anche la
//   memoria dei dialoghi con i PNG (GiaParlato, HaSegno...): la usa AValdorsoPNG, così il saluto e i segni restano
//   anche dopo lo spegnimento del server.
// - La bussola: una striscia in alto al centro (SValdorsoBussola) con i punti cardinali che scorrono con lo sguardo e
//   i segni dei luoghi già scoperti (col nome, quando ci si guarda davanti). Si nasconde quando c'è una schermata
//   aperta (si vede il mouse), nella modalità foto e senza il colono. Valdorso.Bussola 0 la spegne.
// - Il taccuino (tasto J): una pergamena con tre schede, Luoghi, Iscrizioni e Rune (il quaderno delle rune).
//   Si chiude con J, Esc, E o il pulsante Chiudi; 1, 2, 3 (o i dorsali del controller) cambiano scheda.
// Comandi di console (sul server, o giocando da soli): Valdorso.Taccuino.ScopriTutto (tutti i luoghi, per le prove),
// Valdorso.Taccuino.Dimentica (svuota il taccuino del giocatore); (08/11) Valdorso.Dialoghi.Dimentica (i PNG
// dimenticano chi ha già parlato con loro e i segni, anche nei file dei personaggi).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoBattito.h"
#include "ValdorsoTaccuino.generated.h"

class APlayerController;
class SValdorsoTaccuino;
class SWidget;

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoTaccuinoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoTaccuinoComponent();

	/** Il taccuino del giocatore di quel PlayerController (nullptr se non ce l'ha). */
	static UValdorsoTaccuinoComponent* Di(const APlayerController* Controllore);

	// --- Sul server ---------------------------------------------------------------------------------

	/** Il personaggio entra: il suo taccuino, letto dal file (sostituisce quello di prima). */
	void Carica(const FValdorsoTaccuino& Salvato);

	/** Il taccuino com'è adesso, da salvare nel file del personaggio. */
	FValdorsoTaccuino Dati() const;

	/** Una bacheca o un'iscrizione letta: si ricopia nel taccuino (se è nuova si salva il personaggio). */
	void AnnotaIscrizione(const FString& Titolo, const TArray<FString>& Righe);

	/** Annota un luogo come scoperto (con l'avviso al giocatore); falso se lo conosceva già. */
	bool Scopri(const FString& Nome, bool bAvvisa = true);

	/** Per le prove (console): svuota il taccuino (luoghi e iscrizioni; la memoria dei dialoghi resta). */
	void Dimentica();

	// --- (08/11) Sul server: la memoria dei dialoghi con i PNG (ValdorsoDialoghi.h) -----------------------
	// Sta nel file del personaggio (FValdorsoTaccuino::Dialoghi): quando cambia si salva il personaggio, come per i
	// luoghi scoperti. Vale solo con un personaggio vero (HaPersonaggio): chi entra con l'accesso libero dell'editor
	// non ha un file, e i PNG usano la memoria per nome del server (ValdorsoDialoghi::GiaParlato).

	/** Vero se il taccuino è quello di un personaggio salvato (arrivato con Carica). */
	bool HaPersonaggio() const { return bPersonaggio; }

	/** Vero se questo personaggio ha già parlato con il PNG di quel dialogo. */
	bool GiaParlato(FName IdDialogo) const;

	/** Ricorda che ci ha parlato (e salva, se è la prima volta). */
	void SegnaParlato(FName IdDialogo);

	/** Vero se questo personaggio ha quel segno in quel dialogo. */
	bool HaSegno(FName IdDialogo, FName Segno) const;

	/** Gli dà il segno (e salva, se è nuovo). */
	void MettiSegno(FName IdDialogo, FName Segno);

	/** Per le prove (console Valdorso.Dialoghi.Dimentica): i PNG lo salutano di nuovo come la prima volta. */
	void DimenticaDialoghi();

	// --- (14/11, v0.2.4) Sul server: la fiducia dei PNG e i fatti che la valle ricorda (ValdorsoFiducia.h) ----------
	// Stanno nel file del personaggio come la memoria dei dialoghi. Senza un personaggio salvato (l'accesso libero
	// dell'editor) valgono lo stesso, solo nella memoria del server.

	/** Quanto si fida di lui il PNG di quel dialogo (-100..100). */
	int32 Fiducia(FName IdDialogo) const;

	/** Cambia la fiducia (e salva, se e' cambiata). */
	void CambiaFiducia(FName IdDialogo, int32 Delta);

	/** Ha parlato con quel PNG: la prima volta del giorno la fiducia sale un poco. */
	void Chiacchiera(FName IdDialogo);

	/** Un fatto da ricordare (lo annota il registro eventi: ValdorsoFiducia::DopoAnnota). */
	void AnnotaFatto(const FString& Tipo, int64 Quando);

	/** Il fatto di cui quel PNG parlerebbe adesso (falso se non ne ha). */
	bool FattoDaRicordare(FName IdDialogo, int64 Adesso, FValdorsoFatto& OutFatto) const;

	/** Quel PNG ne ha parlato. */
	void SegnaRicordato(FName IdDialogo, int64 Quando);

	// --- (14/11, v0.2.4) Le storie del Battito (ValdorsoBattito.h): com'e' andata, nel file del personaggio ----------

	/** Sul server: le storie del Battito di questo colono. */
	const TArray<FValdorsoStoriaViva>& StorieVive() const { return Storie; }

	/** Sul server: una storia nuova o cambiata (e si salva il personaggio). */
	void CambiaStoria(const FValdorsoStoriaViva& Nuova);

	/** Per le prove (console Valdorso.Battito.Dimentica): le storie ripartono da capo. */
	void DimenticaStorie();

	/** Sul server: la scheda Storie, come la compone il Battito (va al PC solo se e' cambiata). */
	void ImpostaPagineStorie(const TArray<FValdorsoStoriaNelTaccuino>& Nuove);

	/** Sul PC: la scheda Storie. */
	const TArray<FValdorsoStoriaNelTaccuino>& PagineDelleStorie() const { return PagineStorie; }

	/** Dal server: la finestra di una storia, al centro dello schermo (si apre appena non c'e' un'altra schermata). */
	UFUNCTION(Client, Reliable)
	void ClientFinestraStoria(const FValdorsoFinestraStoria& Finestra);

	/** La risposta alla finestra: 0 Accetta, 1 Rifiuta, 2 Ci penso. */
	UFUNCTION(Server, Reliable)
	void ServerRispostaStoria(const FString& Id, uint8 Cosa);

	// --- Sul PC del giocatore -----------------------------------------------------------------------

	/** I luoghi scoperti (arrivano dal server). */
	const TArray<FValdorsoLuogoScoperto>& LuoghiScoperti() const { return Luoghi; }

	/** I titoli delle iscrizioni lette, dalla più vecchia alla più nuova. */
	const TArray<FString>& TitoliIscrizioni() const { return Titoli; }

	/** Il testo di un'iscrizione; nullptr finché non è arrivato (intanto lo chiede al server). */
	const TArray<FString>* RigheIscrizione(int32 Indice);

	/** Cresce a ogni cambiamento del taccuino su questo PC: la schermata lo guarda per rifarsi. */
	int32 Cambiamenti() const { return NumeroCambiamenti; }

	/** Il testo di un'iscrizione, chiesto dal taccuino aperto. */
	UFUNCTION(Server, Reliable)
	void ServerChiediIscrizione(int32 Indice);

	UFUNCTION(Client, Reliable)
	void ClientIscrizione(int32 Indice, int32 RevisioneTesto, const TArray<FString>& Righe);

	/** La schermata aperta (solo sul PC del giocatore). */
	TSharedPtr<SValdorsoTaccuino> Schermata;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Server: guarda se il colono è entrato in un luogo nuovo. */
	void ControllaLuoghi();
	/** Server: il taccuino è cambiato; si salva il personaggio (se ha un account). */
	void Salva();
	/** Server: rifà i titoli da mandare e cambia la revisione (così il PC rilegge i testi). */
	void AggiornaTitoli();
	/** PC: la bussola in alto sullo schermo. */
	void MettiBussola();
	void TogliBussola();

	UFUNCTION()
	void OnRep_Taccuino();

	/** I luoghi scoperti: sul server sono quelli veri, al giocatore arrivano uguali. */
	UPROPERTY(ReplicatedUsing = OnRep_Taccuino)
	TArray<FValdorsoLuogoScoperto> Luoghi;

	UPROPERTY(ReplicatedUsing = OnRep_Taccuino)
	TArray<FString> Titoli;

	/** Cresce a ogni cambiamento delle iscrizioni (anche solo del testo). */
	UPROPERTY(ReplicatedUsing = OnRep_Taccuino)
	int32 Revisione = 0;

	/** Solo sul server: le iscrizioni con il testo. */
	TArray<FValdorsoIscrizioneLetta> Iscrizioni;

	/** (08/11) Solo sul server: la memoria dei dialoghi (non va in rete: il PC non ne ha bisogno). */
	TArray<FValdorsoRicordoDialogo> Ricordi;

	/** (14/11, v0.2.4) Solo sul server: i fatti che la valle ricorda. */
	TArray<FValdorsoFatto> Fatti;

	/** (14/11, v0.2.4) Solo sul server: le storie del Battito. */
	TArray<FValdorsoStoriaViva> Storie;

	/** (14/11) Sul PC: le finestre delle storie arrivate e non ancora mostrate. */
	TArray<FValdorsoFinestraStoria> FinestreInAttesa;

	/** (14/11) La scheda Storie: solo al giocatore che ne e' il padrone. */
	UPROPERTY(ReplicatedUsing = OnRep_PagineStorie)
	TArray<FValdorsoStoriaNelTaccuino> PagineStorie;

	UFUNCTION()
	void OnRep_PagineStorie();

	/** (14/11) Un lavoro sulla memoria dei dialoghi e sui fatti, con le regole di ValdorsoFiducia: salva se cambia. */
	void CambiaMemoria(TFunctionRef<bool(FValdorsoTaccuino&)> Cosa);

	/** (08/11) Solo sul server: vero dopo Carica (c'è un personaggio con il suo file). */
	bool bPersonaggio = false;

	/** Server: quando guardare di nuovo i luoghi; il limite alle richieste dei testi. */
	double ProssimoControllo = 0.0;
	double InizioRichieste = 0.0;
	int32 RichiesteFatte = 0;

	/** PC: i testi arrivati (indice -> revisione e righe) e quelli chiesti (indice -> quando: dopo 2 s si richiede). */
	struct FTestoRicevuto
	{
		int32 Revisione = -1;
		TArray<FString> Righe;
	};
	TMap<int32, FTestoRicevuto> TestiRicevuti;
	TMap<int32, double> TestiChiesti;
	int32 NumeroCambiamenti = 0;
	TSharedPtr<SWidget> Bussola;
};

namespace ValdorsoTaccuino
{
	/** Vero se su questo PC il taccuino è aperto. */
	VALDORSO_API bool Aperto(const APlayerController* Controllore);

	/** Apre il taccuino (se il colono c'è); falso se non si può. */
	VALDORSO_API bool Apri(APlayerController* Controllore);

	/** Lo chiude e torna al gioco. */
	VALDORSO_API void Chiudi(APlayerController* Controllore);

	/** Il tasto J: apre o chiude. */
	VALDORSO_API void Alterna(APlayerController* Controllore);

	/** Server: il giocatore di quel PlayerController ha letto una bacheca o un'iscrizione. */
	VALDORSO_API void AnnotaLettura(APlayerController* Controllore, const FString& Titolo, const TArray<FString>& Righe);
}
