// Valdorso - Le storie del Battito nel gioco (vedi il .h). Scritto da Claude il 14/11/2026.
// Unity build: le cose del namespace anonimo cominciano con Battito.

#include "ValdorsoBattito.h"
#include "ValdorsoLuceBattito.h"
#include "ValdorsoPNG.h"
#include "ValdorsoCielo.h"
#include "ValdorsoChat.h"
#include "ValdorsoFiducia.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoQualita.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Valdorso.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoBattito"

namespace
{
	FString BattitoStringa(const TSharedPtr<FJsonObject>& O, const TCHAR* Campo)
	{
		FString Fuori;
		if (O.IsValid())
		{
			O->TryGetStringField(Campo, Fuori);
		}
		return Fuori.Left(2000);
	}

	int32 BattitoNumero(const TSharedPtr<FJsonObject>& O, const TCHAR* Campo, int32 Altrimenti)
	{
		double Valore = 0.0;
		return (O.IsValid() && O->TryGetNumberField(Campo, Valore)) ? static_cast<int32>(Valore) : Altrimenti;
	}

	TArray<TSharedPtr<FJsonObject>> BattitoOggetti(const TSharedPtr<FJsonObject>& O, const TCHAR* Campo)
	{
		TArray<TSharedPtr<FJsonObject>> Fuori;
		const TArray<TSharedPtr<FJsonValue>>* Valori = nullptr;
		if (O.IsValid() && O->TryGetArrayField(Campo, Valori) && Valori)
		{
			for (const TSharedPtr<FJsonValue>& V : *Valori)
			{
				const TSharedPtr<FJsonObject>* Dentro = nullptr;
				if (V.IsValid() && V->TryGetObject(Dentro) && Dentro)
				{
					Fuori.Add(*Dentro);
				}
			}
		}
		return Fuori;
	}

	TArray<FValdorsoStoriaScritta::FEffetto> BattitoEffetti(const TSharedPtr<FJsonObject>& O)
	{
		TArray<FValdorsoStoriaScritta::FEffetto> Fuori;
		for (const TSharedPtr<FJsonObject>& E : BattitoOggetti(O, TEXT("effetti")))
		{
			FValdorsoStoriaScritta::FEffetto& Nuovo = Fuori.AddDefaulted_GetRef();
			Nuovo.Tipo = BattitoStringa(E, TEXT("tipo"));
			Nuovo.Png = BattitoStringa(E, TEXT("png"));
			Nuovo.Testo = BattitoStringa(E, TEXT("testo"));
			Nuovo.Quanto = FMath::Clamp(BattitoNumero(E, TEXT("quanto"), 0), -2, 2);
		}
		return Fuori;
	}

	/** I PNG delle storie (mattoni.json, "png_nel_gioco") e i luoghi delle storie che non sono nel taccuino. */
	struct FBattitoNome
	{
		const TCHAR* Nome;
		const TCHAR* Altro;
	};

	const FBattitoNome BattitoPng[] = {
		{ TEXT("Gaspare"), TEXT("Gaspare") },
		{ TEXT("Tebaldo"), TEXT("GuardiaCorrado") },
		{ TEXT("Fulvio"), TEXT("GuardiaOttone") },
		{ TEXT("Nonna Edda"), TEXT("NonnaEdda") },
		{ TEXT("Bruno"), TEXT("Bruno") },
		{ TEXT("Padre Anselmo"), TEXT("PadreAnselmo") },
		{ TEXT("Lucia"), TEXT("Lucia") },
		{ TEXT("Mastro Ettore"), TEXT("CapoGilda") },
		{ TEXT("Marta"), TEXT("Marta") },
		{ TEXT("Livia Serra"), TEXT("Livia") },
		{ TEXT("Livia"), TEXT("Livia") },
		{ TEXT("Tobia"), TEXT("Tobia") },
		{ TEXT("Teodoro"), TEXT("Teodoro") },
		{ TEXT("Agnese"), TEXT("Agnese") },
		{ TEXT("Maestro Elric"), TEXT("Elric") },
		{ TEXT("Elric"), TEXT("Elric") },
		{ TEXT("Sibilla"), TEXT("Sibilla") },
	};

	const FBattitoNome BattitoLuoghiPng[] = {
		{ TEXT("Locanda di Bruno"), TEXT("Bruno") },
		{ TEXT("Fucina"), TEXT("Marta") },
		{ TEXT("Ufficio della Gilda"), TEXT("Lucia") },
		{ TEXT("Margine del bosco"), TEXT("Gaspare") },
	};

	const FBattitoNome BattitoLuoghiTaccuino[] = {
		{ TEXT("Pozzo"), TEXT("Piazza del villaggio") },
	};

	/** I mattoni che il gioco sa fare (v0.2.4). */
	const TCHAR* const BattitoMattoni[] = {
		TEXT("parla"), TEXT("vai"), TEXT("porta"), TEXT("raccogli"), TEXT("caccia"), TEXT("notte"), TEXT("veglia"),
		TEXT("siedi"), TEXT("dormi"), TEXT("leggi"), TEXT("incidi"), TEXT("scegli"), TEXT("rovista"), TEXT("ascolta"),
		TEXT("offerta"), TEXT("segui"), TEXT("fantasma"),
	};

	FString BattitoAdessoIso()
	{
		return FDateTime::UtcNow().ToIso8601();
	}

	FString BattitoPerJson(const FString& Testo)
	{
		FString Fuori = Testo.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));
		Fuori.ReplaceInline(TEXT("\r"), TEXT(" "));
		Fuori.ReplaceInline(TEXT("\n"), TEXT(" "));
		return Fuori;
	}

	FString BattitoCartella()
	{
		return FPaths::ProjectSavedDir() / TEXT("Server") / TEXT("Sognatore");
	}

	FString BattitoNomeColono(const APlayerController* Chi)
	{
		const APlayerState* Stato = Chi ? Chi->PlayerState.Get() : nullptr;
		return Stato ? Stato->GetPlayerName() : FString();
	}

	EValdorsoStatoStoria BattitoStato(const FValdorsoStoriaViva& Viva)
	{
		return static_cast<EValdorsoStatoStoria>(FMath::Min<uint8>(Viva.Stato, 5));
	}

	/** La storia viva di quel colono (nullptr se non c'e'). */
	const FValdorsoStoriaViva* BattitoViva(const UValdorsoTaccuinoComponent* Taccuino, const FString& Id)
	{
		return Taccuino ? Taccuino->StorieVive().FindByPredicate([&Id](const FValdorsoStoriaViva& S) { return S.Id == Id; }) : nullptr;
	}

	/** L'ora della valle (12 se non c'e' il cielo). */
	float BattitoOra(const UWorld* Mondo)
	{
		if (Mondo)
		{
			for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
			{
				return It->Ora();
			}
		}
		return 12.f;
	}

	AValdorsoPNG* BattitoPngDi(const UWorld* Mondo, FName Dialogo)
	{
		if (!Mondo || Dialogo.IsNone())
		{
			return nullptr;
		}
		for (TActorIterator<AValdorsoPNG> It(Mondo); It; ++It)
		{
			if (It->Dialogo == Dialogo)
			{
				return *It;
			}
		}
		return nullptr;
	}
}

// ------------------------------------------------------------------------------------------------- le regole

