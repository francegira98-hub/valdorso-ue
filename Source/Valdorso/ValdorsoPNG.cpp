// Valdorso - I PNG della valle (vedi il .h). Scritto da Claude il 07/11/2026.
// Unity build: le cose del namespace anonimo cominciano con Png, così non si scontrano con gli altri file.

#include "ValdorsoPNG.h"
#include "Valdorso.h"
#include "ValdorsoAspetto.h"
#include "ValdorsoAspettoComponent.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ValdorsoPNG"

namespace
{
	/** Mezza altezza e raggio della capsula (cm): come un colono. L'origine dell'attore è al centro della capsula. */
	constexpr float PngMezzaAltezza = 92.f;
	constexpr float PngRaggio = 36.f;

	/** Dopo quanti secondi senza conversazioni il PNG torna a guardare dove guardava. */
	constexpr float PngSecondiRitorno = 4.f;

	/** Il nome con cui il colono entra nella memoria dei dialoghi: il nome del personaggio (o dell'attore). */
	FString PngNomeDi(const APawn* Chi)
	{
		if (!Chi)
		{
			return FString();
		}
		if (const APlayerState* Stato = Chi->GetPlayerState())
		{
			return Stato->GetPlayerName();
		}
		return Chi->GetName();
	}
}

AValdorsoPNG::AValdorsoPNG()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(5.f);
	// Non si uccide: niente danni (e niente statistiche: i pugni non trovano un Ability System).
	SetCanBeDamaged(false);

	Capsula = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsula"));
	Capsula->InitCapsuleSize(PngRaggio, PngMezzaAltezza);
	// Come un colono per le collisioni (i coloni ci sbattono, E la trova), ma ferma: non simula la fisica, quindi non
	// si spinge. La telecamera ci passa attraverso, e sulla testa non si sale.
	Capsula->SetCollisionProfileName(TEXT("Pawn"));
	Capsula->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Capsula->SetSimulatePhysics(false);
	Capsula->CanCharacterStepUpOn = ECB_No;
	Capsula->SetCanEverAffectNavigation(false);
	RootComponent = Capsula;

	// Il manichino di Unreal guarda verso +Y: lo si gira di -90 perché guardi verso la X dell'attore; i piedi in basso.
	Corpo = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Corpo"));
	Corpo->SetupAttachment(Capsula);
	Corpo->SetRelativeLocation(FVector(0.f, 0.f, -PngMezzaAltezza));
	Corpo->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Corpo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Corpo->SetCanEverAffectNavigation(false);
	Corpo->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// Gli stessi del palco del Registro (ValdorsoPalcoRegistro.cpp): il manichino e il suo respiro da fermo.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Quinn(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Fermo(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	ManichinoUomo = Manny.Object;
	ManichinoDonna = Quinn.Object;
	Riposo = Fermo.Object;
	if (ManichinoUomo)
	{
		Corpo->SetSkeletalMesh(ManichinoUomo);
	}

	Aspetto = CreateDefaultSubobject<UValdorsoAspettoComponent>(TEXT("Aspetto"));
}

void AValdorsoPNG::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoPNG, Dialogo);
	DOREPLIFETIME(AValdorsoPNG, Sguardo);
}

void AValdorsoPNG::BeginPlay()
{
	// Prima di Super::BeginPlay (che fa partire i componenti): il MetaHuman va sul manichino del PNG, con il volto
	// deciso dal server. Così il corpo si fa una volta sola.
	const FString Sesso = bDonna ? FString(TEXT("donna")) : FString(TEXT("uomo"));
	if (Corpo)
	{
		USkeletalMesh* Manichino = bDonna ? ManichinoDonna.Get() : ManichinoUomo.Get();
		if (Manichino)
		{
			Corpo->SetSkeletalMesh(Manichino);
		}
		if (Riposo)
		{
			Corpo->PlayAnimation(Riposo, true);
		}
		else
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] PNG %s: manca l'animazione MM_Idle, resta fermo in posa."), *GetName());
		}
	}
	if (Aspetto)
	{
		Aspetto->UsaSupporto(Corpo, nullptr);
		if (HasAuthority())
		{
			FValdorsoAspetto Volto = ValdorsoAspetto::Visibile(FValdorsoAspetto(), Sesso, Dialogo.ToString());
			if (Testa >= 0)
			{
				ValdorsoAspetto::Imposta(Volto, TEXT("Testa"), static_cast<uint8>(FMath::Clamp(Testa, 0, 255)));
			}
			Aspetto->ImpostaAspetto(Volto, Sesso);
		}
	}

	Super::BeginPlay();

	SguardoIniziale = static_cast<float>(GetActorRotation().Yaw);
	if (HasAuthority())
	{
		Sguardo = SguardoIniziale;
		bSguardoValido = true;
		if (!DatiDialogo())
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] PNG %s: il dialogo \"%s\" non esiste (ValdorsoDialoghiTesti.inl)."),
				*GetName(), *Dialogo.ToString());
		}
	}
	else if (bSguardoValido)
	{
		SetActorTickEnabled(true);
	}
}

