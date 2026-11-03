// Valdorso - Sedersi (v0.1.3, interazioni): la panca e il modo in cui il colono si siede e si alza.
// Scritto da Claude il 03/11/2026. Le animazioni sono quelle della panca del Game Animation Sample di Epic, adattate al
// nostro manichino da porta_panca_da_gasp.py (Content/Fab/GameAnimationSample/Panca).
//
// Come funziona:
// - AValdorsoPanca ha dei posti (di serie 2). Con E il colono si siede sul posto libero più vicino; con E di nuovo, o
//   muovendosi, si alza.
// - Il server sceglie l'animazione per sedersi dalla parte da cui arriva il colono (davanti, di lato, da dietro di
//   sbieco) e calcola da dove deve partire perché, alla fine dell'animazione, sia seduto esattamente al suo posto: il
//   movimento lo dà l'animazione stessa (il "root motion"), che ogni PC rilegge per muovere la capsula.
// - Lo stato (posto, fase, animazione, istante di inizio) sta in UValdorsoSedutaComponent, sul personaggio, e viaggia
//   in rete: tutti vedono il colono sedersi e alzarsi allo stesso modo, anche chi arriva dopo.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoSeduta.generated.h"

class UAnimSequence;
class UStaticMeshComponent;
class AValdorsoPanca;

/** Lo stato del colono rispetto alle panche (viaggia in rete). */
USTRUCT()
struct FValdorsoStatoSeduta
{
	GENERATED_BODY()

	/** La panca (nullptr = in piedi). */
	UPROPERTY()
	TObjectPtr<AValdorsoPanca> Panca = nullptr;

	UPROPERTY()
	uint8 Posto = 0;

	/** 0 in piedi, 1 si siede, 2 seduto, 3 si alza. */
	UPROPERTY()
	uint8 Fase = 0;

	/** Quale animazione per sedersi (vedi ValdorsoSeduta.cpp, Entrate). */
	UPROPERTY()
	uint8 Entrata = 0;

	/** Quando è iniziata la fase (orologio del server). */
	UPROPERTY()
	double Inizio = 0.0;

	/** Dove stava la mesh del colono all'inizio della fase (nel mondo): da qui parte il movimento dell'animazione. */
	UPROPERTY()
	FTransform Partenza;

	/** (03/11) Da che punto dell'animazione parte la fase (secondi): sedersi salta la camminata iniziale del GASP. */
	UPROPERTY()
	float Avvio = 0.f;

	/** (03/11) A che punto dell'animazione finisce la fase (0 = alla fine della sequenza). */
	UPROPERTY()
	float Fine = 0.f;

	/** (03/11) Differenza tra dove stava la mesh e dove comincia l'animazione: si assorbe nei primi decimi di secondo (vedi PancaSfumatura). */
	UPROPERTY()
	FVector Scarto = FVector::ZeroVector;

	UPROPERTY()
	float ScartoYaw = 0.f;

	/** (03/11, prova) Gradi in più sulla mesh mentre è sulla panca, se le animazioni adattate guardano storto (Valdorso.Panca.Correzione). */
	UPROPERTY()
	float Correzione = 0.f;

	/** (03/11, prova) Vero = la capsula resta ferma al posto e l'animazione gira sul posto (Valdorso.Panca.Fermo 1). */
	UPROPERTY()
	bool bFermo = false;
};

