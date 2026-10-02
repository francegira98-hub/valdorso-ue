// Valdorso - Le etichette (Gameplay Tags) scritte in C++.
// Un'etichetta è un'etichetta vera: si attacca a un personaggio, a un'abilità o a un evento,
// e gli altri sistemi la leggono ("sta schivando", "è arrivato il colpo").
// Più avanti saranno anche la lingua del Battito e del registro eventi.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace ValdorsoEtichette
{
	/** Il personaggio sta schivando (la porta la schivata finché dura). */
	VALDORSO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stato_Schivata);

	/** Il personaggio sta tirando un pugno (la porta il pugno finché dura). */
	VALDORSO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stato_Pugno);

	/** L'istante in cui il pugno arriva: lo manda la notifica "Valdorso: Colpo" del montaggio. */
	VALDORSO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Evento_Colpo);

	/** Il numero del danno, passato all'effetto del danno al momento del colpo. */
	VALDORSO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dato_Danno);
}
