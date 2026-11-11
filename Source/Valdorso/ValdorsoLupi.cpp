// Valdorso - Gli animali della valle (v0.1.4: i lupi il 09/11, tutti gli altri il 10/11). Scritto da Claude.
// Vedi ValdorsoLupi.h.

#include "ValdorsoLupi.h"
#include "ValdorsoCrescita.h"
#include "Valdorso.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoBosco.h"
#include "ValdorsoCielo.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoFalo.h"
#include "ValdorsoGilda.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoScherma.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoTempo.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
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

#define LOCTEXT_NAMESPACE "ValdorsoAnimali"

namespace
{
	const FValdorsoForma AnimaliForme[] = {
#include "ValdorsoAnimaliForme.inl"
	};

	/** Il passo: la zampa anteriore sinistra va con la posteriore destra (avanti sx, avanti dx, dietro sx, dietro dx). */
	const float AnimaliFaseZampe[4] = { 0.f, PI, PI, 0.f };

	FValdorsoSpecie AnimaliNuova(const TCHAR* Id, const TCHAR* ConArticolo, EValdorsoIndole Indole, float Salute)
	{
		FValdorsoSpecie S;
		S.Id = Id;
		S.ConArticolo = ConArticolo;
		S.Indole = Indole;
		S.Salute = Salute;
		return S;
	}

	void AnimaliBottino(FValdorsoSpecie& S, int32 i, const TCHAR* Id, int32 Quanti)
	{
		S.Bottino[i] = Id;
		S.Quanti[i] = Quanti;
	}

	TArray<FValdorsoSpecie> AnimaliCreaSpecie()
	{
		TArray<FValdorsoSpecie> T;
		{
			FValdorsoSpecie S = AnimaliNuova(TEXT("Lupo"), TEXT("il lupo"), EValdorsoIndole::Predatore, ValdorsoLupi::SaluteLupo);
			S.Danno = ValdorsoLupi::DannoMorso;
			S.Equilibrio = ValdorsoLupi::EquilibrioMorso;
			S.SecondiMorso = ValdorsoLupi::SecondiMorso;
			S.Fiuto = ValdorsoLupi::Fiuto;
			S.Lascia = ValdorsoLupi::Lascia;
			S.Giro = ValdorsoLupi::GiroCaccia;
			S.SecondiAvviso = ValdorsoLupi::SecondiAccerchia;
			S.Portata = ValdorsoLupi::PortataMorso;
			S.Passo = 170.f;
			S.Corsa = 640.f;
			S.Paura = ValdorsoLupi::PauraSotto;
			S.SuonoAvviso = TEXT("S_Lupo_Ringhio");
			S.SuonoDolore = TEXT("S_Lupo_Guaito");
			S.SuonoRichiamo = TEXT("S_Lupo_Ululato");
			AnimaliBottino(S, 0, TEXT("Pelle_Lupo"), 1);
			AnimaliBottino(S, 1, TEXT("Zanne_Lupo"), 1);
			AnimaliBottino(S, 2, TEXT("Carne_Cruda"), 1);
			T.Add(S);
		}
		{
			FValdorsoSpecie S = AnimaliNuova(TEXT("Cinghiale"), TEXT("il cinghiale"), EValdorsoIndole::Territoriale, 90.f);
			S.Danno = 12.f;
			S.Equilibrio = 45.f;
			S.SecondiMorso = 2.f;
			S.Fiuto = 900.f;
			S.Lascia = 2500.f;
			S.SecondiAvviso = 2.f;
			S.Portata = 160.f;
			S.Passo = 120.f;
			S.Corsa = 700.f;
			S.Paura = 0.2f;
			S.SuonoAvviso = TEXT("S_Cinghiale_Grugnito");
			S.SuonoDolore = TEXT("S_Cinghiale_Strillo");
			S.SuonoRichiamo = TEXT("S_Cinghiale_Grugnito");
			S.bRichiamoDiNotte = false;
			AnimaliBottino(S, 0, TEXT("Pelle_Cinghiale"), 1);
			AnimaliBottino(S, 1, TEXT("Zanne_Cinghiale"), 1);
			AnimaliBottino(S, 2, TEXT("Carne_Cruda"), 2);
			T.Add(S);
		}
		{
			FValdorsoSpecie S = AnimaliNuova(TEXT("Orso"), TEXT("l'orso"), EValdorsoIndole::Territoriale, 260.f);
			S.Danno = 24.f;
			S.Equilibrio = 70.f;
			S.SecondiMorso = 2.2f;
			S.Fiuto = 1400.f;
			S.Lascia = 3000.f;
			S.SecondiAvviso = 3.f;
			S.Portata = 230.f;
			S.Passo = 110.f;
			S.Corsa = 620.f;
			S.Paura = 0.15f;
			S.SuonoAvviso = TEXT("S_Orso_Ruggito");
			S.SuonoDolore = TEXT("S_Orso_Ruggito");
			AnimaliBottino(S, 0, TEXT("Pelle_Orso"), 1);
			AnimaliBottino(S, 1, TEXT("Artigli_Orso"), 1);
			AnimaliBottino(S, 2, TEXT("Carne_Cruda"), 3);
			T.Add(S);
		}
		{
			FValdorsoSpecie S = AnimaliNuova(TEXT("Cervo"), TEXT("il cervo"), EValdorsoIndole::Preda, 50.f);
			S.Fiuto = 2500.f;
			S.Lascia = 4000.f;
			S.Passo = 130.f;
			S.Corsa = 800.f;
			S.SuonoRichiamo = TEXT("S_Cervo_Bramito");
			S.bRichiamoDiNotte = false;
			AnimaliBottino(S, 0, TEXT("Pelle_Cervo"), 1);
			AnimaliBottino(S, 1, TEXT("Palchi_Cervo"), 1);
			AnimaliBottino(S, 2, TEXT("Carne_Cruda"), 2);
			T.Add(S);
		}
		{
			FValdorsoSpecie S = AnimaliNuova(TEXT("Lepre"), TEXT("la lepre"), EValdorsoIndole::Preda, 12.f);
			S.Fiuto = 1200.f;
			S.Lascia = 2500.f;
			S.Passo = 90.f;
			S.Corsa = 750.f;
			AnimaliBottino(S, 0, TEXT("Pelle_Lepre"), 1);
			AnimaliBottino(S, 1, TEXT("Carne_Cruda"), 1);
			T.Add(S);
		}
		{
			FValdorsoSpecie S = AnimaliNuova(TEXT("Volpe"), TEXT("la volpe"), EValdorsoIndole::Preda, 25.f);
			S.Fiuto = 1500.f;
			S.Lascia = 3000.f;
			S.Passo = 140.f;
			S.Corsa = 650.f;
			S.SuonoDolore = TEXT("S_Volpe_Verso");
			S.SuonoRichiamo = TEXT("S_Volpe_Verso");
			AnimaliBottino(S, 0, TEXT("Pelle_Volpe"), 1);
			T.Add(S);
		}
		return T;
	}

