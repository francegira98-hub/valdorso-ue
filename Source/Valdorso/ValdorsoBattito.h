// Valdorso - Le storie del Battito nel gioco (v0.2.4 "PNG veri", blocco 3, 14/11/2026). Scritto da Claude.
// Dal Design ("Il Battito come Cardinal 2.0", la finestra delle storie) e dal Piano; idee di Fra del 13/11: "si innesca,
// esce al centro dello schermo, accetta o rifiuta, e anche rifiutare ha conseguenze; serie, trame" (come in Overgeared).
//
// Come funziona:
// - Il Sognatore (Tools/Sognatore/sognatore.py) scrive le storie fuori dal gioco, lo staff le approva, e finiscono in
//   Saved/Server/Sognatore/Approvate/<colono>/<id>.json. Il server (UValdorsoBattito, un sottosistema del mondo) le legge
//   per ogni colono in gioco, ogni minuto.
// - L'innesco: quando succede quello che la storia aspetta (un dialogo con quel PNG, un luogo, magari di notte, un evento
//   del registro, un sogno, la fine di un'altra storia), al colono si apre la FINESTRA al centro dello schermo
//   (SValdorsoFinestraStoria): rarita' e colore, stelle, titolo, filo e capitolo, trama, primo passo, premi (dalle Rare
//   in parte "???"), "Se rifiuti", scadenza; Accetta / Ci penso / Rifiuta. "Ci penso" la tiene fino alla scadenza: la
//   finestra torna quando l'innesco torna (o al prossimo ingresso).
// - I passi (i "mattoni"): parla, porta, vai, notte, veglia, ascolta, siedi, dormi, leggi, rovista, caccia, raccogli,
//   scegli (al PNG: le risposte sono i finali) e i primi tre nuovi: OFFERTA (lasciare un oggetto in un luogo), SEGUI
//   (una lucina che si muove: stalle dietro per un po') e FANTASMA (uno spirito che vede e sente solo il colono). Le
//   storie che usano mattoni che non ci sono ancora (tasca, eco, png nuovo, lettere, stagioni) restano in attesa.
// - I premi: monete, oggetti (dalle storie Insolite in su con il loro grado: Buona, Rara, Epica, Leggendaria e la
//   provenienza "Dono del Battito"), segni, iscrizioni nel taccuino, titoli; gli effetti (fiducia dei PNG, voci).
// - Com'e' andata: il gioco lo scrive in Saved/Server/Sognatore/Esiti_dal_gioco.jsonl e il Sognatore lo legge da solo
//   (al posto di "segna com'e' andata"). Nel file del personaggio resta lo stato (FValdorsoStoriaViva).
// Comandi: Valdorso.Battito.Storie (le storie del colono e a che punto sono), Valdorso.Battito.Offri <id> (apre la
// finestra di una storia, per le prove), Valdorso.Battito.Rileggi (rilegge subito i file), Valdorso.Battito.Dimentica
// (le storie ripartono da capo). Tre storie di prova: Tools/Sognatore/"Sognatore - storie di prova nel gioco.cmd".

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "ValdorsoBattito.generated.h"

class AActor;
class APawn;
class APlayerController;
class AValdorsoPNG;
class AValdorsoLuceBattito;
struct FValdorsoStoriaViva;

/** A che punto e' una storia per un colono (FValdorsoStoriaViva::Stato). */
enum class EValdorsoStatoStoria : uint8
{
	Nessuna = 0,
	Offerta = 1,
	Accettata = 2,
	Finita = 3,
	Rifiutata = 4,
	Scaduta = 5,
};

/** La finestra al centro dello schermo (viaggia dal server al PC del colono). */
USTRUCT()
struct FValdorsoFinestraStoria
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString Intestazione;

	/** "comune", "insolita", "rara", "epica", "leggendaria". */
	UPROPERTY()
	FString Rarita;

	/** Il colore della rarita' ("#4a6fa5"). */
	UPROPERTY()
	FString Colore;

	UPROPERTY()
	int32 Stelle = 1;

	UPROPERTY()
	FString Titolo;

	UPROPERTY()
	FString Trama;

	UPROPERTY()
	FString PrimoPasso;

	UPROPERTY()
	FString Premi;

	UPROPERTY()
	FString SeRifiuti;

	UPROPERTY()
	int32 ScadenzaGiorni = 7;

	/** "ranieri, capitolo 2" (vuoto se non e' di un filo). */
	UPROPERTY()
	FString Serie;
};

/** (14/11) Una storia come si vede nel taccuino (scheda Storie): la manda il server, solo al suo colono. */
USTRUCT()
struct FValdorsoStoriaNelTaccuino
{
	GENERATED_BODY()

	UPROPERTY()
	FString Titolo;