namespace ValdorsoBattito
{
	bool Leggi(const FString& Json, FValdorsoStoriaScritta& Out, FString& OutErrore)
	{
		TSharedPtr<FJsonObject> Radice;
		const TSharedRef<TJsonReader<>> Lettore = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Lettore, Radice) || !Radice.IsValid())
		{
			OutErrore = TEXT("non e' un JSON");
			return false;
		}
		Out = FValdorsoStoriaScritta();
		Out.Id = BattitoStringa(Radice, TEXT("id")).Left(128);
		Out.Seme = BattitoStringa(Radice, TEXT("seme")).Left(128);
		Out.Colono = BattitoStringa(Radice, TEXT("colono"));
		Out.Titolo = BattitoStringa(Radice, TEXT("titolo")).Left(120);
		Out.Rarita = BattitoStringa(Radice, TEXT("rarita")).ToLower();
		Out.Png = BattitoStringa(Radice, TEXT("png"));
		Out.Luogo = BattitoStringa(Radice, TEXT("luogo"));
		Out.Aggancio = BattitoStringa(Radice, TEXT("aggancio"));
		Out.Difficolta = FMath::Clamp(BattitoNumero(Radice, TEXT("difficolta"), 1), 1, 5);
		if (Out.Id.IsEmpty() || Out.Titolo.IsEmpty())
		{
			OutErrore = TEXT("senza id o titolo");
			return false;
		}
		for (const TSharedPtr<FJsonObject>& I : BattitoOggetti(Radice, TEXT("innesco")))
		{
			FValdorsoStoriaScritta::FInnesco& Nuovo = Out.Inneschi.AddDefaulted_GetRef();
			Nuovo.Tipo = BattitoStringa(I, TEXT("tipo")).ToLower();
			Nuovo.Png = BattitoStringa(I, TEXT("png"));
			Nuovo.Luogo = BattitoStringa(I, TEXT("luogo"));
			Nuovo.Evento = BattitoStringa(I, TEXT("evento"));
			Nuovo.Ora = BattitoStringa(I, TEXT("ora")).ToLower();
			Nuovo.Storia = BattitoStringa(I, TEXT("storia"));
			Nuovo.Quando = BattitoStringa(I, TEXT("quando")).ToLower();
		}
		for (const TSharedPtr<FJsonObject>& P : BattitoOggetti(Radice, TEXT("passi")))
		{
			FValdorsoStoriaScritta::FPasso& Nuovo = Out.Passi.AddDefaulted_GetRef();
			Nuovo.Mattone = BattitoStringa(P, TEXT("mattone")).ToLower();
			Nuovo.Testo = BattitoStringa(P, TEXT("testo"));
			Nuovo.Png = BattitoStringa(P, TEXT("png"));
			Nuovo.Luogo = BattitoStringa(P, TEXT("luogo"));
			Nuovo.Oggetto = BattitoStringa(P, TEXT("oggetto"));
			Nuovo.Animale = BattitoStringa(P, TEXT("animale"));
		}
		for (const TSharedPtr<FJsonObject>& F : BattitoOggetti(Radice, TEXT("finali")))
		{
			FValdorsoStoriaScritta::FFinale& Nuovo = Out.Finali.AddDefaulted_GetRef();
			Nuovo.Scelta = BattitoStringa(F, TEXT("scelta"));
			Nuovo.Esito = BattitoStringa(F, TEXT("esito"));
			Nuovo.Effetti = BattitoEffetti(F);
			const TSharedPtr<FJsonObject>* Premio = nullptr;
			if (F->TryGetObjectField(TEXT("premio"), Premio) && Premio && Premio->IsValid())
			{
				Nuovo.Monete = FMath::Clamp(BattitoNumero(*Premio, TEXT("monete"), 0), 0, 1000);
				Nuovo.Segno = BattitoStringa(*Premio, TEXT("segno")).Left(64);
				Nuovo.Iscrizione = BattitoStringa(*Premio, TEXT("iscrizione")).Left(60);
				Nuovo.Titolo = BattitoStringa(*Premio, TEXT("titolo")).Left(60);
				(*Premio)->TryGetStringArrayField(TEXT("oggetti"), Nuovo.Oggetti);
				if (Nuovo.Oggetti.Num() > 5)
				{
					Nuovo.Oggetti.SetNum(5);
				}
			}
		}
		const TSharedPtr<FJsonObject>* Rifiuto = nullptr;
		if (Radice->TryGetObjectField(TEXT("rifiuto"), Rifiuto) && Rifiuto && Rifiuto->IsValid())
		{
			Out.RifiutoTesto = BattitoStringa(*Rifiuto, TEXT("testo"));
			Out.RifiutoEffetti = BattitoEffetti(*Rifiuto);
		}
		// la finestra: quella del Sognatore, e quello che manca lo si fa qui
		FValdorsoFinestraStoria& W = Out.Finestra;
		const TSharedPtr<FJsonObject>* Finestra = nullptr;
		const bool bFinestra = Radice->TryGetObjectField(TEXT("finestra"), Finestra) && Finestra && Finestra->IsValid();
		const TSharedPtr<FJsonObject> FF = bFinestra ? *Finestra : TSharedPtr<FJsonObject>();
		W.Id = Out.Id;
		W.Intestazione = bFinestra ? BattitoStringa(FF, TEXT("intestazione")) : FString();
		if (W.Intestazione.IsEmpty())
		{
			W.Intestazione = TEXT("Il Battito ha intrecciato una storia");
		}
		W.Rarita = Out.Rarita.IsEmpty() ? FString(TEXT("comune")) : Out.Rarita;
		W.Colore = bFinestra ? BattitoStringa(FF, TEXT("colore")) : FString();
		W.Stelle = FMath::Clamp(bFinestra ? BattitoNumero(FF, TEXT("stelle"), Out.Difficolta) : Out.Difficolta, 1, 5);
		W.Titolo = Out.Titolo;
		W.Trama = bFinestra ? BattitoStringa(FF, TEXT("trama")) : FString();
		if (W.Trama.IsEmpty())
		{
			W.Trama = Out.Aggancio;
		}
		W.PrimoPasso = bFinestra ? BattitoStringa(FF, TEXT("primo_passo")) : FString();
		if (W.PrimoPasso.IsEmpty() && Out.Passi.Num() > 0)
		{
			W.PrimoPasso = Out.Passi[0].Testo;
		}
		W.Premi = bFinestra ? BattitoStringa(FF, TEXT("premi")) : FString();
		W.SeRifiuti = bFinestra ? BattitoStringa(FF, TEXT("se_rifiuti")) : FString();
		if (W.SeRifiuti.IsEmpty())
		{
			W.SeRifiuti = Out.RifiutoTesto;
		}
		W.ScadenzaGiorni = FMath::Clamp(bFinestra ? BattitoNumero(FF, TEXT("scadenza_giorni"), ScadenzaDi(W.Rarita)) : ScadenzaDi(W.Rarita), 1, 30);
		W.Serie = bFinestra ? BattitoStringa(FF, TEXT("serie")) : FString();
		return true;
	}

	bool MattoneOggi(const FString& Mattone)
	{
		for (const TCHAR* M : BattitoMattoni)
		{
			if (Mattone.Equals(M, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	bool Giocabile(const FValdorsoStoriaScritta& Storia)
	{
		if (Storia.Passi.Num() == 0 || Storia.Finali.Num() == 0)
		{
			return false;
		}
		for (const FValdorsoStoriaScritta::FPasso& P : Storia.Passi)
		{
			if (!MattoneOggi(P.Mattone))
			{
				return false;
			}
		}
		return true;
	}

	FName DialogoDelPng(const FString& Nome)
	{
		const FString Pulito = Nome.TrimStartAndEnd();
		if (Pulito.IsEmpty())
		{
			return NAME_None;
		}
		for (const FBattitoNome& N : BattitoPng)
		{
			if (Pulito.Equals(N.Nome, ESearchCase::IgnoreCase) || Pulito.Equals(N.Altro, ESearchCase::IgnoreCase))
			{
				return FName(N.Altro);
			}
		}
		return NAME_None;
	}

	FName PngDelLuogo(const FString& Luogo)
	{
		for (const FBattitoNome& N : BattitoLuoghiPng)
		{
			if (Luogo.TrimStartAndEnd().Equals(N.Nome, ESearchCase::IgnoreCase))
			{
				return FName(N.Altro);
			}
		}
		return NAME_None;
	}

	FString LuogoDelTaccuino(const FString& Luogo)
	{
		const FString Pulito = Luogo.TrimStartAndEnd();
		for (const FBattitoNome& N : BattitoLuoghiTaccuino)
		{
			if (Pulito.Equals(N.Nome, ESearchCase::IgnoreCase))
			{
				return N.Altro;
			}
		}
		const FValdorsoLuogo* Trovato = ValdorsoLuoghi::Trova(Pulito);
		return Trovato ? Trovato->Nome : FString();
	}

	uint8 QualitaDelPremio(const FString& Rarita)
	{
		if (Rarita == TEXT("insolita"))
		{
			return ValdorsoQualita::Buona;
		}
		if (Rarita == TEXT("rara"))
		{
			return ValdorsoQualita::Rara;
		}
		if (Rarita == TEXT("epica"))
		{
			return ValdorsoQualita::Epica;
		}
		if (Rarita == TEXT("leggendaria"))
		{
			return ValdorsoQualita::Leggendaria;
		}
		return ValdorsoQualita::Comune;
	}

	int32 ScadenzaDi(const FString& Rarita)
	{
		if (Rarita == TEXT("rara"))
		{
			return 5;
		}
		if (Rarita == TEXT("epica") || Rarita == TEXT("leggendaria"))
		{
			return 3;
		}
		return 7;
	}

	FString NomeRarita(const FString& Rarita)
	{
		const uint8 Q = QualitaDelPremio(Rarita);
		return Rarita == TEXT("insolita") ? FString(TEXT("Insolita")) : (Q == ValdorsoQualita::Comune ? FString(TEXT("Comune")) : FString(ValdorsoQualita::Nome(Q)));
	}

	FLinearColor ColoreRarita(const FString& Rarita, const FString& Esadecimale)
	{
		const FString Codice = Esadecimale.TrimStartAndEnd().Replace(TEXT("#"), TEXT(""));
		if (Codice.Len() == 6)
		{
			return FLinearColor(FColor::FromHex(Codice));
		}
		return ValdorsoQualita::Colore(QualitaDelPremio(Rarita));
	}

	bool InnescoDaEvento(const FValdorsoStoriaScritta::FInnesco& Innesco, const FString& Tipo, FName DialogoPng)
	{
		if (Innesco.Tipo == TEXT("dialogo"))
		{
			return Tipo == TEXT("Dialogo") && !DialogoPng.IsNone() && DialogoDelPng(Innesco.Png) == DialogoPng;
		}
		if (Innesco.Tipo == TEXT("evento"))
		{
			return !Innesco.Evento.IsEmpty() && Tipo.Equals(Innesco.Evento, ESearchCase::IgnoreCase);
		}
		if (Innesco.Tipo == TEXT("sogno"))
		{
			return Tipo == TEXT("Sogno");
		}
		return false;
	}

	FString NomeCartella(const FString& Colono)
	{
		FString Fuori;
		for (const TCHAR C : Colono)
		{
			Fuori.AppendChar((FChar::IsAlnum(C) || C == TEXT('_') || C == TEXT('-') || C == TEXT(' ')) ? C : TEXT('_'));
		}
		return Fuori;
	}

	FString TestoPasso(const FValdorsoStoriaScritta& Storia, int32 Passo)
	{
		if (!Storia.Passi.IsValidIndex(Passo))
		{
			return Storia.Titolo;
		}
		const FValdorsoStoriaScritta::FPasso& P = Storia.Passi[Passo];
		FString Aiuto;
		const FString Png = P.Png.IsEmpty() ? Storia.Png : P.Png;
		if (P.Mattone == TEXT("scegli") || P.Mattone == TEXT("parla") || P.Mattone == TEXT("porta"))
		{
			Aiuto = Png.IsEmpty() ? FString() : FString::Printf(TEXT(" (da %s)"), *Png);
		}
		else if (P.Mattone == TEXT("veglia") || P.Mattone == TEXT("ascolta"))
		{
			Aiuto = TEXT(" (resta li' due minuti)");
		}
		else if (P.Mattone == TEXT("notte"))
		{
			Aiuto = TEXT(" (di notte)");
		}
		else if (P.Mattone == TEXT("segui"))
		{
			Aiuto = TEXT(" (stalle vicino)");
		}
		return FString::Printf(TEXT("%s: %s%s"), *Storia.Titolo, *P.Testo, *Aiuto);
	}
}

// ------------------------------------------------------------------------------------------------- il sottosistema

UValdorsoBattito* UValdorsoBattito::Di(const UObject* QualcosaNelMondo)
{
	const UWorld* Mondo = QualcosaNelMondo ? QualcosaNelMondo->GetWorld() : nullptr;
	return Mondo ? Mondo->GetSubsystem<UValdorsoBattito>() : nullptr;
}

bool UValdorsoBattito::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* Mondo = Cast<UWorld>(Outer);
	return Mondo && Mondo->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
}

void UValdorsoBattito::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() != NM_Client)
	{
		InWorld.GetTimerManager().SetTimer(TimerGiro, FTimerDelegate::CreateUObject(this, &UValdorsoBattito::Giro), 1.f, true, 2.f);
	}
}