	const FValdorsoTana AnimaliTane[] = {
		{ TEXT("Boschetto"), TEXT("Lupo"), 3, 4, EValdorsoOraTana::Notte, 20.f },
		{ TEXT("Collina dell'Orso"), TEXT("Lupo"), 3, 4, EValdorsoOraTana::Notte, 20.f },
		{ TEXT("Collina dell'Orso"), TEXT("Orso"), 1, 1, EValdorsoOraTana::Sempre, 60.f },
		{ TEXT("Boschetto"), TEXT("Cinghiale"), 2, 3, EValdorsoOraTana::Sempre, 20.f },
		{ TEXT("Palude"), TEXT("Cinghiale"), 2, 2, EValdorsoOraTana::Sempre, 20.f },
		{ TEXT("Pascoli"), TEXT("Cervo"), 3, 4, EValdorsoOraTana::Giorno, 15.f },
		{ TEXT("Rovine degli Antichi"), TEXT("Cervo"), 2, 3, EValdorsoOraTana::Giorno, 15.f },
		{ TEXT("Pascoli"), TEXT("Lepre"), 2, 3, EValdorsoOraTana::Giorno, 10.f },
		{ TEXT("Mulino"), TEXT("Lepre"), 2, 2, EValdorsoOraTana::Giorno, 10.f },
		{ TEXT("Cimitero"), TEXT("Volpe"), 1, 2, EValdorsoOraTana::Notte, 15.f },
		{ TEXT("Palude"), TEXT("Volpe"), 1, 1, EValdorsoOraTana::Notte, 15.f },
	};

	constexpr float AnimaliSecondiFuga = ValdorsoLupi::SecondiFuga;

	UStaticMesh* AnimaliModello(const FString& Nome)
	{
		// l'importazione dei .glb mette ogni modello nella sua sottocartella (Animali/SM_X/StaticMeshes/SM_X); i lupi del
		// 09/11 stavano in Lupo/
		const FString Percorsi[] = {
			FString::Printf(TEXT("/Game/Valle/Pericoli/Animali/%s/StaticMeshes/%s.%s"), *Nome, *Nome, *Nome),
			FString::Printf(TEXT("/Game/Valle/Pericoli/Animali/%s.%s"), *Nome, *Nome),
			FString::Printf(TEXT("/Game/Valle/Pericoli/Lupo/%s/StaticMeshes/%s.%s"), *Nome, *Nome, *Nome),
			FString::Printf(TEXT("/Game/Valle/Pericoli/Lupo/%s.%s"), *Nome, *Nome),
		};
		for (const FString& Percorso : Percorsi)
		{
			if (UStaticMesh* Modello = LoadObject<UStaticMesh>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				return Modello;
			}
		}
		UE_LOG(LogValdorso, Warning, TEXT("Animali: manca il modello %s (Menu Valdorso -> Pericoli: importa i pericoli)."), *Nome);
		return nullptr;
	}

	UStaticMeshComponent* AnimaliPezzo(AValdorsoAnimale* Animale, USceneComponent* Sopra, const TCHAR* Nome)
	{
		UStaticMeshComponent* Pezzo = Animale->CreateDefaultSubobject<UStaticMeshComponent>(Nome);
		Pezzo->SetupAttachment(Sopra);
		Pezzo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pezzo->SetGenerateOverlapEvents(false);
		Pezzo->SetCanEverAffectNavigation(false);
		return Pezzo;
	}

	float AnimaliLuce(const UWorld* Mondo)
	{
		for (TActorIterator<AValdorsoCielo> It(const_cast<UWorld*>(Mondo)); It; ++It)
		{
			return It->LuceDelGiorno();
		}
		return 1.f;
	}

	bool AnimaliNebbia(const UWorld* Mondo)
	{
		const AValdorsoTempo* Tempo = AValdorsoTempo::Di(Mondo);
		return Tempo && Tempo->GetTempo() == EValdorsoTempo::Nebbia;
	}

	/** Il colono di un giocatore, se c'e' e non e' a terra. */
	APawn* AnimaliColono(APlayerController* Controllore)
	{
		APawn* Colono = Controllore ? Controllore->GetPawn() : nullptr;
		if (!Colono)
		{
			return nullptr;
		}
		const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Colono);
		return Pericoli && Pericoli->Caduto() ? nullptr : Colono;
	}

	/** Il mondo del gioco: quello della console, o (dalla console dell'editor durante il Play) quello del server del Play. */
	UWorld* AnimaliMondoDiGioco(UWorld* Mondo)
	{
		if (Mondo && Mondo->IsGameWorld() && Mondo->GetNetMode() != NM_Client)
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

	AValdorsoTane* AnimaliTaneDel(UWorld* Mondo)
	{
		AValdorsoTane* Tane = AValdorsoTane::Di(Mondo);
		if (!Tane && Mondo && Mondo->GetNetMode() != NM_Client)
		{
			FActorSpawnParameters Parametri;
			Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Tane = Mondo->SpawnActor<AValdorsoTane>(AValdorsoTane::StaticClass(), FTransform::Identity, Parametri);
		}
		return Tane;
	}

	void AnimaliChiama(UWorld* Console, FName QualeSpecie)
	{
		UWorld* Mondo = AnimaliMondoDiGioco(Console);
		if (!ValdorsoAnimali::Specie(QualeSpecie))
		{
			FString Nomi;
			for (const FValdorsoSpecie& S : ValdorsoAnimali::Tutte())
			{
				Nomi += FString(S.Id) + TEXT(" ");
			}
			UE_LOG(LogValdorso, Display, TEXT("Valdorso.Animali.Chiama <Specie>: %s"), *Nomi);
			return;
		}
		AValdorsoTane* Tane = AnimaliTaneDel(Mondo);
		APlayerController* Primo = nullptr;
		if (Mondo)
		{
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It && !Primo; ++It)
			{
				if (It->Get() && It->Get()->GetPawn())
				{
					Primo = It->Get();
				}
			}
		}
		const APawn* Colono = Primo ? Primo->GetPawn() : nullptr;
		if (!Tane || !Colono)
		{
			UE_LOG(LogValdorso, Display, TEXT("Valdorso.Animali.Chiama: serve un colono in gioco (sul server)."));
			return;
		}
		const FVector Davanti = Colono->GetActorLocation() + Colono->GetActorForwardVector().GetSafeNormal2D() * 2500.f;
		UE_LOG(LogValdorso, Display, TEXT("Valdorso.Animali.Chiama: %d %s."), Tane->Chiama(QualeSpecie, Davanti), *QualeSpecie.ToString());
	}

	void AnimaliVia(UWorld* Console)
	{
		if (AValdorsoTane* Tane = AValdorsoTane::Di(AnimaliMondoDiGioco(Console)))
		{
			Tane->Via();
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoAnimaliChiama(
		TEXT("Valdorso.Animali.Chiama"),
		TEXT("Per le prove: un gruppo di animali a 25 m dal primo colono, anche fuori dalla loro ora. Valdorso.Animali.Chiama <Lupo|Cinghiale|Orso|Cervo|Lepre|Volpe>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Console)
		{
			AnimaliChiama(Console, Argomenti.Num() > 0 ? FName(*Argomenti[0]) : NAME_None);
		}));

	FAutoConsoleCommandWithWorldAndArgs ComandoAnimaliVia(
		TEXT("Valdorso.Animali.Via"),
		TEXT("Tutti gli animali tornano alla tana."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Console) { AnimaliVia(Console); }));

	FAutoConsoleCommandWithWorldAndArgs ComandoLupiChiama(
		TEXT("Valdorso.Lupi.Chiama"),
		TEXT("Per le prove: un branco di lupi a 25 m dal primo colono, anche di giorno."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Console) { AnimaliChiama(Console, TEXT("Lupo")); }));

	FAutoConsoleCommandWithWorldAndArgs ComandoLupiVia(
		TEXT("Valdorso.Lupi.Via"),
		TEXT("Tutti gli animali tornano alla tana."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Console) { AnimaliVia(Console); }));
}

