// Valdorso - Gli oggetti vivi della scena dietro il menu (vedi il .h).

#include "ValdorsoScenaMenu.h"
#include "ValdorsoTemaUI.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimationAsset.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "HAL/PlatformTime.h"

namespace
{
	const FName ParametroColore(TEXT("Colore"));
	const FName ParametroIntensita(TEXT("Intensita"));
	const FName ParametroSeme(TEXT("Seme"));
	const FName ParametroOpacita(TEXT("Opacita"));
	const FName ParametroVerso(TEXT("Verso"));
	const FName ParametroPezzi(TEXT("Pezzi"));

	const TCHAR* PercorsoPiano = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* PercorsoSfera = TEXT("/Engine/BasicShapes/Sphere.Sphere");

	/** Un componente che fa solo scena: niente urti, niente ombre. */
	void SoloScena(UPrimitiveComponent* Componente)
	{
		Componente->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Componente->SetCollisionProfileName(TEXT("NoCollision"));
		Componente->SetCastShadow(false);
		Componente->SetCanEverAffectNavigation(false);
	}

	/** Il piano dell'engine (100 x 100 cm) messo in piedi: il suo lato X va verso l'alto. */
	const FRotator InPiedi(90.f, 0.f, 0.f);
}

// ---------------------------------------------------------------------------------------------
// Il frammento del Cuore
// ---------------------------------------------------------------------------------------------

AValdorsoFrammentoCuore::AValdorsoFrammentoCuore()
{
	PrimaryActorTick.bCanEverTick = true;

	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;

	// Un cubo allungato e girato sulla punta: un cristallo semplice, finché non arriva il modello vero.
	Cristallo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cristallo"));
	Cristallo->SetupAttachment(Radice);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cubo.Succeeded())
	{
		Cristallo->SetStaticMesh(Cubo.Object);
	}
	Cristallo->SetRelativeLocation(FVector(0.f, 0.f, 75.f));
	Cristallo->SetRelativeRotation(FRotator(35.f, 0.f, 45.f));
	Cristallo->SetRelativeScale3D(FVector(0.26f, 0.26f, 0.7f));
	Cristallo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cristallo->SetCastShadow(false);

	Luce = CreateDefaultSubobject<UPointLightComponent>(TEXT("Luce"));
	Luce->SetupAttachment(Radice);
	Luce->SetRelativeLocation(FVector(0.f, 0.f, 75.f));
	Luce->SetLightColor(FLinearColor(1.f, 0.62f, 0.32f));
	Luce->SetIntensity(LuceQuiete);
	Luce->SetAttenuationRadius(1600.f);
	Luce->SetMobility(EComponentMobility::Movable);
}

void AValdorsoFrammentoCuore::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Materiale)
	{
		Cristallo->SetMaterial(0, Materiale);
	}
}

void AValdorsoFrammentoCuore::BeginPlay()
{
	Super::BeginPlay();
	if (Materiale)
	{
		MaterialeVivo = Cristallo->CreateDynamicMaterialInstance(0, Materiale);
		if (MaterialeVivo)
		{
			MaterialeVivo->SetVectorParameterValue(ParametroColore, FLinearColor(1.f, 0.6f, 0.25f));
		}
	}
}

void AValdorsoFrammentoCuore::Risveglia()
{
	bRisveglio = true;
}

