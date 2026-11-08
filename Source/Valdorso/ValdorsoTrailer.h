// Valdorso - Lo strumento per il trailer (v0.1.3, "modalità foto e strumento per il trailer"). Scritto da Claude il
// 08/11/2026.
//
// Vive dentro la modalità foto (ValdorsoModoFoto.h): con K si mettono le tappe di un percorso (posizione, sguardo con
// l'inclinazione, zoom e, se lo sfondo è sfocato, la distanza del fuoco), con L la telecamera lo ripercorre da sola.
// Qui c'è solo la parte che non dipende dal gioco (la matematica, il file JSON, i comandi di console), così i test
// (Test/ValdorsoTestTrailer.cpp, "Valdorso.Trailer.*") la provano senza mondo.
//   - La strada: una Catmull-Rom "centripeta" tra le tappe (non fa riccioli né punte quando le tappe sono a distanze
//     molto diverse). Lo sguardo: quaternioni con le tangenti di "squad" (niente scatti alle tappe). Zoom e fuoco: una
//     curva che non va mai oltre i valori delle tappe (niente zoom che rimbalza).
//   - La velocità: costante lungo la strada (una tabella delle lunghezze), con partenza e arrivo dolci (1,5 s). Una
//     tappa che gira lo sguardo senza muoversi conta come strada: 1 grado = 20 cm, così il giro sul posto non è istantaneo.
//   - Comandi di console: Valdorso.Trailer.Salva <nome>, Valdorso.Trailer.Carica <nome>, Valdorso.Trailer.Elenco,
//     Valdorso.Trailer.Svuota. I file stanno in Saved/Trailer/<nome>.json; chiudendo la modalità foto il percorso si
//     salva da solo in Saved/Trailer/ultimo.json, e alla prima apertura si rilegge da lì.
//   - Valdorso.Trailer.Fotogrammi 1: durante il percorso il tempo va a passi fissi di 1/30 di secondo e ogni fotogramma
//     diventa un'immagine in Saved/Trailer/<nome>/ (solo giocando senza server; vedi ValdorsoModoFoto.cpp).

#pragma once

#include "CoreMinimal.h"

/** Una tappa del percorso: dove sta la telecamera e come guarda. */
struct FValdorsoTappaTrailer
{
	FVector Posizione = FVector::ZeroVector;
	/** Beccheggio, imbardata e inclinazione (rollio), in gradi. */
	FRotator Rotazione = FRotator::ZeroRotator;
	/** Il campo visivo (gradi). */
	float Campo = 90.f;
	/** La distanza del fuoco (cm); 0 = senza sfocatura. */
	float Fuoco = 0.f;
};

/** Il percorso: le tappe, la velocità e se ricomincia da capo. */
struct FValdorsoPercorsoTrailer
{
	TArray<FValdorsoTappaTrailer> Tappe;
	/** cm al secondo (di serie 8 m/s). */
	float Velocita = 800.f;
	/** Vero: dopo l'ultima tappa torna alla prima e ricomincia (un anello chiuso). */
	bool bGiro = false;

	/** Quanti tratti ha la strada: N-1 tappe aperte, N ad anello (0 se ci sono meno di due tappe). */
	int32 Tratti() const
	{
		return Tappe.Num() < 2 ? 0 : (bGiro ? Tappe.Num() : Tappe.Num() - 1);
	}
};

/** La tabella delle lunghezze: dalla distanza percorsa al punto della strada ("chiave": 0 = prima tappa, 1 = seconda...). */
struct VALDORSO_API FValdorsoTabellaTrailer
{
	/** Misura la strada a pezzetti (PassiPerTratto per ogni tratto). */
	void Prepara(const FValdorsoPercorsoTrailer& Percorso, int32 PassiPerTratto = 24);

	/** Quanto è lunga la strada (cm, con i gradi contati come cm). */
	float Lunghezza() const { return Totale; }

	/** La chiave dopo Distanza cm: ad anello si ricomincia, aperta si ferma in fondo. */
	float Chiave(float Distanza) const;

	bool Pronta() const { return Distanze.Num() >= 2; }

private:
	TArray<float> Distanze;
	int32 Passi = 1;
	int32 Tratti = 0;
	bool bGiro = false;
	float Totale = 0.f;
};

namespace ValdorsoTrailer
{
	/** Al massimo tante tappe (il file resta piccolo e la tabella si fa in un attimo). */
	constexpr int32 MassimoTappe = 100;

