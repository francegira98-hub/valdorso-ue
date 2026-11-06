// Valdorso - Le scale a pioli (v0.1.3, interazioni). Scritto da Claude il 06/11/2026, preparato mentre Fra dormiva.
//
// Prima versione, semplice (come i letti): nelle torri di guardia (della milizia, del passo e del nord) una scala a
// pioli sale dal pavimento al ballatoio, che ha una botola.
// - In basso, E "Sali la scala": il colono si mette davanti ai pioli, con la faccia al muro, e sale con W (scende con S).
//   Arrivato in cima esce sul ballatoio. In cima, vicino alla botola, E "Scendi la scala": si mette sulla scala e scende;
//   arrivato in fondo resta in piedi. Spazio (o E) mentre è sulla scala: la lascia e cade (attenzione all'altezza).
// - Sulla scala il movimento è quello "in volo" di Unreal, ma solo su e giù e lento: lo decide il server e il PC lo
//   prevede, come camminare. Lo stato (quale scala, da quando, dove si esce) sta in UValdorsoScalataComponent, sul
//   colono, e viaggia in rete.
// - L'animazione vera per arrampicarsi arriverà più avanti (Mixamo "Climbing Ladder" o il Game Animation Sample):
//   per ora il colono sale fermo, nella posa che l'animazione gli dà in volo.
// - AValdorsoScala non ha un modello: i pioli sono nella mesh della torre (Tools/Generatori/villaggio_edifici.py,
//   torre()). L'attore ha l'altezza, la zona per essere trovato con E e i due punti (in fondo e in cima).
//   Origine: alla base dei montanti, a terra; la X guarda verso il colono (fuori dal muro), la Z in alto.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoScala.generated.h"

class UBoxComponent;
class AValdorsoScala;

namespace ValdorsoScale
{
	/** Che cosa succede a chi è sulla scala: 0 niente, 1 esce in cima, -1 arriva in fondo. ZCentro e Altezza sono
	 *  rispetto alla base della scala (cm), VelocitaZ è quanto sta salendo (cm/s). */
	VALDORSO_API int32 Estremo(float ZCentro, float Altezza, float MezzaCapsula, float VelocitaZ);

	/** Il punto della scala più vicino a quell'altezza, tra un metro da terra e un metro sopra la cima (rispetto alla base). */
	VALDORSO_API float QuotaInterazione(float ZColono, float Altezza);
}

/** Lo stato del colono rispetto alle scale (viaggia in rete). */
USTRUCT()
struct FValdorsoStatoScala
{
	GENERATED_BODY()

	/** La scala (nullptr = non è su una scala). */
	UPROPERTY()
	TObjectPtr<AValdorsoScala> Scala = nullptr;

	/** Quando è salito o sceso (orologio del server): serve a capire se è un cambio nuovo. */
	UPROPERTY()
	double Inizio = 0.0;

	/** Dove mettere il colono quando la cosa comincia (sulla scala) o finisce (in cima o in fondo). */
	UPROPERTY()
	FVector Dove = FVector::ZeroVector;

	/** Verso dove guarda (gradi). */
	UPROPERTY()
	float Yaw = 0.f;

	/** Vero se il server ha deciso un posto preciso (Dove); falso quando la lascia e cade da dov'è. */
	UPROPERTY()
	bool bSposta = false;
};

UCLASS(ClassGroup = (Valdorso), meta = (BlueprintSpawnableComponent))
class VALDORSO_API UValdorsoScalataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoScalataComponent();

	/** Vero se il colono è su una scala. */
	bool Sale() const { return Stato.Scala != nullptr; }

	AValdorsoScala* GetScala() const { return Stato.Scala; }

	/** Solo sul server: si mette sulla scala (dal fondo o dalla cima). */
	void Inizia(AValdorsoScala* Scala, bool bDallaCima);

	/** Solo sul server: scende dalla scala. Estremo 1 = esce in cima, -1 = in fondo, 0 = la lascia e cade. */
	void Finisci(int32 Estremo);

	/** Dal PC del giocatore: vuole lasciare la scala (Spazio). */
	void ChiediDiLasciare();

	UFUNCTION(Server, Reliable)
	void ServerLascia();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Stato)
	FValdorsoStatoScala Stato;

	UFUNCTION()
	void OnRep_Stato();

	/** Mette o toglie il movimento della scala sul colono (su ogni macchina). */
	void ApplicaMovimento(bool bSullaScala);

	double Adesso() const;

	/** Come era il movimento prima di salire (per rimetterlo uguale). */
	float VelocitaVoloPrima = 600.f;
	float FrenataVoloPrima = 0.f;
	bool bGiravaPrima = true;
	bool bMovimentoSalvato = false;

	/** L'ultimo stato già messo in pratica su questa macchina. */
	double InizioVisto = -1.0;
	bool bSullaScalaVisto = false;
	double UltimaRichiesta = -10.0;
};

UCLASS()
class VALDORSO_API AValdorsoScala : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoScala();

	/** Dalla base al ballatoio (cm): in cima il colono esce a questa quota. */
	UPROPERTY(EditAnywhere, Category = "Scala", meta = (ClampMin = "100"))
	float Altezza = 1200.f;

	/** Quanto davanti ai montanti sta il colono che sale (cm, lungo la X). */
	UPROPERTY(EditAnywhere, Category = "Scala", meta = (ClampMin = "20"))
	float DistanzaDavanti = 50.f;

	/** Dove esce in cima, davanti ai montanti (cm, lungo la X): accanto alla botola, sul ballatoio. */
	UPROPERTY(EditAnywhere, Category = "Scala")
	float UscitaCima = 170.f;

	/** Quanto è veloce a salire e scendere (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Scala", meta = (ClampMin = "20"))
	float Velocita = 140.f;

	/** Il punto dove sta chi sale, a quell'altezza dalla base (cm), nel mondo. */
	FVector PuntoSullaScala(float Quota) const;

	/** Verso dove guarda chi sale (la faccia ai pioli). */
	float YawSalita() const;

	/** Vero se il colono sta in alto (sul ballatoio) e non in basso. */
	bool InCima(const APawn* Chi) const;

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;
	virtual FVector PuntoInterazionePer(const APawn* Chi) const override;

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Scala")
	TObjectPtr<UBoxComponent> Zona;

	void SistemaZona();
};
