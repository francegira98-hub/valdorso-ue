// Valdorso - Lo stato del colono che vedono tutti (vedi ValdorsoStatoColono.h). Scritto da Claude l'11/11/2026.
// (14/11, v0.2.4) La colpa e le guardie (ValdorsoFiducia.h).

#include "ValdorsoStatoColono.h"
#include "ValdorsoMonete.h"
#include "ValdorsoFiducia.h"
#include "ValdorsoMilizia.h"
#include "ValdorsoMura.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPNG.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoStatoColono"

namespace
{
	// (16/11, v0.2.6) quanto dura la cella, in secondi veri (scorre anche a gioco chiuso). Per le prove: 20.
	TAutoConsoleVariable<float> CVarCellaSecondi(TEXT("Valdorso.Cella.Secondi"), ValdorsoFiducia::SecondiCella,
		TEXT("Quanti secondi veri dura la cella (la pena scorre anche a gioco chiuso)."));

	/** Mette il colono in piedi su un punto del pavimento. */
	void CellaPorta(ACharacter* Colono, const FVector& Pavimento, float Gradi)
	{
		if (!Colono)
		{
			return;
		}
		const UCapsuleComponent* Capsula = Colono->GetCapsuleComponent();
		const float Mezza = Capsula ? Capsula->GetScaledCapsuleHalfHeight() : 90.f;
		Colono->TeleportTo(Pavimento + FVector(0.f, 0.f, Mezza + 5.f), FRotator(0.f, Gradi, 0.f), false, true);
		if (UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement())
		{
			Movimento->StopMovementImmediately();
		}
	}
}

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
			// (17/11, v0.2.7) chi porta la divisa risponde di piu': fuori dalla milizia, e le chiavi tornano a Livia
			ValdorsoMilizia::Espelli(Giocatore);
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
	const float Pena = FMath::Clamp(CVarCellaSecondi.GetValueOnGameThread(), 5.f, 86400.f);
	bInCellaOra = true;
	bInArresto = false;
	CellaFinoA = Mondo->GetTimeSeconds() + Pena;
	// (16/11, v0.2.6) la fine della pena nel file del personaggio: vale anche se esce, se muore, a gioco chiuso
	SalvaPena(FDateTime::UtcNow().ToUnixTimestamp() + FMath::CeilToInt64(Pena));
	CellaMia = AValdorsoCella::PiuLibera(Mondo);
	TieniInCella();
	if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(GetPlayerController()))
	{
		if (CellaMia.IsValid())
		{
			// in una cella vera ci si muove: l'avviso non lo tiene fermo
			Giocatore->ClientAvviso(LOCTEXT("CellaVera", "Ti chiudono in cella: un tavolaccio, pane duro e le sbarre. La pena passa anche se lasci la valle; quando e' finita ti accompagnano fuori."),
				8.f, 0.f);
		}
		else
		{
			Giocatore->ClientAvviso(LOCTEXT("Cella", "Una notte in cella: paglia, pane duro e una finestra sul passo. All'alba ti lasciano andare."),
				Pena, Pena);
		}
	}
}

void AValdorsoPlayerState::SalvaPena(int64 FinoUtc)
{
	if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(GetPlayerController()))
	{
		Zaino->Cambia([FinoUtc](FValdorsoZaino& Z)
		{
			if (Z.CellaFinoUtc == FinoUtc)
			{
				return false;
			}
			Z.CellaFinoUtc = FinoUtc;
			return true;
		});
	}
}

void AValdorsoPlayerState::LeggiPenaSalvata(double Adesso)
{
	// solo quando c'e' un personaggio con il suo file (nelle prove dell'editor senza file conta CellaFinoA e basta)
	const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(GetPlayerController());
	if (!Zaino || !Zaino->HaPersonaggio())
	{
		return;
	}
	const int64 Fine = Zaino->Contenuto().CellaFinoUtc;
	const int64 Ora = FDateTime::UtcNow().ToUnixTimestamp();
	if (Fine > Ora)
	{
		if (!bInCellaOra)
		{
			// e' rientrato (o ha cambiato personaggio) con la pena ancora da finire
			bInCellaOra = true;
			bInArresto = false;
			CellaMia = nullptr;
			if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(GetPlayerController()))
			{
				Giocatore->ClientAvviso(LOCTEXT("AncoraCella", "Sei ancora in cella: la pena non e' finita."), 5.f, 0.f);
			}
		}
		CellaFinoA = Adesso + static_cast<double>(Fine - Ora);
	}
	else if (!bInCellaOra)
	{
		if (Fine != 0)
		{
			// la pena e' finita mentre era via: e' ancora chiuso nella cella dove ha lasciato il gioco. Lo si fa uscire
			// come gli altri (EsciDallaCella lo porta fuori e azzera il file)
			bInCellaOra = true;
			CellaMia = nullptr;
			CellaFinoA = Adesso;
		}
	}
	else
	{
		// la pena e' finita adesso; oppure (Fine a zero) e' entrato un altro personaggio, che non c'entra
		if (Fine == 0)
		{
			bInCellaOra = false;
			CellaMia = nullptr;
			RidaiMovimento();
			// (secondo controllo) e non eredita nemmeno la colpa dell'altro
			Assolvi();
		}
		else
		{
			CellaFinoA = Adesso;
		}
	}
}

