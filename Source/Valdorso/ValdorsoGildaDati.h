// Valdorso - La Gilda e il Balivo: gli incarichi, i gradi e i proclami (v0.1.4, 10/11/2026). Scritto da Claude.
// Dal Design ("La bacheca della Gilda", come la vuole Fra) e dalla Roadmap (v0.1.4: "il villaggio che lavora").
//
// La bacheca della Gilda (sulla parete di fondo della Gilda): un foglio per incarico, con il titolo, chi lo chiede, cosa
// fare, la paga, la scadenza, i posti ("2 su 4") e il grado con il sigillo di ceralacca (Comune rosso, Bronzo, Argento,
// Oro). Vicino alla scadenza il foglio ingiallisce; quando i posti sono finiti sparisce.
//   1. E sulla bacheca: si leggono i fogli. "Stacca il foglio": e' una prenotazione (mezz'ora), il foglio va nello zaino.
//   2. Il foglio si porta a Lucia, al bancone: lei guarda il grado (per Argento e Oro vuole una cauzione, un decimo della
//      paga, che torna alla fine) e lo assegna. Solo da assegnato l'incarico conta (le prede, il luogo raggiunto).
//   3. Fatto il lavoro si torna da Lucia: per una consegna si lasciano le cose, per gli altri basta il foglio. Lei timbra
//      "compiuto" e il server paga (dalla cassa dell'incarico: Lucia le monete non le tocca) e da' i punti della Gilda.
//   Rinunciare a un incarico assegnato costa 2 punti (e la cauzione). Un incarico scaduto si perde senza punti in meno.
// Chi mette gli incarichi: gli abitanti (Bruno, Marta, Gaspare...: li paga la cassa del villaggio, che si riempie un
// poco ogni giorno; al massimo sei sulla bacheca) e lo staff (con la chat: /incarico, vedi ValdorsoGilda.h). Quelli dei
// giocatori (con la cauzione e la commissione) verranno dopo.
// I gradi: i punti della Gilda dicono quando si e' pronti (Bronzo da 10, Argento da 40, Oro da 120), ma il grado lo
// da' una persona (10/11, deciso da Fra): lo staff, a nome di Mastro Ettore, il capo della filiale (PNG), oppure il capo
// gilda dei giocatori, nominato dallo staff, che promuove fino all'Argento (l'Oro solo lo staff). Tutto nel registro
// eventi, che lo staff vede (finestra dello staff, F9: ValdorsoStaff.h). Niente esperienza.
// Le prove: la consegna (le cose nello zaino), la preda (l'ultimo colpo, AValdorsoAnimale::Muori), il luogo raggiunto
// (il raggio del luogo del taccuino).
//
// La bacheca del Balivo (fuori dalla casa del Balivo, sotto una tettoia): i proclami, solo da leggere. Li scrive lo staff
// (/proclama, /legge: una legge vale dal giorno dopo), firmati "Per ordine del Balivo".
//
// Qui solo i dati e le regole, senza mondo ne' rete (li provano i test Valdorso.Gilda.*). Il tempo e' in secondi Unix.

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoGildaDati.generated.h"

struct FRandomStream;

UENUM()
enum class EValdorsoTipoIncarico : uint8
{
	/** Uccidere delle bestie (l'Obiettivo e' la specie: Lupo, Cinghiale...). */
	Caccia,
	/** Portare a Lucia delle cose (l'Obiettivo e' l'oggetto: Rami_Secchi...). */
	Consegna,
	/** Arrivare in un luogo della valle (l'Obiettivo e' il nome del luogo del taccuino). */
	Raggiungi,
};

UENUM()
enum class EValdorsoGrado : uint8
{
	Comune,
	Bronzo,
	Argento,
	Oro,
};

UENUM()
enum class EValdorsoStatoIncarico : uint8
{
	/** Il foglio e' staccato e nello zaino: va portato a Lucia (entro la prenotazione). */
	Prenotato,
	/** Lucia l'ha assegnato: adesso il lavoro conta. */
	Assegnato,
};

/** Un incarico sulla bacheca della Gilda. */
USTRUCT()
struct FValdorsoIncarico
{
	GENERATED_BODY()

