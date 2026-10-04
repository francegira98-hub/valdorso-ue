// Valdorso - I comandi dell'archivista per la console del server.
// Scritto da Claude il 03/10/2026 (v0.1.2, passo 2.1).
//
// Si scrivono nella console (tasto \ sotto Esc, sulla tastiera italiana) di una partita avviata come server o "autonoma":
//   Valdorso.Invito.Crea [giorni] [nota]          crea un codice d'invito (14 giorni se non si dice altro)
//   Valdorso.Invito.Elenco                        tutti gli inviti, usati e no
//   Valdorso.Invito.Revoca <indizio>              per esempio Valdorso.Invito.Revoca VALD-7K3P
//   Valdorso.Account.Elenco
//   Valdorso.Account.Info <nome>
//   Valdorso.Account.Ruolo <nome> <Giocatore|Staff|Amministratore>
//   Valdorso.Account.ReimpostaPassword <nome>     dà una password temporanea da cambiare al primo accesso
//   Valdorso.Account.Sospendi <nome> <ore> <motivo>
//   Valdorso.Account.Banna <nome> <motivo>
//   Valdorso.Account.Riattiva <nome>
//   Valdorso.Account.Sblocca <nome>               toglie il blocco per troppi tentativi
//   Valdorso.Account.Nota <nome> <testo>          nota dello staff, i giocatori non la vedono
//   Valdorso.Account.Cancella <nome> <nome>       cancella l'account per sempre (il nome due volte, per conferma)
//   Valdorso.Account.Personaggi <nome>            i personaggi dell'account
// Solo nelle versioni di sviluppo (mai nel gioco pubblicato), per provare senza schermata:
//   Valdorso.Prova.Crea <codice> <nome> <password>
//   Valdorso.Prova.Entra <nome> <password>
//   Valdorso.Prova.Cambia <nome> <attuale> <nuova>
// Attenzione: la console ricorda i comandi scritti, quindi nei comandi di prova si usano solo password di prova.

#include "ValdorsoArchivista.h"
#include "ValdorsoPlayerController.h"
#include "Valdorso.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/OutputDevice.h"

namespace
{
	const TCHAR* const AutoreConsole = TEXT("console del server");

	UValdorsoArchivista* TrovaArchivista(UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (!Mondo || !Mondo->GetGameInstance())
		{
			Uscita.Log(TEXT("[Valdorso] Archivista: avvia prima il gioco (Gioca, come autonomo) o il server, poi riscrivi il comando."));
			return nullptr;
		}
		if (Mondo->GetNetMode() == NM_Client)
		{
			Uscita.Log(TEXT("[Valdorso] Archivista: questi comandi si usano nella console del server, non in quella di un giocatore."));
			return nullptr;
		}
		UValdorsoArchivista* Archivista = Mondo->GetGameInstance()->GetSubsystem<UValdorsoArchivista>();
		if (!Archivista)
		{
			Uscita.Log(TEXT("[Valdorso] Archivista: non c'è (questo gioco non fa da server)."));
		}
		return Archivista;
	}

	/** Gli argomenti dal numero Da in poi, uniti con uno spazio (per motivi e note). */
	FString Unisci(const TArray<FString>& Argomenti, int32 Da)
	{
		FString Testo;
		for (int32 i = Da; i < Argomenti.Num(); ++i)
		{
			Testo += (i > Da ? TEXT(" ") : TEXT("")) + Argomenti[i];
		}
		return Testo;
	}

	void Scrivi(FOutputDevice& Uscita, const FString& Testo)
	{
		TArray<FString> Righe;
		Testo.ParseIntoArrayLines(Righe, false);
		for (const FString& Riga : Righe)
		{
			Uscita.Log(Riga);
		}
	}

