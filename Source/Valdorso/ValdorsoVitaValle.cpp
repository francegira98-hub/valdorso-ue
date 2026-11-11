// Valdorso - Il cielo e i boschi che vivono (vedi il .h). Scritto da Claude il 10/11/2026.

#include "ValdorsoVitaValle.h"
#include "Valdorso.h"
#include "ValdorsoBosco.h"
#include "ValdorsoCielo.h"
#include "ValdorsoStatoMondo.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoTempo.h"
#include "ValdorsoImpostazioni.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	TAutoConsoleVariable<int32> CVarVitaValle(
		TEXT("Valdorso.VitaValle"),
		1,
		TEXT("Il cielo e i boschi che vivono (stormi, rapace, rondini, pipistrelli, foglie d'autunno): 1 accesi, 0 spenti."),
		ECVF_Default);

	TAutoConsoleVariable<FString> CVarVitaStagione(
		TEXT("Valdorso.VitaValle.Stagione"),
		TEXT(""),
		TEXT("Per provare la vita della valle in un'altra stagione: primavera, estate, autunno, inverno (vuoto = quella vera)."),
		ECVF_Default);

	constexpr int32 VitaPerStormo = 12;
	constexpr int32 VitaRondini = 10;
	constexpr int32 VitaPipistrelli = 6;
	/** Le foglie: quante al massimo (divise nei tre colori). */
	constexpr int32 VitaFoglie = 90;
	constexpr int32 VitaColori = 3;
	/** Da quanto lontano dal colono cadono (cm). */
	constexpr float VitaRaggioFoglie = 1800.f;
	/** Quanto resta a terra una foglia prima di sparire (s). */
	constexpr double VitaFogliaATerra = 20.0;
	const FName VitaColore(TEXT("Color"));
	TAutoConsoleVariable<int32> CVarVitaDrago(
		TEXT("Valdorso.VitaValle.Drago"),
		0,
		TEXT("Per provare: 1 = il drago passa adesso sopra i monti a nord (poi torna a 0 da solo)."),
		ECVF_Default);

	/** Il drago: quanto vola (s), quanto veloce (cm/s), quanto lontano a nord e quanto in alto sul colono (cm). */
	constexpr float VitaDragoSecondi = 85.f;
	constexpr float VitaDragoVelocita = 3500.f;
	constexpr float VitaDragoNord = 80000.f;
	constexpr float VitaDragoAlto = 45000.f;
	/** L'uccello della valle e' un passero di 35 cm: un drago di una trentina di metri. */
	constexpr float VitaDragoScala = 90.f;
	constexpr int32 VitaFarfalle = 14;
	constexpr int32 VitaLucciole = 40;
	constexpr int32 VitaSbuffi = 128;
	/** Quanti comignoli fumano al massimo (i piu' vicini) e fin dove si cercano (cm). */
	constexpr int32 VitaComignoliMax = 8;
	constexpr float VitaRaggioComignoli = 6000.f;
	/** Il vento che piega il fumo (cm/s). */
	const FVector VitaVento(45.f, 18.f, 0.f);

	void VitaPrepara(UInstancedStaticMeshComponent* C, USceneComponent* Radice)
	{
		C->SetupAttachment(Radice);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->SetCastShadow(false);
		C->SetAffectDistanceFieldLighting(false);
		C->SetAffectDynamicIndirectLighting(false);
		C->SetCanEverAffectNavigation(false);
	}

	/** Le case del villaggio (le mesh fatte da villaggio_edifici.py hanno il comignolo). */
	bool VitaUnaCasa(const FString& NomeMesh)
	{
		static const TCHAR* const Case[] = { TEXT("SM_Casa"), TEXT("SM_Locanda"), TEXT("SM_Fucina"), TEXT("SM_Caserma"),
			TEXT("SM_Fattoria"), TEXT("SM_Palude_Capanna"), TEXT("SM_Villaggio_Capanna"), TEXT("SM_Canonica"), TEXT("SM_Gilda"),
			TEXT("SM_Falegname"), TEXT("SM_Conceria"), TEXT("SM_Mulino"), TEXT("SM_Casermetta") };
		for (const TCHAR* Una : Case)
		{
			if (NomeMesh.StartsWith(Una))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * La cima del comignolo di una casa, nel mondo (ZeroVector se non ce l'ha). Il comignolo e' la cosa piu' alta della casa,
	 * a 150 cm dal bordo del tetto (a destra o a sinistra): si prova li' con linee dall'alto, lungo tutta la profondita'.
	 */
	FVector VitaCimaDelComignolo(UStaticMeshComponent* Pezzo, const UStaticMesh* Forma)
	{
		const FBox Scatola = Forma->GetBoundingBox();
		if (!Scatola.IsValid || Scatola.GetSize().Z < 200.f)
		{
			return FVector::ZeroVector;
		}
		const FTransform Mondo = Pezzo->GetComponentTransform();
		const float Xs[2] = { static_cast<float>(Scatola.Max.X) - 150.f, static_cast<float>(Scatola.Min.X) + 150.f };
		const float Cima = static_cast<float>(Scatola.Max.Z);
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoComignoli), true);
		for (const float X : Xs)
		{
			for (float Y = static_cast<float>(Scatola.Min.Y) + 50.f; Y <= static_cast<float>(Scatola.Max.Y) - 50.f; Y += 40.f)
			{
				FHitResult Colpo;
				const FVector Da = Mondo.TransformPosition(FVector(X, Y, Cima + 60.f));
				const FVector A = Mondo.TransformPosition(FVector(X, Y, Cima - 60.f));
				if (Pezzo->LineTraceComponent(Colpo, Da, A, Parametri)
					&& Mondo.InverseTransformPosition(Colpo.ImpactPoint).Z >= Cima - 35.f)
				{
					return Colpo.ImpactPoint + FVector(0.f, 0.f, 15.f);
				}
			}
		}
		return FVector::ZeroVector;
	}
	/** Una istanza "nascosta": piccolissima (un'istanza non si spegne da sola). */
	const FVector VitaNascosta(0.0001f);

	bool VitaVicino(const FVector& Dove, const TCHAR* Luogo, float Quanto)
	{
		const FValdorsoLuogo* L = ValdorsoLuoghi::Trova(Luogo);
		return L && FVector::Dist2D(L->Posizione, Dove) <= L->Raggio * Quanto;
	}
}

