// Valdorso - Il tempo della valle (vedi il .h). Scritto da Claude il 06/11/2026.

#include "ValdorsoTempo.h"
#include "Valdorso.h"
#include "ValdorsoCielo.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectIterator.h"

namespace ValdorsoTempo
{
	FValdorsoAria Aria(EValdorsoTempo Tempo)
	{
		FValdorsoAria A;
		switch (Tempo)
		{
		case EValdorsoTempo::Nuvoloso:  A.Nuvole = 0.7f; A.Nebbia = 0.15f; break;
		case EValdorsoTempo::Nebbia:    A.Nuvole = 0.5f; A.Nebbia = 1.0f;  break;
		case EValdorsoTempo::Pioggia:   A.Nuvole = 0.9f; A.Nebbia = 0.35f; A.Pioggia = 0.7f; break;
		case EValdorsoTempo::Temporale: A.Nuvole = 1.0f; A.Nebbia = 0.45f; A.Pioggia = 1.0f; break;
		default: break;
		}
		return A;
	}

	FValdorsoAria Mescola(const FValdorsoAria& A, const FValdorsoAria& B, float Alfa)
	{
		const float T = FMath::Clamp(Alfa, 0.f, 1.f);
		FValdorsoAria M;
		M.Nuvole = FMath::Lerp(A.Nuvole, B.Nuvole, T);
		M.Nebbia = FMath::Lerp(A.Nebbia, B.Nebbia, T);
		M.Pioggia = FMath::Lerp(A.Pioggia, B.Pioggia, T);
		return M;
	}

	EValdorsoTempo Prossimo(EValdorsoTempo Adesso, float Caso, float Ora)
	{
		// Le probabilità del tempo dopo (sereno, nuvoloso, nebbia, pioggia, temporale), secondo quello di adesso.
		float P[5];
		switch (Adesso)
		{
		case EValdorsoTempo::Sereno:    P[0] = 0.55f; P[1] = 0.30f; P[2] = 0.08f; P[3] = 0.06f; P[4] = 0.01f; break;
		case EValdorsoTempo::Nuvoloso:  P[0] = 0.35f; P[1] = 0.25f; P[2] = 0.10f; P[3] = 0.24f; P[4] = 0.06f; break;
		case EValdorsoTempo::Nebbia:    P[0] = 0.40f; P[1] = 0.40f; P[2] = 0.15f; P[3] = 0.05f; P[4] = 0.00f; break;
		case EValdorsoTempo::Pioggia:   P[0] = 0.15f; P[1] = 0.45f; P[2] = 0.10f; P[3] = 0.20f; P[4] = 0.10f; break;
		default:                        P[0] = 0.05f; P[1] = 0.30f; P[2] = 0.05f; P[3] = 0.55f; P[4] = 0.05f; break;
		}
		// All'alba (4-8) la nebbia viene il doppio più spesso; di giorno pieno la metà.
		if (Ora >= 4.f && Ora < 8.f)
		{
			P[2] *= 2.f;
		}
		else if (Ora >= 11.f && Ora < 17.f)
		{
			P[2] *= 0.5f;
		}
		float Totale = 0.f;
		for (float V : P)
		{
			Totale += V;
		}
		float Soglia = FMath::Clamp(Caso, 0.f, 0.9999f) * Totale;
		int32 Ultimo = 0;
		for (int32 i = 0; i < 5; ++i)
		{
			if (P[i] <= 0.f)
			{
				continue;
			}
			Ultimo = i;
			if (Soglia < P[i])
			{
				return static_cast<EValdorsoTempo>(i);
			}
			Soglia -= P[i];
		}
		// (gli arrotondamenti possono lasciare un avanzo minuscolo: vale l'ultimo tempo possibile)
		return static_cast<EValdorsoTempo>(Ultimo);
	}

	float LuceSole(const FValdorsoAria& A)
	{
		// (06/11) Con la pioggia il sole quasi sparisce: resta la luce del cielo, senza ombre nette.
		return FMath::Clamp(1.f - 0.75f * A.Nuvole - 0.35f * A.Pioggia - 0.15f * A.Nebbia, 0.05f, 1.f);
	}

