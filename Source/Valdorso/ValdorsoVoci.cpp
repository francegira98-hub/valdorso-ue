// Valdorso - La locanda viva e l'Atto I "Il battito cambiato" (v0.1.6, 11/11/2026). Scritto da Claude. Vedi ValdorsoVoci.h.

#include "ValdorsoVoci.h"
#include "Valdorso.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoGrottaNord.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTerritorio.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#define LOCTEXT_NAMESPACE "ValdorsoVoci"

namespace
{
	/** L'atto letto dal file (una volta per archivio: Server, Solo o Prova). */
	struct FVociAtti
	{
		FString File;
		int32 Atto = 0;
	};

	FVociAtti& VociAtti()
	{
		static FVociAtti Atti;
		return Atti;
	}

	FString VociFile(const UObject* Contesto)
	{
		const UWorld* Mondo = Contesto ? Contesto->GetWorld() : nullptr;
		const UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(Contesto);
		const TCHAR* Cartella = Mondo && Mondo->IsPlayInEditor() ? TEXT("Prova")
			: (Archivista && Archivista->ArchivioSolo() ? TEXT("Solo") : TEXT("Server"));
		return FPaths::ProjectSavedDir() / Cartella / TEXT("Atti.json");
	}

	void VociLeggi(const FString& File)
	{
		FVociAtti& Atti = VociAtti();
		Atti.File = File;
		Atti.Atto = 0;
		FString Testo;
		if (!FFileHelper::LoadFileToString(Testo, *File))
		{
			return;
		}
		TSharedPtr<FJsonObject> Oggetto;
		const TSharedRef<TJsonReader<>> Lettore = TJsonReaderFactory<>::Create(Testo);
		if (FJsonSerializer::Deserialize(Lettore, Oggetto) && Oggetto.IsValid())
		{
			Atti.Atto = FMath::Clamp(static_cast<int32>(Oggetto->GetNumberField(TEXT("Atto"))), 0, 9);
		}
	}

	void VociScrivi(const FString& File, int32 Atto)
	{
		const TSharedRef<FJsonObject> Oggetto = MakeShared<FJsonObject>();
		Oggetto->SetNumberField(TEXT("Versione"), 1);
		Oggetto->SetNumberField(TEXT("Atto"), Atto);
		Oggetto->SetStringField(TEXT("Quando"), FDateTime::UtcNow().ToIso8601());
		FString Testo;
		const TSharedRef<TJsonWriter<>> Scrittore = TJsonWriterFactory<>::Create(&Testo);
		if (FJsonSerializer::Serialize(Oggetto, Scrittore))
		{
			FFileHelper::SaveStringToFile(Testo, *File);
		}
	}

	/** Quante righe del registro sono di quel tipo e hanno quei dettagli (nullptr = tutte). Maiuscole contate: "Orso"
	 *  non deve trovare "ValdorsoAnimale"; i dettagli stanno in fondo alla riga, dopo " · ". */
	int32 VociConta(const TArray<FString>& Righe, const TCHAR* Tipo, const TCHAR* Dettaglio)
	{
		const FString Cerca = FString::Printf(TEXT(" · %s · "), Tipo);
		const FString Dopo = Dettaglio ? FString::Printf(TEXT(" · %s"), Dettaglio) : FString();
		int32 Quante = 0;
		for (const FString& Riga : Righe)
		{
			if (Riga.Contains(Cerca, ESearchCase::CaseSensitive) && (Dopo.IsEmpty() || Riga.Contains(Dopo, ESearchCase::CaseSensitive)))
			{
				++Quante;
			}
		}
		return Quante;
	}

	FString VociNumero(int32 N)
	{
		static const TCHAR* Nomi[] = { TEXT("nessuno"), TEXT("uno"), TEXT("due"), TEXT("tre"), TEXT("quattro"), TEXT("cinque"),
			TEXT("sei"), TEXT("sette"), TEXT("otto"), TEXT("nove"), TEXT("dieci") };
		return N >= 0 && N <= 10 ? FString(Nomi[N]) : FString::FromInt(N);
	}

	void VociAvvisaTutti(UWorld* Mondo, const FText& Testo)
	{
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(It->Get()))
			{
				Giocatore->ClientAvviso(Testo, 6.f, 0.f);
			}
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoVociAtto(
		TEXT("Valdorso.Atto.Battito"),
		TEXT("Atto I \"Il battito cambiato\": 1 il frammento del tempio salta un colpo (da allora resta debole), 0 torna come prima (prove)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			if (Mondo && Mondo->GetNetMode() != NM_Client)
			{
				ValdorsoVoci::ImpostaAtto(Mondo, Argomenti.Num() > 0 ? FCString::Atoi(*Argomenti[0]) : 1);
			}
		}));
}

