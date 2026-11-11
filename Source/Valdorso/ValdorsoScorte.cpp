// Valdorso - Le scorte del villaggio (vedi ValdorsoScorte.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoScorte.h"
#include "Valdorso.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoGilda.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoVitaValle.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Components/SphereComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#define LOCTEXT_NAMESPACE "ValdorsoScorte"

namespace
{
	struct FScorteStato
	{
		FValdorsoScorteValle Valori;
		FString File;
		bool bLette = false;
	};

	FScorteStato& ScorteStato()
	{
		static FScorteStato Stato;
		return Stato;
	}

	FString ScorteFile(const UObject* Contesto)
	{
		const UWorld* Mondo = Contesto ? Contesto->GetWorld() : nullptr;
		const UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(Contesto);
		const TCHAR* Cartella = Mondo && Mondo->IsPlayInEditor() ? TEXT("Prova")
			: (Archivista && Archivista->ArchivioSolo() ? TEXT("Solo") : TEXT("Server"));
		return FPaths::ProjectSavedDir() / Cartella / TEXT("Scorte.json");
	}

	void ScorteLeggi(FScorteStato& Stato)
	{
		Stato.bLette = true;
		Stato.Valori = FValdorsoScorteValle();
		Stato.Valori.UltimoConsumo = FDateTime::UtcNow().ToUnixTimestamp();
		FString Testo;
		if (!FFileHelper::LoadFileToString(Testo, *Stato.File))
		{
			return;
		}
		TSharedPtr<FJsonObject> Oggetto;
		const TSharedRef<TJsonReader<>> Lettore = TJsonReaderFactory<>::Create(Testo);
		if (!FJsonSerializer::Deserialize(Lettore, Oggetto) || !Oggetto.IsValid())
		{
			return;
		}
		// (i campi che mancano restano quelli di partenza)
		const float M = ValdorsoScorte::Massimo;
		double Numero = 0.0;
		if (Oggetto->TryGetNumberField(TEXT("Grano"), Numero)) { Stato.Valori.Grano = FMath::Clamp(static_cast<float>(Numero), 0.f, M); }
		if (Oggetto->TryGetNumberField(TEXT("Legna"), Numero)) { Stato.Valori.Legna = FMath::Clamp(static_cast<float>(Numero), 0.f, M); }
		if (Oggetto->TryGetNumberField(TEXT("Pietra"), Numero)) { Stato.Valori.Pietra = FMath::Clamp(static_cast<float>(Numero), 0.f, M); }
		bool bUsato = false;
		if (Oggetto->TryGetBoolField(TEXT("GranoNascostoUsato"), bUsato)) { Stato.Valori.bGranoNascostoUsato = bUsato; }
		if (Oggetto->TryGetNumberField(TEXT("UltimoConsumo"), Numero) && Numero > 0.0) { Stato.Valori.UltimoConsumo = static_cast<int64>(Numero); }
	}
}

namespace ValdorsoScorte
{
	void Consuma(FValdorsoScorteValle& Scorte, int64 Ora, bool bInverno)
	{
		if (Scorte.UltimoConsumo <= 0 || Ora <= Scorte.UltimoConsumo)
		{
			Scorte.UltimoConsumo = FMath::Max(Scorte.UltimoConsumo, Ora);
			return;
		}
		// a passi interi di dieci minuti (il resto conta la volta dopo); al massimo una settimana
		const int64 Passi = FMath::Min<int64>((Ora - Scorte.UltimoConsumo) / 600, 7 * 24 * 6);
		if (Passi <= 0)
		{
			return;
		}
		Scorte.UltimoConsumo += Passi * 600;
		Scorte.Grano = FMath::Max(0.f, Scorte.Grano - GranoOgniDieci * Passi);
		Scorte.Legna = FMath::Max(0.f, Scorte.Legna - LegnaOgniDieci * (bInverno ? 2.f : 1.f) * Passi);
		// il grano nascosto si riarma quando il granaio torna ben sopra meta' (non con il grano nascosto stesso)
		if (Scorte.Grano > Massimo * 0.75f)
		{
			Scorte.bGranoNascostoUsato = false;
		}
	}

	int32 Prezzo(FName Oggetto)
	{
		if (Oggetto == FName(TEXT("Legna"))) { return 2; }
		if (Oggetto == FName(TEXT("Pietra_Grezza"))) { return 3; }
		if (Oggetto == FName(TEXT("Pietra_Squadrata"))) { return 6; }
		return 0;
	}

