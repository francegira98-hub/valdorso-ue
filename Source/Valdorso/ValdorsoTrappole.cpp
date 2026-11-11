// Valdorso - Le trappole dei nani, le porte segrete e il tesoro che chiama le ossa (11/11/2026). Scritto da Claude.
// Vedi ValdorsoTrappole.h.

#include "ValdorsoTrappole.h"
#include "Valdorso.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoScheletri.h"
#include "ValdorsoTaccuino.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoTrappole"

namespace
{
	const TCHAR* TrappoleCartella = TEXT("/Game/Valle/Grotta/Pezzi/");

	UStaticMesh* TrappoleModello(const TCHAR* Nome)
	{
		const FString Percorso = FString::Printf(TEXT("%s%s.%s"), TrappoleCartella, Nome, Nome);
		if (UStaticMesh* Trovato = LoadObject<UStaticMesh>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			return Trovato;
		}
		const FString Dentro = FString::Printf(TEXT("%s%s/StaticMeshes/%s.%s"), TrappoleCartella, Nome, Nome, Nome);
		return LoadObject<UStaticMesh>(nullptr, *Dentro, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	void TrappoleSuona(const AActor* Dove, const TCHAR* Nome, float Lontano, float Volume, float Tono = 1.f)
	{
		if (!Dove || Dove->GetNetMode() == NM_DedicatedServer)
		{
			return;
		}
		ValdorsoCombattimento::Suona(Dove->GetWorld(), ValdorsoCombattimento::SuonoPericoli(Nome), Dove->GetActorLocation(), Lontano, Volume, Tono);
	}

	/** Una presa invisibile per la E (come la zona dei letti). */
	void TrappolePresa(USphereComponent* Presa, float Raggio)
	{
		Presa->InitSphereRadius(Raggio);
		Presa->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Presa->SetCollisionObjectType(ECC_WorldDynamic);
		Presa->SetCollisionResponseToAllChannels(ECR_Overlap);
		Presa->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Presa->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Presa->SetGenerateOverlapEvents(false);
		Presa->SetHiddenInGame(true);
	}

	/** Solo sul server: l'ultima volta che una trappola ha preso ogni colono (per non annotarlo dieci volte). */
	TMap<TWeakObjectPtr<APawn>, double>& TrappoleUltimi()
	{
		static TMap<TWeakObjectPtr<APawn>, double> Ultimi;
		return Ultimi;
	}

	float TrappoleLiscio(float X)
	{
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(X, 0.f, 1.f));
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoTrappoleMostra(
		TEXT("Valdorso.Trappole.Mostra"),
		TEXT("Mostra le zone delle trappole (in rosso, 20 secondi) su questo PC: per le prove."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Mondo)
		{
			if (!Mondo)
			{
				return;
			}
			for (TActorIterator<AValdorsoTrappola> It(Mondo); It; ++It)
			{
				It->Mostra(20.f);
			}
		}));
}

namespace ValdorsoTrappole
{
	float AngoloPendolo(double Secondi, float Periodo, float Fase, float Ampiezza)
	{
		const double P = FMath::Max(0.5, static_cast<double>(Periodo));
		return Ampiezza * static_cast<float>(FMath::Sin(2.0 * PI * (Secondi / P + static_cast<double>(Fase))));
	}

	float LanceFuori(double DalloScatto, float Ritardo)
	{
		const double Dopo = DalloScatto - static_cast<double>(Ritardo);
		if (Dopo < 0.0)
		{
			return 0.f;
		}
		if (Dopo < 0.08)
		{
			return static_cast<float>(Dopo / 0.08);
		}
		if (Dopo < 1.6)
		{
			return 1.f;
		}
		if (Dopo < 2.6)
		{
			return static_cast<float>(1.0 - (Dopo - 1.6));
		}
		return 0.f;
	}

	bool BotolaAperta(double DalloScatto, float Ritardo, float Ripristino)
	{
		return DalloScatto >= static_cast<double>(Ritardo) && DalloScatto < static_cast<double>(Ritardo + Ripristino);
	}

