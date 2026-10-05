// Valdorso - Le regole degli account e i file dell'archivio (vedi il .h).

#include "ValdorsoRegole.h"
#include "ValdorsoSicurezza.h"
#include "HAL/FileManager.h"

namespace ValdorsoRegole
{
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
		const FString NomeChiave = Nome.TrimStartAndEnd().ToLower();
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

	FString NormalizzaCodiceRecupero(const FString& Codice)
	{
		FString Pulito;
		for (const TCHAR C : Codice)
		{
			if (FChar::IsAlnum(C))
			{
				Pulito.AppendChar(FChar::ToUpper(C));
			}
		}
		return Pulito;
	}

	FString FormaCodiceRecupero(const FString& Grezzo)
	{
		FString Forma;
		for (int32 i = 0; i < Grezzo.Len(); ++i)
		{
			if (i > 0 && i % 4 == 0)
			{
				Forma.AppendChar(TEXT('-'));
			}
			Forma.AppendChar(Grezzo[i]);
		}
		return Forma;
	}

	FString ImprontaCodiceRecupero(const FString& Codice, const FString& Sale)
	{
		return ValdorsoSicurezza::Sha256Esadecimale(Sale + TEXT(":") + NormalizzaCodiceRecupero(Codice));
	}

	namespace
	{
		/** Lettera accentata italiana -> lettera semplice (minuscola). Zero se non è una lettera accentata ammessa. */
		TCHAR SenzaAccento(TCHAR C)
		{
			switch (C)
			{
			case 0x00E0: case 0x00C0: case 0x00E1: case 0x00C1: return TEXT('a');
			case 0x00E8: case 0x00C8: case 0x00E9: case 0x00C9: return TEXT('e');
			case 0x00EC: case 0x00CC: case 0x00ED: case 0x00CD: return TEXT('i');
			case 0x00F2: case 0x00D2: case 0x00F3: case 0x00D3: return TEXT('o');
			case 0x00F9: case 0x00D9: case 0x00FA: case 0x00DA: return TEXT('u');
			default: return 0;
			}
		}

		bool LetteraNome(TCHAR C)
		{
			return (C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('A') && C <= TEXT('Z')) || SenzaAccento(C) != 0;
		}

		bool SeparatoreNome(TCHAR C)
		{
			return C == TEXT(' ') || C == TEXT('\'') || C == TEXT('-');
		}

		/** Insulti e parole che non stanno bene in un nome (si cercano dentro lo scheletro). */
		const TCHAR* const ParoleVietate[] = {
			// Scritte come scheletri (la l diventa i): così valgono anche con le lettere scambiate.
			// Niente parole corte che stanno dentro nomi veri (per esempio "nazi" in Nazario).
			TEXT("cazz"), TEXT("merd"), TEXT("stronz"), TEXT("puttan"), TEXT("troia"), TEXT("vaffan"), TEXT("cogiion"),
			TEXT("minchi"), TEXT("froci"), TEXT("finocchi"), TEXT("nigg"), TEXT("nazist"), TEXT("hitier"),
			TEXT("mussoiini"), TEXT("fuck"), TEXT("shit"), TEXT("bitch"), TEXT("cunt"), TEXT("whore"),
			TEXT("porcodio"), TEXT("diocan"), TEXT("madonnapu"), TEXT("bastard"), TEXT("stupr"), TEXT("pedofi"),
			TEXT("ritardat"), TEXT("mongoioid")
		};

		/** Parole vere che contengono una parola vietata: nei testi liberi si lasciano passare. */
		const TCHAR* const ParoleAmmesse[] = {
			TEXT("cazzuoia"), TEXT("cazzuoie"), TEXT("finocchio"), TEXT("finocchi"),
			TEXT("ritardato"), TEXT("ritardata"), TEXT("ritardati"), TEXT("ritardate")
		};

		/** Parole dello staff: nessun personaggio può fingersi staff. */
		const TCHAR* const ParoleStaff[] = {
			TEXT("admin"), TEXT("amministrat"), TEXT("moderat"), TEXT("staff"), TEXT("narrator"), TEXT("sistema"),
			TEXT("system"), TEXT("server"), TEXT("vaidorso"), TEXT("supporto"), TEXT("support"), TEXT("gamemaster")
		};
	}

