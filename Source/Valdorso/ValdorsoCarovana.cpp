// Valdorso - La carovana (vedi ValdorsoCarovana.h). Scritto da Claude il 12/11/2026.

#include "ValdorsoCarovana.h"
#include "Valdorso.h"
#include "ValdorsoPNG.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoScorte.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"

#define LOCTEXT_NAMESPACE "ValdorsoCarovana"

namespace
{
	bool CarovanaForzata = false;
	/** Il giorno vero dell'ultimo arrivo che ha portato le scorte (una volta al giorno: la giornata della valle e' piu'
	 *  corta di quella vera, e Sibilla torna ogni mattina della valle). */
	int64 CarovanaUltimoGiorno = -1;

	FAutoConsoleCommandWithWorldAndArgs ComandoCarovanaOggi(
		TEXT("Valdorso.Carovana.Oggi"),
		TEXT("Valdorso: Valdorso.Carovana.Oggi 1 fa venire la carovana di Sibilla oggi (prove); 0 torna al suo calendario."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			CarovanaForzata = Argomenti.Num() == 0 || Argomenti[0] != TEXT("0");
		}));
}

namespace ValdorsoCarovana
{
	bool Giorno(int64 Adesso, int32 OgniGiorni)
	{
		if (OgniGiorni <= 1)
		{
			return true;
		}
		const int64 Giorni = Adesso / 86400;
		return Giorni % OgniGiorni == 0;
	}

	bool Forzata()
	{
		return CarovanaForzata;
	}

	void Arriva(AValdorsoPNG* Sibilla)
	{
		UWorld* Mondo = Sibilla ? Sibilla->GetWorld() : nullptr;
		if (!Mondo || !Sibilla->HasAuthority())
		{
			return;
		}
		const int64 Oggi = FDateTime::UtcNow().ToUnixTimestamp() / 86400;
		if (Oggi == CarovanaUltimoGiorno)
		{
			return;
		}
		CarovanaUltimoGiorno = Oggi;
		FValdorsoScorteValle& Scorte = ValdorsoScorte::Adesso(Sibilla);
		Scorte.Grano = FMath::Min(ValdorsoScorte::Massimo, Scorte.Grano + GranoPortato);
		Scorte.Legna = FMath::Min(ValdorsoScorte::Massimo, Scorte.Legna + LegnaPortata);
		ValdorsoScorte::Salva(Sibilla);
		ValdorsoRegistroEventi::Annota(TEXT("Carovana"), Sibilla, nullptr, Sibilla->GetActorLocation(),
			FString::Printf(TEXT("la carovana di Sibilla: +%.0f grano, +%.0f legna al magazzino"), GranoPortato, LegnaPortata));
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(It->Get()))
			{
				Giocatore->ClientAvviso(LOCTEXT("Arrivata", "Al piazzale delle carovane suonano i campanelli dei muli: e' arrivata la carovana di Sibilla, con il lasciapassare della Corona."), 7.f, 0.f);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
