// Valdorso - La grotta del nord (vedi ValdorsoGrottaNord.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoGrottaNord.h"
#include "Valdorso.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoCrescita.h"
#include "ValdorsoDialoghi.h"
#include "ValdorsoAbilitaDati.h"
#include "ValdorsoEffetti.h"
#include "ValdorsoEtichette.h"
#include "ValdorsoGilda.h"
#include "ValdorsoInterfaccia.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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

#define LOCTEXT_NAMESPACE "ValdorsoGrottaNord"

namespace
{
	/** Quanti sassi stanno davanti al portale (spariscono man mano che si scava). */
	constexpr int32 GrottaSassi = 10;
	/** Il forziere si riapre per lo stesso colono dopo questi secondi (venti ore). */
	constexpr int64 GrottaSecondiForziere = 20 * 3600;
	/** Fin dove si sente la frana che trema e che cede (cm). */
	constexpr float GrottaRaggioTrema = 30000.f;

	const TCHAR* GrottaModelloSasso = TEXT("/Game/Valle/Grotta/Pezzi/SM_GrottaNord_Sasso.SM_GrottaNord_Sasso");
	const TCHAR* GrottaModelloForziere = TEXT("/Game/Valle/Grotta/Pezzi/SM_GrottaNord_Forziere.SM_GrottaNord_Forziere");

