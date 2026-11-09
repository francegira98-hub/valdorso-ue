// Valdorso - I lupi (v0.1.4 passo 4.6, provvisori, 09/11/2026). Scritto da Claude. Vedi ValdorsoLupi.h.

#include "ValdorsoLupi.h"
#include "Valdorso.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoBosco.h"
#include "ValdorsoCielo.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoFalo.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoTempo.h"
#include "ValdorsoZaino.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"

namespace
{
	const TCHAR* LupiCartella = TEXT("/Game/Valle/Pericoli/Lupo/");
	constexpr float LupiMezzaAltezza = 40.f;
	constexpr float LupiRaggio = 34.f;
	/** Dove stanno i pezzi (cm, da terra; il lupo guarda +X): come in pericoli.py. */
	const FVector LupiPostoCorpo(0.f, 0.f, 58.f);
	const FVector LupiPostoTesta(44.f, 0.f, 64.f);
	const FVector LupiPostoCoda(-54.f, 0.f, 66.f);
	const FVector LupiPostiZampe[4] = { FVector(32.f, 9.f, 56.f), FVector(32.f, -9.f, 56.f), FVector(-40.f, 8.f, 56.f), FVector(-40.f, -8.f, 56.f) };
	/** Il passo: la zampa anteriore sinistra va con la posteriore destra. */
	const float LupiFaseZampe[4] = { 0.f, PI, PI, 0.f };

	UStaticMesh* LupiModello(const TCHAR* Nome)
	{
		// l'importazione dei .glb mette il modello in una sottocartella (Lupo/SM_Lupo_X/StaticMeshes/SM_Lupo_X); prima quella
		const FString Dentro = FString(LupiCartella) + Nome + TEXT("/StaticMeshes/") + Nome + TEXT(".") + Nome;
		const FString Percorso = FString(LupiCartella) + Nome + TEXT(".") + Nome;
		UStaticMesh* Modello = LoadObject<UStaticMesh>(nullptr, *Dentro, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Modello)
		{
			Modello = LoadObject<UStaticMesh>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		if (!Modello)
		{
			UE_LOG(LogValdorso, Warning, TEXT("Lupi: manca il modello %s (Menu Valdorso -> Pericoli: importa i pericoli)."), *Percorso);
		}
		return Modello;
	}

	UStaticMeshComponent* LupiPezzo(AValdorsoLupo* Lupo, USceneComponent* Sopra, const TCHAR* Nome, const FVector& Dove)
	{
		UStaticMeshComponent* Pezzo = Lupo->CreateDefaultSubobject<UStaticMeshComponent>(Nome);
		Pezzo->SetupAttachment(Sopra);
		Pezzo->SetRelativeLocation(Dove);
		Pezzo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pezzo->SetGenerateOverlapEvents(false);
		Pezzo->SetCanEverAffectNavigation(false);
		return Pezzo;
	}

	/** Luce del giorno e nebbia di adesso (dal cielo e dal tempo del mondo). */
	bool LupiFuoriAdesso(const UWorld* Mondo)
	{
		float Luce = 1.f;
		for (TActorIterator<AValdorsoCielo> It(const_cast<UWorld*>(Mondo)); It; ++It)
		{
			Luce = It->LuceDelGiorno();
			break;
		}
		const AValdorsoTempo* Tempo = AValdorsoTempo::Di(Mondo);
		return ValdorsoLupi::Fuori(Luce, Tempo && Tempo->GetTempo() == EValdorsoTempo::Nebbia);
	}

	bool LupiNotte(const UWorld* Mondo)
	{
		for (TActorIterator<AValdorsoCielo> It(const_cast<UWorld*>(Mondo)); It; ++It)
		{
			return It->LuceDelGiorno() < 0.25f;
		}
		return false;
	}

	/** Il colono di un giocatore, se c'e' e non e' a terra. */
	APawn* LupiColono(APlayerController* Controllore)
	{
		APawn* Colono = Controllore ? Controllore->GetPawn() : nullptr;
		if (!Colono)
		{
			return nullptr;
		}
		const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Colono);
		return Pericoli && Pericoli->Caduto() ? nullptr : Colono;
	}

