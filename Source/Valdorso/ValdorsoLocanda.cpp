// Valdorso - Le camere della Locanda di Bruno. Vedi ValdorsoLocanda.h. Scritto da Claude il 20/11/2026.

#include "ValdorsoLocanda.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Valdorso.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"
#include "Misc/DelayedAutoRegister.h"

#define LOCTEXT_NAMESPACE "ValdorsoLocanda"

namespace
{
	void LocandaAvvisa(APlayerController* Chi, const FText& Testo)
	{
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
		{
			Giocatore->ClientAvviso(Testo, 6.f, 0.f);
		}
	}

	int64 LocandaAdesso()
	{
		return FDateTime::UtcNow().ToUnixTimestamp();
	}

	/** Toglie dallo zaino tutte le chiavi di camera tranne quella di Tieni (0 = tutte). Vero se ha tolto qualcosa. */
	bool LocandaTogliChiavi(FValdorsoZaino& Zaino, int32 Tieni)
	{
		bool bTolto = false;
		for (int32 N = 1; N <= ValdorsoLocanda::Camere; ++N)
		{
			if (N == Tieni)
			{
				continue;
			}
			const FName Chiave = ValdorsoLocanda::ChiaveDellaCamera(N);
			const int32 Quante = ValdorsoZaino::Conta(Zaino, Chiave);
			if (Quante > 0 && ValdorsoZaino::TogliOggetto(Zaino, Chiave, Quante))
			{
				bTolto = true;
			}
		}
		return bTolto;
	}

	/** Vero se quel giocatore e' accanto alla porta chiusa da quella chiave (allora non gliela si toglie: resterebbe dentro). */
	bool LocandaAccantoAllaPorta(const APlayerController* Chi, FName Chiave)
	{
		const APawn* Corpo = Chi ? Chi->GetPawn() : nullptr;
		UWorld* Mondo = Corpo ? Corpo->GetWorld() : nullptr;
		if (!Mondo)
		{
			return false;
		}
		for (TActorIterator<AValdorsoPorta> It(Mondo); It; ++It)
		{
			const FVector Porta = It->GetActorLocation();
			const FVector Lui = Corpo->GetActorLocation();
			if (It->Chiave == Chiave && FVector::Dist2D(Porta, Lui) < ValdorsoLocanda::RaggioPorta
				&& FMath::Abs(Porta.Z - Lui.Z) < ValdorsoLocanda::AltezzaPorta + 100.f)
			{
				return true;
			}
		}
		return false;
	}

	/** Ogni 5 secondi, sul server: a chi e' scaduta la notte Bruno riprende la chiave; e le chiavi di camera "orfane"
	 *  (di una camera che non e' la sua) spariscono. */
	bool LocandaGiro(float)
	{
		if (!GEngine)
		{
			return true;
		}
		const int64 Adesso = LocandaAdesso();
		for (const FWorldContext& Contesto : GEngine->GetWorldContexts())
		{
			UWorld* Mondo = Contesto.World();
			if (!Mondo || !Mondo->IsGameWorld() || Mondo->GetNetMode() == NM_Client)
			{
				continue;
			}
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* Chi = It->Get();
				UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
				if (!Zaino)
				{
					continue;
				}
				const FValdorsoZaino& Dentro = Zaino->Contenuto();
				const int32 Camera = Dentro.CameraLocanda;
				const bool bScaduta = Camera > 0 && Adesso >= Dentro.CameraFinoUtc;
				if (bScaduta && (!Chi->GetPawn() || LocandaAccantoAllaPorta(Chi, ValdorsoLocanda::ChiaveDellaCamera(Camera))))
				{
					continue;   // e' in camera (o sulla soglia): Bruno aspetta che esca
				}
				bool bAvvisa = false;
				Zaino->Cambia([Camera, bScaduta, &bAvvisa](FValdorsoZaino& Z)
				{
					bool bCambiato = LocandaTogliChiavi(Z, bScaduta ? 0 : Camera);
					if (bScaduta)
					{
						Z.CameraLocanda = 0;
						Z.CameraFinoUtc = 0;
						bCambiato = true;
						bAvvisa = true;
					}
					return bCambiato;
				});
				if (bAvvisa)
				{
					LocandaAvvisa(Chi, FText::Format(LOCTEXT("Scaduta", "La notte alla locanda è finita: Bruno si è ripreso la chiave della camera {0}."), FText::AsNumber(Camera)));
					ValdorsoRegistroEventi::Annota(TEXT("Locanda"), Chi ? Chi->GetPawn() : nullptr, nullptr, FVector::ZeroVector, FString::Printf(TEXT("camera %d: notte finita"), Camera));
				}
			}
		}
		return true;
	}

	FDelayedAutoRegisterHelper LocandaAvvio(EDelayedRegisterRunPhase::EndOfEngineInit, []()
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&LocandaGiro), 5.f);
	});

	/** La prima camera che nessuno di quelli in gioco ha (0 = tutte prese). (secondo controllo) Conta anche chi ha la
	 *  notte scaduta ma e' ancora in camera con la chiave: finche' Bruno non se la riprende, la camera e' sua. */
	int32 LocandaCameraLibera(UWorld* Mondo)
	{
		TSet<int32> Prese;
		if (Mondo)
		{
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				if (const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(It->Get()))
				{
					const FValdorsoZaino& Dentro = Zaino->Contenuto();
					if (Dentro.CameraLocanda > 0)
					{
						Prese.Add(Dentro.CameraLocanda);
					}
				}
			}
		}
		for (int32 N = 1; N <= ValdorsoLocanda::Camere; ++N)
		{
			if (!Prese.Contains(N))
			{
				return N;
			}
		}
		return 0;
	}
}

