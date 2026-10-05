// Valdorso - L'archivista (vedi il .h).

#include "ValdorsoArchivista.h"
#include "ValdorsoSicurezza.h"
#include "ValdorsoRegole.h"
#include "Valdorso.h"
#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace
{
	// Regole e file dell'archivio: ora in ValdorsoRegole.h (così i test automatici li controllano).
	using ValdorsoRegole::TentativiPrimaDelBlocco;
	using ValdorsoRegole::LunghezzaCodice;
	using ValdorsoRegole::NomeValido;
	using ValdorsoRegole::NomeRiservato;
	using ValdorsoRegole::ProblemaPassword;
	using ValdorsoRegole::NormalizzaCodice;
	using ValdorsoArchivioFile::ScriviAtomico;
	using ValdorsoArchivioFile::LeggiFile;

	constexpr int32 ErroriIndirizzoMassimi = 20;
	constexpr int64 FinestraIndirizzoSecondi = 10 * 60;
	constexpr int64 BloccoIndirizzoSecondi = 30 * 60;
	constexpr int32 CalcoliInsieme = 4;
	constexpr int32 CodaMassima = 64;
	constexpr int32 GiorniInvitoPredefiniti = 14;

	// Senza caratteri che si confondono (0/O, 1/I/L).
	const TCHAR* const AlfabetoCodici = TEXT("23456789ABCDEFGHJKMNPQRSTUVWXYZ");
	const TCHAR* const AlfabetoPassword = TEXT("23456789abcdefghjkmnpqrstuvwxyz");

	int64 Adesso()
	{
		return FDateTime::UtcNow().ToUnixTimestamp();
	}

	FString Data(int64 Unix)
	{
		return Unix <= 0 ? FString(TEXT("mai")) : FDateTime::FromUnixTimestamp(Unix).ToString(TEXT("%d/%m/%Y %H:%M")) + TEXT(" UTC");
	}

	FString Chiave(const FString& Nome)
	{
		return Nome.TrimStartAndEnd().ToLower();
	}

	int32 Minuti(int64 Secondi)
	{
		return static_cast<int32>(FMath::Max<int64>(1, (Secondi + 59) / 60));
	}

	struct FImprontaNuova
	{
		bool bOk = false;
		FString Sale;
		FString Impronta;
		int32 Iterazioni = 0;
	};

	/** Sale nuovo e impronta di una password. Si chiama fuori dal filo principale. */
	FImprontaNuova CreaImpronta(const FString& Password)
	{
		FImprontaNuova Nuova;
		TArray<uint8> Sale, Impronta;
		if (ValdorsoSicurezza::BytesCasuali(Sale, ValdorsoSicurezza::LunghezzaSale)
			&& ValdorsoSicurezza::CalcolaImpronta(Password, Sale, ValdorsoSicurezza::IterazioniCorrenti, Impronta))
		{
			Nuova.bOk = true;
			Nuova.Sale = FBase64::Encode(Sale);
			Nuova.Impronta = FBase64::Encode(Impronta);
			Nuova.Iterazioni = ValdorsoSicurezza::IterazioniCorrenti;
		}
		return Nuova;
	}

	FValdorsoEsitoAccount Esito(EValdorsoEsitoAccount Tipo, const FString& Messaggio)
	{
		FValdorsoEsitoAccount Risultato;
		Risultato.Esito = Tipo;
		Risultato.Messaggio = Messaggio;
		return Risultato;
	}

	FValdorsoEsitoAccount TroppiTentativi(int64 SecondiRimasti)
	{
		return Esito(EValdorsoEsitoAccount::TroppiTentativi,
			FString::Printf(TEXT("Troppi tentativi sbagliati. Riprova tra %d minuti."), Minuti(SecondiRimasti)));
	}

	FValdorsoEsitoAccount CredenzialiSbagliate()
	{
		return Esito(EValdorsoEsitoAccount::CredenzialiSbagliate, TEXT("Nome o password sbagliati."));
	}

	FValdorsoEsitoAccount ServerOccupato()
	{
		return Esito(EValdorsoEsitoAccount::ServerOccupato, TEXT("Il server è molto occupato: riprova tra qualche secondo."));
	}

	FValdorsoEsitoAccount ErroreInterno()
	{
		return Esito(EValdorsoEsitoAccount::ErroreInterno, TEXT("Qualcosa non ha funzionato sul server. Riprova tra poco."));
	}

	const TCHAR* NomeRuolo(EValdorsoRuolo Ruolo)
	{
		switch (Ruolo)
		{
		case EValdorsoRuolo::Staff: return TEXT("Staff");
		case EValdorsoRuolo::Amministratore: return TEXT("Amministratore");
		default: return TEXT("Giocatore");
		}
	}

	const TCHAR* NomeStato(EValdorsoStatoAccount Stato)
	{
		switch (Stato)
		{
		case EValdorsoStatoAccount::Sospeso: return TEXT("Sospeso");
		case EValdorsoStatoAccount::Bandito: return TEXT("Bandito");
		default: return TEXT("Attivo");
		}
	}

}

// ------------------------------------------------------------------------------------------------
// Vita dell'archivista
// ------------------------------------------------------------------------------------------------

UValdorsoArchivista* UValdorsoArchivista::Di(const UObject* Contesto)
{
	const UWorld* Mondo = Contesto ? Contesto->GetWorld() : nullptr;
	if (!Mondo || Mondo->GetNetMode() == NM_Client || !Mondo->GetGameInstance())
	{
		return nullptr;
	}
	return Mondo->GetGameInstance()->GetSubsystem<UValdorsoArchivista>();
}

bool UValdorsoArchivista::ShouldCreateSubsystem(UObject* Outer) const
{
	// Un gioco che fa solo da client non ha archivio.
	return !IsRunningClientOnly() && Super::ShouldCreateSubsystem(Outer);
}

void UValdorsoArchivista::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Cartella = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Server/Archivio"));
	CartellaAccount = Cartella / TEXT("Account");
	CartellaPersonaggi = Cartella / TEXT("Personaggi");
	FileNomiRiservati = Cartella / TEXT("NomiRiservati.json");
	IFileManager::Get().MakeDirectory(*CartellaPersonaggi, true);
	FileInviti = Cartella / TEXT("Inviti.json");
	FileRegistro = Cartella / TEXT("Registro_accessi.log");
	IFileManager::Get().MakeDirectory(*CartellaAccount, true);

	Scrittore = MakeUnique<UE::Tasks::FPipe>(TEXT("ValdorsoArchivista"));
	ValdorsoSicurezza::BytesCasuali(SaleFinto, ValdorsoSicurezza::LunghezzaSale);

	CaricaTutto();

	int32 InvitiAttivi = 0;
	const int64 Ora = Adesso();
	for (const FValdorsoInvito& Invito : Inviti.Inviti)
	{
		if (Invito.UsatoDa.IsEmpty() && !Invito.bRevocato && Invito.ScadeIl > Ora)
		{
			++InvitiAttivi;
		}
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Archivista pronto: %d account, %d inviti attivi, cartella %s"),
		Account.Num(), InvitiAttivi, *Cartella);

	// Privacy: gli indirizzi IP vecchi si oscurano subito e poi ogni 6 ore.
	PulisciRegistro();
	ManigliaPulizia = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		PulisciRegistro();
		return true;
	}), 6.f * 60.f * 60.f);
}

void UValdorsoArchivista::Deinitialize()
{
	if (ManigliaPulizia.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ManigliaPulizia);
		ManigliaPulizia.Reset();
	}
	// Prima di chiudere si aspetta che tutte le scritture siano finite.
	if (UltimaScrittura.IsValid())
	{
		UltimaScrittura.Wait();
	}
	Scrittore.Reset();
	InAttesa.Empty();
	Super::Deinitialize();
}

// ------------------------------------------------------------------------------------------------
// File
// ------------------------------------------------------------------------------------------------

void UValdorsoArchivista::CaricaTutto()
{
	Account.Empty();
	TArray<FString> File;
	IFileManager::Get().FindFiles(File, *(CartellaAccount / TEXT("*.json")), true, false);
	for (const FString& Nome : File)
	{
		FValdorsoAccount Dati;
		if (!LeggiFile(CartellaAccount / Nome, Dati) || Dati.NomeChiave.IsEmpty() || Dati.Id.IsEmpty())
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a leggere l'account %s (né le copie)"), *Nome);
			continue;
		}
		if (Account.Contains(Dati.NomeChiave))
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: due file per lo stesso nome %s, tengo il primo"), *Dati.NomeChiave);
			continue;
		}
		Account.Add(Dati.NomeChiave, MoveTemp(Dati));
	}

	if (IFileManager::Get().FileExists(*FileInviti) && !LeggiFile(FileInviti, Inviti))
	{
		UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a leggere Inviti.json (né le copie)"));
	}

	// I personaggi (passo 3).
	Personaggi.Empty();
	NomiPersonaggi.Empty();
	TArray<FString> FilePersonaggi;
	IFileManager::Get().FindFiles(FilePersonaggi, *(CartellaPersonaggi / TEXT("*.json")), true, false);
	for (const FString& Nome : FilePersonaggi)
	{
		FValdorsoPersonaggio Dati;
		if (!LeggiFile(CartellaPersonaggi / Nome, Dati) || Dati.Id.IsEmpty() || Dati.AccountId.IsEmpty())
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a leggere il personaggio %s (né le copie)"), *Nome);
			continue;
		}
		if (!AccountPerId(Dati.AccountId))
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Archivista: il personaggio %s è di un account che non c'è più, lo salto"), *Nome);
			continue;
		}
		const FString Scheletro = ValdorsoRegole::ScheletroNome(Dati.Nome);
		if (NomiPersonaggi.Contains(Scheletro))
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: due personaggi con lo stesso nome (%s): tengo il primo"), *Dati.Nome);
			continue;
		}
		NomiPersonaggi.Add(Scheletro, Dati.Id);
		if (Dati.VersioneSchema < 2)
		{
			// Versione 1 (passo 3): il registro non c'era; resta vuoto e si compila al primo ingresso.
			Dati.VersioneSchema = 2;
		}
		Personaggi.Add(Dati.Id, MoveTemp(Dati));
	}
	if (IFileManager::Get().FileExists(*FileNomiRiservati) && !LeggiFile(FileNomiRiservati, NomiRiservati))
	{
		UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a leggere NomiRiservati.json (né le copie)"));
	}
}

void UValdorsoArchivista::SalvaAccount(FValdorsoAccount& Dati)
{
	++Dati.Versione;
	FString Testo;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Dati, Testo))
	{
		UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a preparare il file di %s"), *Dati.Nome);
		return;
	}
	Scrivi(CartellaAccount / (Dati.Id + TEXT(".json")), Testo);
}

