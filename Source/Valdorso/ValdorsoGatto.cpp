// Valdorso - Il gatto della locanda (v0.1.6, 11/11/2026). Scritto da Claude. Vedi ValdorsoGatto.h.

#include "ValdorsoGatto.h"
#include "Valdorso.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoVoci.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoGatto"

namespace
{
	const TCHAR* GattoCartella = TEXT("/Game/Valle/Gatto/");

	UStaticMesh* GattoModello(const TCHAR* Nome)
	{
		const FString Percorso = FString::Printf(TEXT("%s%s.%s"), GattoCartella, Nome, Nome);
		if (UStaticMesh* Trovato = LoadObject<UStaticMesh>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			return Trovato;
		}
		// l'importazione dei .glb puo' mettere il modello in una sottocartella (crea_gatto.py lo rimette a posto)
		const FString Dentro = FString::Printf(TEXT("%s%s/StaticMeshes/%s.%s"), GattoCartella, Nome, Nome, Nome);
		return LoadObject<UStaticMesh>(nullptr, *Dentro, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	TArray<FValdorsoIndizioGatto> GattoTabella()
	{
		TArray<FValdorsoIndizioGatto> T;
		T.Add({ TEXT("battito"), TEXT("Il gatto non vuole carezze: ha il pelo dritto sulla schiena e fissa la porta, verso il tempio. Da quando il battito ha saltato, dicono, nessun animale vuole passare davanti all'altare."), true });
		T.Add({ TEXT("lucia"), TEXT("Il gatto salta sul tuo zaino e annusa a lungo la mappa di carta. Poi ti guarda come per dire: non e' tutta qui. Alla Gilda, c'e' chi ne tiene altre pagine nel cassetto."), false });
		T.Add({ TEXT("grotta_est"), TEXT("Il gatto si struscia sulle tue gambe e ti lascia sul mantello un ciuffo di muschio che, al buio, fa una luce verde. Muschio cosi' cresce solo in fondo alla grotta a est, davanti a una porta senza serratura."), false });
		T.Add({ TEXT("rovine"), TEXT("Il gatto ha le zampe sporche di una polvere bianca, fine come farina: quella delle pietre delle rovine degli Antichi, nel bosco. Di notte, la', qualcosa si accende."), false });
		T.Add({ TEXT("frana"), TEXT("Il gatto drizza le orecchie verso nord, verso la montagna, come se sentisse qualcosa sotto i piedi. Di notte, da lassu', davanti alla frana dei nani, si sentono dei colpi."), false });
		T.Add({ TEXT("belvedere"), TEXT("Il gatto ti porta un fiore di montagna, di quelli che crescono solo sopra la cascata. Lassu', in cima al sentiero, qualcuno ha fatto un terrazzino per guardare tutta la valle."), false });
		T.Add({ TEXT("palude"), TEXT("Il gatto sa di palude e ha un rametto di salice impigliato nel pelo. Nonna Edda gli da' da mangiare, dicono. E lei, di erbe, ne sa piu' del tempio."), false });
		T.Add({ TEXT("gaspare"), TEXT("Il gatto lascia ai tuoi piedi una piuma di rapace. Al margine del bosco c'e' un vecchio cacciatore che di rapaci, e di ombre, se ne intende."), false });
		return T;
	}

	/** Le pose: dove sta il corpo (cm) e come e' girato, e come stanno la testa e la coda. */
	struct FGattoPosa
	{
		FVector Corpo = FVector(0.f, 0.f, 18.f);
		FRotator Girato = FRotator::ZeroRotator;
		FRotator Testa = FRotator::ZeroRotator;
		FRotator Coda = FRotator::ZeroRotator;
	};

	FAutoConsoleCommandWithWorldAndArgs ComandoGattoQui(
		TEXT("Valdorso.Gatto.Qui"),
		TEXT("Porta il gatto della locanda davanti a te (solo su questo PC, fino al prossimo giro)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Mondo)
		{
			const APlayerController* Giocatore = Mondo ? Mondo->GetFirstPlayerController() : nullptr;
			const APawn* Io = Giocatore ? Giocatore->GetPawn() : nullptr;
			if (!Io)
			{
				return;
			}
			for (TActorIterator<AValdorsoGatto> It(Mondo); It; ++It)
			{
				It->VieniQui(Io->GetActorLocation() + Io->GetActorForwardVector() * 150.f - FVector(0.f, 0.f, 90.f));
				break;
			}
		}));
}

namespace ValdorsoGatto
{
	const TArray<FValdorsoIndizioGatto>& Indizi()
	{
		static const TArray<FValdorsoIndizioGatto> Tabella = GattoTabella();
		return Tabella;
	}

	const FValdorsoIndizioGatto* Prossimo(const TSet<FName>& Saputi, int32 Atto)
	{
		for (const FValdorsoIndizioGatto& I : Indizi())
		{
			if (I.bAttoI && Atto < 1)
			{
				continue;
			}
			if (!Saputi.Contains(FName(I.Id)))
			{
				return &I;
			}
		}
		return nullptr;
	}

	int32 PosaDelGiro(int64 Giro, int32 Seme)
	{
		const uint32 Mescola = GetTypeHash(Giro) ^ (static_cast<uint32>(Seme) * 2654435761u);
		return static_cast<int32>((Mescola >> 3) % 3u);
	}
}

AValdorsoGatto::AValdorsoGatto()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);

	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	RootComponent = Presa;
	Presa->InitSphereRadius(40.f);
	Presa->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Presa->SetCollisionObjectType(ECC_WorldDynamic);
	Presa->SetCollisionResponseToAllChannels(ECR_Overlap);
	Presa->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Presa->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Presa->SetGenerateOverlapEvents(false);
	Presa->SetHiddenInGame(true);

