// Valdorso - Gli scheletri delle gallerie dei nani (vedi ValdorsoScheletri.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoScheletri.h"
#include "Valdorso.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoCrescita.h"
#include "ValdorsoFalo.h"
#include "ValdorsoGrottaNord.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "ValdorsoScheletri"

namespace
{
	const TCHAR* ScheletriCartella = TEXT("/Game/Valle/Grotta/Scheletri/");

	/** I pezzi, nell'ordine di AValdorsoScheletro::Pezzi. */
	enum EScheletriPezzo : int32
	{
		ScheletriTeschio,
		ScheletriTorace,
		ScheletriBacino,
		ScheletriBraccioS,
		ScheletriBraccioD,
		ScheletriGambaS,
		ScheletriGambaD,
		ScheletriSpada,
		ScheletriScudo,
		ScheletriNumeroPezzi,
	};

	const TCHAR* ScheletriNomiPezzi[ScheletriNumeroPezzi] = {
		TEXT("Teschio"), TEXT("Torace"), TEXT("Bacino"), TEXT("BraccioSinistro"), TEXT("BraccioDestro"), TEXT("GambaSinistra"),
		TEXT("GambaDestra"), TEXT("Spada"), TEXT("Scudo"),
	};

	/** Il modello di ogni pezzo, dopo il prefisso del tipo (le braccia e le gambe sono lo stesso, a destra e a sinistra). */
	const TCHAR* ScheletriModelli[ScheletriNumeroPezzi] = {
		TEXT("Teschio"), TEXT("Torace"), TEXT("Bacino"), TEXT("Braccio"), TEXT("Braccio"), TEXT("Gamba"), TEXT("Gamba"), TEXT("Spada"),
		TEXT("Scudo"),
	};

	/** In piedi: dove sta ogni pezzo (cm dai piedi), con le giunture in alto (le braccia e le gambe pendono). */
	const FVector ScheletriInPiedi[ScheletriNumeroPezzi] = {
		FVector(2.f, 0.f, 162.f), FVector(0.f, 0.f, 112.f), FVector(0.f, 0.f, 96.f), FVector(0.f, -21.f, 147.f), FVector(0.f, 21.f, 147.f),
		FVector(0.f, -10.f, 92.f), FVector(0.f, 10.f, 92.f), FVector(8.f, 0.f, -60.f), FVector(10.f, 0.f, -32.f),
	};

