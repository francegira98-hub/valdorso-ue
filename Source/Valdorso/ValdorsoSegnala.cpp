// Valdorso - F8: la segnalazione (vedi ValdorsoSegnala.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoSegnala.h"
#include "Valdorso.h"
#include "ValdorsoCielo.h"
#include "ValdorsoVersione.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformOutputDevices.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"
#include "UnrealClient.h"

namespace
{
	/** Quante righe del log, e quanto della sua coda si legge al massimo (byte). */
	constexpr int32 SegnalaRigheLog = 160;
	constexpr int64 SegnalaCodaLog = 96 * 1024;
	constexpr double SegnalaSecondiTra = 10.0;
	double SegnalaUltima = -1000.0;

	FString SegnalaCodaDelLog()
	{
		if (GLog)
		{
			GLog->Flush();
		}
		const FString Percorso = FGenericPlatformOutputDevices::GetAbsoluteLogFilename();
		TUniquePtr<FArchive> Lettore(IFileManager::Get().CreateFileReader(*Percorso, FILEREAD_AllowWrite));
		if (!Lettore)
		{
			return FString::Printf(TEXT("(il log %s non si apre)"), *Percorso);
		}
		const int64 Tutto = Lettore->TotalSize();
		const int64 Da = FMath::Max<int64>(0, Tutto - SegnalaCodaLog);
		TArray<uint8> Byte;
		Byte.SetNumUninitialized(static_cast<int32>(Tutto - Da));
		Lettore->Seek(Da);
		Lettore->Serialize(Byte.GetData(), Byte.Num());
		Lettore->Close();
		FString Testo;
		FFileHelper::BufferToString(Testo, Byte.GetData(), Byte.Num());
		return ValdorsoSegnala::UltimeRighe(Testo, SegnalaRigheLog);
	}
}

namespace ValdorsoSegnala
{
	FString UltimeRighe(const FString& Testo, int32 Quante)
	{
		TArray<FString> Righe;
		Testo.ParseIntoArrayLines(Righe, false);
		while (Righe.Num() > 0 && Righe.Last().TrimStartAndEnd().IsEmpty())
		{
			Righe.Pop();
		}
		const int32 Da = FMath::Max(0, Righe.Num() - FMath::Max(0, Quante));
		TArray<FString> Ultime;
		for (int32 i = Da; i < Righe.Num(); ++i)
		{
			Ultime.Add(Righe[i]);
		}
		return FString::Join(Ultime, TEXT("\n"));
	}

	FString Cartella()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Segnalazioni"));
	}

	bool Fai(APlayerController* Controllore, FString& OutNomeFile, FString& OutBreve)
	{
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		if (!Mondo)
		{
			return false;
		}
		const double Adesso = FPlatformTime::Seconds();
		if (Adesso - SegnalaUltima < SegnalaSecondiTra)
		{
			return false;
		}
		SegnalaUltima = Adesso;

		const FDateTime Ora = FDateTime::Now();
		const FString Nome = FString::Printf(TEXT("Segnalazione_%s"), *Ora.ToString(TEXT("%Y%m%d_%H%M%S")));
		const FString Base = Cartella() / Nome;
		IFileManager::Get().MakeDirectory(*Cartella(), true);

		// la foto, senza interfaccia (si salva alla fine del fotogramma)
		FScreenshotRequest::RequestScreenshot(Base + TEXT(".png"), false, false);

		const APawn* Corpo = Controllore->GetPawn();
		const FVector Dove = Corpo ? Corpo->GetActorLocation() : FVector::ZeroVector;
		const FRotator Sguardo = Controllore->GetControlRotation();
		float OraValle = -1.f;
		for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
		{
			OraValle = It->Ora();
			break;
		}
		const FString Personaggio = Controllore->PlayerState ? Controllore->PlayerState->GetPlayerName() : FString(TEXT("?"));
		const TCHAR* Rete = Mondo->GetNetMode() == NM_Client ? TEXT("collegato a un server") : (Mondo->GetNetMode() == NM_Standalone ? TEXT("da solo") : TEXT("server in casa"));

		TArray<FString> Righe;
		Righe.Add(TEXT("Valdorso - segnalazione (F8)"));
		Righe.Add(FString::Printf(TEXT("Versione: %s (rete %08x)"), *ValdorsoVersione::PerScritte(), ValdorsoVersione::DiRete()));
		Righe.Add(FString::Printf(TEXT("Quando: %s"), *Ora.ToString(TEXT("%d/%m/%Y %H:%M:%S"))));
		Righe.Add(FString::Printf(TEXT("Mappa: %s (%s)"), *UGameplayStatics::GetCurrentLevelName(Mondo, true), Rete));
		Righe.Add(FString::Printf(TEXT("Personaggio: %s"), *Personaggio));
		Righe.Add(FString::Printf(TEXT("Dove: X=%.0f Y=%.0f Z=%.0f"), Dove.X, Dove.Y, Dove.Z));
		Righe.Add(FString::Printf(TEXT("Guarda: %.0f gradi (in su %.0f)"), Sguardo.Yaw, Sguardo.Pitch));
		Righe.Add(OraValle >= 0.f ? FString::Printf(TEXT("Ora della valle: %02d:%02d"), FMath::FloorToInt(OraValle), FMath::FloorToInt(FMath::Frac(OraValle) * 60.f)) : FString(TEXT("Ora della valle: ?")));
		Righe.Add(FString::Printf(TEXT("Foto: %s.png"), *Nome));
		Righe.Add(TEXT(""));
		Righe.Add(TEXT("Cosa non va (scrivilo qui sotto, se vuoi):"));
		Righe.Add(TEXT(""));
		Righe.Add(TEXT(""));
		Righe.Add(FString::Printf(TEXT("--- le ultime %d righe del log ---"), SegnalaRigheLog));
		Righe.Add(SegnalaCodaDelLog());
		FFileHelper::SaveStringToFile(FString::Join(Righe, TEXT("\r\n")), *(Base + TEXT(".txt")), FFileHelper::EEncodingOptions::ForceUTF8);

		OutNomeFile = Nome;
		OutBreve = FString::Printf(TEXT("%s, %s"), ValdorsoVersione::Numero(), *Nome);
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Segnalazione salvata: %s (.png e .txt)"), *Base);
		return true;
	}
}