	FString NormalizzaNomePersonaggio(const FString& Nome)
	{
		FString Pulito;
		bool bInizioParola = true;
		for (const TCHAR C : Nome.TrimStartAndEnd())
		{
			if (C == TEXT(' '))
			{
				if (!Pulito.EndsWith(TEXT(" ")))
				{
					Pulito.AppendChar(C);
				}
				bInizioParola = true;
				continue;
			}
			Pulito.AppendChar(bInizioParola ? FChar::ToUpper(C) : C);
			bInizioParola = false;
		}
		return Pulito;
	}

	FString ScheletroNome(const FString& Nome)
	{
		FString Base;
		for (const TCHAR C : Nome)
		{
			if (const TCHAR Semplice = SenzaAccento(C))
			{
				Base.AppendChar(Semplice);
			}
			else if (C == TEXT('0'))
			{
				Base.AppendChar(TEXT('o'));
			}
			else if (C == TEXT('1') || C == TEXT('l') || C == TEXT('L') || C == TEXT('|'))
			{
				Base.AppendChar(TEXT('i'));
			}
			else if (FChar::IsAlpha(C))
			{
				Base.AppendChar(FChar::ToLower(C));
			}
		}
		Base.ReplaceInline(TEXT("rn"), TEXT("m"));
		Base.ReplaceInline(TEXT("vv"), TEXT("w"));
		return Base;
	}

	bool ContieneInsulto(const FString& Scheletro)
	{
		for (const TCHAR* Parola : ParoleVietate)
		{
			if (Scheletro.Contains(Parola))
			{
				return true;
			}
		}
		return false;
	}

	FString ProblemaTestoLibero(const FString& Testo, int32 Massimo)
	{
		if (Testo.Len() > Massimo)
		{
			return FString::Printf(TEXT("Il testo è troppo lungo: al massimo %d caratteri."), Massimo);
		}
		const FString Minuscolo = Testo.ToLower();
		if (Minuscolo.Contains(TEXT("http")) || Minuscolo.Contains(TEXT("www.")))
		{
			return TEXT("Nel registro non si scrivono indirizzi web.");
		}
		// Parola per parola (le parole sono le sequenze di lettere e cifre).
		FString Parola;
		auto Controlla = [&Parola]()
		{
			const FString Ossa = ScheletroNome(Parola);
			bool bAmmessa = false;
			for (const TCHAR* Ammessa : ParoleAmmesse)
			{
				bAmmessa |= Ossa == Ammessa;
			}
			const bool bInsulto = !Parola.IsEmpty() && !bAmmessa && ContieneInsulto(Ossa);
			Parola.Reset();
			return bInsulto;
		};
		for (const TCHAR C : Testo)
		{
			if (FChar::IsAlnum(C) || C >= 0x00C0)
			{
				Parola.AppendChar(C);
			}
			else if (Controlla())
			{
				return TEXT("Nel testo c'è una parola che non si può usare nella valle.");
			}
		}
		if (Controlla())
		{
			return TEXT("Nel testo c'è una parola che non si può usare nella valle.");
		}
		return FString();
	}

	bool ContieneParolaVietata(const FString& Scheletro)
	{
		if (ContieneInsulto(Scheletro))
		{
			return true;
		}
		for (const TCHAR* Parola : ParoleStaff)
		{
			if (Scheletro.Contains(Parola))
			{
				return true;
			}
		}
		// Parole corte dello staff: solo se sono tutto il nome.
		return Scheletro == TEXT("gm") || Scheletro == TEXT("dio") || Scheletro == TEXT("root");
	}

	FString ProblemaNomePersonaggio(const FString& Nome)
	{
		if (Nome.Len() < 3 || Nome.Len() > 20)
		{
			return TEXT("Il nome deve avere da 3 a 20 caratteri.");
		}
		int32 Spazi = 0;
		int32 Uguali = 1;
		for (int32 i = 0; i < Nome.Len(); ++i)
		{
			const TCHAR C = Nome[i];
			if (SeparatoreNome(C))
			{
				const bool bTraLettere = i > 0 && i < Nome.Len() - 1 && LetteraNome(Nome[i - 1]) && LetteraNome(Nome[i + 1]);
				if (!bTraLettere)
				{
					return TEXT("Spazi, apostrofi e trattini vanno solo tra due lettere.");
				}
				Spazi += C == TEXT(' ') ? 1 : 0;
				Uguali = 1;
				continue;
			}
			if (!LetteraNome(C))
			{
				return TEXT("Nel nome vanno solo lettere (anche accentate), spazi, apostrofi e trattini.");
			}
			Uguali = (i > 0 && FChar::ToLower(Nome[i - 1]) == FChar::ToLower(C)) ? Uguali + 1 : 1;
			if (Uguali >= 3)
			{
				return TEXT("Il nome non può avere tre lettere uguali di fila.");
			}
		}
		if (Spazi > 2)
		{
			return TEXT("Il nome può avere al massimo tre parole.");
		}
		if (ContieneParolaVietata(ScheletroNome(Nome)))
		{
			return TEXT("Questo nome non si può usare nella valle: scegline un altro.");
		}
		return FString();
	}