void UValdorsoBattito::Deinitialize()
{
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerGiro);
	}
	for (TPair<TWeakObjectPtr<APlayerController>, FColono>& Coppia : Coloni)
	{
		TogliLuce(Coppia.Value);
	}
	Coloni.Reset();
	Super::Deinitialize();
}

UValdorsoBattito::FColono& UValdorsoBattito::ColonoDi(APlayerController* Chi)
{
	return Coloni.FindOrAdd(TWeakObjectPtr<APlayerController>(Chi));
}

void UValdorsoBattito::Carica(APlayerController* Chi, FColono& Suo)
{
	Suo.Storie.Reset();
	Suo.LettoAlle = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const FString Nome = BattitoNomeColono(Chi);
	if (Nome.IsEmpty())
	{
		return;
	}
	const FString Cartella = BattitoCartella() / TEXT("Approvate") / ValdorsoBattito::NomeCartella(Nome);
	TArray<FString> File;
	IFileManager::Get().FindFiles(File, *(Cartella / TEXT("*.json")), true, false);
	File.Sort();
	for (const FString& Uno : File)
	{
		FString Testo;
		FString Errore;
		FValdorsoStoriaScritta Storia;
		if (FFileHelper::LoadFileToString(Testo, *(Cartella / Uno)) && ValdorsoBattito::Leggi(Testo, Storia, Errore))
		{
			Suo.Storie.Add(MoveTemp(Storia));
		}
		else
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Battito: la storia %s di %s non si legge (%s)."), *Uno, *Nome, *Errore);
		}
		if (Suo.Storie.Num() >= 100)
		{
			break;
		}
	}
}

const FValdorsoStoriaScritta* UValdorsoBattito::Scritta(FColono& Suo, const FString& Id) const
{
	return Suo.Storie.FindByPredicate([&Id](const FValdorsoStoriaScritta& S) { return S.Id == Id; });
}

bool UValdorsoBattito::Notte() const
{
	const float Ora = BattitoOra(GetWorld());
	return Ora >= 21.f || Ora < 5.f;
}

