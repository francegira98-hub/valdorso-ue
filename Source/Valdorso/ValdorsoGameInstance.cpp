// Valdorso - L'istanza del gioco (vedi il .h).

#include "ValdorsoGameInstance.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoSicurezza.h"
#include "Valdorso.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/Paths.h"

namespace
{
	/** Prima del token c'è la versione del formato: se un giorno cambia, il server lo riconosce. */
	const TCHAR* const VersioneToken = TEXT("v1.");

	/** La chiave di AES-256-GCM è di 32 byte. */
	constexpr int32 ByteChiaveSessione = 32;

	/** Rimette in forma PEM una chiave pubblica scritta su una riga sola. */
	FString PemDaRiga(const FString& Riga)
	{
		FString Corpo = Riga.TrimStartAndEnd();
		Corpo.ReplaceInline(TEXT("-----BEGIN PUBLIC KEY-----"), TEXT(""));
		Corpo.ReplaceInline(TEXT("-----END PUBLIC KEY-----"), TEXT(""));
		Corpo.ReplaceInline(TEXT(" "), TEXT(""));
		Corpo.ReplaceInline(TEXT("\\n"), TEXT(""));
		FString Pem = TEXT("-----BEGIN PUBLIC KEY-----\n");
		for (int32 i = 0; i < Corpo.Len(); i += 64)
		{
			Pem += Corpo.Mid(i, 64) + TEXT("\n");
		}
		Pem += TEXT("-----END PUBLIC KEY-----\n");
		return Pem;
	}

	/** Il contrario: il PEM su una riga, da incollare in DefaultGame.ini. */
	FString RigaDaPem(const FString& Pem)
	{
		FString Riga = Pem;
		Riga.ReplaceInline(TEXT("-----BEGIN PUBLIC KEY-----"), TEXT(""));
		Riga.ReplaceInline(TEXT("-----END PUBLIC KEY-----"), TEXT(""));
		Riga.ReplaceInline(TEXT("\r"), TEXT(""));
		Riga.ReplaceInline(TEXT("\n"), TEXT(""));
		return Riga.TrimStartAndEnd();
	}
}

void UValdorsoGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		ManigliaErroreDiRete = GEngine->OnNetworkFailure().AddUObject(this, &UValdorsoGameInstance::SuErroreDiRete);
	}

	// Un server vero prepara subito la sua chiave (al primo avvio la crea).
	if (IsDedicatedServerInstance())
	{
		PreparaChiaveServer();
	}
}

void UValdorsoGameInstance::Shutdown()
{
	if (GEngine && ManigliaErroreDiRete.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(ManigliaErroreDiRete);
	}
	FMemory::Memzero(ChiaveSessione.GetData(), ChiaveSessione.Num());
	ChiaveSessione.Reset();
	RichiestaInSospeso.Reset();
	DimenticaBiglietto();
	Super::Shutdown();
}

FString UValdorsoGameInstance::CartellaChiavi() const
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Server/Chiavi"));
}

// ------------------------------------------------------------------------------------------------
// Server: la chiave privata e l'apertura dei token
// ------------------------------------------------------------------------------------------------