	/** (09/11) Il mondo del gioco: quello della console, o (dalla console dell'editor durante il Play) quello del Play. */
	UWorld* LupiMondoDiGioco(UWorld* Mondo)
	{
		if (Mondo && Mondo->IsGameWorld())
		{
			return Mondo;
		}
		if (GEngine)
		{
			for (const FWorldContext& Contesto : GEngine->GetWorldContexts())
			{
				UWorld* Gioco = Contesto.World();
				if (Gioco && Gioco->GetNetMode() != NM_Client && (Contesto.WorldType == EWorldType::PIE || Contesto.WorldType == EWorldType::Game))
				{
					return Gioco;
				}
			}
		}
		return Mondo;
	}

	AValdorsoTaneLupi* LupiTane(UWorld* Mondo)
	{
		AValdorsoTaneLupi* Tane = AValdorsoTaneLupi::Di(Mondo);
		if (!Tane && Mondo && Mondo->GetNetMode() != NM_Client)
		{
			FActorSpawnParameters Parametri;
			Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Tane = Mondo->SpawnActor<AValdorsoTaneLupi>(AValdorsoTaneLupi::StaticClass(), FTransform::Identity, Parametri);
		}
		return Tane;
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoLupiChiama(
		TEXT("Valdorso.Lupi.Chiama"),
		TEXT("Per le prove: un branco di lupi a 25 m dal primo colono, anche di giorno (sul server, o da soli)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Console)
		{
			UWorld* Mondo = LupiMondoDiGioco(Console);
			AValdorsoTaneLupi* Tane = LupiTane(Mondo);
			APlayerController* Primo = Mondo ? Mondo->GetFirstPlayerController() : nullptr;
			const APawn* Colono = Primo ? Primo->GetPawn() : nullptr;
			if (!Tane || !Colono)
			{
				UE_LOG(LogValdorso, Display, TEXT("Valdorso.Lupi.Chiama: serve un colono in gioco (sul server)."));
				return;
			}
			const FVector Davanti = Colono->GetActorLocation() + Colono->GetActorForwardVector().GetSafeNormal2D() * 2500.f;
			UE_LOG(LogValdorso, Display, TEXT("Valdorso.Lupi.Chiama: %d lupi."), Tane->Chiama(Davanti));
		}));

	FAutoConsoleCommandWithWorldAndArgs ComandoLupiVia(
		TEXT("Valdorso.Lupi.Via"),
		TEXT("Tutti i lupi tornano alla tana (sul server, o da soli)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Console)
		{
			if (AValdorsoTaneLupi* Tane = AValdorsoTaneLupi::Di(LupiMondoDiGioco(Console)))
			{
				Tane->Via();
			}
		}));
}

// ----------------------------------------------------------------------------------------------- le regole

namespace ValdorsoLupi
{
	bool Fuori(float LuceDelGiorno, bool bNebbia)
	{
		return bNebbia || LuceDelGiorno < 0.25f;
	}

	EValdorsoStatoLupo Prossimo(EValdorsoStatoLupo Attuale, float SaluteParte, float Distanza, bool bProtetta, float Secondi)
	{
		if (Attuale == EValdorsoStatoLupo::Morto || SaluteParte <= 0.f)
		{
			return EValdorsoStatoLupo::Morto;
		}
		if (Attuale == EValdorsoStatoLupo::Fuggi)
		{
			return Secondi < SecondiFuga ? EValdorsoStatoLupo::Fuggi : EValdorsoStatoLupo::Vaga;
		}
		if (SaluteParte < PauraSotto)
		{
			// ferito: fugge da chi caccia; dopo la fuga vaga e non caccia piu'
			return Attuale == EValdorsoStatoLupo::Vaga ? EValdorsoStatoLupo::Vaga : EValdorsoStatoLupo::Fuggi;
		}
		if (Distanza < 0.f || Distanza > Lascia)
		{
			return EValdorsoStatoLupo::Vaga;
		}
		switch (Attuale)
		{
		case EValdorsoStatoLupo::Vaga:
			return Distanza <= Fiuto ? EValdorsoStatoLupo::Fiuta : EValdorsoStatoLupo::Vaga;
		case EValdorsoStatoLupo::Fiuta:
			return Distanza <= 1200.f ? EValdorsoStatoLupo::Accerchia : EValdorsoStatoLupo::Fiuta;
		case EValdorsoStatoLupo::Accerchia:
			// chi sta al fuoco o nella pace: si gira al largo, ma non si morde
			return !bProtetta && Secondi >= SecondiAccerchia ? EValdorsoStatoLupo::Morde : EValdorsoStatoLupo::Accerchia;
		case EValdorsoStatoLupo::Morde:
			// la preda e' corsa al fuoco, o non la si raggiunge: si torna a girare
			return bProtetta || Secondi > 6.f ? EValdorsoStatoLupo::Accerchia : EValdorsoStatoLupo::Morde;
		default:
			return Attuale;
		}
	}