	UStaticMesh* ScheletriModello(const TCHAR* Nome)
	{
		const FString Percorso = FString(ScheletriCartella) + Nome + TEXT(".") + Nome;
		if (UStaticMesh* Trovato = LoadObject<UStaticMesh>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			return Trovato;
		}
		// l'importazione dei .glb puo' mettere il modello in una sottocartella (crea_grotta_nord.py lo rimette a posto)
		const FString Dentro = FString::Printf(TEXT("%s%s/StaticMeshes/%s.%s"), ScheletriCartella, Nome, Nome, Nome);
		return LoadObject<UStaticMesh>(nullptr, *Dentro, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	FValdorsoTipoScheletro ScheletriTipo(const TCHAR* Id, const TCHAR* ConArticolo, float Forza, float Salute, float Danno)
	{
		FValdorsoTipoScheletro T;
		T.Id = Id;
		T.ConArticolo = ConArticolo;
		T.Forza = Forza;
		T.Salute = Salute;
		T.Danno = Danno;
		return T;
	}

	TArray<FValdorsoTipoScheletro> ScheletriCreaTipi()
	{
		TArray<FValdorsoTipoScheletro> C;
		C.Add(ScheletriTipo(TEXT("Scheletro"), TEXT("lo scheletro"), 40.f, 80.f, 9.f));
		{
			FValdorsoTipoScheletro T = ScheletriTipo(TEXT("ScheletroScudo"), TEXT("lo scheletro con lo scudo"), 55.f, 120.f, 12.f);
			T.Parata = 0.5f;
			T.Equilibrio = 25.f;
			C.Add(T);
		}
		{
			FValdorsoTipoScheletro T = ScheletriTipo(TEXT("Capitano"), TEXT("il capitano delle ossa"), 70.f, 260.f, 19.f);
			T.Equilibrio = 40.f;
			T.SecondiColpo = 1.6f;
			T.Portata = 200.f;
			T.Corsa = 300.f;
			T.Grandezza = 1.18f;
			C.Add(T);
		}
		{
			// (11/11) il guardiano della forgia dei nani: di pietra, enorme, lento, non teme il fuoco. Uno solo.
			FValdorsoTipoScheletro T = ScheletriTipo(TEXT("Guardiano"), TEXT("il guardiano della forgia"), 80.f, 900.f, 38.f);
			T.Equilibrio = 60.f;
			T.SecondiColpo = 2.6f;
			T.Portata = 270.f;
			T.Passo = 80.f;
			T.Corsa = 210.f;
			T.Grandezza = 1.9f;
			T.Modello = TEXT("SM_Guardiano_");
			T.Pietra = 0.6f;
			T.bTemeFuoco = false;
			C.Add(T);
		}
		return C;
	}

	APawn* ScheletriColono(AController* Controllore)
	{
		APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
		if (!Corpo || !Corpo->IsPlayerControlled())
		{
			return nullptr;
		}
		const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
		return (Pericoli && Pericoli->Caduto()) ? nullptr : Corpo;
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoScheletriChiama(
		TEXT("Valdorso.Scheletri.Chiama"),
		TEXT("Per le prove: Valdorso.Scheletri.Chiama [Scheletro | ScheletroScudo | Capitano] (sveglio, a cinque metri davanti a te)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			if (!Mondo || Mondo->GetNetMode() == NM_Client)
			{
				return;
			}
			const FName Quale = Argomenti.Num() > 0 ? FName(*Argomenti[0]) : FName(TEXT("Scheletro"));
			if (!ValdorsoScheletri::Tipo(Quale))
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Scheletri: tipo sconosciuto (Scheletro, ScheletroScudo, Capitano)"));
				return;
			}
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				const APawn* Corpo = It->Get() ? It->Get()->GetPawn() : nullptr;
				if (!Corpo)
				{
					continue;
				}
				const FVector Dove = Corpo->GetActorLocation() + Corpo->GetActorForwardVector().GetSafeNormal2D() * 500.f;
				const FTransform Posto(FRotator(0.f, Corpo->GetActorRotation().Yaw + 180.f, 0.f), Dove);
				AValdorsoScheletro* Nuovo = Mondo->SpawnActorDeferred<AValdorsoScheletro>(AValdorsoScheletro::StaticClass(), Posto, nullptr, nullptr,
					ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
				if (Nuovo)
				{
					Nuovo->Prepara(Quale, Dove, true);
					Nuovo->FinishSpawning(Posto);
				}
				break;
			}
		}),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ComandoScheletriVia(
		TEXT("Valdorso.Scheletri.Via"),
		TEXT("Per le prove: tutti gli scheletri via (gli ossari li rifanno dopo i loro minuti)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Mondo)
		{
			AValdorsoOssario::ViaTutti(Mondo);
		}),
		ECVF_Cheat);
}

namespace ValdorsoScheletri
{
	TArrayView<const FValdorsoTipoScheletro> Tipi()
	{
		static const TArray<FValdorsoTipoScheletro> Tutti = ScheletriCreaTipi();
		return Tutti;
	}

	const FValdorsoTipoScheletro* Tipo(FName Id)
	{
		for (const FValdorsoTipoScheletro& T : Tipi())
		{
			if (Id == FName(T.Id))
			{
				return &T;
			}
		}
		return nullptr;
	}

	float Debolezza(EValdorsoTipoColpo Colpo)
	{
		switch (Colpo)
		{
		case EValdorsoTipoColpo::Botta: return 1.5f;
		case EValdorsoTipoColpo::Lama: return 0.7f;
		case EValdorsoTipoColpo::Pugno: return 0.6f;
		case EValdorsoTipoColpo::Morso: return 0.4f;
		default: return 1.f;
		}
	}

	EValdorsoStatoScheletro Prossimo(EValdorsoStatoScheletro Attuale, float Distanza, bool bFerito, bool bProtetto, float SecondiNelloStato,
		float DistanzaDaCasa)
	{
		switch (Attuale)
		{
		case EValdorsoStatoScheletro::Dorme:
			return (bFerito || (Distanza >= 0.f && Distanza <= Sveglia)) ? EValdorsoStatoScheletro::SiAlza : Attuale;
		case EValdorsoStatoScheletro::SiAlza:
			return SecondiNelloStato >= SecondiPerAlzarsi ? EValdorsoStatoScheletro::Caccia : Attuale;
		case EValdorsoStatoScheletro::Caccia:
		case EValdorsoStatoScheletro::Colpisce:
			if (Distanza < 0.f || Distanza > Lascia)
			{
				return EValdorsoStatoScheletro::Torna;
			}
			return Attuale;
		case EValdorsoStatoScheletro::Torna:
			if (Distanza >= 0.f && Distanza <= Sveglia * 0.6f && !bProtetto)
			{
				return EValdorsoStatoScheletro::Caccia;
			}
			return DistanzaDaCasa < 150.f ? EValdorsoStatoScheletro::Dorme : Attuale;
		default:
			return Attuale;
		}
	}
}

// ----------------------------------------------------------------------------------------------- lo scheletro

AValdorsoScheletro::AValdorsoScheletro()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetUpdateFrequency(15.f);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(32.f, 88.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetVisibility(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UCharacterMovementComponent* Movimento = GetCharacterMovement();
	Movimento->bOrientRotationToMovement = true;
	Movimento->RotationRate = FRotator(0.f, 300.f, 0.f);
	Movimento->MaxWalkSpeed = 110.f;
	Movimento->MaxStepHeight = 40.f;
	bUseControllerRotationYaw = false;

	Abilita = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Abilita"));
	Abilita->SetIsReplicated(true);
	Abilita->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Attributi = CreateDefaultSubobject<UValdorsoAttributeSet>(TEXT("Attributi"));
	Reazioni = CreateDefaultSubobject<UValdorsoReazioniComponent>(TEXT("Reazioni"));

	Figura = CreateDefaultSubobject<USceneComponent>(TEXT("Figura"));
	Figura->SetupAttachment(GetMesh());
	for (int32 i = 0; i < ScheletriNumeroPezzi; ++i)
	{
		UStaticMeshComponent* Pezzo = CreateDefaultSubobject<UStaticMeshComponent>(ScheletriNomiPezzi[i]);
		// la spada nella mano destra, lo scudo nella sinistra (attaccati alle braccia)
		if (i == ScheletriSpada || i == ScheletriScudo)
		{
			Pezzo->SetupAttachment(Pezzi.IsValidIndex(i == ScheletriSpada ? ScheletriBraccioD : ScheletriBraccioS)
				? Pezzi[i == ScheletriSpada ? ScheletriBraccioD : ScheletriBraccioS].Get() : Figura.Get());
		}
		else
		{
			Pezzo->SetupAttachment(Figura);
		}
		Pezzo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pezzo->SetGenerateOverlapEvents(false);
		Pezzo->SetCanEverAffectNavigation(false);
		Pezzo->CastShadow = true;
		Pezzi.Add(Pezzo);
	}
}

void AValdorsoScheletro::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoScheletro, Tipo);
	DOREPLIFETIME(AValdorsoScheletro, Stato);
	DOREPLIFETIME(AValdorsoScheletro, bRovistato);
}

const FValdorsoTipoScheletro* AValdorsoScheletro::GetTipo() const
{
	const FValdorsoTipoScheletro* T = ValdorsoScheletri::Tipo(Tipo);
	return T ? T : ValdorsoScheletri::Tipo(TEXT("Scheletro"));
}

float AValdorsoScheletro::Forza() const
{
	const FValdorsoTipoScheletro* T = GetTipo();
	return T ? T->Forza : 40.f;
}

float AValdorsoScheletro::Debolezza(EValdorsoTipoColpo Colpo) const
{
	const float Base = ValdorsoScheletri::Debolezza(Colpo);
	const FValdorsoTipoScheletro* T = GetTipo();
	// (11/11) la pietra: le lame e i pugni rimbalzano, le botte un po' meno
	if (T && T->Pietra > 0.f && (Colpo == EValdorsoTipoColpo::Lama || Colpo == EValdorsoTipoColpo::Pugno))
	{
		return Base * (1.f - T->Pietra);
	}
	return Base;
}

void AValdorsoScheletro::Prepara(FName QualeTipo, const FVector& DoveDorme, bool bSveglio)
{
	Tipo = QualeTipo;
	Casa = DoveDorme;
	Stato = bSveglio ? EValdorsoStatoScheletro::Caccia : EValdorsoStatoScheletro::Dorme;
}

void AValdorsoScheletro::BeginPlay()
{
	Super::BeginPlay();
	Abilita->InitAbilityActorInfo(this, this);
	Vesti();
	const FValdorsoTipoScheletro* T = GetTipo();
	if (HasAuthority() && T)
	{
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteMaxAttribute(), T->Salute);
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute(), T->Salute);
		if (Casa.IsZero())
		{
			Casa = GetActorLocation();
		}
		InizioStato = GetWorld()->GetTimeSeconds();
	}
	InizioStatoQui = GetWorld()->GetTimeSeconds();
	Mucchio = Stato == EValdorsoStatoScheletro::Dorme || Stato == EValdorsoStatoScheletro::Morto ? 1.f : 0.f;
}

