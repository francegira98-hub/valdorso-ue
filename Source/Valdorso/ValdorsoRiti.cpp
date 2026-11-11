// Valdorso - I riti della valle (vedi ValdorsoRiti.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoRiti.h"
#include "Valdorso.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuino.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ValdorsoRiti"

namespace
{
	/** Dopo quanti secondi dall'ingresso arriva l'accoglienza (il tempo di guardarsi intorno). */
	constexpr float RitiSecondiAccoglienza = 6.f;
	/** Quanti segni si leggono sulla pietra, e quanti ne tiene al massimo il file. */
	constexpr int32 RitiSegniLetti = 14;
	constexpr int32 RitiSegniMassimi = 5000;

	FString RitiNomeDi(const APlayerController* Chi)
	{
		return Chi && Chi->PlayerState ? Chi->PlayerState->GetPlayerName() : FString();
	}

	void RitiAvviso(APlayerController* Chi, const FText& Testo, float Secondi)
	{
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
		{
			Giocatore->ClientAvviso(Testo, Secondi, 0.f);
		}
	}

	/** L'accoglienza della prima ora (anche per la console). */
	void RitiAccogli(APlayerController* Chi, int64 FirmatoIl)
	{
		if (!Chi || !Chi->GetPawn())
		{
			return;
		}
		const FValdorsoStella& Stella = ValdorsoRiti::StellaDi(FirmatoIl);
		RitiAvviso(Chi, FText::Format(LOCTEXT("PrimaOra", "La tua prima ora nella valle. Hai firmato il Registro sotto la stella {0}: {1}"),
			FText::FromString(Stella.Nome), FText::FromString(Stella.Motto)), 12.f);
		ValdorsoRegistroEventi::Annota(TEXT("PrimaOra"), Chi->GetPawn(), nullptr, Chi->GetPawn()->GetActorLocation(),
			FString::Printf(TEXT("sotto la stella %s"), Stella.Nome));
	}
}

namespace ValdorsoRiti
{
	const TArray<FValdorsoStella>& Stelle()
	{
		static const TArray<FValdorsoStella> Tutte = {
			{ TEXT("della Lanterna"), TEXT("chi nasce sotto la Lanterna trova la strada anche al buio.") },
			{ TEXT("della Brina"), TEXT("la Brina tiene i segreti finche' il sole non li scioglie.") },
			{ TEXT("del Martello"), TEXT("sotto il Martello si nasce testardi, e le cose si finiscono.") },
			{ TEXT("del Seme"), TEXT("chi ha il Seme sa aspettare: quello che pianta, cresce.") },
			{ TEXT("del Cervo"), TEXT("il Cervo sente il pericolo prima degli altri.") },
			{ TEXT("della Spiga"), TEXT("alla tavola della Spiga non manca mai il pane, ne' un amico.") },
			{ TEXT("del Falco"), TEXT("il Falco guarda lontano, e a volte si scorda dei piedi.") },
			{ TEXT("della Fiamma"), TEXT("la Fiamma scalda chi le sta vicino, e brucia chi la stuzzica.") },
			{ TEXT("della Corda"), TEXT("chi nasce sotto la Corda tiene insieme le cose, e le persone.") },
			{ TEXT("della Campana"), TEXT("la Campana si sente da lontano: chi nasce sotto di lei non sa tacere.") },
			{ TEXT("della Lupa"), TEXT("la Lupa difende i suoi fino all'ultimo.") },
			{ TEXT("dell'Orsa"), TEXT("sotto l'Orsa si nasce custodi, anche senza saperlo.") },
		};
		return Tutte;
	}

	const FValdorsoStella& StellaDi(int64 FirmatoIl)
	{
		const FDateTime Quando = FirmatoIl > 0 ? FDateTime::FromUnixTimestamp(FirmatoIl) : FDateTime::UtcNow();
		// dodici parti dell'anno, di 30 o 31 giorni (l'ultima prende anche il giorno in piu' dei bisestili)
		const int32 Giorno = FMath::Clamp(Quando.GetDayOfYear() - 1, 0, 365);
		const int32 Indice = FMath::Clamp(Giorno * 12 / 366, 0, 11);
		return Stelle()[Indice];
	}

	bool AnnoEUnGiorno(int64 FirmatoIl, int64 Adesso)
	{
		return FirmatoIl > 0 && Adesso - FirmatoIl >= static_cast<int64>(GiorniCerimonia) * 86400;
	}

