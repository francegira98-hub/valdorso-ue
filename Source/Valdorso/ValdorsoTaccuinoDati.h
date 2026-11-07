// Valdorso - Bussola e taccuino di viaggio (v0.1.3): i dati e i conti. Scritto da Claude il 07/11/2026.
//
// Dal piano della v0.1.3: "mappa di carta senza puntino 'sei qui', bussola e taccuino di viaggio".
// Qui ci sono le cose che non disegnano niente (le provano i test Valdorso.Taccuino.*):
//   - FValdorsoTaccuino: quello che il personaggio ha scoperto e letto. Sta nel file del personaggio
//     (FValdorsoPersonaggio::Taccuino, Saved/Server/Archivio/Personaggi/<Id>.json); i file di prima, senza il
//     taccuino, si leggono uguali (taccuino vuoto).
//   - ValdorsoLuoghi: i luoghi della valle con nome, posizione e raggio di scoperta (ValdorsoLuoghiDati.inl, generato da
//     Tools/Generatori/luoghi_gioco.py da Luoghi_3000.json).
//   - ValdorsoBussola: da dove guarda la telecamera ai punti cardinali (N, NE, E, SE, S, SO, O, NO) e ai segni dei luoghi.
//     Nel mondo di Unreal X va a est e Y a sud: il nord è -Y.
//   - ValdorsoTaccuino: aggiungere un luogo, annotare un'iscrizione letta (senza doppioni, al massimo 200), rimettere
//     in ordine un taccuino letto da un file.
// Il componente che lo tiene sul server e lo manda al giocatore, la bussola e la schermata sono in ValdorsoTaccuino.h.

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoTaccuinoDati.generated.h"

/** Un luogo scoperto: il nome (come in ValdorsoLuoghi) e quando. */
USTRUCT()
struct FValdorsoLuogoScoperto
{
	GENERATED_BODY()

	UPROPERTY()
	FString Nome;

	/** Data e ora vere della scoperta (secondi Unix, UTC). */
	UPROPERTY()
	int64 Quando = 0;

	/** L'ora della valle (0..24) quando è stato scoperto; negativa se non si sa. */
	UPROPERTY()
	float Ora = -1.f;
};

/** Un'iscrizione (o una bacheca) letta: il titolo e le righe, come le ha mandate il server. */
USTRUCT()
struct FValdorsoIscrizioneLetta
{
	GENERATED_BODY()

	UPROPERTY()
	FString Titolo;

	UPROPERTY()
	TArray<FString> Righe;

	/** L'ultima volta che è stata letta con un testo nuovo (secondi Unix, UTC). */
	UPROPERTY()
	int64 Quando = 0;
};

/** Il taccuino di viaggio di un personaggio. */
USTRUCT()
struct FValdorsoTaccuino
{
	GENERATED_BODY()

	/** I luoghi scoperti, nell'ordine in cui sono stati trovati. */
	UPROPERTY()
	TArray<FValdorsoLuogoScoperto> Luoghi;

	/** Le iscrizioni lette, dalla più vecchia alla più nuova (al massimo ValdorsoTaccuino::MassimoIscrizioni). */
	UPROPERTY()
	TArray<FValdorsoIscrizioneLetta> Iscrizioni;
};

/** Un luogo della valle. */
struct FValdorsoLuogo
{
	FString Nome;
	/** Centimetri del mondo (X est, Y sud, Z quota). */
	FVector Posizione = FVector::ZeroVector;
	/** Raggio di scoperta (cm, in piano). */
	float Raggio = 0.f;
};

namespace ValdorsoLuoghi
{
	/** Tutti i luoghi della valle (da ValdorsoLuoghiDati.inl). */
	VALDORSO_API const TArray<FValdorsoLuogo>& Tutti();

	/** Il luogo con quel nome (nullptr se non c'è). */
	VALDORSO_API const FValdorsoLuogo* Trova(const FString& Nome);