	/** Partenza e arrivo dolci: quanti secondi per arrivare alla velocità piena. */
	constexpr float RampaSecondi = 1.5f;

	/** Un grado di sguardo (o di zoom) vale tanti cm di strada. */
	constexpr float CmPerGrado = 20.f;

	/** Le bande nere del cinema: 2,39 a 1. */
	constexpr float RapportoCinema = 2.39f;

	/** Un punto della Catmull-Rom centripeta tra P1 (U = 0) e P2 (U = 1). */
	VALDORSO_API FVector CatmullRomCentripeta(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float U);

	/** Un numero tra V1 (U = 0) e V2 (U = 1), morbido, senza mai uscire da [min(V1, V2), max(V1, V2)]. */
	VALDORSO_API float ValoreMorbido(float V0, float V1, float V2, float V3, float U);

	/** Lo sguardo tra Q1 (U = 0) e Q2 (U = 1), con le tangenti di squad prese da Q0 e Q3. */
	VALDORSO_API FQuat RotazioneMorbida(const FQuat& Q0, const FQuat& Q1, const FQuat& Q2, const FQuat& Q3, float U);

	/** La posa della telecamera alla Chiave (0 .. Tratti). */
	VALDORSO_API FValdorsoTappaTrailer Posa(const FValdorsoPercorsoTrailer& Percorso, float Chiave);

	/** Quanto della velocità piena (0..1): sale nei primi Rampa secondi, scende negli ultimi metri (Rimasta < 0 = mai). */
	VALDORSO_API float FattoreVelocita(float Partito, float Rimasta, float Velocita, float Rampa = RampaSecondi);

	/** La velocità dei tasti 1-5: 2, 4, 8, 15 e 30 m/s (in cm/s); 0 per gli altri numeri. */
	VALDORSO_API float VelocitaTasto(int32 Numero);

	/** L'altezza di ognuna delle due bande nere (sopra e sotto) per vedere Rapporto a 1; 0 se lo schermo è già più largo. */
	VALDORSO_API float AltezzaBande(const FVector2D& Schermo, float Rapporto = RapportoCinema);

	/** Il nome di un file di percorso: lettere, numeri, - e _ (gli altri diventano _), al massimo 40; vuoto se non va. */
	VALDORSO_API FString NomeValido(const FString& Nome);

	VALDORSO_API FString InJson(const FValdorsoPercorsoTrailer& Percorso);
	VALDORSO_API bool DaJson(const FString& Testo, FValdorsoPercorsoTrailer& OutPercorso);

	/** Saved/Trailer (percorso pieno). */
	VALDORSO_API FString Cartella();

	/** Il percorso di adesso (uno solo per PC: resta anche chiudendo la modalità foto). */
	VALDORSO_API FValdorsoPercorsoTrailer& Corrente();

	/** Cresce a ogni carica o svuota da fuori: la telecamera che sta percorrendo la strada si ferma. */
	VALDORSO_API int32 Generazione();

	/** Il nome dell'ultimo percorso salvato o caricato ("ultimo" all'inizio): dà il nome alla cartella dei fotogrammi. */
	VALDORSO_API FString NomeCorrente();

	/** Salva il percorso di adesso in Saved/Trailer/<Nome>.json; vero se è andata. */
	VALDORSO_API bool Salva(const FString& Nome, FString& OutFile);

	/** Carica Saved/Trailer/<Nome>.json nel percorso di adesso; falso (con il motivo) se non va. */
	VALDORSO_API bool Carica(const FString& Nome, FString& OutErrore);

	/** I nomi dei percorsi salvati (senza .json). */
	VALDORSO_API TArray<FString> Elenco();

	/** All'apertura della modalità foto: la prima volta rilegge Saved/Trailer/ultimo.json (se c'è). */
	VALDORSO_API void CaricaUltimoSeServe();

	/** Alla chiusura della modalità foto: salva in Saved/Trailer/ultimo.json (se ci sono tappe o c'era il file). */
	VALDORSO_API void SalvaUltimo();

	/** Vero se Valdorso.Trailer.Fotogrammi vale 1. */
	VALDORSO_API bool Fotogrammi();

	/** I fotogrammi al secondo dei fotogrammi salvati (30). */
	constexpr int32 FotogrammiAlSecondo = 30;
}