	float* Scorta(FValdorsoScorteValle& Scorte, FName Oggetto, float& OutQuanto)
	{
		OutQuanto = 0.f;
		if (Oggetto == FName(TEXT("Legna"))) { OutQuanto = 1.f; return &Scorte.Legna; }
		if (Oggetto == FName(TEXT("Pietra_Grezza"))) { OutQuanto = 1.f; return &Scorte.Pietra; }
		if (Oggetto == FName(TEXT("Pietra_Squadrata"))) { OutQuanto = 3.f; return &Scorte.Pietra; }
		return nullptr;
	}

	TArray<FString> Righe(const FValdorsoScorteValle& Scorte)
	{
		auto Barra = [](float V)
		{
			const int32 Pieni = FMath::Clamp(FMath::RoundToInt(V / Massimo * 10.f), 0, 10);
			return FString::ChrN(Pieni, TEXT('#')) + FString::ChrN(10 - Pieni, TEXT('.'));
		};
		TArray<FString> Fuori;
		Fuori.Add(FString::Printf(TEXT("Grano:  [%s] %d su %d"), *Barra(Scorte.Grano), FMath::RoundToInt(Scorte.Grano), FMath::RoundToInt(Massimo)));
		Fuori.Add(FString::Printf(TEXT("Legna:  [%s] %d su %d"), *Barra(Scorte.Legna), FMath::RoundToInt(Scorte.Legna), FMath::RoundToInt(Massimo)));
		Fuori.Add(FString::Printf(TEXT("Pietra: [%s] %d su %d"), *Barra(Scorte.Pietra), FMath::RoundToInt(Scorte.Pietra), FMath::RoundToInt(Massimo)));
		Fuori.Add(Scorte.Grano < SogliaFame ? TEXT("Il granaio e' quasi vuoto: la valle ha fame. Al mulino, qualcuno sa qualcosa.")
			: TEXT("Porta qui legna e pietra: la cassa del villaggio paga, e la Gilda se ne ricorda."));
		return Fuori;
	}

	FValdorsoScorteValle& Adesso(const UObject* Contesto)
	{
		FScorteStato& Stato = ScorteStato();
		const FString File = ScorteFile(Contesto);
		if (!Stato.bLette || Stato.File != File)
		{
			Stato.File = File;
			ScorteLeggi(Stato);
		}
		Consuma(Stato.Valori, FDateTime::UtcNow().ToUnixTimestamp(), ValdorsoVitaValle::StagioneAdesso() == TEXT("inverno"));
		return Stato.Valori;
	}

	void Salva(const UObject* Contesto)
	{
		FScorteStato& Stato = ScorteStato();
		const FValdorsoScorteValle& V = Adesso(Contesto);
		const TSharedRef<FJsonObject> Oggetto = MakeShared<FJsonObject>();
		Oggetto->SetNumberField(TEXT("Versione"), 1);
		Oggetto->SetNumberField(TEXT("Grano"), V.Grano);
		Oggetto->SetNumberField(TEXT("Legna"), V.Legna);
		Oggetto->SetNumberField(TEXT("Pietra"), V.Pietra);
		Oggetto->SetBoolField(TEXT("GranoNascostoUsato"), V.bGranoNascostoUsato);
		Oggetto->SetNumberField(TEXT("UltimoConsumo"), static_cast<double>(V.UltimoConsumo));
		FString Testo;
		const TSharedRef<TJsonWriter<>> Scrittore = TJsonWriterFactory<>::Create(&Testo);
		if (FJsonSerializer::Serialize(Oggetto, Scrittore))
		{
			FFileHelper::SaveStringToFile(Testo, *Stato.File);
		}
	}

	bool Fame(const UObject* Contesto)
	{
		return Adesso(Contesto).Grano < SogliaFame;
	}
}

// ------------------------------------------------------------------------------------------------ il magazzino

AValdorsoMagazzino::AValdorsoMagazzino()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	SetRootComponent(Presa);
	Presa->InitSphereRadius(140.f);
	Presa->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Presa->SetCollisionObjectType(ECC_WorldDynamic);
	Presa->SetCollisionResponseToAllChannels(ECR_Overlap);
	Presa->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Presa->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

FText AValdorsoMagazzino::TestoInterazione(const APawn* Chi) const
{
	return LOCTEXT("Magazzino", "Il magazzino del villaggio: porta legna e pietra, o guarda le scorte");
}