void AValdorsoScheletro::OnRep_Tipo()
{
	Vesti();
}

void AValdorsoScheletro::Vesti()
{
	if (Vestito == Tipo)
	{
		return;
	}
	Vestito = Tipo;
	const FValdorsoTipoScheletro* T = GetTipo();
	const float Grande = T ? T->Grandezza : 1.f;
	GetCapsuleComponent()->SetCapsuleSize(32.f * Grande, 88.f * Grande);
	Figura->SetRelativeLocation(FVector(0.f, 0.f, -88.f * Grande));
	Figura->SetRelativeScale3D(FVector(Grande * (Tipo == FName(TEXT("Capitano")) ? 1.f : FMath::FRandRange(0.95f, 1.05f))));
	// le due pose: in piedi e il mucchio d'ossa (un po' diverso per ognuno)
	PosaInPiedi.SetNum(ScheletriNumeroPezzi);
	PosaMucchio.SetNum(ScheletriNumeroPezzi);
	FRandomStream Caso(static_cast<int32>(GetUniqueID()));
	for (int32 i = 0; i < ScheletriNumeroPezzi; ++i)
	{
		PosaInPiedi[i] = FTransform(FRotator::ZeroRotator, ScheletriInPiedi[i]);
		if (i == ScheletriSpada || i == ScheletriScudo)
		{
			// attaccati al braccio: restano dove sono rispetto alla mano
			PosaMucchio[i] = PosaInPiedi[i];
			continue;
		}
		const FVector Sparso(Caso.FRandRange(-45.f, 45.f), Caso.FRandRange(-45.f, 45.f), Caso.FRandRange(4.f, 16.f));
		const FRotator Girato(Caso.FRandRange(60.f, 100.f) * (Caso.FRand() < 0.5f ? 1.f : -1.f), Caso.FRandRange(0.f, 360.f), Caso.FRandRange(-40.f, 40.f));
		PosaMucchio[i] = FTransform(Girato, Sparso);
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const TCHAR* Prefisso = T ? T->Modello : TEXT("SM_Scheletro_");
	for (int32 i = 0; i < Pezzi.Num() && i < ScheletriNumeroPezzi; ++i)
	{
		const FString NomeModello = FString(Prefisso) + ScheletriModelli[i];
		Pezzi[i]->SetStaticMesh(ScheletriModello(*NomeModello));
	}
	// lo scudo solo a chi lo porta; il capitano ha una spada piu' grande
	if (Pezzi.Num() >= ScheletriNumeroPezzi)
	{
		Pezzi[ScheletriScudo]->SetVisibility(T && T->Parata > 0.f);
		Pezzi[ScheletriSpada]->SetRelativeScale3D(FVector(Tipo == FName(TEXT("Capitano")) ? 1.35f : 1.f));
	}
}

void AValdorsoScheletro::Ferito(AActor* Da)
{
	if (!HasAuthority() || !Vivo())
	{
		return;
	}
	UltimoAggressore = Da;
	bFerito = true;
	if (APawn* Chi = Cast<APawn>(Da); Chi && Chi->IsPlayerControlled())
	{
		Preda = Chi;
	}
}

void AValdorsoScheletro::Stordisci(float Secondi)
{
	if (!HasAuthority() || !Vivo() || Secondi <= 0.f)
	{
		return;
	}
	StorditoFino = FMath::Max(StorditoFino, GetWorld()->GetTimeSeconds() + Secondi);
	GetCharacterMovement()->StopMovementImmediately();
}

APawn* AValdorsoScheletro::CercaPreda() const
{
	APawn* Migliore = nullptr;
	float Vicino = ValdorsoScheletri::Lascia;
	if (APawn* Attuale = Preda.Get())
	{
		const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Attuale);
		const float Distanza = FVector::Dist(Attuale->GetActorLocation(), GetActorLocation());
		if (!(Pericoli && Pericoli->Caduto()) && Distanza <= ValdorsoScheletri::Lascia)
		{
			Migliore = Attuale;
			Vicino = Distanza;
		}
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Colono = ScheletriColono(It->Get());
		if (!Colono || Colono == Migliore)
		{
			continue;
		}
		const float Distanza = FVector::Dist(Colono->GetActorLocation(), GetActorLocation());
		if (Distanza < (Migliore ? Vicino * 0.5f : Vicino))
		{
			Migliore = Colono;
			Vicino = Distanza;
		}
	}
	return Migliore;
}

