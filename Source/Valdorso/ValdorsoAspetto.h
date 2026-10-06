// Valdorso - L'aspetto del colono (v0.1.2, passo 4.4, parte dei dati). Scritto da Claude il 06/10/2026.
//
// Deciso da Fra il 06/10: il colono si crea con Mutable sui dati MetaHuman. Si sceglie una testa base (fatta nel
// MetaHuman Creator) e la si cambia con i cursori del volto; poi pelle, occhi, capelli, barba e corpo, e i segni
// (cicatrici, tatuaggi, segni particolari; chiesti da Fra il 06/10). Le cicatrici prese in battaglia (v0.4) e i segni
// delle fedi (dati dai sacerdoti) non sono qui: si guadagnano vivendo, e si aggiungeranno sopra questi.
// Qui ci sono solo i dati: il catalogo delle voci (in un posto solo, come per il Registro), i valori salvati nel file
// del personaggio e i controlli del server. Il collegamento con l'oggetto Mutable (i nomi dei parametri) arriva con la
// pagina Aspetto, quando l'oggetto del colono sarà pronto nell'editor.
//
// In rete e nel file viaggia un byte per voce (una ventina di byte in tutto): le scelte sono un numero (0 = la prima),
// i cursori vanno da 0 a 255 con 128 nel mezzo.

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoAspetto.generated.h"

/** L'aspetto salvato: un byte per voce del catalogo, nello stesso ordine. Versione 0 e nessun valore = non ancora scelto. */
USTRUCT(BlueprintType)
struct FValdorsoAspetto
{
	GENERATED_BODY()

	/** La versione del catalogo con cui sono scritti i valori (se il catalogo cambia, i vecchi si adattano). */
	UPROPERTY()
	int32 Versione = 0;

	UPROPERTY()
	TArray<uint8> Valori;
};

/** I gruppi della pagina Aspetto. */
enum class EValdorsoGruppoAspetto : uint8
{
	Volto, Pelle, Capelli, Corpo, Segni
};

/** Una voce dell'aspetto: una scelta tra più forme (la testa, l'acconciatura) o un cursore (il naso, l'altezza). */
struct FValdorsoVoceAspetto
{
	/** Il nome stabile della voce (non cambia mai: è anche il nome del parametro dell'oggetto Mutable). */
	const TCHAR* Id;
	/** Il nome da mostrare nella pagina Aspetto. */
	const TCHAR* Nome;
	EValdorsoGruppoAspetto Gruppo;
	/** Vero per un cursore (0..255); falso per una scelta (0..scelte-1). */
	bool bCursore;
	/** Quante scelte ci sono per un uomo e per una donna (1 = solo la prima, per esempio niente barba). */
	uint8 SceltePerUomo;
	uint8 SceltePerDonna;
	/** Il valore di partenza. */
	uint8 Predefinito;
};

namespace ValdorsoAspetto
{
	/**
	 * La versione del catalogo qui sotto. Si alza quando si aggiungono voci (sempre in fondo): allora, al caricamento
	 * del personaggio, va chiamato Adatta sugli aspetti delle versioni vecchie, altrimenti il server li rifiuta.
	 */
	inline constexpr int32 VersioneCatalogo = 1;

	/** Il mezzo dei cursori. */
	inline constexpr uint8 Mezzo = 128;

	/** Tutte le voci, nell'ordine dei valori salvati. */
	VALDORSO_API TArrayView<const FValdorsoVoceAspetto> Voci();

	/** La posizione di una voce nel catalogo; INDEX_NONE se non c'è. */
	VALDORSO_API int32 Indice(const TCHAR* Id);

	/** Quanti valori può avere una voce per questo sesso ("uomo" o "donna"; vuoto = il massimo dei due). 256 per i cursori. */
	VALDORSO_API int32 Possibili(const FValdorsoVoceAspetto& Voce, const FString& Sesso);

	/** L'aspetto di partenza (tutto nel mezzo, la prima testa). */
	VALDORSO_API FValdorsoAspetto Predefinito();

	/** Vero se l'aspetto non è ancora stato scelto (si usa quello di partenza). */
	VALDORSO_API bool NonScelto(const FValdorsoAspetto& Aspetto);

	/** Vuoto se l'aspetto va bene per questo sesso (anche "non scelto"), altrimenti il motivo in italiano. */
	VALDORSO_API FString Problema(const FValdorsoAspetto& Aspetto, const FString& Sesso);

	/** Il valore di una voce (quello di partenza se l'aspetto non è scelto o la voce manca). */
	VALDORSO_API uint8 Valore(const FValdorsoAspetto& Aspetto, const TCHAR* Id);

	/** Cambia il valore di una voce (riempie l'aspetto con quello di partenza, se non era scelto). */
	VALDORSO_API void Imposta(FValdorsoAspetto& Aspetto, const TCHAR* Id, uint8 Nuovo);

	/** Un cursore da -1 a 1 (0 nel mezzo), come lo vuole un parametro di Mutable. */
	VALDORSO_API float Cursore(const FValdorsoAspetto& Aspetto, const TCHAR* Id);

	/** I dadi del destino per l'aspetto: scelte a caso, cursori di solito vicini al mezzo (volti credibili). */
	VALDORSO_API void Casuale(FValdorsoAspetto& Aspetto, const FString& Sesso, FRandomStream& Dadi);

	/**
	 * Mette a posto un aspetto dopo un cambio di sesso o di catalogo: le scelte che non esistono più tornano alla
	 * prima (per esempio la barba per una donna), le voci nuove prendono il valore di partenza.
	 */
	VALDORSO_API void Adatta(FValdorsoAspetto& Aspetto, const FString& Sesso);

	/** Vero se i due aspetti sono uguali (non scelto conta come quello di partenza). */
	VALDORSO_API bool Uguali(const FValdorsoAspetto& A, const FValdorsoAspetto& B);
}