	int32 QuantiNelBranco(int32 Volta)
	{
		return (Volta % 3) == 1 ? 4 : 3;
	}

	float Velocita(EValdorsoStatoLupo Stato)
	{
		switch (Stato)
		{
		case EValdorsoStatoLupo::Vaga: return 170.f;
		case EValdorsoStatoLupo::Fiuta: return 240.f;
		case EValdorsoStatoLupo::Accerchia: return 380.f;
		case EValdorsoStatoLupo::Morde: return 640.f;
		case EValdorsoStatoLupo::Fuggi: return 620.f;
		default: return 0.f;
		}
	}
}

// ----------------------------------------------------------------------------------------------- il lupo

AValdorsoLupo::AValdorsoLupo()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetUpdateFrequency(20.f);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(LupiRaggio, LupiMezzaAltezza);
	// la telecamera dei coloni non si ferma contro un lupo
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetVisibility(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UCharacterMovementComponent* Movimento = GetCharacterMovement();
	Movimento->bOrientRotationToMovement = true;
	Movimento->RotationRate = FRotator(0.f, 420.f, 0.f);
	Movimento->MaxWalkSpeed = ValdorsoLupi::Velocita(EValdorsoStatoLupo::Vaga);
	Movimento->MaxStepHeight = 35.f;
	Movimento->SetWalkableFloorAngle(50.f);
	bUseControllerRotationYaw = false;

	Abilita = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Abilita"));
	Abilita->SetIsReplicated(true);
	Abilita->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Attributi = CreateDefaultSubobject<UValdorsoAttributeSet>(TEXT("Attributi"));
	Reazioni = CreateDefaultSubobject<UValdorsoReazioniComponent>(TEXT("Reazioni"));

	Figura = CreateDefaultSubobject<USceneComponent>(TEXT("Figura"));
	// sul Mesh (vuoto, nascosto): sui PC degli altri il motore ammorbidisce il suo movimento, non quello della capsula
	Figura->SetupAttachment(GetMesh());
	Figura->SetRelativeLocation(FVector(0.f, 0.f, -LupiMezzaAltezza));
	Corpo = LupiPezzo(this, Figura, TEXT("Corpo"), LupiPostoCorpo);
	Testa = LupiPezzo(this, Figura, TEXT("Testa"), LupiPostoTesta);
	Coda = LupiPezzo(this, Figura, TEXT("Coda"), LupiPostoCoda);
	const TCHAR* NomiZampe[4] = { TEXT("ZampaAvantiSinistra"), TEXT("ZampaAvantiDestra"), TEXT("ZampaDietroSinistra"), TEXT("ZampaDietroDestra") };
	for (int32 i = 0; i < 4; ++i)
	{
		Zampe.Add(LupiPezzo(this, Figura, NomiZampe[i], LupiPostiZampe[i]));
	}
}

void AValdorsoLupo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoLupo, Stato);
}

