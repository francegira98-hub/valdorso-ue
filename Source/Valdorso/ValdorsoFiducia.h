// Valdorso - La fiducia dei PNG, la valle che ricorda e la colpa (v0.2.4 "PNG veri", 14/11/2026). Scritto da Claude.
// Approvato da Fra il 14/11 ("Approvo tutto"; "Colpa visibile"; "Parole, non numeri"). Dal Piano, blocchi 1 e 2 della
// v0.2.4, pensati come in SAO (il cursore che diventa arancione), in Overgeared (i PNG che si ricordano di te) e nel
// Cardinal (il mondo che annota tutto e reagisce da solo).
//
// LA FIDUCIA: ogni PNG ha la sua per ogni colono (da -100 a 100), salvata nel file del personaggio con la memoria dei
// dialoghi (FValdorsoRicordoDialogo::Fiducia). Non si vede mai un numero: sotto il nome, sulla pergamena del dialogo, c'e'
// una parola (Diffidente, Neutrale, Cordiale, Amico, Di famiglia), e i saluti cambiano con lei.
// - sale: parlarci (una volta al giorno, fino a Cordiale: oltre servono i fatti), i passi delle storie (ogni segno nuovo
//   di un dialogo);
// - scende: colpirlo (molto), colpire qualcuno davanti a lui (un po').
//
// LA VALLE CHE RICORDA: certi eventi del registro (ValdorsoRegistroEventi) diventano "fatti" del colono (una caccia, una
// storia chiusa, una morte, una porta segreta, una notte in cella...), salvati nel suo file (FValdorsoTaccuino::Fatti).
// Il giorno dopo (fino a tre giorni) ogni PNG che lo incontra ne parla una volta: nel saluto in chat o in cima al dialogo.
//
// LA COLPA (come il cursore arancione di SAO): chi colpisce un PNG diventa colpevole per dieci minuti. Il suo nome sopra
// la testa diventa rosso per tutti (SValdorsoBersaglio), i PNG vicini gridano e scappano, la fiducia scende. Dopo poco le
// guardie della Corona lo prendono e lo portano al posto di guardia (AValdorsoPlayerState): una multa o una notte in
// cella. Tutto resta nel registro del Balivo (il registro eventi: "Colpa", "Multa", "Cella", e nei fatti del colono).
// I PNG non muoiono mai: barcollano, e se li colpisci forte cadono e si rialzano (AValdorsoPNG::Colpito).
//
// Le regole stanno qui, senza mondo ne' rete: le provano i test (Valdorso.PNG.*).

#pragma once

#include "CoreMinimal.h"
#include "ValdorsoTaccuinoDati.h"

class AActor;
class APawn;
class APlayerController;
class AValdorsoPNG;

/** Quanto un PNG si fida di un colono, a parole. */
enum class EValdorsoFiducia : uint8
{
	Diffidente,
	Neutrale,
	Cordiale,
	Amico,
	DiFamiglia,
};

namespace ValdorsoFiducia
{
	inline constexpr int32 Minima = -100;
	inline constexpr int32 Massima = 100;

	/** Parlarci, una volta al giorno (vero, UTC) per PNG: sale fin qui e non oltre (Cordiale; poi servono i fatti). */
	inline constexpr int32 PerChiacchiera = 3;
	inline constexpr int32 TettoChiacchiere = 30;
	/** Un segno nuovo di un dialogo (un passo di una storia, un favore). */
	inline constexpr int32 PerSegno = 4;
	/** Colpito (lui), o qualcuno colpito davanti a lui (entro RaggioTestimoni). */
	inline constexpr int32 PerColpito = -25;
	inline constexpr int32 PerTestimone = -10;

	/** Per quanti giorni (veri) un PNG parla ancora di un fatto; quanti fatti ricorda il file. */
	inline constexpr int32 GiorniRicordo = 3;
	inline constexpr int32 MassimoFatti = 40;

	// --- la colpa (secondi veri, centimetri, Aureus) ---
	inline constexpr float SecondiColpa = 600.f;
	inline constexpr float SecondiPrimaDelleGuardie = 25.f;
	inline constexpr float SecondiDopoLaFuga = 30.f;
	/** Ogni quanto un colpo conta come un'altra colpa (la fiducia, il registro): non a ogni pugno. */
	inline constexpr float SecondiTraColpe = 10.f;
	inline constexpr int32 Multa = 20;
	inline constexpr float SecondiCella = 90.f;
	inline constexpr float RaggioTestimoni = 2000.f;
	inline constexpr float RaggioGrida = 2500.f;
	inline constexpr float RaggioFuga = 1200.f;
	inline constexpr float DistanzaFuga = 650.f;
	inline constexpr float SecondiFuga = 12.f;
	inline constexpr float VelocitaFuga = 420.f;
	/** Fin dove si vede il nome rosso di un colpevole. */
	inline constexpr float RaggioNomeRosso = 3000.f;

	// --- il PNG colpito: barcolla, o cade e si rialza ---
	inline constexpr float SecondiAccumulo = 8.f;
	inline constexpr float SecondiATerra = 2.6f;

