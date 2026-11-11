// Valdorso - La locanda viva e l'Atto I "Il battito cambiato" (v0.1.6 "Vita nella valle", La valle che vive, 11/11/2026).
// Scritto da Claude mentre Fra era a cena. Dalla roadmap ("La locanda viva: le voci vere che cambiano"; "Il Battito di
// base e l'Atto I 'Il battito cambiato': il frammento del tempio salta un colpo, la valle se ne accorge").
//
// Come funziona:
// - Le voci vere: Bruno ("Che si dice in giro?" -> "E di questi giorni?") racconta quello che e' successo davvero da
//   quando il server e' acceso, letto dal registro eventi (ValdorsoRegistroEventi::Ultimi): i lupi e gli orsi abbattuti,
//   i coloni tornati dall'altare, la frana del nord che trema o si apre, il capitano delle ossa, Lucia che chiude il
//   registro, il battito che salta. Mai i nomi dei coloni: in una locanda si dice "un forestiero".
// - I segnaposto nei dialoghi (ValdorsoDialoghiTesti.inl), sostituiti dal server prima di mandare la battuta
//   (AValdorsoPNG::Manda): una riga "{voci}" diventa le voci di adesso; una riga che comincia con "{se:atto1}" resta
//   solo se l'Atto I e' cominciato, "{senza:atto1}" solo se non e' cominciato.
// - L'Atto I: lo apre lo staff (F9 -> Mondo -> "Atto I: il battito salta", o Valdorso.Atto.Battito 1). Da quel momento,
//   e anche dopo un riavvio (Saved/<Server|Solo|Prova>/Atti.json), la pietra di Val d'Orso batte piu' debole e salta
//   i colpi, tutti i coloni sentono il battito fermarsi, le voci e i sogni ne parlano, Padre Anselmo ha le occhiaie.
//   Valdorso.Atto.Battito 0 lo richiude (solo prove).

#pragma once

#include "CoreMinimal.h"

class UObject;
class UWorld;

/** Quello che e' successo nella valle (lo legge DiOggi dal registro eventi; i test lo scrivono a mano). */
struct FValdorsoFattiValle
{
	int32 Lupi = 0;
	int32 Orsi = 0;
	int32 Cinghiali = 0;
	/** Coloni tornati dall'altare (morti e rinati). */
	int32 Morti = 0;
	/** Scheletri distrutti nelle gallerie. */
	int32 Scheletri = 0;
	bool bCapitano = false;
	/** La frana del nord: 0 chiusa, 1 smossa (si scava), 2 aperta. */
	int32 Frana = 0;
	/** Qualcuno ha riportato a Lucia le cose della spedizione. */
	bool bStoriaLucia = false;
	/** L'atto della storia (0 niente, 1 il battito ha saltato). */
	int32 Atto = 0;
};

namespace ValdorsoVoci
{
	/** La forza della pietra di Val d'Orso durante l'Atto I (sotto 0,5 il battito salta). */
	inline constexpr float ForzaAttoI = 0.45f;

	/** L'atto della storia: 0 = non cominciato, 1 = "Il battito cambiato" (il frammento ha saltato un colpo). */
	VALDORSO_API int32 Atto(const UObject* Contesto);

	/** Solo sul server: cambia l'atto (lo salva), con gli effetti: la pietra, l'avviso a tutti, il registro eventi. */
	VALDORSO_API void ImpostaAtto(UWorld* Mondo, int32 Nuovo);

	/** Quello che e' successo, dal registro eventi e dal mondo (solo sul server). */
	VALDORSO_API FValdorsoFattiValle Fatti(UWorld* Mondo);

	/** Le voci, come le dice Bruno: da 1 a 3 righe, le piu' grosse per prime (senza mondo: per i test). */
	VALDORSO_API TArray<FString> DaiFatti(const FValdorsoFattiValle& Fatti);

	/** Le voci di adesso (solo sul server). */
	VALDORSO_API TArray<FString> DiOggi(UWorld* Mondo);

	/** I segnaposto delle battute ({voci}, {se:atto1}, {senza:atto1}), con l'atto e le voci gia' pronti (per i test). */
	VALDORSO_API void RiempiCon(TArray<FString>& Righe, int32 AttoAdesso, const TArray<FString>& Voci);

	/** Solo sul server, prima di mandare una battuta: sostituisce i segnaposto. */
	VALDORSO_API void Riempi(UWorld* Mondo, TArray<FString>& Righe);
}
