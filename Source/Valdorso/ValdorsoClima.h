// Valdorso - Il caldo e il freddo, e le ore del giorno (v0.1.4, seconda parte). Scritto da Claude il 09/11/2026,
// di notte, su richiesta di Fra ("la temperatura con i suoi effetti, e l'orario").
//
// La temperatura dell'aria la fanno la stagione (per ora quella del calendario vero, come lo stato del mondo), l'ora
// (più fresco all'alba, più caldo alle tre del pomeriggio; con le nuvole la differenza è minore), la quota (in alto fa
// più freddo) e il tempo (pioggia e nebbia rinfrescano). Quella che sente il colono (la "percepita") cambia con quello
// che ha intorno: bagnato sente più freddo, il mantello nello zaino e un tetto sopra la testa riparano, accanto al
// falò si sta caldi, nell'acqua conta l'acqua.
// Gli effetti (UValdorsoBagnatoComponent, sul server):
//   Gelo   (sotto 0)    la stamina torna a metà e la salute scende piano (mai sotto un quarto: il freddo indebolisce)
//   Freddo (0..8)       la stamina torna più piano (tre quarti)
//   Fresco, Mite        niente
//   Caldo  (24..30)     niente (per ora)
//   Afa    (oltre 30)   la stamina torna più piano (tre quarti)
// Il freno della stamina del freddo e quello dei vestiti bagnati non si sommano all'infinito: al massimo tre quarti.
// Sullo schermo, ai lati della bussola (SValdorsoBussola): a sinistra l'ora canonica del tempio e l'ora ("Vespro · 18:40"),
// a destra il termometro con i gradi (e "Freddo", "Gelo"... quando non è mite).
// Comando di console: Valdorso.Clima (scrive aria, percepita e sensazione di ogni colono).

#pragma once

#include "CoreMinimal.h"

/** Come si sente il colono. */
enum class EValdorsoSensazione : uint8
{
	Gelo,
	Freddo,
	Fresco,
	Mite,
	Caldo,
	Afa,
};

namespace ValdorsoClima
{
	/** Le soglie (gradi percepiti): sotto Gelo si gela, sotto Freddo si ha freddo... oltre Caldo è afa. */
	inline constexpr float SogliaGelo = 0.f;
	inline constexpr float SogliaFreddo = 8.f;
	inline constexpr float SogliaFresco = 15.f;
	inline constexpr float SogliaMite = 24.f;
	inline constexpr float SogliaCaldo = 30.f;
	/** Quanta salute al secondo toglie il gelo, e fin dove (frazione della salute massima). */
	inline constexpr float SaluteGeloAlSecondo = 0.5f;
	inline constexpr float SaluteMinimaGelo = 0.25f;

	/** La temperatura media della stagione ("inverno", "primavera", "estate", "autunno") in piazza. */
	VALDORSO_API float MediaStagione(const FString& Stagione);

	/**
	 * L'aria (gradi): Ora 0..24, Stagione come sopra, QuotaCm la Z del mondo (la piazza è a circa 55 m), Nuvole,
	 * Pioggia e Nebbia 0..1 (AValdorsoTempo::AriaAdesso).
	 */
	VALDORSO_API float Aria(float Ora, const FString& Stagione, float QuotaCm, float Nuvole, float Pioggia, float Nebbia);

	/** L'acqua del lago e del fiume nella stagione (gradi). */
	VALDORSO_API float Acqua(const FString& Stagione);

	/** Quella che sente il colono (gradi), da quello che ha intorno. */
	VALDORSO_API float Percepita(float GradiAria, float Bagnato, bool bInAcqua, float GradiAcqua, bool bMantello, bool bTetto, bool bFuoco);

	VALDORSO_API EValdorsoSensazione Sensazione(float Gradi);

	/** "Gelo", "Freddo", "Fresco", "Mite", "Caldo", "Afa". */
	VALDORSO_API FText Nome(EValdorsoSensazione Quale);

	/** Quanto frena il ritorno della stamina il caldo o il freddo (0 niente, 0,5 a metà). */
	VALDORSO_API float FrenoTemperatura(float Gradi);

	/** Il freno totale: quello dei vestiti bagnati (0 o 0,5) e quello della temperatura, al massimo 0,75. */
	VALDORSO_API float FrenoTotale(float FrenoBagnato, float FrenoCaldoFreddo);

	/** L'ora canonica del tempio (le campane): Mattutino, Lodi, Prima, Terza, Sesta, Nona, Vespro, Compieta. */
	VALDORSO_API FText OraCanonica(float Ora);

	/** "18:40" (con lo zero davanti). */
	VALDORSO_API FString OraEMinuti(float Ora);
}
