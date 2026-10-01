// Valdorso - Componente di movimento del personaggio.
// Aggiunge la corsa (tenendo Maiusc) al movimento di serie di Unreal, in modo corretto per il gioco online:
// il PC del giocatore prevede subito la corsa, il server la riceve insieme a ogni passo e la conferma.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ValdorsoMovementComponent.generated.h"

UCLASS(ClassGroup = (Valdorso), meta = (BlueprintSpawnableComponent))
class VALDORSO_API UValdorsoMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UValdorsoMovementComponent();

	/** Velocità della camminata, in centimetri al secondo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valdorso|Movimento", meta = (ClampMin = "0"))
	float VelocitaCamminata = 250.f;

	/** Velocità della corsa, in centimetri al secondo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valdorso|Movimento", meta = (ClampMin = "0"))
	float VelocitaCorsa = 550.f;

	/** Accende o spegne la corsa (chiamata dal personaggio quando si preme o si lascia il tasto). */
	UFUNCTION(BlueprintCallable, Category = "Valdorso|Movimento")
	void ImpostaCorsa(bool bCorrere);

	/** Vero mentre il giocatore vuole correre. */
	UFUNCTION(BlueprintPure, Category = "Valdorso|Movimento")
	bool VuoleCorrere() const { return bVuoleCorrere; }

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/** Stato della corsa: viaggia verso il server insieme a ogni passo del giocatore. */
	uint8 bVuoleCorrere : 1;
};