void AValdorsoLupo::BeginPlay()
{
	Super::BeginPlay();
	Abilita->InitAbilityActorInfo(this, this);
	if (HasAuthority())
	{
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteMaxAttribute(), ValdorsoLupi::SaluteLupo);
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute(), ValdorsoLupi::SaluteLupo);
		if (Tana.IsZero())
		{
			Tana = GetActorLocation();
		}
		Meta = GetActorLocation();
		AngoloGiro = FMath::FRandRange(0.f, 2.f * PI);
		VersoGiro = FMath::RandBool() ? 1.f : -1.f;
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		Corpo->SetStaticMesh(LupiModello(TEXT("SM_Lupo_Corpo")));
		Testa->SetStaticMesh(LupiModello(TEXT("SM_Lupo_Testa")));
		Coda->SetStaticMesh(LupiModello(TEXT("SM_Lupo_Coda")));
		UStaticMesh* Zampa = LupiModello(TEXT("SM_Lupo_Zampa"));
		for (UStaticMeshComponent* Pezzo : Zampe)
		{
			Pezzo->SetStaticMesh(Zampa);
		}
		// un po' diversi l'uno dall'altro
		const float Misura = FMath::FRandRange(0.92f, 1.08f);
		Figura->SetRelativeScale3D(FVector(Misura));
		ProssimoUlulato = GetWorld()->GetTimeSeconds() + FMath::FRandRange(8.f, 40.f);
	}
}

void AValdorsoLupo::Ferito(AActor* Da)
{
	if (!HasAuthority() || !Vivo())
	{
		return;
	}
	UltimoAggressore = Da;
	APawn* Chi = Cast<APawn>(Da);
	if (Chi && Chi->IsPlayerControlled())
	{
		Preda = Chi;
		if (Stato == EValdorsoStatoLupo::Vaga || Stato == EValdorsoStatoLupo::Fiuta)
		{
			Cambia(EValdorsoStatoLupo::Accerchia);
		}
	}
}

APawn* AValdorsoLupo::CercaPreda() const
{
	APawn* Migliore = nullptr;
	float Vicino = ValdorsoLupi::Fiuto;
	// chi e' gia' la preda la si segue fino a Lascia
	if (APawn* Attuale = Preda.Get())
	{
		const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Attuale);
		const float Distanza = FVector::Dist(Attuale->GetActorLocation(), GetActorLocation());
		if (!(Pericoli && Pericoli->Caduto()) && Distanza <= ValdorsoLupi::Lascia)
		{
			Migliore = Attuale;
			Vicino = Distanza;
		}
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Colono = LupiColono(It->Get());
		if (!Colono || Colono == Migliore)
		{
			continue;
		}
		const float Distanza = FVector::Dist(Colono->GetActorLocation(), GetActorLocation());
		// si cambia preda solo per una molto piu' vicina
		if (Distanza < (Migliore ? Vicino * 0.5f : Vicino))
		{
			Migliore = Colono;
			Vicino = Distanza;
		}
	}
	return Migliore;
}

void AValdorsoLupo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		Pensa(DeltaSeconds);
		Muoviti(DeltaSeconds);
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		Anima(DeltaSeconds);
	}
}

void AValdorsoLupo::Pensa(float DeltaSeconds)
{
	UWorld* Mondo = GetWorld();
	const double OraMondo = Mondo->GetTimeSeconds();
	if (!Vivo())
	{
		return;
	}
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
		ProssimaRicerca = OraMondo + 0.5;
		Preda = CercaPreda();
		const APawn* Chi = Preda.Get();
		bPredaProtetta = Chi && (AValdorsoFalo::CaldoVicino(Mondo, Chi->GetActorLocation())
			|| ValdorsoCombattimento::InPace(Mondo, Chi->GetActorLocation()));
	}
	const APawn* Chi = Preda.Get();
	const float Distanza = Chi ? FVector::Dist(Chi->GetActorLocation(), GetActorLocation()) : -1.f;
	const EValdorsoStatoLupo Nuovo = ValdorsoLupi::Prossimo(Stato, Salute / FMath::Max(1.f, Massima), Distanza, bPredaProtetta,
		static_cast<float>(OraMondo - InizioStato));
	if (Nuovo != Stato)
	{
		Cambia(Nuovo);
	}
	// il morso: vicino abbastanza, e non troppo spesso
	if (Stato == EValdorsoStatoLupo::Morde && Chi && Distanza <= ValdorsoLupi::PortataMorso && OraMondo >= ProssimoMorso)
	{
		ProssimoMorso = OraMondo + ValdorsoLupi::SecondiMorso;
		const FVector Punto = Chi->GetActorLocation() + (GetActorLocation() - Chi->GetActorLocation()).GetSafeNormal() * 30.f;
		ValdorsoCombattimento::Colpisci(this, const_cast<APawn*>(Chi), ValdorsoLupi::DannoMorso, ValdorsoLupi::EquilibrioMorso, Punto,
			EValdorsoTipoColpo::Morso);
		// dopo il morso si torna a girare (e a volte dall'altra parte)
		VersoGiro = FMath::RandBool() ? 1.f : -1.f;
		Cambia(EValdorsoStatoLupo::Accerchia);
	}
}