	/** Il numero del foglio (unico sul server, non torna mai). */
	UPROPERTY()
	int32 Numero = 0;

	UPROPERTY()
	FString Titolo;

	UPROPERTY()
	FString Testo;

	/** Chi lo chiede ("Bruno, il locandiere"). */
	UPROPERTY()
	FString Committente;

	UPROPERTY()
	EValdorsoTipoIncarico Tipo = EValdorsoTipoIncarico::Caccia;

	/** La specie, l'oggetto o il luogo. */
	UPROPERTY()
	FName Obiettivo;

	/** Quante prede o quante cose (1 per Raggiungi). */
	UPROPERTY()
	int32 Quanti = 1;

	/** Il grado che serve per prenderlo. */
	UPROPERTY()
	EValdorsoGrado Grado = EValdorsoGrado::Comune;

	/** Gli Aureus per ognuno che lo compie, e i punti della Gilda. */
	UPROPERTY()
	int32 Monete = 0;

	UPROPERTY()
	int32 Punti = 1;

	/** Quanti coloni lo possono fare; Presi = quanti l'hanno preso (prenotato, assegnato o compiuto). */
	UPROPERTY()
	int32 Posti = 1;

	UPROPERTY()
	int32 Presi = 0;

	UPROPERTY()
	int32 Compiuti = 0;

	/** Quando scade (secondi Unix). */
	UPROPERTY()
	int64 Scadenza = 0;

	/** Vero se l'hanno messo gli abitanti (lo paga la cassa del villaggio); falso = lo staff. */
	UPROPERTY()
	bool bAbitanti = true;

	/** Il modello degli abitanti da cui viene (per non metterne due uguali); vuoto per lo staff. */
	UPROPERTY()
	FName Modello;

	/** Chi dello staff l'ha scritto (il nome del personaggio; vuoto per gli abitanti). */
	UPROPERTY()
	FString Autore;
};

/** Un incarico preso da un colono (con la copia del foglio, che resta anche se la bacheca cambia). */
USTRUCT()
struct FValdorsoIncaricoPreso
{
	GENERATED_BODY()

	UPROPERTY()
	FValdorsoIncarico Foglio;

	UPROPERTY()
	EValdorsoStatoIncarico Stato = EValdorsoStatoIncarico::Prenotato;

	/** Le prede fatte o il luogo raggiunto (1). Per le consegne non serve: conta lo zaino. */
	UPROPERTY()
	int32 Progresso = 0;

	/** Fin quando vale la prenotazione (secondi Unix). */
	UPROPERTY()
	int64 ScadePrenotazione = 0;

	/** La cauzione data a Lucia (torna alla fine). */
	UPROPERTY()
	int32 Cauzione = 0;
};

/** La Gilda di un colono: si salva con il personaggio (FValdorsoPersonaggio::Gilda). */
USTRUCT()
struct FValdorsoGildaColono
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Punti = 0;

	/** (10/11) Il grado vero: lo da' lo staff o il capo gilda dei giocatori (Nomina), non i punti da soli. */
	UPROPERTY()
	EValdorsoGrado Grado = EValdorsoGrado::Comune;

	/** (10/11) Capo gilda dei giocatori: lo nomina lo staff; promuove chi ha i punti, fino all'Argento. */
	UPROPERTY()
	bool bCapoGilda = false;

	/** (10/11) 0 nei file di prima dei gradi dati dallo staff: allora il grado e' quello dei punti (una volta). */
	UPROPERTY()
	int32 VersioneGradi = 0;

	/** (10/11) Il grado per cui gli e' gia' stato detto "hai i punti" (per dirlo una volta sola). */
	UPROPERTY()
	EValdorsoGrado AvvisatoPronto = EValdorsoGrado::Comune;

	UPROPERTY()
	int32 Compiuti = 0;

	UPROPERTY()
	int32 Abbandonati = 0;

	UPROPERTY()
	TArray<FValdorsoIncaricoPreso> Presi;
};

