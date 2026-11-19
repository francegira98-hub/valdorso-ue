// Valdorso - I PNG della valle (v0.1.3: "Gaspare al margine del bosco; guardie del passo come PNG che non si possono
// uccidere né corrompere"). Scritto da Claude il 07/11/2026.
//
// Come funziona:
// - AValdorsoPNG è un personaggio fermo (le animazioni vere arrivano con una patch più avanti): il manichino di Unreal
//   che respira (MM_Idle) e, sopra, un corpo MetaHuman come quello dei coloni (UValdorsoAspettoComponent: volto "dei
//   dadi", sempre lo stesso per lo stesso PNG; Testa nei Dettagli per sceglierne uno). Se il MetaHuman non c'è su quel
//   PC, resta il manichino.
// - Non si può uccidere né spingere: non ha statistiche (niente Ability System), quindi i pugni lo attraversano senza
//   danno; la capsula è ferma (non simula la fisica) e i coloni ci sbattono contro come contro un palo.
// - Con E ("Parla con Gaspare") il server comincia il dialogo per quel colono (ValdorsoDialoghi.h): manda la battuta al
//   suo PC, che la mostra sulla pergamena; il PC rimanda la risposta scelta, il server controlla (la conversazione
//   esiste, il numero della battuta è giusto, il colono è ancora vicino) e va avanti. Ogni passo finisce nel registro
//   eventi ("Dialogo"; offrire una moneta a una guardia: "Corruzione").
// - Il PNG si gira piano verso chi gli parla, e quando nessuno gli parla più torna a guardare dove guardava.
// - Il saluto cambia dalla seconda volta (il server si ricorda chi ha già parlato con lui). (08/11) La memoria sta nel
//   file del personaggio (il taccuino, UValdorsoTaccuinoComponent): resta anche quando il server si spegne. Senza un
//   personaggio salvato (accesso libero dell'editor) resta solo in memoria, per nome del personaggio.
// - Nel livello lo mette Content/Python/nuovi/metti_png.py (Gaspare al margine del bosco, Tebaldo e Fulvio al passo;
//   (08/11) Nonna Edda alla palude, Bruno alla locanda, Padre Anselmo nel Tempio del Cuore).
// - (14/11, v0.2.4 "PNG veri", ValdorsoFiducia.h) Ora i colpi lo toccano, ma non muore mai: barcolla, e se lo colpisci forte
//   (o tante volte) cade all'indietro e dopo un po' si rialza. Chi l'ha colpito diventa colpevole (il nome rosso, le
//   guardie); lui e chi ha visto gridano, e chi non e' una guardia scappa qualche passo piu' in la' e poi torna. Si
//   ricorda di te: la fiducia (una parola sotto il nome, sulla pergamena), i saluti che cambiano, e i fatti di ieri
//   ("Ieri la caccia ti e' andata bene...").

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoDialoghi.h"
#include "ValdorsoPNG.generated.h"

class APawn;
class APlayerController;
class UAnimSequence;
class UCapsuleComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UValdorsoAspettoComponent;

/**
 * (14/11, "PNG che camminano") Una strada a piedi: i punti (il primo e' dove stava, l'ultimo dove arriva; per "a casa":
 * dentro la porta di casa) e quali punti sono sulla soglia di una porta (li' si ferma, la apre e passa; poco dopo la
 * porta si richiude).
 */
USTRUCT(BlueprintType)
struct FValdorsoStrada
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	TArray<FVector> Punti;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	TArray<int32> Porte;
};

/**
 * (11/11, v0.1.6 "I volti e le giornate") Una tappa della giornata di un PNG: da che ora, dove sta, verso dove guarda,
 * cosa fa (l'animazione in /Game/Mixamo/Gesti: "Martellare", "Spazzare"...; vuoto = sta fermo e respira), o se e'
 * a casa (non si vede: dorme, o e' via).
 * (14/11, "PNG che vivono") Due movenze speciali: "Panca" = si siede sul posto della panca piu' vicina a Dove (con lo
 * stesso seduto dei coloni), "Letto" = dorme nel letto piu' vicino a Dove (lo stesso sonno dei coloni). Se non c'e' una
 * panca o un letto vicino, resta in piedi. E una porta (Porta: dove sta; zero = nessuna) che apre o chiude arrivando.
 */
