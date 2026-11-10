// Valdorso - La Gilda nel mondo: la bacheca degli incarichi, il bancone di Lucia, i proclami del Balivo (v0.1.4,
// 10/11/2026). Scritto da Claude. Le regole stanno in ValdorsoGildaDati.h, la schermata in SValdorsoGilda.h.
//
// Come funziona:
// - UValdorsoBacheche (sul server, uno per mondo) tiene le bacheche della valle: gli incarichi, la cassa del villaggio, i
//   proclami. Le scrive in Saved/Server/Bacheche.json (giocando da soli Saved/Solo, nell'editor Saved/Prova), con una
//   copia .bak. Ogni minuto: la rendita della cassa, via gli scaduti, e gli abitanti mettono un incarico nuovo se ce ne
//   sono pochi (all'avvio subito quattro).
// - UValdorsoGildaComponent sta sul PlayerController: la Gilda del colono (punti, incarichi presi) la tiene il server e
//   arriva solo a lui. Si salva con il personaggio (FValdorsoPersonaggio::Gilda).
// - La bacheca della Gilda (AValdorsoBacheca con Tipo Gilda): E apre i fogli (ClientBacheca); "Stacca il foglio" chiede
//   al server (ServerStacca), che guarda che il colono sia davvero alla bacheca, il grado, i posti e lo zaino.
// - Lucia (il PNG con il dialogo "Lucia"): la risposta "Ho un foglio della Gilda" ha l'evento BanconeGilda
//   (AValdorsoPNG::Scegli lo manda qui) e si apre il bancone (ClientBancone): assegna, consegna, rinuncia (ServerBancone).
// - Il lavoro: AValdorsoAnimale::Muori chiama Preda (l'ultimo che l'ha colpita); il tick guarda i luoghi raggiunti.
// - Lo staff scrive con la chat (ValdorsoChat.cpp passa qui le righe che cominciano con /): /proclama <testo>,
//   /legge <testo> (vale dal giorno dopo), /incarico ... (ValdorsoGilda::DaComando), /togliincarico <n>,
//   /togliproclama <n>, /cassa. Le righe dello staff non le sente nessuno.
// Comandi di console (sul server, o da soli): Valdorso.Gilda.Punti <n> [nome], Valdorso.Gilda.Genera,
// Valdorso.Gilda.Fatto [nome] (gli incarichi assegnati diventano fatti), Valdorso.Gilda.Elenco.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "Math/RandomStream.h"
#include "ValdorsoGildaDati.h"
#include "ValdorsoGilda.generated.h"

class APlayerController;
class SValdorsoGilda;

/** Le bacheche della valle, sul server. */
UCLASS()
class VALDORSO_API UValdorsoBacheche : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Quelle del mondo di Contesto (nullptr sul PC di chi gioca con un server, o fuori dal gioco). */
	static UValdorsoBacheche* Di(const UObject* Contesto);

	/** I dati (letti dal file la prima volta). */
	FValdorsoBachecheSalvate& Dati();

	/** Qualcosa e' cambiato: si salva fra poco. */
	void Segna();

	/** L'ora del server (secondi Unix). */
	static int64 Adesso();

	/** Una riga dello staff dalla chat (comincia con /): vero se era un comando delle bacheche (Risposta: cosa dire). */
	bool ComandoStaff(APlayerController* Chi, const FString& Riga, FString& OutRisposta);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void Carica();
	void Salva();
	void Minuto();
	FString File() const;

	FValdorsoBachecheSalvate Bacheche;
	/** Vero nella valle (sul server, o da soli): nel mondo del menu le bacheche non ci sono. */
	bool bAttive = false;
	bool bCaricate = false;
	bool bDaSalvare = false;
	FTimerHandle Orologio;
	FRandomStream Caso;
	/** Quante volte ha battuto l'orologio (ogni 10 secondi). */
	int32 Battiti = 0;
};