void AValdorsoLupo::Muoviti(float DeltaSeconds)
{
	UCharacterMovementComponent* Movimento = GetCharacterMovement();
	const UWorld* Mondo = GetWorld();
	if (!Movimento || !Vivo())
	{
		return;
	}
	const double OraMondo = Mondo->GetTimeSeconds();
	const FVector Qui = GetActorLocation();
	const APawn* Chi = Preda.Get();
	FVector Verso = FVector::ZeroVector;
	switch (Stato)
	{
	case EValdorsoStatoLupo::Vaga:
		// un punto a caso intorno alla tana, ogni tanto
		if (OraMondo >= NuovaMeta || FVector::Dist2D(Qui, Meta) < 150.f)
		{
			NuovaMeta = OraMondo + FMath::FRandRange(5.f, 10.f);
			const float Angolo = FMath::FRandRange(0.f, 2.f * PI);
			const float Raggio = 1500.f * FMath::Sqrt(FMath::FRand());
			Meta = Tana + FVector(FMath::Cos(Angolo) * Raggio, FMath::Sin(Angolo) * Raggio, 0.f);
		}
		Verso = Meta - Qui;
		break;
	case EValdorsoStatoLupo::Fiuta:
		// si avvicina, ma si ferma a una decina di metri
		if (Chi && FVector::Dist2D(Qui, Chi->GetActorLocation()) > 1000.f)
		{
			Verso = Chi->GetActorLocation() - Qui;
		}
		break;
	case EValdorsoStatoLupo::Accerchia:
		if (Chi)
		{
			const float Raggio = bPredaProtetta ? ValdorsoLupi::GiroAlLargo : ValdorsoLupi::GiroCaccia;
			AngoloGiro += VersoGiro * DeltaSeconds * (ValdorsoLupi::Velocita(Stato) / Raggio) * 0.8f;
			Meta = Chi->GetActorLocation() + FVector(FMath::Cos(AngoloGiro) * Raggio, FMath::Sin(AngoloGiro) * Raggio, 0.f);
			Verso = Meta - Qui;
		}
		break;
	case EValdorsoStatoLupo::Morde:
		if (Chi)
		{
			Verso = Chi->GetActorLocation() - Qui;
		}
		break;
	case EValdorsoStatoLupo::Fuggi:
		Verso = Chi ? Qui - Chi->GetActorLocation() : Tana - Qui;
		break;
	default:
		break;
	}
	Verso.Z = 0.f;
	Movimento->MaxWalkSpeed = ValdorsoLupi::Velocita(Stato);
	if (Verso.SizeSquared() < 40.f * 40.f)
	{
		return;
	}
	// incastrato (un tronco, un masso): per un attimo di lato
	if (OraMondo < ScansaFino)
	{
		Verso = Scansa;
	}
	else if (GetVelocity().Size2D() < 25.f)
	{
		Fermo += DeltaSeconds;
		if (Fermo > 1.2)
		{
			Fermo = 0.0;
			ScansaFino = OraMondo + 1.0;
			Scansa = FVector::CrossProduct(Verso.GetSafeNormal(), FVector::UpVector) * (FMath::RandBool() ? 1.f : -1.f);
			NuovaMeta = OraMondo;
		}
	}
	else
	{
		Fermo = 0.0;
	}
	AddMovementInput(Verso.GetSafeNormal());
}

void AValdorsoLupo::Cambia(EValdorsoStatoLupo Nuovo)
{
	if (Nuovo == Stato && Nuovo != EValdorsoStatoLupo::Accerchia)
	{
		return;
	}
	const bool bDiverso = Nuovo != Stato;
	Stato = Nuovo;
	InizioStato = GetWorld()->GetTimeSeconds();
	if (Stato == EValdorsoStatoLupo::Accerchia && Preda.IsValid())
	{
		// si parte a girare da dove si e'
		const FVector Da = GetActorLocation() - Preda->GetActorLocation();
		AngoloGiro = FMath::Atan2(Da.Y, Da.X);
	}
	if (bDiverso && GetNetMode() != NM_DedicatedServer)
	{
		SuonaStato();
	}
}