	UStaticMesh* GrottaModello(const TCHAR* Percorso)
	{
		if (UStaticMesh* Trovato = LoadObject<UStaticMesh>(nullptr, Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			return Trovato;
		}
		// l'importazione dei .glb puo' mettere il modello in una sottocartella: <cartella>/<Nome>/StaticMeshes/<Nome>
		// (crea_grotta_nord.py lo rimette a posto, ma se non e' passato si cerca anche li')
		FString Cartella;
		FString Nome;
		FString(Percorso).Split(TEXT("."), &Cartella, &Nome, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		Cartella = FPaths::GetPath(Cartella);
		const FString Dentro = FString::Printf(TEXT("%s/%s/StaticMeshes/%s.%s"), *Cartella, *Nome, *Nome, *Nome);
		return LoadObject<UStaticMesh>(nullptr, *Dentro, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	/** (11/11) Una presa invisibile per la E (come la zona dei letti): la trova ValdorsoInterazione::PiuVicina. */
	void GrottaPresa(USphereComponent* Presa, float Raggio)
	{
		Presa->InitSphereRadius(Raggio);
		Presa->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Presa->SetCollisionObjectType(ECC_WorldDynamic);
		Presa->SetCollisionResponseToAllChannels(ECR_Overlap);
		Presa->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Presa->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Presa->SetGenerateOverlapEvents(false);
		Presa->SetHiddenInGame(true);
	}

	AValdorsoPlayerController* GrottaGiocatore(const APawn* Chi)
	{
		return Chi ? Cast<AValdorsoPlayerController>(Chi->GetController()) : nullptr;
	}

	FString GrottaNomeDi(const APawn* Chi)
	{
		const APlayerState* Stato = Chi ? Chi->GetPlayerState() : nullptr;
		return Stato ? Stato->GetPlayerName() : FString();
	}

	/** Le frane del mondo (di solito una), per i comandi. */
	template <typename FFai>
	void GrottaPerOgniFrana(UWorld* Mondo, FFai Fai)
	{
		if (!Mondo || Mondo->GetNetMode() == NM_Client)
		{
			return;
		}
		int32 Quante = 0;
		for (TActorIterator<AValdorsoFrana> It(Mondo); It; ++It)
		{
			Fai(**It);
			++Quante;
		}
		if (Quante == 0)
		{
			UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Grotta: nessuna frana nel livello (lanciare crea_grotta_nord.py)"));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoGrottaTrema(TEXT("Valdorso.Grotta.Trema"),
		TEXT("La notte in cui la terra trema: la frana del nord si smuove (da qui si scava)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Mondo)
		{
			GrottaPerOgniFrana(Mondo, [](AValdorsoFrana& F) { F.Trema(); });
		}), ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ComandoGrottaApri(TEXT("Valdorso.Grotta.Apri"),
		TEXT("La frana del nord cede: le gallerie sono aperte."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Mondo)
		{
			GrottaPerOgniFrana(Mondo, [](AValdorsoFrana& F) { F.Apri(); });
		}), ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ComandoGrottaChiudi(TEXT("Valdorso.Grotta.Chiudi"),
		TEXT("Per le prove: la frana del nord torna chiusa."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Mondo)
		{
			GrottaPerOgniFrana(Mondo, [](AValdorsoFrana& F) { F.Chiudi(); });
		}), ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ComandoGrottaDentro(TEXT("Valdorso.Grotta.Dentro"),
		TEXT("Per le prove: porta i coloni dentro le gallerie del nord (anche con la frana chiusa)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Mondo)
		{
			const AValdorsoFrana* Frana = AValdorsoFrana::Di(Mondo);
			if (!Frana || !Mondo || Mondo->GetNetMode() == NM_Client)
			{
				return;
			}
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				if (APawn* Corpo = It->Get() ? It->Get()->GetPawn() : nullptr)
				{
					ValdorsoGrottaNord::Porta(Corpo, Frana->Dentro, Frana->GradiDentro, LOCTEXT("ProvaDentro", "Le gallerie dei nani (prova)."));
				}
			}
		}), ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ComandoGrottaFuori(TEXT("Valdorso.Grotta.Fuori"),
		TEXT("Per le prove: riporta i coloni davanti alla frana del nord."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* Mondo)
		{
			const AValdorsoFrana* Frana = AValdorsoFrana::Di(Mondo);
			if (!Frana || !Mondo || Mondo->GetNetMode() == NM_Client)
			{
				return;
			}
			const FVector Fuori = Frana->GetActorLocation() + Frana->GetActorForwardVector() * 300.f + FVector(0.f, 0.f, 100.f);
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				if (APawn* Corpo = It->Get() ? It->Get()->GetPawn() : nullptr)
				{
					ValdorsoGrottaNord::Porta(Corpo, Fuori, Frana->GetActorRotation().Yaw, FText::GetEmpty());
				}
			}
		}), ECVF_Cheat);
}

namespace ValdorsoGrottaNord
{
	float PassoScavo(float PuntiTempra)
	{
		return 0.005f * (1.f + 0.5f * FMath::Clamp(PuntiTempra, 0.f, 100.f) / 100.f);
	}

	bool HaAttrezzo(const FValdorsoZaino& Zaino)
	{
		return ValdorsoZaino::Conta(Zaino, TEXT("Piccone")) > 0 || ValdorsoZaino::Conta(Zaino, TEXT("Martello")) > 0;
	}

	TArrayView<const FName> Tracce()
	{
		static const TArray<FName> Tutte = { FName(TEXT("Taccuino_Gisella")), FName(TEXT("Lanterna_Cecco")), FName(TEXT("Anello_Ranieri")) };
		return Tutte;
	}

	FString TitoloStoria()
	{
		return TEXT("Quelli che non tornarono");
	}

	TArray<FString> RigheStoria()
	{
		return {
			TEXT("Me l'ha raccontato Lucia, alla Gilda, con la penna ferma a meta' di una parola."),
			TEXT("Nell'autunno del 309, quando di notte da sotto la montagna del nord cominciarono i colpi, la filiale mando' quattro dei suoi."),
			TEXT("Ranieri, il capo della compagnia. Gisella, che disegnava le mappe. Cecco, minatore della valle, che sapeva leggere i segni dei nani. E Lucia, che resto' fuori con la corda."),
			TEXT("Allargarono la fessura sopra la frana ed entrarono. Il primo giorno la corda tiro' tre volte: tutto bene. Il secondo Gisella ci lego' due pagine della mappa."),
			TEXT("La terza notte la frana scese ancora e chiuse la fessura. Da allora ci passa solo l'aria."),
			TEXT("Le due pagine me le ha date lei. Se trovo le loro cose, gliele riporto: non per la Gilda, per lei."),
		};
	}

	TArray<FString> RigheFine()
	{
		return {
			TEXT("Ho messo sul bancone il taccuino di Gisella, la lanterna di Cecco e l'anello di Ranieri."),
			TEXT("Lucia ha toccato l'anello e non ha detto niente per un po'. Poi ha chiuso il registro, cosa che non fa mai."),
			TEXT("Tra le ossa Ranieri non c'era. Il suo anello si'. Lucia dice che, se c'e' un'altra uscita, e' oltre la porta che i nani hanno chiuso."),
		};
	}

	TArray<FString> RigheTraccia(FName Oggetto)
	{
		if (Oggetto == FName(TEXT("Taccuino_Gisella")))
		{
			return {
				TEXT("Il taccuino di Gisella, nel campo della spedizione. Le ultime pagine:"),
				TEXT("\"Giorno due. Cecco dice che i segni sulle colonne non sono nomi: sono un conto. I nani contavano qualcosa, qui sotto.\""),
				TEXT("\"Giorno tre. La frana. Ranieri dice che c'e' un'altra uscita, a nord, oltre la scala. Non ci crede nemmeno lui.\""),
				TEXT("\"Giorno quattro. Abbiamo sentito i martelli. Non sono martelli.\""),
			};
		}
		if (Oggetto == FName(TEXT("Lanterna_Cecco")))
		{
			return {
				TEXT("La lanterna di Cecco, ai piedi della scala dei nani, nella sala delle ossa. Il vetro e' rotto, il suo nome inciso sul manico."),
				TEXT("Spenta da tre anni. Accanto, sulla pietra, un segno fresco di carbone: una freccia verso nord."),
			};
		}
		if (Oggetto == FName(TEXT("Anello_Ranieri")))
		{
			return {
				TEXT("L'anello d'argento della Gilda, con la lanterna incisa e una R. Lo portava al collo, legato a un laccio, il capitano delle ossa."),
				TEXT("Le ossa del capitano sono antiche, piu' antiche della valle. L'anello no."),
			};
		}
		return {};
	}

	void Porta(APawn* Chi, const FVector& Dove, float Gradi, const FText& Avviso)
	{
		if (!Chi || !Chi->HasAuthority() || Dove.IsZero())
		{
			return;
		}
		Chi->TeleportTo(Dove, FRotator(0.f, Gradi, 0.f), false, true);
		if (AValdorsoPlayerController* Giocatore = GrottaGiocatore(Chi))
		{
			Giocatore->SetControlRotation(FRotator(0.f, Gradi, 0.f));
			Giocatore->ClientSetRotation(FRotator(0.f, Gradi, 0.f), false);
			// lo schermo si riapre dal nero
			Giocatore->ClientSetCameraFade(true, FColor::Black, FVector2D(1.f, 0.f), 1.2f, false, false);
			if (!Avviso.IsEmpty())
			{
				Giocatore->ClientAvviso(Avviso, 3.5f, 0.f);
			}
		}
		ValdorsoRegistroEventi::Annota(TEXT("Passaggio"), Chi, nullptr, Dove, TEXT("gallerie del nord"));
	}

	bool DaiTraccia(APawn* Chi, FName Oggetto)
	{
		AValdorsoPlayerController* Giocatore = GrottaGiocatore(Chi);
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Giocatore);
		UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Giocatore);
		// la storia finita: il segno sta dove lo mette il dialogo (il file del personaggio, o la memoria per nome del server
		// per chi entra senza personaggio: come FPngMemoriaColono in ValdorsoPNG.cpp)
		bool bFinita = false;
		if (Taccuino && Taccuino->HaPersonaggio())
		{
			bFinita = Taccuino->HaSegno(FName(DialogoLucia), FName(SegnoFinite));
		}
		else if (Chi)
		{
			const APlayerState* StatoColono = Chi->GetPlayerState();
			bFinita = ValdorsoDialoghi::HaSegno(FName(DialogoLucia), StatoColono ? StatoColono->GetPlayerName() : Chi->GetName(), FName(SegnoFinite));
		}
		if (!Zaino || !ValdorsoOggetti::Trova(Oggetto) || ValdorsoZaino::Conta(Zaino->Contenuto(), Oggetto) > 0 || bFinita)
		{
			return false;
		}
		if (Zaino->Dai(Oggetto, 1) <= 0)
		{
			Giocatore->ClientAvviso(LOCTEXT("ZainoPienoTraccia", "Lo zaino e' pieno: lascia qualcosa e torna a prenderlo."), 3.f, 0.f);
			return false;
		}
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Oggetto);
		if (Taccuino && Tipo)
		{
			Taccuino->AnnotaIscrizione(FString(Tipo->Nome), RigheTraccia(Oggetto));
		}
		ValdorsoRegistroEventi::Annota(TEXT("Storia"), Chi, nullptr, Chi ? Chi->GetActorLocation() : FVector::ZeroVector,
			FString::Printf(TEXT("Quelli che non tornarono: %s"), *Oggetto.ToString()));
		return true;
	}

	bool EventoDialogo(FName Evento, APlayerController* Chi, FName& InOutProssimo)
	{
		AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi);
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
		if (Evento == FName(EventoMappa))
		{
			// la risposta giusta non basta: bisogna averla vista davvero (l'iscrizione della porta, letta con E, e' nel
			// taccuino). Chi ha solo sentito dire va a "bugia": "O non hai guardato"
			if (Taccuino && !Taccuino->TitoliIscrizioni().Contains(FString(TitoloPortaAntichi)))
			{
				InOutProssimo = FName(TEXT("bugia"));
				return true;
			}
			if (Zaino && ValdorsoZaino::Conta(Zaino->Contenuto(), TEXT("Mappa_Gallerie")) == 0 && Zaino->Dai(TEXT("Mappa_Gallerie"), 1) <= 0)
			{
				// lo zaino e' pieno: Lucia non da' le pagine, il dialogo si chiude senza segno (si puo' tornare)
				if (Giocatore)
				{
					Giocatore->ClientAvviso(LOCTEXT("ZainoPienoMappa", "Lo zaino e' pieno: fai posto, e torna da Lucia per le pagine."), 4.f, 0.f);
				}
				InOutProssimo = NAME_None;
				return true;
			}
			if (Taccuino)
			{
				Taccuino->AnnotaIscrizione(TitoloStoria(), RigheStoria());
			}
			ValdorsoRegistroEventi::Annota(TEXT("Storia"), Chi ? Chi->GetPawn() : nullptr, nullptr, FVector::ZeroVector,
				TEXT("Quelli che non tornarono: Lucia da' le due pagine"));
			return true;
		}
		if (Evento == FName(EventoConsegna))
		{
			if (!Zaino || !Giocatore)
			{
				InOutProssimo = FName(NodoMancano);
				return true;
			}
			TArray<FString> Mancano;
			for (const FName& Traccia : Tracce())
			{
				if (ValdorsoZaino::Conta(Zaino->Contenuto(), Traccia) == 0)
				{
					const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Traccia);
					Mancano.Add(Tipo ? FString(Tipo->Nome).ToLower() : Traccia.ToString());
				}
			}
			if (Mancano.Num() > 0)
			{
				InOutProssimo = FName(NodoMancano);
				Giocatore->ClientAvviso(FText::Format(LOCTEXT("Mancano", "Ti manca ancora: {0}."), FText::FromString(FString::Join(Mancano, TEXT(", ")))), 5.f, 0.f);
				return true;
			}
			// le tre cose vanno a Lucia; a te le monete della cassa e i punti della Gilda
			Zaino->Cambia([](FValdorsoZaino& Z)
			{
				for (const FName& Traccia : Tracce())
				{
					ValdorsoZaino::TogliOggetto(Z, Traccia, 1);
				}
				Z.Monete = FMath::Clamp(Z.Monete + 60, 0, ValdorsoZaino::MassimoMonete);
				return true;
			});
			if (UValdorsoGildaComponent* Gilda = UValdorsoGildaComponent::Di(Chi))
			{
				Gilda->ImpostaPunti(Gilda->Contenuto().Punti + 8);
			}
			if (Taccuino)
			{
				Taccuino->AnnotaIscrizione(TitoloStoria() + TEXT(" (la fine)"), RigheFine());
			}
			Giocatore->ClientAvviso(LOCTEXT("Ricompensa", "Lucia ti da' 60 Aureus dalla cassa della Gilda, e scrive il tuo nome nel registro: 8 punti."), 6.f, 0.f);
			ValdorsoRegistroEventi::Annota(TEXT("Storia"), Chi->GetPawn(), nullptr, FVector::ZeroVector,
				TEXT("Quelli che non tornarono: finita (60 Aureus, 8 punti della Gilda)"));
			return true;
		}
		return false;
	}
}

