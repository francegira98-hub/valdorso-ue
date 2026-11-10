// Valdorso - La Gilda nel mondo (vedi il .h). Scritto da Claude il 10/11/2026.

#include "ValdorsoGilda.h"
#include "SValdorsoGilda.h"
#include "Valdorso.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoGameMode.h"
#include "ValdorsoPermessi.h"
#include "ValdorsoStaff.h"
#include "ValdorsoChat.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoGilda"

namespace
{
	/** Dopo quanti secondi da un cambiamento si salva il personaggio. */
	constexpr double GildaSecondiSalvataggio = 4.0;
	/** Ogni quanto il server guarda le scadenze e i luoghi raggiunti (s). */
	constexpr double GildaSecondiControllo = 2.0;
	/** Una richiesta dal PC ogni tanto (s). */
	constexpr double GildaSecondiRichieste = 0.3;
	/** All'avvio gli abitanti mettono subito tanti incarichi; poi uno ogni GildaMinutiNuovo minuti, fino al massimo. */
	constexpr int32 GildaSubitoAllAvvio = 4;
	constexpr int32 GildaMinutiNuovo = 20;

	int32 GildaDegliAbitanti(const FValdorsoBachecheSalvate& B)
	{
		int32 N = 0;
		for (const FValdorsoIncarico& I : B.Incarichi)
		{
			N += I.bAbitanti ? 1 : 0;
		}
		return N;
	}

	/** Su ogni Gilda con autorita' del mondo (o di quello del Play, dalla console dell'editor), del giocatore con quel nome. */
	int32 GildaPerOgniGiocatore(UWorld* Mondo, const FString& Nome, TFunctionRef<void(UValdorsoGildaComponent&)> Fai)
	{
		auto NelMondo = [&Nome, &Fai](UWorld* W)
		{
			int32 Trovati = 0;
			for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* Controllore = It->Get();
				UValdorsoGildaComponent* Gilda = UValdorsoGildaComponent::Di(Controllore);
				if (!Gilda || !Controllore->HasAuthority())
				{
					continue;
				}
				const APlayerState* Stato = Controllore->PlayerState;
				if (!Nome.IsEmpty() && (!Stato || !Stato->GetPlayerName().Equals(Nome, ESearchCase::IgnoreCase)))
				{
					continue;
				}
				Fai(*Gilda);
				++Trovati;
			}
			return Trovati;
		};
		int32 Trovati = Mondo ? NelMondo(Mondo) : 0;
		if (Trovati > 0 || !GEngine)
		{
			return Trovati;
		}
		for (const FWorldContext& Contesto : GEngine->GetWorldContexts())
		{
			UWorld* Gioco = Contesto.World();
			if (Gioco && Gioco != Mondo && (Contesto.WorldType == EWorldType::PIE || Contesto.WorldType == EWorldType::Game))
			{
				Trovati += NelMondo(Gioco);
			}
		}
		return Trovati;
	}

	/** Le bacheche del mondo (o del Play). */
	UValdorsoBacheche* GildaBachecheDi(UWorld* Mondo)
	{
		if (UValdorsoBacheche* B = UValdorsoBacheche::Di(Mondo))
		{
			return B;
		}
		if (GEngine)
		{
			for (const FWorldContext& Contesto : GEngine->GetWorldContexts())
			{
				if (Contesto.WorldType == EWorldType::PIE || Contesto.WorldType == EWorldType::Game)
				{
					if (UValdorsoBacheche* B = UValdorsoBacheche::Di(Contesto.World()))
					{
						return B;
					}
				}
			}
		}
		return nullptr;
	}

	void GildaEsito(int32 Trovati, const FString& Cosa)
	{
		UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Gilda: %s"), Trovati > 0 ? *FString::Printf(TEXT("%s (%d giocatori)"), *Cosa, Trovati)
			: TEXT("nessun giocatore (il comando vale sul server, o giocando da soli)"));
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoGildaPunti(
		TEXT("Valdorso.Gilda.Punti"),
		TEXT("Per le prove: i punti della Gilda del colono, Valdorso.Gilda.Punti <n> [nome] (10 Bronzo, 40 Argento, 120 Oro)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const int32 Punti = Argomenti.Num() > 0 ? FMath::Max(0, FCString::Atoi(*Argomenti[0])) : 0;
			const FString Nome = Argomenti.Num() > 1 ? Argomenti[1] : FString();
			GildaEsito(GildaPerOgniGiocatore(Mondo, Nome, [Punti](UValdorsoGildaComponent& G) { G.ImpostaPunti(Punti); }),
				FString::Printf(TEXT("%d punti"), Punti));
		}));

	FAutoConsoleCommandWithWorldAndArgs ComandoGildaFatto(
		TEXT("Valdorso.Gilda.Fatto"),
		TEXT("Per le prove: gli incarichi assegnati del colono diventano fatti (le consegne no: servono le cose), Valdorso.Gilda.Fatto [nome]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const FString Nome = Argomenti.Num() > 0 ? Argomenti[0] : FString();
			GildaEsito(GildaPerOgniGiocatore(Mondo, Nome, [](UValdorsoGildaComponent& G) { G.FaiTutto(); }), TEXT("incarichi fatti"));
		}));

	FAutoConsoleCommandWithWorld ComandoGildaGenera(
		TEXT("Valdorso.Gilda.Genera"),
		TEXT("Per le prove: gli abitanti mettono subito un incarico sulla bacheca della Gilda (se ce ne sono meno di sei e la cassa basta)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* Mondo)
		{
			UValdorsoBacheche* B = GildaBachecheDi(Mondo);
			if (!B)
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Gilda: niente bacheche (vale sul server, o giocando da soli)"));
				return;
			}
			FRandomStream Caso(FPlatformTime::Cycles());
			const bool bMesso = ValdorsoGilda::Genera(B->Dati(), UValdorsoBacheche::Adesso(), Caso);
			B->Segna();
			UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Gilda: %s (cassa %d Aureus)"), bMesso ? TEXT("un incarico nuovo") : TEXT("niente di nuovo"), B->Dati().Cassa);
		}));

	FAutoConsoleCommandWithWorld ComandoGildaElenco(
		TEXT("Valdorso.Gilda.Elenco"),
		TEXT("Gli incarichi sulla bacheca della Gilda, la cassa del villaggio e i proclami."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* Mondo)
		{
			UValdorsoBacheche* B = GildaBachecheDi(Mondo);
			if (!B)
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Gilda: niente bacheche (vale sul server, o giocando da soli)"));
				return;
			}
			const FValdorsoBachecheSalvate& D = B->Dati();
			UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Gilda: cassa %d Aureus, %d incarichi, %d proclami"), D.Cassa, D.Incarichi.Num(), D.Proclami.Num());
			for (const FValdorsoIncarico& I : D.Incarichi)
			{
				UE_LOG(LogValdorso, Display, TEXT("  n. %d %s (%s, %s) presi %d su %d, compiuti %d, %s"), I.Numero, *I.Titolo,
					*ValdorsoGilda::TestoLavoro(I).ToString(), *ValdorsoGilda::NomeGrado(I.Grado).ToString(), I.Presi, I.Posti, I.Compiuti,
					*ValdorsoGilda::TestoScadenza(I.Scadenza, UValdorsoBacheche::Adesso()).ToString());
			}
			for (const FValdorsoProclama& P : D.Proclami)
			{
				UE_LOG(LogValdorso, Display, TEXT("  proclama n. %d: %s"), P.Numero, *P.Testo);
			}
		}));
}

