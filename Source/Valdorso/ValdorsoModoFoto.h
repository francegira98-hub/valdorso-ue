// Valdorso - La modalità foto (07/11/2026, v0.1.3, scritta da Claude mentre Fra dormiva).
//
// Dal piano: telecamera libera, niente scritte sullo schermo, foto grandi, per il Kickstarter e il trailer.
// Si apre con P (o con il comando Valdorso.Foto) quando si è nella valle e non c'è un'altra schermata aperta.
//   - (Nascondere il colono con C vale solo su questo PC quando si gioca collegati a un server; in locale da soli
//     è comunque solo per chi gioca.)
//   - Il colono resta fermo dov'è; una telecamera si stacca da lui e vola libera, fino a 30 m di distanza (non si va
//     a curiosare lontano: in un gioco online sarebbe un vantaggio) e senza passare attraverso muri e rocce.
//   - Tasti: W A S D per muoversi, Q ed E per scendere e salire, il mouse per guardare, Maiusc veloce, Alt piano;
//     la rotella avvicina e allontana (lo zoom); Z e X inclinano l'immagine; R rimette dritto e lo zoom normale;
//     F sfoca lo sfondo mettendo a fuoco quello che c'è al centro; G la griglia dei terzi per comporre la foto;
//     C nasconde o mostra il colono; H nasconde o mostra l'aiuto; Spazio (o Invio) scatta la foto; Esc o P esce.
//   - Controller: levetta sinistra muoversi, destra guardare, grilletti su e giù, dorsali inclinare, croce su e giù lo
//     zoom, A scatta, B esce, Y nasconde l'aiuto.
//   - La foto è grande il doppio dello schermo (con 1920 x 1080 viene 3840 x 2160), senza scritte: la fa Unreal
//     (HighResShot) e la salva in Saved/Screenshots. Il moltiplicatore si cambia con Valdorso.Foto.Grandezza 3.
// Tutto succede solo su questo PC: niente va al server, e gli altri vedono il colono fermo.
//
// (08/11) Lo strumento per il trailer (ValdorsoTrailer.h), dentro la modalità foto:
//   - K mette una tappa del percorso dove sta la telecamera (posizione, sguardo con l'inclinazione, zoom e, se lo sfondo
//     è sfocato, la distanza del fuoco): "Tappa 3 aggiunta". Backspace toglie l'ultima.
//   - L fa percorrere il percorso alla telecamera, da sola, a velocità costante con partenza e arrivo dolci; L di nuovo
//     la ferma (e da lì si torna a volare a mano). Mentre va, l'aiuto, la griglia e i messaggi spariscono (per OBS).
//   - B le bande nere del cinema (2,39 a 1); 1 2 3 4 5 la velocità (2, 4, 8, 15, 30 m/s); O il giro (dopo l'ultima
//     tappa torna alla prima e ricomincia, senza fermarsi).
//   - Anche percorrendo la strada valgono le regole del volo a mano: mai più di 30 m dal colono e mai attraverso muri e
//     rocce (la stessa sfera di 15 cm). In un gioco online la telecamera libera non deve servire a guardare oltre un muro
//     o a curiosare lontano: una tappa caricata da un file e messa più in là si ferma al limite.
//   - Console (con la modalità foto chiusa, perché aperta prende tutti i tasti): Valdorso.Trailer.Salva <nome>,
//     Valdorso.Trailer.Carica <nome>, Valdorso.Trailer.Elenco, Valdorso.Trailer.Svuota. File in Saved/Trailer/<nome>.json;
//     chiudendo la modalità foto il percorso si salva in Saved/Trailer/ultimo.json e alla prima apertura si rilegge.
//   - Valdorso.Trailer.Fotogrammi 1 (solo senza server): mentre la telecamera percorre la strada il tempo del gioco va
//     a passi fissi di 1/30 di secondo e ogni fotogramma si salva in Saved/Trailer/<nome>/fotogramma_00000.png (senza le
//     scritte e senza le bande); alla fine tutto torna com'era. Di serie è spento: per il trailer basta registrare con OBS.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "ValdorsoTrailer.h"
#include "ValdorsoModoFoto.generated.h"

