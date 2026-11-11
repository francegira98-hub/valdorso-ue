// Valdorso - I sogni (v0.1.6 "Vita nella valle", i riti: "il primo sogno"; 11/11/2026). Scritto da Claude mentre Fra
// era a cena. I testi sono una Proposta nella Lore (P15, da confermare con Fra).
//
// Come funziona:
// - Quando un colono si addormenta in un letto (UValdorsoSonnoComponent, fase "dorme"), il server decide se sogna.
//   La prima volta che si dorme: sempre, "Il primo sogno" (la valle dall'alto, due colpi e una pausa, l'ombra grande
//   sulla collina dell'Orso, una parola: Ur). Dopo: di notte (dalle 20 alle 6), una volta su tre, non piu' di un sogno
//   ogni venti minuti.
// - Quale sogno: prima quelli del mondo che cambia (con l'Atto I "Il battito che manca", con la frana del nord aperta
//   "I martelli"), una volta sola ciascuno; poi uno dei sogni della valle (le cinque luci, la strada del passo, la
//   collina dell'Orso).
// - Il sogno si legge nel buio del sonno, una riga alla volta, e resta scritto nel taccuino (tasto J, Iscrizioni:
//   "Sogno: ..."). Chi si sveglia prima lo interrompe.
// - Console (prove): Valdorso.Sogni.Sogna [primo|battito|martelli|luci|passo|collina] mostra un sogno su questo PC.

#pragma once

#include "CoreMinimal.h"

class APawn;
class UWorld;

/** Un sogno: l'Id (anche il segno nel taccuino), il titolo e le righe. */
struct FValdorsoSogno
{
	const TCHAR* Id = TEXT("");
	const TCHAR* Titolo = TEXT("");
	TArray<const TCHAR*> Righe;
};

namespace ValdorsoSogni
{
	/** Il dialogo (finto) nel taccuino che tiene i sogni gia' fatti, come segni. */
	inline constexpr const TCHAR* MemoriaSogni = TEXT("Sogni");

	/** Ogni quanti secondi al massimo un sogno, e quante probabilita' di sognare in una notte. */
	inline constexpr double SecondiTraSogni = 1200.0;
	inline constexpr float ProbabilitaSogno = 0.34f;

	/** Tutti i sogni (il primo e' "primo"). */
	VALDORSO_API const TArray<FValdorsoSogno>& Tutti();
	VALDORSO_API const FValdorsoSogno* Trova(FName Id);

	/**
	 * Quale sogno fare (senza mondo, per i test): bPrimo = non ha mai sognato; bNotte; Caso 0..1 (sogna se < probabilita');
	 * Atto e Frana come in ValdorsoVoci.h; GiaFatti = i sogni del mondo gia' visti. NAME_None = niente sogno.
	 */
	VALDORSO_API FName Scegli(bool bPrimo, bool bNotte, float Caso, int32 Atto, int32 Frana, const TSet<FName>& GiaFatti, int32 Giro);

	/** Solo sul server: il colono si e' appena addormentato. */
	VALDORSO_API void Addormentato(APawn* Chi);

	/** Su questo PC: mostra il sogno nel buio del sonno (o lo chiude, con Id vuoto). */
	VALDORSO_API void Mostra(UWorld* Mondo, FName Id);
	VALDORSO_API void Chiudi();
}