// --- Le bacheche (sul server) ---------------------------------------------------------------------------------------

UValdorsoBacheche* UValdorsoBacheche::Di(const UObject* Contesto)
{
	UWorld* Mondo = Contesto ? Contesto->GetWorld() : nullptr;
	if (!Mondo || Mondo->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	UValdorsoBacheche* Quelle = Mondo->GetSubsystem<UValdorsoBacheche>();
	return Quelle && Quelle->bAttive ? Quelle : nullptr;
}

bool UValdorsoBacheche::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* Mondo = Cast<UWorld>(Outer);
	return Mondo && (Mondo->WorldType == EWorldType::Game || Mondo->WorldType == EWorldType::PIE);
}

int64 UValdorsoBacheche::Adesso()
{
	return FDateTime::UtcNow().ToUnixTimestamp();
}

FString UValdorsoBacheche::File() const
{
	const UWorld* Mondo = GetWorld();
	const UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	// le prove nell'editor non toccano le bacheche vere del server (anche se stanno sullo stesso PC)
	const TCHAR* Cartella = Mondo && Mondo->IsPlayInEditor() ? TEXT("Prova") : (Archivista && Archivista->ArchivioSolo() ? TEXT("Solo") : TEXT("Server"));
	return FPaths::ProjectSavedDir() / Cartella / TEXT("Bacheche.json");
}

FValdorsoBachecheSalvate& UValdorsoBacheche::Dati()
{
	if (!bCaricate)
	{
		Carica();
	}
	return Bacheche;
}

void UValdorsoBacheche::Carica()
{
	bCaricate = true;
	Bacheche = FValdorsoBachecheSalvate();
	FString Testo;
	const FString Percorso = File();
	if (FFileHelper::LoadFileToString(Testo, *Percorso)
		|| FFileHelper::LoadFileToString(Testo, *(Percorso + TEXT(".bak"))))
	{
		FValdorsoBachecheSalvate Letto;
		if (FJsonObjectConverter::JsonObjectStringToUStruct(Testo, &Letto, 0, 0))
		{
			Bacheche = MoveTemp(Letto);
		}
		else
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Gilda: %s non si legge: si ricomincia con le bacheche vuote"), *Percorso);
		}
	}
	ValdorsoGilda::Sistema(Bacheche);
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Gilda: bacheche da %s (%d incarichi, %d proclami, cassa %d)"), *Percorso,
		Bacheche.Incarichi.Num(), Bacheche.Proclami.Num(), Bacheche.Cassa);
}

void UValdorsoBacheche::Salva()
{
	if (!bCaricate)
	{
		return;
	}
	bDaSalvare = false;
	FString Testo;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Bacheche, Testo))
	{
		return;
	}
	const FString Percorso = File();
	const FString Nuovo = Percorso + TEXT(".nuovo");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Percorso), true);
	if (!FFileHelper::SaveStringToFile(Testo, *Nuovo, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Gilda: non riesco a scrivere %s"), *Nuovo);
		return;
	}
	// prima la copia di quello che c'era, poi il nuovo al suo posto (un file a meta' non resta mai)
	if (IFileManager::Get().FileExists(*Percorso))
	{
		IFileManager::Get().Copy(*(Percorso + TEXT(".bak")), *Percorso);
	}
	IFileManager::Get().Move(*Percorso, *Nuovo, true);
}

void UValdorsoBacheche::Segna()
{
	// si salva al prossimo battito dell'orologio (al massimo 10 secondi), o alla fine
	bDaSalvare = true;
}

void UValdorsoBacheche::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// solo nella valle (non nel mondo del menu) e non sul PC di chi gioca con un server
	if (InWorld.GetNetMode() == NM_Client || !Cast<AValdorsoGameMode>(InWorld.GetAuthGameMode()))
	{
		return;
	}
	bAttive = true;
	Caso.Initialize(static_cast<int32>(FPlatformTime::Cycles()));
	Dati();
	// all'avvio: i fogli degli abitanti, se la bacheca e' vuota
	const int64 Ora = Adesso();
	ValdorsoGilda::Rendita(Bacheche, Ora);
	ValdorsoGilda::Pulisci(Bacheche, Ora);
	for (int32 k = 0; k < GildaSubitoAllAvvio && GildaDegliAbitanti(Bacheche) < GildaSubitoAllAvvio; ++k)
	{
		if (!ValdorsoGilda::Genera(Bacheche, Ora, Caso))
		{
			break;
		}
		bDaSalvare = true;
	}
	// ogni 10 secondi si salva se serve; ogni minuto le scadenze, la cassa e i fogli nuovi
	InWorld.GetTimerManager().SetTimer(Orologio, FTimerDelegate::CreateUObject(this, &UValdorsoBacheche::Minuto), 10.f, true);
}