	double Orologio(const UObject* Contesto)
	{
		const UWorld* Mondo = Contesto ? Contesto->GetWorld() : nullptr;
		if (!Mondo)
		{
			return 0.0;
		}
		const AGameStateBase* Stato = Mondo->GetGameState();
		return Stato ? Stato->GetServerWorldTimeSeconds() : Mondo->GetTimeSeconds();
	}

	bool Uccidi(AActor* Trappola, APawn* Chi, const TCHAR* Come)
	{
		UValdorsoPericoliComponent* Pericoli = Chi ? UValdorsoPericoliComponent::Di(Chi) : nullptr;
		if (!Trappola || !Pericoli || !Trappola->HasAuthority() || Pericoli->Caduto())
		{
			return false;
		}
		const double Ora = Trappola->GetWorld() ? Trappola->GetWorld()->GetTimeSeconds() : 0.0;
		const TWeakObjectPtr<APawn> ChiaveColono(Chi);
		if (const double* Prima = TrappoleUltimi().Find(ChiaveColono); Prima && Ora - *Prima < 4.0)
		{
			return false;
		}
		for (auto It = TrappoleUltimi().CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent();
			}
		}
		TrappoleUltimi().Add(ChiaveColono, Ora);
		Pericoli->Ferisci(DannoMortale);
		ValdorsoRegistroEventi::Annota(TEXT("Trappola"), Chi, Trappola, Chi->GetActorLocation(), Come);
		return true;
	}
}

// ----------------------------------------------------------------------------------------------- la trappola

AValdorsoTrappola::AValdorsoTrappola()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(2.f);

	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;
	Zona = CreateDefaultSubobject<UBoxComponent>(TEXT("Zona"));
	Zona->SetupAttachment(Radice);
	Zona->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Zona->SetCollisionObjectType(ECC_WorldDynamic);
	Zona->SetCollisionResponseToAllChannels(ECR_Ignore);
	Zona->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Zona->SetGenerateOverlapEvents(true);
	Zona->SetHiddenInGame(true);
	Perno = CreateDefaultSubobject<USceneComponent>(TEXT("Perno"));
	Perno->SetupAttachment(Radice);
	Pezzo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pezzo"));
	Pezzo->SetupAttachment(Perno);
	Pezzo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pezzo->SetCanEverAffectNavigation(false);
}

void AValdorsoTrappola::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Zona->SetBoxExtent(Misura);
	Zona->SetRelativeLocation(FVector(0.f, 0.f, Misura.Z));
	if (Tipo == EValdorsoTrappola::Pendolo)
	{
		// appeso in alto: il perno sta un braccio sopra il pavimento, la lama in fondo al braccio (sfiora il pavimento)
		Perno->SetRelativeLocation(FVector(0.f, 0.f, Braccio + 60.f));
		Pezzo->SetRelativeLocation(FVector(0.f, 0.f, -Braccio));
		Zona->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		Perno->SetRelativeLocation(FVector::ZeroVector);
		Zona->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Pezzo->SetRelativeLocation(FVector(0.f, 0.f, Tipo == EValdorsoTrappola::Lance ? -110.f : 0.f));
	}
	// la botola e' un pezzo di pavimento vero: ci si cammina sopra finche' non cede
	Pezzo->SetCollisionProfileName(Tipo == EValdorsoTrappola::Botola ? TEXT("BlockAll") : TEXT("NoCollision"));
}

void AValdorsoTrappola::BeginPlay()
{
	Super::BeginPlay();
	// (anche sul server: la botola deve reggere i coloni anche li')
	const TCHAR* Nome = Tipo == EValdorsoTrappola::Lance ? TEXT("SM_Trappola_Lance")
		: Tipo == EValdorsoTrappola::Botola ? TEXT("SM_Trappola_Botola")
		: Tipo == EValdorsoTrappola::Pendolo ? TEXT("SM_Trappola_Lama") : TEXT("SM_Trappola_Punte");
	Pezzo->SetStaticMesh(TrappoleModello(Nome));
	if (HasAuthority())
	{
		Zona->OnComponentBeginOverlap.AddDynamic(this, &AValdorsoTrappola::Entrato);
	}
	if (Tipo == EValdorsoTrappola::Fondo)
	{
		SetActorTickEnabled(false);
	}
}

