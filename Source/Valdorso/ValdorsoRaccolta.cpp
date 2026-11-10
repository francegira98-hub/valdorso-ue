// Valdorso - Le cose da raccogliere nella valle (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoRaccolta.h"
#include "ValdorsoAcque.h"
#include "Valdorso.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoTempo.h"
#include "ValdorsoZaino.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ValdorsoRaccolta"

namespace
{
	/** Quanti pezzi di forma di serie ha al massimo una cosa da raccogliere. */
	constexpr int32 RaccoltaPezzi = 6;
	/** Ogni quanto (secondi) il server guarda se un colono si è avvicinato a un posto. */
	constexpr double RaccoltaSecondiControllo = 2.0;
	const FName RaccoltaColore(TEXT("Color"));

	/** Dove si raccoglie cosa: il luogo, l'oggetto, quanti per volta, quanti posti, quanto lontano dal centro del
	 *  luogo (frazione del suo raggio di scoperta) e in quanti secondi ricresce. */
	struct FRaccoltaRegola
	{
		const TCHAR* Luogo;
		const TCHAR* Oggetto;
		int32 Quantita;
		int32 Posti;
		float Lontano;
		float Ricrescita;
		/** (10/11) Sul fondo dell'acqua. */
		bool bFondale = false;
	};
	/** (10/11) Quanto sott'acqua deve stare il fondo per le cose del fondale (cm), e la probabilita' di una perla. */
	constexpr float RaccoltaFondoMinimo = 200.f;
	constexpr float RaccoltaProbabilitaPerla = 0.1f;

	const FRaccoltaRegola RaccoltaRegole[] = {
		{ TEXT("Boschetto"), TEXT("Rami_Secchi"), 2, 14, 0.8f, 360.f },
		{ TEXT("Boschetto"), TEXT("Funghi"), 2, 8, 0.8f, 600.f },
		{ TEXT("Collina dell'Orso"), TEXT("Rami_Secchi"), 2, 8, 0.6f, 360.f },
		{ TEXT("Collina dell'Orso"), TEXT("Funghi"), 2, 6, 0.6f, 600.f },
		{ TEXT("Palude"), TEXT("Erbe_Palude"), 1, 10, 0.7f, 600.f },
		// (10/11) le cozze sul fondo del lago (molti posti: quelli troppo bassi o sulla riva si saltano)
		{ TEXT("Lago"), TEXT("Cozze"), 3, 30, 0.7f, 600.f, true },
	};

	TArray<FValdorsoPuntoRaccolta> RaccoltaCreaPunti()
	{
		TArray<FValdorsoPuntoRaccolta> Tutti;
		int32 Seme = 20261108;
		for (const FRaccoltaRegola& Regola : RaccoltaRegole)
		{
			const FValdorsoLuogo* Luogo = ValdorsoLuoghi::Trova(Regola.Luogo);
			if (!Luogo || !ValdorsoOggetti::Trova(Regola.Oggetto))
			{
				continue;
			}
			FRandomStream Dadi(Seme++);
			const float Raggio = Luogo->Raggio * Regola.Lontano;
			for (int32 i = 0; i < Regola.Posti; ++i)
			{
				// un punto a caso nel cerchio, lontano dagli altri (venti tentativi, poi si lascia stare)
				for (int32 Tentativo = 0; Tentativo < 20; ++Tentativo)
				{
					const float Angolo = Dadi.FRandRange(0.f, 2.f * PI);
					const float Distanza = Raggio * FMath::Sqrt(Dadi.FRand());
					const FVector Dove = Luogo->Posizione + FVector(Distanza * FMath::Cos(Angolo), Distanza * FMath::Sin(Angolo), 0.f);
					bool bLibero = true;
					for (const FValdorsoPuntoRaccolta& Altro : Tutti)
					{
						if (FVector::Dist2D(Altro.Posizione, Dove) < ValdorsoRaccolta::DistanzaMinima)
						{
							bLibero = false;
							break;
						}
					}
					if (bLibero)
					{
						FValdorsoPuntoRaccolta Punto;
						Punto.Oggetto = Regola.Oggetto;
						Punto.Quantita = Regola.Quantita;
						Punto.Posizione = Dove;
						Punto.Ricrescita = Regola.Ricrescita;
						Punto.bFondale = Regola.bFondale;
						Tutti.Add(Punto);
						break;
					}
				}
			}
		}
		return Tutti;
	}