class APlayerController;
class FValdorsoCaratteri;
class SValdorsoFoto;

/** Quello che il giocatore sta premendo (lo scrive lo schermo della foto, lo legge la telecamera). */
struct FValdorsoComandiFoto
{
	bool bAvanti = false;
	bool bIndietro = false;
	bool bSinistra = false;
	bool bDestra = false;
	bool bSu = false;
	bool bGiu = false;
	bool bVeloce = false;
	bool bPiano = false;
	bool bInclinaSinistra = false;
	bool bInclinaDestra = false;
	/** Mouse: quanto si è mosso dall'ultimo fotogramma (pixel); la telecamera lo consuma. */
	FVector2D Sguardo = FVector2D::ZeroVector;
	/** Rotella e croce: scatti di zoom da consumare (+ avvicina). */
	float Zoom = 0.f;
	/** Controller (-1..1). */
	FVector2D LevettaMuovi = FVector2D::ZeroVector;
	FVector2D LevettaGuarda = FVector2D::ZeroVector;
	float GrillettoSu = 0.f;
	float GrillettoGiu = 0.f;
};

namespace ValdorsoFoto
{
	/** Vero se su questo controllore la modalità foto è aperta. */
	VALDORSO_API bool Aperta(const APlayerController* Controllore);

	/** Apre la modalità foto (se il colono c'è); falso se non si può. */
	VALDORSO_API bool Apri(APlayerController* Controllore);

	/** La chiude e rimette tutto com'era. */
	VALDORSO_API void Chiudi(APlayerController* Controllore);

	/** Il punto dove può stare la telecamera: al massimo Raggio dal colono (Centro). */
	VALDORSO_API FVector DentroIlRaggio(const FVector& Centro, const FVector& Voluto, float Raggio);

	/** Il campo visivo dopo uno zoom (Scatti positivi = più vicino), tra 15 e 100 gradi. */
	VALDORSO_API float CampoDopoZoom(float Campo, float Scatti);
}

/** La telecamera libera della modalità foto (solo su questo PC). */
UCLASS(NotPlaceable, Transient)
class VALDORSO_API AValdorsoCameraFoto : public ACameraActor
{
	GENERATED_BODY()

public:
	AValdorsoCameraFoto();

	/** Velocità normale (cm/s); Maiusc la quadruplica, Alt la divide per quattro. */
	UPROPERTY(EditAnywhere, Category = "Foto")
	float Velocita = 450.f;

	/** Fin dove si può allontanare dal colono (cm). */
	UPROPERTY(EditAnywhere, Category = "Foto")
	float RaggioMassimo = 3000.f;

	/** Gradi per pixel del mouse. */
	UPROPERTY(EditAnywhere, Category = "Foto")
	float SensibilitaMouse = 0.12f;

	/** Gradi al secondo con la levetta. */
	UPROPERTY(EditAnywhere, Category = "Foto")
	float VelocitaLevetta = 110.f;

	FValdorsoComandiFoto Comandi;

	/** Il colono da cui non ci si allontana (e che si può nascondere). */
	TWeakObjectPtr<APawn> Colono;

	void Prepara(APawn* InColono, const FVector& Dove, const FRotator& Verso, float Campo);
	void RimettiDritto();
	void AlternaSfocatura();
	void AlternaColono();
	bool Sfocatura() const { return bSfocatura; }
	bool ColonoNascosto() const { return bColonoNascosto; }
	float CampoVisivo() const;
	float Inclinazione() const { return Rollio; }
	/** Scatta: restituisce dove finirà la foto (la cartella). */
	FString Scatta();
	/** Rimette visibile il colono (all'uscita). */
	void MostraColono();

