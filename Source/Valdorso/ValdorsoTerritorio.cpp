// Valdorso - La valle come territorio e la pietra del villaggio nel tempio (vedi il .h).

#include "ValdorsoTerritorio.h"
#include "ValdorsoVoci.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "Valdorso.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "ValdorsoTerritorio"

namespace ValdorsoTerritori
{
	const FValdorsoTerritorio& ValDOrso()
	{
		static const FValdorsoTerritorio Valle = []()
		{
			FValdorsoTerritorio T;
			T.Id = TEXT("valdorso");
			T.Nome = LOCTEXT("Nome", "Val d'Orso");
			T.Signore = LOCTEXT("Signore", "La Corona di Aurelia");
			T.Governo = LOCTEXT("Governo", "Il Balivo, per conto della Corona");
			T.Futuro = LOCTEXT("Futuro", "Un giorno: la Baronia di Valdorso");
			T.Custode = LOCTEXT("Custode", "Padre Anselmo, sacerdote del villaggio");
			T.Fede = LOCTEXT("Fede", "La fede della Corona");
			// Raggio 0 = vale su tutta la mappa (per ora c'è solo la valle). Con i feudi si metteranno i confini veri.
			// (08/11) Coordinate del mondo, non del generatore: il Landscape di L_Valle va da (-150000, -150000) a
			// (150000, 150000), quindi il centro della valle è (0, 0) e la piazza sta in (15300, 42228)
			// (ValdorsoLuoghiDati.inl). Un raggio vero andrà misurato da qui, non da (150000, 150000).
			T.Centro = FVector2D(0.0, 0.0);
			T.Raggio = 0.f;
			return T;
		}();
		return Valle;
	}

	const FValdorsoTerritorio* Di(const FVector& Dove)
	{
		const FValdorsoTerritorio& Valle = ValDOrso();
		if (Valle.Raggio <= 0.f || FVector2D::Distance(FVector2D(Dove.X, Dove.Y), Valle.Centro) <= Valle.Raggio)
		{
			return &Valle;
		}
		return nullptr;
	}

	FString IdDi(const FVector& Dove)
	{
		const FValdorsoTerritorio* Territorio = Di(Dove);
		return Territorio ? Territorio->Id : FString(TEXT("nessuno"));
	}

	FText ParolaForza(float Forza)
	{
		if (Forza >= 0.85f)
		{
			return LOCTEXT("Piena", "piena");
		}
		if (Forza >= 0.6f)
		{
			return LOCTEXT("Buona", "buona");
		}
		if (Forza >= 0.3f)
		{
			return LOCTEXT("Debole", "debole");
		}
		return LOCTEXT("Stremo", "allo stremo");
	}

	bool SaltaIlBattito(int64 Giro, float Forza)
	{
		if (Forza >= 0.5f)
		{
			return false;
		}
		// Un numero "a caso" ma uguale per tutti: dal giro, con un rimescolamento semplice.
		uint32 X = static_cast<uint32>(Giro * 2654435761LL);
		X ^= X >> 16;
		X *= 0x45d9f3bu;
		X ^= X >> 16;
		const float Caso = static_cast<float>(X % 1000u) / 1000.f;
		// a metà forza mai, a forza 0 un giro su due
		const float Probabilita = FMath::Clamp((0.5f - Forza) / 0.5f, 0.f, 1.f) * 0.5f;
		return Caso < Probabilita;
	}

	float Battito(double Secondi, float Forza)
	{
		const double Periodo = 1.6;
		const double Positivo = FMath::Max(0.0, Secondi);
		const int64 Giro = static_cast<int64>(Positivo / Periodo);
		if (SaltaIlBattito(Giro, Forza))
		{
			return 0.f;
		}
		const double P = Positivo - Giro * Periodo;
		auto Colpo = [](double X, double Centro)
		{
			const double D = (X - Centro) / 0.07;
			return static_cast<double>(FMath::Exp(static_cast<float>(-D * D)));
		};
		return static_cast<float>(FMath::Clamp(Colpo(P, 0.1) + 0.75 * Colpo(P, 0.42), 0.0, 1.0));
	}
}