USTRUCT(BlueprintType)
struct FValdorsoTappa
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	float Ora = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	FVector Dove = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	float Gradi = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	FName Movenza;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	bool bAssente = false;

	/** (14/11) Dove sta la porta da aprire (o chiudere) quando comincia questa tappa: la mattina Bruno apre la locanda,
	 *  la sera la chiude. Zero = nessuna porta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	FVector Porta = FVector::ZeroVector;

	/** (14/11) Vero = la apre, falso = la chiude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	bool bApriPorta = true;

	/** (14/11, "PNG che camminano") Le strade per arrivare a questa tappa dalla tappa prima, una per ogni fase del
	 *  villaggio (0 = fase 1, 1 = fase 2, 2 = fase 3: le case, le palizzate e i muri cambiano). Le calcola
	 *  giornate_vive.py sulla pianta del villaggio. Senza la strada della fase di adesso: come prima, cambia posto
	 *  quando nessuno lo vede. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	TArray<FValdorsoStrada> Strade;

	/** (19/11, v0.2.7) Il posto di questa tappa nelle tre fasi del villaggio (0 = fase 1...), per chi sta in un
	 *  edificio che cambia (il tempio: cappella, chiesa, pieve). Vuoto o zero = sempre Dove. Li scrive crea_volti.py;
	 *  quando cambia la fase AValdorsoPNG::VaiAllaFase li copia in Dove e Gradi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	TArray<FVector> DovePerFase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Giornata")
	TArray<float> GradiPerFase;
};

namespace ValdorsoGiornata
{
	/** Quale tappa vale a quell'ora della valle (0..24): l'ultima cominciata; prima della prima, l'ultima del giorno
	 *  prima. -1 se la giornata e' vuota. */
	VALDORSO_API int32 TappaPer(const TArray<FValdorsoTappa>& Giornata, float Ora);
}

