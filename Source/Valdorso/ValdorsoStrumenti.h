// Valdorso - Strumenti per gli script dell'editor (Python): cose che da Python non si possono fare.
// Scritto da Claude il 29/10/2026.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ValdorsoStrumenti.generated.h"

UCLASS()
class VALDORSO_API UValdorsoStrumenti : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Solo nell'editor: fa costruire subito un asset (per esempio un legame dei capelli, groom binding, appena creato)
	 * e aspetta che sia pronto, così si può salvare costruito. Da Python: unreal.ValdorsoStrumenti.costruisci_subito(a).
	 */
	UFUNCTION(BlueprintCallable, Category = "Valdorso|Editor")
	static void CostruisciSubito(UObject* Asset);

	/**
	 * (11/11/2026) La distanza di caricamento (Loading Range, cm) delle griglie di World Partition del mondo, almeno Minimo.
	 * Va con tutte e due le maniere di 5.8: la griglia (Spatial Hash: Grids) e le partizioni (Runtime Hash Set: Runtime
	 * Partitions -> Main Partition), cercando le proprieta' per nome (da Python RuntimeHash non si vede). Cambia solo la
	 * prima griglia (la principale). Restituisce cosa ha fatto, in una riga per griglia; vuoto se non ha trovato niente.
	 * Da Python: unreal.ValdorsoStrumenti.distanza_caricamento(mondo, 51200).
	 */
	UFUNCTION(BlueprintCallable, Category = "Valdorso|Editor", meta = (WorldContext = "Mondo"))
	static TArray<FString> DistanzaCaricamento(UObject* Mondo, int32 Minimo);
};