bool AValdorsoPietraVillaggio::Protegge(const FVector& Dove) const
{
	if (Forza <= 0.f || FVector::DistSquared(Dove, GetActorLocation()) > FMath::Square(RaggioPace))
	{
		return false;
	}
	const UWorld* Mondo = GetWorld();
	const double Secondi = Mondo ? FMath::Max(0.0, Mondo->GetTimeSeconds()) : 0.0;
	return !ValdorsoTerritori::SaltaIlBattito(static_cast<int64>(Secondi / 1.6), Forza);
}

namespace
{
	void ImpostaForzaPietre(float Forza)
	{
		for (TObjectIterator<AValdorsoPietraVillaggio> Pietra; Pietra; ++Pietra)
		{
			if (IsValid(*Pietra) && Pietra->HasAuthority() && Pietra->GetWorld() && Pietra->GetWorld()->IsGameWorld())
			{
				Pietra->ImpostaForza(Forza);
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Pietra di Val d'Orso: forza %.2f"), Pietra->GetForza());
			}
		}
	}

	FAutoConsoleCommand ComandoForzaPietra(
		TEXT("Valdorso.Pietra.Forza"),
		TEXT("La forza della pietra del villaggio, da 0 a 1 (di serie 1). Sotto 0.5 il battito ogni tanto salta. Solo prove."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti)
		{
			if (Argomenti.Num() > 0)
			{
				ImpostaForzaPietre(FCString::Atof(*Argomenti[0]));
			}
		}));
}

AValdorsoPietraVillaggio::AValdorsoPietraVillaggio()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.03f;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);

	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;

	// (07/11 sera) Il cuscino di lino su cui riposa il frammento (dalle Cronache): un ovale basso, color lino.
	Cuscino = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cuscino"));
	Cuscino->SetupAttachment(Radice);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cilindro.Succeeded())
	{
		Cuscino->SetStaticMesh(Cilindro.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Base(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Base.Succeeded())
	{
		MaterialeLino = Base.Object;
		Cuscino->SetMaterial(0, Base.Object);
	}
	Cuscino->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
	Cuscino->SetRelativeScale3D(FVector(0.46f, 0.34f, 0.07f));
	Cuscino->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cuscino->SetCanEverAffectNavigation(false);

	// Il frammento: due coni uniti alla base (una gemma a punta in su e in giù), un po' sopra il cuscino.
	// (Prima era un cubo allungato: con la luce forte sembrava una lastra bianca.)
	Gemma = CreateDefaultSubobject<USceneComponent>(TEXT("Gemma"));
	Gemma->SetupAttachment(Radice);
	Gemma->SetRelativeLocation(FVector(0.f, 0.f, AltezzaGemma));
	Gemma->SetRelativeRotation(FRotator(0.f, 0.f, 12.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cono(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Cristallo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cristallo"));
	Cristallo->SetupAttachment(Gemma);
	CristalloSotto = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CristalloSotto"));
	CristalloSotto->SetupAttachment(Gemma);
	// il cono di Unreal è alto 100 cm con il centro a metà: scalato a 0,26 è alto 26 cm, la base a -13
	Cristallo->SetRelativeLocation(FVector(0.f, 0.f, 13.f * 1.1f));
	Cristallo->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.26f * 1.1f));
	CristalloSotto->SetRelativeLocation(FVector(0.f, 0.f, -9.f));
	CristalloSotto->SetRelativeRotation(FRotator(180.f, 0.f, 0.f));
	CristalloSotto->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.18f));
	for (UStaticMeshComponent* Pezzo : { Cristallo.Get(), CristalloSotto.Get() })
	{
		if (Cono.Succeeded())
		{
			Pezzo->SetStaticMesh(Cono.Object);
		}
		Pezzo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pezzo->SetCastShadow(false);
		Pezzo->SetCanEverAffectNavigation(false);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Luminoso(TEXT("/Game/Menu/M_Luminoso.M_Luminoso"));
	if (Luminoso.Succeeded())
	{
		Materiale = Luminoso.Object;
	}

	Luce = CreateDefaultSubobject<UPointLightComponent>(TEXT("Luce"));
	Luce->SetupAttachment(Radice);
	// (07/11 sera) la luce più in alto: da 40 cm il cuscino e l'altare diventavano bianchi
	Luce->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	Luce->SetIntensityUnits(ELightUnits::Candelas);
	Luce->SetLightColor(FLinearColor(1.f, 0.62f, 0.32f));
	Luce->SetIntensity(LuceQuiete);
	Luce->SetAttenuationRadius(900.f);
	Luce->SetVolumetricScatteringIntensity(0.15f);
	Luce->SetMobility(EComponentMobility::Movable);
	Luce->SetCastShadows(false);

	// La zona del tempio: una scatola di 10 x 10 m intorno all'altare (il tempio è largo 12 m), senza urti con niente;
	// la collisione "solo domande" serve al post process per sapere se la telecamera è dentro.
	ZonaTempio = CreateDefaultSubobject<UBoxComponent>(TEXT("ZonaTempio"));
	ZonaTempio->SetupAttachment(Radice);
	ZonaTempio->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	ZonaTempio->InitBoxExtent(FVector(500.f, 500.f, 450.f));
	ZonaTempio->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZonaTempio->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZonaTempio->SetGenerateOverlapEvents(false);
	ZonaTempio->SetCanEverAffectNavigation(false);
	ZonaTempio->SetHiddenInGame(true);

	VistaTempio = CreateDefaultSubobject<UPostProcessComponent>(TEXT("VistaTempio"));
	VistaTempio->SetupAttachment(ZonaTempio);
	VistaTempio->bUnbound = false;
	VistaTempio->BlendRadius = 250.f;
	VistaTempio->Priority = 1.f;
	VistaTempio->Settings.bOverride_AutoExposureBias = true;
	VistaTempio->Settings.AutoExposureBias = EsposizioneTempio;

	Zona = CreateDefaultSubobject<USphereComponent>(TEXT("Zona"));
	Zona->SetupAttachment(Radice);
	Zona->SetRelativeLocation(FVector(0.f, 0.f, AltezzaGemma));
	Zona->InitSphereRadius(45.f);
	Zona->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Zona->SetCollisionObjectType(ECC_WorldDynamic);
	Zona->SetCollisionResponseToAllChannels(ECR_Overlap);
	Zona->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Zona->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Zona->SetGenerateOverlapEvents(false);
	Zona->SetHiddenInGame(true);
	Zona->SetCanEverAffectNavigation(false);
}

void AValdorsoPietraVillaggio::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoPietraVillaggio, Forza);
}