void AValdorsoFrammentoCuore::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Tempo += DeltaSeconds;

	// Due colpi e una pausa, insieme al titolo del menu.
	float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
	float Forza = 1.f;
	if (bRisveglio)
	{
		// Entrando nella valle il frammento si accende tutto, in un secondo e mezzo.
		Risveglio = FMath::Min(Risveglio + DeltaSeconds / 1.5f, 1.f);
		Colpo = FMath::Max(Colpo, Risveglio);
		Forza = 1.f + 3.f * Risveglio;
	}
	Luce->SetIntensity(FMath::Lerp(LuceQuiete, LuceColpo, Colpo) * Forza);
	if (MaterialeVivo)
	{
		MaterialeVivo->SetScalarParameterValue(ParametroIntensita, FMath::Lerp(BagliorQuiete, BagliorColpo, Colpo) * Forza);
	}

	// Gira piano e galleggia appena.
	Cristallo->AddWorldRotation(FRotator(0.f, Rotazione * (1.f + 6.f * Risveglio) * DeltaSeconds, 0.f));
	Cristallo->SetRelativeLocation(FVector(0.f, 0.f, 75.f + FMath::Sin(Tempo * 0.8f) * 5.f + Risveglio * 25.f));
}

// ---------------------------------------------------------------------------------------------
// Il braciere
// ---------------------------------------------------------------------------------------------

AValdorsoBraciere::AValdorsoBraciere()
{
	PrimaryActorTick.bCanEverTick = true;

	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sfera(PercorsoSfera);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Piano(PercorsoPiano);

	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(Radice);
	if (Cilindro.Succeeded())
	{
		Base->SetStaticMesh(Cilindro.Object);
	}
	Base->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	Base->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.9f));

	Braci = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Braci"));
	Braci->SetupAttachment(Radice);
	if (Sfera.Succeeded())
	{
		Braci->SetStaticMesh(Sfera.Object);
	}
	Braci->SetRelativeLocation(FVector(0.f, 0.f, 92.f));
	Braci->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.14f));
	Braci->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Braci->SetCastShadow(false);

	// Tre lingue di fuoco incrociate a 60 gradi: da qualunque lato le guardi, sembra un fuoco pieno.
	UStaticMesh* MeshPiano = Piano.Succeeded() ? Piano.Object : nullptr;
	Fiamma1 = CreaFiamma(TEXT("Fiamma1"), MeshPiano);
	Fiamma2 = CreaFiamma(TEXT("Fiamma2"), MeshPiano);
	Fiamma3 = CreaFiamma(TEXT("Fiamma3"), MeshPiano);
	SistemaFiamme();

	Luce = CreateDefaultSubobject<UPointLightComponent>(TEXT("Luce"));
	Luce->SetupAttachment(Radice);
	Luce->SetRelativeLocation(FVector(0.f, 0.f, 135.f));
	Luce->SetLightColor(FLinearColor(1.f, 0.45f, 0.16f));
	Luce->SetIntensity(LuceMedia);
	Luce->SetAttenuationRadius(1100.f);
	Luce->SetMobility(EComponentMobility::Movable);
}

UStaticMeshComponent* AValdorsoBraciere::CreaFiamma(const TCHAR* Nome, UStaticMesh* Piano)
{
	UStaticMeshComponent* Fiamma = CreateDefaultSubobject<UStaticMeshComponent>(Nome);
	Fiamma->SetupAttachment(Radice);
	if (Piano)
	{
		Fiamma->SetStaticMesh(Piano);
	}
	SoloScena(Fiamma);
	return Fiamma;
}

void AValdorsoBraciere::SistemaFiamme()
{
	UStaticMeshComponent* Elenco[] = { Fiamma1, Fiamma2, Fiamma3 };
	const FVector Scala(AltezzaFiamma / 100.f, LarghezzaFiamma / 100.f, 1.f);
	const FVector Centro(0.f, 0.f, 88.f + AltezzaFiamma * 0.5f);
	for (int32 i = 0; i < 3; ++i)
	{
		UStaticMeshComponent* Fiamma = Elenco[i];
		if (Fiamma)
		{
			Fiamma->SetRelativeLocation(Centro);
			Fiamma->SetRelativeRotation(FRotator(90.f, i * 60.f, 0.f));
			Fiamma->SetRelativeScale3D(Scala);
		}
	}
}

