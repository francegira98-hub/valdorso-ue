// Valdorso - Scavalcare e salire sugli ostacoli bassi, come abilità del Gameplay Ability System.
// Guarda davanti al personaggio: ostacolo sottile con terra dietro = scavalca (Vault);
// ostacolo spesso con spazio sopra = sale sopra (Mantle). Usa le animazioni del Game Animation Sample
// con il Motion Warping, che piega l'animazione sui bordi veri dell'ostacolo.
//
// Versione 3 (02/10, passo 5.4k): il controllo davanti guarda più lontano quando si corre, e se il
// personaggio è già vicino all'ostacolo il montaggio parte più avanti (salta la rincorsa che non c'è),
// così il corpo non entra nell'ostacolo prima che il Motion Warping cominci.
//
// Le fasi (versione 2 del 02/10, passo 5.4g):
// - all'inizio: volo guidato dall'animazione (MOVE_Flying), urti con l'ostacolo spenti;
// - Vault: appena il corpo ha superato il bordo dietro, urti di nuovo accesi e caduta (MOVE_Falling);
//   quando tocca terra il montaggio sfuma e si torna a camminare;
// - Mantle: alla fine dell'ultimo rettangolo MotionWarping il corpo è sopra il piano: urti accesi
//   (il piano lo regge) e camminata (MOVE_Walking); da fermo l'animazione finisce, in movimento sfuma.
// Durante il passaggio l'asta della telecamera non accorcia contro l'ostacolo.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Abilities/GameplayAbility.h"
#include "ValdorsoAbilitaScavalca.generated.h"

class ACharacter;
class UPrimitiveComponent;
class USpringArmComponent;
class UAnimMontage;

/** Cosa ha trovato il controllo davanti al personaggio. */
struct FValdorsoOstacolo
{
	bool bTrovato = false;
	bool bSaliSopra = false;                 // vero: sale sopra (Mantle); falso: scavalca (Vault)
	float Altezza = 0.f;
	FVector BordoDavanti = FVector::ZeroVector;
	FVector BordoDietro = FVector::ZeroVector;
	FVector TerraDietro = FVector::ZeroVector;
	FVector Normale = FVector::ZeroVector;   // la faccia dell'ostacolo, rivolta verso il personaggio
	TWeakObjectPtr<UPrimitiveComponent> Componente;
};

UCLASS()
class VALDORSO_API UValdorsoAbilitaScavalca : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UValdorsoAbilitaScavalca();

	/** Guarda davanti al personaggio e dice se c'è un ostacolo da scavalcare o su cui salire. */
	static bool TrovaOstacolo(const ACharacter* Personaggio, FValdorsoOstacolo& Risultato);

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Quanto dura la sfumatura finale del montaggio dopo l'atterraggio (secondi). */
	UPROPERTY(EditDefaultsOnly, Category = "Valdorso|Scavalcare")
	float SfumaturaUscita = 0.25f;

protected:
	UFUNCTION()
	void QuandoFinisce();

	/** Finiti i rettangoli MotionWarping: l'animazione ha portato il corpo oltre il bordo. */
	UFUNCTION()
	void QuandoFinisceIlWarping();

	/** Vault: controllato spesso; quando il corpo ha superato il bordo dietro, si torna a cadere. */
	UFUNCTION()
	void ControllaPassaggio();

	/** Riaccende gli urti con l'ostacolo (una volta sola). */
	void RiaccendiUrti();

	/** Vault: il personaggio ha toccato terra dall'altra parte. */
	UFUNCTION()
	void QuandoAtterra(EMovementMode NuovaModalita);

	/** Fine dell'ultimo rettangolo MotionWarping del montaggio, in secondi (0 se non ce ne sono). */
	static float FineUltimoWarping(const UAnimMontage* Montaggio);

	/**
	 * Da che secondo far partire il montaggio perché la rincorsa dell'animazione sia lunga quanto la
	 * distanza vera dall'ostacolo. Pista: quanto è lontano il bordo nell'animazione di Epic.
	 */
	static float CalcolaInizio(const UAnimMontage* Montaggio, FName BersaglioDavanti, float DistanzaVera, float& Pista);

	/** L'ostacolo con cui il personaggio non urta durante il passaggio. */
	TWeakObjectPtr<UPrimitiveComponent> ComponenteIgnorato;

	/** L'asta della telecamera e com'era il suo controllo degli urti prima del passaggio. */
	TWeakObjectPtr<USpringArmComponent> AstaTelecamera;
	bool bAstaUrtiPrima = true;

	/** Vero: si sta salendo sopra (Mantle); falso: si scavalca (Vault). */
	bool bSaliSopraAttivo = false;

	/** Vero se all'inizio il personaggio era fermo (montaggio "stand"). */
	bool bFermoAllInizio = false;

	/** Vault: già passato oltre il muretto e in caduta. */
	bool bOltreIlMuretto = false;

	/** Bordo dietro e faccia dell'ostacolo, per capire quando il corpo l'ha superato. */
	FVector BordoDietroAttivo = FVector::ZeroVector;
	FVector NormaleAttiva = FVector::ZeroVector;

	/** Il secondo da cui è partito il montaggio. */
	float InizioMontaggio = 0.f;

	/** Il controllo ripetuto del Vault. */
	FTimerHandle TimerPassaggio;
};