void AValdorsoTrappola::Entrato(UPrimitiveComponent* Mia, AActor* Altro, UPrimitiveComponent* SuaParte, int32 Indice, bool bSpinta,
	const FHitResult& Urto)
{
	APawn* Chi = Cast<APawn>(Altro);
	if (!HasAuthority() || !Chi || !UValdorsoPericoliComponent::Di(Chi))
	{
		return;
	}
	if (Tipo == EValdorsoTrappola::Fondo)
	{
		ValdorsoTrappole::Uccidi(this, Chi, TEXT("il fondo del pozzo"));
		return;
	}
	if (Tipo == EValdorsoTrappola::Pendolo)
	{
		return;
	}
	const double Ora = ValdorsoTrappole::Orologio(this);
	if (Ora - Scattata < static_cast<double>(Ritardo + Ripristino))
	{
		return;
	}
	Scattata = Ora;
	ForceNetUpdate();
	MulticastScatta(Ora);
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().SetTimer(TimerColpo, this, &AValdorsoTrappola::Colpisci, FMath::Max(0.01f, Ritardo), false);
	}
}

void AValdorsoTrappola::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoTrappola, Scattata);
}

void AValdorsoTrappola::MulticastScatta_Implementation(double Quando)
{
	Scattata = Quando;
	// lo scatto: un clic sotto i piedi (chi lo sente ha mezzo secondo)
	TrappoleSuona(this, TEXT("S_Trappola_Scatto"), 1500.f, 0.9f);
}

void AValdorsoTrappola::Colpisci()
{
	if (!HasAuthority() || Tipo != EValdorsoTrappola::Lance)
	{
		return;
	}
	TArray<AActor*> Dentro;
	Zona->GetOverlappingActors(Dentro, APawn::StaticClass());
	for (AActor* Attore : Dentro)
	{
		ValdorsoTrappole::Uccidi(this, Cast<APawn>(Attore), TEXT("le lance dei nani"));
	}
}

FVector AValdorsoTrappola::PuntaPendolo(float Angolo, float Lungo) const
{
	const FVector Relativo = Perno->GetRelativeLocation() + FRotator(0.f, 0.f, Angolo).RotateVector(FVector(0.f, 0.f, -Lungo));
	return GetActorTransform().TransformPosition(Relativo);
}

void AValdorsoTrappola::PendoloColpisce()
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	// la lama (la mezzaluna larga quasi due metri in fondo al braccio): una scatola girata come lei
	const float Angolo = ValdorsoTrappole::AngoloPendolo(ValdorsoTrappole::Orologio(this), Periodo, Fase, Ampiezza);
	const FQuat Giro = GetActorQuat() * FQuat(FRotator(0.f, 0.f, Angolo));
	const FVector Lama = PuntaPendolo(Angolo, Braccio + 30.f);
	TArray<FOverlapResult> Trovati;
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoPendolo), false, this);
	Mondo->OverlapMultiByObjectType(Trovati, Lama, Giro, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(FVector(20.f, 95.f, 30.f)),
		Parametri);
	for (const FOverlapResult& Trovato : Trovati)
	{
		ValdorsoTrappole::Uccidi(this, Cast<APawn>(Trovato.GetActor()), TEXT("la lama del pendolo"));
	}
}

