// Valdorso - Falò e bivacchi (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoFalo.h"
#include "Valdorso.h"
#include "ValdorsoMovementComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ValdorsoFalo"

namespace
{
	const FName FaloParametroColore(TEXT("Colore"));
	const FName FaloParametroIntensita(TEXT("Intensita"));
	const FName FaloParametroSeme(TEXT("Seme"));
	const FName FaloParametroColoreSemplice(TEXT("Color"));

	/** La luce media del fuoco e quanto sono alte e larghe le fiamme (cm). */
	constexpr float FaloLuceMedia = 60.f;
	constexpr float FaloAltezzaFiamma = 70.f;
	constexpr float FaloLarghezzaFiamma = 55.f;
	constexpr int32 FaloNumeroSassi = 8;

	/** L'orologio del server (uguale su tutti i PC entro il ritardo della rete). */
	double FaloOrologio(const UWorld* Mondo)
	{
		const AGameStateBase* Stato = Mondo ? Mondo->GetGameState() : nullptr;
		return Stato ? Stato->GetServerWorldTimeSeconds() : (Mondo ? Mondo->GetTimeSeconds() : 0.0);
	}

	FAutoConsoleCommandWithWorld ComandoFalo(
		TEXT("Valdorso.Falo"),
		TEXT("Per le prove: accende un falò davanti a ogni colono, senza consumare rami (sul server, o da soli)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* Mondo)
		{
			if (!Mondo)
			{
				return;
			}
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* Controllore = It->Get();
				if (Controllore && Controllore->HasAuthority() && Controllore->GetPawn())
				{
					const FText Esito = AValdorsoFalo::Accendi(Controllore->GetPawn());
					UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Falò: %s"),
						Esito.IsEmpty() ? TEXT("acceso") : *Esito.ToString());
				}
			}
		}));
}

namespace ValdorsoFaloRegole
{
	float Vita(float Rimasti)
	{
		if (Rimasti <= 0.f)
		{
			return 0.f;
		}
		return FMath::Clamp(Rimasti / Spegnimento, 0.f, 1.f);
	}
}

