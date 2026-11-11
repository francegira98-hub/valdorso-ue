// Valdorso - Il pozzo del villaggio (v0.2 "L'acqua della valle", 11/11/2026). Scritto da Claude.
// Sta sul pozzo della piazza (lo mette Content/Python/nuovi/crea_volti.py, sull'attore "Pozzo" del villaggio).
// E: chi ha un secchio vuoto lo cala e lo tira su pieno d'acqua chiara; e intanto riempie la borraccia. Senza secchio,
// solo la borraccia (e se non c'e' nemmeno quella, un sorso con le mani).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoPozzo.generated.h"

class USphereComponent;

UCLASS()
class VALDORSO_API AValdorsoPozzo : public AActor, public IValdorsoInterattivo
{
	GENERATED_BODY()

public:
	AValdorsoPozzo();

	virtual FText TestoInterazione(const APawn* Chi) const override;
	virtual void Interagisci(APawn* Chi) override;
	virtual FVector PuntoInterazione() const override { return GetActorLocation() + FVector(0.f, 0.f, 80.f); }

private:
	UPROPERTY(VisibleAnywhere, Category = "Pozzo")
	TObjectPtr<USphereComponent> Presa;
};