namespace ValdorsoVitaValle
{
	bool OraDegliStormi(float Ora)
	{
		return Ora >= 6.5f && Ora <= 18.5f;
	}

	bool OraDelleRondini(float Ora)
	{
		return (Ora >= 5.8f && Ora <= 7.3f) || (Ora >= 17.5f && Ora <= 19.8f);
	}

	bool OraDeiPipistrelli(float Ora, float LuceDelGiorno)
	{
		return (Ora >= 20.3f || Ora <= 5.0f) && LuceDelGiorno < 0.25f;
	}

	bool StagioneDelleFoglie(const FString& Stagione)
	{
		return Stagione.Equals(TEXT("autunno"), ESearchCase::IgnoreCase);
	}

	FIntPoint Cella(const FVector& Punto)
	{
		return FIntPoint(FMath::FloorToInt32(Punto.X / 1000.0), FMath::FloorToInt32(Punto.Y / 1000.0));
	}

	bool StagioneDeiPetali(const FString& Stagione)
	{
		return Stagione.Equals(TEXT("primavera"), ESearchCase::IgnoreCase);
	}

	bool OraDelleFarfalle(float Ora, const FString& Stagione)
	{
		const bool bCaldo = Stagione.Equals(TEXT("primavera"), ESearchCase::IgnoreCase) || Stagione.Equals(TEXT("estate"), ESearchCase::IgnoreCase);
		return bCaldo && Ora >= 9.f && Ora <= 17.5f;
	}

	bool OraDelleLucciole(float Ora, float LuceDelGiorno, const FString& Stagione)
	{
		return Stagione.Equals(TEXT("estate"), ESearchCase::IgnoreCase) && (Ora >= 21.f || Ora <= 4.f) && LuceDelGiorno < 0.2f;
	}

	float QuantoFumo(float Ora, const FString& Stagione)
	{
		const bool bFreddo = Stagione.Equals(TEXT("autunno"), ESearchCase::IgnoreCase) || Stagione.Equals(TEXT("inverno"), ESearchCase::IgnoreCase);
		if ((Ora >= 6.f && Ora <= 9.f) || (Ora >= 17.5f && Ora <= 22.f))
		{
			return 1.f;  // si cucina
		}
		if (Ora > 22.f || Ora < 6.f)
		{
			return bFreddo ? 0.35f : 0.05f;
		}
		return bFreddo ? 0.7f : 0.15f;
	}

	bool NotteDelDrago(const FDateTime& Giorno)
	{
		const uint32 Seme = static_cast<uint32>(Giorno.GetYear() * 1000 + Giorno.GetDayOfYear()) * 2654435761u;
		return (Seme >> 16) % 4u == 0u;
	}

	bool OraDelDrago(float Ora)
	{
		return Ora >= 20.f && Ora < 20.5f;
	}

	FString StagioneAdesso()
	{
		const FString Voluta = CVarVitaStagione.GetValueOnGameThread().TrimStartAndEnd().ToLower();
		if (Voluta == TEXT("primavera") || Voluta == TEXT("estate") || Voluta == TEXT("autunno") || Voluta == TEXT("inverno"))
		{
			return Voluta;
		}
		return ValdorsoStatoMondo::StagioneDi(FDateTime::Now());
	}
}

AValdorsoVitaValle::AValdorsoVitaValle()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));

	Volatori = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Volatori"));
	Volatori->SetupAttachment(RootComponent);
	Volatori->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Volatori->SetGenerateOverlapEvents(false);
	Volatori->SetCastShadow(false);
	Volatori->SetNumCustomDataFloats(1);           // come la valle viva: 1 = ali che battono
	Volatori->SetAffectDistanceFieldLighting(false);
	Volatori->SetAffectDynamicIndirectLighting(false);
	Volatori->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Semplice(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	for (int32 c = 0; c < VitaColori; ++c)
	{
		UInstancedStaticMeshComponent* F = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Foglie%d"), c));
		F->SetupAttachment(RootComponent);
		F->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		F->SetGenerateOverlapEvents(false);
		F->SetCastShadow(false);
		F->SetAffectDistanceFieldLighting(false);
		F->SetAffectDynamicIndirectLighting(false);
		F->SetCanEverAffectNavigation(false);
		if (Cubo.Succeeded())
		{
			F->SetStaticMesh(Cubo.Object);
		}
		if (Semplice.Succeeded())
		{
			F->SetMaterial(0, Semplice.Object);
		}
		Foglie.Add(F);
	}
	// le farfalle: tre colori, cubetti schiacciati che battono le ali (si allargano e si stringono)
	for (int32 c = 0; c < VitaColori; ++c)
	{
		UInstancedStaticMeshComponent* F = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Farfalle%d"), c));
		VitaPrepara(F, RootComponent);
		if (Cubo.Succeeded())
		{
			F->SetStaticMesh(Cubo.Object);
		}
		if (Semplice.Succeeded())
		{
			F->SetMaterial(0, Semplice.Object);
		}
		Farfalle.Add(F);
	}
	// le lucciole e il fumo: la sfera la mette BeginPlay, solo se ci sono i materiali (crea_vita_valle.py)
	Lucciole = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Lucciole"));
	VitaPrepara(Lucciole, RootComponent);
	Lucciole->SetNumCustomDataFloats(1);           // quanto e' accesa
	Fumo = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Fumo"));
	VitaPrepara(Fumo, RootComponent);
	Fumo->SetNumCustomDataFloats(1);               // quanto si vede
	// il drago di Fab: la mesh la mette BeginPlay, se c'e'
	DragoVero = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DragoVero"));
	DragoVero->SetupAttachment(RootComponent);
	DragoVero->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DragoVero->SetGenerateOverlapEvents(false);
	DragoVero->SetCastShadow(false);
	DragoVero->SetCanEverAffectNavigation(false);
	DragoVero->SetVisibility(false);
	DragoVero->SetUsingAbsoluteLocation(true);
	DragoVero->SetUsingAbsoluteRotation(true);
}

