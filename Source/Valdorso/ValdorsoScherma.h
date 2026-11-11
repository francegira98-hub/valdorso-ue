// Valdorso - La scherma: le guardie, le tecniche alle soglie e i talenti (v0.1.5 "Crescere", 11/11/2026). Scritto da
// Claude. Dalla Roadmap ("Le guardie della scherma italiana e le prime tecniche: tre guardie per le armi, tecniche che si
// aprono alle soglie, nella barra 1-8") e dal Design ("ogni 25 livelli si sblocca una tecnica attiva o un talento").
// Fra il 10/11: le tecniche nella barra 1-8; le guardie "mi fido di te, sii coerente".
//
// Come funziona:
// - Le guardie (dal Flos Duellatorum di Fiore dei Liberi, 1409): ogni famiglia d'arma ne ha tre, con i nomi veri del
//   libro: una ALTA (colpi forti, si para poco: per la spada la Posta di Donna), una LUNGA (la punta tiene lontano: la
//   Posta Longa) e una BASSA (si aspetta e si risponde: la Tutta Porta di Ferro). Si cambia con G (o la croce in su
//   del controller): ci vuole un attimo, meno quanto piu' l'arma e' evoluta. Ognuna batte un'altra, come nel libro:
//   l'alta rompe la bassa (il colpo dall'alto la schiaccia), la bassa passa sotto la lunga (entra sotto la punta), la
//   lunga ferma l'alta (la punta arriva prima). Chi guarda bene l'avversario (agganciato, sotto il rombo si legge la
//   sua guardia) sceglie quella giusta. Con lo scudo e a mani nude ci sono le guardie della lotta.
// - Le tecniche: alle soglie (quando l'abilita' si EVOLVE a 25, 50, 75 e 100) si apre una tecnica attiva o un talento.
//   Le attive entrano da sole nel primo posto libero della barra 1-8 (si spostano dalla scheda K) e si usano con il
//   loro tasto: costano stamina e hanno un tempo di ricarica (la barra lo mostra). I talenti valgono sempre.
// - Chi decide e' il server: il PC chiede (ServerCambiaGuardia, il tasto della barra), il server controlla l'arma in
//   mano, l'abilita', la stamina, la ricarica, e fa il colpo con ValdorsoCombattimento::Colpisci.
// - ValdorsoCombattimento::Colpisci chiama PrimaDelColpo (guardie, talenti, parata perfetta, la rocca, la furia, la
//   schivata) e DopoIlColpo (la risposta della guardia bassa, lo stordimento, il sanguinamento).
// Comandi di console: Valdorso.Scherma.Guardia <0 alta|1 lunga|2 bassa>, Valdorso.Scherma.Tecniche (l'elenco).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoScherma.generated.h"

class APawn;
class UAbilitySystemComponent;

/** La guardia: alta (colpi forti), lunga (la punta tiene lontano), bassa (si aspetta e si risponde). */
UENUM()
enum class EValdorsoGuardia : uint8
{
	Alta,
	Lunga,
	Bassa,
};

/** Che cosa fa una tecnica. */
enum class EValdorsoTipoTecnica : uint8
{
	/** Un colpo (o piu'), con il suo modo: affondo, giro, spallata... (va nella barra). */
	Colpo,
	/** Uno stato per qualche secondo: la parata perfetta, la furia, la rocca (va nella barra). */
	Stato,
	/** Un talento che vale sempre (non va nella barra). */
	Talento,
	/** Arriva piu' avanti (si vede nella scheda, ancora in grigio). */
	InArrivo,
};

/** Una tecnica (o un talento) di un'abilita', a una soglia. */
struct FValdorsoTecnica
{
	/** "Tecnica.Affondo": e' anche l'Id nella barra delle azioni. */
	FName Id;
	/** L'abilita' (ValdorsoAbilitaDati.h): "Spade". */
	FName Abilita;
	/** A quale evoluzione si apre: 1 (soglia 25), 2 (50), 3 (75), 4 (100). */
	int32 Livello = 1;
	EValdorsoTipoTecnica Tipo = EValdorsoTipoTecnica::Colpo;
	const TCHAR* Nome = TEXT("");
	/** Due o tre lettere per il posto della barra ("Aff"). */
	const TCHAR* Sigla = TEXT("");
	const TCHAR* Descrizione = TEXT("");

	// --- per i colpi: rispetto a un colpo normale con l'arma in mano
	float Danno = 1.f;
	float Portata = 1.f;
	/** Equilibrio tolto in piu' (oltre a quello dell'arma). */
	float Equilibrio = 0.f;
	/** Uno slancio in avanti all'inizio (cm/s; 0 = fermo). */
	float Slancio = 0.f;
	/** Colpisce tutti intorno, entro questo raggio (cm; 0 = solo davanti). */
	float Giro = 0.f;
	/** Quanti colpi, e a che distanza l'uno dall'altro (s). */
	int32 Colpi = 1;
	float Intervallo = 0.3f;
	/** Il colpo passa sopra lo scudo (lo spacca o lo scavalca). */
	bool bSopraLoScudo = false;
	/** Quanti secondi resta fermo chi e' colpito. */
	float Stordisce = 0.f;
	/** Il sangue: quanta salute toglie in tutto, in tre secondi. */
	float Sanguina = 0.f;
	/** Spinge piu' del solito (x). */
	float Spinta = 1.f;

