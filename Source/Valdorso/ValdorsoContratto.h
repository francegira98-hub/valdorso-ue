// Valdorso - I contratti firmati e i documenti del Balivo (18/11/2026, v0.2.7 "Il tempio"; chiesti da Fra il 18/11).
//
// - Chi prende un lavoro (il primo: le Guardie di Valdorso, ValdorsoMilizia.h) firma un contratto vero: una pergamena
//   al centro dello schermo con le clausole, e sotto la sua firma disegnata (SValdorsoContratto.h). Il contratto
//   vincola: la ferma, il soldo, le chiavi.
// - Ogni contratto firmato resta fra i documenti del personaggio (FValdorsoZaino::Documenti: che cosa, quando, come
//   e' finito) e viene scritto nel registro del Balivo sul server (Saved/Server/Balivo/Documenti.jsonl, una riga per
//   documento, con la firma). Piu' avanti la fama leggera' da qui: chi ha onorato i contratti e chi li ha rotti.
// - Qui ci sono solo i dati e la scrittura del registro: senza mondo ne' rete.

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoContratto.generated.h"

/** Un documento firmato dal personaggio, cosi' come sta nel suo file. */
USTRUCT()
struct FValdorsoDocumento
{
	GENERATED_BODY()

	/** Che documento e' ("ContrattoGuardie"). */
	UPROPERTY()
	FName Tipo;

	/** "Contratto di arruolamento nelle Guardie di Valdorso". */
	UPROPERTY()
	FString Titolo;

	/** Quando e' stato firmato (secondi Unix, UTC). */
	UPROPERTY()
	int64 FirmatoUtc = 0;

	/** Come sta: ValdorsoContratti::InVigore, Concluso (onorato fino in fondo), Rotto (cacciato, o venuto meno). */
	UPROPERTY()
	uint8 Stato = 0;

	/** Quando e' finito (0 = ancora in vigore). */
	UPROPERTY()
	int64 ChiusoUtc = 0;
};

/** Il contratto da leggere e firmare (viaggia dal server al PC di chi firma). */
USTRUCT()
struct FValdorsoContrattoVista
{
	GENERATED_BODY()

	UPROPERTY()
	FName Tipo;

	/** La riga piccola in cima ("BALIVATO DI VAL D'ORSO"). */
	UPROPERTY()
	FString Intestazione;

	UPROPERTY()
	FString Titolo;

	/** Le clausole, una per riga. */
	UPROPERTY()
	TArray<FString> Righe;

	/** Chi riceve la firma ("Per il Balivo: L. Serra, capitana della Guardia Reale"). */
	UPROPERTY()
	FString Controparte;

	/** Il nome di chi firma, e la sua firma del Registro (per "firma come sul Registro"). */
	UPROPERTY()
	FString Nome;

	UPROPERTY()
	FString FirmaRegistro;

	/** (19/11) Un foglio solo da leggere (il registro della ronda con le firme): niente riquadro della firma, solo
	 *  "Chiudi". */
	UPROPERTY()
	bool bSoloLettura = false;
};

namespace ValdorsoContratti
{
	inline constexpr uint8 InVigore = 0;
	inline constexpr uint8 Concluso = 1;
	inline constexpr uint8 Rotto = 2;

	/** Quanti documenti tiene un personaggio (i piu' vecchi gia' chiusi escono). */
	inline constexpr int32 MassimoDocumenti = 40;

	/** Aggiunge un documento firmato adesso (in vigore) e tiene la lista entro MassimoDocumenti. */
	VALDORSO_API void Aggiungi(TArray<FValdorsoDocumento>& Documenti, FName Tipo, const FString& Titolo, int64 AdessoUtc);

	/** Chiude l'ultimo documento in vigore di quel tipo (Concluso o Rotto). Vero se ne ha chiuso uno. */
	VALDORSO_API bool Chiudi(TArray<FValdorsoDocumento>& Documenti, FName Tipo, uint8 Stato, int64 AdessoUtc);

	/** Sul server: una riga nel registro del Balivo (Saved/Server/Balivo/Documenti.jsonl). Cosa: "firmato",
	 *  "concluso", "rotto". La firma solo alla firma. */
	VALDORSO_API void ScriviAlBalivo(const FString& Nome, FName Tipo, const FString& Titolo, const FString& Cosa, const FString& Firma);
}
