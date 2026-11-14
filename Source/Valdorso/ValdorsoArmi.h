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
// - (09/11) Il tasto Z ripone le armi: la spada nel fodero al fianco, lo scudo dietro la schiena (suono del fodero);
//   di nuovo Z, si riprendono in mano. Riposte non contano nel colpo (si colpisce a mani nude, lo scudo non para).
//   Le animazioni del riporre arriveranno con quelle vere (Fab o Mixamo); per ora le armi cambiano posto e basta.
// - Come stanno in mano (posizione e rotazione rispetto all'osso): per sistemarle a occhio, in gioco,
//   Valdorso.Arma.Presa destra <x> <y> <z> <pitch> <yaw> <roll> (o sinistra): cambia subito e scrive i numeri nel
//   registro (da mandare a Claude, che li scrive qui come valori di serie). Per trovare il verso: Valdorso.Arma.Prova
//   destra <1-6> [giro 0-3]. (09/11) Le armi stanno nelle mani del corpo MetaHuman (non del manichino nascosto).

// - (14/11, v0.2.3 "Armi vere") Le animazioni dei colpi per famiglia d'arma (ValdorsoQualita.h): quelle di Greystone
//   (Paragon, gratis su Fab) riadattate al colono da Content/Python/nuovi/porta_armi_greystone.py in /Game/Fab/Armi/Spada.
//   Pugnali: il colpo rapido; spade: rapido e medio a turno; asce: il colpo pesante; mazze: dall'alto; aste: il medio;
//   lo scudo da solo: la spinta della parata. Se l'animazione non c'e' (o Valdorso.Armi.Animazioni 0) si usa il pugno.
// - (14/11, v0.2.3) L'arco si tiene con due mani (ValdorsoOggetti.h, bDueMani): sta nella sinistra (il posto "arco", da
//   sistemare con Valdorso.Arma.Presa arco ...); il colpo tira una freccia (ValdorsoFreccia.h).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoArmi.generated.h"

class UStaticMeshComponent;
class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FValdorsoTipoOggetto;

namespace ValdorsoArmi
{
	/** Il modello di un'arma ("/Game/Valle/Pericoli/Armi/SM_Arma_Coltello.SM_Arma_Coltello"). */
	VALDORSO_API FString PercorsoModello(FName Id);

	/** (09/11) I posti delle armi: le due mani, il fianco (la spada riposta) e la schiena (lo scudo riposto). */
	inline constexpr int32 PostoDestra = 0;
	inline constexpr int32 PostoSinistra = 1;
	inline constexpr int32 PostoFianco = 2;
	inline constexpr int32 PostoSchiena = 3;
	/** (14/11, v0.2.3) L'arco, nella mano sinistra. */
	inline constexpr int32 PostoArco = 4;
	inline constexpr int32 NumeroPosti = 5;

	/** (14/11, v0.2.3) Quanto si tende l'arco prima che parta la freccia (secondi). */
	inline constexpr float SecondiTendere = 0.45f;

	/** (14/11, v0.2.3) Quanto va veloce una freccia (cm/s). */
	inline constexpr float VelocitaFreccia = 5200.f;

	/**
	 * (14/11, v0.2.3) L'animazione del colpo di un'arma (Numero: quante volte ha gia' colpito, per alternare), con l'istante
	 * del colpo (0..1 della sua durata) e la velocita'. nullptr se quell'arma usa il pugno (o l'animazione non c'e').
	 */
	VALDORSO_API UAnimSequenceBase* AnimazioneColpo(FName IdArma, int32 Numero, float& OutIstante, float& OutVelocita);

	/** Come sta un'arma in quel posto (rispetto al suo osso). */
	VALDORSO_API FTransform Presa(int32 Posto);

	/** L'osso del posto ("hand_r", "hand_l", "pelvis", "spine_05"). */
	VALDORSO_API FName Osso(int32 Posto);
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

	/** L'arma nella destra (nullptr = a mani nude, o riposta) e lo scudo nella sinistra (nullptr = niente, o sulla schiena). */
	const FValdorsoTipoOggetto* Arma() const;
	const FValdorsoTipoOggetto* Scudo() const;

	/** (09/11) Quello che porta, in mano o riposto. */
	const FValdorsoTipoOggetto* ArmaPortata() const;
	const FValdorsoTipoOggetto* ScudoPortato() const;

	/** (09/11) Riposte: la spada nel fodero, lo scudo dietro la schiena (si colpisce a mani nude). */
	bool Riposte() const { return bRiposte; }

	/** (09/11) Il tasto Z: ripone le armi o le riprende in mano. */
	UFUNCTION(Server, Reliable)
	void ServerAlterna();

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

	UStaticMeshComponent* Pezzo(UStaticMeshComponent* Attuale, FName Id, int32 Posto);

	UPROPERTY(ReplicatedUsing = OnRep_Armi)
	FName Destra;

	UPROPERTY(ReplicatedUsing = OnRep_Armi)
	FName Sinistra;

	UPROPERTY(ReplicatedUsing = OnRep_Armi)
	bool bRiposte = false;

	/** Su questo PC: com'erano l'ultima volta (per il suono solo quando cambia). */
	bool bRiposteMostrate = false;
	/** Sul server: non si ripone e si sfodera di continuo. */
	double ProssimoCambio = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> InDestra;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> InSinistra;
};