void AValdorsoPietraVillaggio::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	VistaTempio->Settings.AutoExposureBias = EsposizioneTempio;
	if (Materiale)
	{
		Cristallo->SetMaterial(0, Materiale);
		CristalloSotto->SetMaterial(0, Materiale);
	}
}

void AValdorsoPietraVillaggio::BeginPlay()
{
	Super::BeginPlay();
	// (11/11, v0.1.6) l'Atto I "Il battito cambiato" e' cominciato (anche prima di un riavvio): la pietra resta debole
	if (HasAuthority() && ValdorsoVoci::Atto(this) >= 1)
	{
		ImpostaForza(FMath::Min(Forza, ValdorsoVoci::ForzaAttoI));
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		// sul server dedicato non c'è niente da vedere
		SetActorTickEnabled(false);
		return;
	}
	if (Materiale)
	{
		// un solo materiale vivo per i due coni, così battono insieme
		MaterialeVivo = UMaterialInstanceDynamic::Create(Materiale, this);
		if (MaterialeVivo)
		{
			MaterialeVivo->SetVectorParameterValue(TEXT("Colore"), FLinearColor(1.f, 0.36f, 0.08f));
			Cristallo->SetMaterial(0, MaterialeVivo);
			CristalloSotto->SetMaterial(0, MaterialeVivo);
		}
	}
	if (MaterialeLino)
	{
		if (UMaterialInstanceDynamic* Lino = Cuscino->CreateDynamicMaterialInstance(0, MaterialeLino))
		{
			// BasicShapeMaterial ha il parametro "Color": il lino grezzo, un po' scaldato dalla pietra
			Lino->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.36f, 0.29f, 0.2f));
		}
	}
}