void AValdorsoBraciere::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (MaterialePietra)
	{
		Base->SetMaterial(0, MaterialePietra);
	}
	if (MaterialeBraci)
	{
		Braci->SetMaterial(0, MaterialeBraci);
	}
	SistemaFiamme();
	const bool bFuoco = MaterialeFuoco != nullptr;
	for (UStaticMeshComponent* Fiamma : { Fiamma1, Fiamma2, Fiamma3 })
	{
		Fiamma->SetVisibility(bFuoco);
		if (bFuoco)
		{
			Fiamma->SetMaterial(0, MaterialeFuoco);
		}
	}
}

void AValdorsoBraciere::BeginPlay()
{
	Super::BeginPlay();
	Seme = FMath::FRandRange(0.f, 100.f);
	if (MaterialeBraci)
	{
		BraciVive = Braci->CreateDynamicMaterialInstance(0, MaterialeBraci);
		if (BraciVive)
		{
			BraciVive->SetVectorParameterValue(ParametroColore, FLinearColor(1.f, 0.32f, 0.08f));
		}
	}
	FiammeVive.Reset();
	if (MaterialeFuoco)
	{
		int32 Numero = 0;
		for (UStaticMeshComponent* Fiamma : { Fiamma1, Fiamma2, Fiamma3 })
		{
			if (UMaterialInstanceDynamic* Viva = Fiamma->CreateDynamicMaterialInstance(0, MaterialeFuoco))
			{
				Viva->SetScalarParameterValue(ParametroSeme, Seme + 17.f * Numero);
				FiammeVive.Add(Viva);
			}
			++Numero;
		}
	}
}

void AValdorsoBraciere::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Tempo += DeltaSeconds;

	// Il fuoco non è regolare: tre onde diverse sommate.
	const float Fuoco = 0.72f
		+ 0.16f * FMath::Sin(Tempo * 7.3f + Seme)
		+ 0.08f * FMath::Sin(Tempo * 13.7f + Seme * 2.f)
		+ 0.04f * FMath::Sin(Tempo * 23.1f + Seme * 3.f);

	Luce->SetIntensity(LuceMedia * Fuoco);
	if (BraciVive)
	{
		BraciVive->SetScalarParameterValue(ParametroIntensita, 8.f * Fuoco);
	}
	for (UMaterialInstanceDynamic* Viva : FiammeVive)
	{
		if (Viva)
		{
			Viva->SetScalarParameterValue(ParametroIntensita, IntensitaFuoco * (0.6f + 0.55f * Fuoco));
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Il cerchio di rune
// ---------------------------------------------------------------------------------------------

AValdorsoCerchioRune::AValdorsoCerchioRune()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Piano(PercorsoPiano);

	Cerchio = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cerchio"));
	RootComponent = Cerchio;
	if (Piano.Succeeded())
	{
		Cerchio->SetStaticMesh(Piano.Object);
	}
	SoloScena(Cerchio);
}

void AValdorsoCerchioRune::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const float Scala = Diametro / 100.f;
	Cerchio->SetWorldScale3D(FVector(Scala, Scala, 1.f));
	if (Materiale)
	{
		Cerchio->SetMaterial(0, Materiale);
	}
}

void AValdorsoCerchioRune::BeginPlay()
{
	Super::BeginPlay();
	if (Materiale)
	{
		RuneVive = Cerchio->CreateDynamicMaterialInstance(0, Materiale);
		if (RuneVive)
		{
			RuneVive->SetVectorParameterValue(ParametroColore, Colore);
		}
	}
}

void AValdorsoCerchioRune::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (RuneVive)
	{
		const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds() - Ritardo);
		RuneVive->SetScalarParameterValue(ParametroIntensita, FMath::Lerp(RuneQuiete, RuneColpo, Colpo));
	}
}

// ---------------------------------------------------------------------------------------------
// Il pulviscolo e le lucciole
// ---------------------------------------------------------------------------------------------

