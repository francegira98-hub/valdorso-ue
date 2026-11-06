// Valdorso - Le interazioni con il mondo (v0.1.3, primo pezzo): il tasto E e la porta. Scritto da Claude il
// 03/11/2026, preparato mentre Fra dormiva.
//
// Come funziona:
// - Ogni cosa con cui si può interagire (porte; poi panche, letti, bacheche, imposte, scale a pioli) implementa
//   IValdorsoInterattivo: dice cosa scrivere sullo schermo ("Apri la porta") e cosa succede.
// - Sul PC: il controllore cerca ogni decimo di secondo la cosa più vicina davanti al colono (entro 2 m) e mostra in
//   basso "E · Apri la porta". Premendo E (o X sul controller) chiede al server.
// - Il server decide: controlla che il colono sia davvero vicino (2,5 m) e che la cosa si possa usare, poi la usa e
//   la annota nel registro eventi (entrare dove non si deve, un giorno, potrà essere un crimine).
// - AValdorsoPorta: un'anta che gira sulla cerniera, aperta o chiusa per tutti (lo stato viaggia in rete), che si apre
//   dalla parte opposta a chi spinge. Di serie è un'anta di legno fatta con un cubo: il modello vero si mette nel
//   Blueprint figlio o nei Dettagli (Anta → Static Mesh).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/Interface.h"
#include "ValdorsoInterazione.generated.h"

class APawn;
class UStaticMeshComponent;
class USceneComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UValdorsoInterattivo : public UInterface
{
	GENERATED_BODY()
};

class VALDORSO_API IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	/** Cosa si legge sullo schermo quando il colono è vicino ("Apri la porta"). Vuoto = adesso non si può usare. */
	virtual FText TestoInterazione(const APawn* Chi) const = 0;

	/** Solo sul server: il colono la usa. */
	virtual void Interagisci(APawn* Chi) = 0;

	/** Il punto da cui misurare la distanza (di solito il centro della cosa). */
	virtual FVector PuntoInterazione() const = 0;

	/** (06/11) Il punto da cui misurare la distanza per quel colono: di serie lo stesso per tutti. Le cose lunghe (le
	 *  scale a pioli, alte più di dieci metri) danno il punto alla sua altezza. */
	virtual FVector PuntoInterazionePer(const APawn* Chi) const { return PuntoInterazione(); }
};

namespace ValdorsoInterazione
{
	/** Fin dove il PC mostra il suggerimento (cm). */
	inline constexpr float Raggio = 200.f;

	/** Fin dove il server accetta (cm): un po' di più, per il ritardo della rete. */
	inline constexpr float RaggioServer = 250.f;

	/** La cosa con cui interagire più vicina davanti al colono (nullptr se nessuna). */
	VALDORSO_API AActor* PiuVicina(const APawn* Chi);

	/** (03/11) Vero se tra il colono e la cosa non c'è un muro: niente porte o imposte usate attraverso le pareti. */
	VALDORSO_API bool SiVede(const APawn* Chi, const AActor* Cosa);

	/** (05/11) Vero se il colono può usare quella cosa adesso: seduto su una panca o in un letto, solo quella. */
	VALDORSO_API bool PuoUsare(const APawn* Chi, const AActor* Cosa);
}

UCLASS()
class VALDORSO_API AValdorsoPorta : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoPorta();

	/** Di quanti gradi si apre. */
	UPROPERTY(EditAnywhere, Category = "Porta", meta = (ClampMin = "10", ClampMax = "175"))
	float AngoloApertura = 95.f;

	/** Quanto ci mette ad aprirsi o chiudersi (secondi). */
	UPROPERTY(EditAnywhere, Category = "Porta", meta = (ClampMin = "0.1"))
	float Durata = 0.6f;

	/** Chiusa a chiave: non si apre (le chiavi arriveranno con l'inventario, v0.1.4). */
	UPROPERTY(EditAnywhere, Replicated, Category = "Porta")
	bool bChiusaAChiave = false;

	/** 0 = si apre dalla parte opposta a chi spinge; 1 o -1 = sempre dalla stessa parte (le imposte, verso fuori). */
	UPROPERTY(EditAnywhere, Category = "Porta", meta = (ClampMin = "-1", ClampMax = "1"))
	int32 VersoFisso = 0;

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	/** Le scritte (le imposte le cambiano). */
	virtual FText TestoApri() const;
	virtual FText TestoChiudi() const;
	virtual FText TestoAChiave() const;

	/** La cerniera: l'anta gira intorno a lei. */
	UPROPERTY(VisibleAnywhere, Category = "Porta")
	TObjectPtr<USceneComponent> Cerniera;

	/** L'anta (di serie un cubo color legno, 100 × 6 × 210 cm). */
	UPROPERTY(VisibleAnywhere, Category = "Porta")
	TObjectPtr<UStaticMeshComponent> Anta;

private:
	/** 0 = chiusa; +1 o -1 = aperta da una parte o dall'altra. Lo decide il server. */
	UPROPERTY(ReplicatedUsing = OnRep_Verso)
	int8 Verso = 0;

	UFUNCTION()
	void OnRep_Verso();

	/** L'angolo di adesso (si muove piano verso quello voluto). */
	float AngoloAttuale = 0.f;
};

/**
 * (03/11) Un'imposta di finestra: come la porta, ma più piccola (60 × 4 × 100 cm), si apre sempre verso fuori
 * (il lato X positivo) e quasi tutta, fino al muro. Per una finestra con due imposte se ne mettono due, una girata.
 * L'origine è il centro del davanzale: va messa all'altezza della finestra.
 */
UCLASS()
class VALDORSO_API AValdorsoImposta : public AValdorsoPorta
{
	GENERATED_BODY()

public:
	AValdorsoImposta();

	virtual FVector PuntoInterazione() const override;

protected:
	virtual FText TestoApri() const override;
	virtual FText TestoChiudi() const override;
	virtual FText TestoAChiave() const override;
};

/**
 * (03/11) La bacheca del villaggio: si legge con E. Gli avvisi si scrivono nei Dettagli (Avvisi) e il server li manda a
 * chi la legge. Più avanti ci appenderanno gli incarichi la Gilda, il Balivo e i giocatori.
 */
UCLASS()
class VALDORSO_API AValdorsoBacheca : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoBacheca();

	/** Il nome scritto in cima alla pergamena. */
	UPROPERTY(EditAnywhere, Category = "Bacheca")
	FText Titolo;

	/** Gli avvisi appesi (al massimo 20, di 300 lettere l'uno). */
	UPROPERTY(EditAnywhere, Category = "Bacheca", meta = (MultiLine = "true"))
	TArray<FString> Avvisi;

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Bacheca")
	TObjectPtr<UStaticMeshComponent> Tavola;

	UPROPERTY(VisibleAnywhere, Category = "Bacheca")
	TObjectPtr<UStaticMeshComponent> PaloSinistro;

	UPROPERTY(VisibleAnywhere, Category = "Bacheca")
	TObjectPtr<UStaticMeshComponent> PaloDestro;
};