UCLASS(ClassGroup = (Valdorso), meta = (BlueprintSpawnableComponent))
class VALDORSO_API UValdorsoSedutaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoSedutaComponent();

	/** Vero se il colono è seduto, o si sta sedendo o alzando. */
	bool Occupato() const { return Stato.Fase != 0; }

	/** La panca su cui sta (nullptr se in piedi). */
	AValdorsoPanca* GetPanca() const { return Stato.Panca; }

	/** Solo sul server: si siede su quel posto della panca. */
	void Siedi(AValdorsoPanca* Panca, int32 Posto);

	/** Solo sul server: si alza (se è seduto). */
	void Alzati();

	/** Vero solo quando è seduto del tutto (non mentre si siede o si alza). */
	bool Seduto() const { return Stato.Fase == 2; }

	/** Dal PC del giocatore: si è mosso e vuole alzarsi. Manda la richiesta al più due volte al secondo. */
	void ChiediDiAlzarti();

	/** Dal PC del giocatore: vuole alzarsi (si è mosso). */
	UFUNCTION(Server, Reliable)
	void ServerAlzati();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Come sta di solito la mesh sulla capsula (letta la prima volta che serve, per rimetterla dopo la correzione). */
	FRotator RotazioneMeshBase = FRotator::ZeroRotator;
	bool bRotazioneMeshLetta = false;
	FRotator RotazioneBase();

	/** I gradi di correzione messi adesso sulla mesh (0 = mesh di serie). */
	float CorrezioneMessa = 0.f;

	/** Mette (o toglie, con 0) la correzione sulla mesh. */
	void ApplicaCorrezione(float Gradi);

	/** La panca che la capsula sta ignorando (per tornare a sbatterci contro quando è in piedi). */
	TWeakObjectPtr<AActor> PancaIgnorata;

	/** Scrive nel log dove sta il colono adesso (con Valdorso.Panca.Diagnosi 1, acceso di serie finché la panca è in prova). */
	void Diagnosi(const TCHAR* Quando) const;

	UPROPERTY(ReplicatedUsing = OnRep_Stato)
	FValdorsoStatoSeduta Stato;

	UFUNCTION()
	void OnRep_Stato();

	/** L'animazione della fase di adesso. */
	UAnimSequence* AnimazioneDellaFase() const;

	/** Fa partire sul PC l'animazione della fase (anche a metà, per chi arriva dopo). */
	void SuonaFase();

	/** Muove la capsula seguendo il movimento dell'animazione. */
	void SeguiAnimazione();

	/** Fine della fase (sul server): seduto dopo essersi seduto, in piedi dopo essersi alzato. */
	void CambiaFase(uint8 Nuova);

	double Adesso() const;

	/** La fase già fatta partire su questo PC (per non ripeterla). */
	uint8 FaseSuonata = 0;

	/** Quando questo PC ha chiesto l'ultima volta di alzarsi (per non mandare una richiesta a ogni fotogramma). */
	double UltimaRichiesta = -10.0;
	double InizioSuonato = -1.0;
};

/**
 * La panca: una seduta di legno su quattro gambe (di serie fatta con cubi, 180 × 45 × 45 cm), con dei posti. L'origine è a
 * terra, al centro; la X guarda dalla parte di chi si siede (davanti).
 */
UCLASS()
class VALDORSO_API AValdorsoPanca : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoPanca();

	/** Dove stanno i posti, lungo la panca (cm dal centro, asse Y). */
	UPROPERTY(EditAnywhere, Category = "Panca")
	TArray<float> Posti;

	/** Quanto avanti rispetto al centro della seduta finisce il colono seduto (cm, asse X). Da tarare a occhio. */
	UPROPERTY(EditAnywhere, Category = "Panca")
	float AvantiPosto = 8.f;

	/** Il colono seduto in ogni posto (solo sul server). */
	TArray<TWeakObjectPtr<APawn>> Occupanti;

	/** Dove deve trovarsi il colono (il suo attore) quando è seduto in quel posto. */
	FTransform TrasformaSeduto(int32 Posto, const APawn* Chi) const;

	/** Il posto libero più vicino al colono (INDEX_NONE se la panca è piena). */
	int32 PostoLibero(const APawn* Chi) const;

	/** Libera il posto di quel colono (solo sul server). */
	void Libera(const APawn* Chi);

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Panca")
	TObjectPtr<UStaticMeshComponent> Seduta;

	UPROPERTY(VisibleAnywhere, Category = "Panca")
	TArray<TObjectPtr<UStaticMeshComponent>> Gambe;
};