	FValdorsoOmbreNuvole OmbreNuvole(const FValdorsoAria& A)
	{
		FValdorsoOmbreNuvole O;
		const float Copertura = FMath::Clamp(0.22f + 0.6f * A.Nuvole, 0.f, 0.95f);   // sereno 22%, nuvoloso 64%, pioggia 76%
		O.Soglia = 1.f - Copertura;
		O.Morbidezza = FMath::Clamp(0.08f + 0.35f * A.Pioggia + 0.1f * A.Nebbia, 0.02f, 1.f);
		O.LuceSotto = FMath::Clamp(0.35f + 0.3f * A.Pioggia + 0.2f * A.Nebbia, 0.f, 1.f);
		return O;
	}

	float Cupo(const FValdorsoAria& A)
	{
		return FMath::Clamp(0.35f * A.Nuvole + 0.65f * A.Pioggia + 0.15f * A.Nebbia, 0.f, 1.f);
	}

	float NebbiaVicina(float DiSerie, const FValdorsoAria& A)
	{
		// Densità di Unreal (FogDensity / 1000 per cm, in base 2): 0,3 copre quasi tutto a 100 m, 0,14 a 300 m.
		return FMath::Max(0.f, DiSerie) + 0.3f * A.Nebbia + 0.05f * A.Pioggia;
	}

	float DensitaPerAltezza(float Vicina, float Caduta, float SopraNebbia)
	{
		// Unreal: densità alla telecamera = FogDensity * 2^(-Caduta/1000 * (quota telecamera - quota della nebbia)).
		const float Esponente = FMath::Clamp(Caduta * 0.001f * SopraNebbia, -20.f, 20.f);
		return FMath::Clamp(Vicina * FMath::Exp2(Esponente), 0.f, 10.f);
	}

	FString Nome(EValdorsoTempo Tempo)
	{
		switch (Tempo)
		{
		case EValdorsoTempo::Nuvoloso:  return TEXT("Nuvoloso");
		case EValdorsoTempo::Nebbia:    return TEXT("Nebbia");
		case EValdorsoTempo::Pioggia:   return TEXT("Pioggia");
		case EValdorsoTempo::Temporale: return TEXT("Temporale");
		default:                        return TEXT("Sereno");
		}
	}

	bool DaNome(const FString& Testo, EValdorsoTempo& OutTempo)
	{
		const FString T = Testo.TrimStartAndEnd();
		if (T.IsNumeric())
		{
			const int32 N = FCString::Atoi(*T);
			if (N >= 0 && N <= 4)
			{
				OutTempo = static_cast<EValdorsoTempo>(N);
				return true;
			}
			return false;
		}
		for (int32 i = 0; i <= 4; ++i)
		{
			if (Nome(static_cast<EValdorsoTempo>(i)).Equals(T, ESearchCase::IgnoreCase))
			{
				OutTempo = static_cast<EValdorsoTempo>(i);
				return true;
			}
		}
		return false;
	}
}

namespace
{
	FAutoConsoleCommand ComandoTempoValle(
		TEXT("Valdorso.Tempo"),
		TEXT("Cambia il tempo della valle: Valdorso.Tempo Pioggia (o Sereno, Nuvoloso, Nebbia, Temporale, 0-4). Senza niente dice il tempo di adesso."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti)
		{
			for (TObjectIterator<AValdorsoTempo> It; It; ++It)
			{
				if (!IsValid(*It) || !It->GetWorld() || !It->GetWorld()->IsGameWorld())
				{
					continue;
				}
				EValdorsoTempo Nuovo;
				if (Argomenti.Num() > 0 && It->HasAuthority() && ValdorsoTempo::DaNome(Argomenti[0], Nuovo))
				{
					It->ImpostaTempo(Nuovo);
				}
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Tempo (%s): %s"), It->HasAuthority() ? TEXT("server") : TEXT("PC"),
					*ValdorsoTempo::Nome(It->GetTempo()));
			}
		}));

	const TCHAR* PercorsoMaterialePioggia = TEXT("/Game/Valle/Tempo/M_Pioggia.M_Pioggia");
	const TCHAR* PercorsoSuonoPioggia = TEXT("/Game/Valle/Tempo/S_Pioggia.S_Pioggia");
	const TCHAR* PercorsoCilindro = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	// (il .glb importato finisce in una cartella sua, come i pezzi del villaggio)
	const TCHAR* PercorsoTenda = TEXT("/Game/Valle/Tempo/SM_TendaPioggia/StaticMeshes/SM_TendaPioggia.SM_TendaPioggia");
	const TCHAR* PercorsoTendaPiatta = TEXT("/Game/Valle/Tempo/SM_TendaPioggia.SM_TendaPioggia");

	// Le tende: raggio (cm), quanto si vedono le gocce e quanto sono piccole (Scala del materiale).
	constexpr int32 NumeroTende = 6;
	const float RaggiTende[NumeroTende] = { 150.f, 300.f, 520.f, 850.f, 1400.f, 2600.f };
	const float ForzaTende[NumeroTende] = { 0.45f, 0.8f, 0.8f, 0.75f, 0.7f, 0.55f };
	const float ScalaTende[NumeroTende] = { 0.5f, 0.6f, 0.9f, 1.4f, 2.2f, 3.5f };

	UStaticMeshComponent* TendaDiGocce(AActor* Proprietario, const TCHAR* Nome, USceneComponent* Radice)
	{
		UStaticMeshComponent* C = Proprietario->CreateDefaultSubobject<UStaticMeshComponent>(Nome);
		C->SetupAttachment(Radice);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetGenerateOverlapEvents(false);
		C->SetVisibility(false);
		C->bReceivesDecals = false;
		return C;
	}
}

