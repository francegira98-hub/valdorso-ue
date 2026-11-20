// Valdorso - La campanella della caserma. Vedi ValdorsoCampanella.h. Scritto da Claude il 20/11/2026.

#include "ValdorsoCampanella.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoMilizia.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "ValdorsoCampanella"

AValdorsoCampanella::AValdorsoCampanella()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Modello = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Modello"));
	RootComponent = Modello;
	// il "suona" arriva a tutti (NetMulticast): l'attore deve viaggiare in rete
	bReplicates = true;
	Modello->SetMobility(EComponentMobility::Static);
	Modello->SetCollisionProfileName(TEXT("BlockAll"));
	// la mensola sporge sopra la testa: la telecamera ci passa
	Modello->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	Battaglio = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Battaglio"));
	Battaglio->SetupAttachment(Modello);
	Battaglio->SetMobility(EComponentMobility::Movable);
	// la corda si tocca con E, ma non ferma nessuno
	Battaglio->SetCollisionProfileName(TEXT("OverlapAll"));
	Battaglio->SetGenerateOverlapEvents(false);
}

FText AValdorsoCampanella::TestoInterazione(const APawn* Chi) const
{
	const APlayerController* Giocatore = Chi ? Cast<APlayerController>(Chi->GetController()) : nullptr;
	return ValdorsoMilizia::GradoDi(Giocatore) > 0
		? LOCTEXT("Suona", "Suona la campanella")
		: LOCTEXT("Guarda", "La campanella della caserma");
}

FVector AValdorsoCampanella::PuntoInterazione() const
{
	// la mano sulla corda: 45 cm dal muro (-Y del pezzo), 115 sotto la mensola
	// (senza la scala dell'attore: se il modello e' specchiato il punto resta davanti al muro)
	return GetActorLocation() + GetActorQuat().RotateVector(FVector(0.f, -45.f, -115.f));
}

void AValdorsoCampanella::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi->GetController());
	if (ValdorsoMilizia::GradoDi(Giocatore) == 0)
	{
		if (Giocatore)
		{
			Giocatore->ClientAvviso(LOCTEXT("NonGuardia", "La campanella della caserma: la suonano le Guardie di Valdorso."), 5.f, 0.f);
		}
		return;
	}
	const double Adesso = FPlatformTime::Seconds();
	if (Adesso - UltimaVolta < Secondi)
	{
		return;   // sta ancora suonando
	}
	UltimaVolta = Adesso;
	MulticastSuona();
	ValdorsoRegistroEventi::Annota(TEXT("Campanella"), Chi, this, GetActorLocation(), TEXT("suonata"));
}

void AValdorsoCampanella::MulticastSuona_Implementation()
{
	Dondola = 0.f;
	SetActorTickEnabled(true);
	UWorld* Mondo = GetWorld();
	if (!Mondo || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// il suono suo (S_Campanella, quando ci sara'), o il colpo della grata, piu' acuto
	auto Carica = [](const TCHAR* N) -> USoundBase*
	{
		const FString Percorso = FString::Printf(TEXT("/Game/Valle/Suoni/%s.%s"), N, N);
		return LoadObject<USoundBase>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
	};
	USoundBase* Suono = Carica(TEXT("S_Campanella"));
	const bool bRiserva = Suono == nullptr;
	if (bRiserva)
	{
		Suono = Carica(TEXT("S_Grata_Colpo"));
	}
	if (!Suono)
	{
		Suono = Carica(TEXT("S_Porta_Chiave"));
	}
	if (Suono)
	{
		ValdorsoCombattimento::Suona(Mondo, Suono, GetActorLocation(), 6000.f, 1.f, bRiserva ? 1.8f : 1.f);
	}
}

void AValdorsoCampanella::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Dondola < 0.f)
	{
		SetActorTickEnabled(false);
		return;
	}
	Dondola += DeltaSeconds;
	float Angolo = 0.f;
	if (Dondola >= Secondi)
	{
		Dondola = -1.f;
		SetActorTickEnabled(false);
	}
	else
	{
		// avanti e indietro, sempre meno
		Angolo = Gradi * FMath::Sin(Dondola * 9.f) * (1.f - Dondola / FMath::Max(Secondi, 0.1f));
	}
	if (Battaglio)
	{
		Battaglio->SetRelativeRotation(FRotator(0.f, 0.f, Angolo));
	}
}

#undef LOCTEXT_NAMESPACE