AValdorsoPulviscolo::AValdorsoPulviscolo()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sfera(PercorsoSfera);

	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;

	Polvere = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Polvere"));
	Polvere->SetupAttachment(Radice);
	Lucciole = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Lucciole"));
	Lucciole->SetupAttachment(Radice);

	for (UInstancedStaticMeshComponent* Componente : { Polvere, Lucciole })
	{
		if (Sfera.Succeeded())
		{
			Componente->SetStaticMesh(Sfera.Object);
		}
		SoloScena(Componente);
		Componente->SetMobility(EComponentMobility::Movable);
	}
}

void AValdorsoPulviscolo::Rimetti(FGranello& G, bool bOvunque)
{
	G.Posizione = FVector(
		Caso.FRandRange(-Spazio.X, Spazio.X),
		Caso.FRandRange(-Spazio.Y, Spazio.Y),
		bOvunque ? Caso.FRandRange(0.f, Spazio.Z) : Caso.FRandRange(0.f, Spazio.Z * 0.3f));
	G.Deriva = FVector(Caso.FRandRange(-6.f, 6.f), Caso.FRandRange(-6.f, 6.f), Caso.FRandRange(2.f, 9.f));
	G.Fase = Caso.FRandRange(0.f, 2.f * PI);
}

void AValdorsoPulviscolo::BeginPlay()
{
	Super::BeginPlay();
	Caso.Initialize(2026);

	Granelli.SetNum(FMath::Max(NumeroPolvere, 0));
	for (FGranello& G : Granelli)
	{
		Rimetti(G, true);
		G.Dimensione = Caso.FRandRange(0.008f, 0.016f);   // la sfera è di 100 cm: da 0,8 a 1,6 cm
	}
	Luci.SetNum(FMath::Max(NumeroLucciole, 0));
	for (FGranello& G : Luci)
	{
		Rimetti(G, true);
		G.Posizione.Z = Caso.FRandRange(40.f, 260.f);
		G.Deriva *= 2.f;
		G.Deriva.Z = Caso.FRandRange(-4.f, 4.f);
		G.Dimensione = Caso.FRandRange(0.022f, 0.032f);
	}

	Polvere->ClearInstances();
	for (const FGranello& G : Granelli)
	{
		Polvere->AddInstance(FTransform(FRotator::ZeroRotator, G.Posizione, FVector(G.Dimensione)));
	}
	Lucciole->ClearInstances();
	for (const FGranello& G : Luci)
	{
		Lucciole->AddInstance(FTransform(FRotator::ZeroRotator, G.Posizione, FVector(G.Dimensione)));
	}

	if (Materiale)
	{
		if (UMaterialInstanceDynamic* Viva = Polvere->CreateDynamicMaterialInstance(0, Materiale))
		{
			Viva->SetVectorParameterValue(ParametroColore, FLinearColor(1.f, 0.85f, 0.62f));
			Viva->SetScalarParameterValue(ParametroIntensita, BagliorPolvere);
		}
		if (UMaterialInstanceDynamic* Viva = Lucciole->CreateDynamicMaterialInstance(0, Materiale))
		{
			Viva->SetVectorParameterValue(ParametroColore, FLinearColor(1.f, 0.78f, 0.3f));
			Viva->SetScalarParameterValue(ParametroIntensita, BagliorLucciole);
		}
	}
}

void AValdorsoPulviscolo::Aggiorna(UInstancedStaticMeshComponent* Componente, const TArray<FGranello>& Elenco, bool bLampeggia)
{
	if (Componente == nullptr || Elenco.Num() == 0 || Componente->GetInstanceCount() != Elenco.Num())
	{
		return;
	}
	Trasformazioni.SetNum(Elenco.Num());
	for (int32 i = 0; i < Elenco.Num(); ++i)
	{
		const FGranello& G = Elenco[i];
		float Scala = G.Dimensione;
		if (bLampeggia)
		{
			// Una lucciola si accende piano, resta un attimo e si spegne.
			const float Onda = FMath::Max(0.f, FMath::Sin(Tempo * 0.9f + G.Fase * 3.f));
			Scala *= FMath::Max(Onda * Onda * Onda, 0.001f);
		}
		Trasformazioni[i] = FTransform(FRotator::ZeroRotator, G.Posizione, FVector(Scala));
	}
	Componente->BatchUpdateInstancesTransforms(0, Trasformazioni, false, true, true);
}