	UPROPERTY()
	FString Rarita;

	UPROPERTY()
	FString Colore;

	/** EValdorsoStatoStoria. */
	UPROPERTY()
	uint8 Stato = 0;

	/** Il passo di adesso (in corso), la scadenza (da decidere), com'e' finita (finite). */
	UPROPERTY()
	FString Testo;

	/** A che passo (da 1) e quanti sono (in corso). */
	UPROPERTY()
	int32 Passo = 0;

	UPROPERTY()
	int32 Passi = 0;

	bool operator==(const FValdorsoStoriaNelTaccuino& Altra) const
	{
		return Titolo == Altra.Titolo && Rarita == Altra.Rarita && Colore == Altra.Colore && Stato == Altra.Stato && Testo == Altra.Testo
			&& Passo == Altra.Passo && Passi == Altra.Passi;
	}
};

/** Una storia scritta dal Sognatore, letta dal file (solo sul server). */
struct FValdorsoStoriaScritta
{
	struct FInnesco
	{
		FString Tipo, Png, Luogo, Evento, Ora, Storia, Quando;
	};
	struct FPasso
	{
		FString Mattone, Testo, Png, Luogo, Oggetto, Animale;
	};
	struct FEffetto
	{
		FString Tipo, Png, Testo;
		int32 Quanto = 0;
	};
	struct FFinale
	{
		FString Scelta, Esito;
		int32 Monete = 0;
		TArray<FString> Oggetti;
		FString Segno, Iscrizione, Titolo;
		TArray<FEffetto> Effetti;
	};

	FString Id, Seme, Colono, Titolo, Rarita, Png, Luogo, Aggancio, RifiutoTesto;
	int32 Difficolta = 1;
	TArray<FInnesco> Inneschi;
	TArray<FPasso> Passi;
	TArray<FFinale> Finali;
	TArray<FEffetto> RifiutoEffetti;
	FValdorsoFinestraStoria Finestra;
};

namespace ValdorsoBattito
{
	/** Per quanto si veglia (o si ascolta) un luogo, e per quanto si segue la lucina (secondi veri). */
	inline constexpr float SecondiVeglia = 120.f;
	inline constexpr float SecondiSegui = 45.f;
	/** Quanto vicino si deve stare alla lucina, e allo spirito per sentirlo (cm). */
	inline constexpr float VicinoLuce = 900.f;
	inline constexpr float VicinoSpirito = 350.f;
	/** Quanto aspetta una finestra lasciata con "Ci penso" prima di tornare con lo stesso innesco (secondi veri). */
	inline constexpr double SecondiTraOfferte = 600.0;

	/** Legge una storia approvata (il JSON del Sognatore). Falso con il motivo se non si legge. */
	VALDORSO_API bool Leggi(const FString& Json, FValdorsoStoriaScritta& Out, FString& OutErrore);

	/** Vero se il gioco sa gia' fare quel mattone (gli altri tengono la storia in attesa). */
	VALDORSO_API bool MattoneOggi(const FString& Mattone);

	/** Vero se la storia si puo' giocare adesso (ha passi e finali, tutti i mattoni ci sono). */
	VALDORSO_API bool Giocabile(const FValdorsoStoriaScritta& Storia);

	/** Il dialogo del PNG con quel nome nelle storie ("Nonna Edda" -> NonnaEdda); NAME_None se non c'e'. */
	VALDORSO_API FName DialogoDelPng(const FString& Nome);

	/** Il PNG che "fa da luogo" quando il luogo delle storie non e' nel taccuino ("Locanda di Bruno" -> Bruno). */
	VALDORSO_API FName PngDelLuogo(const FString& Luogo);

	/** Il nome del luogo nel taccuino ("Pozzo" -> "Piazza del villaggio"); vuoto se non c'e'. */
	VALDORSO_API FString LuogoDelTaccuino(const FString& Luogo);

	/** Il grado degli oggetti in premio per quella rarita' (Comune, Buona, Rara, Epica, Leggendaria). */
	VALDORSO_API uint8 QualitaDelPremio(const FString& Rarita);

	/** La scadenza (giorni) per quella rarita' se la finestra non la dice. */
	VALDORSO_API int32 ScadenzaDi(const FString& Rarita);

	/** "Rara" (con la maiuscola); "Comune" se non si conosce. */
	VALDORSO_API FString NomeRarita(const FString& Rarita);

	/** Il colore della rarita' (dalla finestra; se manca, quello del Sognatore). */
	VALDORSO_API FLinearColor ColoreRarita(const FString& Rarita, const FString& Esadecimale);

	/** Vero se un innesco non legato a un luogo vale per questo evento del registro (dialogo, evento, sogno). */
	VALDORSO_API bool InnescoDaEvento(const FValdorsoStoriaScritta::FInnesco& Innesco, const FString& Tipo, FName DialogoPng);

