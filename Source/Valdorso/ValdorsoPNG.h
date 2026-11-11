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
 * (11/11, v0.1.6 "I volti e le giornate") Una tappa della giornata di un PNG: da che ora, dove sta, verso dove guarda,
 * cosa fa (l'animazione in /Game/Mixamo/Gesti: "Martellare", "Spazzare"...; vuoto = sta fermo e respira), o se e'
 * a casa (non si vede: dorme, o e' via).
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

	/** (11/11) Arriva nella valle solo con quell'atto della storia (Maestro Elric: 1, dopo il battito saltato). */
	UPROPERTY(EditAnywhere, Category = "PNG|Giornata", meta = (ClampMin = "0"))
	int32 AttoMinimo = 0;

	/** (12/11, v0.2.0) Viene solo un giorno ogni tanti (giorni veri; 0 = sempre): la carovana di Sibilla, ogni 3. */
	UPROPERTY(EditAnywhere, Category = "PNG|Giornata", meta = (ClampMin = "0"))
	int32 OgniGiorni = 0;

	/** (12/11) Vero se adesso non c'e' (a casa, o non e' il suo giorno). */
	bool Via() const { return bVia; }

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

	/** Manda la battuta al PC (e, se è l'ultima, chiude la conversazione sul server). */
	void Manda(APlayerController* Chi, const FValdorsoBattuta& Battuta);

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
	};
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

	UPROPERTY()
	TObjectPtr<USkeletalMesh> ManichinoUomo;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> ManichinoDonna;

	UPROPERTY()
	TObjectPtr<UAnimSequence> Riposo;
};