	Corpo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Corpo"));
	Corpo->SetupAttachment(Presa);
	Corpo->SetRelativeLocation(FVector(0.f, 0.f, 18.f));
	Testa = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Testa"));
	Testa->SetupAttachment(Corpo);
	Testa->SetRelativeLocation(FVector(21.f, 0.f, 9.f));
	Coda = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Coda"));
	Coda->SetupAttachment(Corpo);
	Coda->SetRelativeLocation(FVector(-21.f, 0.f, 5.f));
	for (UStaticMeshComponent* Pezzo : { Corpo.Get(), Testa.Get(), Coda.Get() })
	{
		Pezzo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pezzo->SetCanEverAffectNavigation(false);
	}
}

void AValdorsoGatto::BeginPlay()
{
	Super::BeginPlay();
	Casa = GetActorLocation();
	Yaw = static_cast<float>(GetActorRotation().Yaw);
	CorpoPos = FVector(0.f, 0.f, 18.f);
	if (GetNetMode() == NM_DedicatedServer)
	{
		// sul server serve solo dove sta (per la E): un passo ogni tanto basta
		SetActorTickInterval(0.25f);
		return;
	}
	Corpo->SetStaticMesh(GattoModello(TEXT("SM_Gatto_Corpo")));
	Testa->SetStaticMesh(GattoModello(TEXT("SM_Gatto_Testa")));
	Coda->SetStaticMesh(GattoModello(TEXT("SM_Gatto_Coda")));
	if (!Corpo->GetStaticMesh())
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Gatto: mancano i modelli in %s (Menu Valdorso -> Gatto della locanda)."), GattoCartella);
	}
}

double AValdorsoGatto::Orologio() const
{
	const UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return 0.0;
	}
	const AGameStateBase* Stato = Mondo->GetGameState();
	return Stato ? Stato->GetServerWorldTimeSeconds() : Mondo->GetTimeSeconds();
}

float AValdorsoGatto::TerraSotto(const FVector& Dove) const
{
	const UWorld* Mondo = GetWorld();
	FHitResult Urto;
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoGattoTerra), false, this);
	// solo la geometria ferma (non i coloni o gli animali di passaggio): cosi' ogni PC trova la stessa terra
	const FCollisionObjectQueryParams Statico(ECC_WorldStatic);
	if (Mondo && Mondo->LineTraceSingleByObjectType(Urto, Dove + FVector(0.f, 0.f, 80.f), Dove - FVector(0.f, 0.f, 200.f), Statico, Parametri))
	{
		return static_cast<float>(Urto.ImpactPoint.Z);
	}
	return static_cast<float>(Casa.Z);
}

FVector AValdorsoGatto::Posto(int64 Giro) const
{
	const UWorld* Mondo = GetWorld();
	FRandomStream Caso(static_cast<int32>(GetTypeHash(Giro) ^ (static_cast<uint32>(Seme) * 2246822519u)));
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoGattoPosto), false, this);
	const FCollisionObjectQueryParams Statico(ECC_WorldStatic);
	for (int32 Prova = 0; Prova < 6; ++Prova)
	{
		const float Angolo = Caso.FRandRange(0.f, 2.f * PI);
		const float Lontano = Caso.FRandRange(60.f, ValdorsoGatto::Raggio);
		const FVector Qui = Casa + FVector(FMath::Cos(Angolo) * Lontano, FMath::Sin(Angolo) * Lontano, 0.f);
		// da casa ci si arriva senza passare attraverso un muro o un mobile
		if (!Mondo || !Mondo->LineTraceTestByObjectType(Casa + FVector(0.f, 0.f, 25.f), Qui + FVector(0.f, 0.f, 25.f), Statico, Parametri))
		{
			return Qui;
		}
	}
	return Casa;
}

