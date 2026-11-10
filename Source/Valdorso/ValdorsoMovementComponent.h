// Valdorso - Componente di movimento del personaggio.
// Aggiunge la corsa (tenendo Maiusc) al movimento di serie di Unreal, in modo corretto per il gioco online:
// il PC del giocatore prevede subito la corsa, il server la riceve insieme a ogni passo e la conferma.
// (06/11) Il nuoto nell'acqua della valle (lago, fiumi, pozza): vedi ValdorsoAcque.h. Con l'acqua al petto si nuota in
// superficie, quando si tocca di nuovo si cammina; lo decidono allo stesso modo il server e il PC.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ValdorsoMovementComponent.generated.h"

class AValdorsoAcque;

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

	/** (08/11) Il peso dello zaino (ValdorsoZaino.h): quanto si va piano (1 = normale) e se si può ancora correre.
	 *  Li mette lo zaino, uguali sul server e sul PC di chi gioca (così la previsione del movimento torna). */
	float FattoreCarico = 1.f;
	bool bCaricoPermetteCorsa = true;

	/** (06/11) Con quanta acqua addosso si comincia a nuotare (0..1 dell'altezza: 0,6 = al petto). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valdorso|Nuoto", meta = (ClampMin = "0.1", ClampMax = "1"))
	float AcquaPerNuotare = 0.6f;

	/** (06/11) Sotto quanta acqua si smette di nuotare e si torna a camminare (più bassa: niente avanti e indietro). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valdorso|Nuoto", meta = (ClampMin = "0", ClampMax = "1"))
	float AcquaPerUscire = 0.35f;

	/** (06/11) Quanto è sott'acqua il corpo adesso secondo le acque della valle (0..1). */
	float ImmersioneNellaValle() const;

	/** (10/11, v0.1.4) Quanti cm d'acqua ci sono sopra la testa (negativo: la testa e' fuori; molto negativo fuori
	 *  dall'acqua). Sott'acqua si trattiene il fiato (ValdorsoBagnato.h). */
	float AcquaSopraLaTesta() const;

	virtual bool IsInWater() const override;
	virtual float ImmersionDepth() const override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

protected:
	/** (06/11) Il nuoto di serie, che alla fine rimette "cadere" se non c'è un Physics Volume d'acqua: qui si torna a nuotare. */
	virtual void PhysSwimming(float deltaTime, int32 Iterations) override;

public:

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/** Stato della corsa: viaggia verso il server insieme a ogni passo del giocatore. */
	uint8 bVuoleCorrere : 1;

private:
	/** Le acque del livello (cercate la prima volta che servono, e di nuovo ogni 2 s finché non ci sono). */
	mutable TWeakObjectPtr<AValdorsoAcque> Acque;
	mutable double ProssimaRicercaAcque = 0.0;
};