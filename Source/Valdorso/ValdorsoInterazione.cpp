// Valdorso - Le interazioni con il mondo (vedi il .h). Scritto da Claude il 03/11/2026.

#include "ValdorsoInterazione.h"
#include "Valdorso.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoSeduta.h"
#include "ValdorsoLetto.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ValdorsoInterazione"

namespace ValdorsoInterazione
{
	bool SiVede(const APawn* Chi, const AActor* Cosa)
	{
		const IValdorsoInterattivo* Interattivo = Cast<IValdorsoInterattivo>(Cosa);
		UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
		if (!Interattivo || !Mondo)
		{
			return false;
		}
		// Dagli occhi del colono al punto della cosa: se in mezzo c'è altro (un muro), non si usa.
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoInterazioneVista), false, Chi);
		Parametri.AddIgnoredActor(Cosa);
		// Anche il corpo MetaHuman attaccato al colono (è un attore a parte) non deve fare da muro.
		TArray<AActor*> Attaccati;
		Chi->GetAttachedActors(Attaccati, true, true);
		Parametri.AddIgnoredActors(Attaccati);
		FHitResult Colpo;
		const FVector Occhi = Chi->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		return !Mondo->LineTraceSingleByChannel(Colpo, Occhi, Interattivo->PuntoInterazione(), ECC_Visibility, Parametri);
	}

	bool PuoUsare(const APawn* Chi, const AActor* Cosa)
	{
		if (!Chi || !Cosa)
		{
			return false;
		}
		// (06/11) Una cosa nascosta (per esempio la porta di una casa di una fase del villaggio che non c'è ancora)
		// non si usa.
		if (Cosa->IsHidden())
		{
			return false;
		}
		if (const UValdorsoSedutaComponent* Seduta = Chi->FindComponentByClass<UValdorsoSedutaComponent>(); Seduta && Seduta->Occupato())
		{
			return Cosa == Seduta->GetPanca();
		}
		if (const UValdorsoSonnoComponent* Sonno = Chi->FindComponentByClass<UValdorsoSonnoComponent>(); Sonno && Sonno->Occupato())
		{
			return Cosa == Sonno->GetLetto();
		}
		return true;
	}

	AActor* PiuVicina(const APawn* Chi)
	{
		UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
		if (!Mondo)
		{
			return nullptr;
		}
		const FVector Centro = Chi->GetActorLocation();
		const FVector Avanti = Chi->GetActorForwardVector();
		TArray<FOverlapResult> Trovati;
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoInterazione), false, Chi);
		Mondo->OverlapMultiByObjectType(Trovati, Centro, FQuat::Identity,
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), FCollisionShape::MakeSphere(Raggio), Parametri);

		AActor* Migliore = nullptr;
		float Punteggio = TNumericLimits<float>::Max();
		for (const FOverlapResult& Trovato : Trovati)
		{
			AActor* Attore = Trovato.GetActor();
			const IValdorsoInterattivo* Cosa = Cast<IValdorsoInterattivo>(Attore);
			if (!Cosa || !PuoUsare(Chi, Attore) || Cosa->TestoInterazione(Chi).IsEmpty())
			{
				continue;
			}
			const FVector Verso = Cosa->PuntoInterazione() - Centro;
			const float Distanza = Verso.Size2D();
			if (Distanza > Raggio || !SiVede(Chi, Attore))
			{
				continue;
			}
			// Conta di più quello che il colono ha davanti: chi sta alle spalle "costa" il doppio.
			const float Davanti = FVector::DotProduct(Avanti, Verso.GetSafeNormal2D());
			const float Costo = Distanza * (Davanti > 0.f ? 1.f : 2.f);
			if (Costo < Punteggio)
			{
				Punteggio = Costo;
				Migliore = Attore;
			}
		}
		return Migliore;
	}
}

AValdorsoPorta::AValdorsoPorta()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(10.f);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent->SetMobility(EComponentMobility::Static);

	// La cerniera sta sul bordo sinistro dell'anta; l'origine dell'attore è al centro della soglia, a terra.
	Cerniera = CreateDefaultSubobject<USceneComponent>(TEXT("Cerniera"));
	Cerniera->SetupAttachment(RootComponent);
	Cerniera->SetMobility(EComponentMobility::Movable);
	Cerniera->SetRelativeLocation(FVector(0.f, -50.f, 0.f));

	Anta = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Anta"));
	Anta->SetupAttachment(Cerniera);
	Anta->SetMobility(EComponentMobility::Movable);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
	// (03/11) Il materiale semplice con il parametro "Color": senza, il cubo usava la griglia a scacchi.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Semplice(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Cubo.Succeeded())
	{
		Anta->SetStaticMesh(Cubo.Object);
		// Il cubo di serie è 100 cm con l'origine al centro: anta 100 × 6 × 210 cm, che parte dalla cerniera.
		Anta->SetRelativeScale3D(FVector(0.06f, 1.f, 2.1f));
		Anta->SetRelativeLocation(FVector(0.f, 50.f, 105.f));
		if (Semplice.Succeeded())
		{
			Anta->SetMaterial(0, Semplice.Object);
		}
	}
	Anta->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// La telecamera non deve "saltare" quando passa vicino all'anta.
	Anta->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void AValdorsoPorta::BeginPlay()
{
	Super::BeginPlay();
	// Il colore del legno sul cubo di serie (solo se nessuno ha messo un modello vero).
	if (Anta && Anta->GetStaticMesh() && Anta->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/")))
	{
		if (UMaterialInstanceDynamic* Legno = Anta->CreateDynamicMaterialInstance(0))
		{
			Legno->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.23f, 0.13f, 0.06f));
		}
	}
	AngoloAttuale = Verso * AngoloApertura;
	Cerniera->SetRelativeRotation(FRotator(0.f, AngoloAttuale, 0.f));
}

