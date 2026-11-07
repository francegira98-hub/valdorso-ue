// Valdorso - Lo stato del mondo che il server racconta (vedi il .h).

#include "ValdorsoStatoMondo.h"
#include "ValdorsoCielo.h"
#include "ValdorsoTempo.h"
#include "ValdorsoVersione.h"
#include "Valdorso.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "CoreGlobals.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "HttpPath.h"

#define LOCTEXT_NAMESPACE "ValdorsoStatoMondo"

namespace ValdorsoStatoMondo
{
	FString InJson(const FValdorsoStatoMondo& Stato)
	{
		TSharedRef<FJsonObject> Oggetto = MakeShared<FJsonObject>();
		Oggetto->SetStringField(TEXT("gioco"), TEXT("Valdorso"));
		Oggetto->SetStringField(TEXT("versione"), Stato.Versione);
		Oggetto->SetStringField(TEXT("nome_versione"), Stato.NomeVersione);
		Oggetto->SetNumberField(TEXT("ora"), FMath::RoundToFloat(Stato.Ora * 100.f) / 100.f);
		Oggetto->SetStringField(TEXT("tempo"), Stato.Tempo);
		Oggetto->SetStringField(TEXT("stagione"), Stato.Stagione);
		Oggetto->SetNumberField(TEXT("coloni"), Stato.Coloni);
		Oggetto->SetNumberField(TEXT("posti"), Stato.Posti);
		Oggetto->SetStringField(TEXT("notizia"), Stato.Notizia);
		Oggetto->SetStringField(TEXT("aggiornato"), Stato.Aggiornato);
		Oggetto->SetBoolField(TEXT("aperto"), Stato.bAperto);

		FString Testo;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Scrittore = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Testo);
		FJsonSerializer::Serialize(Oggetto, Scrittore);
		return Testo;
	}

	bool DaJson(const FString& Testo, FValdorsoStatoMondo& OutStato)
	{
		TSharedPtr<FJsonObject> Oggetto;
		const TSharedRef<TJsonReader<TCHAR>> Lettore = TJsonReaderFactory<TCHAR>::Create(Testo);
		if (!FJsonSerializer::Deserialize(Lettore, Oggetto) || !Oggetto.IsValid())
		{
			return false;
		}
		FString Gioco;
		if (!Oggetto->TryGetStringField(TEXT("gioco"), Gioco) || Gioco != TEXT("Valdorso"))
		{
			return false;
		}
		FValdorsoStatoMondo Stato;
		Oggetto->TryGetStringField(TEXT("versione"), Stato.Versione);
		Oggetto->TryGetStringField(TEXT("nome_versione"), Stato.NomeVersione);
		double Numero = -1.0;
		if (Oggetto->TryGetNumberField(TEXT("ora"), Numero))
		{
			Stato.Ora = static_cast<float>(Numero);
		}
		Oggetto->TryGetStringField(TEXT("tempo"), Stato.Tempo);
		Oggetto->TryGetStringField(TEXT("stagione"), Stato.Stagione);
		if (Oggetto->TryGetNumberField(TEXT("coloni"), Numero))
		{
			Stato.Coloni = FMath::Max(0, FMath::RoundToInt(Numero));
		}
		if (Oggetto->TryGetNumberField(TEXT("posti"), Numero))
		{
			Stato.Posti = FMath::Max(0, FMath::RoundToInt(Numero));
		}
		Oggetto->TryGetStringField(TEXT("notizia"), Stato.Notizia);
		Stato.Notizia = Stato.Notizia.Left(200);
		Oggetto->TryGetStringField(TEXT("aggiornato"), Stato.Aggiornato);
		Oggetto->TryGetBoolField(TEXT("aperto"), Stato.bAperto);
		OutStato = Stato;
		return true;
	}

	FString OraLeggibile(float Ora)
	{
		if (Ora < 0.f)
		{
			return FString();
		}
		int32 Minuti = FMath::FloorToInt(FMath::Fmod(Ora, 24.f) * 60.f + 0.001f);
		Minuti = FMath::Clamp(Minuti, 0, 24 * 60 - 1);
		return FString::Printf(TEXT("%02d:%02d"), Minuti / 60, Minuti % 60);
	}

	FString StagioneDi(const FDateTime& Quando)
	{
		// come i contadini della valle: l'inverno da dicembre a febbraio, e così via
		switch (Quando.GetMonth())
		{
		case 12: case 1: case 2: return TEXT("inverno");
		case 3: case 4: case 5:  return TEXT("primavera");
		case 6: case 7: case 8:  return TEXT("estate");
		default:                 return TEXT("autunno");
		}
	}

	FText Riga(const FValdorsoStatoMondo& Stato)
	{
		if (!Stato.bAperto)
		{
			return LOCTEXT("Chiusa", "La valle è chiusa per lavori: lo staff la riapre presto.");
		}
		TArray<FString> Pezzi;
		const FString Ora = OraLeggibile(Stato.Ora);
		if (!Ora.IsEmpty())
		{
			Pezzi.Add(FText::Format(LOCTEXT("Ore", "Nella valle sono le {0}"), FText::FromString(Ora)).ToString());
		}
		if (!Stato.Tempo.IsEmpty())
		{
			Pezzi.Add(Stato.Tempo.ToLower());
		}
		if (!Stato.Stagione.IsEmpty())
		{
			Pezzi.Add(Stato.Stagione);
		}
		if (Stato.Coloni == 1)
		{
			Pezzi.Add(LOCTEXT("UnColono", "un colono").ToString());
		}
		else if (Stato.Coloni > 1)
		{
			Pezzi.Add(FText::Format(LOCTEXT("Coloni", "{0} coloni"), FText::AsNumber(Stato.Coloni)).ToString());
		}
		else
		{
			Pezzi.Add(LOCTEXT("NessunColono", "nessun colono in giro").ToString());
		}
		return FText::FromString(FString::Join(Pezzi, TEXT(" · ")));
	}

	FString IndirizzoStato(const FString& IndirizzoServer, int32 Porta)
	{
		FString Host = IndirizzoServer.TrimStartAndEnd();
		if (Host.IsEmpty())
		{
			return FString();
		}
		// via un eventuale "unreal://" o "http://" e la porta del gioco (l'IPv6 tra parentesi quadre resta intero)
		int32 Doppi = INDEX_NONE;
		if (Host.FindChar(TEXT('/'), Doppi))
		{
			Host.Split(TEXT("://"), nullptr, &Host);
		}
		if (Host.StartsWith(TEXT("[")))
		{
			int32 Chiusa = INDEX_NONE;
			if (Host.FindChar(TEXT(']'), Chiusa))
			{
				Host = Host.Left(Chiusa + 1);
			}
		}
		else
		{
			int32 DuePunti = INDEX_NONE;
			if (Host.FindLastChar(TEXT(':'), DuePunti))
			{
				Host = Host.Left(DuePunti);
			}
		}
		Host.RemoveFromEnd(TEXT("/"));
		return Host.IsEmpty() ? FString() : FString::Printf(TEXT("http://%s:%d/stato"), *Host, Porta);
	}

	FString NotiziaDaFile(const FString& Testo)
	{
		TArray<FString> Righe;
		Testo.ParseIntoArrayLines(Righe, true);
		for (int32 I = Righe.Num() - 1; I >= 0; --I)
		{
			const FString Riga = Righe[I].TrimStartAndEnd();
			if (!Riga.IsEmpty() && !Riga.StartsWith(TEXT("#")))
			{
				return Riga.Left(200);
			}
		}
		return FString();
	}
}