	int64 DurataBlocco(int32 BlocchiDiFila)
	{
		return FMath::Min<int64>(BloccoBaseSecondi << FMath::Clamp(BlocchiDiFila - 1, 0, 10), BloccoMassimoSecondi);
	}
}

namespace ValdorsoArchivioFile
{
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

	void SostituisciSenzaCopie(const FString& Percorso, const FString& Testo)
	{
		IFileManager& File = IFileManager::Get();
		const FString Temporaneo = Percorso + TEXT(".tmp");
		if (!FFileHelper::SaveStringToFile(Testo, *Temporaneo, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
			|| !File.Move(*Percorso, *Temporaneo, true, true))
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a riscrivere %s"), *Percorso);
		}
	}

	void CancellaConCopie(const FString& Percorso)
	{
		IFileManager& File = IFileManager::Get();
		for (const TCHAR* Fine : { TEXT(""), TEXT(".bak1"), TEXT(".bak2"), TEXT(".tmp") })
		{
			const FString Uno = Percorso + Fine;
			if (File.FileExists(*Uno) && !File.Delete(*Uno, false, true, true))
			{
				UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Archivista: non riesco a cancellare %s"), *Uno);
			}
		}
	}

	namespace
	{
		/** La data all'inizio di una riga del registro ("2026-10-04 22:35:24 UTC | ..."). */
		bool DataDellaRiga(const FString& Riga, FDateTime& Out)
		{
			if (Riga.Len() < 19)
			{
				return false;
			}
			const auto Numero = [&Riga](int32 Da, int32 Quanti) { return FCString::Atoi(*Riga.Mid(Da, Quanti)); };
			const int32 Anno = Numero(0, 4), Mese = Numero(5, 2), Giorno = Numero(8, 2);
			const int32 Ore = Numero(11, 2), Minuti = Numero(14, 2), Secondi = Numero(17, 2);
			if (Riga[4] != TEXT('-') || Riga[7] != TEXT('-') || !FDateTime::Validate(Anno, Mese, Giorno, Ore, Minuti, Secondi, 0))
			{
				return false;
			}
			Out = FDateTime(Anno, Mese, Giorno, Ore, Minuti, Secondi);
			return true;
		}