/** Un proclama del Balivo. */
USTRUCT()
struct FValdorsoProclama
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Numero = 0;

	UPROPERTY()
	FString Testo;

	/** Chi dello staff l'ha scritto (resta nel file, non si vede). */
	UPROPERTY()
	FString Autore;

	UPROPERTY()
	int64 Scritto = 0;

	/** Una legge: vale da ValeDal (il giorno dopo). */
	UPROPERTY()
	bool bLegge = false;

	UPROPERTY()
	int64 ValeDal = 0;
};

/** Le bacheche della valle, come stanno nel file del server (Saved/Server/Bacheche.json). */
USTRUCT()
struct FValdorsoBachecheSalvate
{
	GENERATED_BODY()

	UPROPERTY()
	int32 VersioneSchema = 1;

	UPROPERTY()
	int32 ProssimoNumero = 1;

	UPROPERTY()
	int32 ProssimoProclama = 1;

	/** La cassa del villaggio (Aureus): paga gli incarichi degli abitanti. */
	UPROPERTY()
	int32 Cassa = 400;

	/** L'ultima volta che la cassa ha ricevuto la sua rendita (secondi Unix; 0 = mai). */
	UPROPERTY()
	int64 UltimaRendita = 0;

	UPROPERTY()
	TArray<FValdorsoIncarico> Incarichi;

	UPROPERTY()
	TArray<FValdorsoProclama> Proclami;
};

namespace ValdorsoGilda
{
	/** Quanti incarichi un colono tiene insieme. */
	inline constexpr int32 MassimoPresi = 3;
	/** Quanto vale la prenotazione di un foglio staccato (s). */
	inline constexpr int64 SecondiPrenotazione = 30 * 60;
	/** Quanti punti si perdono rinunciando a un incarico assegnato. */
	inline constexpr int32 PuntiRinuncia = 2;
	/** Al massimo tanti incarichi degli abitanti sulla bacheca, e tanti in tutto. */
	inline constexpr int32 MassimoAbitanti = 6;
	inline constexpr int32 MassimoIncarichi = 24;
	/** La cassa del villaggio: la rendita di ogni giorno e il massimo. */
	inline constexpr int32 RenditaAlGiorno = 150;
	inline constexpr int32 CassaMassima = 800;
	/** Il foglio ingiallisce quando mancano meno di tante ore. */
	inline constexpr int64 SecondiGiallo = 6 * 3600;
	/** Una legge vale dal giorno dopo. */
	inline constexpr int64 SecondiLegge = 24 * 3600;
	/** Al massimo tanti proclami (oltre si toglie il piu' vecchio), e tante lettere (la riga della chat ne ha 200, meno
	 *  il comando: un proclama piu' lungo si scrive in due). */
	inline constexpr int32 MassimoProclami = 20;
	inline constexpr int32 LettereProclama = 190;
	/** Da quanto lontano si lavora alla bacheca e al bancone (cm). */
	inline constexpr float Raggio = 400.f;
	/** Il segno della scelta del dialogo di Lucia che apre il bancone (FValdorsoNodoDialogo::Scelta, Evento). */
	inline const FName EventoBancone(TEXT("BanconeGilda"));
	/** (10/11) La risposta del dialogo di Mastro Ettore che dice a che punto e' il colono con i gradi. */
	inline const FName EventoGrado(TEXT("GradoGilda"));
	/** L'oggetto del foglio nello zaino. */
	inline const FName IdFoglio(TEXT("Foglio_Incarico"));

	/** Il grado che questi punti permettono (non quello vero: FValdorsoGildaColono::Grado), e i punti per un grado. */
	VALDORSO_API EValdorsoGrado GradoDi(int32 Punti);
	VALDORSO_API int32 PuntiPer(EValdorsoGrado Grado);
	VALDORSO_API FText NomeGrado(EValdorsoGrado Grado);

	/** (10/11) Il grado dopo quello del colono (Oro resta Oro), e se ha i punti per salirci. */
	VALDORSO_API EValdorsoGrado GradoDopo(EValdorsoGrado Grado);
	VALDORSO_API bool Pronto(const FValdorsoGildaColono& Colono);

	/** (10/11) Il grado piu' alto che da' il capo gilda dei giocatori. */
	inline constexpr EValdorsoGrado MassimoDelCapo = EValdorsoGrado::Argento;