void AValdorsoVitaValle::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}
	// l'uccello della valle viva (crea_valle_viva.py): senza, niente uccelli (le foglie si').
	UStaticMesh* Uccello = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Valle/Viva/SM_Uccello.SM_Uccello"), nullptr, LOAD_NoWarn);
	if (Uccello)
	{
		Volatori->SetStaticMesh(Uccello);
	}
	else
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Vita della valle: manca /Game/Valle/Viva/SM_Uccello (crea_valle_viva.py): niente uccelli in volo"));
	}
	auto Aggiungi = [this](EGruppo Gruppo, float Scala, float Raggio, float Battito)
	{
		FVolatore V;
		V.Gruppo = Gruppo;
		V.Scala = Scala * FMath::FRandRange(0.85f, 1.15f);
		V.Raggio = Raggio * FMath::FRandRange(0.6f, 1.f);
		V.Fase = FVector(FMath::FRandRange(0.f, 2.f * PI), FMath::FRandRange(0.f, 2.f * PI), FMath::FRandRange(0.f, 2.f * PI));
		const int32 Indice = Volatori->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, VitaNascosta), true);
		Volatori->SetCustomDataValue(Indice, 0, Battito, false);
		Uccelli.Add(V);
	};
	for (int32 s = 0; s < 2; ++s)
	{
		for (int32 i = 0; i < VitaPerStormo; ++i)
		{
			Aggiungi(s == 0 ? EGruppo::Stormo0 : EGruppo::Stormo1, 1.f, 900.f, 1.f);
		}
	}
	// il rapace plana: le ali battono poco
	Aggiungi(EGruppo::Rapace, 2.4f, 5000.f, 0.25f);
	for (int32 i = 0; i < VitaRondini; ++i)
	{
		Aggiungi(EGruppo::Rondini, 0.6f, 2600.f, 1.f);
	}
	for (int32 i = 0; i < VitaPipistrelli; ++i)
	{
		Aggiungi(EGruppo::Pipistrelli, 0.45f, 1300.f, 1.f);
	}
	// il drago: l'ultimo; le ali battono piano rispetto a quanto e' grande
	Aggiungi(EGruppo::Drago, VitaDragoScala, 0.f, 0.6f);
	if (UStaticMesh* Drago = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Valle/Viva/SM_Drago.SM_Drago"), nullptr, LOAD_NoWarn))
	{
		DragoVero->SetStaticMesh(Drago);
	}
	// (11/11, v0.1.7) il ruggito, lontanissimo (Tools/Generatori/suoni_valle.py: ruggito)
	Ruggito = LoadObject<USoundBase>(nullptr, TEXT("/Game/Valle/Suoni/S_Drago_Ruggito.S_Drago_Ruggito"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	Volatori->MarkRenderStateDirty();

	// le foglie: tre colori (d'autunno le foglie, in primavera i petali), VitaFoglie istanze in tutto (nascoste finche'
	// non cadono)
	for (int32 c = 0; c < Foglie.Num(); ++c)
	{
		UInstancedStaticMeshComponent* F = Foglie[c];
		TinteFoglie.Add(F->GetStaticMesh() ? F->CreateDynamicMaterialInstance(0, F->GetMaterial(0)) : nullptr);
	}
	Stagione = ValdorsoVitaValle::StagioneAdesso();
	ColoraFoglie();
	Cadute.SetNum(VitaFoglie);
	for (int32 k = 0; k < Cadute.Num(); ++k)
	{
		FFoglia& Una = Cadute[k];
		Una.Colore = k % VitaColori;
		Una.Istanza = Foglie.IsValidIndex(Una.Colore) ? Foglie[Una.Colore]->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, VitaNascosta), true) : INDEX_NONE;
	}

	// le farfalle
	const FLinearColor Ali[VitaColori] = { FLinearColor(0.85f, 0.7f, 0.08f), FLinearColor(0.82f, 0.82f, 0.76f), FLinearColor(0.85f, 0.35f, 0.04f) };
	for (int32 c = 0; c < Farfalle.Num(); ++c)
	{
		UInstancedStaticMeshComponent* F = Farfalle[c];
		if (UMaterialInstanceDynamic* Tinta = F->GetStaticMesh() ? F->CreateDynamicMaterialInstance(0, F->GetMaterial(0)) : nullptr)
		{
			Tinta->SetVectorParameterValue(VitaColore, Ali[c]);
		}
	}
	auto Nuovo = [this](bool bFarfalla, int32 Colore, UInstancedStaticMeshComponent* Dove)
	{
		FPiccolo P;
		P.bFarfalla = bFarfalla;
		P.Colore = Colore;
		P.Fase = FVector(FMath::FRandRange(0.f, 2.f * PI), FMath::FRandRange(0.f, 2.f * PI), FMath::FRandRange(0.f, 2.f * PI));
		P.Istanza = Dove->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, VitaNascosta), true);
		Piccoli.Add(P);
		return P.Istanza;
	};
	for (int32 i = 0; i < VitaFarfalle; ++i)
	{
		if (Farfalle.IsValidIndex(i % VitaColori) && Farfalle[i % VitaColori]->GetStaticMesh())
		{
			Nuovo(true, i % VitaColori, Farfalle[i % VitaColori]);
		}
	}
	UStaticMesh* Sfera = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"), nullptr, LOAD_NoWarn);
	UMaterialInterface* Lucciola = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Valle/Viva/M_Lucciola.M_Lucciola"), nullptr, LOAD_NoWarn);
	UMaterialInterface* Nebbiolina = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Valle/Viva/M_Fumo.M_Fumo"), nullptr, LOAD_NoWarn);
	if (Sfera && Lucciola)
	{
		Lucciole->SetStaticMesh(Sfera);
		Lucciole->SetMaterial(0, Lucciola);
		for (int32 i = 0; i < VitaLucciole; ++i)
		{
			const int32 Indice = Nuovo(false, 0, Lucciole);
			Lucciole->SetCustomDataValue(Indice, 0, 0.f, false);
		}
	}
	if (Sfera && Nebbiolina)
	{
		Fumo->SetStaticMesh(Sfera);
		Fumo->SetMaterial(0, Nebbiolina);
		Sbuffi.SetNum(VitaSbuffi);
		for (int32 i = 0; i < Sbuffi.Num(); ++i)
		{
			Fumo->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, VitaNascosta), true);
			Fumo->SetCustomDataValue(i, 0, 0.f, false);
		}
	}
	if (!Lucciola || !Nebbiolina)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Vita della valle: mancano M_Lucciola o M_Fumo in /Game/Valle/Viva (crea_vita_valle.py): niente lucciole o fumo dei camini"));
	}
	Lucciole->MarkRenderStateDirty();
	Fumo->MarkRenderStateDirty();
}