void AValdorsoPorta::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoPorta, Verso);
	DOREPLIFETIME(AValdorsoPorta, bChiusaAChiave);
}

FText AValdorsoPorta::TestoApri() const { return LOCTEXT("Apri", "Apri la porta"); }
FText AValdorsoPorta::TestoChiudi() const { return LOCTEXT("Chiudi", "Chiudi la porta"); }
FText AValdorsoPorta::TestoAChiave() const { return LOCTEXT("AChiave", "La porta è chiusa a chiave"); }

FText AValdorsoPorta::TestoInterazione(const APawn* Chi) const
{
	if (Verso != 0)
	{
		return TestoChiudi();
	}
	return bChiusaAChiave ? TestoAChiave() : TestoApri();
}

FVector AValdorsoPorta::PuntoInterazione() const
{
	// Il centro della soglia, a mezza altezza.
	return GetActorLocation() + FVector(0.f, 0.f, 100.f);
}

void AValdorsoPorta::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	if (Verso == 0 && bChiusaAChiave)
	{
		ValdorsoRegistroEventi::Annota(TEXT("PortaChiusa"), Chi, this, GetActorLocation(), TEXT("ha provato ad aprire"));
		return;
	}
	if (Verso != 0)
	{
		Verso = 0;
	}
	else
	{
		// Si apre dalla parte opposta a chi spinge. Con Verso +1 l'anta gira verso il lato X negativo della porta:
		// quindi chi sta sul lato X positivo la spinge con +1. (03/11: prima era al contrario, si apriva addosso.)
		const FVector DaChi = Chi->GetActorLocation() - GetActorLocation();
		Verso = FVector::DotProduct(DaChi, GetActorForwardVector()) >= 0.f ? 1 : -1;
		// Le imposte (e le porte che si vogliono così) si aprono sempre dalla stessa parte.
		if (VersoFisso != 0)
		{
			Verso = VersoFisso > 0 ? 1 : -1;
		}
	}
	ValdorsoRegistroEventi::Annota(Verso != 0 ? TEXT("PortaAperta") : TEXT("PortaChiusa"), Chi, this, GetActorLocation());
	ForceNetUpdate();
	OnRep_Verso();
}

void AValdorsoPorta::OnRep_Verso()
{
	// Si muove nel Tick finché arriva all'angolo voluto.
	SetActorTickEnabled(true);
}

void AValdorsoPorta::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Voluto = Verso * AngoloApertura;
	const float Passo = AngoloApertura / FMath::Max(0.1f, Durata) * DeltaSeconds;
	AngoloAttuale = FMath::Abs(Voluto - AngoloAttuale) <= Passo ? Voluto : AngoloAttuale + FMath::Sign(Voluto - AngoloAttuale) * Passo;
	Cerniera->SetRelativeRotation(FRotator(0.f, AngoloAttuale, 0.f));
	if (AngoloAttuale == Voluto)
	{
		SetActorTickEnabled(false);
	}
}

// ------------------------------------------------------------------------------------------------
// L'imposta
// ------------------------------------------------------------------------------------------------

AValdorsoImposta::AValdorsoImposta()
{
	// Verso fuori: con Verso -1 l'anta gira verso il lato X positivo (fuori dalla casa, se la X guarda fuori).
	VersoFisso = -1;
	AngoloApertura = 160.f;
	Durata = 0.5f;
	Cerniera->SetRelativeLocation(FVector(0.f, -30.f, 0.f));
	if (Anta)
	{
		Anta->SetRelativeScale3D(FVector(0.04f, 0.6f, 1.f));
		Anta->SetRelativeLocation(FVector(0.f, 30.f, 50.f));
	}
}

FVector AValdorsoImposta::PuntoInterazione() const
{
	// Il centro della finestra; il colono sta sotto, quindi si misura un po' più in basso.
	return GetActorLocation() + FVector(0.f, 0.f, -40.f);
}

FText AValdorsoImposta::TestoApri() const { return LOCTEXT("ApriImposta", "Apri l'imposta"); }
FText AValdorsoImposta::TestoChiudi() const { return LOCTEXT("ChiudiImposta", "Chiudi l'imposta"); }
FText AValdorsoImposta::TestoAChiave() const { return LOCTEXT("ImpostaSbarrata", "L'imposta è sbarrata"); }