// ----------------------------------------------------------------------------------------------- le regole

namespace ValdorsoAnimali
{
	TArrayView<const FValdorsoSpecie> Tutte()
	{
		static const TArray<FValdorsoSpecie> Tabella = AnimaliCreaSpecie();
		return Tabella;
	}

	const FValdorsoSpecie* Specie(FName Id)
	{
		if (Id.IsNone())
		{
			return nullptr;
		}
		for (const FValdorsoSpecie& S : Tutte())
		{
			if (Id == FName(S.Id))
			{
				return &S;
			}
		}
		return nullptr;
	}

	const FValdorsoForma* Forma(FName Id)
	{
		for (const FValdorsoForma& F : AnimaliForme)
		{
			if (Id == FName(F.Id))
			{
				return &F;
			}
		}
		return nullptr;
	}

	TArrayView<const FValdorsoTana> Tane()
	{
		return TArrayView<const FValdorsoTana>(AnimaliTane, UE_ARRAY_COUNT(AnimaliTane));
	}

	bool Fuori(EValdorsoOraTana Ora, float LuceDelGiorno, bool bNebbia)
	{
		const bool bNotte = bNebbia || LuceDelGiorno < 0.25f;
		switch (Ora)
		{
		case EValdorsoOraTana::Notte: return bNotte;
		case EValdorsoOraTana::Giorno: return LuceDelGiorno >= 0.25f;
		default: return true;
		}
	}

	EValdorsoStatoLupo Prossimo(const FValdorsoSpecie& S, EValdorsoStatoLupo Attuale, float SaluteParte, float Distanza,
		bool bProtetta, float Secondi)
	{
		using E = EValdorsoStatoLupo;
		if (Attuale == E::Morto || SaluteParte <= 0.f)
		{
			return E::Morto;
		}
		const bool bVicino = Distanza >= 0.f && Distanza <= S.Fiuto;
		if (Attuale == E::Fuggi)
		{
			// le prede fuggono finche' sentono qualcuno; gli altri per un po'
			if (S.Indole == EValdorsoIndole::Preda && bVicino)
			{
				return E::Fuggi;
			}
			return Secondi < AnimaliSecondiFuga ? E::Fuggi : E::Vaga;
		}
		if (S.Indole == EValdorsoIndole::Preda)
		{
			return bVicino ? E::Fuggi : E::Vaga;
		}
		if (SaluteParte < S.Paura)
		{
			// ferito: fugge da chi lo caccia; dopo la fuga vaga e non attacca piu'
			return Attuale == E::Vaga ? E::Vaga : E::Fuggi;
		}
		if (Distanza < 0.f || Distanza > S.Lascia)
		{
			return E::Vaga;
		}
		if (S.Indole == EValdorsoIndole::Territoriale)
		{
			switch (Attuale)
			{
			case E::Vaga:
			case E::Fiuta:
				return bVicino && !bProtetta ? E::Accerchia : E::Vaga;
			case E::Accerchia:
				// avverte; se te ne vai lascia stare, se resti carica
				if (Distanza > S.Fiuto * 1.6f || bProtetta)
				{
					return E::Vaga;
				}
				return Secondi >= S.SecondiAvviso ? E::Morde : E::Accerchia;
			case E::Morde:
				return bProtetta || Secondi > 6.f ? E::Accerchia : E::Morde;
			default:
				return Attuale;
			}
		}
		// il predatore (il lupo)
		switch (Attuale)
		{
		case E::Vaga:
			return bVicino ? E::Fiuta : E::Vaga;
		case E::Fiuta:
			return Distanza <= 1200.f ? E::Accerchia : E::Fiuta;
		case E::Accerchia:
			// chi sta al fuoco o nella pace: si gira al largo, ma non si morde
			return !bProtetta && Secondi >= S.SecondiAvviso ? E::Morde : E::Accerchia;
		case E::Morde:
			// la preda e' corsa al fuoco, o non la si raggiunge: si torna a girare
			return bProtetta || Secondi > 6.f ? E::Accerchia : E::Morde;
		default:
			return Attuale;
		}
	}

	float Velocita(const FValdorsoSpecie& S, EValdorsoStatoLupo Stato)
	{
		switch (Stato)
		{
		case EValdorsoStatoLupo::Vaga: return S.Passo;
		case EValdorsoStatoLupo::Fiuta: return S.Passo * 1.4f;
		case EValdorsoStatoLupo::Accerchia:
			// il lupo gira di corsa; cinghiale e orso avanzano piano, minacciosi
			return S.Indole == EValdorsoIndole::Territoriale ? S.Passo * 0.5f : S.Corsa * 0.6f;
		case EValdorsoStatoLupo::Morde: return S.Corsa;
		case EValdorsoStatoLupo::Fuggi: return S.Corsa * 0.97f;
		default: return 0.f;
		}
	}