void UValdorsoArchivista::SalvaNomiRiservati()
{
	++NomiRiservati.Versione;
	FString Testo;
	if (FJsonObjectConverter::UStructToJsonObjectString(NomiRiservati, Testo))
	{
		Scrivi(FileNomiRiservati, Testo);
	}
}

void UValdorsoArchivista::SalvaPersonaggio(FValdorsoPersonaggio& Dati, bool bGiocato)
{
	++Dati.Versione;
	FString Testo;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Dati, Testo))
	{
		UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a preparare il file del personaggio %s"), *Dati.Nome);
		return;
	}
	Scrivi(CartellaPersonaggi / (Dati.Id + TEXT(".json")), Testo);

	FValdorsoAccount* Proprietario = bGiocato ? AccountPerId(Dati.AccountId) : nullptr;
	if (Proprietario)
	{
		if (Proprietario->UltimoPersonaggio != Dati.Id)
		{
			Proprietario->UltimoPersonaggio = Dati.Id;
			SalvaAccount(*Proprietario);
		}
	}
}

void UValdorsoArchivista::SalvaInviti()
{
	++Inviti.Versione;
	FString Testo;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Inviti, Testo))
	{
		UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a preparare Inviti.json"));
		return;
	}
	Scrivi(FileInviti, Testo);
}

void UValdorsoArchivista::Scrivi(const FString& Percorso, const FString& Testo)
{
	if (!Scrittore.IsValid())
	{
		return;
	}
	UltimaScrittura = Scrittore->Launch(TEXT("ScritturaArchivio"), [Percorso, Testo]()
	{
		ScriviAtomico(Percorso, Testo);
	});
}

void UValdorsoArchivista::Annota(const FString& Riga)
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Archivista: %s"), *Riga);
	if (!Scrittore.IsValid())
	{
		return;
	}
	const FString Percorso = FileRegistro;
	const FString Testo = FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d %H:%M:%S")) + TEXT(" UTC | ") + Riga + LINE_TERMINATOR;
	UltimaScrittura = Scrittore->Launch(TEXT("RegistroAccessi"), [Percorso, Testo]()
	{
		FFileHelper::SaveStringToFile(Testo, *Percorso, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(), FILEWRITE_Append);
	});
}

// ------------------------------------------------------------------------------------------------
// Calcoli in coda (mai più di CalcoliInsieme alla volta: il server non si ferma anche se arrivano in tanti)
// ------------------------------------------------------------------------------------------------

bool UValdorsoArchivista::Accoda(FCalcolo Calcolo)
{
	if (InCorso >= CalcoliInsieme)
	{
		if (InAttesa.Num() >= CodaMassima)
		{
			return false;
		}
		InAttesa.Add(MoveTemp(Calcolo));
		return true;
	}
	Avvia(MoveTemp(Calcolo));
	return true;
}

void UValdorsoArchivista::Avvia(FCalcolo Calcolo)
{
	++InCorso;
	TWeakObjectPtr<UValdorsoArchivista> Debole(this);
	UE::Tasks::Launch(TEXT("ImprontaPassword"), [Debole, Calcolo]()
	{
		TFunction<void()> Seguito = Calcolo();
		AsyncTask(ENamedThreads::GameThread, [Debole, Seguito]()
		{
			if (UValdorsoArchivista* Archivista = Debole.Get())
			{
				if (Seguito)
				{
					Seguito();
				}
				Archivista->CalcoloFinito();
			}
		});
	});
}

void UValdorsoArchivista::CalcoloFinito()
{
	InCorso = FMath::Max(0, InCorso - 1);
	if (InAttesa.Num() > 0 && InCorso < CalcoliInsieme)
	{
		FCalcolo Prossimo = MoveTemp(InAttesa[0]);
		InAttesa.RemoveAt(0);
		Avvia(MoveTemp(Prossimo));
	}
}

// ------------------------------------------------------------------------------------------------
// Tentativi sbagliati
// ------------------------------------------------------------------------------------------------

bool UValdorsoArchivista::IndirizzoBloccato(const FString& Indirizzo, int64 Ora, int64& OutSecondiRimasti) const
{
	const FContoIndirizzo* Conto = ContiIndirizzi.Find(Indirizzo);
	if (Conto && Conto->BloccatoFino > Ora)
	{
		OutSecondiRimasti = Conto->BloccatoFino - Ora;
		return true;
	}
	return false;
}

void UValdorsoArchivista::ErroreDaIndirizzo(const FString& Indirizzo, int64 Ora)
{
	// Pulizia ogni tanto: gli indirizzi tranquilli da tempo si dimenticano.
	if (ContiIndirizzi.Num() > 5000)
	{
		for (auto It = ContiIndirizzi.CreateIterator(); It; ++It)
		{
			if (It->Value.BloccatoFino < Ora && Ora - It->Value.Inizio > FinestraIndirizzoSecondi)
			{
				It.RemoveCurrent();
			}
		}
	}

	FContoIndirizzo& Conto = ContiIndirizzi.FindOrAdd(Indirizzo);
	if (Ora - Conto.Inizio > FinestraIndirizzoSecondi)
	{
		Conto.Inizio = Ora;
		Conto.Errori = 0;
	}
	if (++Conto.Errori >= ErroriIndirizzoMassimi)
	{
		Conto.BloccatoFino = Ora + BloccoIndirizzoSecondi;
		Conto.Errori = 0;
		Annota(FString::Printf(TEXT("INDIRIZZO_BLOCCATO | ip %s | %d minuti"), *Indirizzo, Minuti(BloccoIndirizzoSecondi)));
	}
}

void UValdorsoArchivista::ErroreSullAccount(FValdorsoAccount& Dati, const FString& Indirizzo, int64 Ora)
{
	if (++Dati.TentativiFalliti >= TentativiPrimaDelBlocco)
	{
		++Dati.BlocchiDiFila;
		const int64 Durata = ValdorsoRegole::DurataBlocco(Dati.BlocchiDiFila);
		Dati.BloccatoFino = Ora + Durata;
		Dati.TentativiFalliti = 0;
		Annota(FString::Printf(TEXT("ACCOUNT_BLOCCATO | %s | %d minuti | ip %s"), *Dati.Nome, Minuti(Durata), *Indirizzo));
	}
	SalvaAccount(Dati);
}

// ------------------------------------------------------------------------------------------------
// Ricerca
// ------------------------------------------------------------------------------------------------

FValdorsoInvito* UValdorsoArchivista::TrovaInvitoValido(const FString& ImprontaCodice, int64 Ora)
{
	for (FValdorsoInvito& Invito : Inviti.Inviti)
	{
		if (Invito.ImprontaCodice == ImprontaCodice)
		{
			const bool bValido = Invito.UsatoDa.IsEmpty() && !Invito.bRevocato && (Invito.ScadeIl == 0 || Invito.ScadeIl > Ora);
			return bValido ? &Invito : nullptr;
		}
	}
	return nullptr;
}

FValdorsoAccount* UValdorsoArchivista::TrovaAccount(const FString& Nome)
{
	return Account.Find(Chiave(Nome));
}

const FValdorsoAccount* UValdorsoArchivista::TrovaAccount(const FString& Nome) const
{
	return Account.Find(Chiave(Nome));
}

FString UValdorsoArchivista::IdDi(const FString& Nome) const
{
	const FValdorsoAccount* Dati = TrovaAccount(Nome);
	return Dati ? Dati->Id : FString();
}

// ------------------------------------------------------------------------------------------------
// Accesso
// ------------------------------------------------------------------------------------------------

void UValdorsoArchivista::Accedi(const FString& Nome, const FString& Password, const FString& Indirizzo, FRisposta Risposta)
{
	check(IsInGameThread());
	const int64 Ora = Adesso();
	int64 Rimasti = 0;
	if (IndirizzoBloccato(Indirizzo, Ora, Rimasti))
	{
		Risposta(TroppiTentativi(Rimasti));
		return;
	}

	const FString NomeChiave = Chiave(Nome).Left(32);
	const FValdorsoAccount* Dati = Account.Find(NomeChiave);
	if (Dati && Dati->BloccatoFino > Ora)
	{
		Risposta(TroppiTentativi(Dati->BloccatoFino - Ora));
		return;
	}

	// Se il nome non esiste si fa lo stesso calcolo con un sale finto: la risposta arriva negli stessi tempi.
	TArray<uint8> Sale, Attesa;
	int32 Iterazioni = ValdorsoSicurezza::IterazioniCorrenti;
	bool bEsiste = Dati && FBase64::Decode(Dati->Sale, Sale) && FBase64::Decode(Dati->Impronta, Attesa) && Dati->Iterazioni > 0;
	if (bEsiste)
	{
		Iterazioni = Dati->Iterazioni;
	}
	else
	{
		Sale = SaleFinto;
		Attesa.SetNumZeroed(ValdorsoSicurezza::LunghezzaImpronta);
	}

	const bool bAccodato = Accoda([this, NomeChiave, Password, Indirizzo, Risposta, Sale, Attesa, Iterazioni, bEsiste]() -> TFunction<void()>
	{
		TArray<uint8> Calcolata;
		const bool bCalcolo = ValdorsoSicurezza::CalcolaImpronta(Password, Sale, Iterazioni, Calcolata);
		const bool bGiusta = bEsiste && bCalcolo && ValdorsoSicurezza::UgualiTempoCostante(Calcolata, Attesa);

		// Impronta fatta con meno giri di adesso: si rifà ora che abbiamo la password giusta.
		FImprontaNuova Nuova;
		if (bGiusta && Iterazioni < ValdorsoSicurezza::IterazioniCorrenti)
		{
			Nuova = CreaImpronta(Password);
		}

		return [this, NomeChiave, Indirizzo, Risposta, bGiusta, Nuova]()
		{
			ConcludiAccesso(NomeChiave, Indirizzo, bGiusta, Nuova.bOk ? Nuova.Sale : FString(), Nuova.Impronta, Nuova.Iterazioni, Risposta);
		};
	});

	if (!bAccodato)
	{
		Risposta(ServerOccupato());
	}
}