void AValdorsoLupo::OnRep_Stato()
{
	SuonaStato();
	if (Stato == EValdorsoStatoLupo::Morto)
	{
		// anche qui il corpo non ferma piu' nessuno
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
}

void AValdorsoLupo::SuonaStato()
{
	const TCHAR* Nome = nullptr;
	float Lontano = 2500.f;
	switch (Stato)
	{
	case EValdorsoStatoLupo::Accerchia:
		Nome = TEXT("S_Lupo_Ringhio");
		break;
	case EValdorsoStatoLupo::Fuggi:
	case EValdorsoStatoLupo::Morto:
		Nome = TEXT("S_Lupo_Guaito");
		Lontano = 4000.f;
		break;
	default:
		break;
	}
	if (Nome)
	{
		ValdorsoCombattimento::Suona(GetWorld(), ValdorsoCombattimento::SuonoPericoli(Nome), GetActorLocation(), Lontano,
			1.f, FMath::FRandRange(0.9f, 1.1f));
	}
}

void AValdorsoLupo::Muori()
{
	Cambia(EValdorsoStatoLupo::Morto);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	SetLifeSpan(30.f);
	// la pelle e le zanne a chi l'ha ucciso (se e' un colono con lo zaino)
	const APawn* Cacciatore = Cast<APawn>(UltimoAggressore.Get());
	APlayerController* Controllore = Cacciatore ? Cast<APlayerController>(Cacciatore->GetController()) : nullptr;
	if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore))
	{
		Zaino->Dai(TEXT("Pelle_Lupo"), 1);
		Zaino->Dai(TEXT("Zanne_Lupo"), 1);
	}
	ValdorsoRegistroEventi::Annota(TEXT("Lupo"), Cacciatore, this, GetActorLocation(), TEXT("lupo ucciso"));
}

void AValdorsoLupo::Anima(float DeltaSeconds)
{
	const float Tempo = static_cast<float>(GetWorld()->GetTimeSeconds());
	const float Veloce = static_cast<float>(GetVelocity().Size2D());
	const bool bMorto = Stato == EValdorsoStatoLupo::Morto;

	// le zampe: avanti e indietro con il passo (piu' larghe quando corre)
	Passo = FMath::Fmod(Passo + DeltaSeconds * Veloce / 22.f, 2.f * PI);
	const float Ampiezza = bMorto ? 0.f : FMath::Clamp(Veloce / 320.f, 0.f, 1.f) * 32.f;
	for (int32 i = 0; i < Zampe.Num() && i < 4; ++i)
	{
		Zampe[i]->SetRelativeRotation(FRotator(FMath::Sin(Passo + LupiFaseZampe[i]) * Ampiezza, 0.f, 0.f));
	}
	// la testa: giu' per fiutare e per mordere, su quando ringhia; un poco su e giu' col passo
	float Giu = 0.f;
	switch (Stato)
	{
	case EValdorsoStatoLupo::Vaga:
	case EValdorsoStatoLupo::Fiuta: Giu = -18.f; break;
	case EValdorsoStatoLupo::Morde: Giu = -10.f + FMath::Sin(Tempo * 14.f) * 8.f; break;
	case EValdorsoStatoLupo::Fuggi: Giu = -22.f; break;
	case EValdorsoStatoLupo::Morto: Giu = -30.f; break;
	default: Giu = -4.f; break;
	}
	Testa->SetRelativeRotation(FRotator(Giu + FMath::Sin(Passo * 2.f) * Ampiezza * 0.12f, FMath::Sin(Tempo * 0.7f) * 6.f, 0.f));
	// la coda: bassa quando fugge o caccia, scodinzola poco
	const float Bassa = Stato == EValdorsoStatoLupo::Fuggi ? -30.f : (Caccia() ? -5.f : 8.f);
	Coda->SetRelativeRotation(FRotator(bMorto ? -10.f : Bassa, bMorto ? 0.f : FMath::Sin(Tempo * 5.f) * 12.f, 0.f));
	// morto: si accascia su un fianco
	Accasciato = FMath::FInterpTo(Accasciato, bMorto ? 1.f : 0.f, DeltaSeconds, 4.f);
	Figura->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -LupiMezzaAltezza + Accasciato * 12.f), FRotator(0.f, 0.f, Accasciato * 82.f));

	// di notte, ogni tanto, chi vaga ulula
	const double OraMondo = GetWorld()->GetTimeSeconds();
	if (!bMorto && OraMondo >= ProssimoUlulato)
	{
		ProssimoUlulato = OraMondo + FMath::FRandRange(35.f, 90.f);
		if (Stato == EValdorsoStatoLupo::Vaga && LupiNotte(GetWorld()))
		{
			ValdorsoCombattimento::Suona(GetWorld(), ValdorsoCombattimento::SuonoPericoli(TEXT("S_Lupo_Ululato")), GetActorLocation(), 15000.f,
				1.f, FMath::FRandRange(0.9f, 1.08f));
		}
	}
}