AValdorsoFalo::AValdorsoFalo()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetCanBeDamaged(false);
	// cambia di rado (quando si accende e quando si spegne: ForceNetUpdate); oltre 150 m, la distanza di serie, non si
	// manda in rete
	SetNetUpdateFrequency(1.f);

	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sfera(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Piano(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Semplice(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	MaterialeSemplice = Semplice.Succeeded() ? Semplice.Object : nullptr;
	UStaticMesh* FormaSfera = Sfera.Succeeded() ? Sfera.Object : nullptr;
	UStaticMesh* FormaCilindro = Cilindro.Succeeded() ? Cilindro.Object : nullptr;
	UStaticMesh* FormaPiano = Piano.Succeeded() ? Piano.Object : nullptr;

	// il cerchio di sassi
	for (int32 i = 0; i < FaloNumeroSassi; ++i)
	{
		const float Angolo = 2.f * PI * i / FaloNumeroSassi;
		const float Grandezza = 0.11f + 0.03f * FMath::Sin(i * 2.7f);
		Sassi.Add(Pezzo(*FString::Printf(TEXT("Sasso%d"), i), FormaSfera,
			FVector(40.f * FMath::Cos(Angolo), 40.f * FMath::Sin(Angolo), 3.f), FRotator(0.f, i * 37.f, 0.f),
			FVector(Grandezza * 1.2f, Grandezza, Grandezza * 0.75f)));
	}
	// tre ciocchi incrociati, coricati
	for (int32 i = 0; i < 3; ++i)
	{
		Ciocchi.Add(Pezzo(*FString::Printf(TEXT("Ciocco%d"), i), FormaCilindro, FVector(0.f, 0.f, 7.f + 2.f * i),
			FRotator(90.f, i * 60.f, 0.f), FVector(0.07f, 0.07f, 0.6f)));
	}
	Braci = Pezzo(TEXT("Braci"), FormaSfera, FVector(0.f, 0.f, 4.f), FRotator::ZeroRotator, FVector(0.38f, 0.38f, 0.06f));
	// tre lingue di fuoco incrociate a 60 gradi (come i bracieri del menu): da ogni lato sembra un fuoco pieno
	for (int32 i = 0; i < 3; ++i)
	{
		Fiamme.Add(Pezzo(*FString::Printf(TEXT("Fiamma%d"), i), FormaPiano, FVector(0.f, 0.f, 10.f + FaloAltezzaFiamma * 0.5f),
			FRotator(90.f, i * 60.f, 0.f), FVector(FaloAltezzaFiamma / 100.f, FaloLarghezzaFiamma / 100.f, 1.f)));
	}

	Luce = CreateDefaultSubobject<UPointLightComponent>(TEXT("Luce"));
	Luce->SetupAttachment(Radice);
	Luce->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Luce->SetLightColor(FLinearColor(1.f, 0.45f, 0.16f));
	Luce->SetIntensity(FaloLuceMedia);
	Luce->SetAttenuationRadius(900.f);
	Luce->SetCastShadows(false);
	Luce->SetMobility(EComponentMobility::Movable);
}

UStaticMeshComponent* AValdorsoFalo::Pezzo(const TCHAR* Nome, UStaticMesh* Forma, const FVector& Dove, const FRotator& Giro, const FVector& Scala)
{
	UStaticMeshComponent* Componente = CreateDefaultSubobject<UStaticMeshComponent>(Nome);
	Componente->SetupAttachment(Radice);
	if (Forma)
	{
		Componente->SetStaticMesh(Forma);
	}
	Componente->SetRelativeLocation(Dove);
	Componente->SetRelativeRotation(Giro);
	Componente->SetRelativeScale3D(Scala);
	// fa solo scena: niente urti (ci si cammina sopra), niente ombre
	Componente->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Componente->SetCollisionProfileName(TEXT("NoCollision"));
	Componente->SetCastShadow(false);
	Componente->SetCanEverAffectNavigation(false);
	return Componente;
}

void AValdorsoFalo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoFalo, SpentoAlle);
}

void AValdorsoFalo::BeginPlay()
{
	Super::BeginPlay();
	Seme = FMath::FRandRange(0.f, 100.f);
	// sul server dedicato nessuno lo guarda: niente materiali
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// i colori sulle forme di serie: sassi grigi, ciocchi scuri
	if (MaterialeSemplice)
	{
		int32 Numero = 0;
		for (UStaticMeshComponent* Sasso : Sassi)
		{
			if (UMaterialInstanceDynamic* Pietra = Sasso ? Sasso->CreateDynamicMaterialInstance(0, MaterialeSemplice) : nullptr)
			{
				const float Tono = 0.16f + 0.05f * FMath::Frac(Numero * 0.37f);
				Pietra->SetVectorParameterValue(FaloParametroColoreSemplice, FLinearColor(Tono, Tono * 0.97f, Tono * 0.9f));
			}
			++Numero;
		}
		for (UStaticMeshComponent* Ciocco : Ciocchi)
		{
			if (UMaterialInstanceDynamic* Legno = Ciocco ? Ciocco->CreateDynamicMaterialInstance(0, MaterialeSemplice) : nullptr)
			{
				Legno->SetVectorParameterValue(FaloParametroColoreSemplice, FLinearColor(0.07f, 0.04f, 0.02f));
			}
		}
	}
	// le braci e il fuoco: i materiali della scena del menu (Content/Menu); se mancano, braci arancio e niente fiamme
	UMaterialInterface* Luminoso = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Menu/M_Luminoso.M_Luminoso"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UMaterialInterface* Fuoco = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Menu/M_Fuoco2.M_Fuoco2"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Braci)
	{
		if (Luminoso)
		{
			BraciVive = Braci->CreateDynamicMaterialInstance(0, Luminoso);
			if (BraciVive)
			{
				BraciVive->SetVectorParameterValue(FaloParametroColore, FLinearColor(1.f, 0.32f, 0.08f));
			}
		}
		else if (MaterialeSemplice)
		{
			if (UMaterialInstanceDynamic* Arancio = Braci->CreateDynamicMaterialInstance(0, MaterialeSemplice))
			{
				Arancio->SetVectorParameterValue(FaloParametroColoreSemplice, FLinearColor(0.8f, 0.2f, 0.03f));
			}
		}
	}
	int32 Numero = 0;
	for (UStaticMeshComponent* Fiamma : Fiamme)
	{
		if (!Fiamma)
		{
			continue;
		}
		Fiamma->SetVisibility(Fuoco != nullptr);
		if (Fuoco)
		{
			if (UMaterialInstanceDynamic* Viva = Fiamma->CreateDynamicMaterialInstance(0, Fuoco))
			{
				Viva->SetScalarParameterValue(FaloParametroSeme, Seme + 17.f * Numero);
				FiammeVive.Add(Viva);
			}
		}
		++Numero;
	}
}