void UValdorsoBacheche::Minuto()
{
	// (il timer batte ogni 10 secondi: il lavoro del minuto una volta su sei)
	if (++Battiti % 6 == 0)
	{
		const int64 Ora = Adesso();
		ValdorsoGilda::Rendita(Bacheche, Ora);
		if (ValdorsoGilda::Pulisci(Bacheche, Ora) > 0)
		{
			bDaSalvare = true;
		}
		const int32 Minuti = Battiti / 6;
		if ((Minuti % GildaMinutiNuovo == 0 || GildaDegliAbitanti(Bacheche) < 2) && ValdorsoGilda::Genera(Bacheche, Ora, Caso))
		{
			bDaSalvare = true;
		}
	}
	if (bDaSalvare)
	{
		Salva();
	}
}

void UValdorsoBacheche::Deinitialize()
{
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(Orologio);
	}
	if (bDaSalvare)
	{
		Salva();
	}
	Super::Deinitialize();
}

bool UValdorsoBacheche::ComandoStaff(APlayerController* Chi, const FString& Riga, FString& OutRisposta)
{
	FString Comando;
	FString Resto;
	if (!Riga.Split(TEXT(" "), &Comando, &Resto))
	{
		Comando = Riga;
	}
	Comando = Comando.ToLower();
	Resto.TrimStartAndEndInline();
	static const TCHAR* Comandi[] = { TEXT("/proclama"), TEXT("/legge"), TEXT("/incarico"), TEXT("/togliincarico"), TEXT("/togliproclama"), TEXT("/cassa"), TEXT("/bacheche") };
	bool bNostro = false;
	for (const TCHAR* C : Comandi)
	{
		bNostro |= Comando == C;
	}
	if (!bNostro)
	{
		return false;
	}
	// (10/11) il permesso Bacheche (ValdorsoPermessi.h), non solo il ruolo
	if (!(UValdorsoStaffComponent::PermessiDi(Chi) & ValdorsoPermessi::Bacheche))
	{
		OutRisposta = TEXT("Sulle bacheche scrive solo lo staff con il permesso delle bacheche.");
		return true;
	}
	const FString Autore = Chi->PlayerState ? Chi->PlayerState->GetPlayerName() : FString(TEXT("staff"));
	FValdorsoBachecheSalvate& D = Dati();
	const int64 Ora = Adesso();
	if (Comando == TEXT("/bacheche"))
	{
		OutRisposta = TEXT("/proclama testo · /legge testo (da domani) · /incarico tipo obiettivo quanti grado monete posti giorni titolo | testo · /togliincarico n · /togliproclama n · /cassa");
		return true;
	}
	if (Comando == TEXT("/proclama") || Comando == TEXT("/legge"))
	{
		if (Resto.IsEmpty())
		{
			OutRisposta = TEXT("Scrivi il testo dopo il comando.");
			return true;
		}
		const FValdorsoProclama& P = ValdorsoGilda::Proclama(D, Resto, Autore, Comando == TEXT("/legge"), Ora);
		Segna();
		Salva();
		ValdorsoRegistroEventi::Annota(TEXT("Proclama"), Chi->GetPawn(), nullptr, Chi->GetPawn() ? Chi->GetPawn()->GetActorLocation() : FVector::ZeroVector,
			FString::Printf(TEXT("n. %d%s: %s"), P.Numero, P.bLegge ? TEXT(" (legge)") : TEXT(""), *P.Testo));
		OutRisposta = FString::Printf(TEXT("Proclama n. %d appeso alla bacheca del Balivo%s."), P.Numero, P.bLegge ? TEXT(" (legge: vale da domani)") : TEXT(""));
		return true;
	}
	if (Comando == TEXT("/incarico"))
	{
		FValdorsoIncarico Nuovo;
		FString Errore;
		if (!ValdorsoGilda::DaComando(Resto, Nuovo, Errore))
		{
			OutRisposta = Errore;
			return true;
		}
		Nuovo.Scadenza += Ora;
		Nuovo.Autore = Autore;
		if (!ValdorsoGilda::Metti(D, Nuovo, Ora))
		{
			OutRisposta = TEXT("La bacheca della Gilda e' piena.");
			return true;
		}
		Segna();
		Salva();
		const FValdorsoIncarico& Messo = D.Incarichi.Last();
		ValdorsoRegistroEventi::Annota(TEXT("Gilda"), Chi->GetPawn(), nullptr, FVector::ZeroVector,
			FString::Printf(TEXT("incarico dello staff n. %d: %s (%s)"), Messo.Numero, *Messo.Titolo, *ValdorsoGilda::TestoLavoro(Messo).ToString()));
		OutRisposta = FString::Printf(TEXT("Incarico n. %d appeso: %s (%s, %d Aureus)."), Messo.Numero, *Messo.Titolo.Left(60),
			*ValdorsoGilda::NomeGrado(Messo.Grado).ToString(), Messo.Monete);
		return true;
	}
	if (Comando == TEXT("/togliincarico") || Comando == TEXT("/togliproclama"))
	{
		const int32 Numero = Resto.IsNumeric() ? FCString::Atoi(*Resto) : 0;
		const int32 Tolti = Comando == TEXT("/togliincarico")
			? D.Incarichi.RemoveAll([Numero](const FValdorsoIncarico& I) { return I.Numero == Numero; })
			: D.Proclami.RemoveAll([Numero](const FValdorsoProclama& P) { return P.Numero == Numero; });
		if (Tolti > 0)
		{
			Segna();
			Salva();
		}
		OutRisposta = Tolti > 0 ? FString::Printf(TEXT("Tolto il n. %d."), Numero) : FString::Printf(TEXT("Non c'e' il n. %d (Valdorso.Gilda.Elenco)."), Numero);
		return true;
	}
	// /cassa
	OutRisposta = FString::Printf(TEXT("Cassa del villaggio: %d Aureus. Incarichi: %d (degli abitanti %d). Proclami: %d."),
		D.Cassa, D.Incarichi.Num(), GildaDegliAbitanti(D), D.Proclami.Num());
	return true;
}