	/**
	 * (10/11) Perche' Chi non puo' dare il grado Nuovo a Colono (vuoto se puo'). bStaff: lo staff da' (o toglie) qualunque
	 * grado, anche senza i punti. Il capo gilda dei giocatori (Capo, bCapoGilda) solo il grado dopo, a chi ha i punti,
	 * fino a MassimoDelCapo, e non a se stesso (bStessoColono).
	 */
	VALDORSO_API FText PercheNoNomina(const FValdorsoGildaColono& Colono, EValdorsoGrado Nuovo, bool bStaff, const FValdorsoGildaColono* Capo, bool bStessoColono);

	/** (10/11) Da' il grado (falso se e' gia' quello). */
	VALDORSO_API bool Nomina(FValdorsoGildaColono& Colono, EValdorsoGrado Nuovo);

	/** (10/11) Vero la prima volta che il colono ha i punti per il grado dopo (poi AvvisatoPronto lo ricorda). */
	VALDORSO_API bool DaAvvisarePronto(FValdorsoGildaColono& Colono);

	/** L'incarico con quel numero (nullptr se non c'e'). */
	VALDORSO_API FValdorsoIncarico* Trova(FValdorsoBachecheSalvate& Bacheche, int32 Numero);
	VALDORSO_API const FValdorsoIncarico* Trova(const FValdorsoBachecheSalvate& Bacheche, int32 Numero);
	VALDORSO_API FValdorsoIncaricoPreso* Preso(FValdorsoGildaColono& Colono, int32 Numero);
	VALDORSO_API const FValdorsoIncaricoPreso* Preso(const FValdorsoGildaColono& Colono, int32 Numero);

	/** Vero se il foglio sta sulla bacheca (non scaduto, con dei posti liberi). */
	VALDORSO_API bool SullaBacheca(const FValdorsoIncarico& Incarico, int64 Adesso);

	/** I fogli sulla bacheca, nell'ordine: prima il grado piu' basso, poi la scadenza piu' vicina. */
	VALDORSO_API TArray<FValdorsoIncarico> Visibili(const FValdorsoBachecheSalvate& Bacheche, int64 Adesso);

	/** Perche' questo colono non puo' staccare il foglio (vuoto se puo'). Lo zaino lo guarda chi lo chiama. */
	VALDORSO_API FText PercheNoStacca(const FValdorsoIncarico& Incarico, const FValdorsoGildaColono& Colono, int64 Adesso);

	/** Stacca il foglio: prenotazione e un posto in meno. Falso (e niente cambia) se non si puo'. */
	VALDORSO_API bool Stacca(FValdorsoBachecheSalvate& Bacheche, FValdorsoGildaColono& Colono, int32 Numero, int64 Adesso);

	/** La cauzione che Lucia chiede (Argento e Oro: un decimo della paga, almeno 1). */
	VALDORSO_API int32 Cauzione(const FValdorsoIncarico& Incarico);

	/** Lucia assegna: da Prenotato ad Assegnato, con la cauzione (le monete le toglie chi chiama). */
	VALDORSO_API FText PercheNoAssegna(const FValdorsoGildaColono& Colono, int32 Numero, int32 Monete, int64 Adesso);
	VALDORSO_API bool Assegna(FValdorsoGildaColono& Colono, int32 Numero, int32 Monete, int64 Adesso);

	/** Il lavoro: una preda di quella specie o un luogo raggiunto. Restituisce quanti incarichi sono avanzati. */
	VALDORSO_API int32 Avanza(FValdorsoGildaColono& Colono, EValdorsoTipoIncarico Tipo, FName Obiettivo, int32 Quanti = 1);

	/** Vero se l'incarico assegnato e' fatto (per la consegna: se nello zaino ci sono abbastanza cose, Nello). */
	VALDORSO_API bool Fatto(const FValdorsoIncaricoPreso& Preso, int32 Nello);

	/** "2 lupi su 3", "5 × Rami secchi", "Rovine degli Antichi: raggiunto". */
	VALDORSO_API FText TestoLavoro(const FValdorsoIncarico& Incarico);
	VALDORSO_API FText TestoProgresso(const FValdorsoIncaricoPreso& Preso, int32 Nello);

