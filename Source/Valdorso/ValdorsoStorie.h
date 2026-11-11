// Valdorso - Le storie della valle (v0.1.8, 11/11/2026). Scritto da Claude: le quest segrete dei volti nuovi, dal
// Design MMO ("Quest segrete") e dalla Lore (Proposta P17, da confermare).
//
// Quasi tutto sta nei dialoghi (ValdorsoDialoghiTesti.inl) e nei segni, anche di un altro PNG ("Livia:LetteraData",
// ValdorsoDialoghi::HaSegnoDi). Qui ci sono solo le risposte con un evento, che il server controlla:
// - UNA LETTERA PER CASA (Livia, Nonna Edda): la capitana scrive alla madre, ma il sigillo non lascia passare niente.
//   Nonna Edda: "il velo ferma gli uomini, non i corvi". Il corvo parte; Livia, quando lo sa, ti da' il suo fischietto.
// - IL SOGNO DI TOBIA (Tobia): un sasso a forma d'orso che dorme, sulla collina dell'Orso, con un segno inciso. Chi lo
//   trova e lo legge (l'iscrizione "Il sasso dell'Orso", nel taccuino) e torna da Tobia riceve il suo sasso-orsetto.
//   (evento StoriaSasso: senza l'iscrizione nel taccuino si va a "non_ancora")
// - L'INCUDINE CHE CANTA (Marta): chi ha visto le linee dell'incudine e porta un cuore di pietra dei guardiani le fa
//   battere il cuore sull'incudine: le rune si accendono e cantano. Marta ammette tutto (quasi) e batte il chiodo runico.
//   (evento StoriaIncudine: toglie un Cuore_di_Pietra; senza si va a "senza_cuore")
// - LA VEGLIA DEL FRAMMENTO (Padre Anselmo): di notte (dalle 21 alle 5 della valle) chi lo chiede veglia il frammento
//   con lui: quattro minuti veri accanto all'altare, senza andarsene. Il battito si sente, poi la candela finisce.
//   (evento StoriaVeglia: di giorno si va a "di_giorno")
// Nel registro eventi: "Storia" (chi, quale, com'e' andata).

#pragma once

#include "CoreMinimal.h"

class AActor;
class APlayerController;

namespace ValdorsoStorie
{
	inline const TCHAR* EventoSasso = TEXT("StoriaSasso");
	inline const TCHAR* EventoIncudine = TEXT("StoriaIncudine");
	inline const TCHAR* EventoVeglia = TEXT("StoriaVeglia");
	/** (11/11, v0.2) Nonna Edda prende un secchio d'acqua torbida (per i suoi intrugli) e da' erbe della palude; il
	 *  secchio torna vuoto. Senza l'acqua si va a "acqua_niente". */
	inline const TCHAR* EventoAcquaEdda = TEXT("StoriaAcquaEdda");
	/** (11/11, v0.2.0) Il grano nascosto dei mugnai (ValdorsoScorte.h): solo quando la valle ha fame, una volta per
	 *  carestia ("grano_nascosto"; senza fame "grano_ok", gia' uscito "grano_gia"). */
	inline const TCHAR* EventoGrano = TEXT("StoriaGrano");
	inline constexpr float GranoNascosto = 40.f;

	/** Il titolo dell'iscrizione sulla collina (crea_volti.py la mette li'). */
	inline const TCHAR* TitoloSasso = TEXT("Il sasso dell'Orso");

	/** Quanto dura la veglia (secondi veri) e quanto lontano dal sacerdote si puo' stare (cm). */
	inline constexpr float SecondiVeglia = 240.f;
	inline constexpr float RaggioVeglia = 900.f;

	/** Vero se a quell'ora della valle (0..24) si puo' vegliare (la notte). */
	VALDORSO_API bool OraDiVeglia(float Ora);

	/** Solo sul server: le risposte con un evento delle storie; puo' cambiare il nodo dopo. Vero se l'evento era suo. */
	VALDORSO_API bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Png, FName& InOutProssimo);
}