// --- La Gilda del colono ---------------------------------------------------------------------------------------------

UValdorsoGildaComponent::UValdorsoGildaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;
	SetIsReplicatedByDefault(true);
}

UValdorsoGildaComponent* UValdorsoGildaComponent::Di(const APlayerController* Controllore)
{
	return Controllore ? Controllore->FindComponentByClass<UValdorsoGildaComponent>() : nullptr;
}

void UValdorsoGildaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UValdorsoGildaComponent, Gilda, COND_OwnerOnly);
}

void UValdorsoGildaComponent::OnRep_Gilda()
{
	++NumeroCambiamenti;
}

int64 UValdorsoGildaComponent::OraServer() const
{
	return FDateTime::UtcNow().ToUnixTimestamp() + Scarto;
}

FText UValdorsoGildaComponent::LetturaFoglio(int32 Numero) const
{
	const FValdorsoIncaricoPreso* P = ValdorsoGilda::Preso(Gilda, Numero);
	if (!P)
	{
		return LOCTEXT("FoglioVecchio", "Un foglio della Gilda che non vale piu'.");
	}
	const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Cast<APlayerController>(GetOwner()));
	const int32 Nello = Zaino ? ValdorsoZaino::Conta(Zaino->Contenuto(), P->Foglio.Obiettivo) : 0;
	return FText::Format(LOCTEXT("Lettura", "«{0}» ({1}). {2}. {3}"), FText::FromString(P->Foglio.Titolo), FText::FromString(P->Foglio.Committente),
		ValdorsoGilda::TestoLavoro(P->Foglio), ValdorsoGilda::TestoProgresso(*P, Nello));
}

void UValdorsoGildaComponent::Carica(const FValdorsoGildaColono& Salvato)
{
	Gilda = Salvato;
	ValdorsoGilda::Sistema(Gilda);
	bPersonaggio = true;
	++NumeroCambiamenti;
	Riallinea();
}

void UValdorsoGildaComponent::Avvisa(const FText& Testo, float Secondi) const
{
	if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(GetOwner()))
	{
		Controllore->ClientAvviso(Testo, Secondi, 0.f);
	}
}

void UValdorsoGildaComponent::Annota(const FString& Cosa) const
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
	ValdorsoRegistroEventi::Annota(TEXT("Gilda"), Corpo, Bancone.Get(), Corpo ? Corpo->GetActorLocation() : FVector::ZeroVector, Cosa);
}

void UValdorsoGildaComponent::Cambiato()
{
	++NumeroCambiamenti;
	Riallinea();
	if (bPersonaggio && !bDaSalvare)
	{
		bDaSalvare = true;
		const UWorld* Mondo = GetWorld();
		SalvaDopo = (Mondo ? Mondo->GetTimeSeconds() : 0.0) + GildaSecondiSalvataggio;
	}
}

void UValdorsoGildaComponent::Riallinea()
{
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Cast<APlayerController>(GetOwner()));
	if (!Zaino)
	{
		return;
	}
	TSet<int32> Numeri;
	for (const FValdorsoIncaricoPreso& P : Gilda.Presi)
	{
		Numeri.Add(P.Foglio.Numero);
	}
	bool bSenzaPosto = false;
	Zaino->Cambia([&Numeri, &bSenzaPosto](FValdorsoZaino& Z)
	{
		bool bCambiato = false;
		TSet<int32> Trovati;
		for (FValdorsoPostoZaino& Posto : Z.Posti)
		{
			if (Posto.Vuoto() || Posto.Id != ValdorsoGilda::IdFoglio)
			{
				continue;
			}
			// i fogli che non valgono piu' (o doppi) se ne vanno
			if (!Numeri.Contains(Posto.Numero) || Trovati.Contains(Posto.Numero))
			{
				Posto = FValdorsoPostoZaino();
				bCambiato = true;
				continue;
			}
			Trovati.Add(Posto.Numero);
		}
		for (const int32 N : Numeri)
		{
			if (!Trovati.Contains(N))
			{
				if (ValdorsoZaino::AggiungiNumerato(Z, ValdorsoGilda::IdFoglio, N) == INDEX_NONE)
				{
					bSenzaPosto = true;
				}
				else
				{
					bCambiato = true;
				}
			}
		}
		return bCambiato;
	});
	if (bSenzaPosto)
	{
		Avvisa(LOCTEXT("SenzaPostoFoglio", "Lo zaino e' pieno: un foglio della Gilda non ci sta. Fai posto e torna da Lucia."), 4.f);
	}
}

bool UValdorsoGildaComponent::AllaBacheca() const
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
	UWorld* Mondo = GetWorld();
	if (!Corpo || !Mondo)
	{
		return false;
	}
	for (TActorIterator<AValdorsoBacheca> It(Mondo); It; ++It)
	{
		if (It->Tipo == EValdorsoTipoBacheca::Gilda
			&& FVector::Dist(It->PuntoInterazione(), Corpo->GetActorLocation()) <= ValdorsoGilda::Raggio)
		{
			return true;
		}
	}
	return false;
}

