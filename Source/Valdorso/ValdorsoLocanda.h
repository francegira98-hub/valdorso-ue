// Valdorso - Le camere della Locanda di Bruno (v0.2.7.1 "Le rifiniture", 20/11/2026). Scritto da Claude.
//
// Di sopra, alla locanda, ci sono cinque camere con la porta a chiave. La camerata sotto il tetto resta di tutti e non
// costa niente; chi vuole una camera sua la chiede a Bruno:
// - paga PrezzoNotte Aureus e riceve la chiave di una camera libera (l'oggetto Chiave_Camera_N, ValdorsoOggetti.cpp);
// - la camera e' sua per un giorno della valle (SecondiPerNotte secondi veri: passano anche a gioco chiuso);
// - allo scadere Bruno si riprende la chiave (se in quel momento e' accanto alla porta, aspetta che si allontani: non
//   lo chiude dentro); la puo' anche rendere prima, o pagare un'altra notte quando ce l'ha gia' (si allunga);
// - le porte le chiude a chiave Content/Python/nuovi/crea_mura.py ("Mura: 10. la locanda nuova"): senza quel lancio le
//   camere restano aperte e la chiave e' solo un oggetto.
// Chi ha quale camera sta nello zaino del colono (FValdorsoZaino::CameraLocanda, CameraFinoUtc): niente file a parte.
// Una camera e' "libera" se nessuno di quelli in gioco adesso ce l'ha: due coloni che non si incontrano mai possono
// avere la stessa (per ora va bene: le camere sono poche e i coloni anche).
//
// Dialogo (ValdorsoDialoghiTesti.inl, Bruno): eventi LocandaCamera, LocandaRendi.
// Console (server o da soli): Valdorso.Locanda.Scade [nome]  (la notte finisce subito, per le prove).

#pragma once

#include "CoreMinimal.h"

class AActor;
class APlayerController;

namespace ValdorsoLocanda
{
	inline constexpr int32 Camere = 5;
	inline constexpr int32 PrezzoNotte = 2;
	inline constexpr int64 SecondiPerNotte = 7200;
	/** Finche' e' cosi' vicino alla porta della sua camera, la chiave scaduta non gli viene tolta (cm). */
	inline constexpr float RaggioPorta = 450.f;
	/** ...e sullo stesso piano (cm in altezza): le camere sono una sopra l'altra alla sala. */
	inline constexpr float AltezzaPorta = 200.f;

	/** L'oggetto chiave della camera N (1..Camere); NAME_None se N non vale. */
	VALDORSO_API FName ChiaveDellaCamera(int32 Camera);

	/** Vero se Id e' la chiave di una camera della locanda. */
	VALDORSO_API bool EChiaveDiCamera(FName Id);

	/** Sul server, dal dialogo di Bruno: vero se l'evento era della locanda (allora InOutProssimo e' il nodo dove andare). */
	VALDORSO_API bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Png, FName& InOutProssimo);
}
