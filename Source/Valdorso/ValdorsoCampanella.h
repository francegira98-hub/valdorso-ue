// Valdorso - La campanella della caserma (v0.2.7.1 "Le rifiniture", 20/11/2026). Scritto da Claude.
//
// La campana di bronzo sul corpo di guardia della caserma delle Guardie di Valdorso (modelli della chat 3D:
// SM_Campanella_Caserma e SM_Campanella_Battaglio, V:/Texture/valdorso/campanella). Una guardia tira la corda con E: il
// battaglio dondola, la campana suona e chi e' li' intorno lo sa. Chi non e' delle guardie non la tocca.
// Porta con se' anche il posto della garitta accanto al portale (Garitta, nel mondo): e' li' che si fa il turno di
// guardia in garitta (ValdorsoMilizia.h, TurnoGaritta). La mette Content/Python/nuovi/crea_mura.py ("Mura: 6").

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoCampanella.generated.h"

class UStaticMeshComponent;

UCLASS()
class VALDORSO_API AValdorsoCampanella : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoCampanella();

	/** La mensola con la campana (ferma). */
	UPROPERTY(VisibleAnywhere, Category = "Campanella")
	TObjectPtr<UStaticMeshComponent> Modello;

	/** Il battaglio con la corda: dondola intorno al suo asse X. */
	UPROPERTY(VisibleAnywhere, Category = "Campanella")
	TObjectPtr<UStaticMeshComponent> Battaglio;

	/** Dove sta in piedi la guardia di turno nella garitta (nel mondo; zero = non c'e' la garitta). */
	UPROPERTY(EditAnywhere, Category = "Campanella")
	FVector Garitta = FVector::ZeroVector;

	/** Quanto dondola il battaglio (gradi) e per quanto (secondi). */
	UPROPERTY(EditAnywhere, Category = "Campanella")
	float Gradi = 25.f;

	UPROPERTY(EditAnywhere, Category = "Campanella")
	float Secondi = 3.5f;

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	/** A tutti: la campana suona (il battaglio dondola, il suono). */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSuona();

	/** Da quanto dondola (secondi; negativo = ferma). */
	float Dondola = -1.f;

	/** Sul server: quando e' stata suonata l'ultima volta (FPlatformTime::Seconds). */
	double UltimaVolta = 0.0;
};