void AValdorsoPlayerState::TieniInCella()
{
	UWorld* Mondo = GetWorld();
	ACharacter* Colono = Cast<ACharacter>(GetPawn());
	UCharacterMovementComponent* Movimento = Colono ? Colono->GetCharacterMovement() : nullptr;
	if (!Mondo || !Colono || !Movimento)
	{
		return;
	}
	// chi e' a terra (morto, in attesa di rinascere al tempio) si lascia stare: lo si riporta in cella dopo
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Colono);
	if (Pericoli && Pericoli->Caduto())
	{
		return;
	}
	if (!CellaMia.IsValid() || !CellaMia->Usabile())
	{
		// (secondo controllo) chi rientra resta nella cella dov'e' gia'; altrimenti la piu' libera
		CellaMia = nullptr;
		for (TActorIterator<AValdorsoCella> It(Mondo); It; ++It)
		{
			if (It->Usabile() && It->Dentro(Colono->GetActorLocation(), 40.f))
			{
				CellaMia = *It;
				break;
			}
		}
		if (!CellaMia.IsValid())
		{
			CellaMia = AValdorsoCella::PiuLibera(Mondo);
		}
	}
	AValdorsoCella* Cella = CellaMia.Get();
	if (!Cella)
	{
		// niente prigione (fasi 1 e 2): fermo dov'e', come prima
		if (Movimento->MovementMode != MOVE_None)
		{
			Movimento->StopMovementImmediately();
			Movimento->DisableMovement();
			bFermoPerCella = true;
		}
		return;
	}
	RidaiMovimento();
	if (!Cella->Dentro(Colono->GetActorLocation(), 40.f))
	{
		// e' fuori (appena arrestato, rinato al tempio, rientrato nel gioco, o la porta era aperta): dentro
		CellaPorta(Colono, Cella->GetActorLocation(), Cella->GetActorRotation().Yaw);
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(GetPlayerController()))
		{
			Giocatore->ClientSetRotation(FRotator(0.f, Cella->GetActorRotation().Yaw, 0.f), true);
		}
	}
}

void AValdorsoPlayerState::EsciDallaCella(bool bAvvisa)
{
	if (!bInCellaOra)
	{
		return;
	}
	bInCellaOra = false;
	SalvaPena(0);
	RidaiMovimento();
	if (ACharacter* Colono = Cast<ACharacter>(GetPawn()))
	{
		// fuori, davanti agli alloggi (la porta della cella e quella della prigione restano chiuse a chiave)
		const AValdorsoCella* Cella = CellaMia.Get();
		if (!Cella || !Cella->Dentro(Colono->GetActorLocation(), 60.f))
		{
			// (chi rientra a pena finita non ha ancora la "sua" cella: quella in cui si trova)
			Cella = nullptr;
			for (TActorIterator<AValdorsoCella> It(GetWorld()); It; ++It)
			{
				if (It->Dentro(Colono->GetActorLocation(), 60.f))
				{
					Cella = *It;
					break;
				}
			}
		}
		if (Cella && !Cella->Uscita.IsZero())
		{
			CellaPorta(Colono, Cella->Uscita, Cella->GetActorRotation().Yaw + 90.f);
		}
	}
	CellaMia = nullptr;
	Assolvi();
	if (bAvvisa)
	{
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(GetPlayerController()))
		{
			Giocatore->ClientAvviso(LOCTEXT("Libero", "La pena e' finita: ti accompagnano fuori. Sei libero: tieni le mani a posto."), 4.f, 0.1f);
			// (secondo controllo: 0.1 e non 0, cosi' il PC smette di tenerlo fermo se la cella senza prigione e' finita prima)
		}
	}
}

void AValdorsoPlayerState::RidaiMovimento()
{
	if (!bFermoPerCella)
	{
		return;
	}
	bFermoPerCella = false;
	if (const ACharacter* Colono = Cast<ACharacter>(GetPawn()))
	{
		if (UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement())
		{
			if (Movimento->MovementMode == MOVE_None)
			{
				Movimento->SetMovementMode(MOVE_Walking);
			}
		}
	}
}

void AValdorsoPlayerState::LiberaDallaCella()
{
	if (HasAuthority())
	{
		EsciDallaCella(true);
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
	// (16/11, v0.2.6) la pena scritta nel file del personaggio comanda (chi rientra e' ancora in cella)
	LeggiPenaSalvata(Adesso);
	if (bInCellaOra)
	{
		if (Adesso < CellaFinoA)
		{
			TieniInCella();
			return;
		}
		// (senza corpo, appena entrato o fra un corpo e l'altro, si aspetta: va portato fuori dalla cella)
		// (secondo controllo) e chi e' a terra esce dopo essere rinato
		const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(GetPawn());
		if (GetPawn() && !(Pericoli && Pericoli->Caduto()))
		{
			EsciDallaCella(true);
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
		// (14/11) una guardia che dorme sulla branda non arresta nessuno
		if (!It->Guardia() || It->Via() || It->ATerra() || It->Dorme())
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
				FString::Printf(TEXT("%s al Balivo (senza guardie)"), *ValdorsoMonete::Testo(ValdorsoFiducia::Multa)));
			Giocatore->ClientAvviso(FText::Format(LOCTEXT("MultaSolaMonete", "Il Balivo ti ha mandato il conto: {0} di multa."),
				ValdorsoMonete::Scritto(ValdorsoFiducia::Multa)), 4.f, 0.f);
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