	FAutoConsoleCommandWithWorld ComandoRaccoltaRicresci(
		TEXT("Valdorso.Raccolta.Ricresci"),
		TEXT("Per le prove: tutte le cose da raccogliere (rami, funghi, erbe) tornano subito al loro posto (sul server, o da soli)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* Mondo)
		{
			if (UValdorsoRaccolteSubsystem* Raccolte = Mondo ? Mondo->GetSubsystem<UValdorsoRaccolteSubsystem>() : nullptr)
			{
				Raccolte->RicresciTutto();
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Raccolta: tutto ricresciuto"));
			}
		}));
}

namespace ValdorsoRaccolta
{
	const TArray<FValdorsoPuntoRaccolta>& Punti()
	{
		static const TArray<FValdorsoPuntoRaccolta> Tutti = RaccoltaCreaPunti();
		return Tutti;
	}
}

// ------------------------------------------------------------------------------------------------- la cosa da raccogliere

AValdorsoRaccolta::AValdorsoRaccolta()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetCanBeDamaged(false);
	// cambia di rado (raccolta, ricresciuta: ForceNetUpdate)
	SetNetUpdateFrequency(1.f);

	// la zona che il tasto E trova (OverlapAllDynamic: non ferma niente, né i passi né le linee della vista)
	Zona = CreateDefaultSubobject<USphereComponent>(TEXT("Zona"));
	Zona->InitSphereRadius(45.f);
	Zona->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Zona->SetGenerateOverlapEvents(false);
	Zona->SetCanEverAffectNavigation(false);
	RootComponent = Zona;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sfera(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cono(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Semplice(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	FormaSfera = Sfera.Succeeded() ? Sfera.Object : nullptr;
	FormaCilindro = Cilindro.Succeeded() ? Cilindro.Object : nullptr;
	FormaCono = Cono.Succeeded() ? Cono.Object : nullptr;
	MaterialeSemplice = Semplice.Succeeded() ? Semplice.Object : nullptr;

	for (int32 i = 0; i < RaccoltaPezzi; ++i)
	{
		UStaticMeshComponent* Pezzo = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Pezzo%d"), i));
		Pezzo->SetupAttachment(Zona);
		Pezzo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pezzo->SetCollisionProfileName(TEXT("NoCollision"));
		Pezzo->SetCastShadow(false);
		Pezzo->SetCanEverAffectNavigation(false);
		Pezzo->SetVisibility(false);
		Pezzi.Add(Pezzo);
	}
}

void AValdorsoRaccolta::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoRaccolta, Oggetto);
	DOREPLIFETIME(AValdorsoRaccolta, bDisponibile);
}

void AValdorsoRaccolta::Prepara(FName InOggetto, int32 InQuantita, float InRicrescita)
{
	Oggetto = InOggetto;
	Quantita = FMath::Max(1, InQuantita);
	Ricrescita = FMath::Max(10.f, InRicrescita);
}

void AValdorsoRaccolta::BeginPlay()
{
	Super::BeginPlay();
	Sistema();
	MostraDisponibile();
}

void AValdorsoRaccolta::OnRep_Oggetto()
{
	Sistema();
	MostraDisponibile();
}

void AValdorsoRaccolta::OnRep_Disponibile()
{
	MostraDisponibile();
}

