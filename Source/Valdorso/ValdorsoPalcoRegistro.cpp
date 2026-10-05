// Valdorso - Il palco del Registro (vedi il .h).
// Unity build: le cose del namespace anonimo cominciano con Palco, così non si scontrano con gli altri file.

#include "ValdorsoPalcoRegistro.h"
#include "ValdorsoScenaMenu.h"
#include "ValdorsoTemaUI.h"
#include "Valdorso.h"
#include "Animation/AnimSequence.h"
#include "Components/PointLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "UObject/Package.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	TAutoConsoleVariable<float> CVarPalcoEsposizione(
		TEXT("Valdorso.Ritratto.Esposizione"),
		0.f,
		TEXT("Correzione dell'esposizione del ritratto nel Registro (0 di serie; +1 più chiaro, -1 più scuro)."));

	/** Larghezza e altezza del ritratto, in pixel (4:5, come un quadro). */
	constexpr int32 PalcoLarghezza = 800;
	constexpr int32 PalcoAltezza = 1000;

	/** La telecamera parte un poco più lontana e si avvicina piano, in secondi. */
	constexpr float PalcoAvvicinamento = 4.f;

	/** Il colore dell'elemento di una fede (gli stessi della schermata). */
	FLinearColor PalcoColoreFede(const FString& Fede)
	{
		if (Fede == TEXT("solara")) { return ValdorsoTema::OroChiaro(); }
		if (Fede == TEXT("ignar")) { return ValdorsoTema::Brace(); }
		if (Fede == TEXT("nereia")) { return ValdorsoTema::BluArcano(); }
		if (Fede == TEXT("torvald")) { return ValdorsoTema::Hex(TEXT("8B6B3E")); }
		if (Fede == TEXT("zefira")) { return ValdorsoTema::Hex(TEXT("A9CBDD")); }
		if (Fede == TEXT("vecchidei")) { return ValdorsoTema::VerdeVita(); }
		// Nessuna fede (o non ancora scelta): una luce di luna, fredda.
		return FLinearColor(0.45f, 0.55f, 0.75f);
	}

	UMaterialInterface* PalcoMateriale(const TCHAR* Percorso)
	{
		// I materiali della scena del menu (Content/Menu): se mancano, gli attori usano quello di serie.
		return LoadObject<UMaterialInterface>(nullptr, Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

AValdorsoPalcoRegistro::AValdorsoPalcoRegistro()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);

	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;

	// Il colono guarda verso la telecamera (+X): il manichino di Unreal guarda verso +Y, quindi lo si gira di -90.
	Colono = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Colono"));
	Colono->SetupAttachment(Radice);
	Colono->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Colono->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Colono->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Colono->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	// Lo vede solo la telecamera del palco, mai quella del gioco.
	Colono->SetVisibleInSceneCaptureOnly(true);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Quinn(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Fermo(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	CorpoUomo = Manny.Object;
	CorpoDonna = Quinn.Object;
	Riposo = Fermo.Object;

	Obiettivo = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Obiettivo"));
	Obiettivo->SetupAttachment(Radice);
	Obiettivo->FOVAngle = 28.f;
	Obiettivo->bCaptureEveryFrame = true;
	Obiettivo->bCaptureOnMovement = false;
	Obiettivo->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Obiettivo->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	// Niente cielo, nebbia e sole della mappa: solo le luci del palco.
	Obiettivo->ShowFlags.SetAtmosphere(false);
	Obiettivo->ShowFlags.SetFog(false);
	Obiettivo->ShowFlags.SetVolumetricFog(false);
	Obiettivo->ShowFlags.SetSkyLighting(false);
	Obiettivo->ShowFlags.SetDirectionalLights(false);
	Obiettivo->ShowFlags.SetCloud(false);
	// Profondità di campo da ritratto: il colono a fuoco, il frammento dietro un poco sfocato.
	Obiettivo->PostProcessSettings.bOverride_DepthOfFieldFstop = true;
	Obiettivo->PostProcessSettings.DepthOfFieldFstop = 2.2f;
	Obiettivo->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
	Obiettivo->PostProcessSettings.DepthOfFieldFocalDistance = 270.f;
	Obiettivo->PostProcessSettings.bOverride_VignetteIntensity = true;
	Obiettivo->PostProcessSettings.VignetteIntensity = 0.6f;
	// L'esposizione si adatta da sola (serve che la telecamera ricordi i fotogrammi di prima), in fretta;
	// se il ritratto è troppo chiaro o scuro: Valdorso.Ritratto.Esposizione nella console.
	Obiettivo->bAlwaysPersistRenderingState = true;
	Obiettivo->PostProcessSettings.bOverride_AutoExposureSpeedUp = true;
	Obiettivo->PostProcessSettings.AutoExposureSpeedUp = 8.f;
	Obiettivo->PostProcessSettings.bOverride_AutoExposureSpeedDown = true;
	Obiettivo->PostProcessSettings.AutoExposureSpeedDown = 8.f;
	Obiettivo->PostProcessSettings.bOverride_AutoExposureBias = true;
	Obiettivo->PostProcessSettings.AutoExposureBias = 0.f;

	// Luci mobili (nascono a gioco avviato), in candele come quelle della scena del menu.
	auto NuovaLuce = [this](const TCHAR* NomeLuce, const FVector& Dove, float Forza, float Raggio, const FLinearColor& Tinta, bool bOmbre)
	{
		UPointLightComponent* Luce = CreateDefaultSubobject<UPointLightComponent>(NomeLuce);
		Luce->SetupAttachment(Radice);
		Luce->SetRelativeLocation(Dove);
		Luce->Mobility = EComponentMobility::Movable;
		Luce->IntensityUnits = ELightUnits::Candelas;
		Luce->Intensity = Forza;
		Luce->AttenuationRadius = Raggio;
		Luce->LightColor = Tinta.ToFColor(true);
		Luce->CastShadows = bOmbre;
		return Luce;
	};
	LuceChiave = NuovaLuce(TEXT("LuceChiave"), FVector(170.f, -130.f, 190.f), 60.f, 900.f, FLinearColor(1.f, 0.62f, 0.32f), true);
	LuceFede = NuovaLuce(TEXT("LuceFede"), FVector(-120.f, 90.f, 210.f), 90.f, 700.f, FLinearColor(0.45f, 0.55f, 0.75f), true);
	LuceRiempimento = NuovaLuce(TEXT("LuceRiempimento"), FVector(220.f, 160.f, 120.f), 12.f, 800.f, FLinearColor(0.55f, 0.65f, 0.9f), false);

	ColoreFede = PalcoColoreFede(FString());
	ColoreFedeVoluto = ColoreFede;
}

void AValdorsoPalcoRegistro::BeginPlay()
{
	Super::BeginPlay();

	// Nel pacchetto temporaneo: lo tiene vivo la schermata finché si vede, anche se il palco sparisce prima.
	Ritratto = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
	Ritratto->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	Ritratto->ClearColor = FLinearColor::Black;
	Ritratto->InitAutoFormat(PalcoLarghezza, PalcoAltezza);
	Ritratto->UpdateResourceImmediate(true);
	Obiettivo->TextureTarget = Ritratto;

	PreparaScena();
	CambiaSesso(TEXT("uomo"));
	Obiettivo->ShowOnlyActors.Add(this);
	for (AActor* Pezzo : Scena)
	{
		Obiettivo->ShowOnlyActors.Add(Pezzo);
		// I pezzi della scena: visibili solo alla telecamera del palco, luci senza ombre (sono tante, ogni fotogramma).
		TArray<UPrimitiveComponent*> Forme;
		Pezzo->GetComponents<UPrimitiveComponent>(Forme);
		for (UPrimitiveComponent* Forma : Forme)
		{
			Forma->SetVisibleInSceneCaptureOnly(true);
			Forma->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		TArray<ULightComponent*> Luci;
		Pezzo->GetComponents<ULightComponent>(Luci);
		for (ULightComponent* LuceScena : Luci)
		{
			LuceScena->SetCastShadows(false);
		}
	}
}

void AValdorsoPalcoRegistro::PreparaScena()
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	UMaterialInterface* Rune = PalcoMateriale(TEXT("/Game/Menu/M_Rune2.M_Rune2"));
	UMaterialInterface* Luminoso = PalcoMateriale(TEXT("/Game/Menu/M_Luminoso.M_Luminoso"));
	UMaterialInterface* Pietra = PalcoMateriale(TEXT("/Game/Menu/M_Pietra.M_Pietra"));
	UMaterialInterface* Fuoco = PalcoMateriale(TEXT("/Game/Menu/M_Fuoco2.M_Fuoco2"));
	const FTransform Qui = GetActorTransform();

	FActorSpawnParameters Parametri;
	Parametri.Owner = this;
	Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Parametri.bDeferConstruction = true;

	// Il cerchio di rune sotto i piedi.
	if (AValdorsoCerchioRune* Cerchio = Mondo->SpawnActor<AValdorsoCerchioRune>(AValdorsoCerchioRune::StaticClass(), FTransform(FVector(0.f, 0.f, 1.f)) * Qui, Parametri))
	{
		Cerchio->Materiale = Rune;
		Cerchio->Diametro = 420.f;
		Cerchio->FinishSpawning(FTransform(FVector(0.f, 0.f, 1.f)) * Qui);
		Scena.Add(Cerchio);
	}

	// Il frammento del Cuore alle spalle, a destra di chi guarda.
	// (05/10, dopo la prima prova) Più lontano, più piccolo e meno abbagliante: prima copriva l'angolo del ritratto.
	const FTransform DoveFrammento = FTransform(FRotator::ZeroRotator, FVector(-420.f, -95.f, 175.f), FVector(0.55f)) * Qui;
	if (AValdorsoFrammentoCuore* Frammento = Mondo->SpawnActor<AValdorsoFrammentoCuore>(AValdorsoFrammentoCuore::StaticClass(), DoveFrammento, Parametri))
	{
		Frammento->Materiale = Luminoso;
		Frammento->BagliorQuiete = 0.25f;
		Frammento->BagliorColpo = 1.5f;
		Frammento->FinishSpawning(DoveFrammento);
		Scena.Add(Frammento);
	}

	// Due bracieri dietro, ai lati.
	for (const float Lato : { -1.f, 1.f })
	{
		const FTransform Dove = FTransform(FVector(-170.f, 210.f * Lato, 0.f)) * Qui;
		if (AValdorsoBraciere* Braciere = Mondo->SpawnActor<AValdorsoBraciere>(AValdorsoBraciere::StaticClass(), Dove, Parametri))
		{
			Braciere->MaterialePietra = Pietra;
			Braciere->MaterialeBraci = Luminoso;
			Braciere->MaterialeFuoco = Fuoco;
			Braciere->FinishSpawning(Dove);
			Scena.Add(Braciere);
		}
	}
}

void AValdorsoPalcoRegistro::CambiaSesso(const FString& Sesso)
{
	const FString Voluto = Sesso == TEXT("donna") ? FString(TEXT("donna")) : FString(TEXT("uomo"));
	if (Voluto == SessoAttuale)
	{
		return;
	}
	SessoAttuale = Voluto;
	USkeletalMesh* Corpo = Voluto == TEXT("donna") ? CorpoDonna.Get() : CorpoUomo.Get();
	if (!Corpo)
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Palco del Registro: manca il manichino (%s)"), *Voluto);
		return;
	}
	Colono->SetSkeletalMesh(Corpo);
	if (Riposo)
	{
		Colono->PlayAnimation(Riposo, true);
	}
}

void AValdorsoPalcoRegistro::Aggiorna(const FValdorsoRegistro& Risposte)
{
	if (!Risposte.Sesso.IsEmpty())
	{
		CambiaSesso(Risposte.Sesso);
	}
	ColoreFedeVoluto = PalcoColoreFede(Risposte.Fede);
}

void AValdorsoPalcoRegistro::Presenta(const FValdorsoRegistro& Risposte)
{
	// Il cambio di corpo si fa mentre è girato di lato, poi si volta verso chi guarda.
	SessoAttuale.Empty();
	CambiaSesso(Risposte.Sesso);
	ColoreFedeVoluto = PalcoColoreFede(Risposte.Fede);
	GiroDaFare = 50.f;
	Tempo = FMath::Min(Tempo, PalcoAvvicinamento * 0.6f);
}

void AValdorsoPalcoRegistro::Firmato()
{
	for (AActor* Pezzo : Scena)
	{
		if (AValdorsoFrammentoCuore* Frammento = Cast<AValdorsoFrammentoCuore>(Pezzo))
		{
			Frammento->Risveglia();
		}
		else if (AValdorsoCerchioRune* Cerchio = Cast<AValdorsoCerchioRune>(Pezzo))
		{
			Cerchio->Risveglia();
		}
	}
}

void AValdorsoPalcoRegistro::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Tempo += DeltaSeconds;

	// La telecamera si avvicina piano e poi respira appena.
	const float Avvicina = FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Tempo / PalcoAvvicinamento, 0.f, 1.f), 2.f);
	const float Distanza = FMath::Lerp(330.f, 270.f, Avvicina);
	const FVector Respiro(0.f, FMath::Sin(Tempo * 0.25f) * 4.f, FMath::Sin(Tempo * 0.18f) * 2.f);
	const FVector Occhio = FVector(Distanza, 0.f, 128.f) + Respiro;
	const FVector Guarda(0.f, 0.f, 112.f);
	Obiettivo->SetRelativeLocationAndRotation(Occhio, (Guarda - Occhio).Rotation());
	Obiettivo->PostProcessSettings.DepthOfFieldFocalDistance = Distanza;
	Obiettivo->PostProcessSettings.AutoExposureBias = CVarPalcoEsposizione.GetValueOnGameThread();

	// Il colono si volta verso la telecamera.
	GiroDaFare = FMath::FInterpTo(GiroDaFare, 0.f, DeltaSeconds, 4.f);
	Colono->SetRelativeRotation(FRotator(0.f, -90.f + GiroDaFare, 0.f));

	// La luce della fede cambia colore piano; la luce calda tremola come un fuoco, la luce della fede batte col Cuore.
	ColoreFede = FMath::Lerp(ColoreFede, ColoreFedeVoluto, FMath::Clamp(DeltaSeconds * 2.5f, 0.f, 1.f));
	LuceFede->SetLightColor(ColoreFede);
	const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
	LuceFede->SetIntensity(80.f + 40.f * Colpo);
	const float Fuoco = 0.85f + 0.1f * FMath::Sin(Tempo * 7.3f) + 0.05f * FMath::Sin(Tempo * 13.1f + 1.7f);
	LuceChiave->SetIntensity(60.f * Fuoco);
}

void AValdorsoPalcoRegistro::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AActor* Pezzo : Scena)
	{
		if (IsValid(Pezzo))
		{
			Pezzo->Destroy();
		}
	}
	Scena.Empty();
	Super::EndPlay(EndPlayReason);
}
