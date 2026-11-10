// Valdorso - Lo zaino, con il peso e le monete (v0.1.4 "Pericoli", seconda parte). Scritto da Claude il 08/11/2026.
//
// Come funziona:
// - UValdorsoZainoComponent sta sul PlayerController (uno per giocatore). Lo zaino vero lo tiene il server: il PC
//   chiede (usa, sposta, getta, metti nella barra) e il server controlla e decide; al giocatore arriva lo zaino come
//   è (solo a lui, COND_OwnerOnly). Le regole stanno in ValdorsoZainoDati.h, gli oggetti in ValdorsoOggetti.h.
// - Lo zaino si salva con il personaggio (FValdorsoPersonaggio::Zaino): il PlayerController lo dà al componente
//   quando il personaggio entra (Carica, con il suo Registro) e lo riprende quando salva (Dati). Chi entra la prima
//   volta riceve quello che ha portato con sé: la tessera, pane, mele, la borraccia, coltello, acciarino, mantello,
//   l'attrezzo del vecchio mestiere, il ricordo e gli Aureus (di più per chi è venuto per un debito). Chi entra con
//   l'accesso libero dell'editor riceve le cose di un colono qualunque (e non si salva niente).
// - Il peso: oltre 30 kg non si corre, oltre 40 si cammina piano (lo applicano allo stesso modo il server e il PC,
//   UValdorsoMovementComponent::FattoreCarico). Il mantello nello zaino dimezza la pioggia (ValdorsoBagnato.h).
// - Usare: il pane e le mele si mangiano (salute e stamina), la borraccia si beve a sorsi (si riempie da sola
//   entrando nell'acqua del lago o del fiume), le erbe di Nonna Edda curano, la tessera, il quaderno e il libro si
//   leggono. Un oggetto ogni secondo, e non da caduti.
// - La barra delle azioni (tasti 1-8, ValdorsoBarre.h) usa gli oggetti messi nei suoi posti; si sceglie nello zaino.
// - Lo zaino (tasto I): la pergamena con la griglia 6 x 4, il peso, gli Aureus e gli 8 tasti (SValdorsoZaino.h).
// - (08/11) I doni dei PNG: Bruno alla prima scodella (il pane nero), Nonna Edda a chi le porta rispetto (le erbe).
// Comandi di console (sul server, o giocando da soli): Valdorso.Zaino.Dai <Id> [quanti], Valdorso.Zaino.Monete <n>,
// Valdorso.Zaino.Svuota, Valdorso.Zaino.Elenco (gli Id degli oggetti).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoZainoDati.h"
#include "ValdorsoTessera.h"
#include "ValdorsoZaino.generated.h"