void AValdorsoTrappola::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Ora = ValdorsoTrappole::Orologio(this);
	switch (Tipo)
	{
	case EValdorsoTrappola::Lance:
	{
		const float Fuori = ValdorsoTrappole::LanceFuori(Ora - Scattata, Ritardo);
		Pezzo->SetRelativeLocation(FVector(0.f, 0.f, -110.f * (1.f - Fuori)));
		break;
	}
	case EValdorsoTrappola::Botola:
	{
		const bool bGiu = ValdorsoTrappole::BotolaAperta(Ora - Scattata, Ritardo, Ripristino);
		if (bGiu != bBotolaGiu)
		{
			bBotolaGiu = bGiu;
			Pezzo->SetVisibility(!bGiu);
			Pezzo->SetCollisionEnabled(bGiu ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
			if (bGiu)
			{
				TrappoleSuona(this, TEXT("S_Frana"), 2000.f, 0.5f, 1.4f);
			}
		}
		break;
	}
	case EValdorsoTrappola::Pendolo:
	{
		const float Angolo = ValdorsoTrappole::AngoloPendolo(Ora, Periodo, Fase, Ampiezza);
		Perno->SetRelativeRotation(FRotator(0.f, 0.f, Angolo));
		if (HasAuthority())
		{
			PendoloColpisce();
		}
		break;
	}
	default:
		break;
	}
#if ENABLE_DRAW_DEBUG
	if (MostraFino > FPlatformTime::Seconds() && GetWorld())
	{
		DrawDebugBox(GetWorld(), Zona->GetComponentLocation(), Zona->GetScaledBoxExtent(), Zona->GetComponentQuat(), FColor::Red, false, -1.f, 0, 3.f);
	}
#endif
}

void AValdorsoTrappola::Mostra(float Secondi)
{
	MostraFino = FPlatformTime::Seconds() + Secondi;
}

// ----------------------------------------------------------------------------------------------- la porta segreta

AValdorsoPortaSegreta::AValdorsoPortaSegreta()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;
	Lastra = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lastra"));
	Lastra->SetupAttachment(Radice);
	Lastra->SetCollisionProfileName(TEXT("BlockAll"));
	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	Presa->SetupAttachment(Radice);
	Presa->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	TrappolePresa(Presa, 110.f);
}

void AValdorsoPortaSegreta::BeginPlay()
{
	Super::BeginPlay();
	if (!Lastra->GetStaticMesh())
	{
		Lastra->SetStaticMesh(TrappoleModello(TEXT("SM_GrottaNord_PortaSegreta")));
	}
}

void AValdorsoPortaSegreta::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoPortaSegreta, ApertaAlle);
	DOREPLIFETIME(AValdorsoPortaSegreta, ChiusaAlle);
}

bool AValdorsoPortaSegreta::Aperta() const
{
	return ApertaAlle >= 0.0 && ChiusaAlle < ApertaAlle;
}

bool AValdorsoPortaSegreta::DaFuori(const APawn* Chi) const
{
	return Chi && FVector::DotProduct(GetActorForwardVector(), Chi->GetActorLocation() - GetActorLocation()) > 0.f;
}

FText AValdorsoPortaSegreta::TestoInterazione(const APawn* Chi) const
{
	const float Limite = 170.f + (HasAuthority() && GetNetMode() != NM_Standalone ? 60.f : 0.f);
	if (!Chi || Aperta() || FVector::Dist2D(Chi->GetActorLocation(), GetActorLocation()) > Limite)
	{
		return FText::GetEmpty();
	}
	// mentre risale non si tocca (la lastra salterebbe)
	if (ChiusaAlle >= 0.0 && ValdorsoTrappole::Orologio(this) - ChiusaAlle < 3.0)
	{
		return FText::GetEmpty();
	}
	if (!DaFuori(Chi))
	{
		return LOCTEXT("SpingiDentro", "Spingi la pietra");
	}
	if (!IscrizioneRichiesta.IsEmpty())
	{
		// da fuori la trova solo chi sa cosa cercare (l'iscrizione nel taccuino)
		const UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Cast<APlayerController>(Chi->GetController()));
		if (!Taccuino || !Taccuino->TitoliIscrizioni().Contains(IscrizioneRichiesta))
		{
			return FText::GetEmpty();
		}
	}
	return FText::FromString(Testo);
}