		/** Applica Cambia a ogni riga (tenendo gli a capo come sono) e conta quelle cambiate. */
		FString PerOgniRiga(const FString& Testo, int32& OutCambiate, TFunctionRef<bool(FString&)> Cambia)
		{
			OutCambiate = 0;
			FString Risultato;
			Risultato.Reserve(Testo.Len());
			int32 Inizio = 0;
			while (Inizio < Testo.Len())
			{
				int32 Fine = Testo.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Inizio);
				const bool bACapo = Fine != INDEX_NONE;
				if (!bACapo)
				{
					Fine = Testo.Len();
				}
				FString Riga = Testo.Mid(Inizio, Fine - Inizio);
				if (Cambia(Riga))
				{
					++OutCambiate;
				}
				Risultato += Riga;
				if (bACapo)
				{
					Risultato += TEXT("\n");
				}
				Inizio = Fine + 1;
			}
			return Risultato;
		}
	}

	FString OscuraIndirizziVecchi(const FString& Testo, const FDateTime& Ora, int32 Giorni, int32& OutCambiate)
	{
		const FDateTime Limite = Ora - FTimespan::FromDays(Giorni);
		return PerOgniRiga(Testo, OutCambiate, [&Limite](FString& Riga)
		{
			FDateTime Quando;
			if (!DataDellaRiga(Riga, Quando) || Quando >= Limite)
			{
				return false;
			}
			bool bCambiata = false;
			int32 Da = 0;
			for (;;)
			{
				const int32 Pos = Riga.Find(TEXT("| ip "), ESearchCase::CaseSensitive, ESearchDir::FromStart, Da);
				if (Pos == INDEX_NONE)
				{
					break;
				}
				const int32 InizioIp = Pos + 5;
				int32 FineIp = InizioIp;
				while (FineIp < Riga.Len() && Riga[FineIp] != TEXT(' ') && Riga[FineIp] != TEXT('|') && Riga[FineIp] != TEXT('\r'))
				{
					++FineIp;
				}
				const FString Indirizzo = Riga.Mid(InizioIp, FineIp - InizioIp);
				if (!Indirizzo.IsEmpty() && Indirizzo != TEXT("[rimosso]"))
				{
					Riga = Riga.Left(InizioIp) + TEXT("[rimosso]") + Riga.Mid(FineIp);
					bCambiata = true;
				}
				Da = InizioIp;
			}
			return bCambiata;
		});
	}

	FString SostituisciNome(const FString& Testo, const FString& Nome, const FString& Con, int32& OutCambiate)
	{
		const FString Cercato = Nome.TrimStartAndEnd();
		if (Cercato.IsEmpty())
		{
			OutCambiate = 0;
			return Testo;
		}
		return PerOgniRiga(Testo, OutCambiate, [&Cercato, &Con](FString& Riga)
		{
			const bool bRitorno = Riga.EndsWith(TEXT("\r"));
			TArray<FString> Campi;
			(bRitorno ? Riga.LeftChop(1) : Riga).ParseIntoArray(Campi, TEXT(" | "), false);
			// Il campo 0 è la data e l'1 è l'evento (ACCESSO, BANDITO...): un nome come "Accesso" non li tocca.
			bool bCambiata = false;
			for (int32 i = 2; i < Campi.Num(); ++i)
			{
				if (Campi[i].TrimStartAndEnd().Equals(Cercato, ESearchCase::IgnoreCase))
				{
					Campi[i] = Con;
					bCambiata = true;
				}
			}
			if (bCambiata)
			{
				Riga = FString::Join(Campi, TEXT(" | ")) + (bRitorno ? TEXT("\r") : TEXT(""));
			}
			return bCambiata;
		});
	}
}

