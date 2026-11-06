// Valdorso - Le scale a pioli (vedi il .h). Scritto da Claude il 06/11/2026.

#include "ValdorsoScala.h"
#include "Valdorso.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoSeduta.h"
#include "ValdorsoLetto.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "ValdorsoScala"

namespace ValdorsoScale
{
	int32 Estremo(float ZCentro, float Altezza, float MezzaCapsula, float VelocitaZ)
	{
		// In cima: quando i piedi arrivano poco sotto il ballatoio e sta salendo, esce.
		if (VelocitaZ > 1.f && ZCentro - MezzaCapsula >= Altezza - 60.f)
		{
			return 1;
		}
		// In fondo: quando i piedi toccano terra e sta scendendo, scende.
		if (VelocitaZ < -1.f && ZCentro - MezzaCapsula <= 8.f)
		{
			return -1;
		}
		return 0;
	}

	float QuotaInterazione(float ZColono, float Altezza)
	{
		return FMath::Clamp(ZColono, 100.f, Altezza + 100.f);
	}
}

namespace
{
	float ScalaMezzaCapsula(const ACharacter* Colono)
	{
		return Colono && Colono->GetCapsuleComponent() ? Colono->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	}

	/** Vero se il colono sta già facendo altro (seduto, a letto, su un'altra scala). */
	bool ScalaColonoOccupato(const APawn* Chi)
	{
		if (const UValdorsoSedutaComponent* Seduta = Chi->FindComponentByClass<UValdorsoSedutaComponent>(); Seduta && Seduta->Occupato())
		{
			return true;
		}
		if (const UValdorsoSonnoComponent* Sonno = Chi->FindComponentByClass<UValdorsoSonnoComponent>(); Sonno && Sonno->Occupato())
		{
			return true;
		}
		return false;
	}
}

// ================================================================================================ il componente

UValdorsoScalataComponent::UValdorsoScalataComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UValdorsoScalataComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UValdorsoScalataComponent, Stato);
}

double UValdorsoScalataComponent::Adesso() const
{
	const UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return 0.0;
	}
	const AGameStateBase* StatoGioco = Mondo->GetGameState();
	return StatoGioco ? StatoGioco->GetServerWorldTimeSeconds() : Mondo->GetTimeSeconds();
}

void UValdorsoScalataComponent::Inizia(AValdorsoScala* Scala, bool bDallaCima)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || !Colono->HasAuthority() || !Scala || Sale())
	{
		return;
	}
	const float Mezza = ScalaMezzaCapsula(Colono);
	// Dal fondo: i piedi appena sopra il pavimento. Dalla cima: i piedi poco sotto il ballatoio (la testa passa nella
	// botola), un poco più in basso del punto in cui si esce, così non riesce subito.
	const float Quota = bDallaCima ? Scala->Altezza - 120.f + Mezza : Mezza + 3.f;
	Stato.Scala = Scala;
	Stato.Inizio = Adesso();
	Stato.Dove = Scala->PuntoSullaScala(Quota);
	Stato.Yaw = Scala->YawSalita();
	Stato.bSposta = true;
	ValdorsoRegistroEventi::Annota(TEXT("Scala"), Colono, Scala, Stato.Dove, bDallaCima ? TEXT("scende") : TEXT("sale"));
	Colono->ForceNetUpdate();
	OnRep_Stato();
}

void UValdorsoScalataComponent::Finisci(int32 Estremo)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	AValdorsoScala* Scala = Stato.Scala;
	if (!Colono || !Colono->HasAuthority() || !Scala)
	{
		return;
	}
	const float Mezza = ScalaMezzaCapsula(Colono);
	Stato.Scala = nullptr;
	Stato.Inizio = Adesso();
	Stato.Yaw = Colono->GetActorRotation().Yaw;
	if (Estremo > 0)
	{
		// In cima: sul ballatoio, accanto alla botola, girato verso dove guardava salendo.
		Stato.Dove = Scala->GetActorTransform().TransformPosition(FVector(Scala->UscitaCima, 0.f, Scala->Altezza + Mezza + 5.f));
		Stato.Yaw = Scala->YawSalita() + 180.f;
		Stato.bSposta = true;
	}
	else if (Estremo < 0)
	{
		// In fondo: in piedi dove è arrivato (un passo indietro dai pioli).
		Stato.Dove = Scala->PuntoSullaScala(Mezza + 3.f) + Scala->GetActorForwardVector() * 25.f;
		Stato.bSposta = true;
	}
	else
	{
		// La lascia: cade da dov'è, con una piccola spinta via dalla scala (sotto c'è il vuoto della torre).
		Stato.Dove = FVector::ZeroVector;
		Stato.bSposta = false;
	}
	Colono->ForceNetUpdate();
	OnRep_Stato();
	if (Estremo == 0)
	{
		Colono->LaunchCharacter(Scala->GetActorForwardVector() * 120.f, true, true);
	}
}

