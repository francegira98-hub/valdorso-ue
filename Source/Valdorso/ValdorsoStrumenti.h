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
};
