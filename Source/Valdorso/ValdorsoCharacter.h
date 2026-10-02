// Copyright Epic Games, Inc. All Rights Reserved.
// Modificato per Valdorso: corsa nel componente di movimento, Gameplay Ability System (statistiche e abilità), schivata, pugno.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Logging/LogMacros.h"
#include "ValdorsoCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAbilitySystemComponent;
class UValdorsoAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UAnimMontage;
class UMotionWarpingComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  Il personaggio di Valdorso in terza persona.
 *  Telecamera che orbita, camminata e corsa sincronizzate dal server,
 *  statistiche e abilità con il Gameplay Ability System.
 */
UCLASS(abstract)
class AValdorsoCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Il contenitore delle abilità e degli effetti (Gameplay Ability System) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilita", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	/** Motion Warping: piega le animazioni per appoggiare mani e piedi sui bordi veri degli ostacoli */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UMotionWarpingComponent* MotionWarping;

	/** Le statistiche: salute, stamina, mana */
	UPROPERTY()
	UValdorsoAttributeSet* Attributi;

	/** Vero quando il server ha già dato abilità ed effetti iniziali (per non darli due volte) */
	bool bAbilitaDate = false;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	/** Corsa: tenendo premuto il tasto (Maiusc) il personaggio corre */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;

	/** Schivata: premendo il tasto (C) parte l'abilità della schivata */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* DodgeAction;

	/** Pugno: premendo il tasto (clic sinistro) parte l'abilità del pugno */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* PugnoAction;

	/** L'abilità della schivata (di serie quella di Valdorso in C++) */
	UPROPERTY(EditDefaultsOnly, Category = "Abilita")
	TSubclassOf<UGameplayAbility> AbilitaSchivata;


	/** L'abilità dello scavalcare e del salire sopra (di serie quella di Valdorso in C++) */
	UPROPERTY(EditDefaultsOnly, Category = "Abilita")
	TSubclassOf<UGameplayAbility> AbilitaScavalca;

	/** L'abilità del pugno (di serie quella di Valdorso in C++) */
	UPROPERTY(EditDefaultsOnly, Category = "Abilita")
	TSubclassOf<UGameplayAbility> AbilitaPugno;

	/** Effetti dati al personaggio quando nasce (di serie: il recupero della stamina) */
	UPROPERTY(EditDefaultsOnly, Category = "Abilita")
	TArray<TSubclassOf<UGameplayEffect>> EffettiIniziali;

public:

	/** L'animazione della schivata (montaggio), scelta nel Blueprint del personaggio */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilita")
	UAnimMontage* MontaggioSchivata;

	/**
	 * La capriola: quale pezzo del montaggio della schivata si usa, in fotogrammi (a 30 al secondo).
	 * Se l'ultimo non è più grande del primo, la schivata usa il montaggio intero e la spinta di serie.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilita|Capriola")
	int32 CapriolaPrimoFotogramma = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilita|Capriola")
	int32 CapriolaUltimoFotogramma = 55;

	/** Velocità della capriola (1 = come l'animazione, 1,5 = più svelta). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilita|Capriola", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float CapriolaVelocita = 1.5f;

	/**
	 * Spazio libero che serve davanti per la capriola, in centimetri. Se c'è un muro più vicino,
	 * al posto della capriola parte lo scatto corto (così testa e braccia non entrano nei muri).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilita|Capriola", meta = (ClampMin = "0"))
	float CapriolaSpazioMinimo = 180.f;

	/** Lo scatto corto da usare quando davanti non c'è spazio per la capriola (di serie AM_Schivata). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilita|Capriola")
	UAnimMontage* MontaggioScattoCorto;

	/** Quanta strada fa la capriola, in centimetri (solo se la capriola non ha il suo spostamento). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilita|Capriola", meta = (ClampMin = "0"))
	float CapriolaDistanza = 400.f;

	/** L'animazione del pugno (montaggio sulla corsia UpperBody), scelta nel Blueprint del personaggio */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilita")
	UAnimMontage* MontaggioPugno;


	/** Animazioni dello scavalcare (Vault) e del salire sopra (Mantle), da ferma, camminando, correndo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scavalcare")
	UAnimMontage* ScavalcaDaFermo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scavalcare")
	UAnimMontage* ScavalcaCamminando;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scavalcare")
	UAnimMontage* ScavalcaCorrendo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scavalcare")
	UAnimMontage* SaliDaFermo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scavalcare")
	UAnimMontage* SaliCamminando;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scavalcare")
	UAnimMontage* SaliCorrendo;

	/** Nomi dei bersagli del Motion Warping nei montaggi del Game Animation Sample */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scavalcare")
	FName BersaglioBordoDavanti = TEXT("FrontLedge");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scavalcare")
	FName BersaglioBordoDietro = TEXT("BackLedge");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scavalcare")
	FName BersaglioTerraDietro = TEXT("BackFloor");

	/** Sceglie l'animazione giusta in base all'ostacolo e alla velocità */
	UAnimMontage* ScegliMontaggioScavalca(bool bSaliSopra) const;

	/** Il componente del Motion Warping */
	FORCEINLINE UMotionWarpingComponent* GetMotionWarping() const { return MotionWarping; }

	/** Prova a scavalcare o salire: vero se l'abilità è partita (altrimenti si salta) */
	bool ProvaScavalcare();

	/** Costruttore: usa il componente di movimento di Valdorso al posto di quello di serie */
	AValdorsoCharacter(const FObjectInitializer& ObjectInitializer);

	/** Per il Gameplay Ability System: dove si trovano le abilità di questo personaggio */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Sul server: quando un giocatore prende il personaggio, prepara abilità ed effetti */
	virtual void PossessedBy(AController* NewController) override;

	/** Sul PC del giocatore: prepara il contenitore delle abilità quando arrivano i dati dal server */
	virtual void OnRep_PlayerState() override;

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	/** Inizia a correre (tasto premuto) */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoCorsaInizio();

	/** Smette di correre (tasto lasciato) */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoCorsaFine();

	/** Prova a fare la schivata (parte solo se c'è abbastanza stamina) */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoSchivata();

	/** Prova a tirare un pugno (parte solo a terra e con abbastanza stamina) */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoPugno();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};