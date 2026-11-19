// Valdorso - I contratti firmati e i documenti del Balivo (vedi ValdorsoContratto.h).

#include "ValdorsoContratto.h"
#include "Valdorso.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace ValdorsoContratti
{
	void Aggiungi(TArray<FValdorsoDocumento>& Documenti, FName Tipo, const FString& Titolo, int64 AdessoUtc)
	{
		FValdorsoDocumento Nuovo;
		Nuovo.Tipo = Tipo;
		Nuovo.Titolo = Titolo;
		Nuovo.FirmatoUtc = AdessoUtc;
		Nuovo.Stato = InVigore;
		Documenti.Add(Nuovo);
		// troppi: escono i piu' vecchi gia' chiusi (quelli in vigore restano)
		for (int32 i = 0; i < Documenti.Num() && Documenti.Num() > MassimoDocumenti; )
		{
			if (Documenti[i].Stato != InVigore)
			{
				Documenti.RemoveAt(i);
			}
			else
			{
				++i;
			}
		}
	}

	bool Chiudi(TArray<FValdorsoDocumento>& Documenti, FName Tipo, uint8 Stato, int64 AdessoUtc)
	{
		for (int32 i = Documenti.Num() - 1; i >= 0; --i)
		{
			if (Documenti[i].Tipo == Tipo && Documenti[i].Stato == InVigore)
			{
				Documenti[i].Stato = Stato;
				Documenti[i].ChiusoUtc = AdessoUtc;
				return true;
			}
		}
		return false;
	}

	void ScriviAlBalivo(const FString& Nome, FName Tipo, const FString& Titolo, const FString& Cosa, const FString& Firma)
	{
		const FDateTime Adesso = FDateTime::UtcNow();
		const TSharedRef<FJsonObject> Riga = MakeShared<FJsonObject>();
		Riga->SetStringField(TEXT("quando"), Adesso.ToIso8601());
		Riga->SetStringField(TEXT("chi"), Nome);
		Riga->SetStringField(TEXT("tipo"), Tipo.ToString());
		Riga->SetStringField(TEXT("titolo"), Titolo);
		Riga->SetStringField(TEXT("cosa"), Cosa);
		if (!Firma.IsEmpty())
		{
			Riga->SetStringField(TEXT("firma"), Firma);
		}
		FString Testo;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Scrittore =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Testo);
		if (!FJsonSerializer::Serialize(Riga, Scrittore))
		{
			return;
		}
		Testo += LINE_TERMINATOR;
		const FString File = FPaths::ProjectSavedDir() / TEXT("Server") / TEXT("Balivo") / TEXT("Documenti.jsonl");
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
		if (!FFileHelper::SaveStringToFile(Testo, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append))
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Balivo: non riesco a scrivere %s"), *File);
		}
	}
}