void AValdorsoRaccolta::Sistema()
{
	if (bSistemato || Oggetto.IsNone())
	{
		return;
	}
	bSistemato = true;
	// la zona è a mezz'aria sopra il terreno: i pezzi scendono di quanto è alta
	const float Giu = -Zona->GetUnscaledSphereRadius();
	struct FForma
	{
		UStaticMesh* Mesh;
		FVector Dove;
		FRotator Giro;
		FVector Scala;
		FLinearColor Colore;
	};
	TArray<FForma> Forme;
	const FString Nome = Oggetto.ToString();
	if (Nome == TEXT("Rami_Secchi"))
	{
		// tre rami coricati, incrociati
		for (int32 i = 0; i < 3; ++i)
		{
			Forme.Add({ FormaCilindro, FVector(0.f, 0.f, Giu + 3.f + 2.5f * i), FRotator(90.f, i * 55.f + 10.f, 0.f),
				FVector(0.035f, 0.035f, 0.75f - 0.1f * i), FLinearColor(0.16f, 0.1f, 0.05f) });
		}
	}
	else if (Nome == TEXT("Funghi"))
	{
		// tre funghi: il gambo chiaro, il cappello scuro
		const FVector Dove[3] = { FVector(0.f, 0.f, 0.f), FVector(14.f, 6.f, 0.f), FVector(-6.f, 13.f, 0.f) };
		const float Grandi[3] = { 1.f, 0.75f, 0.6f };
		for (int32 i = 0; i < 3; ++i)
		{
			const float G = Grandi[i];
			Forme.Add({ FormaCilindro, Dove[i] + FVector(0.f, 0.f, Giu + 5.f * G), FRotator::ZeroRotator,
				FVector(0.045f * G, 0.045f * G, 0.1f * G), FLinearColor(0.78f, 0.72f, 0.6f) });
			Forme.Add({ FormaSfera, Dove[i] + FVector(0.f, 0.f, Giu + 10.f * G), FRotator::ZeroRotator,
				FVector(0.15f * G, 0.15f * G, 0.07f * G), FLinearColor(0.36f, 0.19f, 0.07f) });
		}
	}
	else if (Nome == TEXT("Cozze"))
	{
		// (10/11) un mucchietto di cozze sul fondo: gusci scuri, schiacciati e allungati
		const FVector Dove[4] = { FVector(0.f, 0.f, 0.f), FVector(10.f, 4.f, 0.f), FVector(-7.f, 9.f, 0.f), FVector(3.f, -10.f, 0.f) };
		for (int32 i = 0; i < 4; ++i)
		{
			Forme.Add({ FormaSfera, Dove[i] + FVector(0.f, 0.f, Giu + 3.f), FRotator(0.f, i * 47.f, 8.f * i),
				FVector(0.12f, 0.06f, 0.04f), FLinearColor(0.04f, 0.05f, 0.09f) });
		}
	}
	else
	{
		// un ciuffo d'erba: coni sottili, un poco storti (le erbe della palude e tutto il resto)
		for (int32 i = 0; i < 5; ++i)
		{
			const float Angolo = i * 72.f;
			const FVector Lato(9.f * FMath::Cos(FMath::DegreesToRadians(Angolo)), 9.f * FMath::Sin(FMath::DegreesToRadians(Angolo)), 0.f);
			const float Alto = 0.3f + 0.06f * (i % 3);
			Forme.Add({ FormaCono, Lato + FVector(0.f, 0.f, Giu + 50.f * Alto), FRotator(10.f * FMath::Cos(i * 1.7f), Angolo, 12.f * FMath::Sin(i * 1.3f)),
				FVector(0.05f, 0.05f, Alto), FLinearColor(0.12f + 0.02f * i, 0.32f, 0.08f) });
		}
	}
	for (int32 i = 0; i < Pezzi.Num(); ++i)
	{
		UStaticMeshComponent* Pezzo = Pezzi[i];
		if (!Pezzo)
		{
			continue;
		}
		if (!Forme.IsValidIndex(i) || !Forme[i].Mesh)
		{
			Pezzo->SetVisibility(false);
			continue;
		}
		const FForma& Forma = Forme[i];
		Pezzo->SetStaticMesh(Forma.Mesh);
		Pezzo->SetRelativeLocation(Forma.Dove);
		Pezzo->SetRelativeRotation(Forma.Giro);
		Pezzo->SetRelativeScale3D(Forma.Scala);
		if (MaterialeSemplice && GetNetMode() != NM_DedicatedServer)
		{
			if (UMaterialInstanceDynamic* Tinta = Pezzo->CreateDynamicMaterialInstance(0, MaterialeSemplice))
			{
				Tinta->SetVectorParameterValue(RaccoltaColore, Forma.Colore);
			}
		}
		Pezzo->SetVisibility(true);
	}
}