bool UValdorsoGildaComponent::AlBancone() const
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
	const AActor* Lucia = Bancone.Get();
	// (un poco piu' largo della bacheca: si parla con Lucia fino a 4,5 m)
	return Corpo && Lucia && FVector::Dist(Lucia->GetActorLocation(), Corpo->GetActorLocation()) <= ValdorsoGilda::Raggio + 150.f;
}

void UValdorsoGildaComponent::ApriBacheca()
{
	UValdorsoBacheche* Bacheche = UValdorsoBacheche::Di(this);
	if (!Bacheche)
	{
		return;
	}
	const int64 Ora = UValdorsoBacheche::Adesso();
	if (ValdorsoGilda::Pulisci(Bacheche->Dati(), Ora) > 0)
	{
		Bacheche->Segna();
	}
	TArray<FValdorsoIncarico> Visti = ValdorsoGilda::Visibili(Bacheche->Dati(), Ora);
	if (Visti.Num() > ValdorsoGilda::MassimoIncarichi)
	{
		Visti.SetNum(ValdorsoGilda::MassimoIncarichi);
	}
	ClientBacheca(Visti, Ora);
}

void UValdorsoGildaComponent::ApriBancone(AActor* Lucia)
{
	if (!Lucia)
	{
		return;
	}
	Bancone = Lucia;
	ClientBancone(UValdorsoBacheche::Adesso());
}

void UValdorsoGildaComponent::Preda(AActor* Cacciatore, FName Specie)
{
	const APawn* Corpo = Cast<APawn>(Cacciatore);
	APlayerController* Controllore = Corpo ? Cast<APlayerController>(Corpo->GetController()) : nullptr;
	UValdorsoGildaComponent* Suo = Di(Controllore);
	if (!Suo || !Controllore->HasAuthority())
	{
		return;
	}
	if (ValdorsoGilda::Avanza(Suo->Gilda, EValdorsoTipoIncarico::Caccia, Specie) <= 0)
	{
		return;
	}
	for (const FValdorsoIncaricoPreso& P : Suo->Gilda.Presi)
	{
		if (P.Stato == EValdorsoStatoIncarico::Assegnato && P.Foglio.Tipo == EValdorsoTipoIncarico::Caccia && P.Foglio.Obiettivo == Specie)
		{
			Suo->Avvisa(FText::Format(LOCTEXT("Preda", "Gilda, «{0}»: {1}{2}"), FText::FromString(P.Foglio.Titolo), ValdorsoGilda::TestoProgresso(P, 0),
				P.Progresso >= P.Foglio.Quanti ? LOCTEXT("PredaFatto", ". Fatto: torna da Lucia.") : FText::GetEmpty()), 3.5f);
			break;
		}
	}
	Suo->Cambiato();
}

void UValdorsoGildaComponent::Esce()
{
	UValdorsoBacheche* Bacheche = UValdorsoBacheche::Di(this);
	if (!Bacheche || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	TArray<int32> Prenotati;
	for (const FValdorsoIncaricoPreso& P : Gilda.Presi)
	{
		if (P.Stato == EValdorsoStatoIncarico::Prenotato)
		{
			Prenotati.Add(P.Foglio.Numero);
		}
	}
	if (Prenotati.IsEmpty())
	{
		return;
	}
	for (const int32 Numero : Prenotati)
	{
		ValdorsoGilda::Rinuncia(Bacheche->Dati(), Gilda, Numero);
	}
	Bacheche->Segna();
	// i fogli escono dallo zaino adesso: il personaggio si salva subito dopo (PawnLeavingGame)
	Riallinea();
}

void UValdorsoGildaComponent::ImpostaPunti(int32 Punti)
{
	Gilda.Punti = FMath::Max(0, Punti);
	const bool bPronto = ValdorsoGilda::DaAvvisarePronto(Gilda);
	Cambiato();
	if (bPronto)
	{
		AvvisaChiNomina();
	}
	Avvisa(FText::Format(LOCTEXT("PuntiProva", "Punti della Gilda: {0} (grado {1}{2})."), FText::AsNumber(Gilda.Punti),
		ValdorsoGilda::NomeGrado(Gilda.Grado), ValdorsoGilda::Pronto(Gilda) ? LOCTEXT("PuntiPronto", ", pronto a salire") : FText::GetEmpty()), 3.f);
}

void UValdorsoGildaComponent::AvvisaChiNomina() const
{
	// (10/11) a chi da' i gradi (lo staff con il permesso della Gilda e il capo gilda), nella chat: c'e' uno pronto
	const APlayerController* Mio = Cast<APlayerController>(GetOwner());
	UWorld* Mondo = GetWorld();
	if (!Mio || !Mio->PlayerState || !Mondo)
	{
		return;
	}
	const FString Riga = FString::Printf(TEXT("Gilda: %s e' pronto per il grado %s (finestra dello staff, F9)."), *Mio->PlayerState->GetPlayerName(),
		*ValdorsoGilda::NomeGrado(ValdorsoGilda::GradoDopo(Gilda.Grado)).ToString());
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Altro = It->Get();
		const UValdorsoGildaComponent* SuaGilda = Di(Altro);
		const bool bNomina = (UValdorsoStaffComponent::PermessiDi(Altro) & ValdorsoPermessi::Gilda) || (SuaGilda && SuaGilda->Gilda.bCapoGilda);
		if (UValdorsoChatComponent* Chat = bNomina ? UValdorsoChatComponent::Di(Altro) : nullptr)
		{
			Chat->ClientSenti(FString(), Riga, false);
		}
	}
}