	// --- per gli stati
	float Durata = 0.f;

	// --- per tutte quelle attive
	float Stamina = 15.f;
	float Ricarica = 6.f;
	/** Serve lo scudo nella sinistra (le tecniche dello Scudo). */
	bool bScudo = false;

	bool Attiva() const { return Tipo == EValdorsoTipoTecnica::Colpo || Tipo == EValdorsoTipoTecnica::Stato; }
};

/** Una ricarica in corso (arriva solo al colono, per la barra): fino a quando, nel tempo del server. */
USTRUCT()
struct FValdorsoRicaricaTecnica
{
	GENERATED_BODY()

	UPROPERTY()
	FName Id;

	UPROPERTY()
	float Fine = 0.f;

	UPROPERTY()
	float Durata = 0.f;
};

/** Cosa ha deciso PrimaDelColpo (Colpisci lo applica). */
struct FValdorsoEsitoScherma
{
	/** Il colpo non arriva: schivato (la schivata appena partita) o fermato (la parata perfetta). */
	bool bAnnullato = false;
	/** Lo scudo del colpito para anche di lato (cos dell'angolo: 0,34 davanti, -0,2 anche di lato). */
	float AngoloScudo = 0.34f;
	/** Quanto conta lo scudo del colpito (1 tutto, 0 niente: il colpo gli passa sopra). */
	float ScudoConta = 1.f;
	/** Quanto spinge (x). */
	float Spinta = 1.f;
	/** Equilibrio che perde chi ha colpito (la parata perfetta). */
	float EquilibrioAChiColpisce = 0.f;
	/** Equilibrio che perde chi ha colpito se lo scudo del colpito ha parato (Scudo 50: lo scudo che risponde). */
	float EquilibrioSeParato = 0.f;
	/** Cosa leggono i due (vuoto = niente). */
	FText AvvisoChiColpisce;
	FText AvvisoColpito;
	/** Chi e' colpito cresce in questa abilita' (la parata perfetta: l'arma; la schivata: Atletica). */
	FName CresceIlColpito;
};

namespace ValdorsoScherma
{
	/** Le tecniche e i talenti di tutte le abilita', nell'ordine della scheda. */
	VALDORSO_API const TArray<FValdorsoTecnica>& Tecniche();
	VALDORSO_API const FValdorsoTecnica* Tecnica(FName Id);
	/** La tecnica di un'abilita' a un livello (1..4); nullptr se non c'e'. */
	VALDORSO_API const FValdorsoTecnica* TecnicaDi(FName Abilita, int32 Livello);
	/** Vero se l'Id e' di una tecnica attiva (va nella barra). */
	VALDORSO_API bool DaBarra(FName Id);

	/** Il nome della guardia per la famiglia dell'arma ("Spade", "Pugnali", "Asce", "Mazze", "Aste", "Lotta"). */
	VALDORSO_API FText NomeGuardia(FName Famiglia, EValdorsoGuardia Guardia);
	/** Cosa fa, in una riga (per la scheda e l'avviso). */
	VALDORSO_API FText SpiegaGuardia(EValdorsoGuardia Guardia);

	/** Chi vince fra due guardie: +1 la prima (chi colpisce), -1 la seconda, 0 pari. Alta > Bassa > Lunga > Alta. */
	VALDORSO_API int32 Vantaggio(EValdorsoGuardia ChiColpisce, EValdorsoGuardia Colpito);

	/** Quanto rende la guardia di chi colpisce (danno, equilibrio tolto, portata) e quanto difende quella del colpito. */
	VALDORSO_API float DannoGuardia(EValdorsoGuardia Guardia);
	VALDORSO_API float EquilibrioGuardia(EValdorsoGuardia Guardia);
	VALDORSO_API float PortataGuardia(EValdorsoGuardia Guardia);
	VALDORSO_API float StaminaGuardia(EValdorsoGuardia Guardia);
	/** Quanto del colpo che arriva da davanti si ferma con questa guardia (0..1, la bassa). */
	VALDORSO_API float DifesaGuardia(EValdorsoGuardia Guardia);

	/** Quanto ci vuole per cambiare guardia (s): meno con l'arma evoluta (Evoluzioni 0..4). */
	VALDORSO_API float SecondiCambio(int32 Evoluzioni);

	/** Il maestro di guardie (Spade 75): le guardie contano una volta e mezzo. Valore 1 + (x - 1) * Forza. */
	VALDORSO_API float Rafforza(float Valore, float Forza);