namespace ValdorsoLocanda
{
	FName ChiaveDellaCamera(int32 Camera)
	{
		return Camera >= 1 && Camera <= Camere ? FName(*FString::Printf(TEXT("Chiave_Camera_%d"), Camera)) : FName();
	}

	bool EChiaveDiCamera(FName Id)
	{
		return Id.ToString().StartsWith(TEXT("Chiave_Camera_"));
	}

	bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Png, FName& InOutProssimo)
	{
		static const FName EvCamera(TEXT("LocandaCamera"));
		static const FName EvRendi(TEXT("LocandaRendi"));
		if (Evento != EvCamera && Evento != EvRendi)
		{
			return false;
		}
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		if (!Chi || !Zaino)
		{
			InOutProssimo = FName(TEXT("ancora"));
			return true;
		}
		const int64 Adesso = LocandaAdesso();
		const FValdorsoZaino& Dentro = Zaino->Contenuto();
		// la sua camera (anche con la notte scaduta, finche' Bruno non si e' ripreso la chiave) e se la notte vale ancora
		const int32 Sua = Dentro.CameraLocanda;
		const bool bVale = Sua > 0 && Adesso < Dentro.CameraFinoUtc;

		if (Evento == EvRendi)
		{
			if (Sua == 0)
			{
				InOutProssimo = FName(TEXT("camera_nessuna"));
				return true;
			}
			Zaino->Cambia([](FValdorsoZaino& Z)
			{
				LocandaTogliChiavi(Z, 0);
				Z.CameraLocanda = 0;
				Z.CameraFinoUtc = 0;
				return true;
			});
			ValdorsoRegistroEventi::Annota(TEXT("Locanda"), Chi->GetPawn(), Png, Png ? Png->GetActorLocation() : FVector::ZeroVector, FString::Printf(TEXT("camera %d: chiave resa"), Sua));
			InOutProssimo = FName(TEXT("camera_resa"));
			return true;
		}

		// ha gia' la camera ma non la chiave (lo zaino era pieno, o l'ha svuotato): Bruno gliela rida', senza pagare
		if (bVale && ValdorsoZaino::Conta(Dentro, ChiaveDellaCamera(Sua)) == 0)
		{
			// (Dai avvisa da solo: "Ricevi..." o "Lo zaino e' pieno")
			InOutProssimo = FName(Zaino->Dai(ChiaveDellaCamera(Sua), 1, true) < 1 ? TEXT("ancora") : TEXT("camera_presa"));
			return true;
		}
		// una camera: nuova, o un'altra notte in quella che ha gia'
		if (Dentro.Monete < PrezzoNotte)
		{
			InOutProssimo = FName(TEXT("camera_soldi"));
			return true;
		}
		const int32 Camera = Sua > 0 ? Sua : LocandaCameraLibera(Chi->GetWorld());
		if (Camera == 0)
		{
			InOutProssimo = FName(TEXT("camera_piena"));
			return true;
		}
		const FName Chiave = ChiaveDellaCamera(Camera);
		bool bPagato = false;
		Zaino->Cambia([Camera, Sua, Adesso, &bPagato](FValdorsoZaino& Z)
		{
			if (Z.Monete < PrezzoNotte)
			{
				return false;
			}
			Z.Monete -= PrezzoNotte;
			Z.CameraLocanda = static_cast<uint8>(Camera);
			// un'altra notte si aggiunge a quella che resta
			Z.CameraFinoUtc = (Sua > 0 ? FMath::Max(Z.CameraFinoUtc, Adesso) : Adesso) + SecondiPerNotte;
			LocandaTogliChiavi(Z, Camera);
			bPagato = true;
			return true;
		});
		if (!bPagato)
		{
			InOutProssimo = FName(TEXT("camera_soldi"));
			return true;
		}
		if (ValdorsoZaino::Conta(Zaino->Contenuto(), Chiave) == 0 && Zaino->Dai(Chiave, 1, false) < 1)
		{
			// zaino pieno: la notte e' pagata e segnata, la chiave la ritira tornando da Bruno (qui sopra)
			LocandaAvvisa(Chi, LOCTEXT("ZainoPieno", "Lo zaino è pieno: fai posto e torna da Bruno, la chiave ti aspetta."));
		}
		else if (Sua > 0)
		{
			LocandaAvvisa(Chi, FText::Format(LOCTEXT("Allungata", "Camera {0}: un'altra notte pagata."), FText::AsNumber(Camera)));
		}
		else
		{
			LocandaAvvisa(Chi, FText::Format(LOCTEXT("Presa", "Camera {0}, al piano di sopra: è tua fino a domani a quest'ora."), FText::AsNumber(Camera)));
		}
		ValdorsoRegistroEventi::Annota(TEXT("Locanda"), Chi->GetPawn(), Png, Png ? Png->GetActorLocation() : FVector::ZeroVector,
			FString::Printf(TEXT("camera %d: %s, %d Lucio"), Camera, Sua > 0 ? TEXT("un'altra notte") : TEXT("presa"), PrezzoNotte));
		InOutProssimo = FName(Sua > 0 ? TEXT("camera_ancora") : TEXT("camera_presa"));
		return true;
	}
}