void UValdorsoGildaComponent::Nomina(EValdorsoGrado Nuovo, const FString& Da, bool bDalloStaff)
{
	const EValdorsoGrado Prima = Gilda.Grado;
	if (!ValdorsoGilda::Nomina(Gilda, Nuovo))
	{
		return;
	}
	Cambiato();
	Annota(FString::Printf(TEXT("grado da %s a %s, da %s (%s)"), *ValdorsoGilda::NomeGrado(Prima).ToString(), *ValdorsoGilda::NomeGrado(Nuovo).ToString(),
		*Da, bDalloStaff ? TEXT("staff, a nome di Mastro Ettore") : TEXT("capo gilda")));
	if (Nuovo > Prima)
	{
		Avvisa(bDalloStaff
			? FText::Format(LOCTEXT("NominaEttore", "Mastro Ettore, capo della filiale, ti nomina {0} della Gilda di Valdorso."), ValdorsoGilda::NomeGrado(Nuovo))
			: FText::Format(LOCTEXT("NominaCapo", "{0}, capo della Gilda, ti nomina {1} della Gilda di Valdorso."), FText::FromString(Da), ValdorsoGilda::NomeGrado(Nuovo)), 6.f);
	}
	else
	{
		Avvisa(FText::Format(LOCTEXT("Retrocesso", "La Gilda ti riporta al grado {0}."), ValdorsoGilda::NomeGrado(Nuovo)), 5.f);
	}
}

void UValdorsoGildaComponent::ImpostaCapo(bool bCapo, const FString& Da)
{
	if (Gilda.bCapoGilda == bCapo)
	{
		return;
	}
	Gilda.bCapoGilda = bCapo;
	Cambiato();
	Annota(FString::Printf(TEXT("%s capo gilda, da %s"), bCapo ? TEXT("nominato") : TEXT("non piu'"), *Da));
	Avvisa(bCapo ? LOCTEXT("SeiCapo", "Sei il capo della Gilda di Valdorso: con F9 dai i gradi a chi ha i punti (fino all'Argento).")
		: LOCTEXT("NonSeiCapo", "Non sei piu' il capo della Gilda."), 6.f);
}

void UValdorsoGildaComponent::DimmiGrado()
{
	const EValdorsoGrado Dopo = ValdorsoGilda::GradoDopo(Gilda.Grado);
	FText Testo;
	if (Gilda.Grado == EValdorsoGrado::Oro)
	{
		Testo = FText::Format(LOCTEXT("EttoreOro", "Mastro Ettore: \"Sei Oro, {0} punti. Sopra di te c'e' solo la Corona, e non sempre.\""), FText::AsNumber(Gilda.Punti));
	}
	else if (ValdorsoGilda::Pronto(Gilda))
	{
		Testo = FText::Format(LOCTEXT("EttorePronto", "Mastro Ettore: \"{0} punti. Per il {1} ci sei: ci penso, e te lo faro' sapere.\" (Il grado lo decide lo staff, o il capo gilda.)"),
			FText::AsNumber(Gilda.Punti), ValdorsoGilda::NomeGrado(Dopo));
	}
	else
	{
		Testo = FText::Format(LOCTEXT("EttoreNonAncora", "Mastro Ettore: \"Sei {0}, con {1} punti. Per il {2} ne servono {3}. Lavora.\""),
			ValdorsoGilda::NomeGrado(Gilda.Grado), FText::AsNumber(Gilda.Punti), ValdorsoGilda::NomeGrado(Dopo), FText::AsNumber(ValdorsoGilda::PuntiPer(Dopo)));
	}
	Avvisa(Testo, 7.f);
}

void UValdorsoGildaComponent::FaiTutto()
{
	for (FValdorsoIncaricoPreso& P : Gilda.Presi)
	{
		if (P.Stato == EValdorsoStatoIncarico::Assegnato && P.Foglio.Tipo != EValdorsoTipoIncarico::Consegna)
		{
			P.Progresso = P.Foglio.Quanti;
		}
	}
	Cambiato();
}

void UValdorsoGildaComponent::ServerStacca_Implementation(int32 Numero)
{
	UWorld* Mondo = GetWorld();
	UValdorsoBacheche* Bacheche = UValdorsoBacheche::Di(this);
	const double Ora = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	if (!Bacheche || Ora < UltimaRichiesta + GildaSecondiRichieste)
	{
		return;
	}
	UltimaRichiesta = Ora;
	if (!AllaBacheca())
	{
		Avvisa(LOCTEXT("Lontano", "Sei troppo lontano dalla bacheca."));
		return;
	}
	FValdorsoBachecheSalvate& D = Bacheche->Dati();
	const int64 Adesso = UValdorsoBacheche::Adesso();
	const FValdorsoIncarico* Incarico = ValdorsoGilda::Trova(D, Numero);
	if (!Incarico)
	{
		Avvisa(LOCTEXT("NonCePiu", "Quel foglio non c'e' piu'."));
		ApriBacheca();
		return;
	}
	const FText Perche = ValdorsoGilda::PercheNoStacca(*Incarico, Gilda, Adesso);
	if (!Perche.IsEmpty())
	{
		Avvisa(Perche, 3.5f);
		return;
	}
	const FString Titolo = Incarico->Titolo;
	if (!ValdorsoGilda::Stacca(D, Gilda, Numero, Adesso))
	{
		return;
	}
	// il foglio nello zaino: se non c'e' posto, si rimette sulla bacheca
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Cast<APlayerController>(GetOwner()));
	const bool bEntrato = Zaino && Zaino->Cambia([Numero](FValdorsoZaino& Z)
	{
		return ValdorsoZaino::PostoNumerato(Z, ValdorsoGilda::IdFoglio, Numero) != INDEX_NONE
			|| ValdorsoZaino::AggiungiNumerato(Z, ValdorsoGilda::IdFoglio, Numero) != INDEX_NONE;
	});
	if (!bEntrato)
	{
		ValdorsoGilda::Rinuncia(D, Gilda, Numero);
		Avvisa(LOCTEXT("ZainoPieno", "Lo zaino e' pieno: non c'e' posto per il foglio."), 3.f);
		return;
	}
	Bacheche->Segna();
	Cambiato();
	Annota(FString::Printf(TEXT("stacca il foglio n. %d: %s"), Numero, *Titolo));
	Avvisa(FText::Format(LOCTEXT("Staccato", "Stacchi il foglio «{0}». Portalo a Lucia, al bancone, entro mezz'ora."), FText::FromString(Titolo)), 4.f);
	ApriBacheca();
}

