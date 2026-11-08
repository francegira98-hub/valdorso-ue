// Valdorso - Bagnati e asciutti (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoBagnato.h"
#include "Valdorso.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoFalo.h"
#include "ValdorsoMovementComponent.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoTempo.h"
#include "ValdorsoZaino.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
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
	return Zaino && ValdorsoZaino::Conta(Zaino->Contenuto(), TEXT("Mantello")) > 0;
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

	// da bagnati la stamina torna a metà: si toglie metà di quello che è tornato nell'ultimo mezzo secondo
	UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Corpo);
	if (!Abilita || !Abilita->HasAttributeSetForAttribute(UValdorsoAttributeSet::GetStaminaAttribute()))
	{
		return;
	}
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
	float Stamina = Abilita->GetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute());
	const float Tornata = Stamina - StaminaPrima;
	if (StaminaPrima >= 0.f && !bFuoco && ValdorsoBagnato::Infreddolito(Livello) && Tornata > 0.f && Tornata <= BagnatoRitornoAlSecondo * Trascorso + 0.5f
		&& !(Pericoli && Pericoli->Caduto()))
	{
		Stamina = StaminaPrima + Tornata * (1.f - ValdorsoBagnato::FrenoStamina);
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute(), Stamina);
	}
	StaminaPrima = Stamina;
}
