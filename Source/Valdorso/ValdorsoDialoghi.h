// Valdorso - I dialoghi con i PNG (v0.1.3: "Gaspare al margine del bosco; guardie del passo"). Scritto da Claude il
// 07/11/2026.
//
// Come funziona:
// - Un dialogo è un piccolo grafo: dei nodi (cosa dice il PNG: una o più battute) e, per ogni nodo, fino a 4 risposte
//   del colono. Ogni risposta porta a un altro nodo, oppure chiude il dialogo ("Me ne vado"). Un nodo senza risposte è
//   una fine voluta (bFine): il colono legge l'ultima battuta e si congeda.
// - Ogni dialogo ha due ingressi: Inizio (la prima volta che quel personaggio parla con quel PNG) e Ritorno (dalla
//   seconda volta: il saluto cambia). Il server si ricorda chi ha già parlato con chi (per ora solo in memoria, per
//   nome del personaggio: si dimentica quando il server si spegne).
// - Una risposta può avere un Evento: quando il colono la sceglie, il server la annota nel registro eventi con quel
//   tipo (per esempio "Corruzione" quando si offre una moneta a una guardia: un giorno potrà essere un crimine).
// - Le battute che cominciano con "rune:" si mostrano con le rune disegnate (come nelle bacheche, ValdorsoRune.h).
// - I testi sono scritti qui nel C++ (ValdorsoDialoghiTesti.inl): niente file da impacchettare, niente passi
//   nell'editor. I test (Test/ValdorsoTestDialoghi.cpp) controllano che ogni nodo esista, si possa raggiungere e
//   porti a una fine.
// - Chi decide è il server (AValdorsoPNG, ValdorsoPNG.h): manda al PC la battuta di adesso (FValdorsoBattuta), il PC
//   la mostra (SValdorsoDialogo) e rimanda il numero della risposta scelta.

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoDialoghi.generated.h"

/** Una risposta del colono. */
struct FValdorsoSceltaDialogo
{
	/** Cosa dice il colono ("Chi sei?"). */
	FString Testo;

	/** Il nodo dopo; NAME_None = il dialogo finisce qui. */
	FName Verso;

	/** Se non è vuoto, il server annota nel registro eventi un evento con questo tipo ("Corruzione"). */
	FName Evento;
};

/** Un nodo: cosa dice il PNG, e le risposte possibili. */
struct FValdorsoNodoDialogo
{
	FName Id;

	/** Le battute del PNG, una per paragrafo. Una riga "rune: Ur | didascalia" si mostra con le rune. */
	TArray<FString> Righe;

	/** Le risposte (al massimo ValdorsoDialoghi::MassimoScelte). */
	TArray<FValdorsoSceltaDialogo> Scelte;

	/** Una fine voluta: nessuna risposta, il colono si congeda. */
	bool bFine = false;

	/** Per scrivere i testi in fila: aggiunge una risposta e restituisce il nodo. */
	FValdorsoNodoDialogo& Scelta(const FString& Testo, FName Verso, FName Evento = NAME_None)
	{
		FValdorsoSceltaDialogo& Nuova = Scelte.AddDefaulted_GetRef();
		Nuova.Testo = Testo;
		Nuova.Verso = Verso;
		Nuova.Evento = Evento;
		return *this;
	}

	/** Per scrivere i testi in fila: segna il nodo come fine voluta. */
	FValdorsoNodoDialogo& Fine()
	{
		bFine = true;
		return *this;
	}
};

/** Un dialogo intero, di un PNG. */
struct FValdorsoDialogo
{
	/** Il nome stabile ("Gaspare"): lo usa l'attore (AValdorsoPNG::Dialogo) e la memoria di chi ha già parlato. */
	FName Id;

	/** Il nome breve per il suggerimento in basso ("E · Parla con Gaspare"). */
	FString NomeBreve;

	/** Il nome scritto in cima alla pergamena ("Gaspare, il vecchio cacciatore"). */
	FString Titolo;

	/** Il primo nodo la prima volta, e dalla seconda volta in poi. */
	FName Inizio;
	FName Ritorno;

	TArray<FValdorsoNodoDialogo> Nodi;