void AValdorsoPortaSegreta::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || TestoInterazione(Chi).IsEmpty())
	{
		return;
	}
	ApertaAlle = ValdorsoTrappole::Orologio(this);
	ChiusaAlle = -1.0;
	ForceNetUpdate();
	ValdorsoRegistroEventi::Annota(TEXT("PortaSegreta"), Chi, this, GetActorLocation(), Testo);
}

void AValdorsoPortaSegreta::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Ora = ValdorsoTrappole::Orologio(this);
	float Giu = 0.f;
	if (ApertaAlle >= 0.0)
	{
		Giu = ChiusaAlle > ApertaAlle ? 1.f - TrappoleLiscio(static_cast<float>((Ora - ChiusaAlle) / 3.0))
			: TrappoleLiscio(static_cast<float>((Ora - ApertaAlle) / 3.0));
	}
	Lastra->SetRelativeLocation(FVector(0.f, 0.f, -Discesa * Giu));
	// il rumore della pietra che scende (una volta per apertura)
	const bool bMuove = ApertaAlle >= 0.0 && Ora - ApertaAlle < 3.0 && ChiusaAlle < ApertaAlle;
	if (bMuove && !bSuonata)
	{
		bSuonata = true;
		TrappoleSuona(this, TEXT("S_Frana"), 2500.f, 0.45f, 0.7f);
	}
	else if (!bMuove)
	{
		bSuonata = false;
	}
	// sul server: dopo i suoi minuti risale, se nessuno sta sulla soglia
	if (HasAuthority() && Aperta() && Ora - ApertaAlle > static_cast<double>(MinutiAperta) * 60.0)
	{
		bool bQualcuno = false;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			const APawn* Corpo = It->Get() ? It->Get()->GetPawn() : nullptr;
			if (Corpo && FVector::Dist(Corpo->GetActorLocation(), GetActorLocation()) < 260.f)
			{
				bQualcuno = true;
				break;
			}
		}
		if (!bQualcuno)
		{
			ChiusaAlle = Ora;
			ForceNetUpdate();
		}
	}
}

// ----------------------------------------------------------------------------------------------- il tesoro che chiama le ossa

AValdorsoTesoroAllarme::AValdorsoTesoroAllarme()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;
	Grata = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grata"));
	Grata->SetupAttachment(Radice);
	Grata->SetCollisionProfileName(TEXT("BlockAll"));
}

void AValdorsoTesoroAllarme::BeginPlay()
{
	Super::BeginPlay();
	if (!Grata->GetStaticMesh())
	{
		Grata->SetStaticMesh(TrappoleModello(TEXT("SM_GrottaNord_Grata")));
	}
	Grata->SetRelativeRotation(FRotator(0.f, GrataGradi, 0.f));
	Giu = bChiusa ? 1.f : 0.f;
	Grata->SetRelativeLocation(GrataDove - FVector(0.f, 0.f, 430.f * (1.f - Giu)));
}

void AValdorsoTesoroAllarme::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoTesoroAllarme, bChiusa);
}

void AValdorsoTesoroAllarme::OnRep_Chiusa()
{
	// la grata la muove Tick (giu' di colpo, su piano)
}

void AValdorsoTesoroAllarme::MulticastSuona_Implementation(bool bChiude)
{
	if (bChiude)
	{
		TrappoleSuona(this, TEXT("S_Frana"), 3000.f, 0.9f, 0.6f);
		TrappoleSuona(this, TEXT("S_Scheletro_Sveglia"), 2500.f, 1.f);
	}
	else
	{
		TrappoleSuona(this, TEXT("S_Frana"), 2500.f, 0.4f, 1.2f);
	}
}

