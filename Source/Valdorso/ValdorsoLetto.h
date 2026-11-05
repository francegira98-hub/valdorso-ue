// Valdorso - Dormire (v0.1.3, interazioni): il letto e il colono che ci si sdraia. Scritto da Claude il 05/11/2026.
//
// Prima versione, semplice (decisa da Fra il 05/11): E "Dormi" vicino al letto, lo schermo di chi dorme sfuma al
// nero e il colono si ritrova sdraiato sul letto che dorme (animazione "Laying Sleeping" di Mixamo, adattata al
// manichino da porta_dormire_da_mixamo.py: /Game/Mixamo/Dormire/Dorme). E di nuovo, o muoversi: sfuma al nero e il
// colono si ritrova in piedi accanto al letto. Le animazioni vere per sdraiarsi e alzarsi verranno più avanti.
//
// - AValdorsoLetto è la base dei Blueprint dei letti (BP_Letto dei coloni, BP_Letto2 dei nobili, fatti da
//   crea_bp_fab.py): ha i posti, l'altezza del materasso e una zona invisibile per essere trovato con E (i modelli di
//   Fab non hanno sempre la collisione).
// - UValdorsoSonnoComponent sta sul colono: lo stato (letto, posto, fase, dove sta sdraiato, dove si rialza) lo decide
//   il server e viaggia in rete, così tutti vedono chi dorme e in quale letto, anche chi arriva dopo.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoLetto.generated.h"

class UAnimSequence;
class UBoxComponent;
class SWidget;
class AValdorsoLetto;

/** Lo stato del colono rispetto ai letti (viaggia in rete). */
USTRUCT()
struct FValdorsoStatoSonno
{
	GENERATED_BODY()

	/** Il letto (nullptr = sveglio, in piedi). */
	UPROPERTY()
	TObjectPtr<AValdorsoLetto> Letto = nullptr;

	UPROPERTY()
	uint8 Posto = 0;

	/** 0 sveglio, 1 si corica (lo schermo sfuma al nero), 2 dorme, 3 si alza (sfuma al nero). */
	UPROPERTY()
	uint8 Fase = 0;

	/** Quando è iniziata la fase (orologio del server). */
	UPROPERTY()
	double Inizio = 0.0;

	/** Dove sta l'attore del colono mentre dorme (in piedi sul materasso: la telecamera resta alla solita altezza). */
	UPROPERTY()
	FTransform Dove;

	/** (05/11) Come sta la mesh sull'attore mentre dorme: è lei che si sdraia e si abbassa, non la capsula. */
	UPROPERTY()
	FTransform MeshRelativa;

	/** Dove si ritrova in piedi quando si alza. */
	UPROPERTY()
	FTransform Uscita;
};

UCLASS(ClassGroup = (Valdorso), meta = (BlueprintSpawnableComponent))
class VALDORSO_API UValdorsoSonnoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoSonnoComponent();

	/** Vero se dorme, o si sta coricando o alzando. */
	bool Occupato() const { return Stato.Fase != 0; }

	/** Vero solo quando dorme del tutto. */
	bool Dorme() const { return Stato.Fase == 2; }

	AValdorsoLetto* GetLetto() const { return Stato.Letto; }

	/** Solo sul server: si corica in quel posto del letto. */
	void Corica(AValdorsoLetto* Letto, int32 Posto);

	/** Solo sul server: si alza (se dorme). */
	void Alzati();

	/** Dal PC del giocatore: si è mosso e vuole alzarsi (al più due richieste al secondo). */
	void ChiediDiAlzarti();

	UFUNCTION(Server, Reliable)
	void ServerAlzati();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Stato)
	FValdorsoStatoSonno Stato;

	UFUNCTION()
	void OnRep_Stato();

	void CambiaFase(uint8 Nuova);
	double Adesso() const;

	/** Fa partire (o ferma) l'animazione del sonno su questo PC. */
	void SuonaFase();

	/** Il velo nero sullo schermo di chi si corica o si alza (solo sul suo PC). */
	void MostraVelo();
	void TogliVelo();

	/** L'animazione già fatta partire su questo PC (fase e inizio), per non ripeterla. */
	uint8 FaseSuonata = 0;
	double InizioSuonato = -1.0;

	/** L'ultimo stato visto su questo PC, per riconoscere i cambi (spostamenti e velo una volta sola). */
	uint8 FaseVista = 0;
	double InizioVisto = 0.0;
	double UltimaRichiesta = -10.0;

	TSharedPtr<SWidget> Velo;
	double VeloInizio = 0.0;
	/** Quando questo PC ha visto il colono spostato (0 = non ancora): da lì il velo si schiarisce. */
	double VeloFine = 0.0;

	/** La mesh sull'attore "di serie" (letta prima di sdraiarsi), per rimetterla quando si alza. */
	FVector MeshBasePosto = FVector::ZeroVector;
	FRotator MeshBaseVerso = FRotator::ZeroRotator;
	bool bMeshBaseLetta = false;
	void MettiMesh(bool bSdraiata);

	/** Il letto che la capsula sta ignorando (torna un ostacolo quando il colono è in piedi). */
	TWeakObjectPtr<AActor> LettoIgnorato;
};

/**
 * Il letto: la base dei Blueprint dei letti. L'origine è a terra, al centro del letto; il letto è lungo sull'asse Y
 * (come i modelli di Fab) e il colono si alza dalla parte dell'asse X positivo.
 */
UCLASS()
class VALDORSO_API AValdorsoLetto : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoLetto();

	/** Dove stanno i posti, di traverso al letto (cm dal centro, asse X): uno per i letti singoli, due per i grandi. */
	UPROPERTY(EditAnywhere, Category = "Letto")
	TArray<float> Posti;

	/** A che altezza sta il materasso (cm da terra). */
	UPROPERTY(EditAnywhere, Category = "Letto")
	float AltezzaMaterasso = 45.f;

	/** Gradi da aggiungere al corpo sdraiato, se l'animazione lo mette storto rispetto al letto. */
	UPROPERTY(EditAnywhere, Category = "Letto")
	float YawCorpo = 0.f;

	/** Spostamento del corpo sdraiato (cm, assi del letto), per centrarlo sul materasso. */
	UPROPERTY(EditAnywhere, Category = "Letto")
	FVector RitoccoCorpo = FVector::ZeroVector;

	/** Quanto lontano dal centro, dalla parte X positiva, si ritrova il colono quando si alza (cm). */
	UPROPERTY(EditAnywhere, Category = "Letto")
	float DistanzaUscita = 110.f;

	/** Chi dorme in ogni posto (solo sul server). */
	TArray<TWeakObjectPtr<APawn>> Occupanti;

	/** Dove deve stare l'attore del colono sdraiato in quel posto. */
	void TrasformaSdraiato(int32 Posto, const APawn* Chi, FTransform& Attore, FTransform& MeshRelativa) const;

	/** Dove si ritrova in piedi il colono che si alza da quel posto. */
	FTransform TrasformaUscita(int32 Posto, const APawn* Chi) const;

	/** Il posto libero più vicino al colono (INDEX_NONE se il letto è pieno). */
	int32 PostoLibero(const APawn* Chi) const;

	/** Libera il posto di quel colono (solo sul server). */
	void Libera(const APawn* Chi);

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;

protected:
	virtual void BeginPlay() override;

	/** Zona invisibile che fa trovare il letto con E (si adatta da sola alla grandezza del modello). */
	UPROPERTY(VisibleAnywhere, Category = "Letto")
	TObjectPtr<UBoxComponent> Zona;
};