// ----------------------------------------------------------------------------------------------- la frana

AValdorsoFrana::AValdorsoFrana()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	RootComponent = Radice;
	// i sassi davanti al portale: un mucchio, dal basso in alto (spariscono dall'alto)
	FRandomStream Caso(312);
	for (int32 i = 0; i < GrottaSassi; ++i)
	{
		const FName Nome(*FString::Printf(TEXT("Sasso%d"), i));
		UStaticMeshComponent* Sasso = CreateDefaultSubobject<UStaticMeshComponent>(Nome);
		Sasso->SetupAttachment(Radice);
		const float Su = static_cast<float>(i / 4) * 60.f;
		Sasso->SetRelativeLocation(FVector(Caso.FRandRange(-60.f, 30.f), Caso.FRandRange(-180.f, 180.f), 20.f + Su));
		Sasso->SetRelativeRotation(FRotator(Caso.FRandRange(-30.f, 30.f), Caso.FRandRange(0.f, 360.f), Caso.FRandRange(-30.f, 30.f)));
		Sasso->SetRelativeScale3D(FVector(Caso.FRandRange(0.8f, 1.3f)));
		Sasso->SetCollisionProfileName(TEXT("BlockAll"));
		Sassi.Add(Sasso);
	}
	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	Presa->SetupAttachment(Radice);
	Presa->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	GrottaPresa(Presa, 120.f);
}