void UValdorsoArchivista::ConcludiAccesso(const FString& NomeChiave, const FString& Indirizzo, bool bGiusta,
	const FString& NuovoSale, const FString& NuovaImpronta, int32 NuoveIterazioni, FRisposta Risposta)
{
	const int64 Ora = Adesso();
	FValdorsoAccount* Dati = Account.Find(NomeChiave);

	if (!bGiusta || !Dati)
	{
		ErroreDaIndirizzo(Indirizzo, Ora);
		if (Dati)
		{
			ErroreSullAccount(*Dati, Indirizzo, Ora);
		}
		Annota(FString::Printf(TEXT("ACCESSO_SBAGLIATO | %s | ip %s"), *NomeChiave, *Indirizzo));
		Risposta(CredenzialiSbagliate());
		return;
	}

	Dati->TentativiFalliti = 0;
	Dati->BlocchiDiFila = 0;
	Dati->BloccatoFino = 0;

	if (!NuovoSale.IsEmpty())
	{
		Dati->Algoritmo = ValdorsoSicurezza::Algoritmo;
		Dati->Sale = NuovoSale;
		Dati->Impronta = NuovaImpronta;
		Dati->Iterazioni = NuoveIterazioni;
		Annota(FString::Printf(TEXT("IMPRONTA_AGGIORNATA | %s | %d giri"), *Dati->Nome, NuoveIterazioni));
	}

	if (Dati->Stato == EValdorsoStatoAccount::Sospeso && Dati->SospesoFino <= Ora)
	{
		Dati->Stato = EValdorsoStatoAccount::Attivo;
		Dati->MotivoStato.Empty();
		Annota(FString::Printf(TEXT("SOSPENSIONE_FINITA | %s"), *Dati->Nome));
	}

	if (Dati->Stato == EValdorsoStatoAccount::Bandito)
	{
		SalvaAccount(*Dati);
		Annota(FString::Printf(TEXT("ACCESSO_RIFIUTATO | %s | bandito | ip %s"), *Dati->Nome, *Indirizzo));
		Risposta(Esito(EValdorsoEsitoAccount::AccountBandito,
			FString::Printf(TEXT("Questo account è stato bandito dalla valle. Motivo: %s"), *Dati->MotivoStato)));
		return;
	}
	if (Dati->Stato == EValdorsoStatoAccount::Sospeso)
	{
		SalvaAccount(*Dati);
		Annota(FString::Printf(TEXT("ACCESSO_RIFIUTATO | %s | sospeso | ip %s"), *Dati->Nome, *Indirizzo));
		Risposta(Esito(EValdorsoEsitoAccount::AccountSospeso,
			FString::Printf(TEXT("Questo account è sospeso fino al %s. Motivo: %s"), *Data(Dati->SospesoFino), *Dati->MotivoStato)));
		return;
	}

	Dati->UltimoAccesso = Ora;
	SalvaAccount(*Dati);
	Annota(FString::Printf(TEXT("ACCESSO | %s | ip %s"), *Dati->Nome, *Indirizzo));

	FValdorsoEsitoAccount Risultato;
	Risultato.AccountId = Dati->Id;
	Risultato.Nome = Dati->Nome;
	Risultato.Ruolo = Dati->Ruolo;
	if (Dati->bDeveCambiarePassword)
	{
		Risultato.Esito = EValdorsoEsitoAccount::OkDeveCambiarePassword;
		Risultato.Messaggio = FString::Printf(TEXT("Bentornato, %s. La tua password è temporanea: scegline una nuova."), *Dati->Nome);
	}
	else
	{
		Risultato.Esito = EValdorsoEsitoAccount::Ok;
		Risultato.Messaggio = FString::Printf(TEXT("Bentornato nella valle, %s."), *Dati->Nome);

		// Chi non ha ancora i codici di recupero (account di prima del 04/10, o codici finiti) li riceve ora.
		if (Dati->CodiciRecupero.Num() == 0)
		{
			Risultato.CodiciRecupero = CreaCodiciRecupero(*Dati);
		}
	}
	Risposta(Risultato);
}

// ------------------------------------------------------------------------------------------------
// Primo ingresso
// ------------------------------------------------------------------------------------------------

void UValdorsoArchivista::CreaAccount(const FString& CodiceInvito, const FString& Nome, const FString& Password, const FString& Indirizzo, FRisposta Risposta)
{
	check(IsInGameThread());
	const int64 Ora = Adesso();
	int64 Rimasti = 0;
	if (IndirizzoBloccato(Indirizzo, Ora, Rimasti))
	{
		Risposta(TroppiTentativi(Rimasti));
		return;
	}

	const FString NomePulito = Nome.TrimStartAndEnd();
	const FString NomeChiave = Chiave(NomePulito);
	if (!NomeValido(NomePulito))
	{
		Risposta(Esito(EValdorsoEsitoAccount::NomeNonValido,
			TEXT("Il nome deve avere da 3 a 20 caratteri (lettere senza accenti, numeri, punto, trattino o trattino basso) e cominciare con una lettera.")));
		return;
	}
	if (NomeRiservato(NomeChiave) || Account.Contains(NomeChiave) || NomiInCreazione.Contains(NomeChiave))
	{
		Risposta(Esito(EValdorsoEsitoAccount::NomeGiaUsato, TEXT("Questo nome è già preso: scegline un altro.")));
		return;
	}
	const FString Problema = ProblemaPassword(Password, NomePulito);
	if (!Problema.IsEmpty())
	{
		Risposta(Esito(EValdorsoEsitoAccount::PasswordDebole, Problema));
		return;
	}

	const FString ImprontaCodice = ValdorsoSicurezza::Sha256Esadecimale(NormalizzaCodice(CodiceInvito));
	if (!TrovaInvitoValido(ImprontaCodice, Ora) || InvitiInUso.Contains(ImprontaCodice))
	{
		ErroreDaIndirizzo(Indirizzo, Ora);
		Annota(FString::Printf(TEXT("INVITO_SBAGLIATO | ip %s"), *Indirizzo));
		Risposta(Esito(EValdorsoEsitoAccount::InvitoNonValido, TEXT("Il codice d'invito non è valido, è scaduto o è già stato usato.")));
		return;
	}

	// Nome e invito restano prenotati mentre si calcola l'impronta.
	NomiInCreazione.Add(NomeChiave);
	InvitiInUso.Add(ImprontaCodice);

	const bool bAccodato = Accoda([this, NomePulito, ImprontaCodice, Indirizzo, Password, Risposta]() -> TFunction<void()>
	{
		const FImprontaNuova Nuova = CreaImpronta(Password);
		return [this, NomePulito, ImprontaCodice, Indirizzo, Nuova, Risposta]()
		{
			ConcludiCreazione(NomePulito, ImprontaCodice, Indirizzo, Nuova.bOk, Nuova.Sale, Nuova.Impronta, Nuova.Iterazioni, Risposta);
		};
	});

	if (!bAccodato)
	{
		NomiInCreazione.Remove(NomeChiave);
		InvitiInUso.Remove(ImprontaCodice);
		Risposta(ServerOccupato());
	}
}

void UValdorsoArchivista::ConcludiCreazione(const FString& Nome, const FString& ImprontaCodice, const FString& Indirizzo, bool bCalcoloOk,
	const FString& Sale, const FString& Impronta, int32 Iterazioni, FRisposta Risposta)
{
	const FString NomeChiave = Chiave(Nome);
	NomiInCreazione.Remove(NomeChiave);
	InvitiInUso.Remove(ImprontaCodice);

	const int64 Ora = Adesso();
	if (!bCalcoloOk)
	{
		Risposta(ErroreInterno());
		return;
	}
	FValdorsoInvito* Invito = TrovaInvitoValido(ImprontaCodice, Ora);
	if (!Invito)
	{
		Risposta(Esito(EValdorsoEsitoAccount::InvitoNonValido, TEXT("Il codice d'invito non è valido, è scaduto o è già stato usato.")));
		return;
	}
	if (Account.Contains(NomeChiave))
	{
		Risposta(Esito(EValdorsoEsitoAccount::NomeGiaUsato, TEXT("Questo nome è già preso: scegline un altro.")));
		return;
	}

	const bool bPrimo = Account.Num() == 0;

	FValdorsoAccount Nuovo;
	Nuovo.Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Nuovo.Nome = Nome;
	Nuovo.NomeChiave = NomeChiave;
	Nuovo.Algoritmo = ValdorsoSicurezza::Algoritmo;
	Nuovo.Iterazioni = Iterazioni;
	Nuovo.Sale = Sale;
	Nuovo.Impronta = Impronta;
	Nuovo.Ruolo = bPrimo ? EValdorsoRuolo::Amministratore : EValdorsoRuolo::Giocatore;
	Nuovo.CreatoIl = Ora;
	Nuovo.UltimoAccesso = Ora;
	Nuovo.PasswordCambiataIl = Ora;
	Nuovo.InvitoUsato = Invito->Indizio;
	Nuovo.InvitatoDa = Invito->CreatoDa;

	Invito->UsatoDa = Nome;
	Invito->UsatoIl = Ora;
	const FString Indizio = Invito->Indizio;

	FValdorsoAccount& Salvato = Account.Add(NomeChiave, MoveTemp(Nuovo));
	const TArray<FString> Codici = CreaCodiciRecupero(Salvato);
	SalvaAccount(Salvato);  // i codici si salvano dopo, quando il giocatore conferma di averli scritti
	SalvaInviti();

	Annota(FString::Printf(TEXT("ACCOUNT_CREATO | %s | invito %s | ip %s"), *Salvato.Nome, *Indizio, *Indirizzo));
	if (bPrimo)
	{
		Annota(FString::Printf(TEXT("PRIMO_AMMINISTRATORE | %s"), *Salvato.Nome));
	}

	FValdorsoEsitoAccount Risultato;
	Risultato.Esito = EValdorsoEsitoAccount::Ok;
	Risultato.Messaggio = FString::Printf(TEXT("Account creato. Benvenuto nella valle, %s."), *Salvato.Nome);
	Risultato.AccountId = Salvato.Id;
	Risultato.Nome = Salvato.Nome;
	Risultato.Ruolo = Salvato.Ruolo;
	Risultato.CodiciRecupero = Codici;
	Risposta(Risultato);
}

// ------------------------------------------------------------------------------------------------
// Cambio password
// ------------------------------------------------------------------------------------------------