UCLASS()
class VALDORSO_API AValdorsoPNG : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoPNG();

	/** Quale dialogo (ValdorsoDialoghiTesti.inl): "Gaspare", "GuardiaCorrado", "GuardiaOttone"; (08/11) "NonnaEdda",
	 *  "Bruno", "PadreAnselmo". */
	UPROPERTY(EditAnywhere, Replicated, Category = "PNG")
	FName Dialogo = TEXT("Gaspare");

	/** La testa MetaHuman: il numero tra le teste da uomo (o da donna); -1 = quella "dei dadi", sempre la stessa per
	 *  questo dialogo. */
	UPROPERTY(EditAnywhere, Category = "PNG", meta = (ClampMin = "-1"))
	int32 Testa = -1;

	/** Una donna (testa, corpo e manichino da donna). */
	UPROPERTY(EditAnywhere, Category = "PNG")
	bool bDonna = false;

	/** Quanto può allontanarsi il colono mentre parla (cm): oltre, il dialogo si chiude. */
	UPROPERTY(EditAnywhere, Category = "PNG", meta = (ClampMin = "250"))
	float RaggioDialogo = 450.f;

	/** Quanto in fretta si gira verso chi gli parla (gradi al secondo). */
	UPROPERTY(EditAnywhere, Category = "PNG", meta = (ClampMin = "10"))
	float VelocitaGiro = 120.f;

	/** (11/11) La giornata: dove sta alle varie ore (vuota = sempre dove l'hanno messo). Cambia posto solo quando nessun
	 *  colono lo vede (o dopo dieci minuti che vorrebbe andare), e mai mentre parla. */
	UPROPERTY(EditAnywhere, Category = "PNG|Giornata")
	TArray<FValdorsoTappa> Giornata;

	/** (19/11, v0.2.7) Per chi non ha una giornata (Padre Anselmo): dove sta nelle tre fasi del villaggio (0 = fase 1).
	 *  Vuoto o zero = dove l'hanno messo. */
	UPROPERTY(EditAnywhere, Category = "PNG|Giornata")
	TArray<FVector> PostoPerFase;

	UPROPERTY(EditAnywhere, Category = "PNG|Giornata")
	TArray<float> GradiPerFase;

	/** (19/11) Sul server e sui PC, quando il villaggio cambia fase (AValdorsoFasiVillaggio::Applica): mette i posti
	 *  di quella fase nelle tappe e, se serve, lo sposta subito. */
	void VaiAllaFase(int32 Fase);

	/** (19/11) L'ultima fase messa a chi non ha una giornata: AValdorsoFasiVillaggio::Applica gira spesso (ogni pezzo
	 *  di mondo che arriva), e lui si sposta solo quando la fase cambia davvero. Non viaggia in rete. */
	int32 FaseMessa = INDEX_NONE;

	/** (11/11) Arriva nella valle solo con quell'atto della storia (Maestro Elric: 1, dopo il battito saltato). */
	UPROPERTY(EditAnywhere, Category = "PNG|Giornata", meta = (ClampMin = "0"))
	int32 AttoMinimo = 0;

	/** (12/11, v0.2.0) Viene solo un giorno ogni tanti (giorni veri; 0 = sempre): la carovana di Sibilla, ogni 3. */
	UPROPERTY(EditAnywhere, Category = "PNG|Giornata", meta = (ClampMin = "0"))
	int32 OgniGiorni = 0;

	/** (12/11) Vero se adesso non c'e' (a casa, o non e' il suo giorno). */
	bool Via() const { return bVia; }

	/** (14/11, v0.2.4) Il nome breve del suo dialogo ("Bruno"); il nome dell'attore se il dialogo non c'e'. */
	FString NomeBreve() const;

	/** (14/11) Vero se e' una guardia della Corona (le guardie del passo e la capitana Livia): arrestano i colpevoli. */
	bool Guardia() const;

	/** (14/11) Vero se e' a terra (l'hanno colpito forte: fra poco si rialza). */
	bool ATerra() const { return bATerra; }

	/** (14/11) Vero se sta dormendo nel suo letto (non parla, non saluta, non vede le colpe). */
	bool Dorme() const { return Posa == 2; }

	/** (14/11) Vero se e' seduto su una panca. */
	bool Seduto() const { return Posa == 1; }

	/** (14/11) Vero se sta camminando verso la tappa (anche se si e' fermato a parlare). */
	bool Cammina() const { return bCammina; }

	/** (14/11) Solo sul server: un colpo (ValdorsoCombattimento::Colpisci). Barcolla o cade; chi l'ha colpito e' colpevole.
	 *  Vero se il colpo l'ha toccato. */
	bool Colpito(AActor* Da, float Danno);

	/** (14/11) Solo sul server: ha visto (o subito, bVittima) una colpa di quel colono: grida, si gira, e se non e' una
	 *  guardia scappa un poco. */
	void Spaventa(APawn* Colono, bool bVittima);

	/** (14/11) Solo sul server: comincia con quel colono un dialogo che non e' il suo (l'arresto delle guardie). */
	bool ApriDialogo(APawn* Chi, FName IdDialogo);

	/** (14/11, v0.2.4) Solo sul server: la scelta di una storia del Battito (ValdorsoBattito.h): le righe del passo e i
	 *  finali come risposte, sulla pergamena di questo PNG. */
	bool ApriSceltaStoria(APawn* Chi, const FString& IdStoria, const TArray<FString>& Righe, const TArray<FString>& Scelte);

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;

	/**
	 * Solo sul server: il colono di questo PC ha scelto la risposta numero Scelta alla battuta numero Passo
	 * (INDEX_NONE = se ne va). Lo chiama il PlayerController (ServerScegliDialogo).
	 */
	void Scegli(APlayerController* Chi, int32 Passo, int32 Scelta);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** La capsula: ferma, blocca i coloni, non la telecamera. */
	UPROPERTY(VisibleAnywhere, Category = "PNG")
	TObjectPtr<UCapsuleComponent> Capsula;

	/** Il manichino (invisibile quando c'è il MetaHuman, ma è lui che respira: il MetaHuman ne copia la posa). */
	UPROPERTY(VisibleAnywhere, Category = "PNG")
	TObjectPtr<USkeletalMeshComponent> Corpo;

	/** Il corpo MetaHuman sopra il manichino, come per i coloni. */
	UPROPERTY(VisibleAnywhere, Category = "PNG")
	TObjectPtr<UValdorsoAspettoComponent> Aspetto;

