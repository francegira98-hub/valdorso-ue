// Valdorso - Lo zaino: i dati e le regole (v0.1.4, seconda parte). Scritto da Claude il 08/11/2026.
//
// Lo zaino è una griglia di 6 x 4 posti: in ogni posto un oggetto del catalogo (ValdorsoOggetti.h), più d'uno se
// si impilano (il pane fino a 10). A parte: gli Aureus e gli 8 posti della barra delle azioni (che oggetto usa ogni
// tasto da 1 a 8: si salvano con lo zaino). Tutto si salva nel file del personaggio (FValdorsoPersonaggio::Zaino);
// nei file di prima manca, e chi entra la prima volta riceve quello che porta dall'arrivo (bPartenzaData).
// Qui solo le regole, senza mondo né rete: le provano i test (Valdorso.Zaino.*). Le usa UValdorsoZainoComponent.

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoZainoDati.generated.h"

struct FValdorsoRegistro;

/** Un posto dello zaino: quale oggetto e quanti (Id vuoto = posto libero). */
USTRUCT()
struct FValdorsoPostoZaino
{
	GENERATED_BODY()

	UPROPERTY()
	FName Id;

	UPROPERTY()
	int32 Quantita = 0;

	/** Le cariche (la borraccia: i sorsi rimasti). */
	UPROPERTY()
	int32 Cariche = 0;

	/** (10/11) Il numero scritto sull'oggetto: per il foglio di un incarico della Gilda, il numero dell'incarico. */
	UPROPERTY()
	int32 Numero = 0;

	/** (14/11, v0.2.3) Il grado (ValdorsoQualita.h): 0 Comune, 1 Buona, 2 Rara, 3 Epica, 4 Leggendaria. */
	UPROPERTY()
	uint8 Qualita = 0;

	/** (14/11, v0.2.3) Da dove viene ("Fattura di Marta", "Forgiata da Fra"); vuoto = niente di speciale. */
	UPROPERTY()
	FString Provenienza;

	/** (14/11, v0.2.3) Le armi e gli scudi: quanti colpi (o parate) hanno gia' preso (ValdorsoQualita::DurataMassima). */
	UPROPERTY()
	int32 Usura = 0;

	bool Vuoto() const { return Id.IsNone() || Quantita <= 0; }

	/** (14/11) Due posti con lo stesso oggetto si impilano solo se sono uguali anche nel grado e nella provenienza. */
	bool StessaFattura(const FValdorsoPostoZaino& Altro) const
	{
		return Qualita == Altro.Qualita && Provenienza == Altro.Provenienza && Usura == Altro.Usura;
	}

	bool operator==(const FValdorsoPostoZaino& Altro) const
	{
		return Id == Altro.Id && Quantita == Altro.Quantita && Cariche == Altro.Cariche && Numero == Altro.Numero
			&& Qualita == Altro.Qualita && Provenienza == Altro.Provenienza && Usura == Altro.Usura;
	}
};

/** Lo zaino di un personaggio, così come sta nel suo file (e come arriva al suo PC). */
USTRUCT()
struct FValdorsoZaino
{
	GENERATED_BODY()

	/** I posti della griglia, riga per riga (sempre ValdorsoZaino::NumeroPosti dopo Sistema). */
	UPROPERTY()
	TArray<FValdorsoPostoZaino> Posti;

	/** Gli Aureus. */
	UPROPERTY()
	int32 Monete = 0;

	/** La barra delle azioni: l'oggetto di ogni tasto da 1 a 8 (NAME_None = vuoto). */
	UPROPERTY()
	TArray<FName> Azioni;

	/** Vero quando il colono ha già ricevuto quello che porta dall'arrivo. */
	UPROPERTY()
	bool bPartenzaData = false;

	/** (08/11) Le cose aggiunte alla partenza dopo che il colono era già arrivato (ValdorsoZaino::Aggiorna): 0 = nessuna,
	 *  1 = la mappa della valle. */
	UPROPERTY()
	int32 Aggiornamenti = 0;

	/** (09/11, v0.1.4) Quello che si impugna: nella destra (un'arma) e nella sinistra (lo scudo). NAME_None = niente.
	 *  Si impugna solo quello che c'e' nello zaino (Sistema toglie dalle mani quello che non c'e' piu'). */
	UPROPERTY()
	FName Destra;