	/** La famiglia delle guardie di chi ha quell'arma (NAME_None a mani nude: la Lotta). */
	VALDORSO_API FName FamigliaGuardie(FName IdArma);

	/**
	 * Le evoluzioni di un'abilita' del colono di quel corpo (0 se non e' un colono o non si sa). Sul server di tutti, sul
	 * PC solo del proprio.
	 */
	VALDORSO_API int32 Evoluzioni(const AActor* Corpo, FName Abilita);

	/** Vero se il colono di quel corpo ha la tecnica (o il talento) di quell'abilita' a quel livello. */
	VALDORSO_API bool Talento(const AActor* Corpo, FName Abilita, int32 Livello);

	/** Solo sul server, dentro ValdorsoCombattimento::Colpisci: le guardie, i talenti, gli stati. Cambia Danno ed Equilibrio. */
	VALDORSO_API FValdorsoEsitoScherma PrimaDelColpo(AActor* Da, AActor* A, EValdorsoTipoColpo Tipo, bool bDaDavanti, float& Danno, float& Equilibrio);

	/** Solo sul server, dopo un colpo arrivato: la risposta della guardia bassa, e quello che porta la tecnica in corso. */
	VALDORSO_API void DopoIlColpo(AActor* Da, AActor* A, bool bDaDavanti, float DannoFatto);

	/** Il testo della barra per una tecnica: la sigla (vuota se non e' una tecnica). */
	VALDORSO_API FString Sigla(FName Id);

	/**
	 * Solo sul server: un'abilita' del colono di quel PlayerController si e' appena evoluta (Evoluzione 1..4). Se si apre
	 * una tecnica attiva la mette nel primo posto libero della barra; in ogni caso lo dice al colono.
	 */
	VALDORSO_API void DopoEvoluzione(APlayerController* Controllore, FName Abilita, int32 Evoluzione);

	/** Le tecniche (attive e talenti) che il colono di quel corpo sa gia' fare, nell'ordine della tabella. */
	VALDORSO_API TArray<const FValdorsoTecnica*> Conosciute(const AActor* Corpo);
}

/**
 * Sul colono (AValdorsoCharacter): la guardia (la vedono tutti), le ricariche delle tecniche (solo il colono), gli stati a
 * tempo (solo il server).
 */
UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoSchermaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoSchermaComponent();

	static UValdorsoSchermaComponent* Di(const AActor* Chi);

	EValdorsoGuardia GetGuardia() const { return Guardia; }

	/** Vero mentre si cambia guardia (lato server: non vale nessuna guardia). */
	bool InCambio() const;

	/** Il tasto G: la guardia dopo (Alta, Lunga, Bassa, Alta...). Sul PC del colono. */
	void GuardiaDopo();

	UFUNCTION(Server, Reliable)
	void ServerCambiaGuardia(uint8 Nuova);

	/** Dalla scheda (K): mette una tecnica che si sa fare in un tasto della barra (Tasto -1 = il primo libero). */
	UFUNCTION(Server, Reliable)
	void ServerMettiInBarra(FName Id, int32 Tasto);

	/** Solo sul server: usa una tecnica attiva (dalla barra). Vero se e' partita. */
	bool UsaTecnica(FName Id);

	/** Quanto manca alla fine della ricarica di una tecnica (0..1 della ricarica; 0 = pronta). Sul PC del colono. */
	float Ricarica(FName Id) const;

	// --- Solo sul server: gli stati a tempo ---------------------------------------------------------------------
	bool InParataPerfetta() const;
	bool InFuria() const;
	bool InRocca() const;
	/** Il colpo dopo una parata della guardia bassa vale di piu' (la risposta). */
	bool InRisposta() const;
	void ApriRisposta();
	void ChiudiRisposta();

	/** La tecnica che sta colpendo adesso (dentro UsaTecnica e i suoi colpi), nullptr fuori. */
	const FValdorsoTecnica* TecnicaInCorso() const { return InCorso; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Un colpo della tecnica (il primo subito, gli altri dopo Intervallo). */
	void Colpo(const FValdorsoTecnica* Quale, int32 Numero);
	void Avvisa(const FText& Testo, float Secondi = 2.5f) const;
	double Adesso() const;

	/** Su tutti i PC: l'animazione del colpo (il montaggio del pugno, piu' svelto o piu' lento). */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAnimazione(float Velocita);

	UFUNCTION()
	void OnRep_Guardia();

	UPROPERTY(ReplicatedUsing = OnRep_Guardia)
	EValdorsoGuardia Guardia = EValdorsoGuardia::Lunga;

	UPROPERTY(Replicated)
	TArray<FValdorsoRicaricaTecnica> Ricariche;

	// --- solo sul server
	double FineCambio = 0.0;
	double FineParata = 0.0;
	double FineFuria = 0.0;
	double FineRocca = 0.0;
	double FineRisposta = 0.0;
	double UltimaRichiesta = 0.0;
	const FValdorsoTecnica* InCorso = nullptr;
	TArray<FTimerHandle> Timer;
};