void AValdorsoPulviscolo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Tempo += DeltaSeconds;

	auto Muovi = [this, DeltaSeconds](TArray<FGranello>& Elenco, float Ampiezza)
	{
		for (FGranello& G : Elenco)
		{
			const FVector Ondeggio(FMath::Sin(Tempo * 0.31f + G.Fase), FMath::Cos(Tempo * 0.27f + G.Fase), FMath::Sin(Tempo * 0.5f + G.Fase) * 0.5f);
			G.Posizione += (G.Deriva + Ondeggio * Ampiezza) * DeltaSeconds;
			if (FMath::Abs(G.Posizione.X) > Spazio.X || FMath::Abs(G.Posizione.Y) > Spazio.Y
				|| G.Posizione.Z > Spazio.Z || G.Posizione.Z < 0.f)
			{
				Rimetti(G, false);
			}
		}
	};
	Muovi(Granelli, 8.f);
	Muovi(Luci, 30.f);

	Aggiorna(Polvere, Granelli, false);
	Aggiorna(Lucciole, Luci, true);
}

// ---------------------------------------------------------------------------------------------
// L'ombra dell'Orso
// ---------------------------------------------------------------------------------------------

AValdorsoOmbraOrso::AValdorsoOmbraOrso()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Piano(PercorsoPiano);

	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;

	Sagoma = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sagoma"));
	Sagoma->SetupAttachment(Radice);
	if (Piano.Succeeded())
	{
		Sagoma->SetStaticMesh(Piano.Object);
	}
	SoloScena(Sagoma);
	Sagoma->SetRelativeRotation(InPiedi);

	Corpo = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Corpo"));
	Corpo->SetupAttachment(Radice);
	Corpo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Corpo->SetCollisionProfileName(TEXT("NoCollision"));
	Corpo->SetCanEverAffectNavigation(false);
	Corpo->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void AValdorsoOmbraOrso::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// La figura è larga il doppio di quanto è alta.
	Sagoma->SetRelativeScale3D(FVector(Altezza / 100.f, Altezza * 2.f / 100.f, 1.f));
	Sagoma->SetRelativeRotation(InPiedi);
	Sagoma->SetRelativeLocation(FVector(0.f, 0.f, Altezza * 0.5f - 40.f));
	if (Materiale)
	{
		Sagoma->SetMaterial(0, Materiale);
	}

	// Con il modello animato la sagoma piatta non serve piu'.
	Corpo->SetSkeletalMeshAsset(Modello);
	Corpo->SetRelativeScale3D(FVector(ScalaModello));
	Corpo->SetRelativeRotation(FRotator(0.f, 90.f + GiroModello, 0.f));
	Corpo->SetVisibility(Modello != nullptr);
	Sagoma->SetVisibility(Modello == nullptr);
}

void AValdorsoOmbraOrso::BeginPlay()
{
	Super::BeginPlay();
	if (Materiale)
	{
		SagomaViva = Sagoma->CreateDynamicMaterialInstance(0, Materiale);
	}
	if (Modello && Camminata)
	{
		Corpo->PlayAnimation(Camminata, true);
		Corpo->SetPlayRate(VelocitaAnimazione);
	}
	Attesa = PrimaPassata;
	bInCammino = false;
	Posiziona(0.f, 0.f);
}

