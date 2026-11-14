// Valdorso - Il Banditore della Corona (vedi ValdorsoBanditore.h). Scritto da Claude il 13/11/2026.

#include "ValdorsoBanditore.h"
#include "Valdorso.h"
#include "ValdorsoGameMode.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoGilda.h"
#include "ValdorsoGildaDati.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoBanditore"

namespace
{
	/** Chi proclama, se il foglio non lo dice. */
	const TCHAR* BanditoreEnte = TEXT("Il Banditore della Corona");

	/** Ogni quanti secondi guarda i fogli approvati. */
	constexpr float BanditoreSecondi = 30.f;

	/** Quanto resta l'avviso sullo schermo. */
	constexpr float BanditoreSecondiAvviso = 10.f;

	FString BanditoreLeggi(const TSharedPtr<FJsonObject>& Oggetto, const TCHAR* Campo)
	{
		FString Valore;
		return Oggetto.IsValid() && Oggetto->TryGetStringField(Campo, Valore) ? Valore.TrimStartAndEnd() : FString();
	}
}

UValdorsoBanditore* UValdorsoBanditore::Di(const UObject* Contesto)
{
	UWorld* Mondo = Contesto ? Contesto->GetWorld() : nullptr;
	if (!Mondo || Mondo->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	UValdorsoBanditore* Lui = Mondo->GetSubsystem<UValdorsoBanditore>();
	return Lui && Lui->bAttivo ? Lui : nullptr;
}

FString UValdorsoBanditore::Cartella() const
{
	// come le bacheche (UValdorsoBacheche::File): le prove nell'editor e il gioco da soli hanno la loro cartella, così
	// non proclamano i fogli del server vero e non li segnano come fatti (stanno sullo stesso PC)
	const UWorld* Mondo = GetWorld();
	const UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	const TCHAR* Dove = Mondo && Mondo->IsPlayInEditor() ? TEXT("Prova") : (Archivista && Archivista->ArchivioSolo() ? TEXT("Solo") : TEXT("Server"));
	return FPaths::ProjectSavedDir() / Dove / TEXT("Gazzetta");
}

FString UValdorsoBanditore::FileProclamati() const
{
	return Cartella() / TEXT("Proclamati.txt");
}

bool UValdorsoBanditore::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* Mondo = Cast<UWorld>(Outer);
	return Mondo && (Mondo->WorldType == EWorldType::Game || Mondo->WorldType == EWorldType::PIE);
}

void UValdorsoBanditore::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// solo nella valle e solo sul server (come le bacheche)
	if (InWorld.GetNetMode() == NM_Client || !Cast<AValdorsoGameMode>(InWorld.GetAuthGameMode()))
	{
		return;
	}
	bAttivo = true;
	IFileManager::Get().MakeDirectory(*(Cartella() / TEXT("Approvate")), true);
	TArray<FString> Righe;
	if (FFileHelper::LoadFileToStringArray(Righe, *FileProclamati()))
	{
		for (const FString& Riga : Righe)
		{
			if (!Riga.TrimStartAndEnd().IsEmpty())
			{
				Proclamati.Add(Riga.TrimStartAndEnd());
			}
		}
	}
	// la prima volta dopo qualche secondo (le bacheche devono essere pronte), poi ogni 30
	InWorld.GetTimerManager().SetTimer(Orologio, FTimerDelegate::CreateUObject(this, &UValdorsoBanditore::Guarda), BanditoreSecondi, true, 5.f);
}

void UValdorsoBanditore::Deinitialize()
{
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(Orologio);
	}
	Super::Deinitialize();
}