class APlayerController;
class SValdorsoZaino;
class SValdorsoTessera;
class SValdorsoBottega;
struct FValdorsoRegistro;

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoZainoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoZainoComponent();

	/** Lo zaino del giocatore di quel PlayerController (nullptr se non ce l'ha). */
	static UValdorsoZainoComponent* Di(const APlayerController* Controllore);

	/** Lo zaino com'è adesso (sul server quello vero, sul PC quello arrivato dal server). */
	const FValdorsoZaino& Contenuto() const { return Zaino; }

	/** Il peso di adesso, in kg. */
	float Peso() const;

	/** Cresce a ogni cambiamento dello zaino su questo PC: la schermata e la barra lo guardano per rifarsi. */
	int32 Cambiamenti() const { return NumeroCambiamenti; }

	// --- Sul server ---------------------------------------------------------------------------------

	/** Il personaggio entra: il suo zaino letto dal file (e, la prima volta, quello che porta dall'arrivo). */
	void Carica(const FValdorsoZaino& Salvato, const FValdorsoRegistro* Registro);

	/** Lo zaino com'è adesso, da salvare nel file del personaggio. */
	FValdorsoZaino Dati() const { return Zaino; }

	/** Dà oggetti al colono (con l'avviso "Ricevi: ..."); restituisce quanti ne sono entrati. */
	int32 Dai(FName Id, int32 Quantita, bool bAvvisa = true);

	/** Per le prove (console): cambia gli Aureus, svuota lo zaino. */
	void ImpostaMonete(int32 Monete);
	void Svuota();

	/** (09/11) La tessera del personaggio, come si vede (la mette il PlayerController quando il personaggio entra). */
	void ImpostaTessera(const FValdorsoTesseraVista& Vista);

	/** (09/11) Mostra la tessera a chi si ha davanti (un altro colono entro 3,5 m): a lui si apre. */
	UFUNCTION(Server, Reliable)
	void ServerMostraTessera();

	/** (09/11) Apre una tessera su questo PC: la propria, o quella che un altro colono (Colono) ti mostra. */
	UFUNCTION(Client, Reliable)
	void ClientTessera(const FValdorsoTesseraVista& Vista, APawn* Colono, bool bPropria);

	// --- Dal PC del giocatore (il server controlla) ------------------------------------------------------

	UFUNCTION(Server, Reliable)
	void ServerUsa(int32 Posto);

	/** Il tasto della barra delle azioni (0..7): usa il suo oggetto. */
	UFUNCTION(Server, Reliable)
	void ServerUsaAzione(int32 Tasto);

	UFUNCTION(Server, Reliable)
	void ServerSposta(int32 Da, int32 A);

	UFUNCTION(Server, Reliable)
	void ServerGetta(int32 Posto);

	/** Mette l'oggetto di un posto dello zaino in un tasto della barra (Posto INDEX_NONE = vuota il tasto). */
	UFUNCTION(Server, Reliable)
	void ServerAssegna(int32 Tasto, int32 Posto);

	/** (10/11) Il colono e' a un banco (o a un falo'): si apre la sua bottega su questo PC (ValdorsoBotteghe.h). */
	UFUNCTION(Client, Reliable)
	void ClientApriBottega(uint8 Banco);

	/** (10/11) Fa una ricetta di bottega (cuocere, conciare, comprare, vendere): il server controlla il banco e lo zaino. */
	UFUNCTION(Server, Reliable)
	void ServerRicetta(FName Ricetta);

	/** (10/11) La bottega aperta (solo sul PC del giocatore). */
	TSharedPtr<SValdorsoBottega> SchermataBottega;

	/** La schermata aperta (solo sul PC del giocatore). */
	TSharedPtr<SValdorsoZaino> Schermata;

	/** (09/11) La tessera aperta (solo sul PC del giocatore, ValdorsoTesseraSchermo). */
	TSharedPtr<SValdorsoTessera> SchermataTessera;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Server: usa l'oggetto di un posto (le regole: mangiare, bere, curarsi, leggere). */
	void Usa(int32 Posto);
	/** Server: lo zaino è cambiato (al PC arriva da solo); il file si salva fra poco, una volta sola. */
	void Cambiato();
	/** Server: (09/11) il colono ha in mano quello che dice lo zaino (ValdorsoArmi.h). */
	void ArmiInMano();
	/** Server: troppe richieste dal PC in un secondo? (chi ne manda di più sta barando) */
	bool TroppeRichieste();
	/** Server e PC: il peso sul movimento del colono. */
	void ApplicaPeso();
	/** Server: la borraccia si riempie nell'acqua. */
	void ControllaAcqua();
	/** Server: senza personaggio (l'accesso libero dell'editor) una tessera di prova, con il nome di chi gioca. */
	void TesseraDiProva();
	/** Server: un avviso breve in alto sullo schermo del giocatore. */
	void Avvisa(const FText& Testo, float Secondi = 2.5f);

	UFUNCTION()
	void OnRep_Zaino();

	UPROPERTY(ReplicatedUsing = OnRep_Zaino)
	FValdorsoZaino Zaino;

	/** Server: la tessera di chi gioca (vuota finché il personaggio non entra: allora una di prova). */
	FValdorsoTesseraVista Tessera;
	double ProssimaTesseraMostrata = 0.0;

	/** Server: vero dopo Carica (c'è un personaggio con il suo file). */
	bool bPersonaggio = false;
	bool bDaSalvare = false;
	double SalvaDopo = 0.0;
	double ProssimoUso = 0.0;
	double ProssimoControllo = 0.0;
	double InizioRichieste = 0.0;
	int32 RichiesteFatte = 0;
	int32 NumeroCambiamenti = 0;
};

namespace ValdorsoZainoSchermo
{
	/** Vero se su questo PC lo zaino è aperto. */
	VALDORSO_API bool Aperto(const APlayerController* Controllore);

	/** Apre lo zaino (se il colono c'è); falso se non si può. */
	VALDORSO_API bool Apri(APlayerController* Controllore);

	/** Lo chiude e torna al gioco. */
	VALDORSO_API void Chiudi(APlayerController* Controllore);

	/** Il tasto I: apre o chiude. */
	VALDORSO_API void Alterna(APlayerController* Controllore);
}
