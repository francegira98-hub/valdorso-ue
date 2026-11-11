// Valdorso - I tasti da cambiare (v0.1.5 "Crescere", accessibilita', 11/11/2026). Scritto da Claude. Dalla Roadmap
// ("Accessibilita': tasti rimappabili...") e da Fra il 10/11 ("tasti rimappabili, sottotitoli, comodita' del corpo, HUD
// spostabile, e se hai altre idee fai pure").
//
// Come funziona:
// - Ogni azione del colono ha un tasto di serie (W avanti, E interagisci, G la guardia...). Dalle Impostazioni
//   (Controlli -> un'azione -> "Premi un tasto") si sceglie un altro tasto; se era gia' di un'altra azione, le due si
//   scambiano. Si salva solo su questo PC (GameUserSettings.ini, sezione [Valdorso.Tasti]) e vale subito.
// - I tasti del colono che passano dal PlayerController (interagisci, zaino, scheda...) li legge AValdorsoPlayerController
//   da qui. Quelli del corpo (camminare, saltare, correre, schivare, colpire) stanno nell'Input Mapping Context del
//   Blueprint: se il giocatore ne ha cambiato uno, Rimappa fa una copia del contesto con i tasti nuovi (se no si usa
//   quello di serie, cosi' com'e').
// - Restano fissi: Esc e F10 (la pausa), F9 e F7 (lo staff), i tasti da 1 a 8 (la barra), il controller.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class UInputMappingContext;

/** Le azioni che si possono spostare su un altro tasto. */
enum class EValdorsoTasto : uint8
{
	Avanti,
	Indietro,
	Sinistra,
	Destra,
	Salto,
	Corsa,
	Schivata,
	Colpo,
	Immergiti,
	Interagisci,
	Guardia,
	Aggancio,
	Riponi,
	Zaino,
	Scheda,
	Taccuino,
	Mappa,
	Quaderno,
	Foto,
	Chat,
	Numero,
};

namespace ValdorsoTasti
{
	/** Il tasto di adesso per un'azione (quello scelto, o quello di serie). */
	VALDORSO_API FKey Tasto(EValdorsoTasto Azione);

	/** Il tasto di serie. */
	VALDORSO_API FKey DiSerie(EValdorsoTasto Azione);

	/** Il nome dell'azione, per le Impostazioni ("Avanti", "Cambia guardia"...). */
	VALDORSO_API FText Nome(EValdorsoTasto Azione);

	/** Il nome del tasto come lo legge un giocatore ("E", "Maiusc sinistro", "Clic sinistro"). */
	VALDORSO_API FText NomeTasto(const FKey& Tasto);

	/** Vero se quel tasto non si puo' usare per un'azione (la pausa, lo staff, la barra 1-8, il controller). */
	VALDORSO_API bool Riservato(const FKey& Tasto);

	/** L'azione che ha gia' quel tasto (Numero se nessuna). */
	VALDORSO_API EValdorsoTasto ChiHa(const FKey& Tasto);

	/**
	 * Mette il tasto a un'azione e lo salva. Se lo aveva un'altra azione, a quella va il tasto vecchio (si scambiano).
	 * Falso se il tasto e' riservato (non cambia niente).
	 */
	VALDORSO_API bool Imposta(EValdorsoTasto Azione, const FKey& Nuovo);

	/** Tutti i tasti come di serie. */
	VALDORSO_API void RimettiDiSerie();

	/** Vero se il giocatore ha cambiato almeno un tasto del corpo (camminare, saltare, correre, schivare, colpire). */
	VALDORSO_API bool CorpoCambiato();

	/**
	 * Il contesto dei tasti del corpo da usare: quello di serie se non e' cambiato niente, se no una copia (dentro Outer) con
	 * i tasti scelti al posto di quelli di serie (con gli stessi modificatori e inneschi).
	 */
	VALDORSO_API UInputMappingContext* Rimappa(UInputMappingContext* Originale, UObject* Outer);

	/** Suona quando un tasto cambia: i PlayerController ricollegano i loro tasti. */
	VALDORSO_API FSimpleMulticastDelegate& Cambiati();
}