void AValdorsoRaccolta::MostraDisponibile()
{
	// raccolta: sparisce (e il tasto E non la trova più) finché non ricresce
	SetActorHiddenInGame(!bDisponibile);
	if (Zona)
	{
		Zona->SetCollisionEnabled(bDisponibile ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

FText AValdorsoRaccolta::TestoInterazione(const APawn* Chi) const
{
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Oggetto);
	if (!bDisponibile || !Tipo)
	{
		return FText::GetEmpty();
	}
	return FText::Format(LOCTEXT("Raccogli", "Raccogli: {0}"), FText::FromString(Tipo->Nome));
}

FVector AValdorsoRaccolta::PuntoInterazione() const
{
	return GetActorLocation();
}

void AValdorsoRaccolta::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !bDisponibile || !Chi)
	{
		return;
	}
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Cast<APlayerController>(Chi->GetController()));
	const int32 Entrati = Zaino ? Zaino->Dai(Oggetto, Quantita) : 0;
	if (Entrati <= 0)
	{
		return;
	}
	if (Entrati < Quantita)
	{
		// lo zaino era quasi pieno: il resto rimane qui per dopo
		Quantita -= Entrati;
		return;
	}
	// (10/11) dentro le cozze, ogni tanto, una perla
	if (Oggetto == FName(TEXT("Cozze")) && FMath::FRand() < RaccoltaProbabilitaPerla)
	{
		Zaino->Dai(TEXT("Perla"), 1);
	}
	bDisponibile = false;
	MostraDisponibile();
	ForceNetUpdate();
	GetWorldTimerManager().SetTimer(TimerRicrescita, this, &AValdorsoRaccolta::Ricresci, Ricrescita, false);
}

void AValdorsoRaccolta::Ricresci()
{
	if (!HasAuthority())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(TimerRicrescita);
	bDisponibile = true;
	MostraDisponibile();
	ForceNetUpdate();
}

// ------------------------------------------------------------------------------------------------- il sottosistema

bool UValdorsoRaccolteSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* Mondo = Cast<UWorld>(Outer);
	return Mondo && Mondo->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
}

TStatId UValdorsoRaccolteSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UValdorsoRaccolteSubsystem, STATGROUP_Tickables);
}

void UValdorsoRaccolteSubsystem::Tick(float DeltaTime)
{
	UWorld* Mondo = GetWorld();
	if (!Mondo || Mondo->GetNetMode() == NM_Client || !Mondo->HasBegunPlay())
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	if (Adesso < ProssimoControllo)
	{
		return;
	}
	ProssimoControllo = Adesso + RaccoltaSecondiControllo;
	// solo nella valle (dove c'è il tempo della valle): non nel menu di prova né in altre mappe
	if (!bValleControllata)
	{
		bValleControllata = true;
		bValle = AValdorsoTempo::Di(Mondo) != nullptr;
	}
	if (!bValle)
	{
		return;
	}
	const TArray<FValdorsoPuntoRaccolta>& Punti = ValdorsoRaccolta::Punti();
	if (Messi.Num() != Punti.Num())
	{
		Messi.SetNum(Punti.Num());
	}
	// dove sono i coloni (solo quelli veri: il menu nella valle non ha un colono)
	TArray<FVector, TInlineAllocator<16>> Coloni;
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Controllore = It->Get();
		if (const AValdorsoCharacter* Colono = Controllore ? Cast<AValdorsoCharacter>(Controllore->GetPawn()) : nullptr)
		{
			Coloni.Add(Colono->GetActorLocation());
		}
	}
	if (!Coloni.Num())
	{
		return;
	}
	for (int32 i = 0; i < Punti.Num(); ++i)
	{
		if (Messi[i].IsValid() || Saltati.Contains(i))
		{
			continue;
		}
		for (const FVector& Dove : Coloni)
		{
			if (FVector::Dist2D(Dove, Punti[i].Posizione) < ValdorsoRaccolta::RaggioComparsa)
			{
				Metti(i);
				break;
			}
		}
	}
}