	/** Vero se il punto sta nel raggio del luogo (in piano: la quota non conta). */
	VALDORSO_API bool Dentro(const FValdorsoLuogo& Luogo, const FVector& Dove);

	/** I luoghi nel cui raggio sta il punto (di solito nessuno o uno). */
	VALDORSO_API TArray<const FValdorsoLuogo*> Intorno(const FVector& Dove);
}

namespace ValdorsoBussola
{
	/** La rotta (gradi da 0 a 360, 0 = nord, 90 = est) di chi guarda con questa imbardata di Unreal (0 = +X = est). */
	VALDORSO_API float Rotta(float Imbardata);

	/** La rotta da un punto verso un altro (in piano). */
	VALDORSO_API float RottaVerso(const FVector& Da, const FVector& A);

	/** Di quanti gradi una cosa sta a destra (positivo) o a sinistra (negativo) di dove si guarda: da -180 a 180. */
	VALDORSO_API float Scarto(float RottaGuardata, float RottaCosa);

	/**
	 * Dove cade sulla striscia della bussola (pixel da sinistra) una cosa con quello Scarto, se la striscia è larga
	 * Larghezza pixel e mostra Campo gradi. Falso se la cosa sta fuori dalla striscia.
	 */
	VALDORSO_API bool SullaStriscia(float ScartoGradi, float Larghezza, float Campo, float& OutX);

	/** Il punto cardinale (0..7: N, NE, E, SE, S, SO, O, NO) più vicino alla rotta. */
	VALDORSO_API int32 PuntoCardinale(float RottaGradi);

	/** La sigla di un punto cardinale ("N", "NE", ... "NO"); vuota fuori da 0..7. */
	VALDORSO_API FString Sigla(int32 Punto);

	/** Una distanza a parole, come la direbbe un viandante: "40 m", "230 m" (a decine), "1,2 km". */
	VALDORSO_API FString TestoDistanza(double Centimetri);
}

/** Com'è andata l'annotazione di un'iscrizione letta. */
enum class EValdorsoLettura : uint8
{
	/** Già nel taccuino con lo stesso testo (o niente da annotare): non cambia niente. */
	Ignorata,
	/** Nuova: aggiunta in fondo. */
	Nuova,
	/** Lo stesso titolo con un testo diverso (una bacheca con avvisi nuovi): il testo si aggiorna e va in fondo. */
	Aggiornata
};

namespace ValdorsoTaccuino
{
	/** Quante iscrizioni tiene al massimo il taccuino: oltre, si toglie la più vecchia. */
	inline constexpr int32 MassimoIscrizioni = 200;

	/** Le misure di un'iscrizione, come quelle che manda la bacheca (20 righe di 300 lettere, titolo di 60). */
	inline constexpr int32 MassimoRighe = 20;
	inline constexpr int32 MassimoLettere = 300;
	inline constexpr int32 MassimoTitolo = 60;

	/** Vero se il luogo è già nel taccuino. */
	VALDORSO_API bool ConosceLuogo(const FValdorsoTaccuino& Taccuino, const FString& Nome);

	/** Aggiunge un luogo scoperto; falso (e niente cambia) se c'era già o il nome è vuoto. */
	VALDORSO_API bool AggiungiLuogo(FValdorsoTaccuino& Taccuino, const FString& Nome, int64 Quando, float Ora);

	/** Annota un'iscrizione letta: le righe si puliscono e si tagliano alle misure; i doppioni non si annotano. */
	VALDORSO_API EValdorsoLettura AnnotaIscrizione(FValdorsoTaccuino& Taccuino, const FString& Titolo, const TArray<FString>& Righe, int64 Quando);

	/** Rimette in ordine un taccuino letto da un file: via i nomi vuoti e i doppioni, misure e massimo rispettati. */
	VALDORSO_API void Sistema(FValdorsoTaccuino& Taccuino);

	/** Il momento del giorno a parole ("all'alba", "di sera"...) da un'ora della valle; vuoto se l'ora non si sa. */
	VALDORSO_API FText Momento(float Ora);
}