void AValdorsoGatto::VieniQui(const FVector& Dove)
{
	A = Dove;
	Da = Dove;
	ZA = TerraSotto(Dove);
	ZDa = ZA;
}

void AValdorsoGatto::Miagola() const
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	ValdorsoCombattimento::Suona(GetWorld(), ValdorsoCombattimento::SuonoPericoli(TEXT("S_Gatto_Miao")), GetActorLocation(), 1400.f, 0.55f,
		FMath::FRandRange(0.92f, 1.1f));
}

void AValdorsoGatto::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Ora = Orologio();
	const int64 Giro = static_cast<int64>(FMath::FloorToDouble(Ora / ValdorsoGatto::SecondiGiro));
	if (Giro != GiroVisto)
	{
		Da = GiroVisto == MIN_int64 ? Posto(Giro - 1) : A;
		A = Posto(Giro);
		ZDa = TerraSotto(Da);
		ZA = TerraSotto(A);
		if (GiroVisto != MIN_int64 && (Giro + Seme) % 5 == 0)
		{
			Miagola();
		}
		GiroVisto = Giro;
	}
	const float Dentro = static_cast<float>(Ora - static_cast<double>(Giro) * ValdorsoGatto::SecondiGiro);
	const float Tempo = static_cast<float>(FMath::Fmod(Ora, 3600.0));
	int32 Quale = ValdorsoGatto::PosaDelGiro(Giro, Seme);
	float Passo = 0.f;
	FVector Dove = GetActorLocation();
	if (FusaVerso != 0.f && Ora < FusaFino)
	{
		// fa le fusa: resta dov'e', girato verso chi lo accarezza
		Quale = 3;
		Yaw = FMath::FixedTurn(Yaw, FusaVerso, 300.f * DeltaSeconds);
	}
	else if (Dentro < ValdorsoGatto::SecondiCammino)
	{
		const float Alfa = FMath::SmoothStep(0.f, 1.f, Dentro / static_cast<float>(ValdorsoGatto::SecondiCammino));
		Dove = FMath::Lerp(Da, A, Alfa);
		Dove.Z = static_cast<double>(FMath::Lerp(ZDa, ZA, Alfa));
		const FVector Verso = A - Da;
		if (Verso.SizeSquared2D() > 25.f)
		{
			Yaw = FMath::FixedTurn(Yaw, static_cast<float>(Verso.Rotation().Yaw), 360.f * DeltaSeconds);
		}
		Quale = -1;
		Passo = Dentro * 9.f;
	}
	else
	{
		Dove = FVector(A.X, A.Y, ZA);
	}
	SetActorLocationAndRotation(Dove, FRotator(0.f, Yaw, 0.f));
	if (GetNetMode() != NM_DedicatedServer)
	{
		Posa(Quale, Passo, Tempo, DeltaSeconds);
	}
}

void AValdorsoGatto::Posa(int32 Quale, float Passo, float Tempo, float DeltaSeconds)
{
	// (la coda va all'indietro: un "pitch" negativo la alza, positivo la abbassa a terra)
	FGattoPosa P;
	switch (Quale)
	{
	case -1: // cammina
		P.Corpo = FVector(0.f, 0.f, 18.f + FMath::Abs(FMath::Sin(Passo)) * 1.5f);
		P.Testa = FRotator(FMath::Sin(Passo * 2.f) * 3.f, 0.f, 0.f);
		P.Coda = FRotator(-35.f + FMath::Sin(Tempo * 3.f) * 8.f, FMath::Sin(Tempo * 2.f) * 15.f, 0.f);
		break;
	case 1: // sdraiato su un fianco
		P.Corpo = FVector(0.f, 0.f, 9.f);
		P.Girato = FRotator(0.f, 0.f, 78.f);
		P.Testa = FRotator(-10.f, 0.f, -45.f + FMath::Sin(Tempo * 0.3f) * 6.f);
		P.Coda = FRotator(10.f, 30.f + FMath::Sin(Tempo * 0.8f) * 6.f, 0.f);
		break;
	case 2: // si lava
		P.Corpo = FVector(0.f, 0.f, 20.f);
		P.Girato = FRotator(28.f, 0.f, 0.f);
		P.Testa = FRotator(-55.f + FMath::Sin(Tempo * 6.f) * 6.f, 0.f, 0.f);
		P.Coda = FRotator(45.f, FMath::Sin(Tempo * 0.7f) * 10.f, 0.f);
		break;
	case 3: // fa le fusa
		P.Corpo = FVector(0.f, 0.f, 20.f);
		P.Girato = FRotator(28.f, 0.f, 0.f);
		P.Testa = FRotator(-15.f + FMath::Sin(Tempo * 1.5f) * 3.f, 0.f, FMath::Sin(Tempo * 0.9f) * 8.f);
		P.Coda = FRotator(30.f, FMath::Sin(Tempo * 6.f) * 20.f, 0.f);
		break;
	default: // seduto, si guarda intorno
		P.Corpo = FVector(0.f, 0.f, 20.f);
		P.Girato = FRotator(28.f, 0.f, 0.f);
		P.Testa = FRotator(-25.f, FMath::Sin(Tempo * 0.4f) * 25.f, 0.f);
		P.Coda = FRotator(45.f, FMath::Sin(Tempo * 0.7f) * 10.f, 0.f);
		break;
	}
	CorpoPos = FMath::VInterpTo(CorpoPos, P.Corpo, DeltaSeconds, 5.f);
	CorpoRot = FMath::RInterpTo(CorpoRot, P.Girato, DeltaSeconds, 4.f);
	Corpo->SetRelativeLocationAndRotation(CorpoPos, CorpoRot);
	Testa->SetRelativeRotation(FMath::RInterpTo(Testa->GetRelativeRotation(), P.Testa, DeltaSeconds, 6.f));
	Coda->SetRelativeRotation(FMath::RInterpTo(Coda->GetRelativeRotation(), P.Coda, DeltaSeconds, 6.f));
}

