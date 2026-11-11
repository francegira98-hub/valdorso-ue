// Valdorso - Le storie della valle (vedi ValdorsoStorie.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoStorie.h"
#include "Valdorso.h"
#include "ValdorsoCielo.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoScorte.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoStorie"

namespace
{
	/** Ogni quanto si controlla chi veglia (s). */
	constexpr float StorieGiroVeglia = 2.f;

	/** Solo sul server: chi sta vegliando (per non farne due insieme). */
	TSet<TWeakObjectPtr<APlayerController>>& StorieVegliano()
	{
		static TSet<TWeakObjectPtr<APlayerController>> Tutti;
		return Tutti;
	}

	float StorieOra(UWorld* Mondo)
	{
		TActorIterator<AValdorsoCielo> It(Mondo);
		return It ? It->Ora() : 12.f;
	}

	void StorieAvviso(APlayerController* Chi, const FText& Testo, float Secondi)
	{
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
		{
			Giocatore->ClientAvviso(Testo, Secondi, 0.f);
		}
	}

	void StorieAnnota(APlayerController* Chi, AActor* Png, const FString& Come)
	{
		if (APawn* Corpo = Chi ? Chi->GetPawn() : nullptr)
		{
			ValdorsoRegistroEventi::Annota(TEXT("Storia"), Corpo, Png, Corpo->GetActorLocation(), Come);
		}
	}

	/** La veglia: parte qui, e un timer la segue finche' finisce o il colono se ne va. */
	void StorieInizia(APlayerController* Chi, AActor* Sacerdote)
	{
		UWorld* Mondo = Chi->GetWorld();
		StorieVegliano().Add(Chi);
		StorieAnnota(Chi, Sacerdote, TEXT("la veglia del frammento comincia"));
		const TWeakObjectPtr<APlayerController> Lui(Chi);
		const TWeakObjectPtr<AActor> Prete(Sacerdote);
		const double Inizio = Mondo->GetTimeSeconds();
		TSharedRef<int32> Fase = MakeShared<int32>(0);
		TSharedRef<FTimerHandle> Maniglia = MakeShared<FTimerHandle>();
		const TWeakObjectPtr<UWorld> MondoDebole(Mondo);
		Mondo->GetTimerManager().SetTimer(*Maniglia, FTimerDelegate::CreateLambda([Lui, Prete, Inizio, Fase, Maniglia, MondoDebole]()
		{
			APlayerController* Colono = Lui.Get();
			const AActor* Altare = Prete.Get();
			UWorld* Qui = MondoDebole.Get();
			const APawn* Corpo = Colono ? Colono->GetPawn() : nullptr;
			// (si ferma da solo anche se il colono e' uscito dal gioco; si pulisce una copia della maniglia: pulendo il
			// timer sparisce questa funzione, e con lei la maniglia vera)
			auto Smetti = [&]()
			{
				StorieVegliano().Remove(Lui);
				if (Qui)
				{
					FTimerHandle Copia = *Maniglia;
					Qui->GetTimerManager().ClearTimer(Copia);
				}
			};
			if (!Qui || !Colono || !Corpo || !Altare)
			{
				Smetti();
				return;
			}
			const FVector Da = Corpo->GetActorLocation();
			const FVector A = Altare->GetActorLocation();
			if (FVector::DistSquared2D(Da, A) > FMath::Square(ValdorsoStorie::RaggioVeglia) || FMath::Abs(Da.Z - A.Z) > 400.f)
			{
				StorieAvviso(Colono, LOCTEXT("VegliaLasciata", "Ti sei allontanato dal frammento: la veglia e' finita prima dell'alba. Padre Anselmo non dice niente."), 6.f);
				StorieAnnota(Colono, nullptr, TEXT("la veglia lasciata a meta'"));
				Smetti();
				return;
			}
			const double Passati = Qui->GetTimeSeconds() - Inizio;
			const float Durata = ValdorsoStorie::SecondiVeglia;
			if (*Fase == 0 && Passati > Durata * 0.25)
			{
				*Fase = 1;
				StorieAvviso(Colono, LOCTEXT("Veglia1", "Le candele si accorciano. Nel silenzio, dalla pietra: due colpi, e una pausa. Due colpi, e una pausa."), 8.f);
			}
			else if (*Fase == 1 && Passati > Durata * 0.6)
			{
				*Fase = 2;
				StorieAvviso(Colono, LOCTEXT("Veglia2", "Per un respiro il battito si ferma. Padre Anselmo trattiene il fiato. Poi riprende, piu' forte, come se qualcuno avesse bussato da dentro."), 9.f);
			}
			else if (Passati >= Durata)
			{
				UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Colono);
				const bool bPrima = Taccuino && !Taccuino->HaSegno(TEXT("PadreAnselmo"), TEXT("Vegliato"));
				if (Taccuino)
				{
					Taccuino->MettiSegno(TEXT("PadreAnselmo"), TEXT("Vegliato"));
				}
				if (UValdorsoZainoComponent* Zaino = bPrima ? UValdorsoZainoComponent::Di(Colono) : nullptr)
				{
					Zaino->Dai(TEXT("Candela_della_Veglia"), 1);
				}
				StorieAvviso(Colono, LOCTEXT("VegliaFine", "La candela si spegne da sola. Padre Anselmo ti posa una mano sulla spalla: \"Il Cuore ti ha sentito. Adesso vai a dormire.\""), 9.f);
				StorieAnnota(Colono, nullptr, TEXT("la veglia del frammento fino in fondo"));
				Smetti();
			}
		}), StorieGiroVeglia, true);
	}
}