	/** Vero se con quel colpo il PNG cade (un colpo forte, o tanti colpi in pochi secondi). */
	VALDORSO_API bool Cade(float DannoAccumulato, float DannoColpo);

	/** La parola di quel valore. */
	VALDORSO_API EValdorsoFiducia Livello(int32 Fiducia);
	VALDORSO_API const TCHAR* Parola(EValdorsoFiducia Livello);
	VALDORSO_API FLinearColor Colore(EValdorsoFiducia Livello);

	/** Il valore dopo un cambiamento (sempre tra Minima e Massima). */
	VALDORSO_API int32 Applica(int32 Fiducia, int32 Delta);

	/** Il valore dopo una chiacchierata (non oltre TettoChiacchiere; chi e' gia' oltre resta dov'e'). */
	VALDORSO_API int32 DopoChiacchiera(int32 Fiducia);

	/** Il numero del giorno (UTC) di un momento in secondi Unix. */
	inline int64 Giorno(int64 Unix) { return Unix >= 0 ? Unix / 86400 : (Unix - 86399) / 86400; }

	// --- nel taccuino (il file del personaggio): vero solo se e' cambiato davvero (allora si salva) ---

	VALDORSO_API int32 Di(const FValdorsoTaccuino& Taccuino, FName IdDialogo);
	VALDORSO_API bool Cambia(FValdorsoTaccuino& Taccuino, FName IdDialogo, int32 Delta);
	/** Una chiacchierata con quel PNG nel giorno Giorno: la prima del giorno alza la fiducia. */
	VALDORSO_API bool Chiacchiera(FValdorsoTaccuino& Taccuino, FName IdDialogo, int64 Giorno);

	/** Vero se quel tipo di evento del registro diventa un fatto che i PNG ricordano. */
	VALDORSO_API bool Memorabile(const FString& Tipo);
	/** Annota un fatto (uno per tipo e per giorno: lo stesso giorno si aggiorna solo l'ora). */
	VALDORSO_API bool AnnotaFatto(FValdorsoTaccuino& Taccuino, const FString& Tipo, int64 Quando);
	/** Il fatto di cui quel PNG parlerebbe adesso: il piu' nuovo dei giorni passati (non di oggi, al massimo GiorniRicordo
	 *  fa) che non ha ancora detto. */
	VALDORSO_API bool FattoDaRicordare(const FValdorsoTaccuino& Taccuino, FName IdDialogo, int64 Adesso, FValdorsoFatto& OutFatto);
	/** Quel PNG ne ha parlato: non lo ripete (e nemmeno quelli piu' vecchi). */
	VALDORSO_API bool SegnaRicordato(FValdorsoTaccuino& Taccuino, FName IdDialogo, int64 Quando);
	/** La riga che dice il PNG ("Ieri la caccia ti e' andata bene..."); vuota se il tipo non si ricorda. */
	VALDORSO_API FString RigaDelFatto(const FValdorsoFatto& Fatto, int64 Adesso, int32 Seme);
	/** Rimette in ordine i fatti letti da un file: tipi conosciuti, uno per tipo e giorno, al massimo MassimoFatti. */
	VALDORSO_API void SistemaFatti(TArray<FValdorsoFatto>& Fatti);

	/** Un saluto che dipende dalla fiducia (vuoto = vale quello solito del PNG). Nome = il nome del colono. */
	VALDORSO_API FString SalutoDellaFiducia(EValdorsoFiducia Livello, const FString& Nome, int32 Seme);
	/** Il saluto a chi e' colpevole adesso. */
	VALDORSO_API FString SalutoAlColpevole(int32 Seme);
	/** Quello che grida un PNG colpito (bVittima), una guardia o chi vede (bGuardia falso). */
	VALDORSO_API FString Grido(bool bVittima, bool bGuardia, int32 Seme);

	// --- sul server ---

	/** Il registro ha annotato un evento: se e' memorabile, diventa un fatto del colono Chi (lo chiama Annota). */
	VALDORSO_API void DopoAnnota(const FString& Tipo, const AActor* Chi);

	/** Cambia la fiducia del PNG di quel dialogo verso il colono di quel corpo. */
	VALDORSO_API void CambiaPer(const APawn* Colono, FName IdDialogo, int32 Delta);

	/** La fiducia del PNG di quel dialogo verso il colono di quel PlayerController. */
	VALDORSO_API int32 DiColono(const APlayerController* Controllore, FName IdDialogo);

	/** Un colono ha colpito un PNG: la colpa, la fiducia, i testimoni che gridano e scappano, le guardie. */
	VALDORSO_API void Crimine(APawn* Colono, AValdorsoPNG* Vittima);

	/** Le risposte del dialogo dell'arresto: "Multa" (si paga; senza monete si va a "non_basta") e "Cella". Vero se
	 *  l'evento era suo. */
	VALDORSO_API bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Png, FName& InOutProssimo);

	/** Il dialogo dell'arresto (ValdorsoDialoghiTesti.inl). */
	inline const TCHAR* DialogoArresto = TEXT("Arresto");
}
