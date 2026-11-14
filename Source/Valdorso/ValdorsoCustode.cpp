// Valdorso - Il custode del Battito (vedi il .h). Scritto da Claude il 14/11/2026.
// Unity build: le cose del namespace anonimo cominciano con Custode.

#include "ValdorsoCustode.h"
#include "ValdorsoChat.h"
#include "ValdorsoQualita.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoStaff.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Valdorso.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
	FString CustodeNome(const APlayerController* Chi)
	{
		const APlayerState* Stato = Chi ? Chi->PlayerState.Get() : nullptr;
		return Stato ? Stato->GetPlayerName() : FString(TEXT("?"));
	}

	/** Quanti oggetti Rari (o piu') ha nello zaino e nel baule. */
	int32 CustodeRari(const FValdorsoZaino& Zaino)
	{
		int32 Quanti = 0;
		for (const FValdorsoPostoZaino& Posto : Zaino.Posti)
		{
			Quanti += (!Posto.Vuoto() && Posto.Qualita >= ValdorsoQualita::Rara) ? FMath::Max(1, Posto.Quantita) : 0;
		}
		for (const FValdorsoPostoZaino& Posto : Zaino.Baule)
		{
			Quanti += (!Posto.Vuoto() && Posto.Qualita >= ValdorsoQualita::Rara) ? FMath::Max(1, Posto.Quantita) : 0;
		}
		return Quanti;
	}
}

namespace ValdorsoCustode
{
	int32 Conta(TArray<double>& Momenti, double Adesso, double Finestra)
	{
		Momenti.RemoveAll([Adesso, Finestra](double M) { return Adesso - M > Finestra; });
		return Momenti.Num();
	}

	int32 Salite(TArray<TPair<double, int32>>& InSalite, double Adesso, double Finestra)
	{
		InSalite.RemoveAll([Adesso, Finestra](const TPair<double, int32>& S) { return Adesso - S.Key > Finestra; });
		int32 Somma = 0;
		for (const TPair<double, int32>& S : InSalite)
		{
			Somma += FMath::Max(0, S.Value);
		}
		return Somma;
	}

	bool TroppoVeloce(double CentimetriInPiano, double Secondi)
	{
		return Secondi > 0.05 && CentimetriInPiano / Secondi > VelocitaMassima;
	}

	bool SpostaDiColpo(const FString& Tipo)
	{
		static const TCHAR* Quali[] = { TEXT("Rinascita"), TEXT("Morte"), TEXT("Arresto"), TEXT("Passaggio"), TEXT("Staff"),
			TEXT("Dorme"), TEXT("SiSveglia"), TEXT("Siede"), TEXT("Entra"), TEXT("Esce"), TEXT("PortaSegreta"), TEXT("Trappola") };
		for (const TCHAR* Q : Quali)
		{
			if (Tipo.Equals(Q, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}
}

bool UValdorsoCustode::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* Mondo = Cast<UWorld>(Outer);
	return Mondo && Mondo->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
}

void UValdorsoCustode::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() != NM_Client)
	{
		InWorld.GetTimerManager().SetTimer(TimerGiro, FTimerDelegate::CreateUObject(this, &UValdorsoCustode::Giro), 1.f, true, 3.f);
		ProssimaEconomia = InWorld.GetTimeSeconds() + 3600.0;
	}
}

void UValdorsoCustode::Deinitialize()
{
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerGiro);
	}
	Guardati.Reset();
	Super::Deinitialize();
}

UValdorsoCustode::FGuardato& UValdorsoCustode::GuardatoDi(const APlayerController* Chi)
{
	return Guardati.FindOrAdd(TWeakObjectPtr<const APlayerController>(Chi));
}

void UValdorsoCustode::DopoAnnota(const FString& Tipo, const AActor* Chi, const AActor* Bersaglio)
{
	if (Tipo == TEXT("Custode"))
	{
		return;
	}
	const UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
	UValdorsoCustode* Custode = Mondo ? Mondo->GetSubsystem<UValdorsoCustode>() : nullptr;
	if (!Custode)
	{
		return;
	}
	if (const APawn* Colono = Cast<APawn>(Chi))
	{
		Custode->Evento(Cast<APlayerController>(Colono->GetController()), Tipo, false);
	}
	if (const APawn* Colpito = Cast<APawn>(Bersaglio))
	{
		Custode->Evento(Cast<APlayerController>(Colpito->GetController()), Tipo, true);
	}
}