	int32 QuantiNelGruppo(const FValdorsoTana& Tana, int32 Volta)
	{
		const int32 Larghezza = FMath::Max(1, Tana.Massimo - Tana.Minimo + 1);
		return Tana.Minimo + ((Volta * 7 + 3) % Larghezza);
	}

	bool HaLama(const FValdorsoZaino& Zaino)
	{
		return ValdorsoZaino::Conta(Zaino, TEXT("Coltello")) > 0 || ValdorsoZaino::Conta(Zaino, TEXT("Pugnale")) > 0;
	}
}

namespace ValdorsoLupi
{
	bool Fuori(float LuceDelGiorno, bool bNebbia)
	{
		return ValdorsoAnimali::Fuori(EValdorsoOraTana::Notte, LuceDelGiorno, bNebbia);
	}

	EValdorsoStatoLupo Prossimo(EValdorsoStatoLupo Attuale, float SaluteParte, float Distanza, bool bProtetta, float Secondi)
	{
		const FValdorsoSpecie* Lupo = ValdorsoAnimali::Specie(TEXT("Lupo"));
		return Lupo ? ValdorsoAnimali::Prossimo(*Lupo, Attuale, SaluteParte, Distanza, bProtetta, Secondi) : Attuale;
	}

	int32 QuantiNelBranco(int32 Volta)
	{
		return (Volta % 3) == 1 ? 4 : 3;
	}

	float Velocita(EValdorsoStatoLupo Stato)
	{
		const FValdorsoSpecie* Lupo = ValdorsoAnimali::Specie(TEXT("Lupo"));
		return Lupo ? ValdorsoAnimali::Velocita(*Lupo, Stato) : 0.f;
	}
}

// ----------------------------------------------------------------------------------------------- l'animale

AValdorsoAnimale::AValdorsoAnimale()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetUpdateFrequency(20.f);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 40.f);
	// la telecamera dei coloni non si ferma contro un animale
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetVisibility(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UCharacterMovementComponent* Movimento = GetCharacterMovement();
	Movimento->bOrientRotationToMovement = true;
	Movimento->RotationRate = FRotator(0.f, 420.f, 0.f);
	Movimento->MaxWalkSpeed = 170.f;
	Movimento->MaxStepHeight = 35.f;
	Movimento->SetWalkableFloorAngle(50.f);
	bUseControllerRotationYaw = false;

	Abilita = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Abilita"));
	Abilita->SetIsReplicated(true);
	Abilita->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Attributi = CreateDefaultSubobject<UValdorsoAttributeSet>(TEXT("Attributi"));
	Reazioni = CreateDefaultSubobject<UValdorsoReazioniComponent>(TEXT("Reazioni"));

	// sul Mesh (vuoto, nascosto): sui PC degli altri il motore ammorbidisce il suo movimento, non quello della capsula
	Figura = CreateDefaultSubobject<USceneComponent>(TEXT("Figura"));
	Figura->SetupAttachment(GetMesh());
	Corpo = AnimaliPezzo(this, Figura, TEXT("Corpo"));
	Testa = AnimaliPezzo(this, Figura, TEXT("Testa"));
	Coda = AnimaliPezzo(this, Figura, TEXT("Coda"));
	const TCHAR* NomiZampe[4] = { TEXT("ZampaAvantiSinistra"), TEXT("ZampaAvantiDestra"), TEXT("ZampaDietroSinistra"), TEXT("ZampaDietroDestra") };
	for (int32 i = 0; i < 4; ++i)
	{
		Zampe.Add(AnimaliPezzo(this, Figura, NomiZampe[i]));
	}
}

void AValdorsoAnimale::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoAnimale, Specie);
	DOREPLIFETIME(AValdorsoAnimale, Stato);
	DOREPLIFETIME(AValdorsoAnimale, bScuoiato);
}

const FValdorsoSpecie* AValdorsoAnimale::GetSpecie() const
{
	const FValdorsoSpecie* S = ValdorsoAnimali::Specie(Specie);
	return S ? S : ValdorsoAnimali::Specie(TEXT("Lupo"));
}

void AValdorsoAnimale::Prepara(FName QualeSpecie, const FVector& DoveTana)
{
	Specie = QualeSpecie;
	Tana = DoveTana;
}

void AValdorsoAnimale::BeginPlay()
{
	Super::BeginPlay();
	Abilita->InitAbilityActorInfo(this, this);
	Vesti();
	const FValdorsoSpecie* S = GetSpecie();
	if (HasAuthority() && S)
	{
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteMaxAttribute(), S->Salute);
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute(), S->Salute);
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
		ProssimoRichiamo = GetWorld()->GetTimeSeconds() + FMath::FRandRange(8.f, 40.f);
	}
}

void AValdorsoAnimale::OnRep_Specie()
{
	Vesti();
}

