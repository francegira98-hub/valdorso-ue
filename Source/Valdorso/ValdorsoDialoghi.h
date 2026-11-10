// Valdorso - I dialoghi con i PNG (v0.1.3: "Gaspare al margine del bosco; guardie del passo"). Scritto da Claude il
// 07/11/2026.
//
// Come funziona:
// - Un dialogo è un piccolo grafo: dei nodi (cosa dice il PNG: una o più battute) e, per ogni nodo, fino a 4 risposte
//   del colono. Ogni risposta porta a un altro nodo, oppure chiude il dialogo ("Me ne vado"). Un nodo senza risposte è
//   una fine voluta (bFine): il colono legge l'ultima battuta e si congeda.
// - Ogni dialogo ha due ingressi: Inizio (la prima volta che quel personaggio parla con quel PNG) e Ritorno (dalla
//   seconda volta: il saluto cambia). Il server si ricorda chi ha già parlato con chi: (08/11) nel file del
//   personaggio, con il taccuino (FValdorsoMemoriaDialoghi, qui sotto), così resta anche quando il server si spegne;
//   senza un personaggio salvato (accesso libero dell'editor) solo in memoria, per nome.
// - Una risposta può avere un Evento: quando il colono la sceglie, il server la annota nel registro eventi con quel
//   tipo (per esempio "Corruzione" quando si offre una moneta a una guardia: un giorno potrà essere un crimine).
// - I segni (08/11): un nodo, o una risposta, può lasciare un segno al personaggio ("MonetaOfferta"); una risposta può
//   restare nascosta a chi ha già quel segno (NascostaSe). Così l'offerta della moneta alle guardie si fa una volta
//   sola. I segni stanno nella stessa memoria di "chi ha già parlato" (per dialogo e per personaggio, solo sul
//   server; (08/11) nel file del personaggio, come sopra). Il PC vede solo le risposte visibili, e il
//   numero che rimanda è quello tra le visibili (ScelteVisibili lo riporta alla risposta vera).
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

	/** (08/11) Se non è vuoto: chi sceglie questa risposta riceve questo segno (vedi ValdorsoDialoghi::HaSegno). */
	FName Segna;

	/** (08/11) Se non è vuoto: la risposta non si mostra a chi ha già questo segno ("(Offri una moneta)" una volta sola). */
	FName NascostaSe;
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

	/** (08/11) Se non è vuoto: chi arriva a questo nodo riceve questo segno (vedi ValdorsoDialoghi::HaSegno). */
	FName Segna;

	/** Per scrivere i testi in fila: aggiunge una risposta e restituisce il nodo. */
	FValdorsoNodoDialogo& Scelta(const FString& Testo, FName Verso, FName Evento = NAME_None)
	{
		FValdorsoSceltaDialogo& Nuova = Scelte.AddDefaulted_GetRef();
		Nuova.Testo = Testo;
		Nuova.Verso = Verso;
		Nuova.Evento = Evento;
		return *this;
	}

	/** (08/11) Per scrivere i testi in fila: chi arriva a questo nodo riceve il segno. */
	FValdorsoNodoDialogo& Segno(FName Nome)
	{
		Segna = Nome;
		return *this;
	}

	/** (08/11) Per scrivere i testi in fila: chi sceglie l'ultima risposta aggiunta riceve il segno. */
	FValdorsoNodoDialogo& SceltaSegna(FName Nome)
	{
		if (Scelte.Num() > 0)
		{
			Scelte.Last().Segna = Nome;
		}
		return *this;
	}

	/** (08/11) Per scrivere i testi in fila: l'ultima risposta aggiunta non si mostra a chi ha già il segno. */
	FValdorsoNodoDialogo& SceltaNascostaSe(FName Nome)
	{
		if (Scelte.Num() > 0)
		{
			Scelte.Last().NascostaSe = Nome;
		}
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

/**
 * (08/11) La memoria di un personaggio per i dialoghi: se ha già parlato con un PNG e i segni che ha. Il dialogo legge e
 * scrive solo da qui, senza sapere dove sta davvero:
 * - con un personaggio vero sta nel suo file (il taccuino, UValdorsoTaccuinoComponent sul PlayerController): resta
 *   anche quando il server si spegne (ValdorsoPNG.cpp ne fa una di queste attorno al componente);
 * - senza (l'accesso libero dell'editor, i test) c'è FValdorsoMemoriaPerNome: quella di prima, per nome del personaggio,
 *   solo nella memoria del server.
 */
class VALDORSO_API FValdorsoMemoriaDialoghi
{
public:
	virtual ~FValdorsoMemoriaDialoghi() = default;

	/** Vero se ha già parlato con il PNG del dialogo IdDialogo. */
	virtual bool GiaParlato(FName IdDialogo) const = 0;

	/** Ricorda che ci ha parlato. */
	virtual void SegnaParlato(FName IdDialogo) = 0;

	/** Vero se ha il segno Segno nel dialogo IdDialogo (falso con Segno vuoto). */
	virtual bool HaSegno(FName IdDialogo, FName Segno) const = 0;

	/** Gli dà il segno (niente con Segno vuoto). */
	virtual void MettiSegno(FName IdDialogo, FName Segno) = 0;
};

/**
 * (08/11) La memoria del server per nome del personaggio (ValdorsoDialoghi::GiaParlato...): si dimentica quando il
 * server si spegne. Con il nome vuoto non ricorda niente (nessun segno: si vedono tutte le risposte).
 */
class VALDORSO_API FValdorsoMemoriaPerNome : public FValdorsoMemoriaDialoghi
{
public:
	explicit FValdorsoMemoriaPerNome(const FString& InChi) : Chi(InChi) {}

	virtual bool GiaParlato(FName IdDialogo) const override;
	virtual void SegnaParlato(FName IdDialogo) override;
	virtual bool HaSegno(FName IdDialogo, FName Segno) const override;
	virtual void MettiSegno(FName IdDialogo, FName Segno) override;

private:
	FString Chi;
};

namespace ValdorsoDialoghi
{
	/** Le risposte al massimo per nodo (i tasti 1-4). */
	inline constexpr int32 MassimoScelte = 4;

	/** Quanto può essere lunga una battuta (lettere): un messaggio in rete non deve diventare enorme. */
	inline constexpr int32 MassimoLettere = 700;

	/** Il dialogo con questo Id (nullptr se non c'è). */
	VALDORSO_API const FValdorsoDialogo* Trova(FName Id);

	/** (10/11) I saluti a chi passa: da quanto vicino (cm), ogni quanto allo stesso colono (s). */
	inline constexpr float RaggioSaluto = 550.f;
	inline constexpr double SecondiTraSaluti = 420.0;

	/** (10/11) Una riga di saluto del PNG Id per quest'ora della valle (0..24); Seme sceglie fra le righe. Vuota se il
	 *  PNG non saluta (per esempio di notte chi dorme). */
	VALDORSO_API FString Saluto(FName Id, float Ora, int32 Seme);

	/** (10/11) Tutte le righe di saluto di un PNG (per i test). */
	VALDORSO_API TArray<FString> TuttiISaluti(FName Id);

	/** Tutti i dialoghi scritti (per i test). */
	VALDORSO_API const TArray<FValdorsoDialogo>& Tutti();

	/** Il primo nodo: Inizio la prima volta, Ritorno dalla seconda (se il dialogo non ha un Ritorno, sempre Inizio). */
	VALDORSO_API FName NodoIniziale(const FValdorsoDialogo& Dialogo, bool bGiaParlato);

	/**
	 * Il colono sceglie la risposta numero Scelta nel nodo Nodo. Vero se la scelta è valida: allora OutProssimo è il
	 * nodo dopo (NAME_None = il dialogo finisce) e OutEvento l'evento da annotare (NAME_None = nessuno).
	 */
	VALDORSO_API bool Avanza(const FValdorsoDialogo& Dialogo, FName Nodo, int32 Scelta, FName& OutProssimo, FName& OutEvento);

	/**
	 * (08/11) Le risposte di quel nodo che il personaggio Chi vede (i numeri veri, nell'ordine, al massimo MassimoScelte):
	 * senza quelle NascostaSe di un segno che Chi ha già. Chi vuoto = nessun segno, si vedono tutte.
	 */
	VALDORSO_API TArray<int32> ScelteVisibili(const FValdorsoDialogo& Dialogo, FName Nodo, const FString& Chi);

	/** (08/11) Come sopra, con la memoria di quel personaggio (quella del suo file, o quella per nome). */
	VALDORSO_API TArray<int32> ScelteVisibili(const FValdorsoDialogo& Dialogo, FName Nodo, const FValdorsoMemoriaDialoghi& Memoria);

	/**
	 * La battuta da mandare al PC per quel nodo (vuota con bChiudi se il nodo non c'è). Solo le risposte visibili per
	 * Chi (08/11; Chi vuoto = tutte): il numero che il PC rimanda va riportato con ScelteVisibili.
	 */
	VALDORSO_API FValdorsoBattuta Battuta(const FValdorsoDialogo& Dialogo, FName Nodo, int32 Passo, const FString& Chi = FString());

	/** (08/11) Come sopra, con la memoria di quel personaggio. */
	VALDORSO_API FValdorsoBattuta Battuta(const FValdorsoDialogo& Dialogo, FName Nodo, int32 Passo, const FValdorsoMemoriaDialoghi& Memoria);

	/** I problemi di un dialogo (nodi che mancano, irraggiungibili, senza uscita...), in italiano; vuoto = va bene. */
	VALDORSO_API TArray<FString> Problemi(const FValdorsoDialogo& Dialogo);

	// --- La memoria del server per nome: chi ha già parlato con chi ------------------------------------------------
	// (08/11) Solo per chi non ha un personaggio salvato (accesso libero dell'editor) e per i test: i personaggi veri
	// hanno la memoria nel loro file (FValdorsoMemoriaDialoghi, qui sopra).

	/** Vero se il personaggio Chi ha già parlato con il PNG del dialogo Id (da quando il server è acceso). */
	VALDORSO_API bool GiaParlato(FName Id, const FString& Chi);

	/** Ricorda che il personaggio Chi ha parlato con il PNG del dialogo Id. */
	VALDORSO_API void SegnaParlato(FName Id, const FString& Chi);

	/** (08/11) Vero se il personaggio Chi ha il segno Segno nel dialogo Id (da quando il server è acceso). */
	VALDORSO_API bool HaSegno(FName Id, const FString& Chi, FName Segno);

	/** (08/11) Dà al personaggio Chi il segno Segno nel dialogo Id (niente se Segno è vuoto). */
	VALDORSO_API void MettiSegno(FName Id, const FString& Chi, FName Segno);

	/** Dimentica tutto, anche i segni (per i test). */
	VALDORSO_API void Dimentica();
}