	void Entra(APlayerController* Chi, int64 FirmatoIl)
	{
		UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
		UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
		if (!Chi || !Chi->HasAuthority() || !Taccuino || !Taccuino->HaPersonaggio() || !Mondo)
		{
			return;
		}
		const FName Riti(IdRiti);
		const FName Sacerdote(IdSacerdote);
		// (i coloni di prima di questi riti, firmati da piu' di due giorni, ricevono il segno senza l'accoglienza)
		const bool bSenzaSegno = !Taccuino->HaSegno(Riti, FName(SegnoPrimaOra));
		const bool bPrima = bSenzaSegno && (FirmatoIl <= 0 || FDateTime::UtcNow().ToUnixTimestamp() - FirmatoIl < 2 * 86400);
		const bool bAnno = AnnoEUnGiorno(FirmatoIl, FDateTime::UtcNow().ToUnixTimestamp())
			&& !Taccuino->HaSegno(Sacerdote, FName(SegnoCittadino));
		if (bSenzaSegno)
		{
			Taccuino->MettiSegno(Riti, FName(SegnoPrimaOra));
		}
		if (bAnno)
		{
			Taccuino->MettiSegno(Sacerdote, FName(SegnoAnno));
		}
		if (!bPrima && !bAnno)
		{
			return;
		}
		TWeakObjectPtr<APlayerController> Debole(Chi);
		FTimerHandle Maniglia;
		Mondo->GetTimerManager().SetTimer(Maniglia, FTimerDelegate::CreateLambda([Debole, FirmatoIl, bPrima, bAnno]()
		{
			APlayerController* Lui = Debole.Get();
			if (!Lui)
			{
				return;
			}
			if (bPrima)
			{
				RitiAccogli(Lui, FirmatoIl);
			}
			else if (bAnno)
			{
				RitiAvviso(Lui, LOCTEXT("Anno", "Un anno e un giorno da quando hai firmato il Registro. Padre Anselmo ti aspetta al Tempio del Cuore."), 10.f);
			}
		}), RitiSecondiAccoglienza, false);
	}

	bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Sacerdote, FName& InOutProssimo)
	{
		if (Evento != FName(EventoCerimonia))
		{
			return false;
		}
		UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
		const bool bPuo = Taccuino && Taccuino->HaPersonaggio() && Taccuino->HaSegno(FName(IdSacerdote), FName(SegnoAnno))
			&& !Taccuino->HaSegno(FName(IdSacerdote), FName(SegnoCittadino));
		if (!bPuo)
		{
			InOutProssimo = TEXT("presto");
			return true;
		}
		const FString Nome = RitiNomeDi(Chi);
		UWorld* Mondo = Chi->GetWorld();
		if (Mondo)
		{
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				if (APlayerController* Altro = It->Get(); Altro && Altro != Chi)
				{
					RitiAvviso(Altro, FText::Format(LOCTEXT("Cittadino", "Al Tempio del Cuore si legge un nome: {0}. Un anno e un giorno nella valle: da oggi e' di Valdorso."),
						FText::FromString(Nome)), 8.f);
				}
			}
		}
		if (APawn* Corpo = Chi->GetPawn())
		{
			ValdorsoRegistroEventi::Annota(TEXT("Cerimonia"), Corpo, Sacerdote, Corpo->GetActorLocation(), TEXT("un anno e un giorno: cittadino di Valdorso"));
		}
		return true;
	}

	bool Incidi(TArray<FValdorsoSegnoInciso>& Segni, const FString& Nome, int64 Quando, const FString& Stella)
	{
		const FString Pulito = Nome.TrimStartAndEnd().Left(60);
		if (Pulito.IsEmpty() || Segni.ContainsByPredicate([&Pulito](const FValdorsoSegnoInciso& S) { return S.Nome.Equals(Pulito, ESearchCase::IgnoreCase); }))
		{
			return false;
		}
		FValdorsoSegnoInciso& Nuovo = Segni.AddDefaulted_GetRef();
		Nuovo.Nome = Pulito;
		Nuovo.Quando = Quando;
		Nuovo.Stella = Stella;
		return true;
	}

	TArray<FString> Righe(const TArray<FValdorsoSegnoInciso>& Segni, int32 Quante)
	{
		TArray<FString> Fuori;
		if (Segni.IsEmpty())
		{
			Fuori.Add(TEXT("La pietra e' liscia: nessuno ci ha ancora inciso il suo segno."));
			return Fuori;
		}
		Fuori.Add(FString::Printf(TEXT("Sulla pietra ci sono %d segni. Gli ultimi:"), Segni.Num()));
		for (int32 i = Segni.Num() - 1, Letti = 0; i >= 0 && Letti < Quante; --i, ++Letti)
		{
			const FValdorsoSegnoInciso& S = Segni[i];
			const FDateTime Quando = FDateTime::FromUnixTimestamp(S.Quando);
			Fuori.Add(FString::Printf(TEXT("%s, il %s, sotto la stella %s"), *S.Nome, *Quando.ToString(TEXT("%d/%m/%Y")), *S.Stella));
		}
		return Fuori;
	}
}

