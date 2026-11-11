// Valdorso - F8: "c'e' qualcosa che non va" (v0.1.7, parte D "fondamenta", 11/11/2026). Scritto da Claude.
//
// Chi gioca preme F8 (nell'editor F6: li' F8 e' gia' del motore) quando vede un errore (un muro che non c'e', un PNG nel pavimento, una frase sbagliata). Sul suo PC:
// - una foto dello schermo, senza l'interfaccia: Saved/Segnalazioni/Segnalazione_<data>.png;
// - accanto, Segnalazione_<data>.txt: la versione, la data, la mappa, dove era e dove guardava, il personaggio, l'ora
//   della valle, e le ultime righe del log del gioco (quello che il motore ha scritto prima);
// - al server arriva una riga breve (dove, che versione): finisce nel registro eventi come "Segnalazione", cosi' lo staff
//   (F9, Registro) sa che qualcuno ha visto qualcosa li'.
// Poi si mandano i due file a Fra (o allo staff). Al massimo una ogni 10 secondi.

#pragma once

#include "CoreMinimal.h"

class APlayerController;

namespace ValdorsoSegnala
{
	/** Le ultime Quante righe di un testo (per la coda del log). */
	VALDORSO_API FString UltimeRighe(const FString& Testo, int32 Quante);

	/** La cartella delle segnalazioni (Saved/Segnalazioni, assoluta). */
	VALDORSO_API FString Cartella();

	/** Sul PC: fa la segnalazione (foto e scheda). Falso se e' troppo presto; OutBreve e' la riga per il server. */
	VALDORSO_API bool Fai(APlayerController* Controllore, FString& OutNomeFile, FString& OutBreve);
}