	/** Il nodo con questo Id (nullptr se non c'è). */
	const FValdorsoNodoDialogo* Trova(FName IdNodo) const
	{
		// (il parametro non si chiama Nodo: nasconderebbe la funzione Nodo qui sotto, avviso C4458)
		return Nodi.FindByPredicate([IdNodo](const FValdorsoNodoDialogo& Cercato) { return Cercato.Id == IdNodo; });
	}

	/** Per scrivere i testi in fila: aggiunge un nodo e lo restituisce (per aggiungergli le risposte subito dopo). */
	FValdorsoNodoDialogo& Nodo(FName IdNodo, const TArray<FString>& Righe)
	{
		FValdorsoNodoDialogo& Nuovo = Nodi.AddDefaulted_GetRef();
		Nuovo.Id = IdNodo;
		Nuovo.Righe = Righe;
		return Nuovo;
	}
};

/**
 * Quello che il server manda al PC: la battuta di adesso, già pronta da mostrare. Viaggia in rete (ClientMostraDialogo
 * del PlayerController).
 */
USTRUCT()
struct FValdorsoBattuta
{
	GENERATED_BODY()

	/** Il nome in cima alla pergamena. */
	UPROPERTY()
	FString Chi;

	/** Le battute del PNG. */
	UPROPERTY()
	TArray<FString> Righe;

	/** Le risposte possibili (vuoto con bFine). */
	UPROPERTY()
	TArray<FString> Scelte;

	/** Il numero della battuta nella conversazione: il PC lo rimanda con la risposta (così un doppio clic non conta due volte). */
	UPROPERTY()
	int32 Passo = 0;

	/** L'ultima battuta: si legge e ci si congeda. */
	UPROPERTY()
	bool bFine = false;

	/** Il server ha chiuso la conversazione (il colono si è allontanato, o ha scelto di andarsene): via la pergamena. */
	UPROPERTY()
	bool bChiudi = false;
};

namespace ValdorsoDialoghi
{
	/** Le risposte al massimo per nodo (i tasti 1-4). */
	inline constexpr int32 MassimoScelte = 4;

	/** Quanto può essere lunga una battuta (lettere): un messaggio in rete non deve diventare enorme. */
	inline constexpr int32 MassimoLettere = 700;

	/** Il dialogo con questo Id (nullptr se non c'è). */
	VALDORSO_API const FValdorsoDialogo* Trova(FName Id);

	/** Tutti i dialoghi scritti (per i test). */
	VALDORSO_API const TArray<FValdorsoDialogo>& Tutti();

	/** Il primo nodo: Inizio la prima volta, Ritorno dalla seconda (se il dialogo non ha un Ritorno, sempre Inizio). */
	VALDORSO_API FName NodoIniziale(const FValdorsoDialogo& Dialogo, bool bGiaParlato);

	/**
	 * Il colono sceglie la risposta numero Scelta nel nodo Nodo. Vero se la scelta è valida: allora OutProssimo è il
	 * nodo dopo (NAME_None = il dialogo finisce) e OutEvento l'evento da annotare (NAME_None = nessuno).
	 */
	VALDORSO_API bool Avanza(const FValdorsoDialogo& Dialogo, FName Nodo, int32 Scelta, FName& OutProssimo, FName& OutEvento);

	/** La battuta da mandare al PC per quel nodo (vuota con bChiudi se il nodo non c'è). */
	VALDORSO_API FValdorsoBattuta Battuta(const FValdorsoDialogo& Dialogo, FName Nodo, int32 Passo);

	/** I problemi di un dialogo (nodi che mancano, irraggiungibili, senza uscita...), in italiano; vuoto = va bene. */
	VALDORSO_API TArray<FString> Problemi(const FValdorsoDialogo& Dialogo);

	// --- La memoria del server: chi ha già parlato con chi ---------------------------------------------------------

	/** Vero se il personaggio Chi ha già parlato con il PNG del dialogo Id (da quando il server è acceso). */
	VALDORSO_API bool GiaParlato(FName Id, const FString& Chi);

	/** Ricorda che il personaggio Chi ha parlato con il PNG del dialogo Id. */
	VALDORSO_API void SegnaParlato(FName Id, const FString& Chi);

	/** Dimentica tutto (per i test). */
	VALDORSO_API void Dimentica();
}