void AValdorsoVitaValle::ColoraFoglie()
{
	StagioneDeiColori = Stagione;
	const bool bPetali = ValdorsoVitaValle::StagioneDeiPetali(Stagione);
	const FLinearColor Autunno[VitaColori] = { FLinearColor(0.55f, 0.32f, 0.05f), FLinearColor(0.45f, 0.08f, 0.03f), FLinearColor(0.24f, 0.13f, 0.05f) };
	const FLinearColor Petali[VitaColori] = { FLinearColor(0.9f, 0.88f, 0.84f), FLinearColor(0.85f, 0.5f, 0.6f), FLinearColor(0.95f, 0.72f, 0.76f) };
	GrandezzaFoglie = bPetali ? 0.55f : 1.f;
	for (int32 c = 0; c < TinteFoglie.Num() && c < VitaColori; ++c)
	{
		if (TinteFoglie[c])
		{
			TinteFoglie[c]->SetVectorParameterValue(VitaColore, bPetali ? Petali[c] : Autunno[c]);
		}
	}
}

void AValdorsoVitaValle::CercaAlberi()
{
	Alberi.Reset();
	for (TActorIterator<AValdorsoBosco> It(GetWorld()); It; ++It)
	{
		const FTransform DoveBosco = It->GetActorTransform();
		for (const FValdorsoSpecieBosco& S : It->Specie)
		{
			// solo gli alberi in piedi con un tronco (non i cespugli, non i tronchi a terra)
			if (S.RaggioTronco <= 0.f || S.bSdraiato)
			{
				continue;
			}
			for (const FTransform& Dove : S.Dove)
			{
				const FVector Albero = (Dove * DoveBosco).GetLocation();
				Alberi.FindOrAdd(ValdorsoVitaValle::Cella(Albero)) = Albero;
			}
		}
	}
	bAlberiCercati = Alberi.Num() > 0;
}

void AValdorsoVitaValle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* Mondo = GetWorld();
	const APlayerController* Giocatore = Mondo ? Mondo->GetFirstPlayerController() : nullptr;
	const APawn* Corpo = Giocatore ? Giocatore->GetPawn() : nullptr;
	const bool bAcceso = CVarVitaValle.GetValueOnGameThread() != 0;
	if (!Corpo || !bAcceso)
	{
		Stormi[0].bInCielo = Stormi[1].bInCielo = false;
		bRapace = false;
		AggiornaUccelli(FVector::ZeroVector, DeltaSeconds, 0.0, false, false, false);
		for (FFoglia& Una : Cadute)
		{
			Una.bViva = false;
		}
		AggiornaFoglie(FVector::ZeroVector, DeltaSeconds, 0.0, false);
		AggiornaPiccoli(FVector::ZeroVector, DeltaSeconds, 0.0, false, false);
		for (FSbuffo& Uno : Sbuffi)
		{
			Uno.bVivo = false;
		}
		AggiornaFumo(FVector::ZeroVector, DeltaSeconds, 0.0, 0.f);
		bDragoInVolo = false;
		DragoVero->SetVisibility(false);
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	const FVector Colono = Corpo->GetActorLocation();
	if ((!Cielo.IsValid() || !Tempo.IsValid()) && Adesso >= ProssimoControllo)
	{
		for (TActorIterator<AValdorsoCielo> It(Mondo); It && !Cielo.IsValid(); ++It)
		{
			Cielo = *It;
		}
		Tempo = AValdorsoTempo::Di(Mondo);
	}
	if (!bAlberiCercati && Adesso >= ProssimaRicercaAlberi)
	{
		ProssimaRicercaAlberi = Adesso + 10.0;
		CercaAlberi();
	}
	const float Ora = Cielo.IsValid() ? Cielo->Ora() : 12.f;
	const float Luce = Cielo.IsValid() ? Cielo->LuceDelGiorno() : 1.f;
	const float Pioggia = Tempo.IsValid() ? Tempo->AriaAdesso().Pioggia : 0.f;
	if (Adesso >= ProssimoControllo)
	{
		ProssimoControllo = Adesso + 1.0;
		Stagione = ValdorsoVitaValle::StagioneAdesso();
		if (Stagione != StagioneDeiColori)
		{
			ColoraFoglie();
		}
		bNelVillaggio = VitaVicino(Colono, TEXT("Piazza del villaggio"), 2.f);
		bVicinoNotte = VitaVicino(Colono, TEXT("Cimitero"), 1.2f) || VitaVicino(Colono, TEXT("Rovine degli Antichi"), 1.f)
			|| VitaVicino(Colono, TEXT("Piazza del villaggio"), 1.5f) || VitaVicino(Colono, TEXT("Tempio del Cuore"), 1.5f)
			|| VitaVicino(Colono, TEXT("Mulino"), 1.2f);
	}
	const bool bStormi = Volatori->GetStaticMesh() && ValdorsoVitaValle::OraDegliStormi(Ora) && Pioggia < 0.5f;
	AggiornaStormo(0, bStormi, Colono, DeltaSeconds, Adesso);
	AggiornaStormo(1, bStormi && Pioggia < 0.2f, Colono, DeltaSeconds, Adesso);
	const bool bRapaceVoluto = bStormi && Pioggia < 0.1f;
	if (bRapaceVoluto)
	{
		CentroRapace = bRapace ? FMath::VInterpTo(CentroRapace, Colono, DeltaSeconds, 0.05f) : Colono + FVector(3000.f, 0.f, 0.f);
	}
	bRapace = bRapaceVoluto;
	AggiornaDrago(Colono, Adesso, Ora, Pioggia);
	const bool bRondini = ValdorsoVitaValle::OraDelleRondini(Ora) && bNelVillaggio && Pioggia < 0.3f;
	const bool bPipistrelli = ValdorsoVitaValle::OraDeiPipistrelli(Ora, Luce) && bVicinoNotte && Pioggia < 0.3f;
	AggiornaUccelli(Colono, DeltaSeconds, Adesso, bRapace, bRondini, bPipistrelli);
	const bool bCadono = ValdorsoVitaValle::StagioneDelleFoglie(Stagione) || ValdorsoVitaValle::StagioneDeiPetali(Stagione);
	AggiornaFoglie(Colono, DeltaSeconds, Adesso, bCadono && Pioggia < 0.8f);
	const bool bConFarfalle = ValdorsoVitaValle::OraDelleFarfalle(Ora, Stagione) && Pioggia < 0.2f;
	const bool bConLucciole = ValdorsoVitaValle::OraDelleLucciole(Ora, Luce, Stagione) && !bNelVillaggio && Pioggia < 0.2f;
	AggiornaPiccoli(Colono, DeltaSeconds, Adesso, bConFarfalle, bConLucciole);
	if (Fumo->GetStaticMesh() && Adesso >= ProssimaRicercaComignoli)
	{
		ProssimaRicercaComignoli = Adesso + 15.0;
		CercaComignoli(Colono);
	}
	AggiornaFumo(Colono, DeltaSeconds, Adesso, ValdorsoVitaValle::QuantoFumo(Ora, Stagione) * (1.f - FMath::Clamp(Pioggia, 0.f, 1.f) * 0.7f));
}