	UPROPERTY()
	FName Sinistra;

	/** (12/11, v0.2.0) Il baule del colono, da Bruno alla locanda (ValdorsoBaule.h): altri posti, che non pesano e non
	 *  si portano dietro. Nei file di prima manca: si legge vuoto (Sistema lo mette della misura giusta). */
	UPROPERTY()
	TArray<FValdorsoPostoZaino> Baule;
};

namespace ValdorsoZaino
{
	inline constexpr int32 Colonne = 6;
	inline constexpr int32 Righe = 4;
	inline constexpr int32 NumeroPosti = Colonne * Righe;
	inline constexpr int32 NumeroAzioni = 8;
	inline constexpr int32 MassimoMonete = 1000000;
	/** (12/11) I posti del baule (6 x 5). */
	inline constexpr int32 PostiBaule = 30;

	/** Mette in ordine uno zaino letto da un file: le misure giuste, via gli oggetti che non esistono, quantità e
	 *  cariche nei limiti, via dalla barra le azioni che non sono oggetti. */
	VALDORSO_API void Sistema(FValdorsoZaino& Zaino);

	/** Aggiunge Quantita oggetti (prima sulle pile che ci sono, poi nei posti liberi). Cariche < 0 = piene.
	 *  Restituisce quanti ne sono entrati (meno di Quantita se lo zaino è pieno; 0 se l'oggetto non esiste). */
	VALDORSO_API int32 Aggiungi(FValdorsoZaino& Zaino, FName Id, int32 Quantita, int32 Cariche = -1);

	/** (14/11, v0.2.3) Come Aggiungi, con il grado e la provenienza (ValdorsoQualita.h): si impila solo con quelli uguali. */
	VALDORSO_API int32 AggiungiCon(FValdorsoZaino& Zaino, FName Id, int32 Quantita, uint8 Qualita, const FString& Provenienza);

	/** (14/11, v0.2.3) Il posto dell'arma che si ha in mano con quell'Id: se ce n'e' piu' d'una, la migliore non rovinata
	 *  (poi la prima). INDEX_NONE se non c'e'. Lo zaino, la barra e il combattimento usano tutti questa. */
	VALDORSO_API int32 PostoInMano(const FValdorsoZaino& Zaino, FName Id);

	/** (14/11, v0.2.3) L'arma (o lo scudo) in mano si consuma di Quanto colpi. Vero se in quel momento si e' rovinata. */
	VALDORSO_API bool Consuma(FValdorsoZaino& Zaino, bool bSinistra, int32 Quanto = 1);

	/** (14/11, v0.2.3) Quale copia dell'arma con quell'Id si ripara: la consumata di grado piu' alto (e piu' consumata).
	 *  INDEX_NONE se nessuna e' consumata. (Puo' non essere quella in mano: una rovinata lascia il posto a una intera.) */
	VALDORSO_API int32 PostoDaRiparare(const FValdorsoZaino& Zaino, FName Id);

	/** (14/11, v0.2.3) Ripara l'arma (o lo scudo) della mano: la copia di PostoDaRiparare torna nuova. Falso se niente. */
	VALDORSO_API bool Ripara(FValdorsoZaino& Zaino, bool bSinistra);

	/** (10/11) Mette un oggetto con un numero (il foglio di un incarico) nel primo posto libero: il posto, o INDEX_NONE
	 *  se lo zaino e' pieno (o l'oggetto non esiste). Non si impila mai. */
	VALDORSO_API int32 AggiungiNumerato(FValdorsoZaino& Zaino, FName Id, int32 Numero);

	/** (10/11) Il posto dell'oggetto con quell'Id e quel numero (INDEX_NONE se non c'e'). */
	VALDORSO_API int32 PostoNumerato(const FValdorsoZaino& Zaino, FName Id, int32 Numero);

	/** Toglie Quantita oggetti da un posto (falso se non ce ne sono abbastanza). */
	VALDORSO_API bool Togli(FValdorsoZaino& Zaino, int32 Posto, int32 Quantita = 1);