	/** Se l'account è collegato, lo scollega con il motivo. */
	void ScollegaSeCollegato(UWorld* Mondo, UValdorsoArchivista* Archivista, const FString& Nome, const FString& Motivo, FOutputDevice& Uscita)
	{
		const FString Id = Archivista->IdDi(Nome);
		if (Id.IsEmpty() || !Mondo)
		{
			return;
		}
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(It->Get());
			if (Controllore && Controllore->GetAccountId() == Id)
			{
				Controllore->Espelli(Motivo);
				Uscita.Logf(TEXT("%s era collegato: scollegato."), *Nome);
			}
		}
	}

	bool Servono(const TArray<FString>& Argomenti, int32 Quanti, const TCHAR* Uso, FOutputDevice& Uscita)
	{
		if (Argomenti.Num() < Quanti)
		{
			Uscita.Logf(TEXT("Uso: %s"), Uso);
			return false;
		}
		return true;
	}

	// --- Inviti ------------------------------------------------------------------------------------

	void InvitoCrea(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita))
		{
			const int32 Giorni = Argomenti.Num() > 0 ? FCString::Atoi(*Argomenti[0]) : 0;
			FString Codice;
			Scrivi(Uscita, A->CreaInvito(Giorni, AutoreConsole, Unisci(Argomenti, 1), Codice));
		}
	}

	void InvitoElenco(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita))
		{
			Scrivi(Uscita, A->ElencoInviti());
		}
	}

	void InvitoRevoca(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 1, TEXT("Valdorso.Invito.Revoca <indizio, per esempio VALD-7K3P>"), Uscita))
		{
			Scrivi(Uscita, A->RevocaInvito(Argomenti[0], AutoreConsole));
		}
	}

	// --- Account -----------------------------------------------------------------------------------

	void AccountElenco(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita))
		{
			Scrivi(Uscita, A->ElencoAccount());
		}
	}

	void AccountInfo(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 1, TEXT("Valdorso.Account.Info <nome>"), Uscita))
		{
			Scrivi(Uscita, A->InfoAccount(Argomenti[0]));
		}
	}

	void AccountRuolo(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 2, TEXT("Valdorso.Account.Ruolo <nome> <Giocatore|Staff|Amministratore>"), Uscita))
		{
			Scrivi(Uscita, A->ImpostaRuolo(Argomenti[0], Argomenti[1], AutoreConsole));
		}
	}

	void AccountReimposta(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 1, TEXT("Valdorso.Account.ReimpostaPassword <nome>"), Uscita))
		{
			Scrivi(Uscita, A->ReimpostaPassword(Argomenti[0], AutoreConsole));
		}
	}

	void AccountSospendi(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 2, TEXT("Valdorso.Account.Sospendi <nome> <ore> <motivo>"), Uscita))
		{
			const FString Risultato = A->Sospendi(Argomenti[0], FCString::Atoi(*Argomenti[1]), Unisci(Argomenti, 2), AutoreConsole);
			Scrivi(Uscita, Risultato);
			if (Risultato.Contains(TEXT("sospeso fino")))
			{
				ScollegaSeCollegato(Mondo, A, Argomenti[0], Risultato, Uscita);
			}
		}
	}

	void AccountBanna(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 1, TEXT("Valdorso.Account.Banna <nome> <motivo>"), Uscita))
		{
			const FString Risultato = A->Banna(Argomenti[0], Unisci(Argomenti, 1), AutoreConsole);
			Scrivi(Uscita, Risultato);
			if (Risultato.Contains(TEXT("bandito")))
			{
				ScollegaSeCollegato(Mondo, A, Argomenti[0], TEXT("Sei stato bandito dalla valle."), Uscita);
			}
		}
	}

	void AccountRiattiva(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 1, TEXT("Valdorso.Account.Riattiva <nome>"), Uscita))
		{
			Scrivi(Uscita, A->Riattiva(Argomenti[0], AutoreConsole));
		}
	}

	void AccountSblocca(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 1, TEXT("Valdorso.Account.Sblocca <nome>"), Uscita))
		{
			Scrivi(Uscita, A->Sblocca(Argomenti[0], AutoreConsole));
		}
	}

	void AccountCancella(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 1, TEXT("Valdorso.Account.Cancella <nome> <nome di nuovo>"), Uscita))
		{
			const FString Id = A->IdDi(Argomenti[0]);
			const FString Risultato = A->CancellaAccount(Argomenti[0], Argomenti.Num() > 1 ? Argomenti[1] : FString(), AutoreConsole);
			Scrivi(Uscita, Risultato);
			if (Risultato.Contains(TEXT("cancellato per sempre")) && !Id.IsEmpty() && Mondo)
			{
				for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
				{
					AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(It->Get());
					if (Controllore && Controllore->GetAccountId() == Id)
					{
						Controllore->Espelli(TEXT("Il tuo account è stato cancellato."));
						Uscita.Log(TEXT("Era collegato: scollegato."));
					}
				}
			}
		}
	}

	void AccountPersonaggi(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 1, TEXT("Valdorso.Account.Personaggi <nome>"), Uscita))
		{
			Scrivi(Uscita, A->ElencoPersonaggiTesto(Argomenti[0]));
		}
	}

	void AccountNota(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 2, TEXT("Valdorso.Account.Nota <nome> <testo>"), Uscita))
		{
			Scrivi(Uscita, A->AggiungiNota(Argomenti[0], Unisci(Argomenti, 1), AutoreConsole));
		}
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoInvitoCrea(TEXT("Valdorso.Invito.Crea"),
		TEXT("Crea un codice d'invito. Valdorso.Invito.Crea [giorni] [nota]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&InvitoCrea));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoInvitoElenco(TEXT("Valdorso.Invito.Elenco"),
		TEXT("Elenca gli inviti."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&InvitoElenco));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoInvitoRevoca(TEXT("Valdorso.Invito.Revoca"),
		TEXT("Revoca un invito non ancora usato. Valdorso.Invito.Revoca <indizio>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&InvitoRevoca));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountElenco(TEXT("Valdorso.Account.Elenco"),
		TEXT("Elenca gli account."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountElenco));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountInfo(TEXT("Valdorso.Account.Info"),
		TEXT("Tutto su un account. Valdorso.Account.Info <nome>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountInfo));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountRuolo(TEXT("Valdorso.Account.Ruolo"),
		TEXT("Cambia il ruolo. Valdorso.Account.Ruolo <nome> <Giocatore|Staff|Amministratore>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountRuolo));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountReimposta(TEXT("Valdorso.Account.ReimpostaPassword"),
		TEXT("Dà una password temporanea. Valdorso.Account.ReimpostaPassword <nome>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountReimposta));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountSospendi(TEXT("Valdorso.Account.Sospendi"),
		TEXT("Sospende un account. Valdorso.Account.Sospendi <nome> <ore> <motivo>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountSospendi));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountBanna(TEXT("Valdorso.Account.Banna"),
		TEXT("Bandisce un account. Valdorso.Account.Banna <nome> <motivo>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountBanna));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountRiattiva(TEXT("Valdorso.Account.Riattiva"),
		TEXT("Toglie sospensione o bando. Valdorso.Account.Riattiva <nome>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountRiattiva));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountSblocca(TEXT("Valdorso.Account.Sblocca"),
		TEXT("Toglie il blocco per troppi tentativi. Valdorso.Account.Sblocca <nome>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountSblocca));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountCancella(TEXT("Valdorso.Account.Cancella"),
		TEXT("Cancella un account per sempre. Valdorso.Account.Cancella <nome> <nome di nuovo>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountCancella));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountPersonaggi(TEXT("Valdorso.Account.Personaggi"),
		TEXT("I personaggi di un account. Valdorso.Account.Personaggi <nome>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountPersonaggi));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoAccountNota(TEXT("Valdorso.Account.Nota"),
		TEXT("Aggiunge una nota dello staff. Valdorso.Account.Nota <nome> <testo>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&AccountNota));

#if !UE_BUILD_SHIPPING
	// --- Prove (solo sviluppo): la risposta arriva dopo il calcolo, nel Registro output ---------

	void Riferisci(const FString& Cosa, const FValdorsoEsitoAccount& Esito)
	{
		UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Prova %s: %s | %s"), *Cosa,
			*StaticEnum<EValdorsoEsitoAccount>()->GetNameStringByValue(static_cast<int64>(Esito.Esito)), *Esito.Messaggio);
	}

	void ProvaCrea(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 3, TEXT("Valdorso.Prova.Crea <codice> <nome> <password>"), Uscita))
		{
			Uscita.Log(TEXT("Calcolo in corso..."));
			A->CreaAccount(Argomenti[0], Argomenti[1], Argomenti[2], TEXT("console"), [](const FValdorsoEsitoAccount& Esito) { Riferisci(TEXT("Crea"), Esito); });
		}
	}

	void ProvaEntra(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 2, TEXT("Valdorso.Prova.Entra <nome> <password>"), Uscita))
		{
			Uscita.Log(TEXT("Calcolo in corso..."));
			const double Inizio = FPlatformTime::Seconds();
			A->Accedi(Argomenti[0], Argomenti[1], TEXT("console"), [Inizio](const FValdorsoEsitoAccount& Esito)
			{
				Riferisci(FString::Printf(TEXT("Entra (%.2f s)"), FPlatformTime::Seconds() - Inizio), Esito);
			});
		}
	}

	void ProvaCambia(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoArchivista* A = TrovaArchivista(Mondo, Uscita); A && Servono(Argomenti, 3, TEXT("Valdorso.Prova.Cambia <nome> <attuale> <nuova>"), Uscita))
		{
			Uscita.Log(TEXT("Calcolo in corso..."));
			A->CambiaPassword(Argomenti[0], Argomenti[1], Argomenti[2], TEXT("console"), [](const FValdorsoEsitoAccount& Esito) { Riferisci(TEXT("Cambia"), Esito); });
		}
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoProvaCrea(TEXT("Valdorso.Prova.Crea"),
		TEXT("Solo sviluppo. Valdorso.Prova.Crea <codice> <nome> <password>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ProvaCrea));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoProvaEntra(TEXT("Valdorso.Prova.Entra"),
		TEXT("Solo sviluppo. Valdorso.Prova.Entra <nome> <password>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ProvaEntra));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoProvaCambia(TEXT("Valdorso.Prova.Cambia"),
		TEXT("Solo sviluppo. Valdorso.Prova.Cambia <nome> <attuale> <nuova>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ProvaCambia));
#endif
}