AValdorsoFrana* AValdorsoFrana::Di(const UWorld* Mondo)
{
	if (!Mondo)
	{
		return nullptr;
	}
	TActorIterator<AValdorsoFrana> It(Mondo);
	return It ? *It : nullptr;
}

void AValdorsoFrana::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoFrana, Stato);
	DOREPLIFETIME(AValdorsoFrana, Scavo);
}

void AValdorsoFrana::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (UStaticMesh* Modello = GrottaModello(GrottaModelloSasso))
		{
			for (UStaticMeshComponent* Sasso : Sassi)
			{
				Sasso->SetStaticMesh(Modello);
			}
		}
	}
	if (HasAuthority())
	{
		Carica();
	}
	Mostra();
	// con World Partition la frana vera puo' arrivare dopo (si carica quando ci si avvicina): ogni tanto si ricontrolla
	FTimerHandle GrottaGiroMostra;
	GetWorldTimerManager().SetTimer(GrottaGiroMostra, this, &AValdorsoFrana::Mostra, 4.f, true, 4.f);
}

FString AValdorsoFrana::File() const
{
	const UWorld* Mondo = GetWorld();
	const UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	const TCHAR* Cartella = Mondo && Mondo->IsPlayInEditor() ? TEXT("Prova") : (Archivista && Archivista->ArchivioSolo() ? TEXT("Solo") : TEXT("Server"));
	return FPaths::ProjectSavedDir() / Cartella / TEXT("GrottaNord.json");
}