// ----------------------------------------------------------------------------------------------- le tane

AValdorsoTaneLupi::AValdorsoTaneLupi()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);
}

AValdorsoTaneLupi* AValdorsoTaneLupi::Di(const UWorld* Mondo)
{
	if (!Mondo)
	{
		return nullptr;
	}
	for (TActorIterator<AValdorsoTaneLupi> It(const_cast<UWorld*>(Mondo)); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void AValdorsoTaneLupi::Prepara()
{
	bPronte = true;
	for (const TCHAR* Nome : { TEXT("Boschetto"), TEXT("Collina dell'Orso") })
	{
		if (const FValdorsoLuogo* Luogo = ValdorsoLuoghi::Trova(Nome))
		{
			FTana Tana;
			Tana.Nome = Nome;
			Tana.Centro = Luogo->Posizione;
			Tane.Add(Tana);
		}
	}
	UE_LOG(LogValdorso, Log, TEXT("Lupi: %d tane."), Tane.Num());
}

bool AValdorsoTaneLupi::Terra(const FVector& Sopra, FVector& OutTerra) const
{
	// la prima cosa sotto, che non sia un albero (i tronchi del bosco) e che sia abbastanza piana
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoLupiTerra), false);
	const FVector Da = Sopra + FVector(0.f, 0.f, 30000.f);
	const FVector A = Sopra - FVector(0.f, 0.f, 30000.f);
	for (int32 Tentativo = 0; Tentativo < 8; ++Tentativo)
	{
		FHitResult Colpo;
		if (!GetWorld()->LineTraceSingleByChannel(Colpo, Da, A, ECC_WorldStatic, Parametri))
		{
			return false;
		}
		AActor* Sotto = Colpo.GetActor();
		if (Sotto && Sotto->IsA<AValdorsoBosco>())
		{
			Parametri.AddIgnoredActor(Sotto);
			continue;
		}
		if (Colpo.ImpactNormal.Z < 0.7f)
		{
			return false;
		}
		OutTerra = Colpo.ImpactPoint + FVector(0.f, 0.f, LupiMezzaAltezza + 15.f);
		return true;
	}
	return false;
}

int32 AValdorsoTaneLupi::Branco(FTana& Tana, const FVector& Dove)
{
	UWorld* Mondo = GetWorld();
	const int32 Quanti = ValdorsoLupi::QuantiNelBranco(Volte++);
	int32 Usciti = 0;
	for (int32 i = 0; i < Quanti; ++i)
	{
		const float Angolo = (2.f * PI * i) / Quanti + FMath::FRandRange(-0.3f, 0.3f);
		const FVector Punto = Dove + FVector(FMath::Cos(Angolo), FMath::Sin(Angolo), 0.f) * FMath::FRandRange(200.f, 500.f);
		FVector Terreno;
		if (!Terra(Punto, Terreno))
		{
			continue;
		}
		FActorSpawnParameters Parametri;
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		AValdorsoLupo* Lupo = Mondo->SpawnActor<AValdorsoLupo>(AValdorsoLupo::StaticClass(),
			FTransform(FRotator(0.f, FMath::FRandRange(-180.f, 180.f), 0.f), Terreno), Parametri);
		if (Lupo)
		{
			Lupo->Casa(Dove);
			Tana.Lupi.Add(Lupo);
			++Usciti;
		}
	}
	Tana.bFuori = Usciti > 0;
	return Usciti;
}