void AValdorsoTesoroAllarme::Scatta(APawn* Chi)
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo || bChiusa)
	{
		return;
	}
	const double Ora = Mondo->GetTimeSeconds();
	if (Ora < PausaFino)
	{
		return;
	}
	bChiusa = true;
	ChiusaDa = Ora;
	ForceNetUpdate();
	MulticastSuona(true);
	// quelli dell'allarme di prima, se qualcuno e' rimasto in piedi, tornano polvere: la stanza non si riempie
	for (const TWeakObjectPtr<AValdorsoScheletro>& Rimasto : Orda)
	{
		if (AValdorsoScheletro* Vecchio = Rimasto.Get())
		{
			Vecchio->Destroy();
		}
	}
	Orda.Reset();
	for (int32 k = 0; k < Quanti; ++k)
	{
		const float Angolo = 2.f * PI * static_cast<float>(k) / static_cast<float>(Quanti) + FMath::FRandRange(-0.3f, 0.3f);
		const float Lontano = Raggio * FMath::FRandRange(0.55f, 1.f);
		FVector Dove = GetActorLocation() + FVector(FMath::Cos(Angolo) * Lontano, FMath::Sin(Angolo) * Lontano, 0.f);
		const FName Quale = (k % 3 == 2) ? FName(TEXT("ScheletroScudo")) : TipoScheletro;
		const FValdorsoTipoScheletro* SuoTipo = ValdorsoScheletri::Tipo(Quale);
		FHitResult Urto;
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoTesoroAllarme), false, this);
		if (Mondo->LineTraceSingleByChannel(Urto, Dove + FVector(0.f, 0.f, 200.f), Dove - FVector(0.f, 0.f, 300.f), ECC_WorldStatic, Parametri))
		{
			Dove = Urto.ImpactPoint + FVector(0.f, 0.f, 95.f * (SuoTipo ? SuoTipo->Grandezza : 1.f));
		}
		const FTransform Posto(FRotator(0.f, FMath::RadiansToDegrees(Angolo) + 180.f, 0.f), Dove);
		AValdorsoScheletro* Nuovo = Mondo->SpawnActorDeferred<AValdorsoScheletro>(AValdorsoScheletro::StaticClass(), Posto, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (Nuovo)
		{
			Nuovo->Prepara(Quale, Dove, true);
			Nuovo->FinishSpawning(Posto);
			Orda.Add(Nuovo);
		}
	}
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(It->Get());
		const APawn* Corpo = Giocatore ? Giocatore->GetPawn() : nullptr;
		if (Corpo && FVector::Dist(Corpo->GetActorLocation(), GetActorLocation()) < 2000.f)
		{
			Giocatore->ClientAvviso(LOCTEXT("Allarme", "Il coperchio scatta, e alle tue spalle cade la grata. Intorno, le ossa si alzano."), 5.f, 0.f);
		}
	}
	ValdorsoRegistroEventi::Annota(TEXT("TesoroAllarme"), Chi, this, GetActorLocation(), FString::Printf(TEXT("%d scheletri svegliati"), Orda.Num()));
}

void AValdorsoTesoroAllarme::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// la grata: giu' di colpo (un terzo di secondo), su piano (tre secondi)
	Giu = bChiusa ? FMath::Min(1.f, Giu + DeltaSeconds / 0.33f) : FMath::Max(0.f, Giu - DeltaSeconds / 3.f);
	Grata->SetRelativeLocation(GrataDove - FVector(0.f, 0.f, 430.f * (1.f - Giu)));
	if (!HasAuthority() || !bChiusa || !GetWorld())
	{
		return;
	}
	const double Ora = GetWorld()->GetTimeSeconds();
	const bool bTuttiGiu = !Orda.ContainsByPredicate([](const TWeakObjectPtr<AValdorsoScheletro>& S) { return S.IsValid() && S->Vivo(); });
	if (bTuttiGiu || Ora - ChiusaDa > static_cast<double>(SecondiMassimi))
	{
		bChiusa = false;
		PausaFino = Ora + static_cast<double>(MinutiPausa) * 60.0;
		ForceNetUpdate();
		MulticastSuona(false);
	}
}

#undef LOCTEXT_NAMESPACE
