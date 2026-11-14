// Valdorso - Lo stato del colono che vedono tutti (vedi ValdorsoStatoColono.h). Scritto da Claude l'11/11/2026.
// (14/11, v0.2.4) La colpa e le guardie (ValdorsoFiducia.h).

#include "ValdorsoStatoColono.h"
#include "ValdorsoFiducia.h"
#include "ValdorsoPNG.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoStatoColono"

AValdorsoPlayerState::AValdorsoPlayerState()
{
}

void AValdorsoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoPlayerState, Forza);
	DOREPLIFETIME(AValdorsoPlayerState, bForzaNota);
	DOREPLIFETIME(AValdorsoPlayerState, bColpevole);
}

void AValdorsoPlayerState::ImpostaForza(float Nuova)
{
	if (!HasAuthority())
	{
		return;
	}
	const uint8 Arrotondata = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt32(Nuova), 0, 100));
	if (Arrotondata != Forza || !bForzaNota)
	{
		Forza = Arrotondata;
		bForzaNota = true;
		ForceNetUpdate();
	}
}

bool AValdorsoPlayerState::ForzaDi(const AActor* Corpo, float& OutForza)
{
	const APawn* Colono = Cast<APawn>(Corpo);
	const AValdorsoPlayerState* Stato = Colono ? Colono->GetPlayerState<AValdorsoPlayerState>() : nullptr;
	if (!Stato || !Stato->bForzaNota)
	{
		return false;
	}
	OutForza = Stato->GetForza();
	return true;
}

// --- (14/11, v0.2.4) La colpa ----------------------------------------------------------------------------------------

void AValdorsoPlayerState::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TimerColpa, this, &AValdorsoPlayerState::GiroColpa, 1.f, true, 1.f);
	}
}

void AValdorsoPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerColpa);
	}
	Super::EndPlay(EndPlayReason);
}

bool AValdorsoPlayerState::ColpevoleDi(const AActor* Corpo)
{
	const APawn* Colono = Cast<APawn>(Corpo);
	const AValdorsoPlayerState* Stato = Colono ? Colono->GetPlayerState<AValdorsoPlayerState>() : nullptr;
	return Stato && Stato->bColpevole;
}

bool AValdorsoPlayerState::Incolpa(bool bGuardia)
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo)
	{
		return false;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	const bool bPrima = !bColpevole;
	ColpaFinoA = Adesso + ValdorsoFiducia::SecondiColpa;
	if (bPrima)
	{
		bColpevole = true;
		ForceNetUpdate();
		ArrestoAlle = Adesso + ValdorsoFiducia::SecondiPrimaDelleGuardie;
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(GetPlayerController()))
		{
			Giocatore->ClientAvviso(LOCTEXT("Colpa", "Hai alzato le mani su un abitante della valle. Il tuo nome e' rosso, e le guardie ti cercano."), 5.f, 0.f);
		}
	}
	// chi colpisce una guardia lo prendono subito (o quasi)
	if (bGuardia && !bInArresto && !bInCellaOra)
	{
		ArrestoAlle = FMath::Min(ArrestoAlle, Adesso + 1.5);
	}
	const bool bConta = bPrima || Adesso - UltimaColpa >= ValdorsoFiducia::SecondiTraColpe;
	if (bConta)
	{
		UltimaColpa = Adesso;
	}
	return bConta;
}

void AValdorsoPlayerState::Assolvi()
{
	if (!HasAuthority())
	{
		return;
	}
	bInArresto = false;
	if (bColpevole)
	{
		bColpevole = false;
		ForceNetUpdate();
	}
}

void AValdorsoPlayerState::InCella()
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo || bInCellaOra)
	{
		return;
	}
	bInCellaOra = true;
	bInArresto = false;
	CellaFinoA = Mondo->GetTimeSeconds() + ValdorsoFiducia::SecondiCella;
	if (ACharacter* Colono = Cast<ACharacter>(GetPawn()))
	{
		if (UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement())
		{
			Movimento->StopMovementImmediately();
			Movimento->DisableMovement();
		}
	}
	if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(GetPlayerController()))
	{
		Giocatore->ClientAvviso(LOCTEXT("Cella", "Una notte in cella: paglia, pane duro e una finestra sul passo. All'alba ti lasciano andare."),
			ValdorsoFiducia::SecondiCella, ValdorsoFiducia::SecondiCella);
	}
}

void AValdorsoPlayerState::ArrestoChiuso()
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo || !bInArresto)
	{
		return;
	}
	bInArresto = false;
	// e' scappato dal dialogo senza scegliere: lo riprendono fra poco
	if (bColpevole && !bInCellaOra)
	{
		ArrestoAlle = Mondo->GetTimeSeconds() + ValdorsoFiducia::SecondiDopoLaFuga;
	}
}