void UValdorsoCustode::Evento(const APlayerController* Chi, const FString& Tipo, bool bComeBersaglio)
{
	UWorld* Mondo = GetWorld();
	if (!Chi || !Mondo || !Chi->HasAuthority())
	{
		return;
	}
	FGuardato& Suo = GuardatoDi(Chi);
	const double Adesso = Mondo->GetTimeSeconds();
	if (ValdorsoCustode::SpostaDiColpo(Tipo))
	{
		Suo.TreguaFinoA = Adesso + ValdorsoCustode::SecondiTregua;
		Suo.UltimoMomento = -1.0;
	}
	if (bComeBersaglio)
	{
		return;
	}
	if (Tipo == TEXT("Colpo"))
	{
		Suo.Colpi.Add(Adesso);
		if (ValdorsoCustode::Conta(Suo.Colpi, Adesso, ValdorsoCustode::SecondiColpi) > ValdorsoCustode::SogliaColpi)
		{
			Avvisa(Chi, Suo, TEXT("colpi"), FString::Printf(TEXT("piu' di %d colpi in %.0f secondi: forse un trucco per colpire piu' in fretta"),
				ValdorsoCustode::SogliaColpi, ValdorsoCustode::SecondiColpi));
		}
	}
	else if (Tipo == TEXT("Colpa"))
	{
		Suo.Colpe.Add(Adesso);
		const int32 Quante = ValdorsoCustode::Conta(Suo.Colpe, Adesso, ValdorsoCustode::SecondiColpe);
		if (Quante >= ValdorsoCustode::SogliaColpe)
		{
			Avvisa(Chi, Suo, TEXT("colpe"), FString::Printf(TEXT("ha colpito i PNG %d volte in mezz'ora"), Quante));
		}
	}
}

void UValdorsoCustode::Avvisa(const APlayerController* Chi, FGuardato& Suo, const FString& Tipo, const FString& Testo)
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	const double* Ultimo = Suo.UltimoAvviso.Find(Tipo);
	if (Ultimo && Adesso - *Ultimo < ValdorsoCustode::SecondiTraAvvisi)
	{
		return;
	}
	Suo.UltimoAvviso.Add(Tipo, Adesso);
	++Avvisi;
	const FString Nome = CustodeNome(Chi);
	const APawn* Colono = Chi ? Chi->GetPawn() : nullptr;
	const AActor* Chi2 = Colono ? static_cast<const AActor*>(Colono) : static_cast<const AActor*>(Mondo->GetGameState());
	if (Chi2)
	{
		ValdorsoRegistroEventi::Annota(TEXT("Custode"), Chi2, nullptr, Chi2->GetActorLocation(), FString::Printf(TEXT("%s: %s"), *Nome, *Testo));
	}
	UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Custode del Battito: %s: %s"), *Nome, *Testo);
	DilloAlloStaff(FString::Printf(TEXT("%s: %s. Guarda in F9, Registro, filtro \"Custode\". (Non ho fatto niente: decidi tu.)"), *Nome, *Testo));
}

void UValdorsoCustode::DilloAlloStaff(const FString& Testo)
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Giocatore = It->Get();
		if (Giocatore && UValdorsoStaffComponent::PermessiDi(Giocatore) != 0)
		{
			if (UValdorsoChatComponent* Chat = UValdorsoChatComponent::Di(Giocatore))
			{
				Chat->ClientSenti(TEXT("Il custode del Battito"), Testo.Left(400), false);
			}
		}
	}
}