void UValdorsoArchivista::CambiaPassword(const FString& Nome, const FString& Attuale, const FString& Nuova, const FString& Indirizzo, FRisposta Risposta)
{
	check(IsInGameThread());
	const int64 Ora = Adesso();
	int64 Rimasti = 0;
	if (IndirizzoBloccato(Indirizzo, Ora, Rimasti))
	{
		Risposta(TroppiTentativi(Rimasti));
		return;
	}

	const FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		ErroreDaIndirizzo(Indirizzo, Ora);
		Risposta(CredenzialiSbagliate());
		return;
	}
	if (Dati->BloccatoFino > Ora)
	{
		Risposta(TroppiTentativi(Dati->BloccatoFino - Ora));
		return;
	}
	if (Nuova == Attuale)
	{
		Risposta(Esito(EValdorsoEsitoAccount::PasswordDebole, TEXT("La password nuova deve essere diversa da quella di adesso.")));
		return;
	}
	const FString Problema = ProblemaPassword(Nuova, Dati->Nome);
	if (!Problema.IsEmpty())
	{
		Risposta(Esito(EValdorsoEsitoAccount::PasswordDebole, Problema));
		return;
	}

	TArray<uint8> Sale, Attesa;
	if (!FBase64::Decode(Dati->Sale, Sale) || !FBase64::Decode(Dati->Impronta, Attesa))
	{
		Risposta(ErroreInterno());
		return;
	}
	const int32 Iterazioni = Dati->Iterazioni;
	const FString NomeChiave = Dati->NomeChiave;

	const bool bAccodato = Accoda([this, NomeChiave, Attuale, Nuova, Indirizzo, Risposta, Sale, Attesa, Iterazioni]() -> TFunction<void()>
	{
		TArray<uint8> Calcolata;
		const bool bGiusta = ValdorsoSicurezza::CalcolaImpronta(Attuale, Sale, Iterazioni, Calcolata)
			&& ValdorsoSicurezza::UgualiTempoCostante(Calcolata, Attesa);
		FImprontaNuova Nuove;
		if (bGiusta)
		{
			Nuove = CreaImpronta(Nuova);
		}
		return [this, NomeChiave, Indirizzo, Risposta, bGiusta, Nuove]()
		{
			if (bGiusta && !Nuove.bOk)
			{
				Risposta(ErroreInterno());
				return;
			}
			ConcludiCambio(NomeChiave, Indirizzo, bGiusta, Nuove.Sale, Nuove.Impronta, Nuove.Iterazioni, Risposta);
		};
	});

	if (!bAccodato)
	{
		Risposta(ServerOccupato());
	}
}

void UValdorsoArchivista::ConcludiCambio(const FString& NomeChiave, const FString& Indirizzo, bool bGiusta,
	const FString& Sale, const FString& Impronta, int32 Iterazioni, FRisposta Risposta)
{
	const int64 Ora = Adesso();
	FValdorsoAccount* Dati = Account.Find(NomeChiave);
	if (!Dati)
	{
		Risposta(CredenzialiSbagliate());
		return;
	}
	if (!bGiusta)
	{
		ErroreDaIndirizzo(Indirizzo, Ora);
		ErroreSullAccount(*Dati, Indirizzo, Ora);
		Annota(FString::Printf(TEXT("CAMBIO_PASSWORD_SBAGLIATO | %s | ip %s"), *Dati->Nome, *Indirizzo));
		Risposta(Esito(EValdorsoEsitoAccount::CredenzialiSbagliate, TEXT("La password di adesso è sbagliata.")));
		return;
	}

	Dati->Algoritmo = ValdorsoSicurezza::Algoritmo;
	Dati->Sale = Sale;
	Dati->Impronta = Impronta;
	Dati->Iterazioni = Iterazioni;
	Dati->bDeveCambiarePassword = false;
	Dati->PasswordCambiataIl = Ora;
	Dati->TentativiFalliti = 0;
	SalvaAccount(*Dati);
	Annota(FString::Printf(TEXT("PASSWORD_CAMBIATA | %s | ip %s"), *Dati->Nome, *Indirizzo));

	FValdorsoEsitoAccount Risultato;
	Risultato.Esito = EValdorsoEsitoAccount::Ok;
	Risultato.Messaggio = TEXT("Password cambiata.");
	Risultato.AccountId = Dati->Id;
	Risultato.Nome = Dati->Nome;
	Risultato.Ruolo = Dati->Ruolo;
	Risposta(Risultato);
}

// ------------------------------------------------------------------------------------------------
// Collegamenti
// ------------------------------------------------------------------------------------------------

bool UValdorsoArchivista::SegnaCollegato(const FString& AccountId)
{
	if (Collegati.Contains(AccountId))
	{
		return false;
	}
	Collegati.Add(AccountId);
	return true;
}

