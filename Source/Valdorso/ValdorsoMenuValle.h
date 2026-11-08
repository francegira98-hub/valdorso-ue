// Valdorso - Il menu nella valle vera al tramonto (v0.1.3). Scritto da Claude il 08/11/2026.
//
// Dalla roadmap v0.1.3: "menu con la valle vera al tramonto e lo stato del mondo". Come funziona:
// - Il gioco parte sempre da Lvl_Menu (la mappa d'avvio non cambia). Il controllore del menu (AValdorsoMenuController),
//   se trova la valle (/Game/Prove/L_Valle) e il comando Valdorso.Menu.Valle vale 1, mette il nero con la riga
//   "La valle si sveglia..." e apre la valle da solo, senza server, con la modalità di gioco qui sotto
//   (AValdorsoMenuValleGameMode): nessun personaggio, nessun HUD, il cielo fermo al tramonto (Valdorso.Menu.Ora, di
//   serie le 18: nel nostro cielo il sole tocca l'orizzonte alle 18, con metà luce, e si spegne alle 18,15; le rune
//   battono quando il sole cala, alle 18 a metà), il tempo sereno, le fasi del villaggio, i suoni della valle.
// - Nella valle lo stesso controllore mostra l'avvio e il menu come prima (con lo stato del mondo in basso) e fa volare
//   una telecamera in un giro lento intorno al villaggio: circa 90 secondi per giro, sempre 33-53 m sopra il terreno.
//   Le tappe sono qui (ValdorsoMenuValle::Tappe): prese dai dati del villaggio (Villaggio_3000.json, Luoghi_3000.json:
//   la piazza è in (165300, 192228) nel file, cioè (15300, 42228) nel mondo, quota 5517 cm) e controllate sulla mappa
//   delle altezze (Altezze_3000.png).
//   Per ogni tappa c'è anche il suolo più alto dei due tratti vicini: il test controlla che la curva gli stia sopra.
//   In gioco una sonda guarda sotto la telecamera: se un albero o un tetto è a meno di 20 m, la telecamera sale piano.
// - Il nero si apre quando sotto la telecamera c'è la valle (World Partition carica i pezzi intorno alla telecamera,
//   che è la vista del controllore), al massimo dopo 25 secondi.
// - Con "Meno movimento sullo schermo" (Impostazioni, Accessibilità) la telecamera va a un terzo della velocità.
// - "Entra nella valle" e il volo lungo del primo ingresso partono dalla telecamera del giro (qui VoloLungo).
// - Se la valle manca o non si apre, si resta nella scena di sempre (Lvl_Menu): niente cambia.
// Comandi di console: Valdorso.Menu.Valle 0 (la scena di sempre), Valdorso.Menu.Ora 18.1 (un'altra ora del tramonto).

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoMenuGameMode.h"
#include "ValdorsoMenuValle.generated.h"

/** Una tappa del giro: dove sta la telecamera, dove guarda, e il suolo più alto dei due tratti vicini (cm del mondo). */
struct FValdorsoTappaMenu
{
	FVector Posizione = FVector::ZeroVector;
	FVector Sguardo = FVector::ZeroVector;
	float Suolo = 0.f;
};

namespace ValdorsoMenuValle
{
	/** Quanto dura un giro della telecamera, in secondi. */
	inline constexpr float SecondiGiro = 90.f;

	/** Con "Meno movimento sullo schermo" la telecamera va a questa frazione della velocità. */
	inline constexpr float FattoreMenoMovimenti = 0.3f;

	/** Le tappe stanno almeno così sopra il suolo (cm): lo controlla il test. */
	inline constexpr float MinimoSopraSuolo = 2500.f;

	/** In gioco: se sotto la telecamera c'è qualcosa più vicino di così (cm), la telecamera sale. */
	inline constexpr float MinimoSicuro = 2000.f;