void UValdorsoCustode::Giro()
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	for (auto It = Guardati.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	int64 MoneteValle = 0;
	int32 Coloni = 0;
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Chi = It->Get();
		const APawn* Colono = Chi ? Chi->GetPawn() : nullptr;
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		if (!Colono || !Zaino)
		{
			continue;
		}
		const FValdorsoZaino Dati = Zaino->Dati();
		MoneteValle += Dati.Monete;
		++Coloni;
		// lo staff si sposta e si da' oggetti da F9: non si guarda
		if (UValdorsoStaffComponent::PermessiDi(Chi) != 0)
		{
			continue;
		}
		FGuardato& Suo = GuardatoDi(Chi);
		// un corpo nuovo (si entra, si rinasce): si ricomincia a contare da qui
		if (Suo.Corpo.Get() != Colono)
		{
			Suo.Corpo = Colono;
			Suo.UltimoMomento = -1.0;
			Suo.UltimeMonete = -1;
			Suo.UltimiRari = -1;
			Suo.TreguaFinoA = Adesso + ValdorsoCustode::SecondiTregua;
		}
		// le monete e gli oggetti di valore
		if (Suo.UltimeMonete >= 0 && Dati.Monete > Suo.UltimeMonete)
		{
			Suo.MoneteSalite.Add(TPair<double, int32>(Adesso, Dati.Monete - Suo.UltimeMonete));
		}
		Suo.UltimeMonete = Dati.Monete;
		const int32 Guadagno = ValdorsoCustode::Salite(Suo.MoneteSalite, Adesso, 3600.0);
		if (Guadagno > ValdorsoCustode::SogliaMonete)
		{
			Avvisa(Chi, Suo, TEXT("monete"), FString::Printf(TEXT("ha guadagnato %d Aureus nell'ultima ora (ne ha %d)"), Guadagno, Dati.Monete));
		}
		const int32 Rari = CustodeRari(Dati);
		if (Suo.UltimiRari >= 0 && Rari > Suo.UltimiRari)
		{
			Suo.RariSaliti.Add(TPair<double, int32>(Adesso, Rari - Suo.UltimiRari));
		}
		Suo.UltimiRari = Rari;
		const int32 NuoviRari = ValdorsoCustode::Salite(Suo.RariSaliti, Adesso, 3600.0);
		if (NuoviRari >= ValdorsoCustode::SogliaRari)
		{
			Avvisa(Chi, Suo, TEXT("rari"), FString::Printf(TEXT("ha ricevuto %d oggetti Rari (o piu') nell'ultima ora"), NuoviRari));
		}
		// i passi
		const FVector Qui = Colono->GetActorLocation();
		if (Suo.UltimoMomento >= 0.0 && Adesso >= Suo.TreguaFinoA
			&& ValdorsoCustode::TroppoVeloce(FVector::Dist2D(Qui, Suo.UltimoPunto), Adesso - Suo.UltimoMomento))
		{
			Suo.Veloci.Add(Adesso);
			const int32 Volte = ValdorsoCustode::Conta(Suo.Veloci, Adesso, ValdorsoCustode::SecondiPassi);
			if (Volte >= ValdorsoCustode::VolteVeloce)
			{
				Avvisa(Chi, Suo, TEXT("passi"), FString::Printf(TEXT("si e' spostato troppo in fretta %d volte in cinque minuti (l'ultima: %.0f m in un secondo)"),
					Volte, FVector::Dist2D(Qui, Suo.UltimoPunto) / 100.0));
			}
		}
		Suo.UltimoPunto = Qui;
		Suo.UltimoMomento = Adesso;
	}
	// ogni ora: l'economia della valle
	if (Adesso >= ProssimaEconomia)
	{
		ProssimaEconomia = Adesso + 3600.0;
		if (const AActor* StatoGioco = Mondo->GetGameState(); StatoGioco && Coloni > 0)
		{
			ValdorsoRegistroEventi::Annota(TEXT("Custode"), StatoGioco, nullptr, FVector::ZeroVector,
				FString::Printf(TEXT("economia: %lld Aureus tra %d coloni collegati (in media %lld)"), MoneteValle, Coloni, MoneteValle / Coloni));
		}
	}
}

FString UValdorsoCustode::Stato() const
{
	return FString::Printf(TEXT("Custode del Battito: guarda %d coloni, %d avvisi dall'avvio. Soglie: %d Aureus/ora, %d Rari/ora, %.0f m/s, %d colpi in %.0f s, %d colpe in mezz'ora."),
		Guardati.Num(), Avvisi, ValdorsoCustode::SogliaMonete, ValdorsoCustode::SogliaRari, ValdorsoCustode::VelocitaMassima / 100.f,
		ValdorsoCustode::SogliaColpi, ValdorsoCustode::SecondiColpi, ValdorsoCustode::SogliaColpe);
}

namespace
{
	void CustodeComando(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		const UValdorsoCustode* Custode = Mondo ? Mondo->GetSubsystem<UValdorsoCustode>() : nullptr;
		Uscita.Log(Custode ? Custode->Stato() : FString(TEXT("Il custode c'e' solo nel gioco (Play).")));
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice CustodeComandoStato(TEXT("Valdorso.Custode"),
		TEXT("Com'e' la valle per il custode del Battito (quanti guarda, quanti avvisi, le soglie)."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&CustodeComando));
}