bool UValdorsoGameInstance::PreparaChiaveServer()
{
	if (!ChiavePrivataPem.IsEmpty())
	{
		return true;
	}

	const FString Cartella = CartellaChiavi();
	const FString FilePrivata = Cartella / TEXT("server_privata.pem");
	const FString FilePubblica = Cartella / TEXT("server_pubblica.pem");

	if (FFileHelper::LoadFileToString(ChiavePrivataPem, *FilePrivata) && !ChiavePrivataPem.IsEmpty())
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Chiave del server caricata da %s"), *FilePrivata);
		return true;
	}

	FString Privata, Pubblica;
	if (!ValdorsoSicurezza::CreaCoppiaChiavi(Privata, Pubblica))
	{
		return false;
	}
	IFileManager::Get().MakeDirectory(*Cartella, true);
	const bool bScritta = FFileHelper::SaveStringToFile(Privata, *FilePrivata, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
		&& FFileHelper::SaveStringToFile(Pubblica, *FilePubblica, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	const FString Leggimi =
		TEXT("Chiavi del server di Valdorso (create da sole al primo avvio del server).\r\n\r\n")
		TEXT("server_privata.pem  NON va mai su GitHub, mai in una chat, mai a nessuno. Sul server vero si copia in un posto sicuro\r\n")
		TEXT("                    insieme all'archivio (Saved/Server/Archivio). Se si perde, si crea una coppia nuova e si\r\n")
		TEXT("                    ridistribuisce il gioco con la chiave pubblica nuova.\r\n")
		TEXT("server_pubblica.pem si può dare a tutti: va nel gioco, in Config/DefaultGame.ini, sezione\r\n")
		TEXT("                    [/Script/Valdorso.ValdorsoGameInstance], riga ChiavePubblicaServer=<la riga qui sotto>\r\n\r\n")
		TEXT("Riga da incollare:\r\n") + RigaDaPem(Pubblica) + TEXT("\r\n");
	FFileHelper::SaveStringToFile(Leggimi, *(Cartella / TEXT("LEGGIMI.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	if (!bScritta)
	{
		UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Non riesco a salvare le chiavi del server in %s"), *Cartella);
		return false;
	}
	ChiavePrivataPem = Privata;
	UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Creata la coppia di chiavi del server in %s: leggi LEGGIMI.txt"), *Cartella);
	return true;
}

void UValdorsoGameInstance::ReceivedNetworkEncryptionToken(const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate)
{
	FEncryptionKeyResponse Risposta(EEncryptionResponse::Failure, TEXT("Chiave di sessione non valida"));

	TArray<uint8> Cifrati, Chiave;
	if (!PreparaChiaveServer())
	{
		Risposta.ErrorMsg = TEXT("Il server non ha la sua chiave");
	}
	else if (EncryptionToken.StartsWith(VersioneToken)
		&& EncryptionToken.Len() < 2048
		&& ValdorsoSicurezza::DaBase64PerIndirizzo(EncryptionToken.RightChop(FCString::Strlen(VersioneToken)), Cifrati)
		&& ValdorsoSicurezza::DecifraConPrivata(ChiavePrivataPem, Cifrati, Chiave)
		&& Chiave.Num() == ByteChiaveSessione)
	{
		Risposta.Response = EEncryptionResponse::Success;
		Risposta.ErrorMsg.Empty();
		Risposta.EncryptionData.Key = Chiave;
	}
	else
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Connessione rifiutata: chiave di sessione non valida."));
	}

	FMemory::Memzero(Chiave.GetData(), Chiave.Num());
	Delegate.ExecuteIfBound(Risposta);
}

// ------------------------------------------------------------------------------------------------
// Client
// ------------------------------------------------------------------------------------------------

bool UValdorsoGameInstance::ChiavePubblica(FString& OutPem) const
{
	if (!ChiavePubblicaServer.TrimStartAndEnd().IsEmpty())
	{
		OutPem = PemDaRiga(ChiavePubblicaServer);
		return true;
	}
#if !UE_BUILD_SHIPPING
	// Sviluppo: server e client sullo stesso PC.
	if (FFileHelper::LoadFileToString(OutPem, *(CartellaChiavi() / TEXT("server_pubblica.pem"))) && !OutPem.IsEmpty())
	{
		return true;
	}
#endif
	return false;
}

bool UValdorsoGameInstance::PuoiCollegarti(FString& OutErrore, const FString& Indirizzo) const
{
	if (!GetFirstLocalPlayerController())
	{
		OutErrore = TEXT("Il gioco non è pronto: riprova tra un attimo.");
		return false;
	}
	if (Indirizzo.IsEmpty() && IndirizzoServer.TrimStartAndEnd().IsEmpty())
	{
		OutErrore = TEXT("Manca l'indirizzo del server: aggiorna il gioco.");
		return false;
	}
	FString Pem;
	if (!ChiavePubblica(Pem))
	{
		OutErrore = TEXT("Manca la chiave del server: avvia prima il server una volta (la crea lui) oppure aggiorna il gioco.");
		return false;
	}
	return true;
}

bool UValdorsoGameInstance::Collegati(const FValdorsoRichiestaAccesso& Richiesta, FString& OutErrore, const FString& Indirizzo)
{
	if (!PuoiCollegarti(OutErrore, Indirizzo))
	{
		return false;
	}
	APlayerController* Controllore = GetFirstLocalPlayerController();

	FString Pem;
	ChiavePubblica(Pem);

	TArray<uint8> Cifrati;
	if (!ValdorsoSicurezza::BytesCasuali(ChiaveSessione, ByteChiaveSessione)
		|| !ValdorsoSicurezza::CifraConPubblica(Pem, ChiaveSessione, Cifrati))
	{
		OutErrore = TEXT("Non riesco a preparare la connessione sicura.");
		return false;
	}

	RichiestaInSospeso = Richiesta;
	UltimoNome = Richiesta.Nome;
	MessaggioInSospeso.Empty();

	const FString Dove = Indirizzo.IsEmpty() ? IndirizzoServer : Indirizzo;
	const FString Url = Dove + TEXT("?EncryptionToken=") + VersioneToken + ValdorsoSicurezza::Base64PerIndirizzo(Cifrati);
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Collegamento cifrato a %s"), *Dove);
	Controllore->ClientTravel(Url, TRAVEL_Absolute);
	return true;
}

void UValdorsoGameInstance::ReceivedNetworkEncryptionAck(const FOnEncryptionKeyResponse& Delegate)
{
	FEncryptionKeyResponse Risposta(EEncryptionResponse::Failure, TEXT("Nessuna chiave di sessione"));
	if (ChiaveSessione.Num() == ByteChiaveSessione)
	{
		Risposta.Response = EEncryptionResponse::Success;
		Risposta.ErrorMsg.Empty();
		Risposta.EncryptionData.Key = ChiaveSessione;
	}
	Delegate.ExecuteIfBound(Risposta);
}

bool UValdorsoGameInstance::PrendiRichiesta(FValdorsoRichiestaAccesso& Out)
{
	if (!RichiestaInSospeso.IsSet())
	{
		return false;
	}
	Out = RichiestaInSospeso.GetValue();
	RichiestaInSospeso.Reset();
	return true;
}

void UValdorsoGameInstance::RicordaMessaggio(const FString& Messaggio)
{
	// Il primo messaggio è quello giusto (il motivo dell'espulsione); "connessione persa" che arriva dopo non lo copre.
	if (MessaggioInSospeso.IsEmpty())
	{
		MessaggioInSospeso = Messaggio;
	}
}

FString UValdorsoGameInstance::PrendiMessaggio()
{
	FString Messaggio = MoveTemp(MessaggioInSospeso);
	MessaggioInSospeso.Empty();
	return Messaggio;
}

void UValdorsoGameInstance::RicordaBiglietto(const FString& NuovoBiglietto, const FString& Nome)
{
	Biglietto = NuovoBiglietto;
	NomeBiglietto = Nome;
}

void UValdorsoGameInstance::DimenticaBiglietto()
{
	Biglietto.Empty();
	NomeBiglietto.Empty();
	TentativiRientro = 0;
	bCollegamentoInterrotto = false;
}

bool UValdorsoGameInstance::PrendiRientro(FValdorsoRichiestaAccesso& Out)
{
	if (Biglietto.IsEmpty() || !bCollegamentoInterrotto || TentativiRientro >= 3)
	{
		return false;
	}
	++TentativiRientro;
	Out = FValdorsoRichiestaAccesso();
	Out.Modo = EValdorsoModoAccesso::Rientro;
	Out.Nome = NomeBiglietto;
	Out.Biglietto = Biglietto;
	return true;
}

void UValdorsoGameInstance::SuErroreDiRete(UWorld* Mondo, UNetDriver* Driver, ENetworkFailure::Type Tipo, const FString& Errore)
{
	RichiestaInSospeso.Reset();
	// Collegamento interrotto (non un gioco di versione diversa): se c'è il biglietto, il menu proverà a rientrare da solo.
	bCollegamentoInterrotto = Tipo != ENetworkFailure::OutdatedClient && Tipo != ENetworkFailure::OutdatedServer;
	UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Errore di rete: %s"), *Errore);
	switch (Tipo)
	{
	case ENetworkFailure::PendingConnectionFailure:
		RicordaMessaggio(TEXT("Non riesco a raggiungere il server. Forse è spento, oppure l'indirizzo è sbagliato."));
		break;
	case ENetworkFailure::OutdatedClient:
		RicordaMessaggio(TEXT("Il tuo gioco è più vecchio di quello del server: aggiornalo."));
		break;
	case ENetworkFailure::OutdatedServer:
		RicordaMessaggio(TEXT("Il server è più vecchio del tuo gioco."));
		break;
	default:
		RicordaMessaggio(TEXT("La connessione con il server si è interrotta."));
		break;
	}
}

// ------------------------------------------------------------------------------------------------
// Comandi di prova del client (solo sviluppo): la schermata "Prima di entrare" arriva al passo 2.2
// ------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING
namespace
{
	UValdorsoGameInstance* IstanzaDi(UWorld* Mondo, FOutputDevice& Uscita)
	{
		UValdorsoGameInstance* Istanza = Mondo ? Cast<UValdorsoGameInstance>(Mondo->GetGameInstance()) : nullptr;
		if (!Istanza)
		{
			Uscita.Log(TEXT("[Valdorso] Avvia prima il gioco (non nell'editor fermo)."));
		}
		return Istanza;
	}

	void Parti(UWorld* Mondo, FOutputDevice& Uscita, const FValdorsoRichiestaAccesso& Richiesta, const FString& Indirizzo)
	{
		if (UValdorsoGameInstance* Istanza = IstanzaDi(Mondo, Uscita))
		{
			FString Errore;
			if (Istanza->Collegati(Richiesta, Errore, Indirizzo))
			{
				Uscita.Log(TEXT("[Valdorso] Mi collego..."));
			}
			else
			{
				Uscita.Logf(TEXT("[Valdorso] %s"), *Errore);
			}
		}
	}

	void ComandoEntra(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (Argomenti.Num() < 2)
		{
			Uscita.Log(TEXT("Uso: Valdorso.Entra <nome> <password> [indirizzo]"));
			return;
		}
		FValdorsoRichiestaAccesso Richiesta;
		Richiesta.Modo = EValdorsoModoAccesso::Entra;
		Richiesta.Nome = Argomenti[0];
		Richiesta.Password = Argomenti[1];
		Parti(Mondo, Uscita, Richiesta, Argomenti.Num() > 2 ? Argomenti[2] : FString());
	}

	void ComandoPrimoIngresso(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (Argomenti.Num() < 3)
		{
			Uscita.Log(TEXT("Uso: Valdorso.PrimoIngresso <codice> <nome> <password> [indirizzo]"));
			return;
		}
		FValdorsoRichiestaAccesso Richiesta;
		Richiesta.Modo = EValdorsoModoAccesso::PrimoIngresso;
		Richiesta.CodiceInvito = Argomenti[0];
		Richiesta.Nome = Argomenti[1];
		Richiesta.Password = Argomenti[2];
		Parti(Mondo, Uscita, Richiesta, Argomenti.Num() > 3 ? Argomenti[3] : FString());
	}

	void ComandoCambiaPassword(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (Argomenti.Num() < 2)
		{
			Uscita.Log(TEXT("Uso: Valdorso.CambiaPassword <attuale> <nuova>"));
			return;
		}
		AValdorsoPlayerController* Controllore = Mondo ? Cast<AValdorsoPlayerController>(Mondo->GetFirstPlayerController()) : nullptr;
		if (!Controllore)
		{
			Uscita.Log(TEXT("[Valdorso] Funziona solo nell'anticamera del server."));
			return;
		}
		Controllore->ServerCambiaPassword(Argomenti[0], Argomenti[1]);
		Uscita.Log(TEXT("[Valdorso] Richiesta mandata..."));
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoClientEntra(TEXT("Valdorso.Entra"),
		TEXT("Solo sviluppo. Si collega al server e entra. Valdorso.Entra <nome> <password> [indirizzo]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ComandoEntra));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoClientPrimoIngresso(TEXT("Valdorso.PrimoIngresso"),
		TEXT("Solo sviluppo. Crea l'account con un invito. Valdorso.PrimoIngresso <codice> <nome> <password> [indirizzo]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ComandoPrimoIngresso));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoClientCambiaPassword(TEXT("Valdorso.CambiaPassword"),
		TEXT("Solo sviluppo. Cambia la password temporanea nell'anticamera. Valdorso.CambiaPassword <attuale> <nuova>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ComandoCambiaPassword));
}
#endif
