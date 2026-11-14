// Valdorso - Il numero della versione, in un posto solo (07/11/2026, scritto da Claude).
// Lo leggono il menu (in basso a destra), la schermata d'avvio, lo stato del mondo che il server racconta
// (e quindi anche il launcher). A ogni etichetta nuova (git tag) si cambia solo qui.
// (11/11) E il server rifiuta chi ha un numero diverso (DiRete): si aggiorna dal launcher.

#pragma once

#include "CoreMinimal.h"

namespace ValdorsoVersione
{
	/** Il numero ("0.1.3"). */
	inline const TCHAR* Numero() { return TEXT("0.2.4"); }

	/** Il nome della tappa ("La valle"). */
	inline const TCHAR* Nome() { return TEXT("PNG veri"); }

	/** Per le scritte: "v0.1.3 «La valle»". */
	inline FString PerScritte() { return FString::Printf(TEXT("v%s «%s»"), Numero(), Nome()); }

	/** (11/11) Quante volte e' cambiato il modo in cui client e server si parlano (si alza quando cambia senza che cambi
	 *  il Numero, per esempio una correzione al volo). */
	inline constexpr int32 Protocollo = 1;

	/** (11/11) Il numero che il motore confronta quando un PC entra nel server (FNetworkVersion): se e' diverso, il
	 *  server lo rifiuta prima ancora dell'accesso, e il PC dice "il tuo gioco e' piu' vecchio: aggiornalo" (o il
	 *  contrario). Lo collega Valdorso.cpp all'avvio del modulo. */
	inline uint32 DiRete()
	{
		return FCrc::StrCrc32(*FString::Printf(TEXT("Valdorso %s protocollo %d"), Numero(), Protocollo));
	}
}