void AValdorsoAnimale::Vesti()
{
	if (Vestito == Specie)
	{
		return;
	}
	Vestito = Specie;
	const FValdorsoForma* F = ValdorsoAnimali::Forma(Specie);
	if (!F)
	{
		return;
	}
	// la capsula: larga come l'animale (non troppo), alta fino alla schiena
	const float Raggio = FMath::Clamp(F->MezzoLargo * 1.6f, 12.f, 60.f);
	const float MezzaAltezza = FMath::Max(Raggio, F->CorpoZ * 0.75f);
	GetCapsuleComponent()->SetCapsuleSize(Raggio, MezzaAltezza);
	GetCharacterMovement()->MaxStepHeight = FMath::Min(35.f, MezzaAltezza);
	Figura->SetRelativeLocation(FVector(0.f, 0.f, -MezzaAltezza));
	Corpo->SetRelativeLocation(FVector(0.f, 0.f, F->CorpoZ));
	Testa->SetRelativeLocation(FVector(F->TestaX, 0.f, F->TestaZ));
	Coda->SetRelativeLocation(FVector(F->CodaX, 0.f, F->CodaZ));
	const FVector PostiZampe[4] = { FVector(F->ZampeAvantiX, -F->ZampeY, F->ZampaLunga), FVector(F->ZampeAvantiX, F->ZampeY, F->ZampaLunga),
		FVector(F->ZampeDietroX, -F->ZampeY, F->ZampaLunga), FVector(F->ZampeDietroX, F->ZampeY, F->ZampaLunga) };
	for (int32 i = 0; i < Zampe.Num() && i < 4; ++i)
	{
		Zampe[i]->SetRelativeLocation(PostiZampe[i]);
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const FString Nome = Specie.ToString();
	Corpo->SetStaticMesh(AnimaliModello(FString::Printf(TEXT("SM_%s_Corpo"), *Nome)));
	Testa->SetStaticMesh(AnimaliModello(FString::Printf(TEXT("SM_%s_Testa"), *Nome)));
	Coda->SetStaticMesh(AnimaliModello(FString::Printf(TEXT("SM_%s_Coda"), *Nome)));
	UStaticMesh* Zampa = AnimaliModello(FString::Printf(TEXT("SM_%s_Zampa"), *Nome));
	for (UStaticMeshComponent* Pezzo : Zampe)
	{
		Pezzo->SetStaticMesh(Zampa);
	}
	// un po' diversi l'uno dall'altro
	Figura->SetRelativeScale3D(FVector(FMath::FRandRange(0.92f, 1.08f)));
}

void AValdorsoAnimale::Ferito(AActor* Da)
{
	if (!HasAuthority() || !Vivo())
	{
		return;
	}
	UltimoAggressore = Da;
	APawn* Chi = Cast<APawn>(Da);
	const FValdorsoSpecie* S = GetSpecie();
	if (!Chi || !Chi->IsPlayerControlled() || !S)
	{
		return;
	}
	Preda = Chi;
	if (S->Indole == EValdorsoIndole::Preda)
	{
		Cambia(EValdorsoStatoLupo::Fuggi);
	}
	else if (Stato == EValdorsoStatoLupo::Vaga || Stato == EValdorsoStatoLupo::Fiuta)
	{
		// chi viene colpito risponde subito (il cinghiale e l'orso caricano)
		Cambia(S->Indole == EValdorsoIndole::Territoriale ? EValdorsoStatoLupo::Morde : EValdorsoStatoLupo::Accerchia);
	}
}

void AValdorsoAnimale::Stordisci(float Secondi)
{
	if (!HasAuthority() || !Vivo() || Secondi <= 0.f)
	{
		return;
	}
	StorditoFino = FMath::Max(StorditoFino, GetWorld()->GetTimeSeconds() + Secondi);
	GetCharacterMovement()->StopMovementImmediately();
}

APawn* AValdorsoAnimale::CercaPreda() const
{
	const FValdorsoSpecie* S = GetSpecie();
	APawn* Migliore = nullptr;
	float Vicino = S ? S->Fiuto : 0.f;
	// chi e' gia' la preda la si segue fino a Lascia
	if (APawn* Attuale = Preda.Get())
	{
		const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Attuale);
		const float Distanza = FVector::Dist(Attuale->GetActorLocation(), GetActorLocation());
		if (!(Pericoli && Pericoli->Caduto()) && S && Distanza <= S->Lascia)
		{
			Migliore = Attuale;
			Vicino = Distanza;
		}
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Colono = AnimaliColono(It->Get());
		if (!Colono || Colono == Migliore)
		{
			continue;
		}
		// (11/11, v0.1.5) la Caccia: il signore del bosco (100) non lo cercano i predatori se non li ha colpiti
		if (S && S->Indole == EValdorsoIndole::Predatore && Colono != UltimoAggressore.Get()
			&& ValdorsoScherma::Talento(Colono, FName(TEXT("Caccia")), 4))
		{
			continue;
		}
		// (11/11, v0.1.5) il passo del cacciatore (75): lo si sente da piu' vicino (come se fosse piu' lontano)
		const float Orecchio = ValdorsoScherma::Talento(Colono, FName(TEXT("Caccia")), 3) ? 1.f / 0.6f : 1.f;
		const float Distanza = FVector::Dist(Colono->GetActorLocation(), GetActorLocation()) * Orecchio;
		// si cambia preda solo per una molto piu' vicina
		if (Distanza < (Migliore ? Vicino * 0.5f : Vicino))
		{
			Migliore = Colono;
			Vicino = Distanza;
		}
	}
	return Migliore;
}

void AValdorsoAnimale::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		Pensa(DeltaSeconds);
		// (11/11) stordito: pensa (e puo' morire) ma non si muove e non morde
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

void AValdorsoAnimale::Pensa(float DeltaSeconds)
{
	UWorld* Mondo = GetWorld();
	const FValdorsoSpecie* S = GetSpecie();
	if (!Vivo() || !S)
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
		ProssimaRicerca = OraMondo + 0.5;
		Preda = CercaPreda();
		const APawn* Chi = Preda.Get();
		bPredaProtetta = Chi && (AValdorsoFalo::CaldoVicino(Mondo, Chi->GetActorLocation())
			|| ValdorsoCombattimento::InPace(Mondo, Chi->GetActorLocation()));
	}
	const APawn* Chi = Preda.Get();
	const float Distanza = Chi ? FVector::Dist(Chi->GetActorLocation(), GetActorLocation()) : -1.f;
	const EValdorsoStatoLupo Nuovo = ValdorsoAnimali::Prossimo(*S, Stato, Salute / FMath::Max(1.f, Massima), Distanza, bPredaProtetta,
		static_cast<float>(OraMondo - InizioStato));
	if (Nuovo != Stato)
	{
		Cambia(Nuovo);
	}
	// il morso (o la carica): vicino abbastanza, e non troppo spesso (e non da stordito)
	if (Stato == EValdorsoStatoLupo::Morde && Chi && S->Danno > 0.f && Distanza <= S->Portata && OraMondo >= ProssimoMorso && OraMondo >= StorditoFino)
	{
		ProssimoMorso = OraMondo + S->SecondiMorso;
		const FVector Punto = Chi->GetActorLocation() + (GetActorLocation() - Chi->GetActorLocation()).GetSafeNormal() * 30.f;
		const EValdorsoTipoColpo TipoColpo = S->Indole == EValdorsoIndole::Territoriale ? EValdorsoTipoColpo::Botta : EValdorsoTipoColpo::Morso;
		ValdorsoCombattimento::Colpisci(this, const_cast<APawn*>(Chi), S->Danno, S->Equilibrio, Punto, TipoColpo);
		// dopo il colpo si torna a girare o ad avvertire (e a volte dall'altra parte)
		VersoGiro = FMath::RandBool() ? 1.f : -1.f;
		Cambia(EValdorsoStatoLupo::Accerchia);
	}
}

