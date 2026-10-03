// Valdorso - Il controllore del menu principale: mostra il menu "Oro e brace", guarda la scena
// dalla telecamera con l'etichetta MenuCamera (che ondeggia piano), e porta nella valle o chiude il gioco.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ValdorsoMenuController.generated.h"

class SValdorsoMenuPrincipale;

UCLASS()
class VALDORSO_API AValdorsoMenuController : public APlayerController
{
	GENERATED_BODY()

public:
	AValdorsoMenuController();

	/** Il livello che si apre con "Entra nella valle" (per ora la mappa di prova; dalla v0.1.3 la valle). */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	FName LivelloValle = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");

	/** Quanto dura il volo verso il frammento prima di entrare nella valle, in secondi. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	float DurataTransizione = 2.4f;

	/** Quanto ondeggia la telecamera, in centimetri. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Menu")
	float Ondeggio = 14.f;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void EntraNellaValle();
	void Esci();

	TSharedPtr<SValdorsoMenuPrincipale> Menu;
	TWeakObjectPtr<AActor> Telecamera;
	FVector PosizioneBase = FVector::ZeroVector;
	FRotator RotazioneBase = FRotator::ZeroRotator;
	float Tempo = 0.f;

	/** "Entra nella valle": la telecamera vola verso il frammento, il menu sfuma, tutto diventa nero. */
	void AvanzaTransizione(float DeltaSeconds);

	bool bInTransizione = false;
	bool bBuio = false;
	bool bLivelloAperto = false;
	float TempoTransizione = 0.f;
	FVector Partenza = FVector::ZeroVector;
	FQuat GiroPartenza = FQuat::Identity;
	FVector Arrivo = FVector::ZeroVector;
	FQuat GiroArrivo = FQuat::Identity;
};