bool UValdorsoBattito::Dentro(const APawn* Colono, const FString& Luogo) const
{
	if (!Colono)
	{
		return false;
	}
	if (Luogo.TrimStartAndEnd().IsEmpty())
	{
		return true;
	}
	const FString NelTaccuino = ValdorsoBattito::LuogoDelTaccuino(Luogo);
	if (const FValdorsoLuogo* L = NelTaccuino.IsEmpty() ? nullptr : ValdorsoLuoghi::Trova(NelTaccuino))
	{
		return ValdorsoLuoghi::Dentro(*L, Colono->GetActorLocation());
	}
	if (const AValdorsoPNG* Png = BattitoPngDi(GetWorld(), ValdorsoBattito::PngDelLuogo(Luogo)))
	{
		return FVector::Dist2D(Png->GetActorLocation(), Colono->GetActorLocation()) <= 1500.f;
	}
	return false;
}

void UValdorsoBattito::AggiornaPagine(APlayerController* Chi, FColono& Suo)
{
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	if (!Taccuino)
	{
		return;
	}
	// l'ordine della scheda: in corso, da decidere, finite, lasciate andare (e dentro, le piu' nuove prima)
	auto Ordine = [](EValdorsoStatoStoria Stato)
	{
		switch (Stato)
		{
		case EValdorsoStatoStoria::Accettata: return 0;
		case EValdorsoStatoStoria::Offerta: return 1;
		case EValdorsoStatoStoria::Finita: return 2;
		default: return 3;
		}
	};
	TArray<const FValdorsoStoriaViva*> Vive;
	for (const FValdorsoStoriaViva& Viva : Taccuino->StorieVive())
	{
		Vive.Add(&Viva);
	}
	Vive.StableSort([&Ordine](const FValdorsoStoriaViva& A, const FValdorsoStoriaViva& B)
	{
		const int32 OA = Ordine(BattitoStato(A));
		const int32 OB = Ordine(BattitoStato(B));
		return OA != OB ? OA < OB : FMath::Max(A.Offerta, A.Chiusa) > FMath::Max(B.Offerta, B.Chiusa);
	});
	TArray<FValdorsoStoriaNelTaccuino> Pagine;
	for (const FValdorsoStoriaViva* Viva : Vive)
	{
		const FValdorsoStoriaScritta* Storia = Scritta(Suo, Viva->Id);
		if (!Storia || Pagine.Num() >= 60)
		{
			continue;
		}
		FValdorsoStoriaNelTaccuino& Pagina = Pagine.AddDefaulted_GetRef();
		Pagina.Titolo = Storia->Titolo;
		Pagina.Rarita = Storia->Rarita;
		Pagina.Colore = Storia->Finestra.Colore;
		Pagina.Stato = Viva->Stato;
		Pagina.Passi = Storia->Passi.Num();
		switch (BattitoStato(*Viva))
		{
		case EValdorsoStatoStoria::Accettata:
		{
			Pagina.Passo = Viva->Passo + 1;
			FString Testo = ValdorsoBattito::TestoPasso(*Storia, Viva->Passo);
			const FString Prima = Storia->Titolo + TEXT(": ");
			Pagina.Testo = Testo.StartsWith(Prima) ? Testo.RightChop(Prima.Len()) : Testo;
			break;
		}
		case EValdorsoStatoStoria::Offerta:
		{
			const FDateTime Scade = FDateTime::FromUnixTimestamp(Viva->Offerta + static_cast<int64>(Storia->Finestra.ScadenzaGiorni) * 86400);
			Pagina.Testo = FString::Printf(TEXT("Da decidere entro il %s. La finestra torna quando ci ripassi (o quando rientri nella valle)."),
				*Scade.ToString(TEXT("%d/%m")));
			break;
		}
		case EValdorsoStatoStoria::Finita:
			Pagina.Testo = Storia->Finali.IsValidIndex(Viva->Finale - 1) ? Storia->Finali[Viva->Finale - 1].Esito : FString();
			break;
		case EValdorsoStatoStoria::Scaduta:
			Pagina.Testo = TEXT("L'hai lasciata scadere.");
			break;
		default:
			Pagina.Testo = TEXT("L'hai rifiutata.");
			break;
		}
		Pagina.Testo = Pagina.Testo.Left(400);
	}
	Taccuino->ImpostaPagineStorie(Pagine);
}

void UValdorsoBattito::Racconta(APlayerController* Chi, const FString& Nome, const FString& Testo)
{
	if (Testo.IsEmpty())
	{
		return;
	}
	if (UValdorsoChatComponent* Chat = UValdorsoChatComponent::Di(Chi))
	{
		Chat->ClientSenti(Nome.IsEmpty() ? FString(TEXT("Il Battito")) : Nome, Testo.Left(400), false);
	}
}