void AValdorsoFrana::Carica()
{
	FString Testo;
	FValdorsoGrottaSalvata Letta;
	if (FFileHelper::LoadFileToString(Testo, *File()) && FJsonObjectConverter::JsonObjectStringToUStruct(Testo, &Letta, 0, 0))
	{
		Stato = static_cast<EValdorsoStatoFrana>(FMath::Clamp(Letta.Stato, 0, 2));
		Scavo = FMath::Clamp(Letta.Scavo, 0.f, 1.f);
		ApertaAlle = Letta.Aperta;
		UltimoDecimo = FMath::FloorToInt32(Scavo * 10.f);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Grotta: frana %d, scavo %.0f%% (%s)"), static_cast<int32>(Stato), Scavo * 100.f, *File());
}

void AValdorsoFrana::Salva() const
{
	FValdorsoGrottaSalvata Dati;
	Dati.Stato = static_cast<int32>(Stato);
	Dati.Scavo = Scavo;
	Dati.Aperta = ApertaAlle;
	FString Testo;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Dati, Testo))
	{
		return;
	}
	const FString Percorso = File();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Percorso), true);
	FFileHelper::SaveStringToFile(Testo, *Percorso, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void AValdorsoFrana::OnRep_Frana()
{
	Mostra();
}

void AValdorsoFrana::Mostra()
{
	// i sassi: tutti chiusa, sempre meno smossa, nessuno aperta (spariscono quelli in alto per primi)
	const int32 Restano = Stato == EValdorsoStatoFrana::Aperta ? 0
		: (Stato == EValdorsoStatoFrana::Chiusa ? GrottaSassi : FMath::CeilToInt32(GrottaSassi * (1.f - Scavo)));
	for (int32 i = 0; i < Sassi.Num(); ++i)
	{
		const bool bC = i < Restano;
		Sassi[i]->SetVisibility(bC);
		Sassi[i]->SetCollisionEnabled(bC ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
	// la frana vera (crea_villaggio.py, etichetta "ValdorsoFrana"): sparisce quando si apre
	const bool bAperta = Stato == EValdorsoStatoFrana::Aperta;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("ValdorsoFrana")))
		{
			It->SetActorHiddenInGame(bAperta);
			It->SetActorEnableCollision(!bAperta);
		}
	}
}

void AValdorsoFrana::Avvisa(APawn* Chi, const FText& Testo, float Secondi) const
{
	if (AValdorsoPlayerController* Giocatore = GrottaGiocatore(Chi))
	{
		Giocatore->ClientAvviso(Testo, Secondi, 0.f);
	}
}