namespace ValdorsoStorie
{
	bool OraDiVeglia(float Ora)
	{
		return Ora >= 21.f || Ora < 5.f;
	}

	bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Png, FName& InOutProssimo)
	{
		if (!Chi || Evento.IsNone())
		{
			return false;
		}
		if (Evento == FName(EventoSasso))
		{
			const UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
			if (!Taccuino || !Taccuino->TitoliIscrizioni().Contains(FString(TitoloSasso)))
			{
				InOutProssimo = TEXT("non_ancora");
				return true;
			}
			StorieAnnota(Chi, Png, TEXT("il sogno di Tobia: il sasso trovato"));
			return true;
		}
		if (Evento == FName(EventoIncudine))
		{
			UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
			const FName Cuore(TEXT("Cuore_di_Pietra"));
			if (!Zaino || ValdorsoZaino::Conta(Zaino->Contenuto(), Cuore) < 1)
			{
				InOutProssimo = TEXT("senza_cuore");
				return true;
			}
			Zaino->Cambia([&Cuore](FValdorsoZaino& Dentro) { return ValdorsoZaino::TogliOggetto(Dentro, Cuore, 1); });
			StorieAnnota(Chi, Png, TEXT("l'incudine che canta: un cuore di pietra a Marta"));
			return true;
		}
		if (Evento == FName(EventoAcquaEdda))
		{
			UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
			const FName Torbida(TEXT("Secchio_Acqua_Torbida"));
			if (!Zaino || ValdorsoZaino::Conta(Zaino->Contenuto(), Torbida) < 1)
			{
				InOutProssimo = TEXT("acqua_niente");
				return true;
			}
			Zaino->Cambia([&Torbida](FValdorsoZaino& Dentro)
			{
				ValdorsoZaino::TogliOggetto(Dentro, Torbida, 1);
				ValdorsoZaino::Aggiungi(Dentro, TEXT("Secchio"), 1);
				ValdorsoZaino::Aggiungi(Dentro, TEXT("Erbe_Palude"), 3);
				return true;
			});
			StorieAnnota(Chi, Png, TEXT("un secchio d'acqua torbida a Nonna Edda"));
			return true;
		}
		if (Evento == FName(EventoGrano))
		{
			FValdorsoScorteValle& Scorte = ValdorsoScorte::Adesso(Chi);
			if (Scorte.Grano >= ValdorsoScorte::SogliaFame)
			{
				InOutProssimo = TEXT("grano_ok");
				return true;
			}
			if (Scorte.bGranoNascostoUsato)
			{
				InOutProssimo = TEXT("grano_gia");
				return true;
			}
			Scorte.Grano = FMath::Min(ValdorsoScorte::Massimo, Scorte.Grano + GranoNascosto);
			Scorte.bGranoNascostoUsato = true;
			ValdorsoScorte::Salva(Chi);
			StorieAnnota(Chi, Png, TEXT("il grano nascosto dei mugnai, nella fame: il granaio respira"));
			return true;
		}
		if (Evento == FName(EventoVeglia))
		{
			UWorld* Mondo = Chi->GetWorld();
			if (!Mondo || !Png || !OraDiVeglia(StorieOra(Mondo)))
			{
				InOutProssimo = TEXT("di_giorno");
				return true;
			}
			if (!StorieVegliano().Contains(Chi))
			{
				StorieInizia(Chi, Png);
			}
			return true;
		}
		return false;
	}
}

#undef LOCTEXT_NAMESPACE