void AValdorsoPlayerState::GiroColpa()
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo)
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	if (bInCellaOra)
	{
		if (Adesso < CellaFinoA)
		{
			return;
		}
		bInCellaOra = false;
		if (ACharacter* Colono = Cast<ACharacter>(GetPawn()))
		{
			if (UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement())
			{
				Movimento->SetMovementMode(MOVE_Walking);
			}
		}
		Assolvi();
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(GetPlayerController()))
		{
			Giocatore->ClientAvviso(LOCTEXT("Libero", "All'alba la porta si apre. Sei libero: tieni le mani a posto."), 4.f, 0.f);
		}
		return;
	}
	if (!bColpevole)
	{
		return;
	}
	if (Adesso >= ColpaFinoA)
	{
		Assolvi();
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(GetPlayerController()))
		{
			Giocatore->ClientAvviso(LOCTEXT("Passata", "Le guardie hanno smesso di cercarti. Ma la valle si ricorda."), 4.f, 0.f);
		}
		return;
	}
	if (!bInArresto && Adesso >= ArrestoAlle)
	{
		Arresta();
	}
}

void AValdorsoPlayerState::Arresta()
{
	UWorld* Mondo = GetWorld();
	APawn* Colono = GetPawn();
	AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(GetPlayerController());
	if (!Mondo || !Colono || !Giocatore)
	{
		// (morto, o fra un corpo e l'altro: ci riprovano fra poco)
		ArrestoAlle = (Mondo ? Mondo->GetTimeSeconds() : 0.0) + 5.0;
		return;
	}
	// la guardia piu' vicina che c'e'
	AValdorsoPNG* Guardia = nullptr;
	double Migliore = TNumericLimits<double>::Max();
	for (TActorIterator<AValdorsoPNG> It(Mondo); It; ++It)
	{
		if (!It->Guardia() || It->Via() || It->ATerra())
		{
			continue;
		}
		const double Distanza = FVector::DistSquared(It->GetActorLocation(), Colono->GetActorLocation());
		if (Distanza < Migliore)
		{
			Migliore = Distanza;
			Guardia = *It;
		}
	}
	if (!Guardia)
	{
		// nessuna guardia nella valle (le prove nell'editor): la multa si paga da sola, se si puo'
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Giocatore);
		const bool bPagato = Zaino && Zaino->Cambia([](FValdorsoZaino& Z)
		{
			if (Z.Monete < ValdorsoFiducia::Multa)
			{
				return false;
			}
			Z.Monete -= ValdorsoFiducia::Multa;
			return true;
		});
		if (bPagato)
		{
			ValdorsoRegistroEventi::Annota(TEXT("Multa"), Colono, nullptr, Colono->GetActorLocation(),
				FString::Printf(TEXT("%d Aureus al Balivo (senza guardie)"), ValdorsoFiducia::Multa));
			Giocatore->ClientAvviso(FText::Format(LOCTEXT("MultaSola", "Il Balivo ti ha mandato il conto: {0} Aureus di multa."),
				FText::AsNumber(ValdorsoFiducia::Multa)), 4.f, 0.f);
			Assolvi();
		}
		else
		{
			ArrestoAlle = Mondo->GetTimeSeconds() + 60.0;
		}
		return;
	}
	// al posto di guardia, davanti a lei
	const FVector Avanti = Guardia->GetActorForwardVector().GetSafeNormal2D();
	const FVector Dove = Guardia->GetActorLocation() + (Avanti.IsNearlyZero() ? FVector(1.f, 0.f, 0.f) : Avanti) * 170.f + FVector(0.f, 0.f, 10.f);
	const FRotator Verso = (Guardia->GetActorLocation() - Dove).GetSafeNormal2D().Rotation();
	Colono->TeleportTo(Dove, FRotator(0.f, Verso.Yaw, 0.f));
	Giocatore->ClientSetRotation(FRotator(0.f, Verso.Yaw, 0.f), true);
	Giocatore->ClientAvviso(LOCTEXT("Presi", "Le guardie ti hanno preso, e ti portano al posto di guardia."), 3.f, 0.f);
	ValdorsoRegistroEventi::Annota(TEXT("Arresto"), Colono, Guardia, Dove, TEXT("portato al posto di guardia"));
	bInArresto = Guardia->ApriDialogo(Colono, FName(ValdorsoFiducia::DialogoArresto));
	if (!bInArresto)
	{
		ArrestoAlle = Mondo->GetTimeSeconds() + 5.0;
	}
}

#undef LOCTEXT_NAMESPACE
