// Valdorso - L'archivista (vedi il .h).

#include "ValdorsoArchivista.h"
#include "ValdorsoSicurezza.h"
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
	constexpr int32 TentativiPrimaDelBlocco = 5;
	constexpr int64 BloccoBaseSecondi = 15 * 60;
	constexpr int64 BloccoMassimoSecondi = 24 * 60 * 60;
	constexpr int32 ErroriIndirizzoMassimi = 20;
	constexpr int64 FinestraIndirizzoSecondi = 10 * 60;
	constexpr int64 BloccoIndirizzoSecondi = 30 * 60;
	constexpr int32 CalcoliInsieme = 4;
	constexpr int32 CodaMassima = 64;
	constexpr int32 GiorniInvitoPredefiniti = 14;
	constexpr int32 LunghezzaCodice = 12;

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

	bool NomeValido(const FString& Nome)
	{
		if (Nome.Len() < 3 || Nome.Len() > 20)
		{
			return false;
		}
		for (int32 i = 0; i < Nome.Len(); ++i)
		{
			const TCHAR C = Nome[i];
			const bool bLettera = (C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('A') && C <= TEXT('Z'));
			const bool bCifra = C >= TEXT('0') && C <= TEXT('9');
			const bool bSegno = C == TEXT('.') || C == TEXT('-') || C == TEXT('_');
			if ((i == 0 && !bLettera) || (!bLettera && !bCifra && !bSegno))
			{
				return false;
			}
		}
		return true;
	}

	bool NomeRiservato(const FString& NomeChiave)
	{
		static const TCHAR* const Riservati[] = {
			TEXT("admin"), TEXT("administrator"), TEXT("amministratore"), TEXT("amministratrice"), TEXT("staff"),
			TEXT("gm"), TEXT("moderatore"), TEXT("narratore"), TEXT("valdorso"), TEXT("sistema"), TEXT("system"),
			TEXT("server"), TEXT("root"), TEXT("console"), TEXT("supporto"), TEXT("support") };
		for (const TCHAR* Riservato : Riservati)
		{
			if (NomeChiave == Riservato)
			{
				return true;
			}
		}
		return false;
	}

	/** Vuoto se la password va bene, altrimenti il motivo da mostrare al giocatore. */
	FString ProblemaPassword(const FString& Password, const FString& Nome)
	{
		if (Password.Len() < 10)
		{
			return TEXT("La password deve avere almeno 10 caratteri.");
		}
		if (Password.Len() > 128)
		{
			return TEXT("La password può avere al massimo 128 caratteri.");
		}
		const FString Minuscola = Password.ToLower();
		const FString NomeChiave = Chiave(Nome);
		if (NomeChiave.Len() >= 3 && Minuscola.Contains(NomeChiave))
		{
			return TEXT("La password non può contenere il nome dell'account.");
		}
		static const TCHAR* const Comuni[] = {
			TEXT("1234567890"), TEXT("12345678910"), TEXT("0123456789"), TEXT("password123"), TEXT("password1234"),
			TEXT("passwordpassword"), TEXT("qwertyuiop"), TEXT("qwertyuiop1"), TEXT("asdfghjkl1"), TEXT("1q2w3e4r5t"),
			TEXT("iloveyou123"), TEXT("valdorso123"), TEXT("valdorso2026"), TEXT("ciaociao123"), TEXT("forzainter"),
			TEXT("forzamilan"), TEXT("forzajuve1"), TEXT("forzaroma1"), TEXT("napoli1926"), TEXT("dragon12345") };
		for (const TCHAR* Comune : Comuni)
		{
			if (Minuscola == Comune)
			{
				return TEXT("Questa password è troppo comune: scegline un'altra.");
			}
		}
		TSet<TCHAR> Diversi;
		for (const TCHAR C : Password)
		{
			Diversi.Add(C);
		}
		if (Diversi.Num() < 5)
		{
			return TEXT("La password ha troppi caratteri ripetuti.");
		}
		return FString();
	}

	/** Toglie trattini, spazi e "VALD" davanti; tutto maiuscolo. */
	FString NormalizzaCodice(const FString& Codice)
	{
		FString Pulito;
		for (const TCHAR C : Codice)
		{
			if (FChar::IsAlnum(C))
			{
				Pulito.AppendChar(FChar::ToUpper(C));
			}
		}
		if (Pulito.Len() == LunghezzaCodice + 4 && Pulito.StartsWith(TEXT("VALD")))
		{
			Pulito.RightChopInline(4);
		}
		return Pulito;
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

	/** Scrive il file in modo che non resti mai a metà: prima un .tmp, poi le copie, poi la sostituzione. Gira sul filo delle scritture. */
	void ScriviAtomico(const FString& Percorso, const FString& Testo)
	{
		IFileManager& File = IFileManager::Get();
		const FString Temporaneo = Percorso + TEXT(".tmp");
		if (!FFileHelper::SaveStringToFile(Testo, *Temporaneo, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a scrivere %s"), *Temporaneo);
			return;
		}
		if (File.FileExists(*Percorso))
		{
			const FString Copia1 = Percorso + TEXT(".bak1");
			const FString Copia2 = Percorso + TEXT(".bak2");
			if (File.FileExists(*Copia1))
			{
				File.Copy(*Copia2, *Copia1, true, true);
			}
			File.Copy(*Copia1, *Percorso, true, true);
		}
		if (!File.Move(*Percorso, *Temporaneo, true, true))
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a sostituire %s (resta il .tmp)"), *Percorso);
		}
	}

	/** Legge un file dell'archivio; se è rovinato prova le due copie precedenti. */
	template <typename TipoDati>
	bool LeggiFile(const FString& Percorso, TipoDati& Out)
	{
		const TArray<FString> Prove = { Percorso, Percorso + TEXT(".bak1"), Percorso + TEXT(".bak2") };
		for (const FString& Prova : Prove)
		{
			FString Testo;
			TipoDati Letto;
			if (FFileHelper::LoadFileToString(Testo, *Prova) && FJsonObjectConverter::JsonObjectStringToUStruct(Testo, &Letto, 0, 0))
			{
				if (Prova != Percorso)
				{
					UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Archivista: %s era rovinato, letto dalla copia %s"), *Percorso, *Prova);
				}
				Out = MoveTemp(Letto);
				return true;
			}
		}
		return false;
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
}

void UValdorsoArchivista::Deinitialize()
{
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
		const int64 Durata = FMath::Min<int64>(BloccoBaseSecondi << FMath::Clamp(Dati.BlocchiDiFila - 1, 0, 10), BloccoMassimoSecondi);
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
	SalvaAccount(Salvato);
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
