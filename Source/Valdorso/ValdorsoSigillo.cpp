// Valdorso - Il sigillo della Corona sul passo (vedi il .h). Scritto da Claude il 06/11/2026.

#include "ValdorsoSigillo.h"
#include "Valdorso.h"
#include "ValdorsoCielo.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundAttenuation.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"

namespace ValdorsoSigillo
{
	FVector Spinta(const FTransform& Sigillo, const FVector& Punto, float Forza, float Su)
	{
		// Da che parte del velo sta il colono: davanti (X positiva, la valle) o dietro (il passo).
		const FVector Locale = Sigillo.InverseTransformPosition(Punto);
		const float Verso = Locale.X >= 0.f ? 1.f : -1.f;
		FVector Avanti = Sigillo.GetUnitAxis(EAxis::X);
		Avanti.Z = 0.f;
		Avanti = Avanti.GetSafeNormal();
		if (Avanti.IsNearlyZero())
		{
			Avanti = FVector::ForwardVector;
		}
		return Avanti * Verso * Forza + FVector(0.f, 0.f, Su);
	}

	float Luminosita(float Secondi, float Ora)
	{
		// Il battito: due colpi (il secondo più debole) e una pausa, ogni 1,6 s, come il frammento del Cuore.
		const float Giro = FMath::Fmod(FMath::Max(0.f, Secondi), 1.6f);
		const float Colpo1 = FMath::Exp(-FMath::Square((Giro - 0.10f) / 0.07f));
		const float Colpo2 = 0.6f * FMath::Exp(-FMath::Square((Giro - 0.42f) / 0.07f));
		// La notte: piena dalle 21 alle 4, sfuma tra le 19 e le 21 e tra le 4 e le 6.
		float Notte = 0.f;
		if (Ora >= 21.f || Ora < 4.f)
		{
			Notte = 1.f;
		}
		else if (Ora >= 19.f)
		{
			Notte = (Ora - 19.f) / 2.f;
		}
		else if (Ora < 6.f)
		{
			Notte = 1.f - (Ora - 4.f) / 2.f;
		}
		return (1.f + 1.4f * Notte) * (1.f + 0.35f * (Colpo1 + Colpo2));
	}
}

namespace
{
	void ApriOChiudiSigillo(bool bApri)
	{
		for (TObjectIterator<AValdorsoSigillo> Sigillo; Sigillo; ++Sigillo)
		{
			if (IsValid(*Sigillo) && Sigillo->HasAuthority() && Sigillo->GetWorld() && Sigillo->GetWorld()->IsGameWorld())
			{
				Sigillo->ImpostaAperto(bApri);
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Sigillo: %s"), bApri ? TEXT("aperto") : TEXT("chiuso"));
			}
		}
	}

	FAutoConsoleCommand ComandoSigilloApri(TEXT("Valdorso.Sigillo.Apri"), TEXT("Apre il sigillo della Corona sul passo (si passa)."),
		FConsoleCommandDelegate::CreateLambda([]() { ApriOChiudiSigillo(true); }));

	FAutoConsoleCommand ComandoSigilloChiudi(TEXT("Valdorso.Sigillo.Chiudi"), TEXT("Chiude il sigillo della Corona sul passo."),
		FConsoleCommandDelegate::CreateLambda([]() { ApriOChiudiSigillo(false); }));
}