// (06/10, passo 4.5) Il filtro della storia: dentro il namespace delle regole.
namespace ValdorsoRegole
{
	FString ProblemaTestoNelMondo(const FString& Testo, const FString& NomePersonaggio)
	{
		// Emoji e simboli fuori dall'alfabeto (le coppie surrogate UTF-16 e i simboli grafici).
		for (const TCHAR C : Testo)
		{
			const uint32 Codice = static_cast<uint32>(C);
			if ((Codice >= 0xD800 && Codice <= 0xDFFF) || (Codice >= 0x2600 && Codice <= 0x27BF) || (Codice >= 0x1F000))
			{
				return TEXT("Nella storia non si usano emoji: scrivila solo con le parole.");
			}
		}
		// Indirizzi email e numeri di telefono.
		int32 CifreDiFila = 0;
		for (int32 i = 0; i < Testo.Len(); ++i)
		{
			const TCHAR C = Testo[i];
			if (FChar::IsDigit(C))
			{
				// (Da 9 cifre in su, anche divise da spazi o punti: un numero di telefono. Gli anni restano liberi.)
				if (++CifreDiFila >= 9)
				{
					return TEXT("Nella storia non si scrivono numeri di telefono o codici.");
				}
			}
			else if (C != ' ' && C != '.' && C != '-')
			{
				CifreDiFila = 0;
			}
			if (C == '@' && i > 0 && i + 1 < Testo.Len() && FChar::IsAlnum(Testo[i - 1]) && FChar::IsAlnum(Testo[i + 1]))
			{
				return TEXT("Nella storia non si scrivono indirizzi email.");
			}
		}
		// Lettere ripetute per riempire ("aaaaaa") e testo gridato (tutto maiuscolo).
		int32 Uguali = 0;
		int32 Lettere = 0;
		int32 Maiuscole = 0;
		TCHAR Prima = 0;
		for (const TCHAR C : Testo)
		{
			Uguali = (FChar::ToLower(C) == FChar::ToLower(Prima) && FChar::IsAlpha(C)) ? Uguali + 1 : 1;
			Prima = C;
			if (Uguali >= 5)
			{
				return TEXT("Nella storia c'è una lettera ripetuta troppe volte.");
			}
			if (FChar::IsAlpha(C))
			{
				++Lettere;
				Maiuscole += FChar::IsUpper(C) ? 1 : 0;
			}
		}
		if (Lettere >= 20 && Maiuscole * 10 > Lettere * 6)
		{
			return TEXT("Nella storia non si scrive tutto in maiuscolo: nella valle non si grida.");
		}
		// Parole del mondo reale, intere (non pezzi di parola: "romano" va bene, "Roma" no).
		static const TCHAR* const MondoReale[] = {
			TEXT("italia"), TEXT("roma"), TEXT("milano"), TEXT("napoli"), TEXT("torino"), TEXT("venezia"), TEXT("firenze"),
			TEXT("londra"), TEXT("parigi"), TEXT("berlino"), TEXT("madrid"), TEXT("america"), TEXT("europa"), TEXT("africa"),
			TEXT("germania"), TEXT("francia"), TEXT("spagna"), TEXT("inghilterra"), TEXT("cina"), TEXT("giappone"),
			TEXT("russia"), TEXT("ucraina"), TEXT("brasile"), TEXT("messico"), TEXT("canada"), TEXT("australia"),
			TEXT("facebook"), TEXT("instagram"), TEXT("youtube"), TEXT("tiktok"), TEXT("twitch"), TEXT("discord"), TEXT("google"),
			TEXT("whatsapp"), TEXT("telegram"), TEXT("twitter"), TEXT("iphone"), TEXT("android"), TEXT("internet"), TEXT("computer"),
			TEXT("smartphone"), TEXT("cellulare"), TEXT("televisione"), TEXT("netflix"), TEXT("amazon"), TEXT("microsoft"),
			TEXT("pokemon"), TEXT("naruto"), TEXT("goku"), TEXT("gandalf"), TEXT("hogwarts"), TEXT("potter"), TEXT("skyrim"),
			TEXT("minecraft"), TEXT("fortnite"), TEXT("hitler"), TEXT("mussolini"), TEXT("nazi"), TEXT("nazista"), TEXT("fascista"),
			TEXT("covid"), TEXT("euro"), TEXT("dollari"), TEXT("gesù"), TEXT("cristo"), TEXT("allah"), TEXT("buddha"),
		};
		// (Le lettere accentate sì, i segni come l'apostrofo curvo no: "dall’Italia" sono due parole.)
		auto Lettera = [](TCHAR C) { return FChar::IsAlpha(C) || (C >= 0x00C0 && C <= 0x024F && C != 0x00D7 && C != 0x00F7); };
		TArray<FString> DelNome;
		FString Pezzo;
		for (const TCHAR C : NomePersonaggio + TEXT(" "))
		{
			if (Lettera(C))
			{
				Pezzo.AppendChar(C);
			}
			else if (!Pezzo.IsEmpty())
			{
				DelNome.Add(Pezzo.ToLower());
				Pezzo.Reset();
			}
		}
		FString Parola;
		// (Un insieme, fatto una volta sola: la schermata del Registro fa questo controllo a ogni fotogramma.)
		static const TSet<FString> Vietate = []()
		{
			TSet<FString> Insieme;
			for (const TCHAR* Vietata : MondoReale)
			{
				Insieme.Add(Vietata);
			}
			return Insieme;
		}();
		auto Nel = [&Parola, &DelNome]()
		{
			const FString Minuscola = Parola.ToLower();
			Parola.Reset();
			return !Minuscola.IsEmpty() && !DelNome.Contains(Minuscola) && Vietate.Contains(Minuscola);
		};
		for (const TCHAR C : Testo)
		{
			if (Lettera(C))
			{
				Parola.AppendChar(C);
			}
			else if (Nel())
			{
				return TEXT("Nella storia c'è un nome del mondo reale: Valdorso è un mondo a sé, scrivila con luoghi e nomi della valle.");
			}
		}
		if (Nel())
		{
			return TEXT("Nella storia c'è un nome del mondo reale: Valdorso è un mondo a sé, scrivila con luoghi e nomi della valle.");
		}
		return FString();
	}
}
