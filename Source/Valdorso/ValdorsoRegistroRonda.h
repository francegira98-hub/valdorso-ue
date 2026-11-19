// Valdorso - Il registro della ronda (v0.2.7 "Il tempio", 19/11/2026). Scritto da Claude.
//
// Una cassetta di ferro con un libro dentro, su un palo o a muro (modelli della chat 3D: SM_Registro_Ronda_Palo,
// SM_Registro_Ronda_Muro), in quattro punti del villaggio: locanda, forgia, tempio, mulino. La guardia di ronda
// (ValdorsoMilizia.h, il turno "ronda" preso dalla capitana Livia) passa e firma con E: firmati tutti, il giro e' fatto.
// Li mette Content/Python/nuovi/crea_mura.py ("Mura: 12. i registri della ronda"). Se nel livello non ce ne sono
// almeno due, la ronda resta quella di prima (farsi vedere dagli abitanti).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoRegistroRonda.generated.h"

class UStaticMeshComponent;

UCLASS()
class VALDORSO_API AValdorsoRegistroRonda : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoRegistroRonda();

	/** Il nome corto del punto ("Locanda"): due registri con lo stesso nome contano come uno. */
	UPROPERTY(EditAnywhere, Category = "Ronda")
	FName Tappa;

	/** Come lo si nomina negli avvisi ("la locanda di Bruno"). */
	UPROPERTY(EditAnywhere, Category = "Ronda")
	FString Luogo;

	/** Il modello (lo mette lo script). */
	UPROPERTY(VisibleAnywhere, Category = "Ronda")
	TObjectPtr<UStaticMeshComponent> Modello;

	// IValdorsoInterattivo
	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 110.f); }
};