// ------------------------------------------------------------------------------------------------
// La bacheca
// ------------------------------------------------------------------------------------------------

AValdorsoBacheca::AValdorsoBacheca()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);

	Titolo = LOCTEXT("TitoloBacheca", "Bacheca del villaggio");
	// Avvisi di partenza, della valle (anno 312): li tiene la piazza finché lo staff non ne scrive altri.
	Avvisi = {
		TEXT("La Gilda cerca braccia: lupi visti ai pascoli di sud-est. Chi ha un'arma e non ha paura chieda a Lucia, all'ufficio."),
		TEXT("Il mulino macina il martedì e il venerdì. Chi porta grano negli altri giorni aspetta. Teodoro e Agnese."),
		TEXT("Per ordine del Balivo: di notte la porta del passo resta chiusa. Chi arriva tardi bussi al posto di guardia."),
		TEXT("Persa una capra bianca con il campanaccio, verso il boschetto a est. Chi la riporta alle fattorie avrà formaggio."),
		TEXT("Nonna Edda, nella palude sul lago, vende erbe solo a chi saluta. Non fatele domande sul fondo del lago."),
	};

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Semplice(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	auto Pezzo = [&](const TCHAR* Nome, const FVector& Scala, const FVector& Posto)
	{
		UStaticMeshComponent* Nuovo = CreateDefaultSubobject<UStaticMeshComponent>(Nome);
		Nuovo->SetupAttachment(RootComponent);
		if (Cubo.Succeeded())
		{
			Nuovo->SetStaticMesh(Cubo.Object);
		}
		if (Semplice.Succeeded())
		{
			Nuovo->SetMaterial(0, Semplice.Object);
		}
		Nuovo->SetRelativeScale3D(Scala);
		Nuovo->SetRelativeLocation(Posto);
		Nuovo->SetCollisionProfileName(TEXT("BlockAll"));
		Nuovo->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		return Nuovo;
	};
	// Una tavola di 140 × 90 cm su due pali alti 2 m; l'origine è a terra, al centro, e la X guarda chi legge.
	Tavola = Pezzo(TEXT("Tavola"), FVector(0.06f, 1.4f, 0.9f), FVector(0.f, 0.f, 145.f));
	PaloSinistro = Pezzo(TEXT("PaloSinistro"), FVector(0.1f, 0.1f, 2.f), FVector(-6.f, -75.f, 100.f));
	PaloDestro = Pezzo(TEXT("PaloDestro"), FVector(0.1f, 0.1f, 2.f), FVector(-6.f, 75.f, 100.f));
}

void AValdorsoBacheca::BeginPlay()
{
	Super::BeginPlay();
	// Legno scuro per i pali, legno chiaro per la tavola (solo sui cubi di serie).
	auto Colora = [](UStaticMeshComponent* Pezzo, const FLinearColor& Colore)
	{
		if (Pezzo && Pezzo->GetStaticMesh() && Pezzo->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/")))
		{
			if (UMaterialInstanceDynamic* Legno = Pezzo->CreateDynamicMaterialInstance(0))
			{
				Legno->SetVectorParameterValue(TEXT("Color"), Colore);
			}
		}
	};
	Colora(Tavola, FLinearColor(0.42f, 0.29f, 0.16f));
	Colora(PaloSinistro, FLinearColor(0.2f, 0.12f, 0.06f));
	Colora(PaloDestro, FLinearColor(0.2f, 0.12f, 0.06f));
}

namespace
{
	/** (03/11) Vero se il colono sta davanti alla bacheca (lato X positivo, dove sono appesi i fogli). */
	bool DavantiAllaBacheca(const AActor* Bacheca, const APawn* Chi)
	{
		return Chi && FVector::DotProduct(Chi->GetActorLocation() - Bacheca->GetActorLocation(), Bacheca->GetActorForwardVector()) > 0.f;
	}
}

FText AValdorsoBacheca::TestoInterazione(const APawn* Chi) const
{
	// Da dietro si vedono solo le assi: i fogli si leggono stando davanti.
	return DavantiAllaBacheca(this, Chi) ? LOCTEXT("Leggi", "Leggi la bacheca") : FText::GetEmpty();
}

FVector AValdorsoBacheca::PuntoInterazione() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 100.f);
}

void AValdorsoBacheca::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi || !DavantiAllaBacheca(this, Chi))
	{
		return;
	}
	// Solo i primi 20 avvisi, tagliati a 300 lettere: un messaggio in rete non deve diventare enorme.
	TArray<FString> DaMandare;
	for (const FString& Avviso : Avvisi)
	{
		if (DaMandare.Num() >= 20)
		{
			break;
		}
		const FString Pulito = Avviso.TrimStartAndEnd().Left(300);
		if (!Pulito.IsEmpty())
		{
			DaMandare.Add(Pulito);
		}
	}
	if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(Chi->GetController()))
	{
		Controllore->ClientMostraBacheca(Titolo.ToString().Left(60), DaMandare);
	}
}

#undef LOCTEXT_NAMESPACE
