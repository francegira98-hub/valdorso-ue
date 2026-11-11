// Valdorso - Bagnati e asciutti (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoBagnato.h"
#include "ValdorsoVigore.h"
#include "Valdorso.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoFalo.h"
#include "ValdorsoCielo.h"
#include "ValdorsoClima.h"
#include "ValdorsoStatoMondo.h"
#include "ValdorsoMovementComponent.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoTempo.h"
#include "ValdorsoZaino.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"

namespace
{
	/** Ogni quanto (secondi) il server guarda acqua, pioggia e tetto. */
	constexpr double BagnatoSecondiControllo = 0.5;
	/** Il tetto si cerca un poco più di rado (una linea verso l'alto). */
	constexpr double BagnatoSecondiTetto = 1.0;
	/** Quanto deve essere sott'acqua il corpo per bagnarsi tutto (0..1: 0,45 = la vita). */
	constexpr float BagnatoAcquaVita = 0.45f;
	/** Il ritorno della stamina di serie (UValdorsoGE_RecuperoStamina: 10 al secondo) con un margine: più di così non è
	 *  il ritorno (è cibo, o altro) e non si frena. */
	constexpr float BagnatoRitornoAlSecondo = 12.f;

	FAutoConsoleCommandWithWorldAndArgs ComandoBagnato(
		TEXT("Valdorso.Bagnato"),
		TEXT("Per le prove: quanto è bagnato il colono, Valdorso.Bagnato <0..1> [nome] (sul server, o da soli)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			if (Argomenti.Num() < 1 || !Mondo)
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Bagnato: serve un numero da 0 a 1, per esempio Valdorso.Bagnato 1"));
				return;
			}
			const float Quanto = FMath::Clamp(FCString::Atof(*Argomenti[0]), 0.f, 1.f);
			const FString Nome = Argomenti.Num() > 1 ? Argomenti[1] : FString();
			int32 Trovati = 0;
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				const APlayerController* Controllore = It->Get();
				UValdorsoBagnatoComponent* Bagnato = UValdorsoBagnatoComponent::Di(Controllore ? Controllore->GetPawn() : nullptr);
				const APlayerState* Stato = Controllore ? Controllore->PlayerState.Get() : nullptr;
				if (!Bagnato || !Controllore->HasAuthority()
					|| (!Nome.IsEmpty() && (!Stato || !Stato->GetPlayerName().Equals(Nome, ESearchCase::IgnoreCase))))
				{
					continue;
				}
				Bagnato->Imposta(Quanto);
				++Trovati;
			}
			UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Bagnato: %.2f a %d coloni"), Quanto, Trovati);
		}));
}

namespace
{
	FAutoConsoleCommandWithWorld ComandoClima(
		TEXT("Valdorso.Clima"),
		TEXT("Scrive l'aria, la temperatura che sente ogni colono e come si sente (sul server, o da soli: l'aria la sa solo il server)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* Mondo)
		{
			if (!Mondo)
			{
				return;
			}
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				const APlayerController* Controllore = It->Get();
				const APawn* Colono = Controllore ? Controllore->GetPawn() : nullptr;
				const UValdorsoBagnatoComponent* Clima = UValdorsoBagnatoComponent::Di(Colono);
				if (!Clima)
				{
					continue;
				}
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Clima: %s, aria %.1f°, sente %.1f° (%s), bagnato %.2f"),
					*Colono->GetName(), Clima->TemperaturaAria(), Clima->Temperatura(),
					*ValdorsoClima::Nome(ValdorsoClima::Sensazione(Clima->Temperatura())).ToString(), Clima->Bagnato());
			}
		}));
}