/** La Gilda di un colono (sul suo PlayerController). */
UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoGildaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoGildaComponent();

	static UValdorsoGildaComponent* Di(const APlayerController* Controllore);

	/** La Gilda del colono (sul server quella vera, sul PC quella arrivata). */
	const FValdorsoGildaColono& Contenuto() const { return Gilda; }

	/** Cresce a ogni cambiamento (sul PC): la schermata la guarda. */
	int32 Cambiamenti() const { return NumeroCambiamenti; }

	/** Cosa si legge sul foglio con quel numero (nello zaino, clic destro). */
	FText LetturaFoglio(int32 Numero) const;

	// --- Sul server ---------------------------------------------------------------------------------

	/** Il personaggio entra (dopo lo zaino: i fogli nello zaino si rimettono d'accordo con gli incarichi). */
	void Carica(const FValdorsoGildaColono& Salvato);
	FValdorsoGildaColono Dati() const { return Gilda; }

	/** E sulla bacheca della Gilda: si aprono i fogli su questo PC. */
	void ApriBacheca();

	/** Lucia apre il bancone (Chi e' Lucia: il colono deve restarle vicino). */
	void ApriBancone(AActor* Lucia);

	/** Una preda di quella specie uccisa da Cacciatore (il corpo del colono): avanzano le sue cacce. */
	static void Preda(AActor* Cacciatore, FName Specie);

	/** Il colono lascia la valle: i fogli solo prenotati tornano sulla bacheca (la prenotazione e' di mezz'ora, e da
	 *  fuori non scadrebbe). Quelli assegnati restano suoi. */
	void Esce();

	/** (10/11) Il grado, dato dallo staff (a nome di Mastro Ettore) o dal capo gilda dei giocatori Da: chi lo da'
	 *  (i controlli li fa chi chiama: ValdorsoGilda::PercheNoNomina). Finisce nel registro eventi. */
	void Nomina(EValdorsoGrado Nuovo, const FString& Da, bool bDalloStaff);

	/** (10/11) Capo gilda dei giocatori si' o no (lo decide lo staff). */
	void ImpostaCapo(bool bCapo, const FString& Da);

	/** (10/11) I fogli nello zaino tornano d'accordo con gli incarichi presi (dopo che lo staff ha svuotato lo zaino). */
	void RimettiFogli() { Riallinea(); }

	/** (10/11) Mastro Ettore dice a che punto e' il colono (la risposta del dialogo con l'evento GradoGilda). */
	void DimmiGrado();

	/** Per le prove (console): i punti, e gli incarichi assegnati fatti. */
	void ImpostaPunti(int32 Punti);
	void FaiTutto();

	// --- Dal PC del giocatore (il server controlla) ------------------------------------------------------

	UFUNCTION(Server, Reliable)
	void ServerStacca(int32 Numero);

	/** Al bancone: 0 = assegnami, 1 = compiuto (consegna), 2 = rinuncio. */
	UFUNCTION(Server, Reliable)
	void ServerBancone(int32 Numero, uint8 Azione);

	// --- Sul PC --------------------------------------------------------------------------------------

	UFUNCTION(Client, Reliable)
	void ClientBacheca(const TArray<FValdorsoIncarico>& NuoviFogli, int64 Ora);

	UFUNCTION(Client, Reliable)
	void ClientBancone(int64 Ora);

	/** I fogli della bacheca come sono arrivati l'ultima volta, e l'ora del server di adesso (circa). */
	const TArray<FValdorsoIncarico>& FogliVisti() const { return Fogli; }
	int64 OraServer() const;

	/** La schermata aperta (solo sul PC del giocatore, ValdorsoGildaSchermo). */
	TSharedPtr<SValdorsoGilda> Schermata;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Server: la Gilda e' cambiata: i fogli nello zaino d'accordo, e si salva fra poco. */
	void Cambiato();
	/** Server: i fogli nello zaino sono quelli degli incarichi presi (via gli altri, rimessi quelli che mancano). */
	void Riallinea();
	/** Server: il colono e' davvero vicino a una bacheca della Gilda, o a Lucia? */
	bool AllaBacheca() const;
	bool AlBancone() const;
	void Avvisa(const FText& Testo, float Secondi = 3.f) const;
	/** Server: (10/11) dice allo staff della Gilda e al capo gilda che questo colono e' pronto a salire. */
	void AvvisaChiNomina() const;
	void Annota(const FString& Cosa) const;

	UFUNCTION()
	void OnRep_Gilda();

	UPROPERTY(ReplicatedUsing = OnRep_Gilda)
	FValdorsoGildaColono Gilda;

	/** Server: Lucia, quando il bancone e' aperto. */
	TWeakObjectPtr<AActor> Bancone;
	bool bPersonaggio = false;
	bool bDaSalvare = false;
	double SalvaDopo = 0.0;
	double ProssimoControllo = 0.0;
	double UltimaRichiesta = 0.0;
	/** PC: il bancone arrivato mentre il dialogo con Lucia era ancora aperto (si riprova fino a quest'ora). */
	double BanconeInAttesaFino = 0.0;

	/** PC: i fogli arrivati e quanto l'orologio del server e' avanti rispetto a questo PC. */
	TArray<FValdorsoIncarico> Fogli;
	int64 Scarto = 0;
	int32 NumeroCambiamenti = 0;
};
