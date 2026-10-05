// Valdorso - I suoni delle schermate (passo 4.2b, seconda parte). Scritto da Claude il 05/10/2026.
//
// I suoni del Registro stanno in Content/Audio/Registro (importati da Content/Python/importa_registro.py; li genera
// Tools/Generatori/suoni.py, senza licenze). Si suonano "in 2D", come la musica: non vengono da un punto della scena.
// Nei test automatici e sul server non c'è un mondo con l'audio: le funzioni non fanno niente e restituiscono nullptr.

#pragma once

#include "CoreMinimal.h"

class UAudioComponent;
class USoundBase;
class UWorld;

namespace ValdorsoSuoni
{
	/** Il mondo del gioco (anche in Play nell'editor); nullptr nei test. */
	VALDORSO_API UWorld* Mondo();

	/** Un suono di Content/Audio/Registro per nome ("S_Campana"); nullptr se non è importato. */
	VALDORSO_API USoundBase* Carica(const TCHAR* Nome);

	/**
	 * Suona e restituisce il componente (per fermarlo o sfumarlo). Chi lo tiene deve tenerlo vivo
	 * (TStrongObjectPtr), altrimenti la pulizia della memoria di Unreal può interromperlo.
	 * Inizio: da quale secondo del suono partire (per il battito, che va a tempo con il rombo che pulsa).
	 */
	VALDORSO_API UAudioComponent* Suona(const TCHAR* Nome, float Volume = 1.f, float Inizio = 0.f);

	/** Il motivo di una fede (chiave del Registro: "solara", "ignar"...). Niente se la chiave è vuota. */
	VALDORSO_API UAudioComponent* SuonaFede(const FString& Chiave, float Volume = 0.8f);

	/** Sfuma e ferma un suono (se c'è ancora). */
	VALDORSO_API void Sfuma(UAudioComponent* Componente, float Secondi = 0.3f);
}