void UValdorsoArchivista::SegnaScollegato(const FString& AccountId)
{
	Collegati.Remove(AccountId);

	// Da adesso il biglietto del rientro ha 15 minuti di vita.
	const int64 Scadenza = Adesso() + ValdorsoRegole::DurataRientroSecondi;
	for (TPair<FString, FBiglietto>& Coppia : Biglietti)
	{
		if (Coppia.Value.AccountId == AccountId && Coppia.Value.ScadeIl == 0)
		{
			Coppia.Value.ScadeIl = Scadenza;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Rientro senza password
// ------------------------------------------------------------------------------------------------

FString UValdorsoArchivista::CreaBiglietto(const FString& AccountId)
{
	check(IsInGameThread());
	if (AccountId.IsEmpty())
	{
		return FString();
	}
	AnnullaBiglietto(AccountId);

	// Pulizia: via i biglietti scaduti.
	const int64 Ora = Adesso();
	for (auto It = Biglietti.CreateIterator(); It; ++It)
	{
		if (It->Value.ScadeIl != 0 && It->Value.ScadeIl <= Ora)
		{
			It.RemoveCurrent();
		}
	}

	TArray<uint8> Casuali;
	if (!ValdorsoSicurezza::BytesCasuali(Casuali, 32))
	{
		return FString();
	}
	const FString Biglietto = ValdorsoSicurezza::Base64PerIndirizzo(Casuali);
	FMemory::Memzero(Casuali.GetData(), Casuali.Num());

	FBiglietto Nuovo;
	Nuovo.AccountId = AccountId;
	Nuovo.ScadeIl = 0;
	Biglietti.Add(ValdorsoSicurezza::Sha256Esadecimale(Biglietto), Nuovo);
	return Biglietto;
}

void UValdorsoArchivista::AnnullaBiglietto(const FString& AccountId)
{
	for (auto It = Biglietti.CreateIterator(); It; ++It)
	{
		if (It->Value.AccountId == AccountId)
		{
			It.RemoveCurrent();
		}
	}
}

void UValdorsoArchivista::RientraConBiglietto(const FString& Biglietto, const FString& Indirizzo, FRisposta Risposta)
{
	check(IsInGameThread());
	const int64 Ora = Adesso();
	int64 Rimasti = 0;
	if (IndirizzoBloccato(Indirizzo, Ora, Rimasti))
	{
		Risposta(TroppiTentativi(Rimasti));
		return;
	}

	const FValdorsoEsitoAccount NonValido = Esito(EValdorsoEsitoAccount::RientroNonValido,
		TEXT("Il rientro rapido non vale più: scrivi la password."));

	// Il biglietto vale una volta: si toglie subito, che vada bene o no.
	FBiglietto Trovato;
	const FString Impronta = Biglietto.IsEmpty() ? FString() : ValdorsoSicurezza::Sha256Esadecimale(Biglietto);
	if (Impronta.IsEmpty() || !Biglietti.RemoveAndCopyValue(Impronta, Trovato) || (Trovato.ScadeIl != 0 && Trovato.ScadeIl <= Ora))
	{
		ErroreDaIndirizzo(Indirizzo, Ora);
		Annota(FString::Printf(TEXT("RIENTRO_RIFIUTATO | biglietto non valido | ip %s"), *Indirizzo));
		Risposta(NonValido);
		return;
	}

	FValdorsoAccount* Dati = nullptr;
	for (TPair<FString, FValdorsoAccount>& Coppia : Account)
	{
		if (Coppia.Value.Id == Trovato.AccountId)
		{
			Dati = &Coppia.Value;
			break;
		}
	}
	if (!Dati)
	{
		Risposta(NonValido);
		return;
	}

	// Le stesse regole dell'accesso con la password.
	if (Dati->Stato == EValdorsoStatoAccount::Sospeso && Dati->SospesoFino <= Ora)
	{
		Dati->Stato = EValdorsoStatoAccount::Attivo;
		Dati->MotivoStato.Empty();
		Annota(FString::Printf(TEXT("SOSPENSIONE_FINITA | %s"), *Dati->Nome));
	}
	if (Dati->Stato == EValdorsoStatoAccount::Bandito)
	{
		Annota(FString::Printf(TEXT("RIENTRO_RIFIUTATO | %s | bandito | ip %s"), *Dati->Nome, *Indirizzo));
		Risposta(Esito(EValdorsoEsitoAccount::AccountBandito,
			FString::Printf(TEXT("Questo account è stato bandito dalla valle. Motivo: %s"), *Dati->MotivoStato)));
		return;
	}
	if (Dati->Stato == EValdorsoStatoAccount::Sospeso)
	{
		Annota(FString::Printf(TEXT("RIENTRO_RIFIUTATO | %s | sospeso | ip %s"), *Dati->Nome, *Indirizzo));
		Risposta(Esito(EValdorsoEsitoAccount::AccountSospeso,
			FString::Printf(TEXT("Questo account è sospeso fino al %s. Motivo: %s"), *Data(Dati->SospesoFino), *Dati->MotivoStato)));
		return;
	}
	if (Dati->bDeveCambiarePassword || Dati->BloccatoFino > Ora)
	{
		Risposta(NonValido);
		return;
	}

	Dati->UltimoAccesso = Ora;
	SalvaAccount(*Dati);
	Annota(FString::Printf(TEXT("RIENTRO | %s | ip %s"), *Dati->Nome, *Indirizzo));

	FValdorsoEsitoAccount Risultato;
	Risultato.Esito = EValdorsoEsitoAccount::Ok;
	Risultato.AccountId = Dati->Id;
	Risultato.Nome = Dati->Nome;
	Risultato.Ruolo = Dati->Ruolo;
	Risultato.Messaggio = FString::Printf(TEXT("Bentornato nella valle, %s."), *Dati->Nome);
	Risposta(Risultato);
}

// ------------------------------------------------------------------------------------------------
// Amministrazione
// ------------------------------------------------------------------------------------------------

FString UValdorsoArchivista::CreaInvito(int32 Giorni, const FString& Autore, const FString& Nota, FString& OutCodice)
{
	const int64 Ora = Adesso();
	Giorni = Giorni <= 0 ? GiorniInvitoPredefiniti : FMath::Min(Giorni, 365);

	const FString Grezzo = ValdorsoSicurezza::TestoCasuale(AlfabetoCodici, LunghezzaCodice);
	if (Grezzo.Len() != LunghezzaCodice)
	{
		return TEXT("Non riesco a creare l'invito: il generatore di numeri casuali non risponde.");
	}
	OutCodice = FString::Printf(TEXT("VALD-%s-%s-%s"), *Grezzo.Mid(0, 4), *Grezzo.Mid(4, 4), *Grezzo.Mid(8, 4));

	FValdorsoInvito Invito;
	Invito.ImprontaCodice = ValdorsoSicurezza::Sha256Esadecimale(Grezzo);
	Invito.Indizio = TEXT("VALD-") + Grezzo.Left(4);
	Invito.CreatoDa = Autore;
	Invito.CreatoIl = Ora;
	Invito.ScadeIl = Ora + static_cast<int64>(Giorni) * 24 * 60 * 60;
	Invito.Nota = Nota;
	Inviti.Inviti.Add(Invito);
	SalvaInviti();

	Annota(FString::Printf(TEXT("INVITO_CREATO | %s | da %s | scade %s | %s"), *Invito.Indizio, *Autore, *Data(Invito.ScadeIl), *Nota));
	return FString::Printf(TEXT("Invito creato: %s\nVale una volta sola e scade il %s. Copialo adesso: il server ne tiene solo l'impronta."),
		*OutCodice, *Data(Invito.ScadeIl));
}

FString UValdorsoArchivista::ElencoInviti() const
{
	const int64 Ora = Adesso();
	FString Testo = FString::Printf(TEXT("Inviti: %d\n"), Inviti.Inviti.Num());
	for (const FValdorsoInvito& Invito : Inviti.Inviti)
	{
		FString Stato;
		if (!Invito.UsatoDa.IsEmpty())
		{
			Stato = FString::Printf(TEXT("usato da %s il %s"), *Invito.UsatoDa, *Data(Invito.UsatoIl));
		}
		else if (Invito.bRevocato)
		{
			Stato = TEXT("revocato");
		}
		else if (Invito.ScadeIl > 0 && Invito.ScadeIl <= Ora)
		{
			Stato = TEXT("scaduto");
		}
		else
		{
			Stato = FString::Printf(TEXT("attivo, scade il %s"), *Data(Invito.ScadeIl));
		}
		Testo += FString::Printf(TEXT("  %s | creato da %s il %s | %s%s%s\n"), *Invito.Indizio, *Invito.CreatoDa, *Data(Invito.CreatoIl),
			*Stato, Invito.Nota.IsEmpty() ? TEXT("") : TEXT(" | "), *Invito.Nota);
	}
	return Testo;
}

FString UValdorsoArchivista::RevocaInvito(const FString& Indizio, const FString& Autore)
{
	const int64 Ora = Adesso();
	FString Cercato = Indizio.TrimStartAndEnd().ToUpper();
	if (!Cercato.StartsWith(TEXT("VALD-")))
	{
		Cercato = TEXT("VALD-") + Cercato;
	}

	TArray<FValdorsoInvito*> Trovati;
	for (FValdorsoInvito& Invito : Inviti.Inviti)
	{
		if (Invito.Indizio == Cercato && Invito.UsatoDa.IsEmpty() && !Invito.bRevocato && (Invito.ScadeIl == 0 || Invito.ScadeIl > Ora))
		{
			Trovati.Add(&Invito);
		}
	}
	if (Trovati.Num() == 0)
	{
		return FString::Printf(TEXT("Nessun invito attivo con indizio %s."), *Cercato);
	}
	if (Trovati.Num() > 1)
	{
		return FString::Printf(TEXT("Ci sono %d inviti attivi con indizio %s: non so quale revocare. Lasciali scadere o chiedi a Claude."), Trovati.Num(), *Cercato);
	}
	Trovati[0]->bRevocato = true;
	SalvaInviti();
	Annota(FString::Printf(TEXT("INVITO_REVOCATO | %s | da %s"), *Cercato, *Autore));
	return FString::Printf(TEXT("Invito %s revocato."), *Cercato);
}

FString UValdorsoArchivista::ElencoAccount() const
{
	TArray<const FValdorsoAccount*> Ordinati;
	for (const TPair<FString, FValdorsoAccount>& Coppia : Account)
	{
		Ordinati.Add(&Coppia.Value);
	}
	Ordinati.Sort([](const FValdorsoAccount& A, const FValdorsoAccount& B) { return A.NomeChiave < B.NomeChiave; });

	FString Testo = FString::Printf(TEXT("Account: %d\n"), Ordinati.Num());
	for (const FValdorsoAccount* Dati : Ordinati)
	{
		Testo += FString::Printf(TEXT("  %s | %s | %s | ultimo accesso %s%s\n"), *Dati->Nome, NomeRuolo(Dati->Ruolo), NomeStato(Dati->Stato),
			*Data(Dati->UltimoAccesso), Collegati.Contains(Dati->Id) ? TEXT(" | collegato") : TEXT(""));
	}
	return Testo;
}

FString UValdorsoArchivista::InfoAccount(const FString& Nome) const
{
	const FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *Nome);
	}
	const int64 Ora = Adesso();
	FString Testo = FString::Printf(TEXT("%s (Id %s)\n"), *Dati->Nome, *Dati->Id);
	Testo += FString::Printf(TEXT("  Ruolo: %s | Stato: %s%s%s\n"), NomeRuolo(Dati->Ruolo), NomeStato(Dati->Stato),
		Dati->MotivoStato.IsEmpty() ? TEXT("") : TEXT(" | motivo: "), *Dati->MotivoStato);
	if (Dati->Stato == EValdorsoStatoAccount::Sospeso)
	{
		Testo += FString::Printf(TEXT("  Sospeso fino al %s\n"), *Data(Dati->SospesoFino));
	}
	Testo += FString::Printf(TEXT("  Creato il %s con l'invito %s di %s\n"), *Data(Dati->CreatoIl), *Dati->InvitoUsato, *Dati->InvitatoDa);
	Testo += FString::Printf(TEXT("  Ultimo accesso: %s | password cambiata il %s%s\n"), *Data(Dati->UltimoAccesso), *Data(Dati->PasswordCambiataIl),
		Dati->bDeveCambiarePassword ? TEXT(" | password temporanea") : TEXT(""));
	Testo += FString::Printf(TEXT("  Tentativi sbagliati: %d%s\n"), Dati->TentativiFalliti,
		Dati->BloccatoFino > Ora ? *FString::Printf(TEXT(" | bloccato ancora %d minuti"), Minuti(Dati->BloccatoFino - Ora)) : TEXT(""));
	Testo += FString::Printf(TEXT("  Codici di recupero: %d rimasti su %d (creati il %s)\n"),
		Dati->CodiciRecupero.Num(), ValdorsoRegole::NumeroCodiciRecupero, *Data(Dati->CodiciCreatiIl));
	Testo += FString::Printf(TEXT("  Personaggi: %d | Impronta: %s, %d giri | file scritto %lld volte\n"),
		Dati->Personaggi.Num(), *Dati->Algoritmo, Dati->Iterazioni, Dati->Versione);
	for (const FString& Nota : Dati->NoteStaff)
	{
		Testo += TEXT("  Nota: ") + Nota + TEXT("\n");
	}
	return Testo;
}

FString UValdorsoArchivista::ImpostaRuolo(const FString& Nome, const FString& Ruolo, const FString& Autore)
{
	FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *Nome);
	}
	const FString R = Ruolo.ToLower();
	EValdorsoRuolo Nuovo;
	if (R == TEXT("giocatore"))
	{
		Nuovo = EValdorsoRuolo::Giocatore;
	}
	else if (R == TEXT("staff"))
	{
		Nuovo = EValdorsoRuolo::Staff;
	}
	else if (R == TEXT("amministratore") || R == TEXT("amministratrice"))
	{
		Nuovo = EValdorsoRuolo::Amministratore;
	}
	else
	{
		return TEXT("Ruolo sconosciuto: scrivi Giocatore, Staff o Amministratore.");
	}
	const EValdorsoRuolo Vecchio = Dati->Ruolo;
	Dati->Ruolo = Nuovo;
	SalvaAccount(*Dati);
	Annota(FString::Printf(TEXT("RUOLO | %s | da %s a %s | da %s"), *Dati->Nome, NomeRuolo(Vecchio), NomeRuolo(Nuovo), *Autore));
	return FString::Printf(TEXT("%s ora è %s."), *Dati->Nome, NomeRuolo(Nuovo));
}

FString UValdorsoArchivista::ReimpostaPassword(const FString& Nome, const FString& Autore)
{
	const FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *Nome);
	}
	const FString Grezza = ValdorsoSicurezza::TestoCasuale(AlfabetoPassword, 12);
	if (Grezza.Len() != 12)
	{
		return TEXT("Non riesco a creare la password: il generatore di numeri casuali non risponde.");
	}
	const FString Temporanea = FString::Printf(TEXT("%s-%s-%s"), *Grezza.Mid(0, 4), *Grezza.Mid(4, 4), *Grezza.Mid(8, 4));
	const FString NomeChiave = Dati->NomeChiave;

	const bool bAccodato = Accoda([this, NomeChiave, Temporanea, Autore]() -> TFunction<void()>
	{
		const FImprontaNuova Nuova = CreaImpronta(Temporanea);
		return [this, NomeChiave, Nuova, Autore]()
		{
			FValdorsoAccount* Trovato = Account.Find(NomeChiave);
			if (!Trovato || !Nuova.bOk)
			{
				UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: reset della password di %s non riuscito."), *NomeChiave);
				return;
			}
			Trovato->Algoritmo = ValdorsoSicurezza::Algoritmo;
			Trovato->Sale = Nuova.Sale;
			Trovato->Impronta = Nuova.Impronta;
			Trovato->Iterazioni = Nuova.Iterazioni;
			Trovato->bDeveCambiarePassword = true;
			AnnullaBiglietto(Trovato->Id);
			Trovato->TentativiFalliti = 0;
			Trovato->BlocchiDiFila = 0;
			Trovato->BloccatoFino = 0;
			SalvaAccount(*Trovato);
			Annota(FString::Printf(TEXT("PASSWORD_REIMPOSTATA | %s | da %s"), *Trovato->Nome, *Autore));
		};
	});
	if (!bAccodato)
	{
		return TEXT("Il server è occupato: riprova tra qualche secondo.");
	}
	return FString::Printf(TEXT("Password temporanea per %s: %s\nVale tra un secondo. Al prossimo accesso %s dovrà sceglierne una nuova. Dagliela in privato."),
		*Dati->Nome, *Temporanea, *Dati->Nome);
}

FString UValdorsoArchivista::Sospendi(const FString& Nome, int32 Ore, const FString& Motivo, const FString& Autore)
{
	FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *Nome);
	}
	if (Ore <= 0)
	{
		return TEXT("Scrivi per quante ore sospendere (per esempio 24).");
	}
	Dati->Stato = EValdorsoStatoAccount::Sospeso;
	AnnullaBiglietto(Dati->Id);
	Dati->SospesoFino = Adesso() + static_cast<int64>(Ore) * 60 * 60;
	Dati->MotivoStato = Motivo.IsEmpty() ? TEXT("non indicato") : Motivo;
	SalvaAccount(*Dati);
	Annota(FString::Printf(TEXT("SOSPESO | %s | %d ore | %s | da %s"), *Dati->Nome, Ore, *Dati->MotivoStato, *Autore));
	return FString::Printf(TEXT("%s sospeso fino al %s."), *Dati->Nome, *Data(Dati->SospesoFino));
}