void AValdorsoOmbraOrso::Posiziona(float Y, float Opacita)
{
	// Il passo pesante: la sagoma sale e scende appena.
	if (Modello)
	{
		// Il modello vero: cammina sul terreno e guarda dove va; si vede solo mentre cammina.
		Corpo->SetRelativeLocation(FVector(0.f, Y, 0.f));
		Corpo->SetRelativeRotation(FRotator(0.f, (Verso > 0.f ? 90.f : -90.f) + GiroModello, 0.f));
		Corpo->SetVisibility(bInCammino);
		Sagoma->SetVisibility(false);
		return;
	}
	const float Passo = FMath::Abs(FMath::Sin(Tempo * 1.7f)) * 12.f;
	Sagoma->SetRelativeLocation(FVector(0.f, Y, Altezza * 0.5f - 40.f + Passo));
	Sagoma->SetVisibility(Opacita > 0.001f);
	if (SagomaViva)
	{
		SagomaViva->SetScalarParameterValue(ParametroOpacita, Opacita);
		SagomaViva->SetScalarParameterValue(ParametroVerso, Verso);
	}
}

void AValdorsoOmbraOrso::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Tempo += DeltaSeconds;

	if (!bInCammino)
	{
		Attesa -= DeltaSeconds;
		if (Attesa <= 0.f)
		{
			bInCammino = true;
			Avanzamento = 0.f;
			Verso = FMath::RandBool() ? 1.f : -1.f;
		}
		return;
	}

	Avanzamento += Velocita * DeltaSeconds / FMath::Max(Percorso, 100.f);
	if (Avanzamento >= 1.f)
	{
		bInCammino = false;
		Attesa = FMath::FRandRange(PausaMinima, FMath::Max(PausaMinima, PausaMassima));
		Posiziona(0.f, 0.f);
		return;
	}

	// Compare piano dalla nebbia, cammina, e piano sparisce.
	const float Opacita = FMath::Clamp(FMath::Min(Avanzamento, 1.f - Avanzamento) / 0.2f, 0.f, 1.f);
	const float Y = Verso * (-0.5f + Avanzamento) * Percorso;
	Posiziona(Y, FMath::SmoothStep(0.f, 1.f, Opacita));
}

// ---------------------------------------------------------------------------------------------
// I monti all'orizzonte
// ---------------------------------------------------------------------------------------------

AValdorsoMonti::AValdorsoMonti()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Piano(PercorsoPiano);

	Anello = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Anello"));
	RootComponent = Anello;
	if (Piano.Succeeded())
	{
		Anello->SetStaticMesh(Piano.Object);
	}
	SoloScena(Anello);
}

void AValdorsoMonti::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Pezzi di piano in piedi, in cerchio, tutti rivolti verso il centro della scena.
	// Ogni pezzo sa il suo numero (dato del pezzo 0): il materiale disegna le creste senza giunture.
	const int32 Numero = FMath::Clamp(Pezzi, 6, 96);
	const float Corda = 2.f * Raggio * FMath::Sin(PI / Numero) * 1.04f;

	Anello->ClearInstances();
	Anello->SetNumCustomDataFloats(1);
	for (int32 i = 0; i < Numero; ++i)
	{
		const float Angolo = 2.f * PI * i / Numero;
		const FVector Dove(Raggio * FMath::Cos(Angolo), Raggio * FMath::Sin(Angolo), Altezza * 0.5f - Sotto);
		const FRotator Giro(90.f, FMath::RadiansToDegrees(Angolo), 0.f);
		const int32 Indice = Anello->AddInstance(FTransform(Giro, Dove, FVector(Altezza / 100.f, Corda / 100.f, 1.f)));
		Anello->SetCustomDataValue(Indice, 0, static_cast<float>(i), false);
	}
	Anello->MarkRenderStateDirty();

	if (Materiale)
	{
		MontiVivi = UMaterialInstanceDynamic::Create(Materiale, this);
		MontiVivi->SetScalarParameterValue(ParametroPezzi, static_cast<float>(Numero));
		MontiVivi->SetScalarParameterValue(ParametroSeme, Seme);
		MontiVivi->SetVectorParameterValue(ParametroColore, Colore);
		Anello->SetMaterial(0, MontiVivi);
	}
}