bool UValdorsoRaccolteSubsystem::Metti(int32 Indice)
{
	UWorld* Mondo = GetWorld();
	const TArray<FValdorsoPuntoRaccolta>& Punti = ValdorsoRaccolta::Punti();
	if (!Mondo || !Punti.IsValidIndex(Indice))
	{
		return false;
	}
	const FValdorsoPuntoRaccolta& Punto = Punti[Indice];
	// la linea verso il basso: deve trovare il paesaggio (non un tetto, non un muro, non la cima di una roccia)
	// (le chiome degli alberi, quando ci saranno, non contano: la linea le salta e cerca il paesaggio sotto; ma una cosa
	// a meno di 3 m sopra il terreno è un tetto, o una roccia, e il posto si salta)
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoRaccolta), false);
	FVector Alto = FVector(Punto.Posizione.X, Punto.Posizione.Y, Punto.Posizione.Z + 30000.f);
	const FVector Basso = FVector(Punto.Posizione.X, Punto.Posizione.Y, Punto.Posizione.Z - 30000.f);
	FHitResult Colpo;
	float SopraPiuBasso = TNumericLimits<float>::Max();
	bool bTerra = false;
	for (int32 Giro = 0; Giro < 6; ++Giro)
	{
		if (!Mondo->LineTraceSingleByChannel(Colpo, Alto, Basso, ECC_Visibility, Parametri))
		{
			break;
		}
		AActor* Toccato = Colpo.GetActor();
		if (Toccato && Toccato->GetClass()->GetName().Contains(TEXT("Landscape")))
		{
			bTerra = true;
			break;
		}
		// non è il paesaggio: si ricorda quanto in basso arriva, la si salta e si guarda sotto
		SopraPiuBasso = FMath::Min(SopraPiuBasso, static_cast<float>(Colpo.ImpactPoint.Z));
		if (Toccato)
		{
			Parametri.AddIgnoredActor(Toccato);
		}
		Alto = Colpo.ImpactPoint - FVector(0.f, 0.f, 1.f);
	}
	if (!bTerra)
	{
		return false;  // il terreno lì non è ancora caricato: si riprova al prossimo giro
	}
	if (Punto.bFondale)
	{
		// (10/11) sul fondale: sopra c'e' l'acqua (e la sua superficie, che la linea trova), il fondo deve essere ben sotto
		// il pelo; un po' di pendenza va bene
		const AValdorsoAcque* Acque = AValdorsoAcque::Di(Mondo);
		float Pelo = 0.f;
		if (!Acque || !Acque->PeloQui(FVector2D(Colpo.ImpactPoint), Pelo) || Pelo < Colpo.ImpactPoint.Z + RaccoltaFondoMinimo
			|| Colpo.ImpactNormal.Z < 0.7f)
		{
			Saltati.Add(Indice);
			return false;
		}
	}
	else if (SopraPiuBasso < Colpo.ImpactPoint.Z + 300.f || Colpo.ImpactNormal.Z < 0.85f)
	{
		Saltati.Add(Indice);  // sopra c'è altro, o è troppo ripido: questo posto si lascia stare
		return false;
	}
	FActorSpawnParameters Come;
	Come.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Come.bDeferConstruction = true;
	// la zona (45 cm) poggia sul terreno; un giro a caso, sempre lo stesso per quel posto
	const FTransform Dove(FRotator(0.f, FMath::Fmod(Indice * 97.f, 360.f), 0.f), Colpo.ImpactPoint + FVector(0.f, 0.f, 45.f));
	AValdorsoRaccolta* Cosa = Mondo->SpawnActor<AValdorsoRaccolta>(AValdorsoRaccolta::StaticClass(), Dove, Come);
	if (!Cosa)
	{
		return false;
	}
	Cosa->Prepara(Punto.Oggetto, Punto.Quantita, Punto.Ricrescita);
	Cosa->FinishSpawning(Dove);
	Messi[Indice] = Cosa;
	return true;
}

void UValdorsoRaccolteSubsystem::RicresciTutto()
{
	for (const TWeakObjectPtr<AValdorsoRaccolta>& Cosa : Messi)
	{
		if (AValdorsoRaccolta* Viva = Cosa.Get())
		{
			Viva->Ricresci();
		}
	}
}

#undef LOCTEXT_NAMESPACE