void UValdorsoScalataComponent::ChiediDiLasciare()
{
	const double Ora = FPlatformTime::Seconds();
	if (!Sale() || Ora - UltimaRichiesta < 0.4)
	{
		return;
	}
	UltimaRichiesta = Ora;
	ServerLascia();
}

void UValdorsoScalataComponent::ServerLascia_Implementation()
{
	Finisci(0);
}

void UValdorsoScalataComponent::OnRep_Stato()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono)
	{
		return;
	}
	const bool bSullaScala = Stato.Scala != nullptr;
	const bool bNuovo = bSullaScala != bSullaScalaVisto || Stato.Inizio != InizioVisto;
	if (!bNuovo)
	{
		return;
	}
	InizioVisto = Stato.Inizio;
	bSullaScalaVisto = bSullaScala;
	// Si sposta solo su chi decide (server) e su chi lo guida (il suo PC), e solo per un cambio appena fatto: gli altri
	// lo vedono già muoversi con la rete, e chi arriva dopo non deve finire in un'uscita vecchia.
	const bool bRecente = Adesso() - Stato.Inizio < 2.0;
	if (Stato.bSposta && Colono->GetLocalRole() != ROLE_SimulatedProxy && (bSullaScala || bRecente))
	{
		Colono->SetActorLocationAndRotation(Stato.Dove, FRotator(0.f, Stato.Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
	ApplicaMovimento(bSullaScala);
	SetComponentTickEnabled(bSullaScala);
}

void UValdorsoScalataComponent::ApplicaMovimento(bool bSullaScala)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* Movimento = Colono ? Colono->GetCharacterMovement() : nullptr;
	if (!Movimento)
	{
		return;
	}
	if (bSullaScala)
	{
		if (!bMovimentoSalvato)
		{
			VelocitaVoloPrima = Movimento->MaxFlySpeed;
			FrenataVoloPrima = Movimento->BrakingDecelerationFlying;
			bGiravaPrima = Movimento->bOrientRotationToMovement;
			bMovimentoSalvato = true;
		}
		Movimento->StopMovementImmediately();
		Movimento->MaxFlySpeed = Stato.Scala ? Stato.Scala->Velocita : 140.f;
		Movimento->BrakingDecelerationFlying = 4000.f;       // si ferma appena si lascia W o S
		Movimento->bOrientRotationToMovement = false;        // resta con la faccia ai pioli
		Movimento->SetMovementMode(MOVE_Flying);
	}
	else
	{
		if (bMovimentoSalvato)
		{
			Movimento->MaxFlySpeed = VelocitaVoloPrima;
			Movimento->BrakingDecelerationFlying = FrenataVoloPrima;
			Movimento->bOrientRotationToMovement = bGiravaPrima;
			bMovimentoSalvato = false;
		}
		if (Movimento->MovementMode == MOVE_Flying)
		{
			Movimento->SetMovementMode(Stato.bSposta ? MOVE_Walking : MOVE_Falling);
		}
	}
}

void UValdorsoScalataComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	AValdorsoScala* Scala = Stato.Scala;
	if (!Colono || !Scala)
	{
		SetComponentTickEnabled(false);
		return;
	}
	UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement();
	// Se qualcosa (un colpo, la rete) ha tolto il volo mentre è sulla scala, si rimette.
	if (Movimento && Movimento->MovementMode != MOVE_Flying)
	{
		Movimento->SetMovementMode(MOVE_Flying);
	}
	if (!Colono->HasAuthority())
	{
		return;
	}
	// Il server decide quando arriva in cima o in fondo.
	const FVector Locale = Scala->GetActorTransform().InverseTransformPosition(Colono->GetActorLocation());
	const float VelocitaZ = Movimento ? static_cast<float>(Movimento->Velocity.Z) : 0.f;
	const int32 Fine = ValdorsoScale::Estremo(static_cast<float>(Locale.Z), Scala->Altezza, ScalaMezzaCapsula(Colono), VelocitaZ);
	if (Fine != 0)
	{
		Finisci(Fine);
		return;
	}
	// Troppo lontano dai pioli (spinto via da qualcosa): la lascia.
	if (FVector::Dist2D(Colono->GetActorLocation(), Scala->PuntoSullaScala(static_cast<float>(Locale.Z))) > 120.f)
	{
		Finisci(0);
	}
}

void UValdorsoScalataComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Sale())
	{
		ApplicaMovimento(false);
	}
	Super::EndPlay(EndPlayReason);
}

// ================================================================================================ la scala

AValdorsoScala::AValdorsoScala()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));

	// La zona invisibile per essere trovata con E: lungo tutta la scala, dal pavimento a sopra la botola.
	Zona = CreateDefaultSubobject<UBoxComponent>(TEXT("Zona"));
	Zona->SetupAttachment(RootComponent);
	Zona->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Zona->SetCollisionObjectType(ECC_WorldDynamic);
	Zona->SetCollisionResponseToAllChannels(ECR_Overlap);
	Zona->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Zona->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Zona->SetGenerateOverlapEvents(false);
	Zona->SetHiddenInGame(true);
	Zona->SetCanEverAffectNavigation(false);
	SistemaZona();
}

void AValdorsoScala::SistemaZona()
{
	Zona->SetBoxExtent(FVector(60.f, 50.f, (Altezza + 160.f) * 0.5f));
	Zona->SetRelativeLocation(FVector(45.f, 0.f, (Altezza + 160.f) * 0.5f));
}

void AValdorsoScala::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SistemaZona();
}

FVector AValdorsoScala::PuntoSullaScala(float Quota) const
{
	return GetActorTransform().TransformPosition(FVector(DistanzaDavanti, 0.f, Quota));
}

float AValdorsoScala::YawSalita() const
{
	// Chi sale guarda i pioli: al contrario della X della scala.
	return GetActorRotation().Yaw + 180.f;
}

bool AValdorsoScala::InCima(const APawn* Chi) const
{
	if (!Chi)
	{
		return false;
	}
	const FVector Locale = GetActorTransform().InverseTransformPosition(Chi->GetActorLocation());
	return Locale.Z > Altezza * 0.5f;
}

FText AValdorsoScala::TestoInterazione(const APawn* Chi) const
{
	if (!Chi)
	{
		return FText::GetEmpty();
	}
	if (const UValdorsoScalataComponent* Scalata = Chi->FindComponentByClass<UValdorsoScalataComponent>())
	{
		if (Scalata->Sale())
		{
			return Scalata->GetScala() == this ? LOCTEXT("Lascia", "Lascia la scala") : FText::GetEmpty();
		}
	}
	else
	{
		return FText::GetEmpty();
	}
	return InCima(Chi) ? LOCTEXT("Scendi", "Scendi la scala") : LOCTEXT("Sali", "Sali la scala");
}

void AValdorsoScala::Interagisci(APawn* Chi)
{
	ACharacter* Colono = Cast<ACharacter>(Chi);
	UValdorsoScalataComponent* Scalata = Colono ? Colono->FindComponentByClass<UValdorsoScalataComponent>() : nullptr;
	if (!Scalata || !HasAuthority())
	{
		return;
	}
	if (Scalata->Sale())
	{
		if (Scalata->GetScala() == this)
		{
			Scalata->Finisci(0);
		}
		return;
	}
	if (ScalaColonoOccupato(Chi))
	{
		return;
	}
	// Il server controlla che stia davvero vicino al fondo o alla cima (non a metà, fuori dalla torre).
	const FVector Locale = GetActorTransform().InverseTransformPosition(Chi->GetActorLocation());
	const bool bCima = InCima(Chi);
	const float Mezza = ScalaMezzaCapsula(Colono);
	const float Piedi = static_cast<float>(Locale.Z) - Mezza;
	if ((!bCima && FMath::Abs(Piedi) > 80.f) || (bCima && FMath::Abs(Piedi - Altezza) > 80.f))
	{
		return;
	}
	Scalata->Inizia(this, bCima);
}

FVector AValdorsoScala::PuntoInterazione() const
{
	return PuntoSullaScala(100.f);
}

FVector AValdorsoScala::PuntoInterazionePer(const APawn* Chi) const
{
	if (!Chi)
	{
		return PuntoInterazione();
	}
	const FVector Locale = GetActorTransform().InverseTransformPosition(Chi->GetActorLocation());
	return PuntoSullaScala(ValdorsoScale::QuotaInterazione(static_cast<float>(Locale.Z), Altezza));
}

#undef LOCTEXT_NAMESPACE
