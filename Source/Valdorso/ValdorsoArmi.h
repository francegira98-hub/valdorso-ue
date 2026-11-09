// Valdorso - Le armi in mano (v0.1.4 passo 4.5, provvisorio, 09/11/2026). Scritto da Claude.
//
// Come funziona:
// - Nello zaino, clic destro su un'arma (coltello, pugnale, falcetto, martello, bastone da pastore, spada corta) la
//   impugna nella destra; sullo scudo di legno, nella sinistra. Di nuovo, la si rimette via. Quello che si impugna sta
//   nello zaino (FValdorsoZaino::Destra e Sinistra, salvato con il personaggio): se si getta, sparisce anche di mano.
// - UValdorsoArmiComponent sta su ogni colono: sa cosa ha in mano (arriva a tutti, cosi' tutti lo vedono) e lo
//   attacca alle mani del manichino (ossa hand_r e hand_l), con i modelli provvisori di Tools/Generatori/pericoli.py
//   (/Game/Valle/Pericoli/Armi/SM_Arma_<Id>). Quando arrivano quelli di Fab si cambia il modello, il resto resta.
// - Il colpo: il tasto del pugno, con l'arma in mano, usa il danno, l'equilibrio, la stamina e la portata dell'arma
//   (ValdorsoOggetti.cpp, "le armi") e il suo suono (lama o botta). Lo scudo nella sinistra ferma una parte dei colpi
//   che arrivano davanti (ValdorsoCombattimento::Colpisci).
// - Come stanno in mano (posizione e rotazione rispetto all'osso): per sistemarle a occhio, in gioco,
//   Valdorso.Arma.Presa destra <x> <y> <z> <pitch> <yaw> <roll> (o sinistra): cambia subito e scrive i numeri nel
//   registro (da mandare a Claude, che li scrive qui come valori di serie). Per trovare il verso: Valdorso.Arma.Prova
//   destra <1-6> [giro 0-3]. (09/11) Le armi stanno nelle mani del corpo MetaHuman (non del manichino nascosto).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoArmi.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
struct FValdorsoTipoOggetto;

namespace ValdorsoArmi
{
	/** Il modello di un'arma ("/Game/Valle/Pericoli/Armi/SM_Arma_Coltello.SM_Arma_Coltello"). */
	VALDORSO_API FString PercorsoModello(FName Id);

	/** Come sta un'arma nella mano (rispetto all'osso della mano). */
	VALDORSO_API FTransform Presa(bool bSinistra);

	/** L'osso a cui si attacca ("hand_r", "hand_l"). */
	VALDORSO_API FName Osso(bool bSinistra);
}

UCLASS(ClassGroup = (Valdorso))
class VALDORSO_API UValdorsoArmiComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoArmiComponent();

	static UValdorsoArmiComponent* Di(const AActor* Chi);

	/** Solo sul server: cosa ha in mano (dallo zaino). */
	void Imposta(FName NuovaDestra, FName NuovaSinistra);

	FName GetDestra() const { return Destra; }
	FName GetSinistra() const { return Sinistra; }

	/** L'arma nella destra (nullptr = a mani nude) e lo scudo nella sinistra (nullptr = niente). */
	const FValdorsoTipoOggetto* Arma() const;
	const FValdorsoTipoOggetto* Scudo() const;

	/** Rimette i modelli in mano (anche dopo Valdorso.Arma.Presa). */
	void Mostra();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_Armi();

	/** (09/11) Le mani che si vedono: il corpo MetaHuman se c'e', se no il manichino. */
	USkeletalMeshComponent* Mano() const;

	UStaticMeshComponent* Pezzo(UStaticMeshComponent* Attuale, FName Id, bool bSinistra);

	UPROPERTY(ReplicatedUsing = OnRep_Armi)
	FName Destra;

	UPROPERTY(ReplicatedUsing = OnRep_Armi)
	FName Sinistra;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> InDestra;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> InSinistra;
};