void AValdorsoVitaValle::AggiornaDrago(const FVector& Colono, double Adesso, float Ora, float Pioggia)
{
	const FDateTime Oggi = FDateTime::UtcNow();
	const bool bProva = CVarVitaDrago.GetValueOnGameThread() != 0;
	if (bProva)
	{
		CVarVitaDrago->Set(0, ECVF_SetByConsole);
	}
	if (!bDragoInVolo && (bProva || (Oggi.GetDayOfYear() != GiornoDelDrago && ValdorsoVitaValle::NotteDelDrago(Oggi)
		&& ValdorsoVitaValle::OraDelDrago(Ora) && Pioggia < 0.5f)))
	{
		// (la prova non conta: la notte vera del drago resta)
		if (!bProva)
		{
			GiornoDelDrago = Oggi.GetDayOfYear();
		}
		bDragoInVolo = true;
		DragoInizio = Adesso;
		RuggitiFatti = 0;
		// da ovest a est, alto sopra i monti a nord del colono
		DragoDa = Colono + FVector(-VitaDragoSecondi * VitaDragoVelocita * 0.5f, -VitaDragoNord, VitaDragoAlto);
	}
	if (bDragoInVolo)
	{
		const float Volato = static_cast<float>(Adesso - DragoInizio);
		if (Volato > VitaDragoSecondi || Volato < 0.f)
		{
			bDragoInVolo = false;
		}
		else
		{
			DragoDove = DragoDa + FVector(Volato * VitaDragoVelocita, FMath::Sin(Volato * 0.05f) * 4000.f, FMath::Sin(Volato * 0.11f) * 1500.f);
			// (11/11) due ruggiti per passaggio, a un terzo e a due terzi del volo: piu' forte quanto piu' e' vicino
			const float Momenti[2] = { VitaDragoSecondi * 0.33f, VitaDragoSecondi * 0.7f };
			if (Ruggito && RuggitiFatti < 2 && Volato >= Momenti[RuggitiFatti])
			{
				++RuggitiFatti;
				const float Lontano = FVector::Dist(DragoDove, Colono);
				const float Volume = FMath::GetMappedRangeValueClamped(FVector2D(40000.f, 120000.f), FVector2D(0.9f, 0.35f), Lontano);
				UGameplayStatics::PlaySound2D(this, Ruggito, Volume * ValdorsoImpostazioni::VolumeSuono(false), FMath::FRandRange(0.92f, 1.05f));
			}
		}
	}
	const bool bVero = bDragoInVolo && DragoVero->GetStaticMesh() != nullptr;
	DragoVero->SetVisibility(bVero);
	if (bVero)
	{
		DragoVero->SetWorldLocationAndRotation(DragoDove, FRotator::ZeroRotator);
	}
}

void AValdorsoVitaValle::CercaComignoli(const FVector& Colono)
{
	Comignoli.Reset();
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AStaticMeshActor* Casa = *It;
		if (FVector::DistSquared2D(Casa->GetActorLocation(), Colono) > FMath::Square(VitaRaggioComignoli))
		{
			continue;
		}
		const TWeakObjectPtr<AActor> CasaVista(Casa);
		if (const FVector* Gia = CaseGuardate.Find(CasaVista))
		{
			if (!Gia->IsZero())
			{
				Comignoli.Add(*Gia);
			}
			continue;
		}
		FVector Cima = FVector::ZeroVector;
		UStaticMeshComponent* Pezzo = Casa->GetStaticMeshComponent();
		const UStaticMesh* Forma = Pezzo ? Pezzo->GetStaticMesh() : nullptr;
		if (!Forma)
		{
			continue;  // non ancora pronta: la riguardo la prossima volta
		}
		if (VitaUnaCasa(Forma->GetName()))
		{
			if (!Pezzo->GetBodySetup())
			{
				continue;  // la collisione non e' ancora pronta
			}
			Cima = VitaCimaDelComignolo(Pezzo, Forma);
		}
		CaseGuardate.Add(CasaVista, Cima);
		if (!Cima.IsZero())
		{
			Comignoli.Add(Cima);
		}
	}
	// i piu' vicini
	Comignoli.Sort([&Colono](const FVector& Uno, const FVector& Altro) { return FVector::DistSquared2D(Uno, Colono) < FVector::DistSquared2D(Altro, Colono); });
	if (Comignoli.Num() > VitaComignoliMax)
	{
		Comignoli.SetNum(VitaComignoliMax);
	}
}

