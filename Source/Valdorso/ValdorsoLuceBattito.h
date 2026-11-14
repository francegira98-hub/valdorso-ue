// Valdorso - La lucina e lo spirito del Battito (v0.2.4 "PNG veri", blocco 3, 14/11/2026). Scritto da Claude.
//
// Due dei tre mattoni nuovi delle storie del Battito (ValdorsoBattito.h):
// - SEGUI: una lucina azzurra che si muove piano nel luogo della storia, a mezz'aria; il colono la deve seguire (stare
//   vicino) per un po'. Se la perde si spegne, e si riaccende quando torna li'.
// - FANTASMA: uno spirito del passato, una luce pallida che respira e ondeggia; avvicinandosi lo si sente parlare.
// Le vede SOLO il colono della storia (l'attore va in rete solo al suo PC: e' suo). La fa nascere il server; la toglie
// il Battito (o, per prudenza, dopo cinque minuti).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoLuceBattito.generated.h"

class APlayerController;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class VALDORSO_API AValdorsoLuceBattito : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoLuceBattito();

	/** Solo sul server: accende una lucina (o uno spirito) per quel colono, vicino a quel punto. */
	static AValdorsoLuceBattito* Accendi(APlayerController* Per, const FVector& Dove, bool bInSpirito);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Spirito();

	void Colora();

	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<UStaticMeshComponent> Sfera;

	UPROPERTY(VisibleAnywhere, Category = "Valdorso")
	TObjectPtr<UPointLightComponent> Luce;

	UPROPERTY(ReplicatedUsing = OnRep_Spirito)
	bool bSpirito = false;

	/** Solo sul server: da dove e' partita (la lucina non si allontana troppo) e verso dove va. */
	FVector Partenza = FVector::ZeroVector;
	float Rotta = 0.f;
	float Tempo = 0.f;
};