	/** (08/11) Il trailer: mette una tappa dove sta la telecamera; falso se ce ne sono già troppe. OutNumero: quante adesso. */
	bool AggiungiTappa(int32& OutNumero);
	/** Toglie l'ultima tappa; falso se non ce n'erano. OutRimaste: quante ne restano. */
	bool TogliTappa(int32& OutRimaste);
	/** Parte lungo il percorso; falso (con il motivo in OutMessaggio) se non si può. OutMessaggio può avere anche un avviso. */
	bool AvviaPercorso(FText& OutMessaggio);
	/** Si ferma dove si trova (da lì si vola di nuovo a mano). */
	void FermaPercorso();
	/** Rifà la tabella delle lunghezze (dopo O, mentre va). */
	void RifaiTabella();
	bool InPercorso() const { return bInPercorso; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Muovi(float DeltaSeconds);
	void MettiAFuoco();
	/** (08/11) Un passo lungo il percorso del trailer. */
	void MuoviSulPercorso(float DeltaSeconds);
	/** Da Da verso A, senza attraversare niente (la sfera di 15 cm): dove si arriva. */
	FVector PassoSicuro(const FVector& Da, const FVector& A) const;
	/** Vero se in A la sfera della telecamera non tocca niente (per saltare alla prima tappa). */
	bool LiberoIn(const FVector& A) const;
	void IniziaFotogrammi();
	void FinisciFotogrammi();
	/** Rimette la sfocatura come la vuole F (dopo un percorso che aveva il suo fuoco). */
	void RimettiSfocatura();

	float Imbardata = 0.f;
	float Beccheggio = 0.f;
	float Rollio = 0.f;
	bool bSfocatura = false;
	bool bColonoNascosto = false;
	TArray<TWeakObjectPtr<AActor>> Nascosti;

	// (08/11) Il percorso del trailer mentre la telecamera lo percorre
	FValdorsoTabellaTrailer Tabella;
	bool bInPercorso = false;
	bool bPrimoPasso = false;
	/** Vero se qualche tappa ha il suo fuoco: allora il fuoco lo decide il percorso, non il centro dell'immagine. */
	bool bFuocoDalPercorso = false;
	float TempoPercorso = 0.f;
	float DistanzaPercorso = 0.f;
	float VelocitaUsata = 0.f;
	int32 GenerazionePercorso = 0;

	// (08/11) I fotogrammi (Valdorso.Trailer.Fotogrammi 1) e come era il tempo del gioco prima
	bool bFotogrammi = false;
	bool bPassoFissoPrima = false;
	double DeltaFissoPrima = 1.0 / 30.0;
	int32 NumeroFotogramma = 0;
	FString CartellaFotogrammi;
};

/** Lo schermo della modalità foto: raccoglie tasti, mouse e controller, e mostra l'aiuto, la griglia e i messaggi. */
class VALDORSO_API SValdorsoFoto : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoFoto) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AValdorsoCameraFoto>, Camera)
		SLATE_EVENT(FSimpleDelegate, OnEsci)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/** Il messaggio in alto per qualche secondo ("Foto salvata..."). */
	void Messaggio(const FText& Testo);

private:
	FReply Cattura();
	void Scatta();
	FValdorsoComandiFoto* Comandi() const;
	EActiveTimerReturnType Anima(double Tempo, float Delta);

	TWeakObjectPtr<AValdorsoCameraFoto> Camera;
	FSimpleDelegate OnEsci;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FSlateBrush Pieno;
	FSlateBrush Pannello;
	bool bAiuto = true;
	bool bGriglia = false;
	/** (08/11) Le bande nere del cinema (B). */
	bool bBande = false;
	/** (08/11) Per dire "fine del percorso" quando la telecamera si ferma da sola. */
	bool bEraInPercorso = false;
	/** (08/11) Vero mentre la telecamera percorre il percorso: niente aiuto, griglia e messaggi. */
	bool InPercorso() const;
	/** (08/11) I tasti del trailer (K, Backspace, L, B, O, 1-5); vero se il tasto era uno di questi. */
	bool TastoTrailer(const FKey& Tasto);
	FText TestoMessaggio;
	double FineMessaggio = 0.0;
	/** Il lampo bianco dello scatto (1 -> 0). */
	float Lampo = 0.f;
};