void AValdorsoScheletro::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		Pensa(DeltaSeconds);
		if (GetWorld()->GetTimeSeconds() >= StorditoFino)
		{
			Muoviti(DeltaSeconds);
		}
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		Anima(DeltaSeconds);
	}
}

void AValdorsoScheletro::Cambia(EValdorsoStatoScheletro Nuovo)
{
	if (Nuovo == Stato)
	{
		return;
	}
	Stato = Nuovo;
	InizioStato = GetWorld()->GetTimeSeconds();
	bColpoDato = false;
	if (Stato == EValdorsoStatoScheletro::Dorme)
	{
		// a casa: torna intero
		bFerito = false;
		const FValdorsoTipoScheletro* T = GetTipo();
		if (T)
		{
			Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute(), T->Salute);
		}
	}
	OnRep_Stato();
}

void AValdorsoScheletro::OnRep_Stato()
{
	InizioStatoQui = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (!HasActorBegunPlay() || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (Stato == EValdorsoStatoScheletro::SiAlza)
	{
		Suona(TEXT("S_Scheletro_Sveglia"));
	}
	else if (Stato == EValdorsoStatoScheletro::Morto)
	{
		Suona(TEXT("S_Scheletro_Crollo"));
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
}

void AValdorsoScheletro::Suona(const TCHAR* Nome) const
{
	USoundBase* Suono = ValdorsoCombattimento::SuonoPericoli(Nome);
	if (!Suono)
	{
		// (finche' crea_grotta_nord.py non ha messo i suoni: le ossa suonano come una botta)
		Suono = ValdorsoCombattimento::SuonoPericoli(TEXT("S_Colpo_Botta"));
	}
	ValdorsoCombattimento::Suona(GetWorld(), Suono, GetActorLocation(), 2500.f, 0.8f, FMath::FRandRange(0.6f, 0.8f));
}

void AValdorsoScheletro::Pensa(float DeltaSeconds)
{
	UWorld* Mondo = GetWorld();
	const FValdorsoTipoScheletro* T = GetTipo();
	if (!Vivo() || !T)
	{
		return;
	}
	const double OraMondo = Mondo->GetTimeSeconds();
	float Salute = 0.f;
	float Massima = 1.f;
	ValdorsoCombattimento::Salute(this, Salute, Massima);
	if (Salute <= 0.f)
	{
		Muori();
		return;
	}
	if (OraMondo >= ProssimaRicerca)
	{
		ProssimaRicerca = OraMondo + 0.4;
		Preda = CercaPreda();
		const APawn* Chi = Preda.Get();
		// temono il fuoco (e il battito del tempio)
		const FValdorsoTipoScheletro* SuoTipo = GetTipo();
		bProtetto = Chi && (!SuoTipo || SuoTipo->bTemeFuoco) && (AValdorsoFalo::CaldoVicino(Mondo, Chi->GetActorLocation())
			|| ValdorsoCombattimento::InPace(Mondo, Chi->GetActorLocation()));
	}
	const APawn* Chi = Preda.Get();
	const float Distanza = Chi ? static_cast<float>(FVector::Dist(Chi->GetActorLocation(), GetActorLocation())) : -1.f;
	const EValdorsoStatoScheletro Nuovo = ValdorsoScheletri::Prossimo(Stato, Distanza, bFerito, bProtetto,
		static_cast<float>(OraMondo - InizioStato), static_cast<float>(FVector::Dist2D(GetActorLocation(), Casa)));
	if (Nuovo != Stato)
	{
		Cambia(Nuovo);
	}
	if (OraMondo < StorditoFino)
	{
		return;
	}
	// il colpo: vicino abbastanza, non troppo spesso, e non contro chi sta vicino al fuoco
	if (Stato == EValdorsoStatoScheletro::Caccia && Chi && !bProtetto && Distanza <= T->Portata && OraMondo >= ProssimoColpo)
	{
		ProssimoColpo = OraMondo + T->SecondiColpo;
		Cambia(EValdorsoStatoScheletro::Colpisce);
	}
	if (Stato == EValdorsoStatoScheletro::Colpisce)
	{
		const float Dentro = static_cast<float>(OraMondo - InizioStato);
		if (!bColpoDato && Dentro >= 0.35f)
		{
			bColpoDato = true;
			if (Chi && Distanza <= T->Portata * 1.2f)
			{
				const FVector Punto = Chi->GetActorLocation() + (GetActorLocation() - Chi->GetActorLocation()).GetSafeNormal() * 30.f;
				ValdorsoCombattimento::Colpisci(this, const_cast<APawn*>(Chi), T->Danno, T->Equilibrio, Punto, EValdorsoTipoColpo::Lama);
			}
		}
		if (Dentro >= 0.7f)
		{
			Cambia(EValdorsoStatoScheletro::Caccia);
		}
	}
}

void AValdorsoScheletro::Muoviti(float DeltaSeconds)
{
	UCharacterMovementComponent* Movimento = GetCharacterMovement();
	const FValdorsoTipoScheletro* T = GetTipo();
	if (!Movimento || !Vivo() || !T)
	{
		return;
	}
	const double OraMondo = GetWorld()->GetTimeSeconds();
	const FVector Qui = GetActorLocation();
	const APawn* Chi = Preda.Get();
	FVector Verso = FVector::ZeroVector;
	float Velocita = T->Passo;
	switch (Stato)
	{
	case EValdorsoStatoScheletro::Caccia:
		if (Chi)
		{
			const FVector Da = Chi->GetActorLocation() - Qui;
			const float Distanza = static_cast<float>(Da.Size2D());
			if (bProtetto)
			{
				// resta lontano dal fuoco: avanti fino a quattro metri, indietro se e' troppo vicino
				Verso = Distanza > ValdorsoScheletri::LontanoDalFuoco ? Da : (Distanza < ValdorsoScheletri::LontanoDalFuoco * 0.8f ? -Da : FVector::ZeroVector);
			}
			else if (Distanza > T->Portata * 0.8f)
			{
				Verso = Da;
				Velocita = Distanza > 600.f ? T->Corsa : T->Passo * 1.4f;
			}
		}
		break;
	case EValdorsoStatoScheletro::Torna:
		Verso = Casa - Qui;
		break;
	default:
		break;
	}
	Verso.Z = 0.f;
	Movimento->MaxWalkSpeed = Velocita;
	if (Verso.SizeSquared() < 40.f * 40.f)
	{
		// fermo: guarda chi ha davanti
		if (Chi && (Stato == EValdorsoStatoScheletro::Caccia || Stato == EValdorsoStatoScheletro::Colpisce))
		{
			const FVector Da = (Chi->GetActorLocation() - Qui).GetSafeNormal2D();
			if (!Da.IsNearlyZero())
			{
				SetActorRotation(FMath::RInterpTo(GetActorRotation(), Da.Rotation(), DeltaSeconds, 6.f));
			}
		}
		return;
	}
	if (OraMondo < ScansaFino)
	{
		Verso = Scansa;
	}
	else if (GetVelocity().Size2D() < 20.f)
	{
		Fermo += DeltaSeconds;
		if (Fermo > 1.2)
		{
			Fermo = 0.0;
			ScansaFino = OraMondo + 1.0;
			Scansa = FVector::CrossProduct(Verso.GetSafeNormal(), FVector::UpVector) * (FMath::RandBool() ? 1.f : -1.f);
		}
	}
	else
	{
		Fermo = 0.0;
	}
	AddMovementInput(Verso.GetSafeNormal());
}

void AValdorsoScheletro::Muori()
{
	Cambia(EValdorsoStatoScheletro::Morto);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	SetLifeSpan(ValdorsoScheletri::SecondiOssa);
	const FValdorsoTipoScheletro* T = GetTipo();
	ValdorsoRegistroEventi::Annota(TEXT("Scheletro"), UltimoAggressore.Get(), this, GetActorLocation(),
		FString::Printf(TEXT("%s distrutto"), T ? T->Id : TEXT("scheletro")));
}

FText AValdorsoScheletro::TestoInterazione(const APawn* Chi) const
{
	// il capitano resta da guardare per tutti: al collo ha l'anello della storia di Lucia (uno per colono)
	if (Vivo() || (bRovistato && Tipo != FName(TEXT("Capitano"))))
	{
		return FText::GetEmpty();
	}
	return LOCTEXT("Rovista", "Rovista tra le ossa");
}

void AValdorsoScheletro::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || Vivo())
	{
		return;
	}
	AValdorsoPlayerController* Controllore = Chi ? Cast<AValdorsoPlayerController>(Chi->GetController()) : nullptr;
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
	if (!Zaino)
	{
		return;
	}
	const bool bCapitano = Tipo == FName(TEXT("Capitano"));
	if (bCapitano)
	{
		// la storia di Lucia: il capitano porta al collo l'anello della Gilda (ValdorsoGrottaNord.h). Per ogni colono,
		// anche dopo che un altro ha gia' rovistato (e se lo zaino e' pieno si puo' tornare a prenderlo)
		ValdorsoGrottaNord::DaiTraccia(Chi, TEXT("Anello_Ranieri"));
	}
	if (bRovistato)
	{
		return;
	}
	bRovistato = true;
	FRandomStream Caso(static_cast<int32>(FPlatformTime::Cycles()));
	const bool bGuardiano = Tipo == FName(TEXT("Guardiano"));
	const int32 Monete = bGuardiano ? Caso.RandRange(10, 16) : (bCapitano ? Caso.RandRange(4, 7) : Caso.RandRange(1, 3));
	Zaino->Dai(TEXT("Moneta_Nani"), Monete);
	if (Tipo == FName(TEXT("ScheletroScudo")) && Caso.FRand() < 0.12f)
	{
		Zaino->Dai(TEXT("Pugnale_Nani"), 1);
	}
	if (bCapitano)
	{
		if (Caso.FRand() < 0.35f)
		{
			Zaino->Dai(TEXT("Pugnale_Nani"), 1);
		}
	}
	if (bGuardiano)
	{
		// (11/11) il cuore di pietra che lo muoveva, e a volte il suo martello
		Zaino->Dai(TEXT("Cuore_di_Pietra"), 1);
		if (Caso.FRand() < 0.5f)
		{
			Zaino->Dai(TEXT("Martello_Nani"), 1);
		}
	}
	ValdorsoRegistroEventi::Annota(TEXT("Rovista"), Chi, this, GetActorLocation(), FString::Printf(TEXT("%s: %d monete"), *Tipo.ToString(), Monete));
	// le ossa del capitano restano i loro due minuti (l'anello, per chi arriva dopo); le altre spariscono presto
	SetLifeSpan((bCapitano || bGuardiano) ? ValdorsoScheletri::SecondiOssa : 8.f);
}

