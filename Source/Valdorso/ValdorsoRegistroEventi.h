// Valdorso - Il registro eventi del mondo: primo mattone.
// Ogni azione che un giorno potrebbe essere un crimine (colpire, rubare, uccidere, entrare dove non si deve)
// passa da qui, con chi, cosa, dove e quando. Scrive una riga nel Registro output (cercare "Registro:") e, dal
// 03/11/2026 (v0.1.3), una riga JSON sul disco del server (Saved/Server/Eventi, un file al giorno). Più avanti avrà
// anche i testimoni e lo leggeranno leggi, taglie, dicerie e Battito.

#pragma once

#include "CoreMinimal.h"

class AActor;

namespace ValdorsoRegistroEventi
{
	/**
	 * Annota un evento. Lo fa solo il server (chi decide il mondo).
	 * Tipo: che cosa è successo ("Colpo"); Chi: chi l'ha fatto; Bersaglio: a chi (può mancare);
	 * Dove: il punto nel mondo; Dettagli: una riga in più (per esempio il danno).
	 */
	VALDORSO_API void Annota(const FString& Tipo, const AActor* Chi, const AActor* Bersaglio, const FVector& Dove, const FString& Dettagli = FString());
}