AValdorsoSigillo::AValdorsoSigillo()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.03f;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(2.f);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));

	// Il velo: solo da vedere (niente collisione, niente ombre).
	Velo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Velo"));
	Velo->SetupAttachment(RootComponent);
	Velo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Velo->SetCastShadow(false);
	Velo->SetGenerateOverlapEvents(false);

	// Il muro invisibile: ferma solo i corpi (Pawn); telecamera, vista e colpi lo attraversano.
	Muro = CreateDefaultSubobject<UBoxComponent>(TEXT("Muro"));
	Muro->SetupAttachment(RootComponent);
	Muro->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Muro->SetCollisionObjectType(ECC_WorldStatic);
	Muro->SetCollisionResponseToAllChannels(ECR_Ignore);
	Muro->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Muro->SetGenerateOverlapEvents(false);
	Muro->SetCanEverAffectNavigation(false);

	// La soglia: un poco più spessa del muro, per sentire chi arriva e respingerlo prima che lo tocchi.
	Soglia = CreateDefaultSubobject<UBoxComponent>(TEXT("Soglia"));
	Soglia->SetupAttachment(RootComponent);
	Soglia->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Soglia->SetCollisionObjectType(ECC_WorldDynamic);
	Soglia->SetCollisionResponseToAllChannels(ECR_Ignore);
	Soglia->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Soglia->SetGenerateOverlapEvents(true);
	Soglia->SetCanEverAffectNavigation(false);

	// Il ronzio: si sente fino a circa 40 m.
	Ronzio = CreateDefaultSubobject<UAudioComponent>(TEXT("Ronzio"));
	Ronzio->SetupAttachment(RootComponent);
	Ronzio->bAutoActivate = true;
	Ronzio->bOverrideAttenuation = true;
	Ronzio->AttenuationOverrides.bAttenuate = true;
	Ronzio->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
	Ronzio->AttenuationOverrides.AttenuationShapeExtents = FVector(800.f, 0.f, 0.f);
	Ronzio->AttenuationOverrides.FalloffDistance = 3200.f;
	Ronzio->AttenuationOverrides.bSpatialize = true;

	SistemaMisure();
}

void AValdorsoSigillo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoSigillo, bAperto);
}

void AValdorsoSigillo::SistemaMisure()
{
	const float MezzaLarghezza = FMath::Max(50.f, Larghezza * 0.5f);
	const float MezzaAltezza = FMath::Max(50.f, Altezza * 0.5f);
	// Il muro e la soglia vanno oltre il velo che si vede: 10 m per parte dentro la roccia e 40 m più in alto, così
	// nessuno lo scavalca arrampicandosi sui fianchi della gola (il velo sfuma, la magia no).
	const float Sopra = 4000.f;
	const FVector Centro(0.f, CentroTraverso, Base + MezzaAltezza + Sopra * 0.5f);
	const FVector Mezzi(0.f, MezzaLarghezza + 1000.f, MezzaAltezza + Sopra * 0.5f);
	// Il muro sottile (60 cm); la soglia 90 cm in più per parte.
	Muro->SetRelativeLocation(Centro);
	Muro->SetBoxExtent(FVector(30.f, Mezzi.Y, Mezzi.Z));
	Soglia->SetRelativeLocation(Centro);
	Soglia->SetBoxExtent(FVector(120.f, Mezzi.Y, Mezzi.Z));
	Ronzio->SetRelativeLocation(FVector(0.f, CentroTraverso, 250.f));
}

void AValdorsoSigillo::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SistemaMisure();
}

void AValdorsoSigillo::BeginPlay()
{
	Super::BeginPlay();
	SistemaMisure();
	Soglia->OnComponentBeginOverlap.AddDynamic(this, &AValdorsoSigillo::QuandoTocca);
	GetWorldTimerManager().SetTimer(TimerSoglia, this, &AValdorsoSigillo::ControllaSoglia, 0.25f, true);

	if (GetNetMode() == NM_DedicatedServer)
	{
		// Il server dedicato non disegna e non suona niente: gli basta respingere.
		SetActorTickEnabled(false);
		Ronzio->Stop();
	}
	else if (Velo->GetMaterial(0))
	{
		MaterialeVelo = Velo->CreateDynamicMaterialInstance(0);
	}
	if (UWorld* Mondo = GetWorld())
	{
		for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
		{
			Cielo = *It;
			break;
		}
	}
	ProssimoTremito = GetWorld() ? GetWorld()->GetTimeSeconds() + FMath::FRandRange(30.f, 80.f) : 0.0;
	ApplicaAperto();
}

void AValdorsoSigillo::ImpostaAperto(bool bNuovo)
{
	if (!HasAuthority() || bAperto == bNuovo)
	{
		return;
	}
	bAperto = bNuovo;
	ValdorsoRegistroEventi::Annota(TEXT("Sigillo"), this, nullptr, GetActorLocation(), bAperto ? TEXT("aperto") : TEXT("chiuso"));
	ForceNetUpdate();
	ApplicaAperto();
}

void AValdorsoSigillo::OnRep_Aperto()
{
	ApplicaAperto();
}