void AValdorsoVitaValle::AggiornaFumo(const FVector& Colono, float Delta, double Adesso, float Quanto)
{
	if (!Fumo->GetStaticMesh() || Sbuffi.Num() == 0)
	{
		return;
	}
	// uno sbuffo nuovo ogni tanto, a turno da ogni comignolo
	if (!Colono.IsZero() && Quanto > 0.02f && Comignoli.Num() > 0 && Adesso >= ProssimoSbuffo)
	{
		ProssimoSbuffo = Adesso + FMath::Max(0.05, 0.7 / (static_cast<double>(Quanto) * Comignoli.Num()));
		ProssimoComignolo = (ProssimoComignolo + 1) % Comignoli.Num();
		if (FSbuffo* Libero = Sbuffi.FindByPredicate([](const FSbuffo& S) { return !S.bVivo; }))
		{
			const FVector2D Poco = FMath::RandPointInCircle(12.f);
			Libero->Posto = Comignoli[ProssimoComignolo] + FVector(Poco.X, Poco.Y, 0.f);
			Libero->Nato = Adesso;
			Libero->Durata = FMath::FRandRange(6.f, 9.f);
			Libero->Grande = FMath::FRandRange(0.8f, 1.2f);
			Libero->bVivo = true;
		}
	}
	bool bCambiato = false;
	for (int32 i = 0; i < Sbuffi.Num(); ++i)
	{
		FSbuffo& S = Sbuffi[i];
		const float Eta = S.bVivo ? static_cast<float>((Adesso - S.Nato) / S.Durata) : 1.f;
		if (Eta >= 1.f || Eta < 0.f)
		{
			S.bVivo = false;
			if (!S.bNascosto)
			{
				S.bNascosto = true;
				Fumo->UpdateInstanceTransform(i, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, VitaNascosta), true, false, true);
				Fumo->SetCustomDataValue(i, 0, 0.f, false);
				bCambiato = true;
			}
			continue;
		}
		// sale (sempre piu' piano), il vento lo porta (sempre di piu'), si allarga e svanisce
		S.Posto += (FVector(0.f, 0.f, 85.f * (1.f - Eta * 0.7f)) + VitaVento * (0.3f + Eta)) * Delta;
		const float Misura = (0.35f + 1.9f * Eta) * S.Grande;
		const float Visto = FMath::Min(1.f, Eta * 6.f) * (1.f - Eta);
		Fumo->UpdateInstanceTransform(i, FTransform(FRotator(0.f, static_cast<float>(i) * 37.f, 0.f), S.Posto, FVector(Misura)), true, false, true);
		Fumo->SetCustomDataValue(i, 0, Visto, false);
		S.bNascosto = false;
		bCambiato = true;
	}
	if (bCambiato)
	{
		Fumo->MarkRenderStateDirty();
	}
}

void AValdorsoVitaValle::AggiornaPiccoli(const FVector& Colono, float Delta, double Adesso, bool bConFarfalle, bool bConLucciole)
{
	const float T = static_cast<float>(Adesso);
	const APlayerController* Giocatore = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	bool bCambiate[VitaColori] = { false, false, false };
	bool bAccese = false;
	for (FPiccolo& P : Piccoli)
	{
		UInstancedStaticMeshComponent* Dove = P.bFarfalla ? (Farfalle.IsValidIndex(P.Colore) ? Farfalle[P.Colore].Get() : nullptr) : Lucciole.Get();
		if (!Dove || !Dove->GetStaticMesh() || P.Istanza == INDEX_NONE)
		{
			continue;
		}
		const bool bVoluto = !Colono.IsZero() && (P.bFarfalla ? bConFarfalle : bConLucciole);
		P.Presenza = Colono.IsZero() ? 0.f : FMath::FInterpConstantTo(P.Presenza, bVoluto ? 1.f : 0.f, Delta, 0.5f);
		const float Raggio = P.bFarfalla ? 900.f : 1500.f;
		const bool bLontana = P.bPosto && FVector::Dist2D(P.Ancora, Colono) > Raggio * 1.6f;
		if (P.Presenza > 0.f && (Adesso >= P.CambiaAncora || bLontana))
		{
			// un posto nuovo vicino al colono, sul terreno
			const FVector2D Disco = FMath::RandPointInCircle(Raggio);
			const FVector Sopra = Colono + FVector(Disco.X, Disco.Y, 3000.f);
			FHitResult Colpo;
			FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoPiccoli), false);
			if (Giocatore && Giocatore->GetPawn())
			{
				Parametri.AddIgnoredActor(Giocatore->GetPawn());
			}
			P.CambiaAncora = Adesso + (P.bFarfalla ? FMath::FRandRange(6.0, 12.0) : FMath::FRandRange(10.0, 20.0));
			if (GetWorld()->LineTraceSingleByChannel(Colpo, Sopra, Sopra - FVector(0.f, 0.f, 8000.f), ECC_Visibility, Parametri))
			{
				P.Meta = Colpo.ImpactPoint;
				// la prima volta (o se il colono e' andato lontano) ci va subito: e' ancora quasi invisibile o fuori vista
				if (!P.bPosto || P.Presenza < 0.05f || bLontana)
				{
					P.Ancora = P.Meta;
				}
				P.bPosto = true;
			}
		}
		if (P.bPosto)
		{
			P.Ancora = FMath::VInterpConstantTo(P.Ancora, P.Meta, Delta, P.bFarfalla ? 120.f : 40.f);
		}
		const bool bVisibile = P.bPosto && P.Presenza > 0.01f;
		if (!bVisibile && !P.bVisibile)
		{
			continue;
		}
		P.bVisibile = bVisibile;
		FTransform Forma(FRotator::ZeroRotator, FVector::ZeroVector, VitaNascosta);
		float Luce = 0.f;
		if (bVisibile)
		{
			if (P.bFarfalla)
			{
				auto Volo = [&P](float t)
				{
					return P.Ancora + FVector(FMath::Sin(t * 0.9f + P.Fase.X) * 180.f, FMath::Cos(t * 0.7f + P.Fase.Y) * 180.f,
						70.f + FMath::Sin(t * 1.6f + P.Fase.Z) * 40.f + FMath::Abs(FMath::Sin(t * 3.f + P.Fase.X)) * 30.f);
				};
				const FVector Qui = Volo(T);
				const FVector Passo = Volo(T + 0.1f) - Qui;
				const float Battito = 0.25f + 0.75f * FMath::Abs(FMath::Cos(T * 15.f + P.Fase.X * 5.f));
				Forma = FTransform(FRotator(0.f, Passo.Rotation().Yaw, 0.f), Qui, FVector(0.035f, 0.09f * Battito, 0.004f) * P.Presenza);
			}
			else
			{
				const FVector Qui = P.Ancora + FVector(FMath::Sin(T * 0.35f + P.Fase.X) * 120.f, FMath::Cos(T * 0.3f + P.Fase.Y) * 120.f,
					50.f + P.Fase.Z * 16.f + FMath::Sin(T * 0.5f + P.Fase.Z) * 35.f);
				// si accende e si spegne, ognuna col suo tempo
				Luce = P.Presenza * (0.04f + FMath::Pow(FMath::Max(0.f, FMath::Sin(T * 1.3f + P.Fase.X * 3.f)), 6.f));
				Forma = FTransform(FRotator::ZeroRotator, Qui, FVector(0.04f * P.Presenza));
			}
		}
		Dove->UpdateInstanceTransform(P.Istanza, Forma, true, false, true);
		if (P.bFarfalla)
		{
			bCambiate[P.Colore % VitaColori] = true;
		}
		else
		{
			Dove->SetCustomDataValue(P.Istanza, 0, Luce, false);
			bAccese = true;
		}
	}
	for (int32 c = 0; c < Farfalle.Num() && c < VitaColori; ++c)
	{
		if (bCambiate[c])
		{
			Farfalle[c]->MarkRenderStateDirty();
		}
	}
	if (bAccese)
	{
		Lucciole->MarkRenderStateDirty();
	}
}