AValdorsoTempo::AValdorsoTempo()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));

	// (06/11) Sei tende di gocce intorno alla telecamera, da 1,5 m a 26 m: così piove anche tra la telecamera e il
	// colono (con la tenda vicina a 6 m il colono stava in una bolla asciutta). Girate in modo diverso, così le gocce
	// delle tende non si allineano.
	for (int32 i = 0; i < NumeroTende; ++i)
	{
		UStaticMeshComponent* C = TendaDiGocce(this, *FString::Printf(TEXT("Gocce%d"), i), RootComponent);
		const float Scala = RaggiTende[i] / 50.f;                 // la tenda è larga 1 m
		C->SetRelativeScale3D(FVector(Scala, Scala, i + 1 < NumeroTende ? 16.f : 40.f));
		C->SetRelativeRotation(FRotator(0.f, i * 47.f, 0.f));
		Tende.Add(C);
	}

	SuonoPioggia = CreateDefaultSubobject<UAudioComponent>(TEXT("SuonoPioggia"));
	SuonoPioggia->SetupAttachment(RootComponent);
	SuonoPioggia->bAutoActivate = false;
	SuonoPioggia->bAllowSpatialization = false;     // la pioggia è tutto intorno
	SuonoPioggia->bIsUISound = false;

	Cupezza = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Cupezza"));
	Cupezza->SetupAttachment(RootComponent);
	Cupezza->bUnbound = true;
	Cupezza->Priority = 50.f;
	Cupezza->BlendWeight = 0.f;
	Cupezza->Settings.bOverride_AutoExposureBias = true;
	Cupezza->Settings.AutoExposureBias = -1.8f;
	Cupezza->Settings.bOverride_ColorSaturation = true;
	Cupezza->Settings.ColorSaturation = FVector4(0.6f, 0.6f, 0.6f, 1.f);
}

void AValdorsoTempo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoTempo, Tempo);
	DOREPLIFETIME(AValdorsoTempo, TempoPrima);
	DOREPLIFETIME(AValdorsoTempo, InizioCambio);
}

double AValdorsoTempo::TempoServer() const
{
	const UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return 0.0;
	}
	const AGameStateBase* Stato = Mondo->GetGameState();
	return Stato ? Stato->GetServerWorldTimeSeconds() : Mondo->GetTimeSeconds();
}

AValdorsoTempo* AValdorsoTempo::Di(const UWorld* Mondo)
{
	if (!Mondo)
	{
		return nullptr;
	}
	for (TActorIterator<AValdorsoTempo> It(const_cast<UWorld*>(Mondo)); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void AValdorsoTempo::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Tempo = TempoIniziale;
		TempoPrima = TempoIniziale;
		InizioCambio = TempoServer() - SecondiCambio;
		ProssimoCambio = TempoServer() + FMath::FRandRange(MinutiMinimi, FMath::Max(MinutiMinimi, MinutiMassimi)) * 60.0;
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		// la tenda senza tappi (crea_tempo.py); se non c'è, il cilindro di Unreal (con i tappi: un disco di gocce in alto)
		UStaticMesh* Cilindro = LoadObject<UStaticMesh>(nullptr, PercorsoTenda, nullptr, LOAD_NoWarn);
		if (!Cilindro)
		{
			Cilindro = LoadObject<UStaticMesh>(nullptr, PercorsoTendaPiatta, nullptr, LOAD_NoWarn);
		}
		if (!Cilindro)
		{
			Cilindro = LoadObject<UStaticMesh>(nullptr, PercorsoCilindro, nullptr, LOAD_NoWarn);
		}
		UMaterialInterface* Materiale = LoadObject<UMaterialInterface>(nullptr, PercorsoMaterialePioggia, nullptr, LOAD_NoWarn);
		if (Cilindro && Materiale)
		{
			for (int32 i = 0; i < Tende.Num(); ++i)
			{
				UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Materiale, this);
				Tende[i]->SetStaticMesh(Cilindro);
				Tende[i]->SetMaterial(0, M);
				MaterialiTende.Add(M);
			}
		}
		else
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Tempo: manca M_Pioggia (lanciare Content/Python/nuovi/crea_tempo.py): la pioggia non si vede"));
		}
		if (USoundBase* Suono = LoadObject<USoundBase>(nullptr, PercorsoSuonoPioggia, nullptr, LOAD_NoWarn))
		{
			SuonoPioggia->SetSound(Suono);
		}
	}
	else
	{
		// Il server dedicato non disegna: gli basta decidere il tempo, una volta al secondo.
		SetActorTickInterval(1.f);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Tempo: %s"), *ValdorsoTempo::Nome(Tempo));
}