namespace ValdorsoVoci
{
	int32 Atto(const UObject* Contesto)
	{
		const FString File = VociFile(Contesto);
		if (VociAtti().File != File)
		{
			VociLeggi(File);
		}
		return VociAtti().Atto;
	}

	void ImpostaAtto(UWorld* Mondo, int32 Nuovo)
	{
		if (!Mondo || Mondo->GetNetMode() == NM_Client)
		{
			return;
		}
		Nuovo = FMath::Clamp(Nuovo, 0, 9);
		const int32 Prima = Atto(Mondo);
		const FString File = VociFile(Mondo);
		VociAtti().File = File;
		VociAtti().Atto = Nuovo;
		VociScrivi(File, Nuovo);
		AActor* Pietra = nullptr;
		for (TActorIterator<AValdorsoPietraVillaggio> It(Mondo); It; ++It)
		{
			It->ImpostaForza(Nuovo >= 1 ? FMath::Min(It->GetForza(), ForzaAttoI) : 1.f);
			Pietra = *It;
		}
		if (Nuovo >= 1 && Prima < 1)
		{
			VociAvvisaTutti(Mondo, LOCTEXT("AttoUno",
				"Dal Tempio del Cuore, all'improvviso, il silenzio: il battito si e' fermato. Un respiro. Poi riprende, piu' piano."));
		}
		if (Pietra)
		{
			ValdorsoRegistroEventi::Annota(TEXT("Atto"), Pietra, nullptr, Pietra->GetActorLocation(),
				FString::Printf(TEXT("Atto I \"Il battito cambiato\": %d (prima %d)"), Nuovo, Prima));
		}
		UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Atto: %d (prima %d), %s"), Nuovo, Prima, *File);
	}

	FValdorsoFattiValle Fatti(UWorld* Mondo)
	{
		FValdorsoFattiValle Fuori;
		const TArray<FString> Righe = ValdorsoRegistroEventi::Ultimi(500);
		Fuori.Lupi = VociConta(Righe, TEXT("Animale"), TEXT("Lupo ucciso"));
		Fuori.Orsi = VociConta(Righe, TEXT("Animale"), TEXT("Orso ucciso"));
		Fuori.Cinghiali = VociConta(Righe, TEXT("Animale"), TEXT("Cinghiale ucciso"));
		Fuori.Morti = VociConta(Righe, TEXT("Morte"), nullptr);
		Fuori.Scheletri = VociConta(Righe, TEXT("Scheletro"), nullptr);
		Fuori.bCapitano = VociConta(Righe, TEXT("Scheletro"), TEXT("Capitano distrutto")) > 0;
		Fuori.bStoriaLucia = VociConta(Righe, TEXT("Storia"), TEXT("Quelli che non tornarono: finita")) > 0;
		if (const AValdorsoFrana* Frana = AValdorsoFrana::Di(Mondo))
		{
			Fuori.Frana = static_cast<int32>(Frana->GetStato());
		}
		Fuori.Atto = Atto(Mondo);
		return Fuori;
	}

