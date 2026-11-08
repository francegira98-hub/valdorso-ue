// Valdorso - La chat di prossimità (v0.1.4 "Pericoli", prima parte). Scritto da Claude il 08/11/2026.
//
// Come funziona:
// - UValdorsoChatComponent sta sul PlayerController (uno per giocatore), come il taccuino.
// - Invio apre una riga in basso a sinistra (SValdorsoRigaChat, SValdorsoChat.h); Invio manda la frase, Esc annulla.
//   Mentre si scrive i tasti vanno solo alla riga (il colono non si muove e non salta); chiusa la riga si torna al gioco.
//   Invio apre la riga solo in gioco, senza schermate aperte: dentro la modalità foto (dove Invio scatta) e dentro i
//   dialoghi (dove Invio sceglie) i tasti li prendono quelle schermate.
// - Il server non si fida: pulisce la frase (via i caratteri di controllo e quelli invisibili, spazi in fila ridotti a
//   uno, al massimo 200 caratteri), accetta al massimo una frase al secondo da ogni giocatore e la manda a tutti i
//   giocatori il cui colono è entro 30 metri da chi parla (lui compreso). "/grida testo": entro 80 metri.
// - Ogni frase passa dal registro eventi ("Chat", con il testo).
// - Le frasi si leggono in basso a sinistra come "Nome: testo" (il nome del personaggio, quello del PlayerState);
//   si tengono le ultime 8 e ognuna sfuma via dopo 20 secondi (mentre si scrive si rivedono tutte). Spariscono con le
//   schermate aperte, nella modalità foto e nel taccuino (come la bussola).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoChat.generated.h"

class APlayerController;
class SWidget;
class SValdorsoRigaChat;

/** Una frase arrivata (o un avviso di questo PC, con Nome vuoto). */
struct FValdorsoFraseChat
{
	FString Nome;
	FString Testo;
	bool bGrida = false;
	/** Quando è arrivata (FPlatformTime::Seconds). */
	double Quando = 0.0;
};

namespace ValdorsoChat
{
	/** Al massimo tanti caratteri per frase. */
	inline constexpr int32 LunghezzaMassima = 200;
	/** Fin dove arriva la voce, e il grido (cm). */
	inline constexpr float RaggioVoce = 3000.f;
	inline constexpr float RaggioGrido = 8000.f;
	/** Una frase al secondo: il PC aspetta 1 s; il server ne accetta una ogni 0,75 s (la rete può avvicinarle). */
	inline constexpr double SecondiTraFrasi = 1.0;
	inline constexpr double SecondiTraFrasiServer = 0.75;
	/** Quante frasi si tengono sullo schermo, quanto restano e in quanto sfumano. */
	inline constexpr int32 FrasiTenute = 8;
	inline constexpr double SecondiVisibile = 20.0;
	inline constexpr double SecondiSfumatura = 2.0;

	/**
	 * La frase pulita: senza caratteri di controllo né invisibili (a capo, tabulazioni, direzione del testo...), gli
	 * spazi in fila ridotti a uno, senza spazi all'inizio e alla fine, al massimo LunghezzaMassima caratteri.
	 */
	VALDORSO_API FString Pulisci(const FString& Testo);

	/** Se la frase comincia con "/grida", lo toglie e restituisce vero (gridare arriva più lontano). */
	VALDORSO_API bool TogliGrido(FString& InOutTesto);

	/** Vero se può parlare di nuovo: dall'ultima frase (Ultima, secondi) sono passati almeno Intervallo secondi. */
	VALDORSO_API bool PuoParlare(double Ultima, double Adesso, double Intervallo = SecondiTraFrasi);

	/** Vero se chi sta in A sente chi parla in Da (o grida). */
	VALDORSO_API bool Sente(const FVector& Da, const FVector& A, bool bGrida);

	/** Quanto si vede una frase arrivata Eta secondi fa (1 per 20 secondi, poi sfuma in 2). */
	VALDORSO_API float Presenza(double Eta);

	/** La riga da leggere: "Nome: testo", "Nome grida: testo", o solo il testo per gli avvisi di questo PC. */
	VALDORSO_API FString Riga(const FValdorsoFraseChat& Frase);

	/** Aggiunge una frase tenendo solo le ultime FrasiTenute. */
	VALDORSO_API void Aggiungi(TArray<FValdorsoFraseChat>& Elenco, const FValdorsoFraseChat& Frase);
}

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoChatComponent();

	/** La chat di quel PlayerController (nullptr se non ce l'ha). */
	static UValdorsoChatComponent* Di(const APlayerController* Controllore);

	// --- Sul PC del giocatore -----------------------------------------------------------------------

	/** Invio: apre la riga per scrivere (falso se non si può). */
	bool ApriRiga();

	/** Chiude la riga; se non c'è un'altra schermata aperta, si torna al gioco. */
	void ChiudiRiga();

	/** Vero mentre la riga è aperta. */
	bool RigaAperta() const { return Riga.IsValid(); }

	/** Dalla riga: Invio con questo testo (vuoto = niente). */
	void Invia(const FString& Testo);

	/** Le frasi da leggere (le ultime 8). */
	const TArray<FValdorsoFraseChat>& Frasi() const { return Elenco; }

	/** Cresce a ogni frase nuova: lo schermo lo guarda per rifarsi. */
	int32 Cambiamenti() const { return NumeroCambiamenti; }

	// --- In rete ------------------------------------------------------------------------------------

	/** La frase scritta (il server la pulisce e la manda a chi è vicino). */
	UFUNCTION(Server, Reliable)
	void ServerDici(const FString& Testo);

	/** Una frase sentita. */
	UFUNCTION(Client, Reliable)
	void ClientSenti(const FString& Nome, const FString& Testo, bool bGrida);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void AggiungiFrase(const FString& Nome, const FString& Testo, bool bGrida);
	void MettiPannello();
	void TogliPannello();

	TArray<FValdorsoFraseChat> Elenco;
	int32 NumeroCambiamenti = 0;
	TSharedPtr<SWidget> Pannello;
	TSharedPtr<SValdorsoRigaChat> Riga;
	TSharedPtr<SWidget> RigaContenitore;
	/** PC: l'ultima frase mandata; server: l'ultima accettata (FPlatformTime::Seconds). */
	double UltimaInviata = -1000.0;
	double UltimaAccettata = -1000.0;
};