float AValdorsoFalo::Rimasti() const
{
	// prima che arrivi l'ora dal server (il primo istante sui PC) il fuoco è pieno
	if (SpentoAlle <= 0.0)
	{
		return ValdorsoFaloRegole::Durata;
	}
	return static_cast<float>(SpentoAlle - FaloOrologio(GetWorld()));
}

void AValdorsoFalo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Tempo += DeltaSeconds;
	const float Vita = ValdorsoFaloRegole::Vita(Rimasti());
	if (HasAuthority() && Vita <= 0.f && SpentoAlle > 0.0)
	{
		Destroy();
		return;
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// il fuoco non è regolare: tre onde diverse sommate (come i bracieri)
	const float Fuoco = 0.72f
		+ 0.16f * FMath::Sin(Tempo * 7.3f + Seme)
		+ 0.08f * FMath::Sin(Tempo * 13.7f + Seme * 2.f)
		+ 0.04f * FMath::Sin(Tempo * 23.1f + Seme * 3.f);
	Luce->SetIntensity(FaloLuceMedia * Fuoco * Vita);
	if (BraciVive)
	{
		BraciVive->SetScalarParameterValue(FaloParametroIntensita, 8.f * Fuoco * FMath::Max(0.3f, Vita));
	}
	for (UMaterialInstanceDynamic* Viva : FiammeVive)
	{
		if (Viva)
		{
			Viva->SetScalarParameterValue(FaloParametroIntensita, 2.5f * (0.6f + 0.55f * Fuoco) * Vita);
		}
	}
	// spegnendosi, le fiamme si abbassano
	const float Alte = FMath::Max(0.15f, Vita);
	for (UStaticMeshComponent* Fiamma : Fiamme)
	{
		if (Fiamma)
		{
			Fiamma->SetRelativeScale3D(FVector(FaloAltezzaFiamma / 100.f * Alte, FaloLarghezzaFiamma / 100.f, 1.f));
			Fiamma->SetRelativeLocation(FVector(0.f, 0.f, 10.f + FaloAltezzaFiamma * 0.5f * Alte));
		}
	}
}