void AValdorsoTempo::ImpostaTempo(EValdorsoTempo Nuovo)
{
	if (!HasAuthority())
	{
		return;
	}
	TempoPrima = Tempo;
	Tempo = Nuovo;
	InizioCambio = TempoServer();
	ProssimoCambio = TempoServer() + FMath::FRandRange(MinutiMinimi, FMath::Max(MinutiMinimi, MinutiMassimi)) * 60.0;
	ForceNetUpdate();
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Tempo: da %s a %s"), *ValdorsoTempo::Nome(TempoPrima), *ValdorsoTempo::Nome(Tempo));
}

FValdorsoAria AValdorsoTempo::AriaAdesso() const
{
	const float Alfa = static_cast<float>((TempoServer() - InizioCambio) / FMath::Max(1.f, SecondiCambio));
	// sfumatura morbida (piano all'inizio e alla fine)
	const float Morbida = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alfa, 0.f, 1.f));
	return ValdorsoTempo::Mescola(ValdorsoTempo::Aria(TempoPrima), ValdorsoTempo::Aria(Tempo), Morbida);
}

float AValdorsoTempo::LuceSole() const
{
	return ValdorsoTempo::LuceSole(AriaAdesso());
}

void AValdorsoTempo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Adesso = TempoServer();
	if (HasAuthority() && Adesso >= ProssimoCambio)
	{
		const AValdorsoCielo* CieloOra = nullptr;
		for (TActorIterator<AValdorsoCielo> It(GetWorld()); It; ++It)
		{
			CieloOra = *It;
			break;
		}
		ImpostaTempo(ValdorsoTempo::Prossimo(Tempo, FMath::FRand(), CieloOra ? CieloOra->Ora() : 12.f));
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const FValdorsoAria A = AriaAdesso();

	// Il cielo cupo (esposizione più bassa e colori spenti): vale anche sotto un tetto.
	Cupezza->BlendWeight = ValdorsoTempo::Cupo(A);

	APlayerController* Giocatore = GetWorld()->GetFirstPlayerController();
	const APlayerCameraManager* Camera = Giocatore ? Giocatore->PlayerCameraManager : nullptr;
	if (!Camera)
	{
		return;
	}
	const FVector Occhio = Camera->GetCameraLocation();
	const double Ora = GetWorld()->GetTimeSeconds();

	// La nebbia del livello (si cerca finché non c'è: con World Partition può arrivare dopo).
	// Se dopo 10 secondi non c'è, la mette il tempo: leggerissima col sereno, come se non ci fosse.
	if (!Nebbia.IsValid() && Ora >= ProssimaRicercaNebbia)
	{
		ProssimaRicercaNebbia = Ora + 3.0;
		for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
		{
			Nebbia = It->GetComponent();
			break;
		}
		if (!Nebbia.IsValid() && !bNebbiaMessa && Ora > 10.0)
		{
			bNebbiaMessa = true;
			FActorSpawnParameters Parametri;
			Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AExponentialHeightFog* Nuova = GetWorld()->SpawnActor<AExponentialHeightFog>(Occhio - FVector(0.f, 0.f, 3000.f), FRotator::ZeroRotator, Parametri))
			{
				Nuova->GetComponent()->SetFogDensity(0.004f);
				Nebbia = Nuova->GetComponent();
				UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Tempo: il livello non ha la nebbia (Exponential Height Fog), la metto io"));
			}
		}
		if (UExponentialHeightFogComponent* N = Nebbia.Get())
		{
			DensitaNebbia = N->FogDensity;
			CadutaNebbia = N->FogHeightFalloff;
			ColoreNebbia = N->FogInscatteringLuminance;
		}
	}
	if (UExponentialHeightFogComponent* N = Nebbia.Get())
	{
		// (06/11) La nebbia del livello cala con l'altezza: nella valle, molto sopra l'attore della nebbia, non si vedeva.
		// Si guarda quanto è fitta di serie dove sta la telecamera, quanto deve esserlo con questo tempo, e si mette
		// il valore che la fa così. Col sereno torna esattamente quella del livello.
		const float SopraNebbia = static_cast<float>(Occhio.Z - N->GetComponentLocation().Z);
		const float DiSerie = ValdorsoTempo::DensitaPerAltezza(DensitaNebbia, CadutaNebbia, -SopraNebbia);
		const float Peso = FMath::Clamp(1.5f * A.Nebbia + 0.5f * A.Pioggia, 0.f, 1.f);
		// con la nebbia fitta la caduta con l'altezza quasi sparisce: la nebbia riempie la valle, anche sui pendii
		const float Caduta = FMath::Lerp(CadutaNebbia, 0.02f, Peso);
		N->SetFogHeightFalloff(Caduta);
		N->SetFogDensity(ValdorsoTempo::DensitaPerAltezza(ValdorsoTempo::NebbiaVicina(DiSerie, A), Caduta, SopraNebbia));
		// il colore: più grigio quando è cupo; di notte la nebbia fitta si scurisce invece di brillare
		if (!Cielo.IsValid() && Ora >= ProssimaRicercaCielo)
		{
			ProssimaRicercaCielo = Ora + 2.0;
			for (TActorIterator<AValdorsoCielo> It(GetWorld()); It; ++It)
			{
				Cielo = *It;
				break;
			}
		}
		const float Giorno = Cielo.IsValid() ? Cielo->LuceDelGiorno() : 1.f;
		const float Grigio = ColoreNebbia.GetLuminance();
		FLinearColor Colore = FMath::Lerp(ColoreNebbia, FLinearColor(Grigio, Grigio, Grigio * 1.05f), 0.6f * ValdorsoTempo::Cupo(A));
		Colore *= FMath::Lerp(1.f, 0.08f + 0.92f * Giorno, Peso);
		Colore.A = 1.f;
		N->SetFogInscatteringColor(Colore);
	}

	// La pioggia segue la telecamera del giocatore di questo PC.
	SetActorLocation(Occhio + FVector(0.f, 0.f, 200.f));

	// Sotto un tetto? Una linea verso l'alto, quattro volte al secondo.
	if (A.Pioggia > 0.01f && Ora >= ProssimoControlloTetto)
	{
		ProssimoControlloTetto = Ora + 0.25;
		FHitResult Colpo;
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoTempoTetto), false, Giocatore->GetPawn());
		bTettoSopra = GetWorld()->LineTraceSingleByChannel(Colpo, Occhio, Occhio + FVector(0.f, 0.f, 3000.f), ECC_Visibility, Parametri);
	}
	AlRiparo = FMath::FInterpTo(AlRiparo, bTettoSopra ? 1.f : 0.f, DeltaSeconds, 3.f);

	const float Gocce = A.Pioggia * (1.f - AlRiparo);
	const bool bVisibile = Gocce > 0.01f;
	for (int32 i = 0; i < Tende.Num() && i < NumeroTende; ++i)
	{
		UMaterialInstanceDynamic* M = MaterialiTende.IsValidIndex(i) ? MaterialiTende[i].Get() : nullptr;
		Tende[i]->SetVisibility(bVisibile && M != nullptr);
		if (M && bVisibile)
		{
			M->SetScalarParameterValue(TEXT("Pioggia"), Gocce * ForzaTende[i]);
			M->SetScalarParameterValue(TEXT("Scala"), ScalaTende[i]);
		}
	}

	// Il suono: sotto un tetto si sente più piano (la pioggia sul tetto).
	const float Volume = A.Pioggia * (1.f - 0.55f * AlRiparo);
	if (SuonoPioggia->Sound)
	{
		if (Volume > 0.02f)
		{
			if (!SuonoPioggia->IsPlaying())
			{
				SuonoPioggia->Play();
			}
			SuonoPioggia->SetVolumeMultiplier(Volume);
		}
		else if (SuonoPioggia->IsPlaying())
		{
			SuonoPioggia->Stop();
		}
	}
}