void AValdorsoMagazzino::Interagisci(APawn* Chi)
{
	AValdorsoPlayerController* Giocatore = Chi ? Cast<AValdorsoPlayerController>(Chi->GetController()) : nullptr;
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Giocatore);
	if (!HasAuthority() || !Zaino || FVector::Dist(Chi->GetActorLocation(), GetActorLocation()) > 400.f)
	{
		return;
	}
	FValdorsoScorteValle& Scorte = ValdorsoScorte::Adesso(this);
	int32 Pezzi = 0;
	int32 Paga = 0;
	Zaino->Cambia([&](FValdorsoZaino& Dentro)
	{
		for (const TCHAR* Id : { TEXT("Legna"), TEXT("Pietra_Grezza"), TEXT("Pietra_Squadrata") })
		{
			const FName Nome(Id);
			float Vale = 0.f;
			float* Dove = ValdorsoScorte::Scorta(Scorte, Nome, Vale);
			const int32 Ne = ValdorsoZaino::Conta(Dentro, Nome);
			if (!Dove || Ne <= 0 || Vale <= 0.f)
			{
				continue;
			}
			// solo quanti ne entrano nella scorta
			const int32 Entrano = FMath::Min(Ne, FMath::FloorToInt((ValdorsoScorte::Massimo - *Dove) / Vale));
			if (Entrano <= 0)
			{
				continue;
			}
			ValdorsoZaino::TogliOggetto(Dentro, Nome, Entrano);
			*Dove += Entrano * Vale;
			Pezzi += Entrano;
			Paga += Entrano * ValdorsoScorte::Prezzo(Nome);
		}
		if (Paga > 0)
		{
			Dentro.Monete = FMath::Min(Dentro.Monete + Paga, ValdorsoZaino::MassimoMonete);
		}
		return Pezzi > 0;
	});
	if (Pezzi <= 0)
	{
		Giocatore->ClientMostraBacheca(TEXT("Il magazzino del villaggio"), ValdorsoScorte::Righe(Scorte));
		return;
	}
	ValdorsoScorte::Salva(this);
	if (UValdorsoGildaComponent* Gilda = UValdorsoGildaComponent::Di(Giocatore); Gilda && Pezzi >= 5)
	{
		Gilda->AggiungiPunti(Pezzi / 5);
	}
	Giocatore->ClientAvviso(FText::Format(LOCTEXT("Portato", "Porti al magazzino {0} pezzi: la cassa del villaggio ti da' {1} Aureus."),
		FText::AsNumber(Pezzi), FText::AsNumber(Paga)), 4.f, 0.f);
	ValdorsoRegistroEventi::Annota(TEXT("Magazzino"), Chi, this, GetActorLocation(), FString::Printf(TEXT("%d pezzi, %d Aureus"), Pezzi, Paga));
}

// ------------------------------------------------------------------------------------------------ console (prove)

namespace
{
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoScorteMostra(
		TEXT("Valdorso.Scorte.Mostra"),
		TEXT("Valdorso: le scorte del villaggio (sul server)."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
		{
			if (Mondo)
			{
				Uscita.Log(FString::Join(ValdorsoScorte::Righe(ValdorsoScorte::Adesso(Mondo)), TEXT("\n")));
			}
		}));

	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoScorteImposta(
		TEXT("Valdorso.Scorte.Imposta"),
		TEXT("Valdorso: Valdorso.Scorte.Imposta <grano> <legna> <pietra> (prove: per esempio 30 per la fame)."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
		{
			if (!Mondo || Argomenti.Num() < 3)
			{
				Uscita.Log(TEXT("Valdorso.Scorte.Imposta <grano> <legna> <pietra>"));
				return;
			}
			FValdorsoScorteValle& S = ValdorsoScorte::Adesso(Mondo);
			const float M = ValdorsoScorte::Massimo;
			S.Grano = FMath::Clamp(FCString::Atof(*Argomenti[0]), 0.f, M);
			S.Legna = FMath::Clamp(FCString::Atof(*Argomenti[1]), 0.f, M);
			S.Pietra = FMath::Clamp(FCString::Atof(*Argomenti[2]), 0.f, M);
			S.bGranoNascostoUsato = false;
			ValdorsoScorte::Salva(Mondo);
			Uscita.Log(FString::Join(ValdorsoScorte::Righe(S), TEXT("\n")));
		}));
}

#undef LOCTEXT_NAMESPACE