	/** "fra 2 giorni", "fra 5 ore", "fra 20 minuti", "scaduto". */
	VALDORSO_API FText TestoScadenza(int64 Scadenza, int64 Adesso);

	/** Lucia timbra "compiuto": via dagli incarichi del colono, i punti; OutPreso e' quello che c'era (la paga e la cauzione
	 *  le da' chi chiama). Sulla bacheca un compiuto in piu' (finiti i posti, il foglio si toglie). */
	VALDORSO_API bool Compi(FValdorsoBachecheSalvate& Bacheche, FValdorsoGildaColono& Colono, int32 Numero, FValdorsoIncaricoPreso& OutPreso);

	/** Rinuncia: il posto torna libero sulla bacheca; da assegnato costa PuntiRinuncia (e la cauzione resta alla Gilda). */
	VALDORSO_API bool Rinuncia(FValdorsoBachecheSalvate& Bacheche, FValdorsoGildaColono& Colono, int32 Numero);

	/** Le prenotazioni scadute e gli incarichi scaduti di un colono: via (senza punti in meno). Restituisce quelli tolti. */
	VALDORSO_API TArray<FValdorsoIncaricoPreso> Scadute(FValdorsoBachecheSalvate& Bacheche, FValdorsoGildaColono& Colono, int64 Adesso);

	/** Via dalla bacheca gli incarichi scaduti o finiti (quello che resta degli abitanti torna nella cassa). Quanti tolti. */
	VALDORSO_API int32 Pulisci(FValdorsoBachecheSalvate& Bacheche, int64 Adesso);

	/** La rendita della cassa del villaggio per i giorni passati (fino a CassaMassima). */
	VALDORSO_API void Rendita(FValdorsoBachecheSalvate& Bacheche, int64 Adesso);

	/** Gli abitanti mettono un incarico (se ce ne sono meno di MassimoAbitanti e la cassa basta). Vero se l'hanno messo. */
	VALDORSO_API bool Genera(FValdorsoBachecheSalvate& Bacheche, int64 Adesso, FRandomStream& Caso);

	/** Quanti modelli di incarichi degli abitanti ci sono (per i test). */
	VALDORSO_API int32 NumeroModelli();

	/**
	 * Un incarico dello staff dalla riga della chat (dopo "/incarico "):
	 *   <caccia|consegna|raggiungi> <obiettivo> <quanti> <comune|bronzo|argento|oro> <monete> <posti> <giorni> <titolo> | <testo>
	 * Per esempio: "caccia Lupo 3 bronzo 40 2 3 I lupi del boschetto | Tre lupi, prima della luna nuova."
	 * L'obiettivo: la specie, l'Id dell'oggetto o il luogo (con _ al posto degli spazi). Falso con Errore se non va.
	 */
	VALDORSO_API bool DaComando(const FString& Riga, FValdorsoIncarico& Out, FString& Errore);

	/** Mette un incarico sulla bacheca (con il suo numero). Falso se la bacheca e' piena. */
	VALDORSO_API bool Metti(FValdorsoBachecheSalvate& Bacheche, FValdorsoIncarico Incarico, int64 Adesso);

	/** Un proclama nuovo (una legge vale dal giorno dopo); il piu' vecchio se ne va oltre MassimoProclami. */
	VALDORSO_API const FValdorsoProclama& Proclama(FValdorsoBachecheSalvate& Bacheche, const FString& Testo, const FString& Autore, bool bLegge, int64 Adesso);

	/** Le righe della bacheca del Balivo, il piu' nuovo per primo ("12 novembre 312 - Per ordine del Balivo: ..."). */
	VALDORSO_API TArray<FString> RigheProclami(const FValdorsoBachecheSalvate& Bacheche, int64 Adesso);

	/** Mette in ordine dati letti da un file (numeri, limiti). */
	VALDORSO_API void Sistema(FValdorsoBachecheSalvate& Bacheche);
	VALDORSO_API void Sistema(FValdorsoGildaColono& Colono);
}
