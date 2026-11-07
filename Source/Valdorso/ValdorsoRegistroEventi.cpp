// Valdorso - Il registro eventi del mondo (vedi il .h).
// (03/11/2026, v0.1.3) Gli eventi si scrivono anche su disco, sul server: Saved/Server/Eventi/Eventi_AAAA-MM-GG.jsonl,
// una riga JSON per evento (un file al giorno, ora UTC). È la base per leggi, taglie, dicerie e Battito.

#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTerritorio.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Valdorso.h"

namespace
{
	/** Il nome con cui un attore entra nel registro: per i coloni il nome del personaggio, per gli altri il nome dell'attore. */
	FString NomeNelRegistro(const AActor* Attore)
	{
		if (Attore == nullptr)
		{
			return FString();
		}
		if (const APawn* Corpo = Cast<APawn>(Attore))
		{
			if (const APlayerState* Stato = Corpo->GetPlayerState())
			{
				return Stato->GetPlayerName();
			}
		}
		return Attore->GetName();
	}

	/** Una stringa sicura dentro il JSON (virgolette, barre e a capo). */
	FString PerJson(const FString& Testo)
	{
		FString Fuori = Testo.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));
		Fuori.ReplaceInline(TEXT("\r"), TEXT(" "));
		Fuori.ReplaceInline(TEXT("\n"), TEXT(" "));
		Fuori.ReplaceInline(TEXT("\t"), TEXT(" "));
		return Fuori;
	}

	/** Il file del giorno (ora UTC). */
	FString FileDelGiorno(const FDateTime& Adesso)
	{
		return FPaths::ProjectSavedDir() / TEXT("Server") / TEXT("Eventi") / FString::Printf(TEXT("Eventi_%s.jsonl"), *Adesso.ToString(TEXT("%Y-%m-%d")));
	}
}

namespace ValdorsoRegistroEventi
{
	void Annota(const FString& Tipo, const AActor* Chi, const AActor* Bersaglio, const FVector& Dove, const FString& Dettagli)
	{
		// Solo il server annota: il PC di un giocatore non decide cosa è successo nel mondo.
		if (Chi == nullptr || !Chi->HasAuthority())
		{
			return;
		}

		const UWorld* Mondo = Chi->GetWorld();
		const float Quando = Mondo ? Mondo->GetTimeSeconds() : 0.f;
		const FString NomeChi = NomeNelRegistro(Chi);
		const FString NomeBersaglio = NomeNelRegistro(Bersaglio);
		// (07/11) In quale territorio è successo (per ora sempre "valdorso"): lo chiede il Design, per leggi e conquiste.
		const FString Territorio = ValdorsoTerritori::IdDi(Dove);

		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Registro: %s | chi %s | bersaglio %s | dove %s (%s) | quando %.2f s | %s"),
			*Tipo,
			*NomeChi,
			Bersaglio ? *NomeBersaglio : TEXT("nessuno"),
			*Dove.ToCompactString(),
			*Territorio,
			Quando,
			*Dettagli);

		// Su disco solo un server vero o la prova nell'editor; mai nei test automatici (nessun mondo di gioco).
		if (Mondo == nullptr || !(Mondo->IsGameWorld()))
		{
			return;
		}
		const FDateTime Adesso = FDateTime::UtcNow();
		static bool bCartellaPronta = false;
		if (!bCartellaPronta)
		{
			bCartellaPronta = IFileManager::Get().MakeDirectory(*FPaths::GetPath(FileDelGiorno(Adesso)), true);
		}
		const FString Riga = FString::Printf(
			TEXT("{\"utc\":\"%s\",\"mappa\":\"%s\",\"tempo\":%.2f,\"tipo\":\"%s\",\"chi\":\"%s\",\"bersaglio\":\"%s\",\"dove\":[%.0f,%.0f,%.0f],\"territorio\":\"%s\",\"dettagli\":\"%s\"}%s"),
			*Adesso.ToIso8601(),
			*PerJson(Mondo->GetMapName()),
			Quando,
			*PerJson(Tipo),
			*PerJson(NomeChi),
			*PerJson(NomeBersaglio),
			Dove.X, Dove.Y, Dove.Z,
			*PerJson(Territorio),
			*PerJson(Dettagli),
			LINE_TERMINATOR);
		// (Per ora la scrittura è immediata: gli eventi sono pochi. Con molti giocatori andrà in coda, come l'Archivista.)
		if (!FFileHelper::SaveStringToFile(Riga, *FileDelGiorno(Adesso), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(), FILEWRITE_Append))
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Registro eventi: non riesco a scrivere %s"), *FileDelGiorno(Adesso));
		}
	}
}