// ------------------------------------------------------------------------------------------------
// L'attore sul server
// ------------------------------------------------------------------------------------------------

AValdorsoStatoMondo::AValdorsoStatoMondo()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.f;
	// Vive solo sul server: ai PC non serve (lo chiedono alla pagina, prima di entrare).
	bReplicates = false;
	SetCanBeDamaged(false);
	JsonPronto = MakeShared<FString, ESPMode::ThreadSafe>();
}

void AValdorsoStatoMondo::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority() || GetNetMode() == NM_Client)
	{
		SetActorTickEnabled(false);
		return;
	}
	// Solo il server dedicato (o la prova, se chiesta): sul PC di chi gioca da solo non serve.
	if (!(IsRunningDedicatedServer() || bServiAncheInProva))
	{
		SetActorTickEnabled(false);
		return;
	}
	Componi();
	ApriPagina();
}

void AValdorsoStatoMondo::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ChiudiPagina();
	Super::EndPlay(EndPlayReason);
}

void AValdorsoStatoMondo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Attesa += DeltaSeconds;
	if (Attesa >= Intervallo)
	{
		Attesa = 0.f;
		Componi();
	}
}

void AValdorsoStatoMondo::Componi()
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	FValdorsoStatoMondo Stato;
	Stato.Versione = ValdorsoVersione::Numero();
	Stato.NomeVersione = ValdorsoVersione::Nome();
	for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
	{
		Stato.Ora = It->Ora();
		break;
	}
	if (const AValdorsoTempo* Tempo = AValdorsoTempo::Di(Mondo))
	{
		Stato.Tempo = ValdorsoTempo::Nome(Tempo->GetTempo());
	}
	const FDateTime Adesso = FDateTime::UtcNow();
	Stato.Stagione = ValdorsoStatoMondo::StagioneDi(Adesso);
	if (AGameModeBase* Modo = Mondo->GetAuthGameMode())
	{
		Stato.Coloni = Modo->GetNumPlayers();
		if (Modo->GameSession)
		{
			Stato.Posti = Modo->GameSession->MaxPlayers;
		}
	}
	// La notizia dello staff: Saved/Server/Notizie.txt (si legge ogni volta: si può cambiare mentre il server gira).
	const FString FileNotizie = FPaths::ProjectSavedDir() / TEXT("Server") / TEXT("Notizie.txt");
	FString TestoNotizie;
	if (FFileHelper::LoadFileToString(TestoNotizie, *FileNotizie))
	{
		Stato.Notizia = ValdorsoStatoMondo::NotiziaDaFile(TestoNotizie);
	}
	Stato.Aggiornato = Adesso.ToIso8601();
	Ultimo = Stato;
	*JsonPronto = ValdorsoStatoMondo::InJson(Stato);

	// Una copia su disco (la può leggere anche lo staff o un programma che la pubblica sul sito).
	if (Mondo->IsGameWorld())
	{
		const FString File = FPaths::ProjectSavedDir() / TEXT("Server") / TEXT("StatoMondo.json");
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
		FFileHelper::SaveStringToFile(*JsonPronto, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

void AValdorsoStatoMondo::ApriPagina()
{
	if (Router.IsValid())
	{
		return;
	}
	FHttpServerModule& Modulo = FHttpServerModule::Get();
	Router = Modulo.GetHttpRouter(static_cast<uint32>(FMath::Clamp(PortaStato, 1, 65535)), /*bFailOnBindFailure*/ true);
	if (!Router.IsValid())
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Stato del mondo: non riesco ad aprire la porta %d (è già usata?)"), PortaStato);
		return;
	}
	// La pagina tiene una copia del puntatore al JSON: se risponde mentre l'attore sparisce, non tocca memoria morta.
	TSharedPtr<FString, ESPMode::ThreadSafe> Json = JsonPronto;
	Maniglia = Router->BindRoute(FHttpPath(TEXT("/stato")), EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([Json](const FHttpServerRequest& Richiesta, const FHttpResultCallback& Fatto)
		{
			TUniquePtr<FHttpServerResponse> Risposta = FHttpServerResponse::Create(Json.IsValid() ? *Json : FString(TEXT("{}")), TEXT("application/json"));
			// il launcher e il sito possono leggerla anche da un'altra pagina
			Risposta->Headers.Add(TEXT("Access-Control-Allow-Origin"), { TEXT("*") });
			Risposta->Headers.Add(TEXT("Cache-Control"), { TEXT("no-store") });
			Fatto(MoveTemp(Risposta));
			return true;
		}));
	if (!Maniglia.IsValid())
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Stato del mondo: la pagina /stato è già legata sulla porta %d"), PortaStato);
		return;
	}
	Modulo.StartAllListeners();
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Stato del mondo: pagina aperta su http://<questo PC>:%d/stato"), PortaStato);
}

void AValdorsoStatoMondo::ChiudiPagina()
{
	if (Router.IsValid() && Maniglia.IsValid())
	{
		Router->UnbindRoute(Maniglia);
	}
	Maniglia.Reset();
	Router.Reset();
}

#undef LOCTEXT_NAMESPACE
