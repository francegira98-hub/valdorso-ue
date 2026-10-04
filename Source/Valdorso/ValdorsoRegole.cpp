// Valdorso - Le regole degli account e i file dell'archivio (vedi il .h).

#include "ValdorsoRegole.h"
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
}