FString UValdorsoArchivista::Banna(const FString& Nome, const FString& Motivo, const FString& Autore)
{
	FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *Nome);
	}
	if (Dati->Ruolo == EValdorsoRuolo::Amministratore)
	{
		return TEXT("Un Amministratore non si può bandire: prima cambiagli il ruolo.");
	}
	Dati->Stato = EValdorsoStatoAccount::Bandito;
	AnnullaBiglietto(Dati->Id);
	Dati->SospesoFino = 0;
	Dati->MotivoStato = Motivo.IsEmpty() ? TEXT("non indicato") : Motivo;
	SalvaAccount(*Dati);
	Annota(FString::Printf(TEXT("BANDITO | %s | %s | da %s"), *Dati->Nome, *Dati->MotivoStato, *Autore));
	return FString::Printf(TEXT("%s bandito. Motivo: %s"), *Dati->Nome, *Dati->MotivoStato);
}

FString UValdorsoArchivista::Riattiva(const FString& Nome, const FString& Autore)
{
	FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *Nome);
	}
	Dati->Stato = EValdorsoStatoAccount::Attivo;
	Dati->SospesoFino = 0;
	Dati->MotivoStato.Empty();
	SalvaAccount(*Dati);
	Annota(FString::Printf(TEXT("RIATTIVATO | %s | da %s"), *Dati->Nome, *Autore));
	return FString::Printf(TEXT("%s di nuovo attivo."), *Dati->Nome);
}

FString UValdorsoArchivista::Sblocca(const FString& Nome, const FString& Autore)
{
	FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *Nome);
	}
	Dati->TentativiFalliti = 0;
	Dati->BlocchiDiFila = 0;
	Dati->BloccatoFino = 0;
	SalvaAccount(*Dati);
	Annota(FString::Printf(TEXT("SBLOCCATO | %s | da %s"), *Dati->Nome, *Autore));
	return FString::Printf(TEXT("%s sbloccato: può riprovare subito."), *Dati->Nome);
}

FString UValdorsoArchivista::AggiungiNota(const FString& Nome, const FString& Nota, const FString& Autore)
{
	FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *Nome);
	}
	if (Nota.TrimStartAndEnd().IsEmpty())
	{
		return TEXT("Scrivi il testo della nota.");
	}
	Dati->NoteStaff.Add(FString::Printf(TEXT("%s, %s: %s"), *Data(Adesso()), *Autore, *Nota.TrimStartAndEnd()));
	SalvaAccount(*Dati);
	Annota(FString::Printf(TEXT("NOTA | %s | da %s"), *Dati->Nome, *Autore));
	return FString::Printf(TEXT("Nota aggiunta a %s."), *Dati->Nome);
}

// ------------------------------------------------------------------------------------------------
// Privacy
// ------------------------------------------------------------------------------------------------

void UValdorsoArchivista::PulisciRegistro()
{
	if (!Scrittore.IsValid())
	{
		return;
	}
	// Sul filo delle scritture, così non si mescola con le righe nuove del registro.
	const FString Percorso = FileRegistro;
	UltimaScrittura = Scrittore->Launch(TEXT("PuliziaRegistro"), [Percorso]()
	{
		FString Testo;
		if (!IFileManager::Get().FileExists(*Percorso) || !FFileHelper::LoadFileToString(Testo, *Percorso))
		{
			return;
		}
		int32 Cambiate = 0;
		const FString Pulito = ValdorsoArchivioFile::OscuraIndirizziVecchi(Testo, FDateTime::UtcNow(), ValdorsoRegole::GiorniIndirizzi, Cambiate);
		if (Cambiate > 0)
		{
			ValdorsoArchivioFile::SostituisciSenzaCopie(Percorso, Pulito);
			UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Archivista: oscurati gli indirizzi IP vecchi in %d righe del registro"), Cambiate);
		}
	});
}

FString UValdorsoArchivista::CancellaAccount(const FString& Nome, const FString& Conferma, const FString& Autore)
{
	check(IsInGameThread());
	const FValdorsoAccount* Dati = TrovaAccount(Nome);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *Nome);
	}
	if (!Conferma.TrimStartAndEnd().Equals(Dati->Nome, ESearchCase::IgnoreCase))
	{
		return FString::Printf(TEXT("Per confermare riscrivi il nome: Valdorso.Account.Cancella %s %s"), *Dati->Nome, *Dati->Nome);
	}
	if (Dati->Ruolo == EValdorsoRuolo::Amministratore)
	{
		return TEXT("Un Amministratore non si può cancellare: prima cambiagli il ruolo.");
	}

	const FString Id = Dati->Id;
	const FString NomeVero = Dati->Nome;
	const FString NomeChiave = Dati->NomeChiave;
	const FString Al = TEXT("[account cancellato]");

	// I suoi personaggi: via i file, i nomi restano riservati 30 giorni (nessuno si spaccia per loro).
	TArray<FString> IdPersonaggi;
	for (const TPair<FString, FValdorsoPersonaggio>& Coppia : Personaggi)
	{
		if (Coppia.Value.AccountId == Id)
		{
			IdPersonaggi.Add(Coppia.Key);
		}
	}
	for (const FString& IdPersonaggio : IdPersonaggi)
	{
		FValdorsoPersonaggio Tolto;
		Personaggi.RemoveAndCopyValue(IdPersonaggio, Tolto);
		const FString Scheletro = ValdorsoRegole::ScheletroNome(Tolto.Nome);
		NomiPersonaggi.Remove(Scheletro);
		FValdorsoNomeRiservato Riservato;
		Riservato.Scheletro = Scheletro;
		Riservato.Fino = Adesso() + static_cast<int64>(ValdorsoRegole::GiorniNomeRiservato) * 24 * 60 * 60;
		NomiRiservati.Nomi.Add(Riservato);
		if (Scrittore.IsValid())
		{
			const FString FilePersonaggio = CartellaPersonaggi / (IdPersonaggio + TEXT(".json"));
			UltimaScrittura = Scrittore->Launch(TEXT("CancellaPersonaggio"), [FilePersonaggio]()
			{
				ValdorsoArchivioFile::CancellaConCopie(FilePersonaggio);
			});
		}
	}
	if (IdPersonaggi.Num() > 0)
	{
		SalvaNomiRiservati();
	}

	// Via dall'archivio in memoria.
	AnnullaBiglietto(Id);
	CodiciInAttesa.Remove(Id);
	Collegati.Remove(Id);
	NomiInCreazione.Remove(NomeChiave);
	Account.Remove(NomeChiave);
	Dati = nullptr;

	// Negli inviti e negli altri account il nome non resta. Si riscrivono senza le copie vecchie, che lo conterrebbero.
	auto SenzaCopieVecchie = [this](const FString& Percorso)
	{
		if (Scrittore.IsValid())
		{
			UltimaScrittura = Scrittore->Launch(TEXT("CopieVecchie"), [Percorso]()
			{
				IFileManager::Get().Delete(*(Percorso + TEXT(".bak1")), false, true, true);
				IFileManager::Get().Delete(*(Percorso + TEXT(".bak2")), false, true, true);
			});
		}
	};
	bool bInviti = false;
	for (FValdorsoInvito& Invito : Inviti.Inviti)
	{
		if (Invito.UsatoDa.Equals(NomeVero, ESearchCase::IgnoreCase))
		{
			Invito.UsatoDa = Al;
			bInviti = true;
		}
		if (Invito.CreatoDa.Equals(NomeVero, ESearchCase::IgnoreCase))
		{
			Invito.CreatoDa = Al;
			bInviti = true;
		}
	}
	if (bInviti)
	{
		SalvaInviti();
		SenzaCopieVecchie(FileInviti);
	}
	for (TPair<FString, FValdorsoAccount>& Coppia : Account)
	{
		if (Coppia.Value.InvitatoDa.Equals(NomeVero, ESearchCase::IgnoreCase))
		{
			Coppia.Value.InvitatoDa = Al;
			SalvaAccount(Coppia.Value);
			SenzaCopieVecchie(CartellaAccount / (Coppia.Value.Id + TEXT(".json")));
		}
	}

	// Il file dell'account con le copie, e il nome nel registro.
	if (Scrittore.IsValid())
	{
		const FString FileAccount = CartellaAccount / (Id + TEXT(".json"));
		const FString Registro = FileRegistro;
		UltimaScrittura = Scrittore->Launch(TEXT("CancellaAccount"), [FileAccount, Registro, NomeVero, Al]()
		{
			ValdorsoArchivioFile::CancellaConCopie(FileAccount);
			FString Testo;
			if (FFileHelper::LoadFileToString(Testo, *Registro))
			{
				int32 Cambiate = 0;
				const FString Pulito = ValdorsoArchivioFile::SostituisciNome(Testo, NomeVero, Al, Cambiate);
				if (Cambiate > 0)
				{
					ValdorsoArchivioFile::SostituisciSenzaCopie(Registro, Pulito);
				}
			}
		});
	}
	Annota(FString::Printf(TEXT("ACCOUNT_CANCELLATO | id %s | da %s"), *Id, *Autore));
	return FString::Printf(TEXT("Account %s cancellato per sempre (file, copie, biglietto; nel registro e negli inviti il nome è stato tolto)."), *NomeVero);
}

// ------------------------------------------------------------------------------------------------
// Codici di recupero
// ------------------------------------------------------------------------------------------------

TArray<FString> UValdorsoArchivista::CreaCodiciRecupero(const FValdorsoAccount& Dati)
{
	TArray<FString> DaMostrare;
	TArray<uint8> Sale;
	if (!ValdorsoSicurezza::BytesCasuali(Sale, ValdorsoSicurezza::LunghezzaSale))
	{
		return DaMostrare;
	}
	const FString SaleTesto = FBase64::Encode(Sale);
	TArray<FString> Impronte;
	for (int32 i = 0; i < ValdorsoRegole::NumeroCodiciRecupero; ++i)
	{
		const FString Grezzo = ValdorsoSicurezza::TestoCasuale(AlfabetoCodici, ValdorsoRegole::LunghezzaCodiceRecupero);
		if (Grezzo.Len() != ValdorsoRegole::LunghezzaCodiceRecupero)
		{
			return TArray<FString>();
		}
		Impronte.Add(ValdorsoRegole::ImprontaCodiceRecupero(Grezzo, SaleTesto));
		DaMostrare.Add(ValdorsoRegole::FormaCodiceRecupero(Grezzo));
	}
	FCodiciInAttesa& DaConfermare = CodiciInAttesa.FindOrAdd(Dati.Id);
	DaConfermare.Sale = SaleTesto;
	DaConfermare.Impronte = MoveTemp(Impronte);
	return DaMostrare;
}