// ------------------------------------------------------------------------------------------------ la pietra dei segni

AValdorsoPietraSegni::AValdorsoPietraSegni()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Pietra = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pietra"));
	SetRootComponent(Pietra);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cilindro.Succeeded())
	{
		Pietra->SetStaticMesh(Cilindro.Object);
		// un menhir: 70 cm di diametro, 1,8 m (il cilindro base e' di 1 m, con l'origine nel mezzo)
		Pietra->SetRelativeScale3D(FVector(0.7f, 0.55f, 1.8f));
	}
	Pietra->SetCollisionProfileName(TEXT("BlockAll"));

	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	Presa->SetupAttachment(Pietra);
	Presa->SetAbsolute(false, false, true);
	Presa->InitSphereRadius(90.f);
	Presa->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Presa->SetCollisionObjectType(ECC_WorldDynamic);
	Presa->SetCollisionResponseToAllChannels(ECR_Overlap);
	Presa->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Presa->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void AValdorsoPietraSegni::BeginPlay()
{
	Super::BeginPlay();
	// (il file si legge alla prima E: prima l'archivio deve sapere se e' il gioco da solo o il server)
}

FText AValdorsoPietraSegni::TestoInterazione(const APawn* Chi) const
{
	return LOCTEXT("Pietra", "La pietra dei segni: incidi il tuo, o leggi");
}

FString AValdorsoPietraSegni::File() const
{
	const UWorld* Mondo = GetWorld();
	const UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	const TCHAR* Cartella = Mondo && Mondo->IsPlayInEditor() ? TEXT("Prova")
		: (Archivista && Archivista->ArchivioSolo() ? TEXT("Solo") : TEXT("Server"));
	return FPaths::ProjectSavedDir() / Cartella / TEXT("Segni.json");
}

void AValdorsoPietraSegni::Leggi()
{
	bLetti = true;
	bIlleggibile = false;
	Segni.Reset();
	FString Testo;
	if (!FFileHelper::LoadFileToString(Testo, *File()))
	{
		return;
	}
	TSharedPtr<FJsonObject> Oggetto;
	const TSharedRef<TJsonReader<>> Lettore = TJsonReaderFactory<>::Create(Testo);
	if (!FJsonSerializer::Deserialize(Lettore, Oggetto) || !Oggetto.IsValid())
	{
		// non si riscrive sopra: il file resta com'e' (e se ne tiene una copia), la pietra si legge ma non si incide
		bIlleggibile = true;
		IFileManager::Get().Copy(*(File() + TEXT(".rotto")), *File());
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Segni.json illeggibile: copiato in Segni.json.rotto, la pietra non si incide finche' non si sistema"));
		return;
	}
	const TArray<TSharedPtr<FJsonValue>>* Elenco = nullptr;
	if (Oggetto->TryGetArrayField(TEXT("Segni"), Elenco) && Elenco)
	{
		for (const TSharedPtr<FJsonValue>& Valore : *Elenco)
		{
			const TSharedPtr<FJsonObject>* Uno = nullptr;
			if (Valore.IsValid() && Valore->TryGetObject(Uno) && Uno)
			{
				FValdorsoSegnoInciso& S = Segni.AddDefaulted_GetRef();
				S.Nome = (*Uno)->GetStringField(TEXT("Nome")).Left(60);
				S.Quando = static_cast<int64>((*Uno)->GetNumberField(TEXT("Quando")));
				S.Stella = (*Uno)->GetStringField(TEXT("Stella")).Left(40);
			}
		}
	}
}