void AValdorsoVitaValle::AggiornaStormo(int32 Quale, bool bVoluto, const FVector& Colono, float Delta, double Adesso)
{
	FStormo& S = Stormi[Quale];
	auto NuovaMeta = [&S, &Colono, Adesso]()
	{
		const FVector2D Disco = FMath::RandPointInCircle(25000.f);
		S.Meta = Colono + FVector(Disco.X, Disco.Y, FMath::FRandRange(4500.f, 9000.f));
		S.CambiaMeta = Adesso + FMath::FRandRange(20.0, 40.0);
	};
	if (bVoluto && !S.bInCielo)
	{
		// arriva da lontano, gia' in volo (non compare sotto gli occhi)
		const FVector Da = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * 30000.f;
		S.Centro = Colono + Da + FVector(0.f, 0.f, 6000.f);
		S.Velocita = -Da.GetSafeNormal() * 1100.f;
		S.bInCielo = true;
		S.bVaVia = false;
		NuovaMeta();
	}
	if (!S.bInCielo)
	{
		return;
	}
	// se ne andava ma il tempo e' tornato bello: torna indietro
	if (bVoluto && S.bVaVia)
	{
		S.bVaVia = false;
		NuovaMeta();
	}
	if (!bVoluto && !S.bVaVia)
	{
		// la sera: se ne va lontano, dalla parte opposta al colono
		S.bVaVia = true;
		const FVector Via = (S.Centro - Colono).GetSafeNormal2D();
		S.Meta = S.Centro + (Via.IsNearlyZero() ? FVector::ForwardVector : Via) * 90000.f + FVector(0.f, 0.f, 4000.f);
	}
	if (!S.bVaVia && (Adesso >= S.CambiaMeta || FVector::Dist(S.Centro, S.Meta) < 3000.f))
	{
		NuovaMeta();
	}
	// gira piano verso la meta (le curve larghe di uno stormo vero)
	const FVector Voluta = (S.Meta - S.Centro).GetSafeNormal() * 1100.f;
	S.Velocita = FMath::VInterpTo(S.Velocita, Voluta, Delta, 0.6f);
	S.Centro += S.Velocita * Delta;
	if (S.bVaVia && FVector::Dist2D(S.Centro, Colono) > 70000.f)
	{
		S.bInCielo = false;
	}
}

void AValdorsoVitaValle::AggiornaUccelli(const FVector& Colono, float Delta, double Adesso, bool bRapaceVoluto, bool bRondini, bool bPipistrelli)
{
	if (!Volatori->GetStaticMesh() || Uccelli.Num() == 0)
	{
		return;
	}
	const float T = static_cast<float>(Adesso);
	bool bCambiato = false;
	for (int32 i = 0; i < Uccelli.Num(); ++i)
	{
		FVolatore& V = Uccelli[i];
		// voluto adesso; quelli vicini (rapace, rondini, pipistrelli) arrivano e se ne vanno piano (Presenza)
		bool bVoluto = false;
		bool bPosto = true;
		FVector Dove = FVector::ZeroVector;
		FRotator Verso = FRotator::ZeroRotator;
		switch (V.Gruppo)
		{
		case EGruppo::Stormo0:
		case EGruppo::Stormo1:
		{
			const FStormo& S = Stormi[V.Gruppo == EGruppo::Stormo0 ? 0 : 1];
			bVoluto = S.bInCielo;
			bPosto = S.bInCielo;
			if (S.bInCielo)
			{
				const FVector Giro(FMath::Sin(T * 0.6f + V.Fase.X) * V.Raggio, FMath::Cos(T * 0.45f + V.Fase.Y) * V.Raggio * 0.8f,
					FMath::Sin(T * 0.8f + V.Fase.Z) * 220.f);
				// il giro intorno al centro, girato come va lo stormo
				Dove = S.Centro + FRotator(0.f, S.Velocita.Rotation().Yaw, 0.f).RotateVector(Giro);
				Verso = FRotator(FMath::Clamp(S.Velocita.Rotation().Pitch, -15.f, 15.f), S.Velocita.Rotation().Yaw + FMath::Sin(T * 1.3f + V.Fase.X) * 12.f, 0.f);
			}
			break;
		}
		case EGruppo::Rapace:
		{
			bVoluto = bRapaceVoluto;
			const float A = T * 0.22f + V.Fase.X;
			Dove = CentroRapace + FVector(FMath::Cos(A) * V.Raggio, FMath::Sin(A) * V.Raggio, 11000.f);
			Verso = FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, -25.f);
			break;
		}
		case EGruppo::Rondini:
			bVoluto = bRondini;
			Dove = Colono + FVector(FMath::Sin(T * 1.7f + V.Fase.X) * V.Raggio, FMath::Sin(T * 1.1f + V.Fase.Y) * V.Raggio,
				900.f + FMath::Sin(T * 2.3f + V.Fase.Z) * 400.f);
			break;
		case EGruppo::Drago:
			// l'uccello grande come un drago, solo se non c'e' il drago di Fab
			bVoluto = bDragoInVolo && !DragoVero->GetStaticMesh();
			Dove = DragoDove;
			Verso = FRotator(FMath::Sin(T * 0.4f) * 4.f, 0.f, FMath::Sin(T * 0.3f) * 8.f);
			break;
		case EGruppo::Pipistrelli:
			bVoluto = bPipistrelli;
			Dove = Colono + FVector(FMath::Sin(T * 2.9f + V.Fase.X) * V.Raggio + FMath::Sin(T * 5.3f + V.Fase.Y) * 120.f,
				FMath::Cos(T * 3.3f + V.Fase.Y) * V.Raggio, 500.f + FMath::Sin(T * 4.1f + V.Fase.Z) * 180.f);
			break;
		}
		const bool bLontani = V.Gruppo == EGruppo::Stormo0 || V.Gruppo == EGruppo::Stormo1;
		V.Presenza = bLontani || Colono.IsZero() ? (bVoluto ? 1.f : 0.f) : FMath::FInterpConstantTo(V.Presenza, bVoluto ? 1.f : 0.f, Delta, 0.8f);
		const bool bVisibile = bPosto && V.Presenza > 0.01f;
		if (bVisibile && (V.Gruppo == EGruppo::Rondini || V.Gruppo == EGruppo::Pipistrelli))
		{
			// guardano dove vanno (dalla posizione di prima)
			const FVector Passo = Dove - V.Prima;
			if (V.bVisibile && !Passo.IsNearlyZero())
			{
				Verso = FRotator(FMath::Clamp(Passo.Rotation().Pitch, -30.f, 30.f), Passo.Rotation().Yaw, 0.f);
			}
		}
		if (!bVisibile && !V.bVisibile)
		{
			continue;
		}
		V.Prima = Dove;
		V.bVisibile = bVisibile;
		const FTransform Forma = bVisibile ? FTransform(Verso, Dove, FVector(V.Scala * V.Presenza)) : FTransform(FRotator::ZeroRotator, FVector::ZeroVector, VitaNascosta);
		Volatori->UpdateInstanceTransform(i, Forma, true, false, true);
		bCambiato = true;
	}
	if (bCambiato)
	{
		Volatori->MarkRenderStateDirty();
	}
}