void AValdorsoPNG::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerRitorno);
	}
	Conversazioni.Reset();
	Super::EndPlay(EndPlayReason);
}

const FValdorsoDialogo* AValdorsoPNG::DatiDialogo() const
{
	return ValdorsoDialoghi::Trova(Dialogo);
}

FText AValdorsoPNG::TestoInterazione(const APawn* Chi) const
{
	const FValdorsoDialogo* Dati = DatiDialogo();
	return Dati ? FText::Format(LOCTEXT("Parla", "Parla con {0}"), FText::FromString(Dati->NomeBreve)) : FText::GetEmpty();
}

FVector AValdorsoPNG::PuntoInterazione() const
{
	// Il centro della capsula: all'altezza del centro del colono, così la distanza si misura in piano.
	return GetActorLocation();
}

void AValdorsoPNG::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	const FValdorsoDialogo* Dati = DatiDialogo();
	APlayerController* Controllore = Cast<APlayerController>(Chi->GetController());
	if (!Dati || !Controllore)
	{
		return;
	}
	// Via le conversazioni di chi se n'è andato dal server.
	for (auto It = Conversazioni.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	const FString Nome = PngNomeDi(Chi);
	const bool bGiaParlato = ValdorsoDialoghi::GiaParlato(Dati->Id, Nome);
	ValdorsoDialoghi::SegnaParlato(Dati->Id, Nome);

	FConversazione Nuova;
	Nuova.Nodo = ValdorsoDialoghi::NodoIniziale(*Dati, bGiaParlato);
	Nuova.Passo = ProssimoPasso++;
	Conversazioni.Add(TWeakObjectPtr<APlayerController>(Controllore), Nuova);

	ValdorsoRegistroEventi::Annota(TEXT("Dialogo"), Chi, this, GetActorLocation(),
		FString::Printf(TEXT("parla con %s (%s)"), *Dati->NomeBreve, bGiaParlato ? TEXT("di nuovo") : TEXT("la prima volta")));

	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerRitorno);
	}
	GuardaVerso(Chi);
	Manda(Controllore, ValdorsoDialoghi::Battuta(*Dati, Nuova.Nodo, Nuova.Passo));
}

void AValdorsoPNG::Scegli(APlayerController* Chi, int32 Passo, int32 Scelta)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	FConversazione* Aperta = Conversazioni.Find(TWeakObjectPtr<APlayerController>(Chi));
	// Una risposta a una battuta vecchia (doppio clic, ritardo della rete) o senza conversazione: non vale.
	if (!Aperta || Aperta->Passo != Passo)
	{
		return;
	}
	APawn* Colono = Chi->GetPawn();
	const FValdorsoDialogo* Dati = DatiDialogo();
	if (!Colono || !Dati)
	{
		Chiudi(Chi, true, TEXT("senza colono"));
		return;
	}
	if (Scelta == INDEX_NONE)
	{
		Chiudi(Chi, false, TEXT("se ne va"));
		return;
	}
	// Il server non si fida: il colono deve essere ancora vicino (e vedere il PNG).
	if (FVector::Dist(Colono->GetActorLocation(), PuntoInterazionePer(Colono)) > RaggioDialogo)
	{
		Chiudi(Chi, true, TEXT("si è allontanato"));
		return;
	}

	const FName Prima = Aperta->Nodo;
	const FValdorsoNodoDialogo* Nodo = Dati->Trova(Prima);
	FName Prossimo;
	FName Evento;
	if (!Nodo || !ValdorsoDialoghi::Avanza(*Dati, Prima, Scelta, Prossimo, Evento))
	{
		return;  // una risposta che non c'è: si ignora (il PC mostra ancora la stessa battuta)
	}
	const FString Detta = Nodo->Scelte[Scelta].Testo;
	// (Per esempio offrire una moneta a una guardia: "Corruzione". Un giorno potrà essere un crimine.)
	if (!Evento.IsNone())
	{
		ValdorsoRegistroEventi::Annota(Evento.ToString(), Colono, this, GetActorLocation(),
			FString::Printf(TEXT("a %s: %s"), *Dati->NomeBreve, *Detta.Left(120)));
	}
	ValdorsoRegistroEventi::Annota(TEXT("Dialogo"), Colono, this, GetActorLocation(),
		FString::Printf(TEXT("%s: %s -> %s (%s)"), *Dati->NomeBreve, *Prima.ToString(),
			Prossimo.IsNone() ? TEXT("fine") : *Prossimo.ToString(), *Detta.Left(80)));

	if (Prossimo.IsNone())
	{
		Chiudi(Chi, true, FString());
		return;
	}
	Aperta->Nodo = Prossimo;
	Aperta->Passo = ProssimoPasso++;
	GuardaVerso(Colono);
	Manda(Chi, ValdorsoDialoghi::Battuta(*Dati, Aperta->Nodo, Aperta->Passo));
}