void UValdorsoArchivista::ConfermaCodiciRecupero(const FString& AccountId)
{
	check(IsInGameThread());
	FCodiciInAttesa DaConfermare;
	if (!CodiciInAttesa.RemoveAndCopyValue(AccountId, DaConfermare))
	{
		return;
	}
	for (TPair<FString, FValdorsoAccount>& Coppia : Account)
	{
		if (Coppia.Value.Id == AccountId)
		{
			Coppia.Value.SaleRecupero = DaConfermare.Sale;
			Coppia.Value.CodiciRecupero = MoveTemp(DaConfermare.Impronte);
			Coppia.Value.CodiciCreatiIl = Adesso();
			SalvaAccount(Coppia.Value);
			Annota(FString::Printf(TEXT("CODICI_RECUPERO_CREATI | %s"), *Coppia.Value.Nome));
			return;
		}
	}
}

void UValdorsoArchivista::RecuperaConCodice(const FString& Nome, const FString& Codice, const FString& Nuova, const FString& Indirizzo, FRisposta Risposta)
{
	check(IsInGameThread());
	const int64 Ora = Adesso();
	int64 Rimasti = 0;
	if (IndirizzoBloccato(Indirizzo, Ora, Rimasti))
	{
		Risposta(TroppiTentativi(Rimasti));
		return;
	}

	const FString NomeChiave = Chiave(Nome).Left(32);
	FValdorsoAccount* Dati = Account.Find(NomeChiave);
	if (Dati && Dati->BloccatoFino > Ora)
	{
		Risposta(TroppiTentativi(Dati->BloccatoFino - Ora));
		return;
	}
	const FString Problema = ProblemaPassword(Nuova, Nome);
	if (!Problema.IsEmpty())
	{
		Risposta(Esito(EValdorsoEsitoAccount::PasswordDebole, Problema));
		return;
	}

	// Si cerca il codice tra tutti, confrontandoli tutti (lo stesso tempo, che ci sia o no).
	int32 Trovato = INDEX_NONE;
	// Anche per un nome che non esiste si calcola un'impronta: la risposta arriva negli stessi tempi.
	const FString Cercata = ValdorsoRegole::ImprontaCodiceRecupero(Codice, Dati && !Dati->SaleRecupero.IsEmpty() ? Dati->SaleRecupero : FBase64::Encode(SaleFinto));
	if (Dati && !Dati->SaleRecupero.IsEmpty())
	{
		const FTCHARToUTF8 CercataUtf8(*Cercata);
		const TArray<uint8> CercataByte(reinterpret_cast<const uint8*>(CercataUtf8.Get()), CercataUtf8.Length());
		for (int32 i = 0; i < Dati->CodiciRecupero.Num(); ++i)
		{
			const FTCHARToUTF8 Utf8(*Dati->CodiciRecupero[i]);
			const TArray<uint8> Byte(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
			if (ValdorsoSicurezza::UgualiTempoCostante(Byte, CercataByte) && Trovato == INDEX_NONE)
			{
				Trovato = i;
			}
		}
	}
	if (Trovato == INDEX_NONE)
	{
		ErroreDaIndirizzo(Indirizzo, Ora);
		if (Dati)
		{
			ErroreSullAccount(*Dati, Indirizzo, Ora);
		}
		Annota(FString::Printf(TEXT("RECUPERO_SBAGLIATO | %s | ip %s"), *NomeChiave, *Indirizzo));
		Risposta(Esito(EValdorsoEsitoAccount::CredenzialiSbagliate, TEXT("Nome o codice di recupero sbagliati.")));
		return;
	}
	if (Dati->Stato == EValdorsoStatoAccount::Bandito)
	{
		Risposta(Esito(EValdorsoEsitoAccount::AccountBandito,
			FString::Printf(TEXT("Questo account è stato bandito dalla valle. Motivo: %s"), *Dati->MotivoStato)));
		return;
	}
	if (Dati->Stato == EValdorsoStatoAccount::Sospeso && Dati->SospesoFino > Ora)
	{
		Risposta(Esito(EValdorsoEsitoAccount::AccountSospeso,
			FString::Printf(TEXT("Questo account è sospeso fino al %s. Motivo: %s"), *Data(Dati->SospesoFino), *Dati->MotivoStato)));
		return;
	}

	// Il codice si consuma subito (così non si usa due volte nello stesso istante); se il calcolo fallisce, torna al suo posto.
	const FString CodiceUsato = Dati->CodiciRecupero[Trovato];
	Dati->CodiciRecupero.RemoveAt(Trovato);

	const bool bAccodato = Accoda([this, NomeChiave, Nuova, Indirizzo, Risposta, CodiceUsato]() -> TFunction<void()>
	{
		const FImprontaNuova NuovaImpronta = CreaImpronta(Nuova);
		return [this, NomeChiave, Indirizzo, Risposta, CodiceUsato, NuovaImpronta]()
		{
			FValdorsoAccount* Aggiornato = Account.Find(NomeChiave);
			if (!Aggiornato)
			{
				Risposta(ErroreInterno());
				return;
			}
			if (!NuovaImpronta.bOk)
			{
				Aggiornato->CodiciRecupero.Add(CodiceUsato);
				SalvaAccount(*Aggiornato);
				Risposta(ErroreInterno());
				return;
			}
			const int64 Adesso_ = Adesso();
			Aggiornato->Algoritmo = ValdorsoSicurezza::Algoritmo;
			Aggiornato->Sale = NuovaImpronta.Sale;
			Aggiornato->Impronta = NuovaImpronta.Impronta;
			Aggiornato->Iterazioni = NuovaImpronta.Iterazioni;
			Aggiornato->bDeveCambiarePassword = false;
			Aggiornato->TentativiFalliti = 0;
			Aggiornato->BlocchiDiFila = 0;
			Aggiornato->BloccatoFino = 0;
			Aggiornato->PasswordCambiataIl = Adesso_;
			Aggiornato->UltimoAccesso = Adesso_;
			if (Aggiornato->Stato == EValdorsoStatoAccount::Sospeso && Aggiornato->SospesoFino <= Adesso_)
			{
				Aggiornato->Stato = EValdorsoStatoAccount::Attivo;
				Aggiornato->MotivoStato.Empty();
			}
			AnnullaBiglietto(Aggiornato->Id);

			FValdorsoEsitoAccount Risultato;
			Risultato.Esito = EValdorsoEsitoAccount::Ok;
			Risultato.AccountId = Aggiornato->Id;
			Risultato.Nome = Aggiornato->Nome;
			Risultato.Ruolo = Aggiornato->Ruolo;
			Risultato.CodiciRimasti = Aggiornato->CodiciRecupero.Num();
			if (Aggiornato->CodiciRecupero.Num() == 0)
			{
				// Finiti: subito 8 nuovi, da scrivere (valgono quando il giocatore conferma).
				Risultato.CodiciRecupero = CreaCodiciRecupero(*Aggiornato);
			}
			SalvaAccount(*Aggiornato);
			Annota(FString::Printf(TEXT("RECUPERO | %s | %d codici rimasti | ip %s"), *Aggiornato->Nome, Aggiornato->CodiciRecupero.Num(), *Indirizzo));
			Risultato.Messaggio = FString::Printf(TEXT("Password cambiata. Bentornato nella valle, %s. Codici di recupero rimasti: %d."),
				*Aggiornato->Nome, Aggiornato->CodiciRecupero.Num());
			Risposta(Risultato);
		};
	});
	if (!bAccodato)
	{
		Dati->CodiciRecupero.Insert(CodiceUsato, Trovato);
		Risposta(ServerOccupato());
	}
}

// ------------------------------------------------------------------------------------------------
// Personaggi (v0.1.2, passo 3)
// ------------------------------------------------------------------------------------------------

const FValdorsoAccount* UValdorsoArchivista::AccountPerId(const FString& AccountId) const
{
	for (const TPair<FString, FValdorsoAccount>& Coppia : Account)
	{
		if (Coppia.Value.Id == AccountId)
		{
			return &Coppia.Value;
		}
	}
	return nullptr;
}

FValdorsoAccount* UValdorsoArchivista::AccountPerId(const FString& AccountId)
{
	return const_cast<FValdorsoAccount*>(static_cast<const UValdorsoArchivista*>(this)->AccountPerId(AccountId));
}

bool UValdorsoArchivista::SomigliaAlloStaff(const FString& Scheletro, const FString& AccountId) const
{
	for (const TPair<FString, FValdorsoAccount>& Coppia : Account)
	{
		const FValdorsoAccount& Membro = Coppia.Value;
		if (Membro.Ruolo == EValdorsoRuolo::Giocatore || Membro.Id == AccountId)
		{
			continue;
		}
		if (ValdorsoRegole::ScheletroNome(Membro.Nome) == Scheletro)
		{
			return true;
		}
		for (const FString& IdPersonaggio : Membro.Personaggi)
		{
			const FValdorsoPersonaggio* DelloStaff = Personaggi.Find(IdPersonaggio);
			if (DelloStaff && ValdorsoRegole::ScheletroNome(DelloStaff->Nome) == Scheletro)
			{
				return true;
			}
		}
	}
	return false;
}

TArray<FValdorsoPersonaggioBreve> UValdorsoArchivista::ElencoPersonaggi(const FString& AccountId) const
{
	TArray<FValdorsoPersonaggioBreve> Elenco;
	const FValdorsoAccount* Proprietario = AccountPerId(AccountId);
	if (!Proprietario)
	{
		return Elenco;
	}
	for (const FString& IdPersonaggio : Proprietario->Personaggi)
	{
		if (const FValdorsoPersonaggio* Dati = Personaggi.Find(IdPersonaggio))
		{
			FValdorsoPersonaggioBreve Breve;
			Breve.Id = Dati->Id;
			Breve.Nome = Dati->Nome;
			Breve.UltimoGioco = Dati->UltimoGioco;
			Breve.TempoDiGioco = Dati->TempoDiGioco;
			Breve.bRegistroFirmato = Dati->Registro.bFirmato;
			Breve.Sesso = Dati->Registro.Sesso;
			Breve.Fede = Dati->Registro.Fede;
			Elenco.Add(Breve);
		}
	}
	return Elenco;
}

FString UValdorsoArchivista::CreaPersonaggio(const FString& AccountId, const FString& NomeScritto, FString& OutId)
{
	check(IsInGameThread());
	FValdorsoAccount* Proprietario = AccountPerId(AccountId);
	if (!Proprietario)
	{
		return TEXT("Account non trovato: ricollegati.");
	}
	// Si contano solo i personaggi che esistono davvero (un Id rimasto senza file non occupa un posto).
	Proprietario->Personaggi.RemoveAll([this](const FString& IdPersonaggio) { return !Personaggi.Contains(IdPersonaggio); });
	if (Proprietario->Personaggi.Num() >= ValdorsoRegole::PersonaggiPerAccount)
	{
		return FString::Printf(TEXT("Hai già %d personaggi: per crearne un altro devi prima cancellarne uno."), ValdorsoRegole::PersonaggiPerAccount);
	}

	const FString NomeNuovo = ValdorsoRegole::NormalizzaNomePersonaggio(NomeScritto);
	const FString Problema = ValdorsoRegole::ProblemaNomePersonaggio(NomeNuovo);
	if (!Problema.IsEmpty())
	{
		return Problema;
	}
	const FString Scheletro = ValdorsoRegole::ScheletroNome(NomeNuovo);
	if (NomiPersonaggi.Contains(Scheletro))
	{
		return TEXT("Nella valle c'è già qualcuno con questo nome (o uno che gli somiglia troppo): scegline un altro.");
	}
	const int64 Ora = Adesso();
	bool bRiservatiCambiati = false;
	for (int32 i = NomiRiservati.Nomi.Num() - 1; i >= 0; --i)
	{
		if (NomiRiservati.Nomi[i].Fino <= Ora)
		{
			NomiRiservati.Nomi.RemoveAt(i);
			bRiservatiCambiati = true;
		}
		else if (NomiRiservati.Nomi[i].Scheletro == Scheletro)
		{
			return TEXT("Questo nome è stato usato da poco da un personaggio che non c'è più: per ora è riservato, scegline un altro.");
		}
	}
	if (bRiservatiCambiati)
	{
		SalvaNomiRiservati();
	}
	if (SomigliaAlloStaff(Scheletro, AccountId))
	{
		return TEXT("Questo nome somiglia troppo a quello di un membro dello staff: scegline un altro.");
	}

	FValdorsoPersonaggio Nuovo;
	Nuovo.Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Nuovo.AccountId = AccountId;
	Nuovo.Nome = NomeNuovo;
	Nuovo.CreatoIl = Ora;
	OutId = Nuovo.Id;

	FValdorsoPersonaggio& Salvato = Personaggi.Add(Nuovo.Id, MoveTemp(Nuovo));
	NomiPersonaggi.Add(Scheletro, Salvato.Id);
	Proprietario->Personaggi.Add(Salvato.Id);
	SalvaAccount(*Proprietario);
	SalvaPersonaggio(Salvato, false);
	Annota(FString::Printf(TEXT("PERSONAGGIO_CREATO | %s | %s"), *Proprietario->Nome, *Salvato.Nome));
	return FString();
}

FString UValdorsoArchivista::CancellaPersonaggio(const FString& AccountId, const FString& PersonaggioId, const FString& Conferma, const FString& Autore)
{
	check(IsInGameThread());
	FValdorsoAccount* Proprietario = AccountPerId(AccountId);
	const FValdorsoPersonaggio* Dati = Personaggi.Find(PersonaggioId);
	if (!Proprietario || !Dati || Dati->AccountId != AccountId)
	{
		return TEXT("Personaggio non trovato.");
	}
	if (!Conferma.TrimStartAndEnd().Equals(Dati->Nome, ESearchCase::IgnoreCase))
	{
		return FString::Printf(TEXT("Per cancellare %s scrivi il suo nome esatto."), *Dati->Nome);
	}

	const FString NomeTolto = Dati->Nome;
	const FString Scheletro = ValdorsoRegole::ScheletroNome(NomeTolto);
	Personaggi.Remove(PersonaggioId);
	Dati = nullptr;
	NomiPersonaggi.Remove(Scheletro);
	Proprietario->Personaggi.Remove(PersonaggioId);
	if (Proprietario->UltimoPersonaggio == PersonaggioId)
	{
		Proprietario->UltimoPersonaggio.Empty();
	}
	SalvaAccount(*Proprietario);

	FValdorsoNomeRiservato Riservato;
	Riservato.Scheletro = Scheletro;
	Riservato.Fino = Adesso() + static_cast<int64>(ValdorsoRegole::GiorniNomeRiservato) * 24 * 60 * 60;
	NomiRiservati.Nomi.Add(Riservato);
	SalvaNomiRiservati();

	if (Scrittore.IsValid())
	{
		const FString FilePersonaggio = CartellaPersonaggi / (PersonaggioId + TEXT(".json"));
		UltimaScrittura = Scrittore->Launch(TEXT("CancellaPersonaggio"), [FilePersonaggio]()
		{
			ValdorsoArchivioFile::CancellaConCopie(FilePersonaggio);
		});
	}
	Annota(FString::Printf(TEXT("PERSONAGGIO_CANCELLATO | %s | %s | da %s"), *Proprietario->Nome, *NomeTolto, *Autore));
	return FString();
}

FValdorsoPersonaggio* UValdorsoArchivista::TrovaPersonaggio(const FString& AccountId, const FString& PersonaggioId)
{
	FValdorsoPersonaggio* Dati = Personaggi.Find(PersonaggioId);
	return Dati && Dati->AccountId == AccountId ? Dati : nullptr;
}

FString UValdorsoArchivista::UltimoPersonaggio(const FString& AccountId) const
{
	const FValdorsoAccount* Proprietario = AccountPerId(AccountId);
	if (!Proprietario || !Personaggi.Contains(Proprietario->UltimoPersonaggio))
	{
		return FString();
	}
	return Proprietario->UltimoPersonaggio;
}

FString UValdorsoArchivista::ElencoPersonaggiTesto(const FString& NomeAccount) const
{
	const FValdorsoAccount* Proprietario = TrovaAccount(NomeAccount);
	if (!Proprietario)
	{
		return FString::Printf(TEXT("Nessun account di nome %s."), *NomeAccount);
	}
	FString Testo = FString::Printf(TEXT("Personaggi di %s (%d su %d):\n"), *Proprietario->Nome, Proprietario->Personaggi.Num(), ValdorsoRegole::PersonaggiPerAccount);
	for (const FValdorsoPersonaggioBreve& Breve : ElencoPersonaggi(Proprietario->Id))
	{
		Testo += FString::Printf(TEXT("  %s | creato %s | ultimo gioco %s | %lld minuti di gioco\n"),
			*Breve.Nome, *Data(Personaggi.FindChecked(Breve.Id).CreatoIl), *Data(Breve.UltimoGioco), Breve.TempoDiGioco / 60);
	}
	return Testo;
}

// ------------------------------------------------------------------------------------------------
// Il Registro di Val d'Orso (v0.1.2, passo 4.1)
// ------------------------------------------------------------------------------------------------

FString UValdorsoArchivista::SalvaRegistro(const FString& AccountId, const FString& PersonaggioId, const FValdorsoRegistro& Proposto, bool bFirma, FValdorsoRegistro& OutSalvato)
{
	check(IsInGameThread());
	FValdorsoPersonaggio* Dati = TrovaPersonaggio(AccountId, PersonaggioId);
	if (!Dati)
	{
		return TEXT("Personaggio non trovato.");
	}
	OutSalvato = Dati->Registro;
	if (Dati->Registro.bFirmato)
	{
		return TEXT("Il registro di questo personaggio è già firmato.");
	}

	// Si prendono solo le risposte: firma e data le decide il server.
	FValdorsoRegistro Nuovo = Proposto;
	Nuovo.bFirmato = false;
	Nuovo.FirmatoIl = 0;
	Nuovo.Racconto = Nuovo.Racconto.TrimStartAndEnd();
	Nuovo.Storia = Nuovo.Storia.TrimStartAndEnd();
	if (bFirma && Nuovo.Racconto.IsEmpty())
	{
		Nuovo.Racconto = ValdorsoRegistro::ComponiRacconto(Nuovo, Dati->Nome);
	}
	const FString Errore = ValdorsoRegistro::Problema(Nuovo, bFirma);
	if (!Errore.IsEmpty())
	{
		return Errore;
	}
	if (bFirma)
	{
		Nuovo.bFirmato = true;
		Nuovo.FirmatoIl = Adesso();
	}
	Dati->Registro = Nuovo;
	SalvaPersonaggio(*Dati, false);
	if (bFirma)
	{
		Annota(FString::Printf(TEXT("REGISTRO_FIRMATO | %s"), *Dati->Nome));
	}
	OutSalvato = Dati->Registro;
	return FString();
}

FValdorsoPersonaggio* UValdorsoArchivista::PersonaggioPerNome(const FString& Nome)
{
	const FString* Id = NomiPersonaggi.Find(ValdorsoRegole::ScheletroNome(Nome));
	return Id ? Personaggi.Find(*Id) : nullptr;
}

FString UValdorsoArchivista::RegistroTesto(const FString& NomePersonaggio)
{
	const FValdorsoPersonaggio* Dati = PersonaggioPerNome(NomePersonaggio);
	if (!Dati)
	{
		return FString::Printf(TEXT("Nessun personaggio di nome %s."), *NomePersonaggio);
	}
	const FValdorsoRegistro& R = Dati->Registro;
	FString Righe = FString::Printf(TEXT("%s | registro %s\n"), *Dati->Nome,
		R.bFirmato ? *FString::Printf(TEXT("firmato il %s"), *Data(R.FirmatoIl)) : TEXT("non ancora firmato"));
	Righe += FString::Printf(TEXT("  Età: %d\n"), R.Eta);
	for (int32 i = 0; i < static_cast<int32>(ValdorsoRegistro::EDomanda::Numero); ++i)
	{
		const ValdorsoRegistro::EDomanda Quale = static_cast<ValdorsoRegistro::EDomanda>(i);
		const FString Scelta = ValdorsoRegistro::Testo(Quale, ValdorsoRegistro::Risposta(R, Quale), R.Sesso);
		Righe += FString::Printf(TEXT("  %s %s\n"), ValdorsoRegistro::Domanda(Quale), Scelta.IsEmpty() ? TEXT("-") : *Scelta);
	}
	Righe += TEXT("  Racconto: ") + (R.Racconto.IsEmpty() ? FString(TEXT("-")) : R.Racconto) + TEXT("\n");
	Righe += TEXT("  La sua storia: ") + (R.Storia.IsEmpty() ? FString(TEXT("-")) : R.Storia) + TEXT("\n");
	return Righe;
}