void AValdorsoFrana::AvvisaVicini(const FText& Testo, float Raggio) const
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(It->Get());
		const APawn* Corpo = Giocatore ? Giocatore->GetPawn() : nullptr;
		if (Corpo && FVector::Dist2D(Corpo->GetActorLocation(), GetActorLocation()) <= Raggio)
		{
			Giocatore->ClientAvviso(Testo, 5.f, 0.f);
		}
	}
}

void AValdorsoFrana::Trema()
{
	if (!HasAuthority() || Stato != EValdorsoStatoFrana::Chiusa)
	{
		return;
	}
	Stato = EValdorsoStatoFrana::Smossa;
	Scavo = 0.f;
	UltimoDecimo = 0;
	Salva();
	Mostra();
	MulticastTrema(false);
	AvvisaVicini(LOCTEXT("Trema", "La terra trema sotto la montagna del nord. Qualcosa, laggiu', si e' mosso."), GrottaRaggioTrema);
	ValdorsoRegistroEventi::Annota(TEXT("Grotta"), this, nullptr, GetActorLocation(), TEXT("la frana del nord trema: si puo' scavare"));
}

void AValdorsoFrana::Apri()
{
	if (!HasAuthority() || Stato == EValdorsoStatoFrana::Aperta)
	{
		return;
	}
	Stato = EValdorsoStatoFrana::Aperta;
	Scavo = 1.f;
	ApertaAlle = FDateTime::UtcNow().ToUnixTimestamp();
	Salva();
	Mostra();
	MulticastTrema(true);
	AvvisaVicini(LOCTEXT("Cede", "La frana del nord cede con un boato: le gallerie dei nani sono aperte."), GrottaRaggioTrema);
	ValdorsoRegistroEventi::Annota(TEXT("Grotta"), this, nullptr, GetActorLocation(), TEXT("la frana del nord e' aperta"));
}

void AValdorsoFrana::Chiudi()
{
	if (!HasAuthority())
	{
		return;
	}
	Stato = EValdorsoStatoFrana::Chiusa;
	Scavo = 0.f;
	UltimoDecimo = 0;
	ApertaAlle = 0;
	Salva();
	Mostra();
	ValdorsoRegistroEventi::Annota(TEXT("Grotta"), this, nullptr, GetActorLocation(), TEXT("la frana del nord e' di nuovo chiusa (staff)"));
}

