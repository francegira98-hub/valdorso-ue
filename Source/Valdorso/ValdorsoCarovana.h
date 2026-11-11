// Valdorso - La carovana (v0.2.0 "Mani e materiali", 12/11/2026). Scritto da Claude; la scelta di lore l'ha lasciata
// a Claude Fra ("scegli tu in base alla lore e coerenza"): Lore P18.
//
// Il sigillo non lascia passare nessuno, tranne chi ha il LASCIAPASSARE della Corona: una carovana ogni tre giorni
// (veri), perquisita al passo dalla capitana Livia, senza lettere ne' persone. La guida Sibilla, la venditrice
// ambulante, che quel lasciapassare l'ha sempre (e nessuno sa come lo rinnova cosi' in fretta).
// Il giorno della carovana Sibilla sta al piazzale delle carovane dalle 9 alle 20 della valle (la sua giornata,
// AValdorsoPNG::OgniGiorni = 3): si parla con lei e si apre il suo banco (EValdorsoTipoBanco::Carovana: sale, spezie,
// stoffa; compra perle, ferro, chiodi). Quando arriva porta al magazzino del villaggio grano e legna (ValdorsoScorte),
// e tutti i coloni lo sanno.
// Console (server): Valdorso.Carovana.Oggi 1 (la carovana viene oggi, per le prove), Valdorso.Carovana.Oggi 0 (di nuovo
// il suo calendario).

#pragma once

#include "CoreMinimal.h"

class AValdorsoPNG;

namespace ValdorsoCarovana
{
	/** Quanto porta al magazzino a ogni arrivo. */
	inline constexpr float GranoPortato = 30.f;
	inline constexpr float LegnaPortata = 10.f;

	/** Vero se in quel momento (secondi veri dal 1970) e' un giorno di carovana (uno ogni OgniGiorni). */
	VALDORSO_API bool Giorno(int64 Adesso, int32 OgniGiorni);

	/** Vero se la carovana e' stata fatta venire oggi dalla console. */
	VALDORSO_API bool Forzata();

	/** Solo sul server: Sibilla e' arrivata (le scorte, il registro, l'avviso a tutti). */
	VALDORSO_API void Arriva(AValdorsoPNG* Sibilla);
}