	/** ...ma al massimo di così (cm): la sonda non può far scappare la telecamera in cielo. */
	inline constexpr float AlzataMassima = 4000.f;

	/** Il centro della piazza del villaggio (cm del mondo, Luoghi_3000.json). */
	VALDORSO_API FVector Piazza();

	/** Le tappe del giro intorno al villaggio, in ordine (si torna dalla prima dopo l'ultima). */
	VALDORSO_API const TArray<FValdorsoTappaMenu>& Tappe();

	/** La curva di Catmull-Rom tra P1 (T = 0) e P2 (T = 1). */
	VALDORSO_API FVector CatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);

	/** Un punto della curva chiusa che passa per i Punti: Chiave 0 = il primo, 1 = il secondo... N = di nuovo il primo. */
	VALDORSO_API FVector SuGiro(const TArray<FVector>& Punti, float Chiave);

	/** La velocità della telecamera (cm al secondo) per fare un giro lungo Lunghezza in Secondi. */
	VALDORSO_API float Velocita(float Lunghezza, float Secondi, bool bMenoMovimenti);

	/** Quanto deve salire la telecamera (cm, 0 = niente) se sotto di lei c'è qualcosa alla quota QuotaSotto. */
	VALDORSO_API float Alzata(float QuotaTelecamera, float QuotaSotto, float Minimo);

	/** I punti del volo lungo del primo ingresso nella valle: dalla Partenza, sopra la piazza, poi su verso i monti. */
	VALDORSO_API void VoloLungo(const FVector& Partenza, TArray<FVector>& Out);

	/** Valdorso.Menu.Valle: vero se il menu deve andare nella valle vera (di serie sì). */
	VALDORSO_API bool Attiva();

	/** Valdorso.Menu.Ora: l'ora del tramonto del menu (0..24). */
	VALDORSO_API float OraDelMenu();

	/** Le opzioni per aprire la valle con la modalità del menu ("game=/Script/Valdorso.ValdorsoMenuValleGameMode"). */
	VALDORSO_API FString OpzioniLivello();
}

/** Il giro della telecamera: le tappe chiuse in un anello, percorse a velocità costante. */
struct VALDORSO_API FValdorsoGiroMenu
{
	/** Prepara il giro (misura la curva a pezzetti, PassiPerTratto per ogni tratto tra due tappe). */
	void Prepara(const TArray<FValdorsoTappaMenu>& Tappe, int32 PassiPerTratto = 32);

	/** La lunghezza di un giro, in centimetri. */
	float Lunghezza() const { return Totale; }

	int32 NumeroTappe() const { return Posizioni.Num(); }

	/** La chiave della curva (0..N) dopo Distanza centimetri dalla prima tappa (oltre un giro si ricomincia). */
	float Chiave(float Distanza) const;

	/** Dove sta la telecamera e dove guarda dopo Distanza centimetri dalla prima tappa. */
	void Posa(float Distanza, FVector& OutPosizione, FVector& OutSguardo) const;

private:
	TArray<FVector> Posizioni;
	TArray<FVector> Sguardi;
	/** La distanza dalla prima tappa a ogni pezzetto (N * Passi + 1 numeri, l'ultimo è il giro intero). */
	TArray<float> Distanze;
	int32 Passi = 32;
	float Totale = 0.f;
};

/**
 * La modalità di gioco del menu nella valle: come quella del menu (nessun personaggio, solo il controllore del menu),
 * più il cielo fermo al tramonto, il tempo sereno, le fasi del villaggio, le rune e i suoni della valle.
 * Si usa solo aprendo la valle dal menu (UGameplayStatics::OpenLevel con ValdorsoMenuValle::OpzioniLivello()).
 */
UCLASS()
class VALDORSO_API AValdorsoMenuValleGameMode : public AValdorsoMenuGameMode
{
	GENERATED_BODY()

public:
	AValdorsoMenuValleGameMode();

	virtual void StartPlay() override;
};
