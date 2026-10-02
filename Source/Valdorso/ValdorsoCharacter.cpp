// Copyright Epic Games, Inc. All Rights Reserved.
// Modificato per Valdorso: corsa nel componente di movimento, Gameplay Ability System (statistiche e abilità), schivata.

#include "ValdorsoCharacter.h"
#include "ValdorsoMovementComponent.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoEffetti.h"
#include "ValdorsoAbilitaSchivata.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Valdorso.h"

AValdorsoCharacter::AValdorsoCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UValdorsoMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Le velocità di camminata e corsa stanno nel componente di movimento di Valdorso
	// (VelocitaCamminata e VelocitaCorsa, regolabili anche nel Blueprint del personaggio).
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Gameplay Ability System: il contenitore delle abilità viaggia in rete;
	// "Mixed" = il giocatore riceve tutti i dettagli dei suoi effetti, gli altri solo l'essenziale.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Le statistiche (salute, stamina, mana), che il contenitore trova da solo.
	Attributi = CreateDefaultSubobject<UValdorsoAttributeSet>(TEXT("Attributi"));

	// Di serie: la schivata di Valdorso e il recupero della stamina.
	AbilitaSchivata = UValdorsoAbilitaSchivata::StaticClass();
	EffettiIniziali.Add(UValdorsoGE_RecuperoStamina::StaticClass());

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

UAbilitySystemComponent* AValdorsoCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AValdorsoCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent == nullptr)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Solo il server dà le abilità e gli effetti iniziali, e una volta sola.
	if (HasAuthority() && !bAbilitaDate)
	{
		bAbilitaDate = true;

		if (AbilitaSchivata)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilitaSchivata, 1, INDEX_NONE, this));
		}

		FGameplayEffectContextHandle Contesto = AbilitySystemComponent->MakeEffectContext();
		Contesto.AddSourceObject(this);
		for (const TSubclassOf<UGameplayEffect>& Effetto : EffettiIniziali)
		{
			if (Effetto)
			{
				AbilitySystemComponent->ApplyGameplayEffectToSelf(Effetto->GetDefaultObject<UGameplayEffect>(), 1.f, Contesto);
			}
		}
	}
}

void AValdorsoCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AValdorsoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AValdorsoCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AValdorsoCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AValdorsoCharacter::Look);

		// Corsa: si collega solo se nel Blueprint è stata scelta l'azione (IA_Sprint)
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AValdorsoCharacter::DoCorsaInizio);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AValdorsoCharacter::DoCorsaFine);
		}

		// Schivata: si collega solo se nel Blueprint è stata scelta l'azione (IA_Schivata)
		if (DodgeAction)
		{
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AValdorsoCharacter::DoSchivata);
		}
	}
	else
	{
		UE_LOG(LogValdorso, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AValdorsoCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AValdorsoCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AValdorsoCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AValdorsoCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AValdorsoCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AValdorsoCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AValdorsoCharacter::DoCorsaInizio()
{
	if (UValdorsoMovementComponent* Movimento = Cast<UValdorsoMovementComponent>(GetCharacterMovement()))
	{
		Movimento->ImpostaCorsa(true);
	}
}

void AValdorsoCharacter::DoCorsaFine()
{
	if (UValdorsoMovementComponent* Movimento = Cast<UValdorsoMovementComponent>(GetCharacterMovement()))
	{
		Movimento->ImpostaCorsa(false);
	}
}

void AValdorsoCharacter::DoSchivata()
{
	if (AbilitySystemComponent && AbilitaSchivata)
	{
		AbilitySystemComponent->TryActivateAbilityByClass(AbilitaSchivata);
	}
}