void AValdorsoPietraSegni::Scrivi() const
{
	if (bIlleggibile)
	{
		return;
	}
	const TSharedRef<FJsonObject> Oggetto = MakeShared<FJsonObject>();
	Oggetto->SetNumberField(TEXT("Versione"), 1);
	TArray<TSharedPtr<FJsonValue>> Elenco;
	for (const FValdorsoSegnoInciso& S : Segni)
	{
		const TSharedRef<FJsonObject> Uno = MakeShared<FJsonObject>();
		Uno->SetStringField(TEXT("Nome"), S.Nome);
		Uno->SetNumberField(TEXT("Quando"), static_cast<double>(S.Quando));
		Uno->SetStringField(TEXT("Stella"), S.Stella);
		Elenco.Add(MakeShared<FJsonValueObject>(Uno));
	}
	Oggetto->SetArrayField(TEXT("Segni"), Elenco);
	FString Testo;
	const TSharedRef<TJsonWriter<>> Scrittore = TJsonWriterFactory<>::Create(&Testo);
	if (FJsonSerializer::Serialize(Oggetto, Scrittore))
	{
		FFileHelper::SaveStringToFile(Testo, *File());
	}
}

void AValdorsoPietraSegni::Interagisci(APawn* Chi)
{
	AValdorsoPlayerController* Giocatore = Chi ? Cast<AValdorsoPlayerController>(Chi->GetController()) : nullptr;
	if (!HasAuthority() || !Giocatore || FVector::Dist(Chi->GetActorLocation(), GetActorLocation()) > 400.f)
	{
		return;
	}
	if (!bLetti)
	{
		Leggi();
	}
	UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Giocatore);
	const FString Nome = RitiNomeDi(Giocatore);
	// solo un personaggio vero incide (chi entra con l'accesso libero dell'editor la legge e basta)
	const bool bPuoIncidere = Taccuino && Taccuino->HaPersonaggio() && !Nome.IsEmpty() && Segni.Num() < RitiSegniMassimi
		&& !bIlleggibile;
	UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	const FValdorsoPersonaggio* Dati = Archivista ? Archivista->TrovaPersonaggio(Giocatore->GetAccountId(), Giocatore->GetPersonaggioId()) : nullptr;
	const int64 Firmato = Dati ? Dati->Registro.FirmatoIl : 0;
	if (bPuoIncidere && ValdorsoRiti::Incidi(Segni, Nome, FDateTime::UtcNow().ToUnixTimestamp(), ValdorsoRiti::StellaDi(Firmato).Nome))
	{
		Scrivi();
		Giocatore->ClientAvviso(FText::Format(LOCTEXT("Inciso", "Hai inciso il tuo segno sulla pietra, accanto ad altri {0}. Restera' qui."),
			FText::AsNumber(Segni.Num() - 1)), 6.f, 0.f);
		ValdorsoRegistroEventi::Annota(TEXT("Segno"), Chi, this, GetActorLocation(), TEXT("inciso sulla pietra dei segni"));
		return;
	}
	Giocatore->ClientMostraBacheca(TEXT("La pietra dei segni"), ValdorsoRiti::Righe(Segni, RitiSegniLetti));
}

// ------------------------------------------------------------------------------------------------ console (prove)

namespace
{
	APlayerController* RitiPrimo(UWorld* Mondo)
	{
		return Mondo ? Mondo->GetFirstPlayerController() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoRitiPrimaOra(
		TEXT("Valdorso.Riti.PrimaOra"),
		TEXT("Valdorso: l'accoglienza della prima ora nella valle, di nuovo (sul server, al primo colono)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			if (APlayerController* Chi = RitiPrimo(Mondo); Chi && Chi->HasAuthority())
			{
				RitiAccogli(Chi, 0);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ComandoRitiAnno(
		TEXT("Valdorso.Riti.Anno"),
		TEXT("Valdorso: come se fosse passato un anno e un giorno (sul server, al primo colono): Padre Anselmo lo aspetta."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			APlayerController* Chi = RitiPrimo(Mondo);
			UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
			if (Chi && Chi->HasAuthority() && Taccuino && Taccuino->HaPersonaggio())
			{
				Taccuino->MettiSegno(FName(ValdorsoRiti::IdSacerdote), FName(ValdorsoRiti::SegnoAnno));
				RitiAvviso(Chi, LOCTEXT("AnnoProva", "(prova) Un anno e un giorno: Padre Anselmo ti aspetta al Tempio del Cuore."), 8.f);
			}
		}));
}

#undef LOCTEXT_NAMESPACE