void AValdorsoFrana::MulticastTrema_Implementation(bool bCede)
{
	UWorld* Mondo = GetWorld();
	if (!Mondo || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	Mostra();
	USoundBase* Suono = ValdorsoCombattimento::SuonoPericoli(TEXT("S_Frana"));
	if (!Suono)
	{
		Suono = ValdorsoCombattimento::SuonoPericoli(TEXT("S_Colpo_Botta"));
	}
	ValdorsoCombattimento::Suona(Mondo, Suono, GetActorLocation(), GrottaRaggioTrema, 1.f, bCede ? 0.35f : 0.5f);
	const APlayerController* Giocatore = Mondo->GetFirstPlayerController();
	const APawn* Corpo = Giocatore ? Giocatore->GetPawn() : nullptr;
	if (Corpo && FVector::Dist(Corpo->GetActorLocation(), GetActorLocation()) <= GrottaRaggioTrema)
	{
		if (UValdorsoReazioniComponent* Reazioni = UValdorsoReazioniComponent::Di(Corpo))
		{
			Reazioni->Scossa(bCede ? 1.f : 0.6f);
		}
		ValdorsoInterfaccia::Sottotitolo(Mondo, LOCTEXT("SottotitoloTrema", "La terra trema"), GetActorLocation());
	}
}

FText AValdorsoFrana::TestoInterazione(const APawn* Chi) const
{
	switch (Stato)
	{
	case EValdorsoStatoFrana::Aperta:
		return LOCTEXT("Entra", "Entra nelle gallerie dei nani");
	case EValdorsoStatoFrana::Smossa:
	{
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(GrottaGiocatore(Chi));
		if (Zaino && !ValdorsoGrottaNord::HaAttrezzo(Zaino->Contenuto()))
		{
			return LOCTEXT("SenzaPiccone", "Per spostare i sassi ti serve un piccone (o un martello)");
		}
		return FText::Format(LOCTEXT("Scava", "Sposta i sassi (la frana: {0}% liberata)"), FText::AsNumber(FMath::FloorToInt32(Scavo * 100.f)));
	}
	default:
		return LOCTEXT("Guarda", "Guarda la frana");
	}
}

void AValdorsoFrana::Interagisci(APawn* Chi)
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Chi || !Mondo)
	{
		return;
	}
	if (Stato == EValdorsoStatoFrana::Aperta)
	{
		ValdorsoGrottaNord::Porta(Chi, Dentro, GradiDentro, LOCTEXT("Dentro", "Scendi nelle gallerie dei nani. L'aria e' ferma e fredda."));
		return;
	}
	if (Stato == EValdorsoStatoFrana::Chiusa)
	{
		static const FText Righe[] = {
			LOCTEXT("Chiusa1", "Sassi e terra fino in cima. Qualcuno, tanto tempo fa, ha fatto crollare la galleria apposta."),
			LOCTEXT("Chiusa2", "Appoggi l'orecchio alla roccia. Niente. Poi, lontano, un colpo. E un altro."),
			LOCTEXT("Chiusa3", "Dura come la montagna. Con le mani non si sposta niente: ci vorrebbe che la terra si muovesse da sola."),
		};
		Avvisa(Chi, Righe[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Righe)) - 1)], 4.f);
		return;
	}
	// smossa: un colpo di piccone
	const double Ora = Mondo->GetTimeSeconds();
	double& Ultimo = UltimoColpo.FindOrAdd(TWeakObjectPtr<APawn>(Chi));
	if (Ora - Ultimo < ValdorsoGrottaNord::SecondiTraColpi && Ora >= Ultimo)
	{
		return;
	}
	Ultimo = Ora;
	const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(GrottaGiocatore(Chi));
	if (!Zaino || !ValdorsoGrottaNord::HaAttrezzo(Zaino->Contenuto()))
	{
		Avvisa(Chi, LOCTEXT("ServePiccone", "A mani nude non si sposta niente: ti serve un piccone (alla fucina di Marta) o un martello."));
		return;
	}
	UAbilitySystemComponent* Statistiche = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Chi);
	if (!Statistiche || !Statistiche->GetSet<UValdorsoAttributeSet>()
		|| Statistiche->GetNumericAttribute(UValdorsoAttributeSet::GetStaminaAttribute()) < ValdorsoGrottaNord::StaminaScavo)
	{
		Avvisa(Chi, LOCTEXT("FiatoScavo", "Sei senza fiato: riposati un momento."), 2.f);
		return;
	}
	FGameplayEffectSpecHandle Fatica = Statistiche->MakeOutgoingSpec(UValdorsoGE_Fatica::StaticClass(), 1.f, Statistiche->MakeEffectContext());
	if (Fatica.IsValid())
	{
		Fatica.Data->SetSetByCallerMagnitude(ValdorsoEtichette::Dato_Stamina, -ValdorsoGrottaNord::StaminaScavo);
		Statistiche->ApplyGameplayEffectSpecToSelf(*Fatica.Data.Get());
	}
	const UValdorsoCrescitaComponent* Crescita = UValdorsoCrescitaComponent::DelCorpo(Chi);
	const float Tempra = Crescita ? ValdorsoCrescere::Punti(Crescita->Contenuto(), FName(TEXT("Tempra"))) : 0.f;
	Scavo = FMath::Min(1.f, Scavo + ValdorsoGrottaNord::PassoScavo(Tempra));
	UValdorsoCrescitaComponent::UsaDa(Chi, FName(TEXT("Tempra")), 0.5f, this);
	ValdorsoCombattimento::Suona(Mondo, ValdorsoCombattimento::SuonoPericoli(TEXT("S_Colpo_Botta")), GetActorLocation(), 2500.f, 0.7f,
		FMath::FRandRange(0.7f, 0.9f));
	const int32 Decimo = FMath::FloorToInt32(Scavo * 10.f);
	if (Decimo != UltimoDecimo)
	{
		UltimoDecimo = Decimo;
		Salva();
		AvvisaVicini(FText::Format(LOCTEXT("Avanza", "La frana del nord: {0}% liberata."), FText::AsNumber(Decimo * 10)), 6000.f);
	}
	Mostra();
	if (Scavo >= 1.f)
	{
		ValdorsoRegistroEventi::Annota(TEXT("Grotta"), Chi, this, GetActorLocation(), TEXT("l'ultimo sasso della frana"));
		Apri();
	}
}

// ----------------------------------------------------------------------------------------------- il passaggio

AValdorsoPassaggio::AValdorsoPassaggio()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	RootComponent = Presa;
	GrottaPresa(Presa, 90.f);
}

FText AValdorsoPassaggio::TestoInterazione(const APawn* Chi) const
{
	return Destinazione.IsZero() ? FText::GetEmpty() : FText::FromString(Testo);
}