FText AValdorsoGatto::TestoInterazione(const APawn* Chi) const
{
	return LOCTEXT("Accarezza", "Accarezza il gatto");
}

void AValdorsoGatto::MulticastFusa_Implementation(float Verso, double Fino)
{
	FusaVerso = FMath::IsNearlyZero(Verso) ? 0.01f : Verso;
	FusaFino = Fino;
	if (GetNetMode() != NM_DedicatedServer)
	{
		ValdorsoCombattimento::Suona(GetWorld(), ValdorsoCombattimento::SuonoPericoli(TEXT("S_Gatto_Fusa")), GetActorLocation(), 700.f, 0.8f);
	}
}

void AValdorsoGatto::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi->GetController());
	if (!Giocatore)
	{
		return;
	}
	const float Verso = static_cast<float>((Chi->GetActorLocation() - GetActorLocation()).Rotation().Yaw);
	MulticastFusa(Verso, Orologio() + 6.0);

	const UWorld* Mondo = GetWorld();
	const double Ora = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	const APlayerState* StatoColono = Chi->GetPlayerState();
	const FString Nome = StatoColono ? StatoColono->GetPlayerName() : Chi->GetName();
	if (const double* Ultimo = UltimoIndizio.Find(Nome); Ultimo && Ora - *Ultimo < ValdorsoGatto::SecondiTraIndizi)
	{
		Giocatore->ClientAvviso(LOCTEXT("SoloFusa", "Il gatto fa le fusa e chiude gli occhi."), 3.f, 0.f);
		return;
	}
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Giocatore);
	const FName Memoria(ValdorsoGatto::MemoriaGatto);
	TSet<FName> Saputi;
	for (const FValdorsoIndizioGatto& I : ValdorsoGatto::Indizi())
	{
		if (Taccuino && Taccuino->HaSegno(Memoria, FName(I.Id)))
		{
			Saputi.Add(FName(I.Id));
		}
	}
	const FValdorsoIndizioGatto* Nuovo = ValdorsoGatto::Prossimo(Saputi, ValdorsoVoci::Atto(this));
	if (!Nuovo)
	{
		Giocatore->ClientAvviso(LOCTEXT("TuttoDetto", "Il gatto fa le fusa. Quello che sapeva, te l'ha gia' detto."), 3.5f, 0.f);
		return;
	}
	UltimoIndizio.Add(Nome, Ora);
	Giocatore->ClientAvviso(FText::FromString(Nuovo->Testo), 9.f, 0.f);
	if (Taccuino)
	{
		Taccuino->MettiSegno(Memoria, FName(Nuovo->Id));
		Saputi.Add(FName(Nuovo->Id));
		// la pagina del taccuino: tutto quello che il gatto ha "detto", nell'ordine della tabella
		TArray<FString> Righe;
		for (const FValdorsoIndizioGatto& I : ValdorsoGatto::Indizi())
		{
			if (Saputi.Contains(FName(I.Id)))
			{
				Righe.Add(I.Testo);
			}
		}
		Taccuino->AnnotaIscrizione(TEXT("Il gatto della locanda"), Righe);
	}
	ValdorsoRegistroEventi::Annota(TEXT("Gatto"), Chi, this, GetActorLocation(), Nuovo->Id);
}

#undef LOCTEXT_NAMESPACE
