// Valdorso - La qualita' degli oggetti e le armi vere (v0.2.3 "Armi vere", 14/11/2026). Scritto da Claude.
// Idea di Fra, approvata il 14/11 ("approvo tutto"); dal Piano, "Idee di Claude" del 14/11.
//
// Cinque gradi, con gli stessi colori delle storie del Battito (Tools/Sognatore/mattoni.json), cosi' tutto il gioco parla
// la stessa lingua: Comune (nessun colore), Buona (verde), Rara (blu), Epica (viola), Leggendaria (oro).
// - Nello zaino, nel baule e nella bottega lo sfondo della casella prende il colore, con un bordo sottile; nel riquadro
//   dell'oggetto il nome e' colorato e c'e' la riga della provenienza ("Fattura di Marta", "Forgiata da Fra",
//   "Trovata nella grotta del nord").
// - Da dove viene: la bottega (chi forgia piu' bravo, un po' di fortuna: Forgiata), i premi delle storie del Battito
//   (Rare ed Epiche), i ritrovamenti (la grotta del nord). Le Leggendarie non si forgiano: hanno un nome e una storia.
// - Cosa cambia: armi e scudi colpiscono, parano e durano di piu'; il cibo ristora di piu'; i vestiti scaldano di piu'.
//
// Le armi vere: ogni arma e ogni scudo si consuma (un colpo andato a segno, una parata). Consumata del tutto e'
// "rovinata": colpisce e para la meta' finche' non la si fa riparare (la fucina di Marta per il ferro, il falegname per il
// legno). Le famiglie (Pugnali, Spade, Asce, Mazze, Aste, Scudo, Arco) decidono quanto dura un'arma e come si muove chi la
// usa (le animazioni di Greystone, ValdorsoArmi.h).
// Solo regole, senza mondo ne' rete: le provano i test (Valdorso.Zaino.Qualita).

#pragma once

#include "CoreMinimal.h"

struct FValdorsoTipoOggetto;
struct FValdorsoPostoZaino;

/** La famiglia di un'arma: decide quanto dura, l'animazione del colpo e l'abilita' che sale. */
enum class EValdorsoFamigliaArma : uint8
{
	Nessuna,
	Pugnali,
	Spade,
	Asce,
	Mazze,
	Aste,
	Scudo,
	Arco,
};

namespace ValdorsoQualita
{
	inline constexpr uint8 Comune = 0;
	inline constexpr uint8 Buona = 1;
	inline constexpr uint8 Rara = 2;
	inline constexpr uint8 Epica = 3;
	inline constexpr uint8 Leggendaria = 4;
	inline constexpr uint8 Massima = Leggendaria;

	/** Quanto colpisce e para un'arma rovinata (consumata del tutto), finche' non la si ripara. */
	inline constexpr float FattoreRovinata = 0.5f;

	/** "Comune", "Buona", "Rara", "Epica", "Leggendaria". */
	VALDORSO_API const TCHAR* Nome(uint8 Qualita);

	/** Il colore del grado (quelli delle storie del Battito). Comune: il grigio della carta, che nello zaino non si mostra. */
	VALDORSO_API FLinearColor Colore(uint8 Qualita);

	/** Il colore si vede (dalla Buona in su). */
	inline bool Colorata(uint8 Qualita) { return Qualita > Comune; }

	/** Quanto di piu' colpisce un'arma (1 Comune .. 1,5 Leggendaria). */
	VALDORSO_API float Danno(uint8 Qualita);

	/** Quanto di piu' para uno scudo (1 .. 1,2: la parata resta comunque al massimo il 95%). */
	VALDORSO_API float Parata(uint8 Qualita);

	/** Quanto di piu' dura un'arma (1 .. 3). */
	VALDORSO_API float Durata(uint8 Qualita);

	/** Quanto di piu' ristora il cibo (salute, stamina, equilibrio: 1 .. 1,75). */
	VALDORSO_API float Ristoro(uint8 Qualita);

	/** Quanto di piu' scalda un vestito (1 .. 1,6). */
	VALDORSO_API float Calore(uint8 Qualita);

	/** La famiglia di un oggetto (Nessuna se non e' un'arma ne' uno scudo). */
	VALDORSO_API EValdorsoFamigliaArma Famiglia(FName Id);

	/** Il nome della famiglia ("Spade"): e' anche l'Id dell'abilita' che sale (ValdorsoAbilitaDati.h). */
	VALDORSO_API FName NomeFamiglia(EValdorsoFamigliaArma Famiglia);

	/** Vero se l'arma e' di legno (la ripara il falegname); falso se di ferro (la ripara Marta). */
	VALDORSO_API bool DiLegno(FName Id);

	/** Quanti colpi (o parate) regge un'arma di quel grado prima di rovinarsi; 0 = non si consuma (non e' un'arma). */
	VALDORSO_API int32 DurataMassima(FName Id, uint8 Qualita);

	/** Vero se l'oggetto di quel posto e' un'arma consumata del tutto. */
	VALDORSO_API bool Rovinata(const FValdorsoPostoZaino& Posto);

	/** Vero se l'oggetto di quel posto si e' un po' consumato (si puo' riparare). */
	VALDORSO_API bool Consumata(const FValdorsoPostoZaino& Posto);

	/** Quanto colpisce (o para) adesso l'oggetto di quel posto: il grado, e la meta' se e' rovinato. */
	VALDORSO_API float ForzaDelPosto(const FValdorsoPostoZaino& Posto, bool bParata);

	/**
	 * Il grado di un'arma forgiata: dalla bravura di chi forgia (l'abilita' "Bottega", 0..100) e da un po' di fortuna
	 * (Caso 0..1). Comune, Buona, Rara (dai 50 punti), Epica (dai 75). Mai Leggendaria: quelle hanno una storia.
	 */
	VALDORSO_API uint8 Forgiata(float PuntiBottega, float Caso);

	/** La riga dello stato ("Stato: 112 colpi su 150", "Rovinata: portala a Marta"); vuota se non si consuma. */
	VALDORSO_API FString TestoStato(const FValdorsoPostoZaino& Posto);

	/** La riga del grado e della provenienza ("Rara · Trovata nella grotta del nord"); vuota per un oggetto comune qualunque. */
	VALDORSO_API FString TestoQualita(const FValdorsoPostoZaino& Posto);
}