private:
	/** Il dialogo di questo PNG (nullptr se il nome nei Dettagli è sbagliato). */
	const FValdorsoDialogo* DatiDialogo() const;

	/** Manda la battuta al PC (e, se è l'ultima, chiude la conversazione sul server). (14/11) Con IdFiducia la parola
	 *  della fiducia di quel dialogo sotto il nome. */
	void Manda(APlayerController* Chi, const FValdorsoBattuta& Battuta, FName IdFiducia = NAME_None);

	/** (14/11) Comincia una conversazione (il suo dialogo, o quello di IdAltro) e manda la prima battuta. */
	bool Comincia(APawn* Chi, const FValdorsoDialogo& Dati, FName IdAltro);

	/** Il server chiude la conversazione con quel PC (con bAvvisa dice al PC di togliere la pergamena). */
	void Chiudi(APlayerController* Chi, bool bAvvisa, const FString& Come);

	/** Si gira verso quel colono (sul server: il voluto va in rete). */
	void GuardaVerso(const AActor* Chi);

	/** Se nessuno gli parla più, fra poco torna a guardare dove guardava. */
	void ForseTornaAlPosto();
	void TornaAlPosto();

	/** Una conversazione aperta (solo sul server). */
	struct FConversazione
	{
		FName Nodo;
		int32 Passo = 0;
		/** (14/11) Un dialogo che non e' il suo (l'arresto); vuoto = il suo. "Battito" = la scelta di una storia. */
		FName Altro;
		/** (14/11) La storia del Battito di quella scelta, e quante risposte ha. */
		FString Storia;
		int32 Scelte = 0;
	};

	/** (14/11) Il dialogo di quella conversazione. */
	const FValdorsoDialogo* DatiDi(const FConversazione& Conversazione) const;
	TMap<TWeakObjectPtr<APlayerController>, FConversazione> Conversazioni;

	/** Il numero della prossima battuta (cresce sempre: una risposta vecchia non vale per una battuta nuova). */
	int32 ProssimoPasso = 1;

	/** Dove guarda il PNG (gradi): lo decide il server, il giro si fa su ogni PC. */
	UPROPERTY(ReplicatedUsing = OnRep_Sguardo)
	float Sguardo = 0.f;

	UFUNCTION()
	void OnRep_Sguardo();

	/** Vero quando c'è uno Sguardo da seguire (sul server sempre; sul PC dopo il primo arrivato dalla rete). */
	bool bSguardoValido = false;

	/** Dove guardava nel livello. */
	float SguardoIniziale = 0.f;

	FTimerHandle TimerRitorno;

	/** (11/11) La giornata: la tappa di adesso e se e' via (a casa, o non ancora arrivato), in rete. */
	UPROPERTY(ReplicatedUsing = OnRep_Tappa)
	int32 Tappa = -1;

	UPROPERTY(ReplicatedUsing = OnRep_Tappa)
	bool bVia = false;

	UFUNCTION()
	void OnRep_Tappa();

	/** Solo sul server, ogni dieci secondi: e' ora di cambiare posto? */
	void GiroGiornata();
	FTimerHandle TimerGiornata;
	double VuoleCambiareDa = -1.0;
	bool bGiornataPartita = false;

	/** (10/11) I saluti a chi passa vicino (solo sul server): una riga nella chat di quel colono, ogni tanto. */
	void Saluta();
	FTimerHandle TimerSaluti;
	TMap<TWeakObjectPtr<APlayerController>, double> Salutati;

	// --- (14/11, v0.2.4) Colpito: barcolla, cade e si rialza, scappa -------------------------------------------

	/** Crescono a ogni barcollata / caduta: su ogni PC il corpo si piega (o cade all'indietro e si rialza). */
	UPROPERTY(ReplicatedUsing = OnRep_Barcollate)
	uint8 Barcollate = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Cadute)
	uint8 Cadute = 0;

	UPROPERTY(Replicated)
	bool bATerra = false;

	/** La fuga: dove sta andando (via da chi ha colpito, o di nuovo al suo posto). */
	UPROPERTY(ReplicatedUsing = OnRep_Fuga)
	FVector_NetQuantize DoveFuga;

	UPROPERTY(ReplicatedUsing = OnRep_Fuga)
	bool bInFuga = false;

	UFUNCTION()
	void OnRep_Barcollate();

	UFUNCTION()
	void OnRep_Cadute();

	UFUNCTION()
	void OnRep_Fuga();

	/** Su ogni PC: il corpo che si piega o cade (vero finche' c'e' da muoversi). */
	bool AnimaCorpo();

	/** Su ogni PC: un passo della fuga (vero finche' non e' arrivato). */
	bool MuoviFuga(float DeltaSeconds);

	/** Rimette l'animazione della tappa (o il respiro). */
	void RimettiMovenza();

	/** Solo sul server: si rialza; torna dalla fuga. */
	void Rialza();
	void RitornaDallaFuga();
	void Fuggi(const AActor* Da);

	/** Solo sul server: una riga nella chat di chi e' vicino. */
	void Grida(const FString& Frase);

	/** Le ore (di questo PC) in cui sono cominciate la barcollata e la caduta (-1: niente). */
	double BarcolloDa = -1.0;
	double CadutaDa = -1.0;
	/** Su ogni PC: il corpo e' piegato (va rimesso dritto); sta correndo (l'animazione della corsa). */
	bool bPiegato = false;
	bool bCorre = false;
	/** Su ogni PC: ha visto cominciare la fuga (solo allora, alla fine, si rimette dove dice il server). */
	bool bVistaFuga = false;

	/** Solo sul server. */
	float DannoRecente = 0.f;
	double DannoDa = -100.0;
	double UltimoGrido = -100.0;
	FVector PostoFuga = FVector::ZeroVector;
	bool bRitornoFuga = false;
	FTimerHandle TimerATerra;
	FTimerHandle TimerFuga;

	UPROPERTY()
	TObjectPtr<UAnimSequence> Corsa;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> ManichinoUomo;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> ManichinoDonna;

	UPROPERTY()
	TObjectPtr<UAnimSequence> Riposo;

	// --- (14/11, "PNG che vivono") seduti sulle panche, a letto, le porte ---------------------------------------

	/** Il server l'ha fatto alzare (colpito, spaventato) prima della fine della tappa: resta in piedi accanto alla panca o
	 *  al letto fino alla tappa dopo. */
	UPROPERTY(ReplicatedUsing = OnRep_Alzato)
	bool bAlzato = false;

	UFUNCTION()
	void OnRep_Alzato();

	/** Su ogni PC (e sul server): si siede, si corica o resta in piedi, come vuole la tappa (e bAlzato). */
	void MettiPosa();

	/** Solo sul server: se e' seduto o dorme, si alza. */
	void Alzati();

	/** Lascia libero il posto della panca o del letto (sul server toglie la prenotazione). */
	void LiberaMobile();

	/** Solo sul server: apre o chiude la porta della tappa. */
	void UsaPorta(const FValdorsoTappa& Arrivo);

	/** 0 in piedi, 1 seduto, 2 dorme. */
	uint8 Posa = 0;

	/** La panca o il letto di adesso, e il posto. */
	TWeakObjectPtr<AActor> Mobile;
	int32 PostoMobile = INDEX_NONE;

	// --- (14/11, "PNG che camminano") da una tappa all'altra a piedi --------------------------------------------
	// Il server decide quando parte (CamminoInizio, ora del server) e se si e' fermato (CamminoFermo: a che punto del
	// cammino, in secondi); ogni PC rifa' gli stessi conti sul Percorso della tappa, quindi tutti lo vedono nello
	// stesso punto senza mandare la posizione in rete.

	UPROPERTY(ReplicatedUsing = OnRep_Cammino)
	bool bCammina = false;

	UPROPERTY(ReplicatedUsing = OnRep_Cammino)
	double CamminoInizio = 0.0;

	UPROPERTY(ReplicatedUsing = OnRep_Cammino)
	float CamminoFermo = -1.f;

	/** Quale strada della tappa (la fase del villaggio quando e' partito, meno uno). */
	UPROPERTY(ReplicatedUsing = OnRep_Cammino)
	uint8 StradaUsata = 0;

	/** La strada del cammino di adesso (nullptr se non c'e'). */
	const FValdorsoStrada* StradaDiAdesso() const;

	UFUNCTION()
	void OnRep_Cammino();

	/** L'ora del server (uguale su tutti i PC). */
	double OraServer() const;

	/** Da quanti secondi cammina (fermo: dove si e' fermato). */
	float TempoDelCammino() const;

	/** Dove sta dopo Tempo secondi di cammino, verso dove va, se e' fermo a una porta (e quale punto). Falso = arrivato. */
	bool PuntoDelCammino(float Tempo, FVector& Dove, float& Gradi, int32& PortaSulPercorso) const;

	/** Su ogni PC: un passo del cammino (vero finche' cammina). */
	bool MuoviCammino(float DeltaSeconds);

	/** Solo sul server, ogni quarto di secondo mentre cammina: si ferma e riparte, apre e chiude le porte, arriva. */
	void ControllaCammino();
	void FinisciCammino();
	FTimerHandle TimerCammino;

	/** Solo sul server: se all'arrivo sparisce (la tappa "a casa"). */
	bool bViaAllArrivo = false;
	int32 UltimaPortaAperta = INDEX_NONE;
	struct FPortaDaChiudere
	{
		TWeakObjectPtr<AValdorsoPorta> Porta;
		double Alle = 0.0;
	};
	TArray<FPortaDaChiudere> PorteDaChiudere;

	/** Su ogni PC: quale animazione del cammino sta suonando (0 nessuna, 1 cammina, 2 apre la porta, 3 fermo), e la
	 *  quota dei piedi dell'ultimo passo. */
	uint8 AnimCammino = 0;
	double CamminoZ = 0.0;
	bool bCamminoZ = false;

	UPROPERTY()
	TObjectPtr<UAnimSequence> Passeggio;

	UPROPERTY()
	TObjectPtr<UAnimSequence> ApreLaPorta;
};