void AValdorsoAnimale::Muoviti(float DeltaSeconds)
{
	UCharacterMovementComponent* Movimento = GetCharacterMovement();
	const UWorld* Mondo = GetWorld();
	const FValdorsoSpecie* S = GetSpecie();
	if (!Movimento || !Vivo() || !S)
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
		// un punto a caso intorno alla tana, ogni tanto (a volte si ferma a brucare o ad annusare)
		if (OraMondo >= NuovaMeta || FVector::Dist2D(Qui, Meta) < 150.f)
		{
			NuovaMeta = OraMondo + FMath::FRandRange(5.f, 12.f);
			const float Angolo = FMath::FRandRange(0.f, 2.f * PI);
			const float Raggio = 1500.f * FMath::Sqrt(FMath::FRand());
			Meta = FMath::RandBool() || S->Indole == EValdorsoIndole::Predatore
				? Tana + FVector(FMath::Cos(Angolo) * Raggio, FMath::Sin(Angolo) * Raggio, 0.f) : Qui;
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
		if (Chi && S->Indole == EValdorsoIndole::Territoriale)
		{
			// avanza piano verso chi gli e' troppo vicino, ma non oltre qualche metro
			if (FVector::Dist2D(Qui, Chi->GetActorLocation()) > S->Portata * 2.f)
			{
				Verso = Chi->GetActorLocation() - Qui;
			}
		}
		else if (Chi)
		{
			const float Raggio = bPredaProtetta ? ValdorsoLupi::GiroAlLargo : S->Giro;
			AngoloGiro += VersoGiro * DeltaSeconds * (ValdorsoAnimali::Velocita(*S, Stato) / Raggio) * 0.8f;
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
	Movimento->MaxWalkSpeed = ValdorsoAnimali::Velocita(*S, Stato);
	if (Verso.SizeSquared() < 40.f * 40.f)
	{
		// fermo: guarda chi ha davanti (il cinghiale e l'orso che avvertono)
		if (Chi && Stato == EValdorsoStatoLupo::Accerchia)
		{
			const FVector Da = (Chi->GetActorLocation() - Qui).GetSafeNormal2D();
			if (!Da.IsNearlyZero())
			{
				SetActorRotation(FMath::RInterpTo(GetActorRotation(), Da.Rotation(), DeltaSeconds, 5.f));
			}
		}
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

void AValdorsoAnimale::Cambia(EValdorsoStatoLupo Nuovo)
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

void AValdorsoAnimale::OnRep_Stato()
{
	// chi entra in gioco adesso non sente i versi di quello che e' gia' successo
	if (HasActorBegunPlay())
	{
		SuonaStato();
	}
	if (Stato == EValdorsoStatoLupo::Morto)
	{
		// anche qui il corpo non ferma piu' nessuno
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
}

void AValdorsoAnimale::SuonaStato()
{
	const FValdorsoSpecie* S = GetSpecie();
	if (!S)
	{
		return;
	}
	const TCHAR* Nome = nullptr;
	float Lontano = 2500.f;
	switch (Stato)
	{
	case EValdorsoStatoLupo::Accerchia:
		Nome = S->SuonoAvviso;
		break;
	case EValdorsoStatoLupo::Fuggi:
	case EValdorsoStatoLupo::Morto:
		Nome = S->SuonoDolore;
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

void AValdorsoAnimale::Muori()
{
	Cambia(EValdorsoStatoLupo::Morto);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	// resta a terra un po': chi ha un coltello lo scuoia (E)
	SetLifeSpan(ValdorsoLupi::SecondiATerra);
	const FValdorsoSpecie* S = GetSpecie();
	ValdorsoRegistroEventi::Annota(TEXT("Animale"), UltimoAggressore.Get(), this, GetActorLocation(),
		FString::Printf(TEXT("%s ucciso"), S ? S->Id : TEXT("animale")));
	// (10/11) le cacce della Gilda: conta la preda a chi l'ha colpita per ultimo (ValdorsoGilda.h)
	UValdorsoGildaComponent::Preda(UltimoAggressore.Get(), Specie);
}

FText AValdorsoAnimale::TestoInterazione(const APawn* Chi) const
{
	const FValdorsoSpecie* S = GetSpecie();
	if (Vivo() || bScuoiato || !S || !S->Bottino[0])
	{
		return FText::GetEmpty();
	}
	const APlayerController* Controllore = Chi ? Cast<APlayerController>(Chi->GetController()) : nullptr;
	const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
	if (Zaino && !ValdorsoAnimali::HaLama(Zaino->Contenuto()))
	{
		return FText::Format(LOCTEXT("SenzaLama", "Per scuoiare {0} ti serve un coltello"), FText::FromString(S->ConArticolo));
	}
	return FText::Format(LOCTEXT("Scuoia", "Scuoia {0}"), FText::FromString(S->ConArticolo));
}

void AValdorsoAnimale::Interagisci(APawn* Chi)
{
	const FValdorsoSpecie* S = GetSpecie();
	if (!HasAuthority() || Vivo() || bScuoiato || !S)
	{
		return;
	}
	AValdorsoPlayerController* Controllore = Chi ? Cast<AValdorsoPlayerController>(Chi->GetController()) : nullptr;
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
	if (!Zaino)
	{
		return;
	}
	if (!ValdorsoAnimali::HaLama(Zaino->Contenuto()))
	{
		Controllore->ClientAvviso(FText::Format(LOCTEXT("SenzaLamaAvviso", "Per scuoiare {0} ti serve un coltello."),
			FText::FromString(S->ConArticolo)), 3.f, 0.f);
		return;
	}
	bool bTutto = true;
	// (11/11, v0.1.5) la Caccia: le pelli migliori (25), una in piu' della prima cosa (la pelle)
	const int32 InPiu = ValdorsoScherma::Talento(Chi, FName(TEXT("Caccia")), 1) ? 1 : 0;
	for (int32 i = 0; i < 3; ++i)
	{
		if (S->Bottino[i] && S->Quanti[i] > 0)
		{
			const int32 Quanti = S->Quanti[i] + (i == 0 ? InPiu : 0);
			bTutto &= Zaino->Dai(S->Bottino[i], Quanti) >= Quanti;
		}
	}
	if (!bTutto)
	{
		Controllore->ClientAvviso(LOCTEXT("ZainoPieno", "Lo zaino è pieno: parte di quello che hai preso resta a terra."), 3.f, 0.f);
	}
	bScuoiato = true;
	ValdorsoRegistroEventi::Annota(TEXT("Scuoia"), Chi, this, GetActorLocation(), FString::Printf(TEXT("%s scuoiato"), S->Id));
	// (10/11, scheletro della v0.1.5) scuoiare fa crescere la Caccia
	UValdorsoCrescitaComponent::UsaDa(Chi, FName(TEXT("Caccia")), 1.f, this);
	OnRep_Scuoiato();
	// quello che resta sparisce presto
	SetLifeSpan(8.f);
}

void AValdorsoAnimale::OnRep_Scuoiato()
{
	if (!bScuoiato || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (HasActorBegunPlay())
	{
		ValdorsoCombattimento::Suona(GetWorld(), ValdorsoCombattimento::SuonoPericoli(TEXT("S_Scuoia")), GetActorLocation(), 1500.f);
	}
	// la pelle non c'e' piu': resta poco, piu' scuro e piu' piccolo
	Figura->SetRelativeScale3D(Figura->GetRelativeScale3D() * FVector(0.85f, 0.85f, 0.5f));
}

void AValdorsoAnimale::Anima(float DeltaSeconds)
{
	const FValdorsoSpecie* S = GetSpecie();
	const FValdorsoForma* F = ValdorsoAnimali::Forma(Specie);
	if (!S || !F)
	{
		return;
	}
	const float Tempo = static_cast<float>(GetWorld()->GetTimeSeconds());
	const float Veloce = static_cast<float>(GetVelocity().Size2D());
	const bool bMorto = Stato == EValdorsoStatoLupo::Morto;

	// le zampe: avanti e indietro con il passo (piu' larghe quando corre; il passo va con la lunghezza della zampa)
	Passo = FMath::Fmod(Passo + DeltaSeconds * Veloce / FMath::Max(6.f, F->ZampaLunga * 0.4f), 2.f * PI);
	const float Ampiezza = bMorto ? 0.f : FMath::Clamp(Veloce / FMath::Max(100.f, S->Corsa * 0.5f), 0.f, 1.f) * 32.f;
	for (int32 i = 0; i < Zampe.Num() && i < 4; ++i)
	{
		Zampe[i]->SetRelativeRotation(FRotator(FMath::Sin(Passo + AnimaliFaseZampe[i]) * Ampiezza, 0.f, 0.f));
	}
	// la testa: giu' per fiutare (o brucare) e per mordere, su quando avverte; un poco su e giu' col passo
	float Giu = 0.f;
	switch (Stato)
	{
	case EValdorsoStatoLupo::Vaga: Giu = Veloce < 20.f && S->Indole == EValdorsoIndole::Preda ? -35.f : -12.f; break;
	case EValdorsoStatoLupo::Fiuta: Giu = -18.f; break;
	case EValdorsoStatoLupo::Morde: Giu = -10.f + FMath::Sin(Tempo * 14.f) * 8.f; break;
	case EValdorsoStatoLupo::Fuggi: Giu = S->Indole == EValdorsoIndole::Preda ? 8.f : -22.f; break;
	case EValdorsoStatoLupo::Morto: Giu = -30.f; break;
	default: Giu = S->Indole == EValdorsoIndole::Territoriale ? -15.f + FMath::Sin(Tempo * 6.f) * 6.f : -4.f; break;
	}
	Testa->SetRelativeRotation(FRotator(Giu + FMath::Sin(Passo * 2.f) * Ampiezza * 0.12f, FMath::Sin(Tempo * 0.7f) * 6.f, 0.f));
	// la coda: bassa quando fugge o caccia, scodinzola poco (la lepre e il cervo la alzano quando scappano)
	float Bassa = Caccia() ? -5.f : 8.f;
	if (Stato == EValdorsoStatoLupo::Fuggi)
	{
		Bassa = S->Indole == EValdorsoIndole::Preda ? 35.f : -30.f;
	}
	Coda->SetRelativeRotation(FRotator(bMorto ? -10.f : Bassa, bMorto ? 0.f : FMath::Sin(Tempo * 5.f) * 12.f, 0.f));
	// morto: si accascia su un fianco
	Accasciato = FMath::FInterpTo(Accasciato, bMorto ? 1.f : 0.f, DeltaSeconds, 4.f);
	const float MezzaAltezza = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	Figura->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -MezzaAltezza + Accasciato * F->MezzoLargo * 0.8f),
		FRotator(0.f, 0.f, Accasciato * 82.f));

	// ogni tanto, chi vaga si fa sentire (il lupo ulula di notte; il cervo bramisce, il cinghiale grugnisce di giorno)
	const double OraMondo = GetWorld()->GetTimeSeconds();
	if (!bMorto && S->SuonoRichiamo && OraMondo >= ProssimoRichiamo)
	{
		ProssimoRichiamo = OraMondo + FMath::FRandRange(35.f, 90.f);
		const bool bNotte = AnimaliLuce(GetWorld()) < 0.25f;
		if (Stato == EValdorsoStatoLupo::Vaga && bNotte == S->bRichiamoDiNotte)
		{
			const float Lontano = S->Indole == EValdorsoIndole::Predatore ? 15000.f : 5000.f;
			ValdorsoCombattimento::Suona(GetWorld(), ValdorsoCombattimento::SuonoPericoli(S->SuonoRichiamo), GetActorLocation(), Lontano,
				1.f, FMath::FRandRange(0.9f, 1.08f));
		}
	}
}

// ----------------------------------------------------------------------------------------------- le tane

AValdorsoTane::AValdorsoTane()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);
}

AValdorsoTane* AValdorsoTane::Di(const UWorld* Mondo)
{
	if (!Mondo)
	{
		return nullptr;
	}
	for (TActorIterator<AValdorsoTane> It(const_cast<UWorld*>(Mondo)); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void AValdorsoTane::Prepara()
{
	bPronte = true;
	for (const FValdorsoTana& Regola : ValdorsoAnimali::Tane())
	{
		const FValdorsoLuogo* Luogo = ValdorsoLuoghi::Trova(Regola.Luogo);
		if (!Luogo || !ValdorsoAnimali::Specie(Regola.Specie))
		{
			continue;
		}
		FTanaViva Viva;
		Viva.Regola = &Regola;
		// le tane non stanno tutte nel mezzo del luogo: un poco di lato, ognuna la sua
		const float Angolo = static_cast<float>(Vive.Num()) * 2.39996f;
		Viva.Centro = Luogo->Posizione + FVector(FMath::Cos(Angolo), FMath::Sin(Angolo), 0.f) * Luogo->Raggio * 0.35f;
		Vive.Add(Viva);
	}
	UE_LOG(LogValdorso, Log, TEXT("Animali: %d tane."), Vive.Num());
}

bool AValdorsoTane::Terra(const FVector& Sopra, FVector& OutTerra) const
{
	// la prima cosa sotto, che non sia un albero (i tronchi del bosco) e che sia abbastanza piana
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoAnimaliTerra), false);
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
		OutTerra = Colpo.ImpactPoint;
		return true;
	}
	return false;
}

int32 AValdorsoTane::Gruppo(FName QualeSpecie, int32 Quanti, const FVector& Dove, TArray<TWeakObjectPtr<AValdorsoAnimale>>& Dentro)
{
	UWorld* Mondo = GetWorld();
	const FValdorsoForma* F = ValdorsoAnimali::Forma(QualeSpecie);
	const float Alto = F ? FMath::Max(FMath::Clamp(F->MezzoLargo * 1.6f, 12.f, 60.f), F->CorpoZ * 0.75f) : 40.f;
	int32 Usciti = 0;
	for (int32 i = 0; i < Quanti; ++i)
	{
		const float Angolo = (2.f * PI * i) / FMath::Max(1, Quanti) + FMath::FRandRange(-0.3f, 0.3f);
		const FVector Punto = Dove + FVector(FMath::Cos(Angolo), FMath::Sin(Angolo), 0.f) * FMath::FRandRange(200.f, 500.f);
		FVector Terreno;
		if (!Terra(Punto, Terreno))
		{
			continue;
		}
		const FTransform Posto(FRotator(0.f, FMath::FRandRange(-180.f, 180.f), 0.f), Terreno + FVector(0.f, 0.f, Alto + 15.f));
		AValdorsoAnimale* Animale = Mondo->SpawnActorDeferred<AValdorsoAnimale>(AValdorsoAnimale::StaticClass(), Posto, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
		if (!Animale)
		{
			continue;
		}
		Animale->Prepara(QualeSpecie, Dove);
		Animale->FinishSpawning(Posto);
		if (IsValid(Animale))
		{
			Dentro.Add(Animale);
			++Usciti;
		}
	}
	return Usciti;
}

int32 AValdorsoTane::Chiama(FName QualeSpecie, const FVector& Vicino)
{
	int32 Quanti = 1;
	for (const FValdorsoTana& Regola : ValdorsoAnimali::Tane())
	{
		if (QualeSpecie == FName(Regola.Specie))
		{
			Quanti = ValdorsoAnimali::QuantiNelGruppo(Regola, Volte++);
			break;
		}
	}
	return Gruppo(QualeSpecie, Quanti, Vicino, Chiamati);
}

void AValdorsoTane::Via()
{
	auto Manda = [](TArray<TWeakObjectPtr<AValdorsoAnimale>>& Animali)
	{
		for (const TWeakObjectPtr<AValdorsoAnimale>& Animale : Animali)
		{
			if (Animale.IsValid())
			{
				Animale->Destroy();
			}
		}
		Animali.Reset();
	};
	for (FTanaViva& Viva : Vive)
	{
		Manda(Viva.Animali);
		Viva.bFuori = false;
	}
	Manda(Chiamati);
}

void AValdorsoTane::Tick(float DeltaSeconds)
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
	const float Luce = AnimaliLuce(Mondo);
	const bool bNebbia = AnimaliNebbia(Mondo);
	for (FTanaViva& Viva : Vive)
	{
		const FValdorsoTana& Regola = *Viva.Regola;
		const bool bOra = ValdorsoAnimali::Fuori(Regola.Ora, Luce, bNebbia);
		// quanto e' vicino il colono piu' vicino
		float Vicino = TNumericLimits<float>::Max();
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			if (const APawn* Colono = AnimaliColono(It->Get()))
			{
				Vicino = FMath::Min(Vicino, static_cast<float>(FVector::Dist2D(Colono->GetActorLocation(), Viva.Centro)));
			}
		}
		int32 Vivi = 0;
		bool bCaccia = false;
		for (const TWeakObjectPtr<AValdorsoAnimale>& Animale : Viva.Animali)
		{
			if (Animale.IsValid() && Animale->Vivo())
			{
				++Vivi;
				bCaccia |= Animale->Caccia();
				// caduto fuori dal mondo (una parte di valle non caricata): via
				if (Animale->GetActorLocation().Z < Viva.Centro.Z - 20000.f)
				{
					Animale->Destroy();
				}
			}
		}
		if (Viva.bFuori && Vivi == 0)
		{
			// il gruppo e' stato ucciso tutto: torna fra i suoi minuti
			Viva.bFuori = false;
			Viva.Animali.Reset();
			Viva.Ritorno = OraMondo + Regola.MinutiRitorno * 60.0;
			UE_LOG(LogValdorso, Log, TEXT("Animali: il gruppo di %s (%s) e' finito, torna fra %.0f minuti."), Regola.Specie, Regola.Luogo,
				Regola.MinutiRitorno);
			continue;
		}
		if (Viva.bFuori && !bCaccia && (!bOra || Vicino > ValdorsoLupi::LontanoTana))
		{
			// non e' piu' la sua ora, o nessuno vicino: tornano alla tana (i morti restano a terra)
			for (const TWeakObjectPtr<AValdorsoAnimale>& Animale : Viva.Animali)
			{
				if (Animale.IsValid() && Animale->Vivo())
				{
					Animale->Destroy();
				}
			}
			Viva.Animali.Reset();
			Viva.bFuori = false;
			continue;
		}
		if (!Viva.bFuori && bOra && Vicino <= ValdorsoLupi::VicinoTana && OraMondo >= Viva.Ritorno)
		{
			const int32 Usciti = Gruppo(Regola.Specie, ValdorsoAnimali::QuantiNelGruppo(Regola, Volte++), Viva.Centro, Viva.Animali);
			Viva.bFuori = Usciti > 0;
			if (Usciti > 0)
			{
				UE_LOG(LogValdorso, Log, TEXT("Animali: %d %s escono dalla tana (%s)."), Usciti, Regola.Specie, Regola.Luogo);
			}
			else
			{
				// il terreno non c'e' ancora (parte di valle non caricata): si riprova fra mezzo minuto
				Viva.Ritorno = OraMondo + 30.0;
			}
		}
	}
	Chiamati.RemoveAll([](const TWeakObjectPtr<AValdorsoAnimale>& Animale) { return !Animale.IsValid(); });
}

#undef LOCTEXT_NAMESPACE