void UValdorsoGildaComponent::ServerBancone_Implementation(int32 Numero, uint8 Azione)
{
	UWorld* Mondo = GetWorld();
	UValdorsoBacheche* Bacheche = UValdorsoBacheche::Di(this);
	const double Ora = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	if (!Bacheche || Ora < UltimaRichiesta + GildaSecondiRichieste)
	{
		return;
	}
	UltimaRichiesta = Ora;
	if (!AlBancone())
	{
		Avvisa(LOCTEXT("LuciaLontana", "Lucia e' troppo lontana: parlale al bancone."));
		return;
	}
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Cast<APlayerController>(GetOwner()));
	const FValdorsoIncaricoPreso* P = ValdorsoGilda::Preso(Gilda, Numero);
	if (!Zaino || !P)
	{
		return;
	}
	FValdorsoBachecheSalvate& D = Bacheche->Dati();
	const int64 Adesso = UValdorsoBacheche::Adesso();
	const FValdorsoIncarico Foglio = P->Foglio;
	const FText Titolo = FText::FromString(Foglio.Titolo);
	switch (Azione)
	{
	case 0:
	{
		const FText Perche = ValdorsoGilda::PercheNoAssegna(Gilda, Numero, Zaino->Contenuto().Monete, Adesso);
		if (!Perche.IsEmpty())
		{
			Avvisa(Perche, 3.5f);
			return;
		}
		ValdorsoGilda::Assegna(Gilda, Numero, Zaino->Contenuto().Monete, Adesso);
		const int32 Cauzione = ValdorsoGilda::Preso(Gilda, Numero)->Cauzione;
		if (Cauzione > 0)
		{
			Zaino->Cambia([Cauzione](FValdorsoZaino& Z) { Z.Monete -= Cauzione; return true; });
		}
		Annota(FString::Printf(TEXT("Lucia assegna il n. %d: %s (cauzione %d)"), Numero, *Foglio.Titolo, Cauzione));
		Avvisa(Cauzione > 0
			? FText::Format(LOCTEXT("AssegnatoCauzione", "Lucia timbra il foglio «{0}» e tiene {1} Aureus di cauzione. L'incarico e' tuo."), Titolo, FText::AsNumber(Cauzione))
			: FText::Format(LOCTEXT("Assegnato", "Lucia timbra il foglio «{0}»: l'incarico e' tuo."), Titolo), 4.f);
		break;
	}
	case 1:
	{
		const int32 Nello = ValdorsoZaino::Conta(Zaino->Contenuto(), Foglio.Obiettivo);
		if (!ValdorsoGilda::Fatto(*P, Nello))
		{
			Avvisa(P->Stato == EValdorsoStatoIncarico::Prenotato ? LOCTEXT("PrimaAssegna", "Prima Lucia te lo deve assegnare.")
				: FText::Format(LOCTEXT("NonFatto", "Non e' ancora fatto: {0}."), ValdorsoGilda::TestoProgresso(*P, Nello)), 3.5f);
			return;
		}
		FValdorsoIncaricoPreso Compiuto;
		if (!ValdorsoGilda::Compi(D, Gilda, Numero, Compiuto))
		{
			return;
		}
		const int32 Paga = Compiuto.Foglio.Monete + Compiuto.Cauzione;
		Zaino->Cambia([&Compiuto, Paga](FValdorsoZaino& Z)
		{
			if (Compiuto.Foglio.Tipo == EValdorsoTipoIncarico::Consegna)
			{
				ValdorsoZaino::TogliOggetto(Z, Compiuto.Foglio.Obiettivo, Compiuto.Foglio.Quanti);
			}
			Z.Monete = FMath::Min(ValdorsoZaino::MassimoMonete, Z.Monete + Paga);
			return true;
		});
		Annota(FString::Printf(TEXT("compiuto il n. %d: %s (%d Aureus, %d punti)"), Numero, *Foglio.Titolo, Paga, Compiuto.Foglio.Punti));
		// (10/11) i punti dicono quando si e' pronti; il grado lo da' una persona (Nomina)
		const bool bPronto = ValdorsoGilda::DaAvvisarePronto(Gilda);
		Avvisa(bPronto
			? FText::Format(LOCTEXT("CompiutoPronto", "«Compiuto.» Lucia ti paga {0} Aureus. Hai i punti per il grado {1}: ne parlera' Mastro Ettore, il capo della filiale."),
				FText::AsNumber(Paga), ValdorsoGilda::NomeGrado(ValdorsoGilda::GradoDopo(Gilda.Grado)))
			: FText::Format(LOCTEXT("Compiuto", "«Compiuto.» Lucia timbra il foglio e ti paga {0} Aureus (+{1} punti della Gilda)."), FText::AsNumber(Paga), FText::AsNumber(Compiuto.Foglio.Punti)), 5.f);
		if (bPronto)
		{
			Annota(FString::Printf(TEXT("pronto per il grado %s (%d punti)"), *ValdorsoGilda::NomeGrado(ValdorsoGilda::GradoDopo(Gilda.Grado)).ToString(), Gilda.Punti));
			AvvisaChiNomina();
		}
		break;
	}
	case 2:
	{
		const bool bAssegnato = P->Stato == EValdorsoStatoIncarico::Assegnato;
		const int32 Cauzione = P->Cauzione;
		ValdorsoGilda::Rinuncia(D, Gilda, Numero);
		if (Cauzione > 0)
		{
			D.Cassa = FMath::Min(ValdorsoGilda::CassaMassima, D.Cassa + Cauzione);
		}
		Annota(FString::Printf(TEXT("rinuncia al n. %d: %s"), Numero, *Foglio.Titolo));
		Avvisa(bAssegnato
			? FText::Format(LOCTEXT("Rinuncia", "Lucia straccia il foglio «{0}». Rinunciare costa: -{1} punti della Gilda."), Titolo, FText::AsNumber(ValdorsoGilda::PuntiRinuncia))
			: FText::Format(LOCTEXT("RinunciaPrenotato", "Lucia rimette il foglio «{0}» sulla bacheca."), Titolo), 4.f);
		break;
	}
	default:
		return;
	}
	Bacheche->Segna();
	Cambiato();
}