void UValdorsoBattito::ScriviEsito(const FString& Colono, const FString& Id, const FString& Cosa)
{
	const FString File = BattitoCartella() / TEXT("Esiti_dal_gioco.jsonl");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
	const FString Riga = FString::Printf(TEXT("{\"colono\":\"%s\",\"storia\":\"%s\",\"cosa\":\"%s\",\"utc\":\"%s\"}%s"),
		*BattitoPerJson(Colono), *BattitoPerJson(Id), *BattitoPerJson(Cosa), *BattitoAdessoIso(), LINE_TERMINATOR);
	FFileHelper::SaveStringToFile(Riga, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}

void UValdorsoBattito::TogliLuce(FColono& Suo)
{
	if (AValdorsoLuceBattito* Luce = Suo.Luce.Get())
	{
		Luce->Destroy();
	}
	Suo.Luce.Reset();
	Suo.LuceDi.Reset();
}

bool UValdorsoBattito::Offri(APlayerController* Chi, FColono& Suo, const FValdorsoStoriaScritta& Storia)
{
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	UWorld* Mondo = GetWorld();
	if (!Taccuino || !Mondo || Suo.bFinestraAperta)
	{
		return false;
	}
	const FValdorsoStoriaViva* Gia = BattitoViva(Taccuino, Storia.Id);
	if (!Gia)
	{
		FValdorsoStoriaViva Nuova;
		Nuova.Id = Storia.Id;
		Nuova.Seme = Storia.Seme;
		Nuova.Stato = static_cast<uint8>(EValdorsoStatoStoria::Offerta);
		Nuova.Offerta = FDateTime::UtcNow().ToUnixTimestamp();
		Taccuino->CambiaStoria(Nuova);
		if (APawn* Colono = Chi->GetPawn())
		{
			ValdorsoRegistroEventi::Annota(TEXT("Battito"), Colono, nullptr, Colono->GetActorLocation(),
				FString::Printf(TEXT("storia offerta: %s (%s)"), *Storia.Titolo, *Storia.Rarita));
		}
	}
	else if (BattitoStato(*Gia) != EValdorsoStatoStoria::Offerta)
	{
		return false;
	}
	Suo.UltimaOfferta.Add(Storia.Id, Mondo->GetTimeSeconds());
	Suo.bFinestraAperta = true;
	Suo.FinestraDa = Mondo->GetTimeSeconds();
	Taccuino->ClientFinestraStoria(Storia.Finestra);
	return true;
}

void UValdorsoBattito::Risposta(APlayerController* Chi, const FString& IdStoria, uint8 Cosa)
{
	FColono& Suo = ColonoDi(Chi);
	Suo.bFinestraAperta = false;
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	const FValdorsoStoriaScritta* Storia = Scritta(Suo, IdStoria);
	const FValdorsoStoriaViva* Viva = BattitoViva(Taccuino, IdStoria);
	if (!Taccuino || !Storia || !Viva || BattitoStato(*Viva) != EValdorsoStatoStoria::Offerta)
	{
		return;
	}
	if (Cosa == 0)
	{
		FValdorsoStoriaViva Nuova = *Viva;
		Nuova.Stato = static_cast<uint8>(EValdorsoStatoStoria::Accettata);
		Nuova.Passo = 0;
		Taccuino->CambiaStoria(Nuova);
		ScriviEsito(BattitoNomeColono(Chi), Storia->Id, TEXT("accettata"));
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
		{
			Giocatore->ClientAvviso(FText::FromString(ValdorsoBattito::TestoPasso(*Storia, 0)), 6.f, 0.f);
		}
		Racconta(Chi, FString(), FString::Printf(TEXT("Hai accettato \"%s\". %s"), *Storia->Titolo, *ValdorsoBattito::TestoPasso(*Storia, 0)));
		return;
	}
	if (Cosa == 1)
	{
		Chiudi(Chi, Suo, *Storia, EValdorsoStatoStoria::Rifiutata);
		return;
	}
	const FDateTime Scade = FDateTime::FromUnixTimestamp(Viva->Offerta + static_cast<int64>(Storia->Finestra.ScadenzaGiorni) * 86400);
	Racconta(Chi, FString(), FString::Printf(TEXT("\"%s\" ti aspetta fino al %s. Tornera' quando ci ripasserai."), *Storia->Titolo,
		*Scade.ToString(TEXT("%d/%m"))));
}

void UValdorsoBattito::Chiudi(APlayerController* Chi, FColono& Suo, const FValdorsoStoriaScritta& Storia, EValdorsoStatoStoria Come)
{
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	const FValdorsoStoriaViva* Viva = BattitoViva(Taccuino, Storia.Id);
	if (!Taccuino || !Viva)
	{
		return;
	}
	FValdorsoStoriaViva Nuova = *Viva;
	Nuova.Stato = static_cast<uint8>(Come);
	Nuova.Chiusa = FDateTime::UtcNow().ToUnixTimestamp();
	Taccuino->CambiaStoria(Nuova);
	ScriviEsito(BattitoNomeColono(Chi), Storia.Id, Come == EValdorsoStatoStoria::Scaduta ? TEXT("scaduta") : TEXT("rifiutata"));
	Racconta(Chi, FString(), Storia.RifiutoTesto);
	Effetti(Chi, Storia.RifiutoEffetti, Storia);
	if (Suo.LuceDi == Storia.Id)
	{
		TogliLuce(Suo);
	}
}

void UValdorsoBattito::Effetti(APlayerController* Chi, const TArray<FValdorsoStoriaScritta::FEffetto>& Quali, const FValdorsoStoriaScritta& Storia)
{
	APawn* Colono = Chi ? Chi->GetPawn() : nullptr;
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	for (const FValdorsoStoriaScritta::FEffetto& E : Quali)
	{
		if (E.Tipo == TEXT("fiducia") && Taccuino)
		{
			const FName Id = ValdorsoBattito::DialogoDelPng(E.Png.IsEmpty() ? Storia.Png : E.Png);
			if (!Id.IsNone())
			{
				Taccuino->CambiaFiducia(Id, E.Quanto * 8);
			}
		}
		else if ((E.Tipo == TEXT("voce") || E.Tipo == TEXT("mondo")) && Colono && !E.Testo.IsEmpty())
		{
			// la voce gira (il registro: la Gazzetta e il Banditore la possono leggere)
			ValdorsoRegistroEventi::Annota(E.Tipo == TEXT("voce") ? TEXT("Voce") : TEXT("Mondo"), Colono, nullptr, Colono->GetActorLocation(), E.Testo.Left(200));
		}
		// "passa" e "chiusa" li fa il Sognatore leggendo l'esito
	}
}

void UValdorsoBattito::Avanza(APlayerController* Chi, FColono& Suo, const FValdorsoStoriaScritta& Storia, int32 PassoVisto)
{
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	const FValdorsoStoriaViva* Viva = BattitoViva(Taccuino, Storia.Id);
	if (!Taccuino || !Viva || BattitoStato(*Viva) != EValdorsoStatoStoria::Accettata || Viva->Passo != PassoVisto)
	{
		return;
	}
	Suo.Progresso.Remove(Storia.Id);
	if (Suo.LuceDi == Storia.Id)
	{
		TogliLuce(Suo);
	}
	const int32 Dopo = Viva->Passo + 1;
	if (Dopo >= Storia.Passi.Num())
	{
		Finisci(Chi, Suo, Storia, 1);
		return;
	}
	FValdorsoStoriaViva Nuova = *Viva;
	Nuova.Passo = Dopo;
	Taccuino->CambiaStoria(Nuova);
	const FString Testo = ValdorsoBattito::TestoPasso(Storia, Dopo);
	if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
	{
		Giocatore->ClientAvviso(FText::FromString(Testo), 6.f, 0.f);
	}
	Racconta(Chi, FString(), Testo);
}

void UValdorsoBattito::Finisci(APlayerController* Chi, FColono& Suo, const FValdorsoStoriaScritta& Storia, int32 Finale)
{
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	const FValdorsoStoriaViva* Viva = BattitoViva(Taccuino, Storia.Id);
	APawn* Colono = Chi ? Chi->GetPawn() : nullptr;
	if (!Taccuino || !Viva || !Storia.Finali.IsValidIndex(Finale - 1))
	{
		return;
	}
	FValdorsoStoriaViva Nuova = *Viva;
	Nuova.Stato = static_cast<uint8>(EValdorsoStatoStoria::Finita);
	Nuova.Finale = Finale;
	Nuova.Chiusa = FDateTime::UtcNow().ToUnixTimestamp();
	Taccuino->CambiaStoria(Nuova);
	if (Suo.LuceDi == Storia.Id)
	{
		TogliLuce(Suo);
	}
	const FValdorsoStoriaScritta::FFinale& F = Storia.Finali[Finale - 1];
	// i premi
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
	if (Zaino && F.Monete > 0)
	{
		const int32 Quante = F.Monete;
		Zaino->Cambia([Quante](FValdorsoZaino& Z)
		{
			Z.Monete = FMath::Min(Z.Monete + Quante, ValdorsoZaino::MassimoMonete);
			return true;
		});
	}
	const uint8 Grado = ValdorsoBattito::QualitaDelPremio(Storia.Rarita);
	for (const FString& Oggetto : F.Oggetti)
	{
		if (Zaino && !Oggetto.IsEmpty())
		{
			Zaino->Dai(FName(*Oggetto), 1, true, Grado, Grado > ValdorsoQualita::Comune ? FString::Printf(TEXT("Dono del Battito: %s"), *Storia.Titolo.Left(40)) : FString());
		}
	}
	const FName DialogoStoria = ValdorsoBattito::DialogoDelPng(Storia.Png);
	if (!F.Segno.IsEmpty())
	{
		Taccuino->MettiSegno(DialogoStoria.IsNone() ? FName(TEXT("Battito")) : DialogoStoria, FName(*F.Segno));
	}
	if (!F.Iscrizione.IsEmpty())
	{
		Taccuino->AnnotaIscrizione(F.Iscrizione, { (F.Esito.IsEmpty() ? Storia.Titolo : F.Esito).Left(ValdorsoTaccuino::MassimoLettere) });
	}
	if (!F.Titolo.IsEmpty())
	{
		Taccuino->MettiSegno(FName(TEXT("Battito")), FName(*FString::Printf(TEXT("Titolo:%s"), *F.Titolo.Left(50))));
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
		{
			Giocatore->ClientAvviso(FText::Format(LOCTEXT("Titolo", "Ora nella valle ti chiamano «{0}»."), FText::FromString(F.Titolo)), 6.f, 0.f);
		}
	}
	Effetti(Chi, F.Effetti, Storia);
	if (Colono)
	{
		ValdorsoRegistroEventi::Annota(TEXT("Storia"), Colono, nullptr, Colono->GetActorLocation(),
			FString::Printf(TEXT("Battito: %s (%s, finale %d)"), *Storia.Titolo, *Storia.Rarita, Finale));
	}
	if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
	{
		Giocatore->ClientAvviso(FText::Format(LOCTEXT("Finita", "Storia finita: {0}"), FText::FromString(Storia.Titolo)), 5.f, 0.f);
	}
	ScriviEsito(BattitoNomeColono(Chi), Storia.Id, FString::Printf(TEXT("finale%d"), Finale));
}

bool UValdorsoBattito::ApriScelta(AValdorsoPNG* Png, APawn* Colono)
{
	APlayerController* Chi = Colono ? Cast<APlayerController>(Colono->GetController()) : nullptr;
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	if (!Png || !Taccuino)
	{
		return false;
	}
	FColono& Suo = ColonoDi(Chi);
	for (const FValdorsoStoriaViva& Viva : Taccuino->StorieVive())
	{
		const FValdorsoStoriaScritta* Storia = BattitoStato(Viva) == EValdorsoStatoStoria::Accettata ? Scritta(Suo, Viva.Id) : nullptr;
		if (!Storia || !Storia->Passi.IsValidIndex(Viva.Passo) || Storia->Passi[Viva.Passo].Mattone != TEXT("scegli"))
		{
			continue;
		}
		const FValdorsoStoriaScritta::FPasso& P = Storia->Passi[Viva.Passo];
		if (ValdorsoBattito::DialogoDelPng(P.Png.IsEmpty() ? Storia->Png : P.Png) != Png->Dialogo)
		{
			continue;
		}
		TArray<FString> Scelte;
		for (const FValdorsoStoriaScritta::FFinale& F : Storia->Finali)
		{
			Scelte.Add(F.Scelta.IsEmpty() ? FString(TEXT("...")) : F.Scelta);
		}
		return Png->ApriSceltaStoria(Colono, Storia->Id, { P.Testo }, Scelte);
	}
	return false;
}

bool UValdorsoBattito::Scegli(APlayerController* Chi, const FString& IdStoria, int32 Indice, FString& OutEsito)
{
	FColono& Suo = ColonoDi(Chi);
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	const FValdorsoStoriaScritta* Storia = Scritta(Suo, IdStoria);
	const FValdorsoStoriaViva* Viva = BattitoViva(Taccuino, IdStoria);
	if (!Storia || !Viva || BattitoStato(*Viva) != EValdorsoStatoStoria::Accettata || !Storia->Finali.IsValidIndex(Indice)
		|| !Storia->Passi.IsValidIndex(Viva->Passo) || Storia->Passi[Viva->Passo].Mattone != TEXT("scegli"))
	{
		return false;
	}
	OutEsito = Storia->Finali[Indice].Esito;
	Finisci(Chi, Suo, *Storia, Indice + 1);
	return true;
}

void UValdorsoBattito::DopoAnnota(const FString& Tipo, const AActor* Chi, const AActor* Bersaglio, const FString& Dettagli)
{
	const APawn* Colono = Cast<APawn>(Chi);
	APlayerController* Giocatore = Colono ? Cast<APlayerController>(Colono->GetController()) : nullptr;
	UValdorsoBattito* Battito = Giocatore ? Di(Giocatore) : nullptr;
	// i suoi stessi eventi non contano
	if (!Battito || Tipo == TEXT("Battito") || !Giocatore->HasAuthority())
	{
		return;
	}
	// (un dialogo conta una volta: quando comincia, "parla con ...", non quando si chiude)
	const AValdorsoPNG* Png = Cast<AValdorsoPNG>(Bersaglio);
	const bool bInizio = Tipo != TEXT("Dialogo") || Dettagli.StartsWith(TEXT("parla con"));
	Battito->Evento(Giocatore, Tipo, (Png && bInizio) ? Png->Dialogo : FName(NAME_None), Dettagli);
}

void UValdorsoBattito::Evento(APlayerController* Chi, const FString& Tipo, FName DialogoPng, const FString& Dettagli)
{
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	APawn* Colono = Chi ? Chi->GetPawn() : nullptr;
	UWorld* Mondo = GetWorld();
	if (!Taccuino || !Colono || !Mondo)
	{
		return;
	}
	FColono& Suo = ColonoDi(Chi);
	if (Suo.bPrimoGiro)
	{
		return;  // le storie non sono ancora lette: ci pensa il primo giro
	}
	// i passi delle storie accettate (una copia: Avanza cambia il taccuino)
	const TArray<FValdorsoStoriaViva> Vive = Taccuino->StorieVive();
	for (const FValdorsoStoriaViva& Viva : Vive)
	{
		const FValdorsoStoriaScritta* Storia = BattitoStato(Viva) == EValdorsoStatoStoria::Accettata ? Scritta(Suo, Viva.Id) : nullptr;
		if (!Storia || !Storia->Passi.IsValidIndex(Viva.Passo))
		{
			continue;
		}
		const FValdorsoStoriaScritta::FPasso& P = Storia->Passi[Viva.Passo];
		const FName SuoPng = ValdorsoBattito::DialogoDelPng(P.Png.IsEmpty() ? Storia->Png : P.Png);
		bool bFatto = false;
		if (P.Mattone == TEXT("parla"))
		{
			bFatto = Tipo == TEXT("Dialogo") && !DialogoPng.IsNone() && DialogoPng == SuoPng;
		}
		else if (P.Mattone == TEXT("porta"))
		{
			if (Tipo == TEXT("Dialogo") && !DialogoPng.IsNone() && DialogoPng == SuoPng)
			{
				UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
				const FName Oggetto(*P.Oggetto);
				bFatto = Zaino && Zaino->Cambia([Oggetto](FValdorsoZaino& Z) { return ValdorsoZaino::TogliOggetto(Z, Oggetto, 1); });
				if (!bFatto)
				{
					Racconta(Chi, FString(), FString::Printf(TEXT("%s: ti serve %s."), *Storia->Titolo, *P.Oggetto.Replace(TEXT("_"), TEXT(" "))));
				}
			}
		}
		else if (P.Mattone == TEXT("caccia"))
		{
			bFatto = Tipo == TEXT("Animale") && (P.Animale.IsEmpty() || Dettagli.Contains(P.Animale, ESearchCase::IgnoreCase));
		}
		else if (P.Mattone == TEXT("siedi"))
		{
			bFatto = Tipo == TEXT("Siede") && Dentro(Colono, P.Luogo);
		}
		else if (P.Mattone == TEXT("dormi"))
		{
			bFatto = Tipo == TEXT("Dorme") || Tipo == TEXT("Sogno");
		}
		else if (P.Mattone == TEXT("leggi"))
		{
			bFatto = Tipo == TEXT("Legge") && Dentro(Colono, P.Luogo);
		}
		else if (P.Mattone == TEXT("incidi"))
		{
			bFatto = Tipo == TEXT("Segno");
		}
		else if (P.Mattone == TEXT("rovista"))
		{
			bFatto = (Tipo == TEXT("Rovista") || Tipo == TEXT("Forziere")) && Dentro(Colono, P.Luogo);
		}
		if (bFatto)
		{
			Avanza(Chi, Suo, *Storia, Viva.Passo);
		}
	}
	// gli inneschi delle storie non ancora offerte (e di quelle lasciate con "Ci penso")
	const double Adesso = Mondo->GetTimeSeconds();
	for (const FValdorsoStoriaScritta& Storia : Suo.Storie)
	{
		if (Suo.bFinestraAperta)
		{
			break;
		}
		if (!ValdorsoBattito::Giocabile(Storia))
		{
			continue;
		}
		const FValdorsoStoriaViva* Viva = BattitoViva(Taccuino, Storia.Id);
		if (Viva && BattitoStato(*Viva) != EValdorsoStatoStoria::Offerta)
		{
			continue;
		}
		const double* Ultima = Suo.UltimaOfferta.Find(Storia.Id);
		if (Ultima && Adesso - *Ultima < ValdorsoBattito::SecondiTraOfferte)
		{
			continue;
		}
		for (const FValdorsoStoriaScritta::FInnesco& I : Storia.Inneschi)
		{
			if (ValdorsoBattito::InnescoDaEvento(I, Tipo, DialogoPng))
			{
				Offri(Chi, Suo, Storia);
				break;
			}
		}
	}
}

void UValdorsoBattito::Giro()
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	const int64 AdessoUnix = FDateTime::UtcNow().ToUnixTimestamp();
	const bool bNotte = Notte();
	// via chi se n'e' andato
	for (auto It = Coloni.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			TogliLuce(It.Value());
			It.RemoveCurrent();
		}
	}
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Chi = It->Get();
		APawn* Colono = Chi ? Chi->GetPawn() : nullptr;
		UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
		if (!Colono || !Taccuino || BattitoNomeColono(Chi).IsEmpty())
		{
			continue;
		}
		FColono& Suo = ColonoDi(Chi);
		if (Suo.bPrimoGiro || Adesso - Suo.LettoAlle > 60.0)
		{
			Carica(Chi, Suo);
		}
		const bool bPrimo = Suo.bPrimoGiro;
		Suo.bPrimoGiro = false;
		// una finestra senza risposta da cinque minuti (il PC non l'ha potuta mostrare): si va avanti lo stesso
		if (Suo.bFinestraAperta && Adesso - Suo.FinestraDa > 300.0)
		{
			Suo.bFinestraAperta = false;
		}
		if (bPrimo)
		{
			// chi entra: le storie che ha in corso, in chat
			for (const FValdorsoStoriaViva& Viva : Taccuino->StorieVive())
			{
				const FValdorsoStoriaScritta* Storia = BattitoStato(Viva) == EValdorsoStatoStoria::Accettata ? Scritta(Suo, Viva.Id) : nullptr;
				if (Storia)
				{
					Racconta(Chi, FString(), ValdorsoBattito::TestoPasso(*Storia, Viva.Passo));
				}
			}
		}
		const TArray<FValdorsoStoriaViva> Vive = Taccuino->StorieVive();
		// le offerte scadute
		for (const FValdorsoStoriaViva& Viva : Vive)
		{
			const FValdorsoStoriaScritta* Storia = BattitoStato(Viva) == EValdorsoStatoStoria::Offerta ? Scritta(Suo, Viva.Id) : nullptr;
			if (Storia && AdessoUnix > Viva.Offerta + static_cast<int64>(Storia->Finestra.ScadenzaGiorni) * 86400)
			{
				Chiudi(Chi, Suo, *Storia, EValdorsoStatoStoria::Scaduta);
			}
		}
		// i passi che dipendono da dove sei, dall'ora, dallo zaino
		for (const FValdorsoStoriaViva& Viva : Vive)
		{
			const FValdorsoStoriaScritta* Storia = BattitoStato(Viva) == EValdorsoStatoStoria::Accettata ? Scritta(Suo, Viva.Id) : nullptr;
			if (!Storia || !Storia->Passi.IsValidIndex(Viva.Passo))
			{
				continue;
			}
			const FValdorsoStoriaScritta::FPasso& P = Storia->Passi[Viva.Passo];
			const bool bQui = Dentro(Colono, P.Luogo);
			float& Fatto = Suo.Progresso.FindOrAdd(Storia->Id);
			bool bAvanti = false;
			if (P.Mattone == TEXT("vai"))
			{
				bAvanti = bQui;
			}
			else if (P.Mattone == TEXT("notte"))
			{
				bAvanti = bQui && bNotte;
			}
			else if (P.Mattone == TEXT("veglia") || P.Mattone == TEXT("ascolta"))
			{
				Fatto = bQui ? Fatto + 1.f : 0.f;
				bAvanti = Fatto >= ValdorsoBattito::SecondiVeglia;
			}
			else if (P.Mattone == TEXT("raccogli"))
			{
				const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
				bAvanti = Zaino && !P.Oggetto.IsEmpty() && ValdorsoZaino::Conta(Zaino->Dati(), FName(*P.Oggetto)) > 0;
			}
			else if (P.Mattone == TEXT("offerta"))
			{
				// fermo nel luogo con il dono nello zaino: dopo qualche secondo lo lasci
				UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
				const FName Dono(*P.Oggetto);
				const bool bHa = Zaino && (P.Oggetto.IsEmpty() || ValdorsoZaino::Conta(Zaino->Dati(), Dono) > 0);
				Fatto = (bQui && bHa) ? Fatto + 1.f : 0.f;
				if (Fatto >= 8.f)
				{
					bAvanti = P.Oggetto.IsEmpty() || Zaino->Cambia([Dono](FValdorsoZaino& Z) { return ValdorsoZaino::TogliOggetto(Z, Dono, 1); });
					if (bAvanti)
					{
						Racconta(Chi, FString(), FString::Printf(TEXT("Lasci il tuo dono, %s. Per un attimo l'aria si fa piu' fredda."),
							*P.Oggetto.Replace(TEXT("_"), TEXT(" ")).ToLower()));
					}
				}
			}
			else if (P.Mattone == TEXT("segui") || P.Mattone == TEXT("fantasma"))
			{
				const bool bSpirito = P.Mattone == TEXT("fantasma");
				AValdorsoLuceBattito* Luce = Suo.LuceDi == Storia->Id ? Suo.Luce.Get() : nullptr;
				if (!bQui)
				{
					if (Luce)
					{
						TogliLuce(Suo);
					}
					Fatto = 0.f;
				}
				else if (!Luce && !Suo.Luce.IsValid())
				{
					const FVector Davanti = Colono->GetActorLocation() + Colono->GetActorForwardVector().GetSafeNormal2D() * (bSpirito ? 700.f : 500.f);
					Luce = AValdorsoLuceBattito::Accendi(Chi, Davanti, bSpirito);
					Suo.Luce = Luce;
					Suo.LuceDi = Storia->Id;
					if (Luce && !bSpirito)
					{
						Racconta(Chi, FString(), TEXT("Una lucina si accende davanti a te, e comincia ad allontanarsi..."));
					}
				}
				else if (Luce)
				{
					const float Distanza = static_cast<float>(FVector::Dist(Luce->GetActorLocation(), Colono->GetActorLocation()));
					if (bSpirito)
					{
						if (Distanza <= ValdorsoBattito::VicinoSpirito)
						{
							Racconta(Chi, TEXT("Lo spirito"), P.Testo);
							bAvanti = true;
						}
					}
					else
					{
						Fatto = Distanza <= ValdorsoBattito::VicinoLuce ? Fatto + 1.f : FMath::Max(0.f, Fatto - 2.f);
						bAvanti = Fatto >= ValdorsoBattito::SecondiSegui;
						if (Distanza > ValdorsoBattito::VicinoLuce * 2.5f)
						{
							Racconta(Chi, FString(), TEXT("La lucina si spegne: l'hai persa. Torna dove l'hai vista."));
							TogliLuce(Suo);
							Fatto = 0.f;
						}
					}
				}
			}
			if (bAvanti)
			{
				Avanza(Chi, Suo, *Storia, Viva.Passo);
			}
		}
		// la scheda Storie del taccuino (va in rete solo se e' cambiata)
		AggiornaPagine(Chi, Suo);
		// gli inneschi che dipendono da dove sei e dall'ora (e quelli sempre veri del Registro)
		if (Suo.bFinestraAperta || ValdorsoTaccuino::Aperto(Chi))
		{
			continue;
		}
		for (const FValdorsoStoriaScritta& Storia : Suo.Storie)
		{
			if (!ValdorsoBattito::Giocabile(Storia))
			{
				continue;
			}
			const FValdorsoStoriaViva* Viva = BattitoViva(Taccuino, Storia.Id);
			if (Viva && BattitoStato(*Viva) != EValdorsoStatoStoria::Offerta)
			{
				continue;
			}
			const double* Ultima = Suo.UltimaOfferta.Find(Storia.Id);
			if (Ultima && Adesso - *Ultima < ValdorsoBattito::SecondiTraOfferte)
			{
				continue;
			}
			bool bInnesco = false;
			for (const FValdorsoStoriaScritta::FInnesco& I : Storia.Inneschi)
			{
				const bool bOra = I.Ora.IsEmpty() || (I.Ora.Contains(TEXT("notte")) ? bNotte : !bNotte);
				if (I.Tipo == TEXT("luogo"))
				{
					bInnesco = bOra && Dentro(Colono, I.Luogo);
				}
				else if (I.Tipo == TEXT("registro"))
				{
					bInnesco = true;
				}
				else if (I.Tipo == TEXT("tempo"))
				{
					bInnesco = I.Quando.Contains(TEXT("notte")) ? bNotte : (I.Quando.Contains(TEXT("alba")) ? (BattitoOra(Mondo) >= 5.f && BattitoOra(Mondo) < 7.f) : true);
				}
				else if (I.Tipo == TEXT("storia"))
				{
					bInnesco = Taccuino->StorieVive().ContainsByPredicate([&I](const FValdorsoStoriaViva& S)
					{
						return BattitoStato(S) == EValdorsoStatoStoria::Finita && (S.Seme == I.Storia || S.Id == I.Storia);
					});
				}
				// (una storia "Ci penso" riaperta al primo giro: torna appena si rientra)
				bInnesco = bInnesco || (bPrimo && Viva);
				if (bInnesco)
				{
					break;
				}
			}
			if (bInnesco)
			{
				Offri(Chi, Suo, Storia);
				break;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------- le prove

void UValdorsoBattito::Rileggi()
{
	for (TPair<TWeakObjectPtr<APlayerController>, FColono>& Coppia : Coloni)
	{
		Coppia.Value.LettoAlle = -1000.0;
	}
}

void UValdorsoBattito::Dimentica(APlayerController* Chi)
{
	if (UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi))
	{
		Taccuino->DimenticaStorie();
	}
	FColono& Suo = ColonoDi(Chi);
	TogliLuce(Suo);
	Suo.Progresso.Reset();
	Suo.UltimaOfferta.Reset();
	Suo.bFinestraAperta = false;
	Suo.bPrimoGiro = true;
}

FString UValdorsoBattito::Elenco(APlayerController* Chi)
{
	FColono& Suo = ColonoDi(Chi);
	Carica(Chi, Suo);
	const UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
	TArray<FString> Righe;
	Righe.Add(FString::Printf(TEXT("Storie approvate per %s: %d"), *BattitoNomeColono(Chi), Suo.Storie.Num()));
	for (const FValdorsoStoriaScritta& S : Suo.Storie)
	{
		const FValdorsoStoriaViva* Viva = BattitoViva(Taccuino, S.Id);
		static const TCHAR* Nomi[] = { TEXT("aspetta l'innesco"), TEXT("offerta"), TEXT("accettata"), TEXT("finita"), TEXT("rifiutata"), TEXT("scaduta") };
		const int32 Stato = Viva ? static_cast<int32>(BattitoStato(*Viva)) : 0;
		Righe.Add(FString::Printf(TEXT("  %s  \"%s\" (%s)  %s%s%s"), *S.Id, *S.Titolo, *S.Rarita,
			ValdorsoBattito::Giocabile(S) ? Nomi[Stato] : TEXT("in attesa: usa mattoni che non ci sono ancora"),
			Viva && Stato == 2 ? *FString::Printf(TEXT(", passo %d di %d"), Viva->Passo + 1, S.Passi.Num()) : TEXT(""),
			Viva && Stato == 3 ? *FString::Printf(TEXT(", finale %d"), Viva->Finale) : TEXT("")));
	}
	return FString::Join(Righe, TEXT("\n"));
}

bool UValdorsoBattito::OffriPerProva(APlayerController* Chi, const FString& IdStoria)
{
	FColono& Suo = ColonoDi(Chi);
	Carica(Chi, Suo);
	Suo.bPrimoGiro = false;
	Suo.bFinestraAperta = false;
	const FValdorsoStoriaScritta* Storia = Scritta(Suo, IdStoria);
	if (!Storia)
	{
		return false;
	}
	return Offri(Chi, Suo, *Storia);
}

namespace
{
	APlayerController* BattitoGiocatore(UWorld* Mondo)
	{
		if (!Mondo)
		{
			return nullptr;
		}
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			if (It->Get() && It->Get()->HasAuthority())
			{
				return It->Get();
			}
		}
		return nullptr;
	}

	void BattitoComandoStorie(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		UValdorsoBattito* Battito = Mondo ? Mondo->GetSubsystem<UValdorsoBattito>() : nullptr;
		APlayerController* Chi = BattitoGiocatore(Mondo);
		if (!Battito || !Chi)
		{
			Uscita.Log(TEXT("Valdorso.Battito.Storie: solo sul server, con un colono in gioco."));
			return;
		}
		Uscita.Log(Battito->Elenco(Chi));
	}

	void BattitoComandoOffri(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		UValdorsoBattito* Battito = Mondo ? Mondo->GetSubsystem<UValdorsoBattito>() : nullptr;
		APlayerController* Chi = BattitoGiocatore(Mondo);
		if (!Battito || !Chi || Argomenti.Num() < 1)
		{
			Uscita.Log(TEXT("Valdorso.Battito.Offri <id della storia> (gli id: Valdorso.Battito.Storie)"));
			return;
		}
		Uscita.Log(Battito->OffriPerProva(Chi, Argomenti[0]) ? TEXT("Finestra aperta.") : TEXT("Non trovo quella storia (o e' gia' accettata, finita o rifiutata)."));
	}

	void BattitoComandoRileggi(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		if (UValdorsoBattito* Battito = Mondo ? Mondo->GetSubsystem<UValdorsoBattito>() : nullptr)
		{
			Battito->Rileggi();
			Uscita.Log(TEXT("Il Battito rilegge le storie approvate al prossimo secondo."));
		}
	}

	void BattitoComandoDimentica(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		UValdorsoBattito* Battito = Mondo ? Mondo->GetSubsystem<UValdorsoBattito>() : nullptr;
		if (APlayerController* Chi = BattitoGiocatore(Mondo); Battito && Chi)
		{
			Battito->Dimentica(Chi);
			Uscita.Log(TEXT("Le storie del Battito ripartono da capo (le finestre si riaprono)."));
		}
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice BattitoComando4(TEXT("Valdorso.Battito.Dimentica"),
		TEXT("Le storie del Battito del colono ripartono da capo (per le prove)."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&BattitoComandoDimentica));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice BattitoComando1(TEXT("Valdorso.Battito.Storie"),
		TEXT("Le storie del Battito del colono e a che punto sono."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&BattitoComandoStorie));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice BattitoComando2(TEXT("Valdorso.Battito.Offri"),
		TEXT("Apre la finestra di una storia del colono (per le prove). Valdorso.Battito.Offri <id>"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&BattitoComandoOffri));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice BattitoComando3(TEXT("Valdorso.Battito.Rileggi"),
		TEXT("Rilegge subito le storie approvate dai file del Sognatore."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&BattitoComandoRileggi));
}

#undef LOCTEXT_NAMESPACE
