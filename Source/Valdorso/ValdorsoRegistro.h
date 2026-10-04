// Valdorso - Il Registro di Val d'Orso: le risposte del nuovo colono (v0.1.2, passo 4.1; aiuti per la schermata al 4.2a). Scritto da Claude il 05/10/2026.
//
// Il sacerdote scrive il colono nel registro: sesso, età, fede, da dove viene, cosa faceva, perché è venuto,
// cosa porta con sé, cosa teme, com'è il suo carattere, se ha sentito il richiamo della magia; poi compone un breve
// racconto (che il giocatore può ritoccare) e il giocatore aggiunge, se vuole, "La tua storia" con parole sue.
// Il catalogo delle risposte sta qui, in un posto solo: lo usano il server (che controlla) e la schermata (che mostra).
// I vantaggi delle risposte (documento, "I vantaggi delle risposte") si accendono con i sistemi che li usano:
// abilità e fama alla v0.1.5, inventario alla v0.1.4/v0.2, fede e magia più avanti. Qui si salvano soltanto.

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoRegistro.generated.h"

/** Le risposte del Registro, salvate nel file del personaggio. Le scelte sono chiavi del catalogo (vuote = non ancora). */
USTRUCT(BlueprintType)
struct FValdorsoRegistro
{
	GENERATED_BODY()

	/** Vero quando il registro è firmato: da lì le risposte non cambiano più (si cambia vivendo). */
	UPROPERTY()
	bool bFirmato = false;

	UPROPERTY()
	int64 FirmatoIl = 0;

	UPROPERTY()
	FString Sesso;

	UPROPERTY()
	int32 Eta = 0;

	UPROPERTY()
	FString Fede;

	UPROPERTY()
	FString Origine;

	UPROPERTY()
	FString Mestiere;

	UPROPERTY()
	FString Motivo;

	UPROPERTY()
	FString Ricordo;

	UPROPERTY()
	FString Paura;

	/** Il carattere, quattro coppie: onesto/furbo, coraggioso/prudente, devoto/scettico, gentile/duro. */
	UPROPERTY()
	FString Onesta;

	UPROPERTY()
	FString Coraggio;

	UPROPERTY()
	FString Devozione;

	UPROPERTY()
	FString Animo;

	/** "Hai mai sentito qualcosa rispondere alla tua volontà?" (si/no; vuoto = non ancora). */
	UPROPERTY()
	FString RichiamoMagia;

	/** Il racconto del sacerdote, composto dalle risposte e ritoccabile dal giocatore. */
	UPROPERTY()
	FString Racconto;

	/** La tua storia, con parole del giocatore (facoltativa). Lo staff può chiedere di correggerla (passo 4.5). */
	UPROPERTY()
	FString Storia;
};

/** Una risposta del catalogo: la chiave salvata, e il testo da mostrare (maschile e femminile se cambia). */
struct FValdorsoVoceRegistro
{
	const TCHAR* Chiave;
	const TCHAR* Maschile;
	const TCHAR* Femminile;
};

namespace ValdorsoRegistro
{
	inline constexpr int32 EtaMinima = 18;
	inline constexpr int32 EtaMassima = 60;
	inline constexpr int32 MassimoRacconto = 2000;
	inline constexpr int32 MassimoStoria = 3000;

	/** Le pagine del registro con domande a scelta, nell'ordine in cui il sacerdote le fa. */
	enum class EDomanda : uint8
	{
		Sesso, Fede, Origine, Mestiere, Motivo, Ricordo, Paura, Onesta, Coraggio, Devozione, Animo, RichiamoMagia, Numero
	};

	/** Le risposte possibili per una domanda. */
	VALDORSO_API TArrayView<const FValdorsoVoceRegistro> Voci(EDomanda Quale);

	/** La domanda come la fa il sacerdote ("Da dove vieni?"), al femminile se Sesso è "donna". */
	VALDORSO_API const TCHAR* Domanda(EDomanda Quale, const FString& Sesso = FString());

	/** La risposta salvata per una domanda (riferimento al campo del registro). */
	VALDORSO_API FString& Risposta(FValdorsoRegistro& Registro, EDomanda Quale);
	VALDORSO_API const FString& Risposta(const FValdorsoRegistro& Registro, EDomanda Quale);

	/** Il testo da mostrare per una risposta (secondo il sesso già scelto). Vuoto se la chiave non c'è. */
	VALDORSO_API FString Testo(EDomanda Quale, const FString& Scelta, const FString& Sesso);

	/**
	 * Vuoto se il registro va bene, altrimenti il motivo. bCompleto: tutte le risposte devono esserci (alla firma);
	 * falso: si controllano solo quelle date (la bozza).
	 */
	VALDORSO_API FString Problema(const FValdorsoRegistro& Registro, bool bCompleto);

	/** Il racconto del sacerdote, composto dalle risposte. */
	VALDORSO_API FString ComponiRacconto(const FValdorsoRegistro& Registro, const FString& Nome);

	/** I dadi del destino: riempie le risposte vuote (o tutte, se bTutte) a caso. */
	VALDORSO_API void Casuale(FValdorsoRegistro& Registro, FRandomStream& Dadi, bool bTutte);

	/** I dadi del destino per una sola domanda (passo 4.2a): una risposta a caso, diversa da quella di prima se si può. */
	VALDORSO_API void CasualeUna(FValdorsoRegistro& Registro, EDomanda Quale, FRandomStream& Dadi);

	/** Vero se le due versioni hanno le stesse risposte, età, racconto e storia (per non salvare bozze uguali). */
	VALDORSO_API bool StesseRisposte(const FValdorsoRegistro& A, const FValdorsoRegistro& B);

	/** La prima domanda ancora senza risposta (l'età conta come Sesso, sulla stessa pagina); Numero se non manca niente. */
	VALDORSO_API EDomanda PrimaMancante(const FValdorsoRegistro& Registro);

	/**
	 * Cosa porterà una risposta nella valle (documento, "I vantaggi delle risposte", approvati il 28/09), in una riga
	 * da mostrare nel Registro (passo 4.2a, ritocchi del 05/10). Vuoto se la risposta non ha un vantaggio da dire
	 * (sesso e fede). I vantaggi si accendono con i sistemi che li usano: qui sono solo parole.
	 */
	VALDORSO_API const TCHAR* Vantaggio(EDomanda Quale, const FString& Scelta);
}