void AValdorsoSigillo::ApplicaAperto()
{
	Muro->SetCollisionEnabled(bAperto ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	Soglia->SetCollisionEnabled(bAperto ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	Velo->SetVisibility(!bAperto);
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (bAperto)
		{
			Ronzio->Stop();
		}
		else if (!Ronzio->IsPlaying() && Ronzio->Sound)
		{
			Ronzio->Play();
		}
	}
}

void AValdorsoSigillo::QuandoTocca(UPrimitiveComponent* Componente, AActor* Altro, UPrimitiveComponent* AltroComponente,
	int32 Indice, bool bDaSpinta, const FHitResult& Colpo)
{
	Respingi(Cast<ACharacter>(Altro));
}

void AValdorsoSigillo::ControllaSoglia()
{
	if (bAperto)
	{
		return;
	}
	TArray<AActor*> Dentro;
	Soglia->GetOverlappingActors(Dentro, ACharacter::StaticClass());
	for (AActor* Attore : Dentro)
	{
		Respingi(Cast<ACharacter>(Attore));
	}
	// chi non c'è più esce dalla lista
	for (auto It = UltimaSpinta.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void AValdorsoSigillo::Respingi(ACharacter* Colono)
{
	UWorld* Mondo = GetWorld();
	if (bAperto || !Colono || !Mondo)
	{
		return;
	}
	// Solo il server (che decide) e il PC di chi viene respinto (perché la spinta si senta subito).
	const bool bServer = HasAuthority();
	if (!bServer && !Colono->IsLocallyControlled())
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	double& Ultima = UltimaSpinta.FindOrAdd(Colono, -1000.0);
	if (Adesso - Ultima < 0.6)
	{
		return;
	}
	Ultima = Adesso;

	const FVector Spinta = ValdorsoSigillo::Spinta(GetActorTransform(), Colono->GetActorLocation(), ForzaSpinta, SpintaSu);
	Colono->LaunchCharacter(Spinta, true, true);
	if (bServer)
	{
		ValdorsoRegistroEventi::Annota(TEXT("Sigillo"), Colono, this, Colono->GetActorLocation(), TEXT("respinto dal sigillo della Corona"));
		if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(Colono->GetController()))
		{
			Controllore->ClientAvviso(NSLOCTEXT("ValdorsoSigillo", "Respinto", "Il sigillo della Corona ti respinge."), 3.f, SecondiStordimento);
		}
	}
}

void AValdorsoSigillo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAperto)
	{
		return;
	}
	const double Adesso = GetWorld()->GetTimeSeconds();
	// Il cielo nasce dalla modalità di gioco dopo gli attori del livello (e sui PC arriva dalla rete): si cerca finché c'è.
	if (!Cielo.IsValid() && Adesso >= ProssimaRicercaCielo)
	{
		ProssimaRicercaCielo = Adesso + 1.0;
		for (TActorIterator<AValdorsoCielo> It(GetWorld()); It; ++It)
		{
			Cielo = *It;
			break;
		}
	}
	const float Ora = Cielo.IsValid() ? Cielo->Ora() : 12.f;
	float Luce = ValdorsoSigillo::Luminosita(static_cast<float>(Adesso), Ora);

	// Di notte, ogni tanto, il sigillo tremola per un istante (i segni del risveglio: il battito si indebolisce).
	const bool bNotte = Ora >= 21.f || Ora < 4.f;
	if (bNotte && Adesso >= ProssimoTremito)
	{
		FineTremito = Adesso + FMath::FRandRange(0.25f, 0.6f);
		ProssimoTremito = Adesso + FMath::FRandRange(40.f, 110.f);
	}
	if (Adesso < FineTremito)
	{
		Luce *= 0.15f + 0.35f * FMath::Abs(FMath::Sin(static_cast<float>(Adesso) * 47.f));
	}
	if (MaterialeVelo)
	{
		MaterialeVelo->SetScalarParameterValue(TEXT("Forza"), Luce);
		MaterialeVelo->SetVectorParameterValue(TEXT("Colore"), Colore);
	}
	Ronzio->SetVolumeMultiplier(FMath::Clamp(0.45f + 0.25f * Luce, 0.2f, 1.3f));
}