void AValdorsoScheletro::Anima(float DeltaSeconds)
{
	if (Pezzi.Num() < ScheletriNumeroPezzi || PosaInPiedi.Num() < ScheletriNumeroPezzi)
	{
		return;
	}
	const UWorld* Mondo = GetWorld();
	const double Ora = Mondo->GetTimeSeconds();
	const float Dentro = static_cast<float>(Ora - InizioStatoQui);
	// il mucchio: 1 addormentato o morto, 0 in piedi; si alza in due secondi, crolla in mezzo
	float Voluto = 0.f;
	switch (Stato)
	{
	case EValdorsoStatoScheletro::Dorme:
	case EValdorsoStatoScheletro::Morto:
		Voluto = 1.f;
		break;
	case EValdorsoStatoScheletro::SiAlza:
		Voluto = FMath::Clamp(1.f - Dentro / ValdorsoScheletri::SecondiPerAlzarsi, 0.f, 1.f);
		break;
	default:
		Voluto = 0.f;
		break;
	}
	Mucchio = Voluto > Mucchio ? FMath::FInterpConstantTo(Mucchio, Voluto, DeltaSeconds, 2.5f) : Voluto;
	// il passo: le gambe avanti e indietro, le braccia al contrario
	const float Veloce = static_cast<float>(GetVelocity().Size2D());
	Passo = FMath::Fmod(Passo + DeltaSeconds * Veloce / 28.f, 2.f * PI);
	const float Ampiezza = FMath::Clamp(Veloce / 250.f, 0.f, 1.f) * 30.f;
	// il colpo: il braccio destro su (fino a 0,35 s) poi giu' di colpo
	float BraccioColpo = 0.f;
	if (Stato == EValdorsoStatoScheletro::Colpisce)
	{
		BraccioColpo = Dentro < 0.35f ? -140.f * (Dentro / 0.35f) : FMath::Lerp(-140.f, 30.f, FMath::Clamp((Dentro - 0.35f) / 0.15f, 0.f, 1.f));
	}
	const float Respiro = FMath::Sin(static_cast<float>(Ora) * 1.7f + static_cast<float>(GetUniqueID() % 7)) * 2.f;
	for (int32 i = 0; i < ScheletriNumeroPezzi; ++i)
	{
		FTransform Posa = PosaInPiedi[i];
		if (Mucchio < 0.999f)
		{
			FRotator Giro = FRotator::ZeroRotator;
			switch (i)
			{
			case ScheletriTeschio: Giro = FRotator(Respiro - 4.f, FMath::Sin(static_cast<float>(Ora) * 0.6f) * 10.f, 0.f); break;
			case ScheletriBraccioS: Giro = FRotator(FMath::Sin(Passo + PI) * Ampiezza, 0.f, -6.f); break;
			case ScheletriBraccioD: Giro = FRotator(BraccioColpo != 0.f ? BraccioColpo : FMath::Sin(Passo) * Ampiezza, 0.f, 6.f); break;
			case ScheletriGambaS: Giro = FRotator(FMath::Sin(Passo) * Ampiezza, 0.f, 0.f); break;
			case ScheletriGambaD: Giro = FRotator(FMath::Sin(Passo + PI) * Ampiezza, 0.f, 0.f); break;
			case ScheletriTorace: Giro = FRotator(Respiro, 0.f, 0.f); break;
			default: break;
			}
			Posa.SetRotation(Giro.Quaternion());
		}
		if (Mucchio > 0.001f && i != ScheletriSpada && i != ScheletriScudo)
		{
			const FTransform& Giu = PosaMucchio[i];
			Posa.SetLocation(FMath::Lerp(Posa.GetLocation(), Giu.GetLocation(), Mucchio));
			Posa.SetRotation(FQuat::Slerp(Posa.GetRotation(), Giu.GetRotation(), Mucchio));
		}
		Pezzi[i]->SetRelativeLocationAndRotation(Posa.GetLocation(), Posa.GetRotation());
	}
}