namespace ValdorsoBagnato
{
	float Passo(float Bagnato, float Delta, bool bInAcqua, float Pioggia, bool bTetto, bool bMantello, bool bFuoco)
	{
		if (bInAcqua)
		{
			return 1.f;
		}
		Delta = FMath::Max(0.f, Delta);
		if (bFuoco)
		{
			return FMath::Clamp(Bagnato - Delta / SecondiAsciugaFuoco, 0.f, 1.f);
		}
		if (Pioggia > 0.05f && !bTetto)
		{
			const float Secondi = SecondiPioggia * (bMantello ? 2.f : 1.f);
			return FMath::Clamp(Bagnato + Delta * FMath::Clamp(Pioggia, 0.f, 1.f) / Secondi, 0.f, 1.f);
		}
		const float Secondi = bTetto ? SecondiAsciugaDentro : SecondiAsciugaFuori;
		return FMath::Clamp(Bagnato - Delta / Secondi, 0.f, 1.f);
	}

	bool Infreddolito(float Bagnato)
	{
		return Bagnato >= Soglia;
	}

	float PassoFiato(float Fiato, float Delta, bool bSottAcqua)
	{
		Delta = FMath::Max(0.f, Delta);
		return FMath::Clamp(Fiato + (bSottAcqua ? -Delta / SecondiFiato : Delta / SecondiRespiro), 0.f, 1.f);
	}
}

UValdorsoBagnatoComponent::UValdorsoBagnatoComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	SetIsReplicatedByDefault(true);
}

UValdorsoBagnatoComponent* UValdorsoBagnatoComponent::Di(const AActor* Corpo)
{
	return Corpo ? Corpo->FindComponentByClass<UValdorsoBagnatoComponent>() : nullptr;
}

void UValdorsoBagnatoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UValdorsoBagnatoComponent, Livello);
	DOREPLIFETIME(UValdorsoBagnatoComponent, Percepita);
	// (10/11) il fiato lo vede solo chi nuota
	DOREPLIFETIME_CONDITION(UValdorsoBagnatoComponent, FiatoInRete, COND_OwnerOnly);
}

void UValdorsoBagnatoComponent::Imposta(float Nuovo)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		LivelloEsatto = FMath::Clamp(Nuovo, 0.f, 1.f);
		Livello = LivelloEsatto;
	}
}

bool UValdorsoBagnatoComponent::TettoSopra() const
{
	const AActor* Corpo = GetOwner();
	UWorld* Mondo = GetWorld();
	if (!Corpo || !Mondo)
	{
		return false;
	}
	// come la pioggia di AValdorsoTempo: una linea di 30 m verso l'alto dalla testa
	const FVector Testa = Corpo->GetActorLocation() + FVector(0.f, 0.f, 80.f);
	FHitResult Colpo;
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoBagnatoTetto), false, Corpo);
	return Mondo->LineTraceSingleByChannel(Colpo, Testa, Testa + FVector(0.f, 0.f, 3000.f), ECC_Visibility, Parametri);
}

bool UValdorsoBagnatoComponent::HaMantello() const
{
	const APawn* Corpo = Cast<APawn>(GetOwner());
	const APlayerController* Controllore = Corpo ? Cast<APlayerController>(Corpo->GetController()) : nullptr;
	const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
	// (09/11) anche il mantello di pelle di lupo ripara dalla pioggia
	return Zaino && (ValdorsoZaino::Conta(Zaino->Contenuto(), TEXT("Mantello")) > 0
		|| ValdorsoZaino::Conta(Zaino->Contenuto(), TEXT("Mantello_Lupo")) > 0
		|| ValdorsoZaino::Conta(Zaino->Contenuto(), TEXT("Mantello_Orso")) > 0);
}

float UValdorsoBagnatoComponent::CaloreVestiti() const
{
	const APawn* Corpo = Cast<APawn>(GetOwner());
	const APlayerController* Controllore = Corpo ? Cast<APlayerController>(Corpo->GetController()) : nullptr;
	const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
	if (!Zaino)
	{
		return 0.f;
	}
	// il piu' caldo di quelli che si portano (non si sommano: un mantello alla volta)
	float Caldo = 0.f;
	for (const FValdorsoTipoOggetto& Tipo : ValdorsoOggetti::Catalogo())
	{
		if (Tipo.Calore > Caldo && ValdorsoZaino::Conta(Zaino->Contenuto(), Tipo.Id) > 0)
		{
			Caldo = Tipo.Calore;
		}
	}
	return Caldo;
}

void UValdorsoBagnatoComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Corpo = GetOwner();
	UWorld* Mondo = GetWorld();
	if (!Corpo || !Mondo || !Corpo->HasAuthority())
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	if (Adesso < ProssimoControllo)
	{
		return;
	}
	ProssimoControllo = Adesso + BagnatoSecondiControllo;
	const float Trascorso = UltimoControllo < 0.0 ? static_cast<float>(BagnatoSecondiControllo)
		: FMath::Clamp(static_cast<float>(Adesso - UltimoControllo), 0.f, 2.f);
	UltimoControllo = Adesso;

	const ACharacter* Personaggio = Cast<ACharacter>(Corpo);
	const UValdorsoMovementComponent* Movimento = Personaggio ? Cast<UValdorsoMovementComponent>(Personaggio->GetCharacterMovement()) : nullptr;
	const bool bInAcqua = Movimento && (Movimento->IsSwimming() || Movimento->ImmersioneNellaValle() >= BagnatoAcquaVita);
	const AValdorsoTempo* Tempo = AValdorsoTempo::Di(Mondo);
	const float Pioggia = Tempo ? Tempo->AriaAdesso().Pioggia : 0.f;
	if (Adesso >= ProssimoTetto)
	{
		ProssimoTetto = Adesso + BagnatoSecondiTetto;
		bTetto = TettoSopra();
	}
	// il valore esatto cambia a ogni controllo; quello in rete solo quando si sposta di mezzo centesimo (o arriva in fondo)
	const bool bFuoco = !bInAcqua && AValdorsoFalo::CaldoVicino(Mondo, Corpo->GetActorLocation());
	LivelloEsatto = ValdorsoBagnato::Passo(LivelloEsatto, Trascorso, bInAcqua, Pioggia, bTetto, HaMantello(), bFuoco);
	if (FMath::Abs(LivelloEsatto - Livello) >= 0.005f || LivelloEsatto <= 0.f || LivelloEsatto >= 1.f)
	{
		Livello = LivelloEsatto;
	}

	// (10/11, v0.1.4) il fiato: con la bocca sott'acqua si trattiene, fuori si riprende; finito, si annega
	{
		const bool bApnea = Movimento && Movimento->IsSwimming() && Movimento->AcquaSopraLaTesta() >= ValdorsoBagnato::AcquaPerApnea;
		// (11/11, v0.1.5) il Nuoto: il fiato lungo (50) e l'abisso (100) fanno durare di piu' il fiato (ValdorsoVigore.h)
		FiatoEsatto = ValdorsoBagnato::PassoFiato(FiatoEsatto, bApnea ? Trascorso / ValdorsoVigore::FattoreFiato(GetOwner()) : Trascorso, bApnea);
		if (FMath::Abs(FiatoEsatto - FiatoInRete) >= 0.01f || FiatoEsatto <= 0.f || FiatoEsatto >= 1.f)
		{
			FiatoInRete = FiatoEsatto;
		}
		UValdorsoPericoliComponent* Annega = UValdorsoPericoliComponent::Di(Corpo);
		if (bApnea && FiatoEsatto <= 0.f && Annega && !Annega->Caduto())
		{
			Annega->Ferisci(ValdorsoBagnato::AnnegaAlSecondo * Trascorso);
		}
		// un avviso quando il fiato sta per finire (una volta per immersione)
		const APawn* Nuotatore = Cast<APawn>(Corpo);
		if (bApnea && FiatoEsatto < 0.3f && !bAvvisatoFiato)
		{
			bAvvisatoFiato = true;
			if (AValdorsoPlayerController* Suo = Nuotatore ? Cast<AValdorsoPlayerController>(Nuotatore->GetController()) : nullptr)
			{
				Suo->ClientAvviso(NSLOCTEXT("ValdorsoBagnato", "Fiato", "Ti manca il fiato: torna su! (Spazio)"), 3.f, 0.f);
			}
		}
		else if (!bApnea && FiatoEsatto > 0.6f)
		{
			bAvvisatoFiato = false;
		}
	}

	// (09/11) la temperatura: l'aria (stagione, ora, quota, tempo) e quella che sente lui (ValdorsoClima.h)
	if (!bCieloCercato)
	{
		bCieloCercato = true;
		for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
		{
			Cielo = *It;
			break;
		}
	}
	const float Ora = Cielo.IsValid() ? Cielo->Ora() : 12.f;
	const FString Stagione = ValdorsoStatoMondo::StagioneDi(FDateTime::Now());
	const FValdorsoAria Aria = Tempo ? Tempo->AriaAdesso() : FValdorsoAria();
	GradiAria = ValdorsoClima::Aria(Ora, Stagione, static_cast<float>(Corpo->GetActorLocation().Z), Aria.Nuvole, Aria.Pioggia, Aria.Nebbia);
	float Sentita = ValdorsoClima::Percepita(GradiAria, LivelloEsatto, bInAcqua, ValdorsoClima::Acqua(Stagione), HaMantello(), bTetto, bFuoco);
	// (09/11) i vestiti caldi (il mantello di pelle di lupo): qualche grado in piu', fuori dall'acqua
	if (!bInAcqua)
	{
		Sentita += CaloreVestiti();
	}
	// in rete solo a mezzi gradi (e solo quando cambia davvero)
	const float Arrotondata = FMath::RoundToFloat(Sentita * 2.f) / 2.f;
	if (!FMath::IsNearlyEqual(Arrotondata, Percepita))
	{
		Percepita = Arrotondata;
	}

	UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Corpo);
	if (!Abilita || !Abilita->HasAttributeSetForAttribute(UValdorsoAttributeSet::GetStaminaAttribute()))
	{
		return;
	}
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
	const bool bCaduto = Pericoli && Pericoli->Caduto();

	// il ritorno della stamina: bagnati a metà, e il caldo o il freddo lo frenano ancora (al massimo tre quarti);
	// si toglie la parte frenata di quello che è tornato dall'ultimo controllo
	const float FrenoBagnato = (!bFuoco && ValdorsoBagnato::Infreddolito(Livello)) ? ValdorsoBagnato::FrenoStamina : 0.f;
	const float Freno = ValdorsoClima::FrenoTotale(FrenoBagnato, ValdorsoClima::FrenoTemperatura(Sentita));
	float Stamina = Abilita->GetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute());
	const float Tornata = Stamina - StaminaPrima;
	if (StaminaPrima >= 0.f && Freno > 0.f && Tornata > 0.f && Tornata <= BagnatoRitornoAlSecondo * Trascorso + 0.5f && !bCaduto)
	{
		Stamina = StaminaPrima + Tornata * (1.f - Freno);
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute(), Stamina);
	}
	StaminaPrima = Stamina;

	// il gelo indebolisce: la salute scende piano, ma mai sotto un quarto (il freddo da solo non uccide)
	if (!bCaduto && ValdorsoClima::Sensazione(Sentita) == EValdorsoSensazione::Gelo
		&& Abilita->HasAttributeSetForAttribute(UValdorsoAttributeSet::GetSaluteAttribute()))
	{
		const float Massima = Abilita->GetNumericAttribute(UValdorsoAttributeSet::GetSaluteMaxAttribute());
		const float Salute = Abilita->GetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute());
		const float Pavimento = Massima * ValdorsoClima::SaluteMinimaGelo;
		if (Salute > Pavimento)
		{
			Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute(),
				FMath::Max(Pavimento, Salute - ValdorsoClima::SaluteGeloAlSecondo * Trascorso));
		}
	}
}
