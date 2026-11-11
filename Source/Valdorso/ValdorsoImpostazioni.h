// Valdorso - Le impostazioni del giocatore (v0.1.2, passo 6, seconda parte). Scritto da Claude il 02/11/2026.
//
// Grafica, audio, controlli e accessibilità. Stanno solo su questo PC (file GameUserSettings.ini, sezione
// [Valdorso.Impostazioni]): non vanno al server e non danno vantaggi. Si applicano subito, appena cambiate, e
// all'avvio del gioco (UValdorsoGameInstance::Init). Sul server dedicato non si fa niente.

#pragma once

#include "CoreMinimal.h"

struct VALDORSO_API FValdorsoImpostazioni
{
	// --- Grafica ---
	/** -1 = automatica (Unreal misura il PC), 0 bassa, 1 media, 2 alta, 3 epica. */
	int32 Qualita = -1;
	/** 0 = schermo intero, 1 = finestra senza bordi, 2 = in finestra (nell'editor non si cambia). */
	int32 Finestra = 1;
	/** (10/11) Quanto lontano si vede bene: 0 vicino, 1 normale, 2 lontano (di serie), 3 lontanissimo. Alberi, rocce,
	 *  case e terreno restano dettagliati piu' lontano, e si caricano piu' lontano i pezzi della valle (costa un po'). */
	int32 Distanza = 2;

	// --- Audio (0..1) ---
	float VolumeGenerale = 1.f;
	float VolumeMusica = 0.8f;
	float VolumeEffetti = 1.f;

	// --- Controlli ---
	/** Quanto gira la telecamera con mouse e levetta (0,25..3). */
	float Sensibilita = 1.f;
	bool bInvertiY = false;

	// --- Accessibilità ---
	/** La grandezza di tutte le scritte (1 = normale; 1,15 grandi; 1,3 molto grandi). */
	float GrandezzaTesto = 1.f;
	/** Meno movimento sullo schermo (niente sfocatura di movimento; più avanti anche meno scosse della telecamera). */
	bool bMenoMovimenti = false;
	/** Colori per chi vede i colori in modo diverso: 0 nessuno, 1 deuteranopia, 2 protanopia, 3 tritanopia. */
	int32 Daltonismo = 0;

	// --- (11/11, v0.1.5) Accessibilita', seconda parte (scelte da Fra il 10/11) ---
	/** Quanto trema la telecamera quando arriva un colpo (0 niente, 1 normale). */
	float Scosse = 1.f;
	/** La corsa "premi una volta": Maiusc accende e spegne la corsa invece di tenerlo premuto. */
	bool bCorsaAlterna = false;
	/** Le scritte dei suoni importanti in basso ("[Un ululato, a nord]"): ValdorsoInterfaccia.h. */
	bool bSottotitoli = true;
	/** Dove stanno le scritte del gioco: spostamento in pixel (X a destra, Y in giu') delle barre con la barra 1-8, della
	 *  bussola e della chat. */
	int32 BarreX = 0;
	int32 BarreY = 0;
	int32 BussolaX = 0;
	int32 BussolaY = 0;
	int32 ChatX = 0;
	int32 ChatY = 0;
	/** Le scritte del gioco (barre, bussola, chat) un po' piu' grandi o piu' piccole (0,8..1,4). */
	float GrandezzaHud = 1.f;
};

namespace ValdorsoImpostazioni
{
	/** Le impostazioni di adesso (lette dal file la prima volta). */
	VALDORSO_API const FValdorsoImpostazioni& Correnti();

	/** Cambia, applica subito e salva nel file. */
	VALDORSO_API void Cambia(const FValdorsoImpostazioni& Nuove);

	/** Applica le impostazioni di adesso (all'avvio, o dopo che il motore le ha toccate). */
	VALDORSO_API void Applica();

	/** (10/11) La distanza visiva nei mondi di gioco aperti (anche il caricamento dei pezzi della valle): la chiama il
	 *  PlayerController all'inizio e la rifanno le impostazioni quando cambia. */
	VALDORSO_API void ApplicaDistanza();

	/** Il volume di un suono dell'interfaccia: musica (motivi delle fedi, bordone) o effetti. */
	VALDORSO_API float VolumeSuono(bool bMusica);
}