	TArray<FString> DaiFatti(const FValdorsoFattiValle& F)
	{
		TArray<FString> Voci;
		if (F.Atto >= 1)
		{
			Voci.Add(TEXT("Il frammento del tempio salta i colpi. Non una notte sola, adesso: anche di giorno, quando meno te l'aspetti. Padre Anselmo non dorme piu', si vede dagli occhi. E Tobia, che ci rideva sopra, ha smesso di ridere."));
		}
		if (F.Frana == 2)
		{
			Voci.Add(TEXT("La frana del nord ha ceduto. Chi e' sceso nelle gallerie dei nani torna con le monete quadrate e la faccia bianca: dice che laggiu' le ossa si alzano e camminano. Io dico che hanno bevuto. Pero' non ci scendo."));
		}
		else if (F.Frana == 1)
		{
			Voci.Add(TEXT("Sotto la montagna del nord la terra ha tremato. Dicono che la frana dei nani si sia mossa, e che con un piccone e la schiena buona si possa aprire. Marta non fa che vendere picconi: mai vista cosi' contenta."));
		}
		if (F.bCapitano)
		{
			Voci.Add(TEXT("Qualcuno, laggiu' nelle gallerie, ha fatto a pezzi il capitano delle ossa. Il capitano! Hanno brindato qui fino a tardi, e hanno pagato con le monete dei nani. Guarda: quadrate. Le tengo sotto il bancone, non si sa mai."));
		}
		else if (F.Scheletri > 0)
		{
			Voci.Add(FString::Printf(TEXT("Nelle gallerie del nord hanno rimesso a dormire %s scheletri. A colpi di mazza, dicono: le lame ci rimbalzano. Me lo segno, se mai mi venisse voglia di scendere. Non mi verra'."),
				*VociNumero(F.Scheletri)));
		}
		if (F.bStoriaLucia)
		{
			Voci.Add(TEXT("Lucia della Gilda ha chiuso il registro, l'altro giorno. Lucia! Chi la conosce dice che aveva gli occhi rossi. Chi la conosce bene non dice niente, e beve alla sua salute."));
		}
		if (F.Orsi > 0)
		{
			Voci.Add(TEXT("Un orso! Un forestiero ha abbattuto un orso, e la pelle e' finita alla conceria. Gaspare e' passato a vederla e ha detto solo: \"Non era lui.\" Lui chi, non l'ha detto."));
		}
		if (F.Lupi > 0)
		{
			Voci.Add(F.Lupi == 1
				? FString(TEXT("Ai pascoli hanno fatto la pelle a un lupo. Elia dorme un po' meglio, e Fumo abbaia un po' meno. Le pecore non hanno ancora detto niente."))
				: FString::Printf(TEXT("Ai pascoli hanno fatto la pelle a %s lupi, da quando si apre la porta la mattina. Elia dorme meglio, e Fumo abbaia meno. Ma i lupi sono come le tasse: tornano."),
					*VociNumero(F.Lupi)));
		}
		if (F.Cinghiali > 0 && Voci.Num() < 3)
		{
			Voci.Add(TEXT("Qualcuno ha portato giu' un cinghiale dal boschetto. Se me lo vendono, domani zuppa di cinghiale. Se non me lo vendono, domani zuppa di cipolle. Come sempre."));
		}
		if (F.Morti > 0 && Voci.Num() < 3)
		{
			Voci.Add(F.Morti == 1
				? FString(TEXT("Un forestiero e' tornato dall'altare del tempio, l'altro giorno: mezzo morto e mezzo vivo, come si torna. Padre Anselmo dice che e' il battito che riporta indietro. Io dico: state attenti lo stesso."))
				: FString::Printf(TEXT("Dall'altare del tempio sono tornati in %s, ultimamente: mezzi morti e mezzi vivi, come si torna. Padre Anselmo dice che il battito li riporta. Io dico che un giorno si stanchera'."),
					*VociNumero(F.Morti)));
		}
		if (Voci.Num() == 0)
		{
			Voci.Add(TEXT("Niente di nuovo. Che e' gia' una notizia: quando la valle sta zitta, di solito sta ascoltando."));
		}
		if (Voci.Num() > 3)
		{
			Voci.SetNum(3);
		}
		return Voci;
	}

	TArray<FString> DiOggi(UWorld* Mondo)
	{
		return DaiFatti(Fatti(Mondo));
	}

	void RiempiCon(TArray<FString>& Righe, int32 AttoAdesso, const TArray<FString>& Voci)
	{
		static const FString SegnoVoci(TEXT("{voci}"));
		static const FString SegnoSe(TEXT("{se:atto1}"));
		static const FString SegnoSenza(TEXT("{senza:atto1}"));
		TArray<FString> Fuori;
		Fuori.Reserve(Righe.Num() + Voci.Num());
		for (const FString& Riga : Righe)
		{
			if (Riga == SegnoVoci)
			{
				Fuori.Append(Voci);
			}
			else if (Riga.StartsWith(SegnoSe))
			{
				if (AttoAdesso >= 1)
				{
					Fuori.Add(Riga.RightChop(SegnoSe.Len()).TrimStart());
				}
			}
			else if (Riga.StartsWith(SegnoSenza))
			{
				if (AttoAdesso < 1)
				{
					Fuori.Add(Riga.RightChop(SegnoSenza.Len()).TrimStart());
				}
			}
			else
			{
				Fuori.Add(Riga);
			}
		}
		Righe = MoveTemp(Fuori);
	}

	void Riempi(UWorld* Mondo, TArray<FString>& Righe)
	{
		bool bServe = false;
		for (const FString& Riga : Righe)
		{
			if (Riga.StartsWith(TEXT("{")))
			{
				bServe = true;
				break;
			}
		}
		if (!bServe)
		{
			return;
		}
		const bool bVoci = Righe.Contains(FString(TEXT("{voci}")));
		RiempiCon(Righe, Atto(Mondo), bVoci ? DiOggi(Mondo) : TArray<FString>());
	}
}

#undef LOCTEXT_NAMESPACE
