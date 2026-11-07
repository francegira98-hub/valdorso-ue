// Valdorso - Il numero della versione, in un posto solo (07/11/2026, scritto da Claude).
// Lo leggono il menu (in basso a destra), la schermata d'avvio, lo stato del mondo che il server racconta
// (e quindi anche il launcher). A ogni etichetta nuova (git tag) si cambia solo qui.

#pragma once

#include "CoreMinimal.h"

namespace ValdorsoVersione
{
	/** Il numero ("0.1.3"). */
	inline const TCHAR* Numero() { return TEXT("0.1.3"); }

	/** Il nome della tappa ("La valle"). */
	inline const TCHAR* Nome() { return TEXT("La valle"); }

	/** Per le scritte: "v0.1.3 «La valle»". */
	inline FString PerScritte() { return FString::Printf(TEXT("v%s «%s»"), Numero(), Nome()); }
}
