// Valdorso - La tessera del colono (v0.1.4, 09/11/2026, chiesta da Fra). Scritto da Claude.
//
// La tessera è il lasciapassare con cui ogni colono è arrivato al passo: un foglio vero, con la sua faccia, nome, età,
// da dove viene, il vecchio mestiere, la fede, la qualifica ("Colono della valle dell'Orso"), il numero, la firma e il
// timbro della capitana. Si apre dallo zaino (clic destro sulla tessera) e si può MOSTRARE a chi si ha davanti: a
// lui si apre la stessa tessera, con la tua faccia (la fotografa il suo PC dal tuo colono, che vede).
// Cosa si salva (FValdorsoTessera, nel file del personaggio): il numero, la qualifica, dove e quando è stata rilasciata,
// i timbri e le note. Il resto si legge dal Registro, che non cambia più dopo la firma.
// Più avanti (roadmap): chi ti assume cambia la qualifica (il comandante della milizia scrive "Miliziano di Valdorso"),
// i timbri dei luoghi (il Balivo, la Gilda), le guardie che la chiedono al passo, la tessera falsa (con l'abilità e il
// rischio), la tessera persa e rifatta dal Balivo.
// Le regole qui sono senza mondo né rete (le provano i test Valdorso.Tessera.*). Le usa UValdorsoZainoComponent.

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoTessera.generated.h"

struct FValdorsoRegistro;

/** La tessera di un personaggio, così come sta nel suo file. */
USTRUCT()
struct FValdorsoTessera
{
	GENERATED_BODY()

	/** "VO-312-4K7P": l'anno dell'arrivo e quattro segni presi dal personaggio. Vuoto = non ancora rilasciata. */
	UPROPERTY()
	FString Numero;

	/** "Colono della valle dell'Orso"; chi ti assume la può cambiare (più avanti). */
	UPROPERTY()
	FString Qualifica;

	/** "Al passo, con la carovana di Pietralta". */
	UPROPERTY()
	FString Rilascio;

	/** I timbri, dal primo: "Visto al passo: L. Serra, capitana". */
	UPROPERTY()
	TArray<FString> Timbri;

	/** Le note scritte da chi ha autorità (più avanti). */
	UPROPERTY()
	TArray<FString> Note;
};

/** Quello che si vede aprendo una tessera (viaggia dal server al PC di chi la guarda). */
USTRUCT()
struct FValdorsoTesseraVista
{
	GENERATED_BODY()

	UPROPERTY()
	FString Nome;

	/** "uomo" o "donna" (per gli accordi: "Nato" o "Nata"). */
	UPROPERTY()
	FString Sesso;

	UPROPERTY()
	int32 Eta = 0;

	/** I testi del Registro: da dove viene, il vecchio mestiere, la fede (corta). */
	UPROPERTY()
	FString Origine;

	UPROPERTY()
	FString Mestiere;

	UPROPERTY()
	FString Fede;

	UPROPERTY()
	FString Firma;

	UPROPERTY()
	FValdorsoTessera Tessera;
};

namespace ValdorsoTessera
{
	inline constexpr int32 MassimoTimbri = 12;
	inline constexpr int32 MassimoNote = 12;
	inline constexpr int32 MassimaLunghezza = 120;

	/** Il numero di tessera di un personaggio: sempre lo stesso per lo stesso Id ("VO-312-4K7P"). */
	VALDORSO_API FString Numero(const FString& IdPersonaggio);

	/** La tessera rilasciata al passo all'arrivo. */
	VALDORSO_API FValdorsoTessera Nuova(const FString& IdPersonaggio);

	/** Mette in ordine una tessera letta da un file (lunghezze, numeri di timbri e note); se manca, la rilascia. */
	VALDORSO_API void Sistema(FValdorsoTessera& Tessera, const FString& IdPersonaggio);

	/** Quello che si vede: dal nome, dal Registro e dalla tessera salvata. */
	VALDORSO_API FValdorsoTesseraVista Vista(const FString& Nome, const FValdorsoRegistro* Registro, const FValdorsoTessera& Tessera);
}