void AValdorsoPassaggio::Interagisci(APawn* Chi)
{
	if (HasAuthority())
	{
		ValdorsoGrottaNord::Porta(Chi, Destinazione, GradiDestinazione, LOCTEXT("Fuori", "Risali verso la luce."));
	}
}

// ----------------------------------------------------------------------------------------------- il forziere

AValdorsoForziere::AValdorsoForziere()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Cassa = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cassa"));
	RootComponent = Cassa;
	Cassa->SetCollisionProfileName(TEXT("BlockAll"));
}

void AValdorsoForziere::BeginPlay()
{
	Super::BeginPlay();
	// il modello lo mette crea_grotta_nord.py; se manca (o non e' ancora stato importato) lo si cerca qui
	if (!Cassa->GetStaticMesh())
	{
		if (UStaticMesh* Modello = GrottaModello(GrottaModelloForziere))
		{
			Cassa->SetStaticMesh(Modello);
		}
	}
}

FText AValdorsoForziere::TestoInterazione(const APawn* Chi) const
{
	return LOCTEXT("ApriForziere", "Apri il forziere dei nani");
}

void AValdorsoForziere::Interagisci(APawn* Chi)
{
	AValdorsoPlayerController* Giocatore = GrottaGiocatore(Chi);
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Giocatore);
	if (!HasAuthority() || !Zaino)
	{
		return;
	}
	const FString Nome = GrottaNomeDi(Chi);
	const int64 Ora = FDateTime::UtcNow().ToUnixTimestamp();
	if (const int64* Quando = Aperto.Find(Nome); Quando && Ora - *Quando < GrottaSecondiForziere)
	{
		Giocatore->ClientAvviso(LOCTEXT("Vuoto", "Il forziere e' vuoto. Dentro, solo polvere e il tuo segno di ieri."), 3.f, 0.f);
		return;
	}
	Aperto.Add(Nome, Ora);
	FRandomStream Caso(static_cast<int32>(FPlatformTime::Cycles()));
	const int32 Monete = Caso.RandRange(3, 6) * Ricchezza;
	const int32 Aureus = Caso.RandRange(10, 25) * Ricchezza;
	Zaino->Dai(TEXT("Moneta_Nani"), Monete);
	Zaino->Cambia([Aureus](FValdorsoZaino& Z)
	{
		Z.Monete = FMath::Clamp(Z.Monete + Aureus, 0, ValdorsoZaino::MassimoMonete);
		return true;
	});
	if (Caso.FRand() < 0.15f * static_cast<float>(Ricchezza))
	{
		Zaino->Dai(TEXT("Pugnale_Nani"), 1);
	}
	Giocatore->ClientAvviso(FText::Format(LOCTEXT("Aperto", "Nel forziere: {0} monete dei nani e {1} Aureus."), FText::AsNumber(Monete),
		FText::AsNumber(Aureus)), 4.f, 0.f);
	ValdorsoRegistroEventi::Annota(TEXT("Forziere"), Chi, this, GetActorLocation(), FString::Printf(TEXT("%d monete, %d Aureus"), Monete, Aureus));
}

// ----------------------------------------------------------------------------------------------- le tracce

AValdorsoTraccia::AValdorsoTraccia()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Figura = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Figura"));
	RootComponent = Figura;
	Figura->SetCollisionProfileName(TEXT("NoCollision"));
	Presa = CreateDefaultSubobject<USphereComponent>(TEXT("Presa"));
	Presa->SetupAttachment(Figura);
	Presa->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	GrottaPresa(Presa, 45.f);
}

FText AValdorsoTraccia::TestoInterazione(const APawn* Chi) const
{
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Oggetto);
	const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(GrottaGiocatore(Chi));
	if (!Tipo || (Zaino && ValdorsoZaino::Conta(Zaino->Contenuto(), Oggetto) > 0))
	{
		return FText::GetEmpty();
	}
	return FText::Format(LOCTEXT("Raccogli", "Raccogli: {0}"), FText::FromString(Tipo->Nome));
}

void AValdorsoTraccia::Interagisci(APawn* Chi)
{
	if (HasAuthority() && !ValdorsoGrottaNord::DaiTraccia(Chi, Oggetto))
	{
		if (AValdorsoPlayerController* Giocatore = GrottaGiocatore(Chi))
		{
			Giocatore->ClientAvviso(LOCTEXT("GiaPresa", "Non c'e' piu' niente da prendere: e' gia' con te, o con Lucia."), 3.f, 0.f);
		}
	}
}

#undef LOCTEXT_NAMESPACE