void AValdorsoPNG::Manda(APlayerController* Chi, const FValdorsoBattuta& Battuta)
{
	if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
	{
		Giocatore->ClientMostraDialogo(this, Battuta);
	}
	// L'ultima battuta: sul server la conversazione è finita (il PC la legge e si congeda da solo).
	if (Battuta.bFine || Battuta.bChiudi)
	{
		Chiudi(Chi, false, Battuta.bFine ? TEXT("fine del dialogo") : TEXT("chiuso"));
	}
}

void AValdorsoPNG::Chiudi(APlayerController* Chi, bool bAvvisa, const FString& Come)
{
	const TWeakObjectPtr<APlayerController> Chiave(Chi);
	const FConversazione* Aperta = Conversazioni.Find(Chiave);
	if (!Aperta)
	{
		return;
	}
	const int32 Passo = Aperta->Passo;
	Conversazioni.Remove(Chiave);
	if (APawn* Colono = Chi ? Chi->GetPawn() : nullptr; Colono && !Come.IsEmpty())
	{
		ValdorsoRegistroEventi::Annota(TEXT("Dialogo"), Colono, this, GetActorLocation(), Come);
	}
	if (bAvvisa)
	{
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
		{
			FValdorsoBattuta Fine;
			Fine.Passo = Passo;
			Fine.bChiudi = true;
			Giocatore->ClientMostraDialogo(this, Fine);
		}
	}
	ForseTornaAlPosto();
}

// --- Lo sguardo --------------------------------------------------------------------------------------------------

void AValdorsoPNG::GuardaVerso(const AActor* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	const FVector Verso = Chi->GetActorLocation() - GetActorLocation();
	if (Verso.SizeSquared2D() < 1.f)
	{
		return;
	}
	Sguardo = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Verso.Y, Verso.X)));
	bSguardoValido = true;
	ForceNetUpdate();
	OnRep_Sguardo();
}

void AValdorsoPNG::ForseTornaAlPosto()
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo || Conversazioni.Num() > 0)
	{
		return;
	}
	Mondo->GetTimerManager().SetTimer(TimerRitorno, this, &AValdorsoPNG::TornaAlPosto, PngSecondiRitorno, false);
}

void AValdorsoPNG::TornaAlPosto()
{
	if (!HasAuthority() || Conversazioni.Num() > 0)
	{
		return;
	}
	Sguardo = SguardoIniziale;
	ForceNetUpdate();
	OnRep_Sguardo();
}

void AValdorsoPNG::OnRep_Sguardo()
{
	bSguardoValido = true;
	// Si gira nel Tick finché arriva (solo dopo BeginPlay: prima il Tick non parte comunque).
	SetActorTickEnabled(true);
}

void AValdorsoPNG::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bSguardoValido)
	{
		SetActorTickEnabled(false);
		return;
	}
	const float Adesso = static_cast<float>(GetActorRotation().Yaw);
	const float Manca = FMath::FindDeltaAngleDegrees(Adesso, Sguardo);
	const float Passo = VelocitaGiro * DeltaSeconds;
	if (FMath::Abs(Manca) <= Passo)
	{
		SetActorRotation(FRotator(0.f, Sguardo, 0.f));
		SetActorTickEnabled(false);
		return;
	}
	SetActorRotation(FRotator(0.f, Adesso + FMath::Sign(Manca) * Passo, 0.f));
}

#undef LOCTEXT_NAMESPACE