	/** Il nome del file della cartella del colono, come lo scrive il Sognatore (lettere, cifre, _ - e spazio). */
	VALDORSO_API FString NomeCartella(const FString& Colono);

	/** Il passo da mostrare al colono ("Le lucine della palude: Segui le lucine...", con un aiuto per lo "scegli"). */
	VALDORSO_API FString TestoPasso(const FValdorsoStoriaScritta& Storia, int32 Passo);
}

/**
 * Il Battito nel mondo: solo sul server. Ogni secondo guarda i coloni (luoghi, notte, zaino, la lucina e lo spirito);
 * ascolta il registro eventi (DopoAnnota) per i dialoghi, le cacce, i sogni...
 */
UCLASS()
class VALDORSO_API UValdorsoBattito : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UValdorsoBattito* Di(const UObject* QualcosaNelMondo);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Il registro eventi ha annotato qualcosa (lo chiama ValdorsoRegistroEventi::Annota, sul server). */
	static void DopoAnnota(const FString& Tipo, const AActor* Chi, const AActor* Bersaglio, const FString& Dettagli);

	/** Il colono parla con quel PNG: se una sua storia aspetta la scelta da lui, si apre quella (vero) al posto del dialogo. */
	bool ApriScelta(AValdorsoPNG* Png, APawn* Colono);

	/** La risposta alla scelta di una storia (il numero del finale, da 0): vero con il testo di com'e' andata. */
	bool Scegli(APlayerController* Chi, const FString& IdStoria, int32 Indice, FString& OutEsito);

	/** La risposta alla finestra: 0 Accetta, 1 Rifiuta, 2 Ci penso. */
	void Risposta(APlayerController* Chi, const FString& IdStoria, uint8 Cosa);

	/** Per le prove (console). */
	void Rileggi();
	void Dimentica(APlayerController* Chi);
	FString Elenco(APlayerController* Chi);
	bool OffriPerProva(APlayerController* Chi, const FString& IdStoria);

private:
	struct FColono
	{
		TArray<FValdorsoStoriaScritta> Storie;
		double LettoAlle = -1000.0;
		bool bPrimoGiro = true;
		bool bFinestraAperta = false;
		/** Quando si e' aperta l'ultima finestra (se il PC non risponde, dopo cinque minuti si va avanti lo stesso). */
		double FinestraDa = 0.0;
		/** Quanto si e' vegliato o seguito al passo di adesso, per storia (secondi). */
		TMap<FString, float> Progresso;
		/** Quando e' stata offerta l'ultima volta una storia (secondi del mondo). */
		TMap<FString, double> UltimaOfferta;
		TWeakObjectPtr<AValdorsoLuceBattito> Luce;
		FString LuceDi;
	};

	void Giro();
	FColono& ColonoDi(APlayerController* Chi);
	/** La scheda Storie del taccuino di quel colono (va in rete solo se e' cambiata). */
	void AggiornaPagine(APlayerController* Chi, FColono& Suo);
	void Carica(APlayerController* Chi, FColono& Suo);
	const FValdorsoStoriaScritta* Scritta(FColono& Suo, const FString& Id) const;

	/** Il colono sta in quel luogo delle storie (del taccuino, o vicino al PNG che fa da luogo). */
	bool Dentro(const APawn* Colono, const FString& Luogo) const;
	bool Notte() const;

	bool Offri(APlayerController* Chi, FColono& Suo, const FValdorsoStoriaScritta& Storia);
	/** Il passo dopo (solo se la storia e' ancora al passo PassoVisto: un evento dentro un altro non la fa saltare due volte). */
	void Avanza(APlayerController* Chi, FColono& Suo, const FValdorsoStoriaScritta& Storia, int32 PassoVisto);
	void Finisci(APlayerController* Chi, FColono& Suo, const FValdorsoStoriaScritta& Storia, int32 Finale);
	void Chiudi(APlayerController* Chi, FColono& Suo, const FValdorsoStoriaScritta& Storia, EValdorsoStatoStoria Come);
	void Effetti(APlayerController* Chi, const TArray<FValdorsoStoriaScritta::FEffetto>& Quali, const FValdorsoStoriaScritta& Storia);
	void TogliLuce(FColono& Suo);
	void Racconta(APlayerController* Chi, const FString& Nome, const FString& Testo);
	void ScriviEsito(const FString& Colono, const FString& Id, const FString& Cosa);

	/** Gli eventi del registro per questo colono. */
	void Evento(APlayerController* Chi, const FString& Tipo, FName DialogoPng, const FString& Dettagli);

	TMap<TWeakObjectPtr<APlayerController>, FColono> Coloni;
	FTimerHandle TimerGiro;
};