	/** (08/11) Toglie Quantita oggetti con quell'Id, da qualunque posto (dagli ultimi posti ai primi). Falso, e non
	 *  toglie niente, se in tutto non ce ne sono abbastanza. */
	VALDORSO_API bool TogliOggetto(FValdorsoZaino& Zaino, FName Id, int32 Quantita);

	/** Quanti oggetti con quell'Id ci sono in tutto. */
	VALDORSO_API int32 Conta(const FValdorsoZaino& Zaino, FName Id);

	/** Il primo posto con quell'Id (per la barra delle azioni: la borraccia con dei sorsi, se c'è); INDEX_NONE se no. */
	VALDORSO_API int32 PostoDi(const FValdorsoZaino& Zaino, FName Id);

	/** Sposta un posto su un altro: nel posto libero ci va, sulla stessa pila si unisce (fin dove sta), sugli altri
	 *  oggetti si scambia. Falso se non è cambiato niente. */
	VALDORSO_API bool Sposta(FValdorsoZaino& Zaino, int32 Da, int32 A);

	/** Il peso di tutto (oggetti e monete), in kg. */
	VALDORSO_API float Peso(const FValdorsoZaino& Zaino);

	/** Vero se l'oggetto di quel posto si può gettare (i ricordi e la tessera no). */
	VALDORSO_API bool SiPuoGettare(const FValdorsoZaino& Zaino, int32 Posto);

	/** Vero se un oggetto ha senso nella barra delle azioni (si usa: mangiare, bere, curarsi, leggere). */
	VALDORSO_API bool DaBarra(FName Id);

	/** Mette un oggetto in un tasto della barra (NAME_None lo vuota). Falso se non si può. */
	VALDORSO_API bool Assegna(FValdorsoZaino& Zaino, int32 Tasto, FName Id);

	/** Quello che il colono ha all'arrivo, dal suo Registro (nullptr = un colono qualunque): si aggiunge a quello che
	 *  c'è, e bPartenzaData diventa vero. La barra prende il pane (tasto 1) e la borraccia (tasto 2). */
	VALDORSO_API void DaiPartenza(FValdorsoZaino& Zaino, const FValdorsoRegistro* Registro);

	/** Quante volte è cambiata la partenza dopo la v0.1.4 (vedi FValdorsoZaino::Aggiornamenti). */
	inline constexpr int32 UltimoAggiornamento = 2;

	/** (08/11) A chi era già arrivato dà le cose aggiunte poi alla partenza (la mappa della valle). Vero se ha dato
	 *  qualcosa. DaiPartenza segna già tutto come dato. */
	VALDORSO_API bool Aggiorna(FValdorsoZaino& Zaino);

	/** Riempie le borracce (falso se erano già piene, o non ce n'è). */
	VALDORSO_API bool RiempiBorracce(FValdorsoZaino& Zaino);

	/** (09/11, v0.1.4) Impugna un oggetto nella sua mano, o lo rimette via se e' gia' in mano. Vero se adesso e' in mano.
	 *  Falso anche se non si impugna o non c'e' nello zaino. */
	VALDORSO_API bool Impugna(FValdorsoZaino& Zaino, FName Id);

	/** (12/11) Vero se l'oggetto di quel posto dello zaino si puo' mettere nel baule (la tessera no: va sempre con te; e nemmeno il foglio
	 *  di un incarico della Gilda). */
	VALDORSO_API bool SiPuoDepositare(const FValdorsoZaino& Zaino, int32 Posto);

	/** (12/11) Mette nel baule tutto quello che c'e' in un posto dello zaino (sulle pile uguali, poi nei posti liberi). Se
	 *  il baule e' pieno ne mette quanto ci sta: il resto rimane nello zaino. Falso se non e' entrato niente. */
	VALDORSO_API bool Deposita(FValdorsoZaino& Zaino, int32 Posto);

	/** (12/11) Riprende nello zaino quello che c'e' in un posto del baule (fin dove ci sta). Falso se niente. */
	VALDORSO_API bool Ritira(FValdorsoZaino& Zaino, int32 PostoBaule);

	/** (09/11) Toglie dalle mani quello che non c'e' piu' nello zaino (o non si impugna in quella mano). Vero se cambia. */
	VALDORSO_API bool SistemaMani(FValdorsoZaino& Zaino);
}