// ----------------------------------------------------------------------------------------------- l'ossario

AValdorsoOssario::AValdorsoOssario()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 2.f;
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Punto"));
	SetActorHiddenInGame(true);
}

void AValdorsoOssario::BeginPlay()
{
	Super::BeginPlay();
	// non si replica e sta nel livello: sui PC dei coloni HasAuthority() sarebbe vero. Gli scheletri li fa solo il server
	if (GetNetMode() == NM_Client)
	{
		SetActorTickEnabled(false);
		return;
	}
	FaiNascere();
}

void AValdorsoOssario::FaiNascere()
{
	UWorld* Mondo = GetWorld();
	if (!Mondo || Mondo->GetNetMode() == NM_Client || !ValdorsoScheletri::Tipo(TipoScheletro))
	{
		return;
	}
	bNati = true;
	Vivi.RemoveAll([](const TWeakObjectPtr<AValdorsoScheletro>& S) { return !S.IsValid(); });
	for (int32 k = Vivi.Num(); k < Quanti; ++k)
	{
		const float Angolo = 2.f * PI * static_cast<float>(k) / static_cast<float>(FMath::Max(1, Quanti)) + FMath::FRandRange(-0.4f, 0.4f);
		const float Lontano = Raggio * FMath::Sqrt(FMath::FRandRange(0.15f, 1.f));
		FVector Dove = GetActorLocation() + FVector(FMath::Cos(Angolo) * Lontano, FMath::Sin(Angolo) * Lontano, 0.f);
		// il pavimento sotto (le sale sono fatte dal generatore: si cerca la terra fino a tre metri sotto)
		FHitResult Urto;
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoOssario), false, this);
		if (Mondo->LineTraceSingleByChannel(Urto, Dove + FVector(0.f, 0.f, 150.f), Dove - FVector(0.f, 0.f, 300.f), ECC_WorldStatic, Parametri))
		{
			const FValdorsoTipoScheletro* SuoTipo = ValdorsoScheletri::Tipo(TipoScheletro);
			Dove = Urto.ImpactPoint + FVector(0.f, 0.f, 92.f * (SuoTipo ? SuoTipo->Grandezza : 1.f));
		}
		const FTransform Posto(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Dove);
		AValdorsoScheletro* Nuovo = Mondo->SpawnActorDeferred<AValdorsoScheletro>(AValdorsoScheletro::StaticClass(), Posto, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (Nuovo)
		{
			Nuovo->Prepara(TipoScheletro, Dove, false);
			Nuovo->FinishSpawning(Posto);
			Vivi.Add(Nuovo);
		}
	}
}

void AValdorsoOssario::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* Mondo = GetWorld();
	if (!Mondo || Mondo->GetNetMode() == NM_Client)
	{
		return;
	}
	const double Ora = Mondo->GetTimeSeconds();
	const bool bTuttiMorti = !Vivi.ContainsByPredicate([](const TWeakObjectPtr<AValdorsoScheletro>& S) { return S.IsValid() && S->Vivo(); });
	if (bTuttiMorti && bNati)
	{
		// tutti a terra: tornano dopo i loro minuti
		bNati = false;
		Ritorno = Ora + MinutiRitorno * 60.0;
	}
	if (!bNati && Ora >= Ritorno)
	{
		FaiNascere();
	}
}

void AValdorsoOssario::ViaTutti(UWorld* Mondo)
{
	if (!Mondo || Mondo->GetNetMode() == NM_Client)
	{
		return;
	}
	for (TActorIterator<AValdorsoScheletro> It(Mondo); It; ++It)
	{
		It->Destroy();
	}
}

#undef LOCTEXT_NAMESPACE