void UValdorsoBanditore::Guarda()
{
	TArray<FString> Fogli;
	IFileManager::Get().FindFiles(Fogli, *(Cartella() / TEXT("Approvate") / TEXT("*.json")), true, false);
	Fogli.Sort();
	for (const FString& Nome : Fogli)
	{
		if (Proclamati.Contains(Nome))
		{
			continue;
		}
		// segnato subito, anche se il foglio è rotto: non si riprova a ogni giro
		Proclamati.Add(Nome);
		FFileHelper::SaveStringToFile(Nome + LINE_TERMINATOR, *FileProclamati(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(), FILEWRITE_Append);

		FString Testo;
		TSharedPtr<FJsonObject> Foglio;
		if (!FFileHelper::LoadFileToString(Testo, *(Cartella() / TEXT("Approvate") / Nome))
			|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Testo), Foglio) || !Foglio.IsValid())
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Banditore: non riesco a leggere %s"), *Nome);
			continue;
		}
		const FString Tipo = BanditoreLeggi(Foglio, TEXT("tipo"));
		// un foglio vecchio (più di una settimana: per esempio la prima volta in una cartella nuova) non si grida più
		FDateTime Data;
		if (FDateTime::ParseIso8601(*BanditoreLeggi(Foglio, TEXT("data")), Data) && (FDateTime::Now() - Data).GetTotalDays() > 7.0)
		{
			continue;
		}
		const FString Titolo = BanditoreLeggi(Foglio, TEXT("titolo"));
		FString Riassunto = BanditoreLeggi(Foglio, TEXT("riassunto"));
		if (Riassunto.IsEmpty())
		{
			Riassunto = BanditoreLeggi(Foglio, TEXT("testo"));
		}
		FString Ente = BanditoreLeggi(Foglio, TEXT("ente"));
		if (Ente.IsEmpty())
		{
			Ente = BanditoreEnte;
		}
		// sulla bacheca il riassunto (al massimo 190 lettere, come i proclami); a voce, per la Gazzetta il titolo, per un
		// annuncio il testo stesso
		const bool bAnnuncio = Tipo == TEXT("annuncio");
		const FString Bacheca = Riassunto;
		const FString Avviso = FString::Printf(TEXT("%s: %s"), *Ente, (bAnnuncio || Titolo.IsEmpty()) ? *Riassunto.Left(120) : *Titolo);
		Proclama(Bacheca, Avviso, Ente, BanditoreLeggi(Foglio, TEXT("autore")), Tipo);
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Banditore: proclamato %s (%s)"), *Nome, *Tipo);
	}
}

void UValdorsoBanditore::Proclama(const FString& Riassunto, const FString& Avviso, const FString& Ente, const FString& Autore, const FString& Tipo)
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	if (UValdorsoBacheche* Bacheche = UValdorsoBacheche::Di(this))
	{
		ValdorsoGilda::Proclama(Bacheche->Dati(), Riassunto, Autore.IsEmpty() ? FString(TEXT("staff")) : Autore, false,
			UValdorsoBacheche::Adesso(), Ente);
		Bacheche->Segna();
	}
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(It->Get()))
		{
			Giocatore->ClientAvviso(FText::FromString(Avviso), BanditoreSecondiAvviso, 0.f);
		}
	}
	// (il registro vuole un attore del server: le impostazioni del mondo fanno da "chi")
	ValdorsoRegistroEventi::Annota(TEXT("Banditore"), Mondo->GetWorldSettings(), nullptr, FVector::ZeroVector,
		FString::Printf(TEXT("%s · %s"), Tipo.IsEmpty() ? TEXT("foglio") : *Tipo, *Avviso.Left(120)));
}

FString UValdorsoBanditore::Annuncia(const FString& Testo, const FString& Autore)
{
	const FString Pulito = Testo.TrimStartAndEnd().Left(ValdorsoGilda::LettereProclama);
	if (Pulito.IsEmpty())
	{
		return TEXT("Scrivi il testo dell'annuncio.");
	}
	// scritto tra gli approvati come i fogli della Gazzetta: così va anche sul sito e nel launcher
	const FDateTime Ora = FDateTime::Now();
	// (un nome che non c'è ancora: due annunci nello stesso secondo non si sovrascrivono)
	FString Nome = FString::Printf(TEXT("Annuncio_%s.json"), *Ora.ToString(TEXT("%Y%m%d_%H%M%S")));
	for (int32 k = 2; IFileManager::Get().FileExists(*(Cartella() / TEXT("Approvate") / Nome)); ++k)
	{
		Nome = FString::Printf(TEXT("Annuncio_%s_%d.json"), *Ora.ToString(TEXT("%Y%m%d_%H%M%S")), k);
	}
	FString Json;
	const TSharedRef<TJsonWriter<>> Scrittore = TJsonWriterFactory<>::Create(&Json);
	Scrittore->WriteObjectStart();
	Scrittore->WriteValue(TEXT("tipo"), TEXT("annuncio"));
	Scrittore->WriteValue(TEXT("data"), Ora.ToString(TEXT("%Y-%m-%d")));
	Scrittore->WriteValue(TEXT("etichetta"), TEXT("Annuncio"));
	Scrittore->WriteValue(TEXT("titolo"), TEXT("Un annuncio per la valle"));
	Scrittore->WriteValue(TEXT("riassunto"), Pulito);
	Scrittore->WriteValue(TEXT("testo"), Pulito);
	Scrittore->WriteValue(TEXT("ente"), BanditoreEnte);
	Scrittore->WriteValue(TEXT("autore"), Autore);
	Scrittore->WriteObjectEnd();
	Scrittore->Close();
	if (!FFileHelper::SaveStringToFile(Json, *(Cartella() / TEXT("Approvate") / Nome), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		return TEXT("Non riesco a scrivere l'annuncio (Saved/Server/Gazzetta/Approvate).");
	}
	Guarda();
	return TEXT("Il Banditore l'ha proclamato: sulla bacheca della piazza e a tutti i coloni nella valle. Andrà anche sul sito con Pubblica (2).");
}

#undef LOCTEXT_NAMESPACE