int32 AValdorsoTaneLupi::Chiama(const FVector& Vicino)
{
	FTana Finta;
	const int32 Usciti = Branco(Finta, Vicino);
	Chiamati.Append(Finta.Lupi);
	return Usciti;
}

void AValdorsoTaneLupi::Via()
{
	auto Manda = [](TArray<TWeakObjectPtr<AValdorsoLupo>>& Lupi)
	{
		for (const TWeakObjectPtr<AValdorsoLupo>& Lupo : Lupi)
		{
			if (Lupo.IsValid())
			{
				Lupo->Destroy();
			}
		}
		Lupi.Reset();
	};
	for (FTana& Tana : Tane)
	{
		Manda(Tana.Lupi);
		Tana.bFuori = false;
	}
	Manda(Chiamati);
}

void AValdorsoTaneLupi::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* Mondo = GetWorld();
	if (!Mondo || Mondo->GetNetMode() == NM_Client)
	{
		return;
	}
	const double OraMondo = Mondo->GetTimeSeconds();
	if (OraMondo < ProssimoControllo)
	{
		return;
	}
	ProssimoControllo = OraMondo + 2.0;
	if (!bPronte)
	{
		Prepara();
	}
	const bool bFuori = LupiFuoriAdesso(Mondo);
	for (FTana& Tana : Tane)
	{
		// quanto e' vicino il colono piu' vicino
		float Vicino = TNumericLimits<float>::Max();
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			if (const APawn* Colono = LupiColono(It->Get()))
			{
				Vicino = FMath::Min(Vicino, static_cast<float>(FVector::Dist2D(Colono->GetActorLocation(), Tana.Centro)));
			}
		}
		int32 Vivi = 0;
		bool bCaccia = false;
		for (const TWeakObjectPtr<AValdorsoLupo>& Lupo : Tana.Lupi)
		{
			if (Lupo.IsValid() && Lupo->Vivo())
			{
				++Vivi;
				bCaccia |= Lupo->Caccia();
				// caduto fuori dal mondo (una parte di valle non caricata): via
				if (Lupo->GetActorLocation().Z < Tana.Centro.Z - 20000.f)
				{
					Lupo->Destroy();
				}
			}
		}
		if (Tana.bFuori && Vivi == 0)
		{
			// il branco e' stato ucciso tutto: torna fra venti minuti
			Tana.bFuori = false;
			Tana.Lupi.Reset();
			Tana.Ritorno = OraMondo + ValdorsoLupi::SecondiRitorno;
			UE_LOG(LogValdorso, Log, TEXT("Lupi: il branco di %s e' finito, torna fra 20 minuti."), *Tana.Nome);
			continue;
		}
		if (Tana.bFuori && !bCaccia && (!bFuori || Vicino > ValdorsoLupi::LontanoTana))
		{
			// il giorno, o nessuno vicino: tornano alla tana
			for (const TWeakObjectPtr<AValdorsoLupo>& Lupo : Tana.Lupi)
			{
				if (Lupo.IsValid() && Lupo->Vivo())
				{
					Lupo->Destroy();
				}
			}
			Tana.Lupi.Reset();
			Tana.bFuori = false;
			continue;
		}
		if (!Tana.bFuori && bFuori && Vicino <= ValdorsoLupi::VicinoTana && OraMondo >= Tana.Ritorno)
		{
			const int32 Usciti = Branco(Tana, Tana.Centro);
			if (Usciti > 0)
			{
				UE_LOG(LogValdorso, Log, TEXT("Lupi: %d lupi escono dalla tana di %s."), Usciti, *Tana.Nome);
			}
			else
			{
				// il terreno non c'e' ancora (parte di valle non caricata): si riprova fra mezzo minuto
				Tana.Ritorno = OraMondo + 30.0;
			}
		}
	}
	Chiamati.RemoveAll([](const TWeakObjectPtr<AValdorsoLupo>& Lupo) { return !Lupo.IsValid(); });
}