// ---------------------------------------------------------------------------------------------------------- console

namespace
{
	FAutoConsoleCommandWithWorldArgsAndOutputDevice LocandaComandoScade(TEXT("Valdorso.Locanda.Scade"),
		TEXT("Valdorso.Locanda.Scade [nome]: la notte alla locanda finisce subito (per le prove). Sul server o da soli."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Parole, UWorld* Mondo, FOutputDevice& Uscita)
		{
			if (!Mondo || Mondo->GetNetMode() == NM_Client)
			{
				Uscita.Log(TEXT("Valdorso.Locanda.Scade: solo sul server (o giocando da soli)."));
				return;
			}
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* Chi = It->Get();
				const FString Nome = Chi && Chi->PlayerState ? Chi->PlayerState->GetPlayerName() : FString();
				if (Parole.Num() > 0 && !Nome.Equals(Parole[0], ESearchCase::IgnoreCase))
				{
					continue;
				}
				if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi))
				{
					Zaino->Cambia([](FValdorsoZaino& Z)
					{
						if (Z.CameraLocanda == 0)
						{
							return false;
						}
						Z.CameraFinoUtc = 1;
						return true;
					});
					Uscita.Logf(TEXT("Valdorso.Locanda.Scade: %s, notte finita (la chiave sparisce entro 5 secondi, se non e' accanto alla porta)."), *Nome);
				}
			}
		}));
}

#undef LOCTEXT_NAMESPACE