FText AValdorsoFalo::Accendi(APawn* Chi)
{
	ACharacter* Personaggio = Cast<ACharacter>(Chi);
	UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
	if (!Personaggio || !Mondo || !Chi->HasAuthority())
	{
		return LOCTEXT("Niente", "Adesso non puoi accendere un fuoco.");
	}
	const UValdorsoMovementComponent* Movimento = Cast<UValdorsoMovementComponent>(Personaggio->GetCharacterMovement());
	if (Movimento && (Movimento->IsSwimming() || Movimento->ImmersioneNellaValle() > 0.05f))
	{
		return LOCTEXT("Acqua", "Nell'acqua il fuoco non si accende.");
	}
	if (Movimento && !Movimento->IsMovingOnGround())
	{
		return LOCTEXT("Terra", "Prima metti i piedi per terra.");
	}
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoFalo), false, Chi);
	TArray<AActor*> Attaccati;
	Chi->GetAttachedActors(Attaccati, true, true);
	Parametri.AddIgnoredActors(Attaccati);
	// all'aperto: sopra la testa niente tetto (come la pioggia di AValdorsoTempo)
	const FVector Testa = Chi->GetActorLocation() + FVector(0.f, 0.f, 80.f);
	FHitResult Colpo;
	if (Mondo->LineTraceSingleByChannel(Colpo, Testa, Testa + FVector(0.f, 0.f, 3000.f), ECC_Visibility, Parametri))
	{
		return LOCTEXT("Tetto", "Sotto un tetto no: il fumo non saprebbe dove andare. Esci all'aperto.");
	}
	// il punto, un metro e mezzo davanti: niente in mezzo, e per terra non troppo in pendenza
	const FVector Avanti = Chi->GetActorForwardVector().GetSafeNormal2D();
	const FVector Sopra = Chi->GetActorLocation() + Avanti * ValdorsoFaloRegole::Davanti;
	if (Mondo->LineTraceSingleByChannel(Colpo, Chi->GetActorLocation(), Sopra, ECC_Visibility, Parametri))
	{
		return LOCTEXT("Davanti", "C'è qualcosa davanti: fatti un po' di spazio.");
	}
	if (!Mondo->LineTraceSingleByChannel(Colpo, Sopra + FVector(0.f, 0.f, 100.f), Sopra - FVector(0.f, 0.f, 400.f), ECC_Visibility, Parametri))
	{
		return LOCTEXT("Vuoto", "Lì davanti non c'è terra su cui accenderlo.");
	}
	if (Colpo.ImpactNormal.Z < 0.8f)
	{
		return LOCTEXT("Pendenza", "Qui il terreno pende troppo: il fuoco rotolerebbe via.");
	}
	const FVector Punto = Colpo.ImpactPoint;
	AController* Controllore = Chi->GetController();
	// uno a testa: il falò di prima si spegne, ma solo quando quello nuovo è acceso davvero
	AValdorsoFalo* Vecchio = nullptr;
	for (TActorIterator<AValdorsoFalo> It(Mondo); It; ++It)
	{
		AValdorsoFalo* Altro = *It;
		if (!IsValid(Altro))
		{
			continue;
		}
		if (Controllore && Altro->Acceso.Get() == Controllore)
		{
			Vecchio = Altro;
			continue;
		}
		if (FVector::Dist2D(Altro->GetActorLocation(), Punto) < ValdorsoFaloRegole::DistanzaMinima)
		{
			return LOCTEXT("Vicino", "C'è già un fuoco qui accanto.");
		}
	}
	FActorSpawnParameters Come;
	Come.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AValdorsoFalo* Nuovo = Mondo->SpawnActor<AValdorsoFalo>(AValdorsoFalo::StaticClass(), Punto, FRotator(0.f, Chi->GetActorRotation().Yaw, 0.f), Come);
	if (!Nuovo)
	{
		return LOCTEXT("NonRiesce", "Il fuoco non prende. Riprova.");
	}
	Nuovo->Acceso = Controllore;
	Nuovo->SpentoAlle = FaloOrologio(Mondo) + ValdorsoFaloRegole::Durata;
	Nuovo->ForceNetUpdate();
	if (Vecchio)
	{
		Vecchio->Destroy();
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Falò acceso da %s in %s"), *Chi->GetName(), *Punto.ToString());
	return FText::GetEmpty();
}

bool AValdorsoFalo::CaldoVicino(const UWorld* Mondo, const FVector& Dove)
{
	if (!Mondo)
	{
		return false;
	}
	for (TActorIterator<AValdorsoFalo> It(Mondo); It; ++It)
	{
		const AValdorsoFalo* Fuoco = *It;
		if (IsValid(Fuoco) && ValdorsoFaloRegole::Vita(Fuoco->Rimasti()) > 0.f
			&& FVector::Dist(Fuoco->GetActorLocation(), Dove) < ValdorsoFaloRegole::RaggioCaldo)
		{
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