void AValdorsoVitaValle::AggiornaFoglie(const FVector& Colono, float Delta, double Adesso, bool bVoluto)
{
	const float T = static_cast<float>(Adesso);
	// una foglia nuova ogni tanto, se sotto gli alberi intorno al colono
	if (bVoluto && Alberi.Num() > 0 && Adesso >= ProssimaFoglia)
	{
		ProssimaFoglia = Adesso + 0.12;
		FFoglia* Libera = Cadute.FindByPredicate([](const FFoglia& F) { return !F.bViva && F.Istanza != INDEX_NONE; });
		const FVector2D Disco = FMath::RandPointInCircle(VitaRaggioFoglie);
		const FVector* Albero = Alberi.Find(ValdorsoVitaValle::Cella(Colono + FVector(Disco.X, Disco.Y, 0.f)));
		if (Libera && Albero)
		{
			const FVector2D Chioma = FMath::RandPointInCircle(350.f);
			const FVector Inizio = *Albero + FVector(Chioma.X, Chioma.Y, FMath::FRandRange(600.f, 1100.f));
			FHitResult Colpo;
			FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoFoglie), false);
			const APlayerController* Giocatore = GetWorld()->GetFirstPlayerController();
			if (Giocatore && Giocatore->GetPawn())
			{
				Parametri.AddIgnoredActor(Giocatore->GetPawn());
			}
			if (GetWorld()->LineTraceSingleByChannel(Colpo, Inizio, Inizio - FVector(0.f, 0.f, 3000.f), ECC_Visibility, Parametri))
			{
				Libera->Posto = Inizio;
				Libera->Terra = static_cast<float>(Colpo.ImpactPoint.Z);
				Libera->Caduta = FMath::FRandRange(60.f, 110.f);
				Libera->Fase = FVector(FMath::FRandRange(0.f, 2.f * PI), FMath::FRandRange(0.f, 2.f * PI), FMath::FRandRange(0.f, 2.f * PI));
				Libera->Posata = -1.0;
				Libera->bViva = true;
			}
		}
	}
	bool bCambiato[VitaColori] = { false, false, false };
	for (FFoglia& F : Cadute)
	{
		if (F.Istanza == INDEX_NONE || !Foglie.IsValidIndex(F.Colore))
		{
			continue;
		}
		FTransform Forma(FRotator::ZeroRotator, FVector::ZeroVector, VitaNascosta);
		if (F.bViva)
		{
			float Grande = 1.f;
			FRotator Giro;
			if (F.Posata < 0.0)
			{
				// cade ondeggiando, girando su se stessa, e il vento la porta un poco
				F.Posto.Z -= F.Caduta * Delta;
				F.Posto.X += (FMath::Sin(T * 1.3f + F.Fase.X) * 70.f + 35.f) * Delta;
				F.Posto.Y += (FMath::Cos(T * 1.1f + F.Fase.Y) * 70.f + 15.f) * Delta;
				Giro = FRotator(FMath::Sin(T * 2.f + F.Fase.Z) * 60.f, T * 90.f + F.Fase.X * 57.f, FMath::Cos(T * 1.7f + F.Fase.Y) * 50.f);
				if (F.Posto.Z <= F.Terra + 1.f)
				{
					F.Posto.Z = F.Terra + 1.f;
					F.Posata = Adesso;
				}
			}
			else
			{
				Giro = FRotator(0.f, F.Fase.X * 57.f, 0.f);
				const double ATerra = Adesso - F.Posata;
				Grande = ATerra > VitaFogliaATerra ? 1.f - static_cast<float>((ATerra - VitaFogliaATerra) / 2.0) : 1.f;
			}
			if (Grande <= 0.f || FVector::Dist2D(F.Posto, Colono) > 3500.f)
			{
				F.bViva = false;
			}
			else
			{
				Forma = FTransform(Giro, F.Posto, FVector(0.07f, 0.05f, 0.004f) * (Grande * GrandezzaFoglie));
			}
		}
		else if (F.Posata == -2.0)
		{
			continue;  // gia' nascosta
		}
		if (!F.bViva)
		{
			F.Posata = -2.0;
		}
		Foglie[F.Colore]->UpdateInstanceTransform(F.Istanza, Forma, true, false, true);
		bCambiato[F.Colore] = true;
	}
	for (int32 c = 0; c < Foglie.Num() && c < VitaColori; ++c)
	{
		if (bCambiato[c])
		{
			Foglie[c]->MarkRenderStateDirty();
		}
	}
}