void AValdorsoPietraVillaggio::ImpostaForza(float Nuova)
{
	if (HasAuthority())
	{
		Forza = FMath::Clamp(Nuova, 0.f, 1.f);
		ForceNetUpdate();
	}
}

void AValdorsoPietraVillaggio::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Tempo += DeltaSeconds;

	// Il battito dall'orologio del mondo (uguale su tutti i PC entro il ritardo della rete).
	const UWorld* Mondo = GetWorld();
	const AGameStateBase* Stato = Mondo ? Mondo->GetGameState() : nullptr;
	const double Secondi = Stato ? Stato->GetServerWorldTimeSeconds() : static_cast<double>(Tempo);
	const float Colpo = ValdorsoTerritori::Battito(Secondi, Forza);
	// più debole la pietra, più fioca la luce (mai spenta del tutto finché c'è forza)
	const float Vigore = 0.25f + 0.75f * Forza;
	Luce->SetIntensity(FMath::Lerp(LuceQuiete, LuceColpo, Colpo) * Vigore);
	if (MaterialeVivo)
	{
		MaterialeVivo->SetScalarParameterValue(TEXT("Intensita"), FMath::Lerp(BagliorQuiete, BagliorColpo, Colpo) * Vigore);
	}
	// gira piano e galleggia appena sopra il cuscino
	Gemma->AddRelativeRotation(FRotator(0.f, 6.f * DeltaSeconds, 0.f));
	Gemma->SetRelativeLocation(FVector(0.f, 0.f, AltezzaGemma + FMath::Sin(Tempo * 0.8f) * 2.f));
}

FText AValdorsoPietraVillaggio::TestoInterazione(const APawn* Chi) const
{
	return LOCTEXT("Guarda", "Guarda la pietra");
}

FVector AValdorsoPietraVillaggio::PuntoInterazione() const
{
	return Zona ? Zona->GetComponentLocation() : GetActorLocation();
}

void AValdorsoPietraVillaggio::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	const FValdorsoTerritorio* Territorio = ValdorsoTerritori::Di(GetActorLocation());
	const FValdorsoTerritorio& T = Territorio ? *Territorio : ValdorsoTerritori::ValDOrso();

	TArray<FString> Righe;
	Righe.Add(TEXT("IL CUORE BATTE ANCORA. Le lettere incise sull'altare sono consumate dalle mani di chi le ha toccate."));
	Righe.Add(FText::Format(LOCTEXT("RigaTerra", "Terra: {0}. Signore: {1}. Governa: {2}. {3}."),
		T.Nome, T.Signore, T.Governo, T.Futuro).ToString());
	Righe.Add(FText::Format(LOCTEXT("RigaCustode", "Custode della pietra: {0}."), T.Custode).ToString());
	const int32 Percento = FMath::RoundToInt(Forza * 100.f);
	if (Forza >= 0.5f)
	{
		Righe.Add(FText::Format(LOCTEXT("RigaForzaBuona", "La pietra è {0} ({1}%): il battito è regolare, due colpi e una pausa. Finché batte così, le ombre restano oltre la barriera."),
			ValdorsoTerritori::ParolaForza(Forza), FText::AsNumber(Percento)).ToString());
	}
	else
	{
		Righe.Add(FText::Format(LOCTEXT("RigaForzaDebole", "La pietra è {0} ({1}%): ogni tanto il battito salta un colpo. Va ricaricata con un rituale, prima che la barriera tremi."),
			ValdorsoTerritori::ParolaForza(Forza), FText::AsNumber(Percento)).ToString());
	}
	Righe.Add(TEXT("Chi firma il Registro di Val d'Orso riceve terra e protezione, e giura di restare nella valle."));

	if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(Chi->GetController()))
	{
		Controllore->ClientMostraBacheca(FText::Format(LOCTEXT("Titolo", "La pietra di {0}"), T.Nome).ToString().Left(60), Righe);
	}
	// Toccare la pietra non è un crimine, ma un giorno potrebbe esserlo (rubarla, spezzarla): passa dal registro.
	ValdorsoRegistroEventi::Annota(TEXT("GuardaPietra"), Chi, this, GetActorLocation(), FString::Printf(TEXT("forza %d%%"), Percento));
}

#undef LOCTEXT_NAMESPACE