void UValdorsoGildaComponent::ClientBacheca_Implementation(const TArray<FValdorsoIncarico>& NuoviFogli, int64 Ora)
{
	Fogli = NuoviFogli;
	Scarto = Ora - FDateTime::UtcNow().ToUnixTimestamp();
	++NumeroCambiamenti;
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (Schermata.IsValid())
	{
		ValdorsoGildaSchermo::Rifai(Controllore);
		return;
	}
	ValdorsoGildaSchermo::Apri(Controllore, false);
}

void UValdorsoGildaComponent::ClientBancone_Implementation(int64 Ora)
{
	Scarto = Ora - FDateTime::UtcNow().ToUnixTimestamp();
	++NumeroCambiamenti;
	// se e' aperta la bacheca, si chiude: adesso c'e' il bancone
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (Schermata.IsValid() && !Schermata->EBancone())
	{
		ValdorsoGildaSchermo::Chiudi(Controllore);
	}
	// il dialogo con Lucia si chiude in questo stesso momento: se e' ancora aperto si riprova fra poco
	if (!ValdorsoGildaSchermo::Apri(Controllore, true))
	{
		const UWorld* Mondo = GetWorld();
		BanconeInAttesaFino = (Mondo ? Mondo->GetTimeSeconds() : 0.0) + 2.0;
	}
}

void UValdorsoGildaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UWorld* Mondo = GetWorld();
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (!Mondo || !Controllore)
	{
		return;
	}
	const double Ora = Mondo->GetTimeSeconds();
	// PC: il bancone aspettava che il dialogo con Lucia si chiudesse
	if (BanconeInAttesaFino > 0.0 && Controllore->IsLocalController())
	{
		if (Ora > BanconeInAttesaFino || ValdorsoGildaSchermo::Apri(Controllore, true))
		{
			BanconeInAttesaFino = 0.0;
		}
	}
	if (!Controllore->HasAuthority())
	{
		return;
	}
	if (bDaSalvare && Ora >= SalvaDopo)
	{
		bDaSalvare = false;
		if (AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore))
		{
			Nostro->SalvaPersonaggio();
		}
	}
	if (Ora < ProssimoControllo)
	{
		return;
	}
	ProssimoControllo = Ora + GildaSecondiControllo;
	if (Gilda.Presi.IsEmpty())
	{
		return;
	}
	bool bCambiato = false;
	// le scadenze: la prenotazione (mezz'ora) e l'incarico
	if (UValdorsoBacheche* Bacheche = UValdorsoBacheche::Di(this))
	{
		const TArray<FValdorsoIncaricoPreso> Scaduti = ValdorsoGilda::Scadute(Bacheche->Dati(), Gilda, UValdorsoBacheche::Adesso());
		for (const FValdorsoIncaricoPreso& P : Scaduti)
		{
			bCambiato = true;
			Bacheche->Segna();
			if (P.Cauzione > 0)
			{
				// scaduto senza colpa: la cauzione torna
				if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore))
				{
					const int32 Cauzione = P.Cauzione;
					Zaino->Cambia([Cauzione](FValdorsoZaino& Z) { Z.Monete = FMath::Min(ValdorsoZaino::MassimoMonete, Z.Monete + Cauzione); return true; });
				}
			}
			Annota(FString::Printf(TEXT("scade il n. %d: %s"), P.Foglio.Numero, *P.Foglio.Titolo));
			Avvisa(P.Stato == EValdorsoStatoIncarico::Prenotato && P.Foglio.Scadenza > UValdorsoBacheche::Adesso()
				? FText::Format(LOCTEXT("PrenotazioneFinita", "Non hai portato a Lucia il foglio «{0}»: torna sulla bacheca."), FText::FromString(P.Foglio.Titolo))
				: FText::Format(LOCTEXT("IncaricoScaduto", "L'incarico «{0}» e' scaduto."), FText::FromString(P.Foglio.Titolo)), 4.f);
		}
	}
	// i luoghi da raggiungere: il server guarda dov'e' davvero il corpo
	if (const APawn* Corpo = Controllore->GetPawn())
	{
		for (FValdorsoIncaricoPreso& P : Gilda.Presi)
		{
			if (P.Stato != EValdorsoStatoIncarico::Assegnato || P.Foglio.Tipo != EValdorsoTipoIncarico::Raggiungi || P.Progresso >= P.Foglio.Quanti)
			{
				continue;
			}
			const FValdorsoLuogo* Luogo = ValdorsoLuoghi::Trova(P.Foglio.Obiettivo.ToString());
			if (Luogo && ValdorsoLuoghi::Dentro(*Luogo, Corpo->GetActorLocation()))
			{
				P.Progresso = P.Foglio.Quanti;
				bCambiato = true;
				Annota(FString::Printf(TEXT("raggiunge %s (n. %d)"), *Luogo->Nome, P.Foglio.Numero));
				Avvisa(FText::Format(LOCTEXT("Raggiunto", "Gilda, «{0}»: sei arrivato. Torna da Lucia a raccontarlo."), FText::FromString(P.Foglio.Titolo)), 4.f);
			}
		}
	}
	if (bCambiato)
	{
		Cambiato();
	}
}

void UValdorsoGildaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Schermata.IsValid())
	{
		ValdorsoGildaSchermo::Chiudi(Cast<APlayerController>(GetOwner()));
	}
	Super::EndPlay(EndPlayReason);
}

#undef LOCTEXT_NAMESPACE
