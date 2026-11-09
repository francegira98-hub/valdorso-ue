// Valdorso - Il bosco (v0.1.4, 09/11/2026, chiesto da Fra: "il bosco e gli alberi ovunque"). Scritto da Claude.
//
// Un AValdorsoBosco tiene gli alberi di un pezzo della valle (750 x 750 m): per ogni modello (abete, faggio, nocciolo...)
// l'elenco delle posizioni. In gioco li disegna tutti insieme (Hierarchical Instanced Static Mesh: un solo modello
// per migliaia di alberi) e mette a ogni tronco un cilindro invisibile che ferma il colono (le fronde no: ci si passa
// in mezzo). Il server dedicato non disegna niente: tiene solo i cilindri (il movimento lo decide lui).
// I modelli e le posizioni li fanno Tools/Generatori/bosco.py (le regole: bosco grande a ovest, boschetto a est,
// radure, niente sulle strade) e Content/Python/nuovi/crea_bosco.py (che mette gli attori nel livello).
// Quando arrivano gli alberi di Fab, basta cambiare il Modello di una specie: le posizioni restano.
// Comando di console: Valdorso.Bosco.Lontano <metri> (da quanto lontano spariscono cespugli, ceppi e tronchi a terra;
// 0 = come deciso dal generatore).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoBosco.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

/** Una specie (un modello) del bosco e dove sta. */
USTRUCT(BlueprintType)
struct FValdorsoSpecieBosco
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bosco")
	TObjectPtr<UStaticMesh> Modello = nullptr;

	/** Dove sta ogni albero (rispetto all'attore). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bosco")
	TArray<FTransform> Dove;

	/** Il raggio del tronco (cm, prima della scala dell'albero): il cilindro che ferma il colono. 0 = niente. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bosco")
	float RaggioTronco = 0.f;

	/** Quanto e' alto il cilindro (cm): per un tronco a terra, quanto e' lungo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bosco")
	float AltezzaTronco = 400.f;

	/** Vero per i tronchi caduti: il cilindro sta sdraiato lungo X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bosco")
	bool bSdraiato = false;

	/** Da quanti cm non si disegna piu' (0 = sempre). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bosco")
	float Sparisce = 0.f;

	/** Fa ombra (i cespugli lontani no). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bosco")
	bool bOmbra = true;
};

namespace ValdorsoBosco
{
	/** Il cilindro invisibile di un tronco per un albero messo con Albero (il cilindro di Unreal e' alto 100 e largo 100,
	 *  con il centro a meta'). Raggio e altezza in cm; con bSdraiato il cilindro sta lungo X, appoggiato a terra. */
	VALDORSO_API FTransform Tronco(const FTransform& Albero, float Raggio, float Altezza, bool bSdraiato);
}

UCLASS()
class VALDORSO_API AValdorsoBosco : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoBosco();

	/** Le specie di questo pezzo di bosco. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bosco")
	TArray<FValdorsoSpecieBosco> Specie;

	/** (Per crea_bosco.py) Toglie tutte le specie. */
	UFUNCTION(BlueprintCallable, Category = "Bosco")
	void Svuota();

	/** (Per crea_bosco.py) Aggiunge una specie con i suoi alberi. */
	UFUNCTION(BlueprintCallable, Category = "Bosco")
	void AggiungiSpecie(UStaticMesh* Modello, const TArray<FTransform>& Dove, float RaggioTronco, float AltezzaTronco,
		bool bSdraiato, float Sparisce, bool bOmbra);

	/** Rifa' le istanze dalle specie (nell'editor lo fa da solo quando cambia qualcosa). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Bosco")
	void Ricostruisci();

	/** Quanti alberi in tutto. */
	UFUNCTION(BlueprintPure, Category = "Bosco")
	int32 QuantiAlberi() const;

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Togli();

	/** Il cilindro dei tronchi invisibili (/Engine/BasicShapes/Cylinder). */
	UPROPERTY()
	TObjectPtr<UStaticMesh> Cilindro;

	/** I componenti fatti da Ricostruisci (non si salvano: si rifanno ogni volta dalle specie). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Fatti;
};
