// Valdorso - I PNG della valle (vedi il .h). Scritto da Claude il 07/11/2026.
// Unity build: le cose del namespace anonimo cominciano con Png, così non si scontrano con gli altri file.

#include "ValdorsoPNG.h"
#include "ValdorsoGrottaNord.h"
#include "ValdorsoRiti.h"
#include "ValdorsoCarovana.h"
#include "ValdorsoBotteghe.h"
#include "ValdorsoStorie.h"
#include "ValdorsoFiducia.h"
#include "ValdorsoMilizia.h"
#include "ValdorsoLocanda.h"
#include "ValdorsoContratto.h"
#include "ValdorsoBattito.h"
#include "ValdorsoStatoColono.h"
#include "ValdorsoVoci.h"
#include "ValdorsoGilda.h"
#include "ValdorsoChat.h"
#include "ValdorsoCielo.h"
#include "ValdorsoFasiVillaggio.h"
#include "ValdorsoDialoghi.h"
#include "EngineUtils.h"
#include "Valdorso.h"
#include "ValdorsoAspetto.h"
#include "ValdorsoAspettoComponent.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoZaino.h"
#include "ValdorsoSeduta.h"
#include "ValdorsoLetto.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ValdorsoPNG"

namespace
{
	/** Mezza altezza e raggio della capsula (cm): come un colono. L'origine dell'attore è al centro della capsula. */
	constexpr float PngMezzaAltezza = 92.f;
	constexpr float PngRaggio = 36.f;

	/** Dopo quanti secondi senza conversazioni il PNG torna a guardare dove guardava. */
	constexpr float PngSecondiRitorno = 4.f;

	// (14/11, "PNG che vivono") seduti, a letto, le porte
	constexpr uint8 PngInPiedi = 0;
	constexpr uint8 PngSeduto = 1;
	constexpr uint8 PngDorme = 2;
	const TCHAR* PngMovenzaPanca = TEXT("Panca");
	const TCHAR* PngMovenzaLetto = TEXT("Letto");
	/** Le stesse animazioni dei coloni: il seduto della panca del GASP (porta_panca_da_gasp.py) e il sonno di Mixamo
	 *  (porta_dormire_da_mixamo.py), gia' messe a punto sulle nostre panche e sui nostri letti. */
	const TCHAR* PngAnimSeduto = TEXT("/Game/Fab/GameAnimationSample/Panca/Seduto.Seduto");
	const TCHAR* PngAnimDorme = TEXT("/Game/Mixamo/Dormire/Dorme.Dorme");
	/** Quanto lontano da Dove puo' stare la panca o il letto della tappa (cm, in piano; e in altezza meno di 2 m). */
	constexpr float PngRaggioMobile = 350.f;
	/** Quanto davanti alla panca si rimette in piedi chi si alza (cm). */
	constexpr float PngDavantiPanca = 65.f;
	/** Quanto vicino a Porta deve stare la porta della tappa (cm). */
	constexpr float PngRaggioPorta = 250.f;

	// (14/11, "PNG che camminano")
	/** Il passo dei PNG (cm al secondo): uguale per tutti i PC (i conti del cammino si fanno su ogni PC). */
	constexpr float PngVelocitaPasso = 140.f;
	/** Quanto si ferma sulla soglia per aprire la porta (secondi), e dopo quanto la porta si richiude. */
	constexpr float PngSostaPorta = 1.4f;
	constexpr double PngChiudiDopo = 4.0;
	const TCHAR* PngAnimPasseggio = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd");
	/** Aprire la porta: dalla Free Animation Library (porta_armi_paragon.py la porta nel nostro manichino). */
	const TCHAR* PngAnimApre = TEXT("/Game/Fab/Gesti/Apre_Porta.Apre_Porta");

	TAutoConsoleVariable<float> CVarPngRitmoPasso(
		TEXT("Valdorso.PNG.RitmoPasso"),
		0.9f,
		TEXT("Velocita' dell'animazione del passo dei PNG (1 = come e' fatta): da ritoccare se i piedi scivolano."),
		ECVF_Default);

	/** L'animazione del seduto o del sonno, ferma sul posto come per i coloni (la radice resta dove la mettiamo noi). */
	UAnimSequence* PngAnimPosa(const TCHAR* Percorso)
	{
		UAnimSequence* Sequenza = LoadObject<UAnimSequence>(nullptr, Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (Sequenza)
		{
			Sequenza->bEnableRootMotion = true;
			Sequenza->RootMotionRootLock = ERootMotionRootLock::RefPose;
		}
		return Sequenza;
	}

	/** Il nome con cui il colono entra nella memoria dei dialoghi: il nome del personaggio (o dell'attore). */
	FString PngNomeDi(const APawn* Chi)
	{
		if (!Chi)
		{
			return FString();
		}
		if (const APlayerState* Stato = Chi->GetPlayerState())
		{
			return Stato->GetPlayerName();
		}
		return Chi->GetName();
	}

	/**
	 * (08/11) La memoria dei dialoghi di quel colono: nel file del suo personaggio (il taccuino sul PlayerController,
	 * che salva il personaggio quando cambia), così resta anche quando il server si spegne. Chi non ha un personaggio
	 * salvato (l'accesso libero dell'editor) usa la memoria per nome del server, come prima.
	 */
	class FPngMemoriaColono final : public FValdorsoMemoriaDialoghi
	{
	public:
		explicit FPngMemoriaColono(const APawn* Colono)
			: PerNome(PngNomeDi(Colono))
		{
			const APlayerController* Controllore = Colono ? Cast<APlayerController>(Colono->GetController()) : nullptr;
			Zaino = UValdorsoZainoComponent::Di(Controllore);
			UValdorsoTaccuinoComponent* Diario = UValdorsoTaccuinoComponent::Di(Controllore);
			Taccuino = (Diario && Diario->HaPersonaggio()) ? Diario : nullptr;
			// (14/11, v0.2.4) la fiducia vale anche senza un personaggio salvato (solo nella memoria del server)
			PerFiducia = Diario;
		}

		/** Vero se la memoria è quella del file del personaggio. */
		bool NelFile() const { return Taccuino != nullptr; }

		virtual bool GiaParlato(FName IdDialogo) const override
		{
			return Taccuino ? Taccuino->GiaParlato(IdDialogo) : PerNome.GiaParlato(IdDialogo);
		}

		virtual void SegnaParlato(FName IdDialogo) override
		{
			if (Taccuino)
			{
				Taccuino->SegnaParlato(IdDialogo);
			}
			else
			{
				PerNome.SegnaParlato(IdDialogo);
			}
		}

		virtual bool HaSegno(FName IdDialogo, FName Segno) const override
		{
			return Taccuino ? Taccuino->HaSegno(IdDialogo, Segno) : PerNome.HaSegno(IdDialogo, Segno);
		}

		virtual void MettiSegno(FName IdDialogo, FName Segno) override
		{
			// (08/11) certi segni portano un dono nello zaino (il pane di Bruno, le erbe di Nonna Edda): solo la prima volta
			FName Dono = NAME_None;
			int32 Quanti = 0;
			const bool bNuovo = !Segno.IsNone() && !HaSegno(IdDialogo, Segno);
			if (Zaino && bNuovo && ValdorsoOggetti::DonoDelSegno(IdDialogo, Segno, Dono, Quanti))
			{
				Zaino->Dai(Dono, Quanti);
			}
			// (14/11, v0.2.4) un passo di una storia, un favore: la fiducia sale
			if (bNuovo && PerFiducia)
			{
				PerFiducia->CambiaFiducia(IdDialogo, ValdorsoFiducia::PerSegno);
			}
			if (Taccuino)
			{
				Taccuino->MettiSegno(IdDialogo, Segno);
			}
			else
			{
				PerNome.MettiSegno(IdDialogo, Segno);
			}
		}

	private:
		FValdorsoMemoriaPerNome PerNome;
		UValdorsoTaccuinoComponent* Taccuino = nullptr;
		UValdorsoTaccuinoComponent* PerFiducia = nullptr;
		UValdorsoZainoComponent* Zaino = nullptr;
	};

	/** (14/11, v0.2.4) La riga di un fatto dei giorni passati di cui questo PNG non ha ancora parlato (e da adesso ne ha
	 *  parlato); vuota se non ce n'e'. */
	FString PngRicordo(UValdorsoTaccuinoComponent* Taccuino, FName IdDialogo)
	{
		FValdorsoFatto Fatto;
		const int64 Adesso = FDateTime::UtcNow().ToUnixTimestamp();
		if (!Taccuino || !Taccuino->FattoDaRicordare(IdDialogo, Adesso, Fatto))
		{
			return FString();
		}
		Taccuino->SegnaRicordato(IdDialogo, Fatto.Quando);
		return ValdorsoFiducia::RigaDelFatto(Fatto, Adesso, FMath::Rand());
	}

	/** (14/11) Le animazioni della corsa del manichino (la prima che c'e'). */
	const TCHAR* const PngCorse[] = {
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd"),
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MM_Jog_Fwd.MM_Jog_Fwd"),
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"),
	};

	/** (08/11) Chi arriva al nodo Nodo riceve il suo segno, se ne ha uno (vedi ValdorsoDialoghi.h, "I segni"). */
	void PngSegnaNodo(const FValdorsoDialogo& Dati, FName Nodo, FValdorsoMemoriaDialoghi& Memoria)
	{
		if (const FValdorsoNodoDialogo* Arrivo = Dati.Trova(Nodo))
		{
			ValdorsoDialoghi::MettiSegnoDi(Memoria, Dati.Id, Arrivo->Segna);
		}
	}
}

AValdorsoPNG::AValdorsoPNG()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(5.f);
	// Non si uccide: niente danni (e niente statistiche: i pugni non trovano un Ability System).
	SetCanBeDamaged(false);

	Capsula = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsula"));
	Capsula->InitCapsuleSize(PngRaggio, PngMezzaAltezza);
	// Come un colono per le collisioni (i coloni ci sbattono, E la trova), ma ferma: non simula la fisica, quindi non
	// si spinge. La telecamera ci passa attraverso, e sulla testa non si sale.
	Capsula->SetCollisionProfileName(TEXT("Pawn"));
	Capsula->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Capsula->SetSimulatePhysics(false);
	Capsula->CanCharacterStepUpOn = ECB_No;
	Capsula->SetCanEverAffectNavigation(false);
	RootComponent = Capsula;

	// Il manichino di Unreal guarda verso +Y: lo si gira di -90 perché guardi verso la X dell'attore; i piedi in basso.
	Corpo = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Corpo"));
	Corpo->SetupAttachment(Capsula);
	Corpo->SetRelativeLocation(FVector(0.f, 0.f, -PngMezzaAltezza));
	Corpo->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Corpo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Corpo->SetCanEverAffectNavigation(false);
	Corpo->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// Gli stessi del palco del Registro (ValdorsoPalcoRegistro.cpp): il manichino e il suo respiro da fermo.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Quinn(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Fermo(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	ManichinoUomo = Manny.Object;
	ManichinoDonna = Quinn.Object;
	Riposo = Fermo.Object;
	if (ManichinoUomo)
	{
		Corpo->SetSkeletalMesh(ManichinoUomo);
	}

	Aspetto = CreateDefaultSubobject<UValdorsoAspettoComponent>(TEXT("Aspetto"));
}

void AValdorsoPNG::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoPNG, Dialogo);
	DOREPLIFETIME(AValdorsoPNG, Sguardo);
	DOREPLIFETIME(AValdorsoPNG, Tappa);
	DOREPLIFETIME(AValdorsoPNG, bVia);
	DOREPLIFETIME(AValdorsoPNG, Barcollate);
	DOREPLIFETIME(AValdorsoPNG, Cadute);
	DOREPLIFETIME(AValdorsoPNG, bATerra);
	DOREPLIFETIME(AValdorsoPNG, DoveFuga);
	DOREPLIFETIME(AValdorsoPNG, bInFuga);
	DOREPLIFETIME(AValdorsoPNG, bAlzato);
	DOREPLIFETIME(AValdorsoPNG, bCammina);
	DOREPLIFETIME(AValdorsoPNG, CamminoInizio);
	DOREPLIFETIME(AValdorsoPNG, CamminoFermo);
	DOREPLIFETIME(AValdorsoPNG, StradaUsata);
}

void AValdorsoPNG::BeginPlay()
{
	// Prima di Super::BeginPlay (che fa partire i componenti): il MetaHuman va sul manichino del PNG, con il volto
	// deciso dal server. Così il corpo si fa una volta sola.
	const FString Sesso = bDonna ? FString(TEXT("donna")) : FString(TEXT("uomo"));
	if (Corpo)
	{
		USkeletalMesh* Manichino = bDonna ? ManichinoDonna.Get() : ManichinoUomo.Get();
		if (Manichino)
		{
			Corpo->SetSkeletalMesh(Manichino);
		}
		if (Riposo)
		{
			Corpo->PlayAnimation(Riposo, true);
		}
		else
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] PNG %s: manca l'animazione MM_Idle, resta fermo in posa."), *GetName());
		}
	}
	if (Aspetto)
	{
		Aspetto->UsaSupporto(Corpo, nullptr);
		if (HasAuthority())
		{
			FValdorsoAspetto Volto = ValdorsoAspetto::Visibile(FValdorsoAspetto(), Sesso, Dialogo.ToString());
			if (Testa >= 0)
			{
				ValdorsoAspetto::Imposta(Volto, TEXT("Testa"), static_cast<uint8>(FMath::Clamp(Testa, 0, 255)));
			}
			Aspetto->ImpostaAspetto(Volto, Sesso);
		}
	}

	Super::BeginPlay();

	SguardoIniziale = static_cast<float>(GetActorRotation().Yaw);
	if (HasAuthority())
	{
		Sguardo = SguardoIniziale;
		bSguardoValido = true;
		if (!DatiDialogo())
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] PNG %s: il dialogo \"%s\" non esiste (ValdorsoDialoghiTesti.inl)."),
				*GetName(), *Dialogo.ToString());
		}
		// (10/11) i saluti a chi passa (ognuno con il suo ritmo, non tutti insieme)
		GetWorldTimerManager().SetTimer(TimerSaluti, this, &AValdorsoPNG::Saluta, 2.f, true, FMath::FRandRange(1.f, 3.f));
		// (11/11) la giornata (e chi arriva solo con un atto della storia)
		if (Giornata.Num() > 0 || AttoMinimo > 0 || OgniGiorni > 0)
		{
			GetWorldTimerManager().SetTimer(TimerGiornata, this, &AValdorsoPNG::GiroGiornata, 10.f, true, 0.5f);
		}
	}
	else if (bSguardoValido)
	{
		SetActorTickEnabled(true);
	}
	// (11/11) la tappa arrivata dalla rete prima del BeginPlay: di nuovo, ora che il corpo c'e' (e il Riposo qui sopra
	// non deve coprire il gesto della tappa)
	if (!HasAuthority() && (Tappa != INDEX_NONE || bVia))
	{
		OnRep_Tappa();
	}
	// (14/11) arrivato mentre scappa: corre anche qui
	if (!HasAuthority() && bInFuga)
	{
		OnRep_Fuga();
	}
	// (14/11) arrivato mentre cammina
	if (!HasAuthority() && bCammina)
	{
		OnRep_Cammino();
	}
}

void AValdorsoPNG::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerRitorno);
		Mondo->GetTimerManager().ClearTimer(TimerSaluti);
		Mondo->GetTimerManager().ClearTimer(TimerGiornata);
		Mondo->GetTimerManager().ClearTimer(TimerATerra);
		Mondo->GetTimerManager().ClearTimer(TimerFuga);
		Mondo->GetTimerManager().ClearTimer(TimerCammino);
	}
	LiberaMobile();
	Conversazioni.Reset();
	Super::EndPlay(EndPlayReason);
}

const FValdorsoDialogo* AValdorsoPNG::DatiDialogo() const
{
	return ValdorsoDialoghi::Trova(Dialogo);
}

const FValdorsoDialogo* AValdorsoPNG::DatiDi(const FConversazione& Conversazione) const
{
	// (la scelta di una storia del Battito si fa con il suo dialogo: il nome e il titolo)
	return (Conversazione.Altro.IsNone() || Conversazione.Altro == FName(TEXT("Battito"))) ? DatiDialogo() : ValdorsoDialoghi::Trova(Conversazione.Altro);
}

FString AValdorsoPNG::NomeBreve() const
{
	const FValdorsoDialogo* Dati = DatiDialogo();
	return Dati ? Dati->NomeBreve : GetName();
}

bool AValdorsoPNG::Guardia() const
{
	const FString Id = Dialogo.ToString();
	return Id.StartsWith(TEXT("Guardia")) || Id.Equals(TEXT("Livia"), ESearchCase::IgnoreCase);
}

FText AValdorsoPNG::TestoInterazione(const APawn* Chi) const
{
	// (14/11) a terra o in fuga non si parla; (14/11) nemmeno con chi dorme
	if (bVia || bATerra || bInFuga || Dorme())
	{
		return FText::GetEmpty();
	}
	const FValdorsoDialogo* Dati = DatiDialogo();
	return Dati ? FText::Format(LOCTEXT("Parla", "Parla con {0}"), FText::FromString(Dati->NomeBreve)) : FText::GetEmpty();
}

FVector AValdorsoPNG::PuntoInterazione() const
{
	// Il centro della capsula: all'altezza del centro del colono, così la distanza si misura in piano.
	return GetActorLocation();
}

void AValdorsoPNG::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi || bVia || bATerra || bInFuga || Dorme())
	{
		return;
	}
	// (14/11, v0.2.4) una storia del Battito aspetta la scelta da lui: quella, al posto del dialogo
	if (UValdorsoBattito* Battito = UValdorsoBattito::Di(this); Battito && Battito->ApriScelta(this, Chi))
	{
		return;
	}
	if (const FValdorsoDialogo* Dati = DatiDialogo())
	{
		Comincia(Chi, *Dati, NAME_None);
	}
}

bool AValdorsoPNG::ApriSceltaStoria(APawn* Chi, const FString& IdStoria, const TArray<FString>& Righe, const TArray<FString>& Scelte)
{
	APlayerController* Controllore = Chi ? Cast<APlayerController>(Chi->GetController()) : nullptr;
	const FValdorsoDialogo* Dati = DatiDialogo();
	if (!HasAuthority() || !Controllore || Scelte.Num() == 0)
	{
		return false;
	}
	Chiudi(Controllore, false, FString());
	FConversazione Nuova;
	Nuova.Altro = FName(TEXT("Battito"));
	Nuova.Storia = IdStoria;
	Nuova.Scelte = FMath::Min(Scelte.Num(), ValdorsoDialoghi::MassimoScelte);
	Nuova.Passo = ProssimoPasso++;
	Conversazioni.Add(TWeakObjectPtr<APlayerController>(Controllore), Nuova);
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerRitorno);
	}
	GuardaVerso(Chi);
	FValdorsoBattuta Battuta;
	Battuta.Chi = Dati ? Dati->Titolo : NomeBreve();
	Battuta.Passo = Nuova.Passo;
	for (const FString& Riga : Righe)
	{
		Battuta.Righe.Add(Riga.Left(ValdorsoDialoghi::MassimoLettere));
	}
	for (int32 i = 0; i < Nuova.Scelte; ++i)
	{
		Battuta.Scelte.Add(Scelte[i].Left(ValdorsoDialoghi::MassimoLettere));
	}
	Manda(Controllore, Battuta, Dati ? Dati->Id : FName(NAME_None));
	return true;
}

bool AValdorsoPNG::ApriDialogo(APawn* Chi, FName IdDialogo)
{
	const FValdorsoDialogo* Dati = ValdorsoDialoghi::Trova(IdDialogo);
	if (!HasAuthority() || !Chi || !Dati)
	{
		return false;
	}
	return Comincia(Chi, *Dati, IdDialogo);
}

bool AValdorsoPNG::Comincia(APawn* Chi, const FValdorsoDialogo& Dati, FName IdAltro)
{
	APlayerController* Controllore = Cast<APlayerController>(Chi->GetController());
	if (!Controllore)
	{
		return false;
	}
	// Via le conversazioni di chi se n'è andato dal server.
	for (auto It = Conversazioni.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	// (14/11) una conversazione gia' aperta con lui si chiude prima (cosi' anche l'arresto lasciato a meta' si sa)
	Chiudi(Controllore, false, FString());

	// (08/11) La memoria di questo colono: nel file del suo personaggio (o, senza, per nome).
	FPngMemoriaColono Memoria(Chi);
	const bool bGiaParlato = Memoria.GiaParlato(Dati.Id);
	Memoria.SegnaParlato(Dati.Id);

	FConversazione Nuova;
	Nuova.Nodo = ValdorsoDialoghi::NodoIniziale(Dati, bGiaParlato);
	Nuova.Passo = ProssimoPasso++;
	Nuova.Altro = IdAltro;
	Conversazioni.Add(TWeakObjectPtr<APlayerController>(Controllore), Nuova);
	PngSegnaNodo(Dati, Nuova.Nodo, Memoria);

	ValdorsoRegistroEventi::Annota(TEXT("Dialogo"), Chi, this, GetActorLocation(),
		FString::Printf(TEXT("parla con %s (%s%s)"), *Dati.NomeBreve, bGiaParlato ? TEXT("di nuovo") : TEXT("la prima volta"),
			Memoria.NelFile() ? TEXT("") : TEXT(", senza personaggio salvato")));

	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerRitorno);
	}
	GuardaVerso(Chi);
	FValdorsoBattuta Prima = ValdorsoDialoghi::Battuta(Dati, Nuova.Nodo, Nuova.Passo, Memoria);
	// (14/11, v0.2.4) il suo dialogo: la chiacchierata del giorno (la fiducia sale un poco), e in cima i fatti di ieri o
	// lo sguardo di chi non si fida
	if (IdAltro.IsNone())
	{
		UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Controllore);
		if (Taccuino)
		{
			Taccuino->Chiacchiera(Dati.Id);
		}
		const FString Ricordo = PngRicordo(Taccuino, Dati.Id);
		if (!Ricordo.IsEmpty())
		{
			Prima.Righe.Insert(Ricordo, 0);
		}
		else if (AValdorsoPlayerState::ColpevoleDi(Chi))
		{
			Prima.Righe.Insert(TEXT("(Fa un passo indietro, e ti guarda le mani.)"), 0);
		}
		else if (Taccuino && ValdorsoFiducia::Livello(Taccuino->Fiducia(Dati.Id)) == EValdorsoFiducia::Diffidente)
		{
			Prima.Righe.Insert(TEXT("(Ti squadra senza un sorriso.)"), 0);
		}
	}
	Manda(Controllore, Prima, IdAltro.IsNone() ? Dati.Id : FName(NAME_None));
	return true;
}

void AValdorsoPNG::Scegli(APlayerController* Chi, int32 Passo, int32 Scelta)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	FConversazione* Aperta = Conversazioni.Find(TWeakObjectPtr<APlayerController>(Chi));
	// Una risposta a una battuta vecchia (doppio clic, ritardo della rete) o senza conversazione: non vale.
	if (!Aperta || Aperta->Passo != Passo)
	{
		return;
	}
	APawn* Colono = Chi->GetPawn();
	// (14/11) il dialogo di questa conversazione (il suo, o quello dell'arresto)
	const FValdorsoDialogo* Dati = DatiDi(*Aperta);
	const FName IdFiducia = Aperta->Altro.IsNone() ? (Dati ? Dati->Id : FName(NAME_None)) : FName(NAME_None);
	if (!Colono || !Dati)
	{
		Chiudi(Chi, true, TEXT("senza colono"));
		return;
	}
	if (Scelta == INDEX_NONE)
	{
		Chiudi(Chi, false, TEXT("se ne va"));
		return;
	}
	// Il server non si fida: il colono deve essere ancora vicino (e vedere il PNG).
	if (FVector::Dist(Colono->GetActorLocation(), PuntoInterazionePer(Colono)) > RaggioDialogo)
	{
		Chiudi(Chi, true, TEXT("si è allontanato"));
		return;
	}
	// (14/11, v0.2.4) la scelta di una storia del Battito: il finale, e com'e' andata
	if (Aperta->Altro == FName(TEXT("Battito")))
	{
		const FString IdStoria = Aperta->Storia;
		const int32 PassoStoria = ProssimoPasso++;
		FString Esito;
		UValdorsoBattito* Battito = UValdorsoBattito::Di(this);
		if (Scelta < 0 || Scelta >= Aperta->Scelte || !Battito || !Battito->Scegli(Chi, IdStoria, Scelta, Esito))
		{
			Chiudi(Chi, true, FString());
			return;
		}
		FValdorsoBattuta Fine;
		Fine.Chi = Dati->Titolo;
		Fine.Passo = PassoStoria;
		Fine.Righe.Add(Esito.IsEmpty() ? FString(TEXT("(Annuisce, in silenzio.)")) : Esito.Left(ValdorsoDialoghi::MassimoLettere));
		Fine.bFine = true;
		Manda(Chi, Fine, Dati->Id);
		return;
	}

	const FName Prima = Aperta->Nodo;
	const FValdorsoNodoDialogo* Nodo = Dati->Trova(Prima);
	// (08/11) Il PC ha visto solo le risposte visibili per questo personaggio (senza quelle nascoste dai segni): il
	// numero che rimanda si riporta alla risposta vera.
	FPngMemoriaColono Memoria(Colono);
	const TArray<int32> Visibili = ValdorsoDialoghi::ScelteVisibili(*Dati, Prima, Memoria);
	if (!Nodo || !Visibili.IsValidIndex(Scelta))
	{
		return;  // una risposta che non c'è: si ignora (il PC mostra ancora la stessa battuta)
	}
	const int32 Vera = Visibili[Scelta];
	FName Prossimo;
	FName Evento;
	if (!ValdorsoDialoghi::Avanza(*Dati, Prima, Vera, Prossimo, Evento))
	{
		return;
	}
	// (11/11, v0.1.7) la storia di Lucia, "Quelli che non tornarono" (ValdorsoGrottaNord.h): le due pagine, e la consegna
	// (senza tutte le cose si va a "mancano"); prima dei segni, cosi' il segno della fine lo prende solo chi ha finito
	ValdorsoGrottaNord::EventoDialogo(Evento, Chi, Prossimo);
	// (11/11) i riti: la cerimonia dell'anno e un giorno (ValdorsoRiti.h; chi non puo' va a "presto")
	// (lo annota lui, quando la cerimonia c'e' davvero: qui sotto non si annota due volte)
	if (ValdorsoRiti::EventoDialogo(Evento, Chi, this, Prossimo))
	{
		Evento = NAME_None;
	}
	// (11/11, v0.1.8) le storie dei volti (ValdorsoStorie.h): il sasso di Tobia, l'incudine di Marta, la veglia
	if (ValdorsoStorie::EventoDialogo(Evento, Chi, this, Prossimo))
	{
		Evento = NAME_None;
	}
	// (14/11, v0.2.4) l'arresto: la multa (senza monete si va a "non_basta") o la cella (ValdorsoFiducia.h)
	if (ValdorsoFiducia::EventoDialogo(Evento, Chi, this, Prossimo))
	{
		Evento = NAME_None;
	}
	// (20/11, v0.2.7.1) le camere della locanda: Bruno da' la chiave per una notte (ValdorsoLocanda.h)
	if (ValdorsoLocanda::EventoDialogo(Evento, Chi, this, Prossimo))
	{
		Evento = NAME_None;
	}
	// (17/11, v0.2.7) la milizia: arruolarsi da Livia, le chiavi del grado, lasciare (ValdorsoMilizia.h)
	// (18/11) "Fammi leggere il contratto": il dialogo si chiude e al giocatore si apre la pergamena da firmare
	UValdorsoZainoComponent* Contratto = nullptr;
	if (ValdorsoMilizia::EventoDialogo(Evento, Chi, this, Prossimo))
	{
		if (Evento == FName(TEXT("MiliziaContratto")) && Prossimo.IsNone())
		{
			Contratto = UValdorsoZainoComponent::Di(Chi);
		}
		Evento = NAME_None;
	}
	const FString Detta = Nodo->Scelte[Vera].Testo;
	// (08/11) I segni: della risposta scelta (per esempio "MonetaOfferta": la moneta non si offre più) e del nodo dopo.
	ValdorsoDialoghi::MettiSegnoDi(Memoria, Dati->Id, Nodo->Scelte[Vera].Segna);
	PngSegnaNodo(*Dati, Prossimo, Memoria);
	// (Per esempio offrire una moneta a una guardia: "Corruzione". Un giorno potrà essere un crimine.)
	if (!Evento.IsNone())
	{
		ValdorsoRegistroEventi::Annota(Evento.ToString(), Colono, this, GetActorLocation(),
			FString::Printf(TEXT("a %s: %s"), *Dati->NomeBreve, *Detta.Left(120)));
	}
	ValdorsoRegistroEventi::Annota(TEXT("Dialogo"), Colono, this, GetActorLocation(),
		FString::Printf(TEXT("%s: %s -> %s (%s)"), *Dati->NomeBreve, *Prima.ToString(),
			Prossimo.IsNone() ? TEXT("fine") : *Prossimo.ToString(), *Detta.Left(80)));

	// (10/11) Lucia: "Ho un foglio della Gilda" apre il bancone (ValdorsoGilda.h), dopo che il dialogo si e' chiuso
	UValdorsoGildaComponent* Bancone = Evento == ValdorsoGilda::EventoBancone ? UValdorsoGildaComponent::Di(Chi) : nullptr;
	// (12/11, v0.2.0) Sibilla: "Fammi vedere le merci" apre il banco della carovana, a dialogo chiuso (ValdorsoCarovana.h)
	UValdorsoZainoComponent* Merci = Evento == FName(TEXT("Carovana")) ? UValdorsoZainoComponent::Di(Chi) : nullptr;
	// (10/11) Mastro Ettore: "A che punto sono?" (il server dice i punti veri)
	if (Evento == ValdorsoGilda::EventoGrado)
	{
		if (UValdorsoGildaComponent* Suo = UValdorsoGildaComponent::Di(Chi))
		{
			Suo->DimmiGrado();
		}
	}
	if (Prossimo.IsNone())
	{
		Chiudi(Chi, true, FString());
		if (Bancone)
		{
			Bancone->ApriBancone(this);
		}
		if (Merci)
		{
			Merci->ClientApriBottega(static_cast<uint8>(EValdorsoTipoBanco::Carovana));
		}
		if (Contratto)
		{
			Contratto->OffriContratto(ValdorsoMilizia::Contratto(), this);
		}
		return;
	}
	Aperta->Nodo = Prossimo;
	Aperta->Passo = ProssimoPasso++;
	GuardaVerso(Colono);
	Manda(Chi, ValdorsoDialoghi::Battuta(*Dati, Aperta->Nodo, Aperta->Passo, Memoria), IdFiducia);
	if (Bancone)
	{
		Bancone->ApriBancone(this);
	}
}

void AValdorsoPNG::Manda(APlayerController* Chi, const FValdorsoBattuta& Battuta, FName IdFiducia)
{
	if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
	{
		// (11/11, v0.1.6) i segnaposto: le voci vere della locanda ({voci}) e le righe dell'Atto I (ValdorsoVoci.h)
		FValdorsoBattuta Piena = Battuta;
		ValdorsoVoci::Riempi(GetWorld(), Piena.Righe);
		// (14/11, v0.2.4) quanto si fida di te, a parole, sotto il nome
		if (!IdFiducia.IsNone() && !Piena.bChiudi)
		{
			const EValdorsoFiducia Livello = ValdorsoFiducia::Livello(ValdorsoFiducia::DiColono(Chi, IdFiducia));
			Piena.Fiducia = ValdorsoFiducia::Parola(Livello);
			Piena.GradoFiducia = static_cast<uint8>(Livello);
		}
		Giocatore->ClientMostraDialogo(this, Piena);
	}
	// L'ultima battuta: sul server la conversazione è finita (il PC la legge e si congeda da solo).
	if (Battuta.bFine || Battuta.bChiudi)
	{
		Chiudi(Chi, false, Battuta.bFine ? TEXT("fine del dialogo") : TEXT("chiuso"));
	}
}

void AValdorsoPNG::Chiudi(APlayerController* Chi, bool bAvvisa, const FString& Come)
{
	const TWeakObjectPtr<APlayerController> Chiave(Chi);
	const FConversazione* Aperta = Conversazioni.Find(Chiave);
	if (!Aperta)
	{
		return;
	}
	const int32 Passo = Aperta->Passo;
	const bool bArresto = Aperta->Altro == FName(ValdorsoFiducia::DialogoArresto);
	Conversazioni.Remove(Chiave);
	// (14/11, v0.2.4) l'arresto chiuso: se la colpa c'e' ancora (e' scappato dal dialogo), lo riprendono fra poco
	if (bArresto && Chi)
	{
		if (AValdorsoPlayerState* Stato = Chi->GetPlayerState<AValdorsoPlayerState>())
		{
			Stato->ArrestoChiuso();
		}
	}
	if (APawn* Colono = Chi ? Chi->GetPawn() : nullptr; Colono && !Come.IsEmpty())
	{
		ValdorsoRegistroEventi::Annota(TEXT("Dialogo"), Colono, this, GetActorLocation(), Come);
	}
	if (bAvvisa)
	{
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
		{
			FValdorsoBattuta Fine;
			Fine.Passo = Passo;
			Fine.bChiudi = true;
			Giocatore->ClientMostraDialogo(this, Fine);
		}
	}
	ForseTornaAlPosto();
}

// --- Lo sguardo --------------------------------------------------------------------------------------------------

void AValdorsoPNG::GuardaVerso(const AActor* Chi)
{
	// (14/11) seduto o a letto non si gira (girerebbe la panca con se')
	if (!HasAuthority() || !Chi || Posa != PngInPiedi)
	{
		return;
	}
	const FVector Verso = Chi->GetActorLocation() - GetActorLocation();
	if (Verso.SizeSquared2D() < 1.f)
	{
		return;
	}
	Sguardo = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Verso.Y, Verso.X)));
	bSguardoValido = true;
	ForceNetUpdate();
	OnRep_Sguardo();
}

void AValdorsoPNG::ForseTornaAlPosto()
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo || Conversazioni.Num() > 0)
	{
		return;
	}
	Mondo->GetTimerManager().SetTimer(TimerRitorno, this, &AValdorsoPNG::TornaAlPosto, PngSecondiRitorno, false);
}

void AValdorsoPNG::TornaAlPosto()
{
	if (!HasAuthority() || Conversazioni.Num() > 0)
	{
		return;
	}
	Sguardo = SguardoIniziale;
	ForceNetUpdate();
	OnRep_Sguardo();
}

void AValdorsoPNG::OnRep_Sguardo()
{
	bSguardoValido = true;
	// Si gira nel Tick finché arriva (solo dopo BeginPlay: prima il Tick non parte comunque).
	SetActorTickEnabled(true);
}

void AValdorsoPNG::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bAncora = false;
	// (14/11) cammina verso la tappa (fermo se parla, e' a terra o scappa: lo decide il server)
	if (bCammina && !bInFuga)
	{
		bAncora = MuoviCammino(DeltaSeconds);
	}
	// (14/11) in fuga guarda dove corre; altrimenti si gira piano verso lo sguardo voluto (camminando: dove va)
	if (bInFuga)
	{
		// (alla fine della fuga, un giro in piu': lo sguardo torna al suo posto)
		bAncora = MuoviFuga(DeltaSeconds) || !bInFuga;
	}
	else if (bSguardoValido && (!bCammina || CamminoFermo >= 0.f))
	{
		const float Adesso = static_cast<float>(GetActorRotation().Yaw);
		const float Manca = FMath::FindDeltaAngleDegrees(Adesso, Sguardo);
		const float Passo = VelocitaGiro * DeltaSeconds;
		if (FMath::Abs(Manca) <= Passo)
		{
			SetActorRotation(FRotator(0.f, Sguardo, 0.f));
		}
		else
		{
			SetActorRotation(FRotator(0.f, Adesso + FMath::Sign(Manca) * Passo, 0.f));
			bAncora = true;
		}
	}
	// (14/11) il corpo che barcolla o cade e si rialza
	if (AnimaCorpo())
	{
		bAncora = true;
	}
	if (!bAncora)
	{
		SetActorTickEnabled(false);
	}
}

// --- (10/11) I saluti a chi passa ----------------------------------------------------------------------------------

void AValdorsoPNG::Saluta()
{
	UWorld* Mondo = GetWorld();
	const FValdorsoDialogo* Dati = DatiDialogo();
	if (!HasAuthority() || !Mondo || !Dati || bVia || bATerra || bInFuga || Dorme())
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	float Ora = 12.f;
	for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
	{
		Ora = It->Ora();
		break;
	}
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Giocatore = It->Get();
		APawn* Colono = Giocatore ? Giocatore->GetPawn() : nullptr;
		if (!Colono || Conversazioni.Contains(TWeakObjectPtr<APlayerController>(Giocatore))
			|| FVector::Dist(Colono->GetActorLocation(), GetActorLocation()) > ValdorsoDialoghi::RaggioSaluto)
		{
			continue;
		}
		const double* Ultimo = Salutati.Find(TWeakObjectPtr<APlayerController>(Giocatore));
		if (Ultimo && Adesso - *Ultimo < ValdorsoDialoghi::SecondiTraSaluti)
		{
			continue;
		}
		FString Frase = ValdorsoDialoghi::Saluto(Dati->Id, Ora, FMath::Rand());
		Salutati.Add(TWeakObjectPtr<APlayerController>(Giocatore), Adesso);
		if (Frase.IsEmpty())
		{
			continue;
		}
		// (14/11, v0.2.4) chi e' colpevole, i fatti di ieri, la fiducia (i freddi e gli amici)
		if (AValdorsoPlayerState::ColpevoleDi(Colono))
		{
			Frase = ValdorsoFiducia::SalutoAlColpevole(FMath::Rand());
		}
		else
		{
			UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Giocatore);
			const FString Ricordo = PngRicordo(Taccuino, Dati->Id);
			if (!Ricordo.IsEmpty())
			{
				Frase = Ricordo;
			}
			else if (Taccuino)
			{
				const EValdorsoFiducia Livello = ValdorsoFiducia::Livello(Taccuino->Fiducia(Dati->Id));
				const bool bCambia = Livello == EValdorsoFiducia::Diffidente
					|| ((Livello == EValdorsoFiducia::Amico || Livello == EValdorsoFiducia::DiFamiglia) && FMath::RandBool());
				const APlayerState* SuoStato = Giocatore->PlayerState.Get();
				const FString Suo = bCambia ? ValdorsoFiducia::SalutoDellaFiducia(Livello, SuoStato ? SuoStato->GetPlayerName() : FString(), FMath::Rand()) : FString();
				if (!Suo.IsEmpty())
				{
					Frase = Suo;
				}
			}
		}
		if (UValdorsoChatComponent* Chat = UValdorsoChatComponent::Di(Giocatore))
		{
			Chat->ClientSenti(Dati->NomeBreve, Frase, false);
		}
		// si gira un momento verso chi passa, poi torna al suo posto (non se sta parlando con un altro)
		if (Conversazioni.Num() == 0)
		{
			GuardaVerso(Colono);
			ForseTornaAlPosto();
		}
	}
	// i saluti di chi se n'e' andato dal gioco
	for (auto It = Salutati.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

#undef LOCTEXT_NAMESPACE

// ----------------------------------------------------------------------------------------------- (11/11) la giornata

namespace ValdorsoGiornata
{
	int32 TappaPer(const TArray<FValdorsoTappa>& Giornata, float Ora)
	{
		int32 Migliore = INDEX_NONE;
		float OraMigliore = -1.f;
		int32 Ultima = INDEX_NONE;
		float OraUltima = -1.f;
		for (int32 i = 0; i < Giornata.Num(); ++i)
		{
			const float Inizio = Giornata[i].Ora;
			if (Inizio <= Ora && Inizio > OraMigliore)
			{
				Migliore = i;
				OraMigliore = Inizio;
			}
			if (Inizio > OraUltima)
			{
				Ultima = i;
				OraUltima = Inizio;
			}
		}
		// prima della prima tappa del mattino vale l'ultima della sera prima
		return Migliore != INDEX_NONE ? Migliore : Ultima;
	}
}

void AValdorsoPNG::GiroGiornata()
{
	UWorld* Mondo = GetWorld();
	// (14/11) mentre cammina decide ControllaCammino
	if (!HasAuthority() || !Mondo || Conversazioni.Num() > 0 || bInFuga || bATerra || bCammina)
	{
		return;
	}
	float OraValle = 12.f;
	for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
	{
		OraValle = It->Ora();
		break;
	}
	const int32 Voluta = ValdorsoGiornata::TappaPer(Giornata, OraValle);
	// (12/11) chi viene solo certi giorni (la carovana di Sibilla, ValdorsoCarovana.h)
	const bool bNonOggi = OgniGiorni > 0 && !ValdorsoCarovana::Forzata()
		&& !ValdorsoCarovana::Giorno(FDateTime::UtcNow().ToUnixTimestamp(), OgniGiorni);
	const bool bVuoleVia = bNonOggi || ValdorsoVoci::Atto(this) < AttoMinimo || (Giornata.IsValidIndex(Voluta) && Giornata[Voluta].bAssente);
	if (Voluta == Tappa && bVuoleVia == bVia)
	{
		VuoleCambiareDa = -1.0;
		bGiornataPartita = true;
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	// (14/11, "PNG che camminano") la tappa nuova ha la sua strada: ci va a piedi, subito (anche se lo guardano). Non
	// la prima volta (all'avvio si mette subito al suo posto), e non chi non deve esserci (non e' il suo giorno, l'atto).
	// (la strada della fase del villaggio di adesso: le case, le palizzate e i muri cambiano)
	int32 FaseVillaggio = 1;
	for (TActorIterator<AValdorsoFasiVillaggio> It(Mondo); It; ++It)
	{
		FaseVillaggio = It->GetFase();
		break;
	}
	const FValdorsoTappa* Nuova = Giornata.IsValidIndex(Voluta) ? &Giornata[Voluta] : nullptr;
	const int32 IndiceStrada = FMath::Clamp(FaseVillaggio, 1, 255) - 1;
	if (bGiornataPartita && Voluta != Tappa && Nuova && Nuova->Strade.IsValidIndex(IndiceStrada)
		&& Nuova->Strade[IndiceStrada].Punti.Num() >= 2 && !bNonOggi && ValdorsoVoci::Atto(this) >= AttoMinimo)
	{
		StradaUsata = static_cast<uint8>(IndiceStrada);
		VuoleCambiareDa = -1.0;
		Tappa = Voluta;
		bVia = false;
		bViaAllArrivo = bVuoleVia;
		bAlzato = false;
		bCammina = true;
		CamminoInizio = OraServer();
		CamminoFermo = -1.f;
		UltimaPortaAperta = INDEX_NONE;
		ForceNetUpdate();
		OnRep_Tappa();
		OnRep_Cammino();
		GetWorldTimerManager().SetTimer(TimerCammino, this, &AValdorsoPNG::ControllaCammino, 0.25f, true);
		return;
	}
	if (VuoleCambiareDa < 0.0)
	{
		VuoleCambiareDa = Adesso;
	}
	// non sparisce e non compare sotto gli occhi di qualcuno (dopo dieci minuti, pazienza)
	const FVector Qui = GetActorLocation();
	const FVector La = Giornata.IsValidIndex(Voluta) ? Giornata[Voluta].Dove : Qui;
	bool bVisto = false;
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It && !bVisto; ++It)
	{
		const APawn* Corpo2 = It->Get() ? It->Get()->GetPawn() : nullptr;
		bVisto = Corpo2 && (FVector::Dist(Corpo2->GetActorLocation(), Qui) < 1500.f || FVector::Dist(Corpo2->GetActorLocation(), La) < 1500.f);
	}
	// (la prima volta, appena comincia il gioco, si mette subito al suo posto)
	if (bVisto && bGiornataPartita && Adesso - VuoleCambiareDa < 600.0)
	{
		return;
	}
	// (12/11) la carovana arriva: le scorte e l'avviso (non la prima volta che si mette a posto, all'avvio)
	const bool bArriva = bGiornataPartita && bVia && !bVuoleVia && OgniGiorni > 0;
	VuoleCambiareDa = -1.0;
	bGiornataPartita = true;
	const bool bTappaNuova = Tappa != Voluta;
	Tappa = Voluta;
	bVia = bVuoleVia;
	// (14/11) una tappa nuova: di nuovo come la vuole (seduto, a letto), anche se prima l'avevano fatto alzare
	if (bTappaNuova)
	{
		bAlzato = false;
	}
	if (bArriva)
	{
		ValdorsoCarovana::Arriva(this);
	}
	ForceNetUpdate();
	OnRep_Tappa();
	// (14/11) la porta della tappa: la bottega che apre la mattina, la locanda che chiude la sera
	if (bTappaNuova && Giornata.IsValidIndex(Tappa))
	{
		UsaPorta(Giornata[Tappa]);
	}
}

void AValdorsoPNG::VaiAllaFase(int32 Fase)
{
	const int32 Indice = FMath::Clamp(Fase, 1, 255) - 1;
	bool bQuestaCambia = false;
	for (int32 i = 0; i < Giornata.Num(); ++i)
	{
		FValdorsoTappa& Una = Giornata[i];
		if (!Una.DovePerFase.IsValidIndex(Indice) || Una.DovePerFase[Indice].IsZero())
		{
			continue;
		}
		const float Gradi = Una.GradiPerFase.IsValidIndex(Indice) ? Una.GradiPerFase[Indice] : Una.Gradi;
		if (!Una.Dove.Equals(Una.DovePerFase[Indice], 1.0) || !FMath::IsNearlyEqual(Una.Gradi, Gradi, 0.5f))
		{
			bQuestaCambia = bQuestaCambia || i == Tappa;
			Una.Dove = Una.DovePerFase[Indice];
			Una.Gradi = Gradi;
		}
	}
	if (Giornata.Num() == 0)
	{
		// chi sta sempre nello stesso posto (Padre Anselmo davanti all'altare).
		// (secondo controllo) solo quando la fase cambia davvero: Applica gira a ogni pezzo di mondo che arriva, e lo
		// rimetterebbe dritto mentre parla o mentre scappa
		if (Indice != FaseMessa && PostoPerFase.IsValidIndex(Indice) && !PostoPerFase[Indice].IsZero())
		{
			FaseMessa = Indice;
			if (bInFuga)
			{
				// sta scappando: quando torna, torna al posto della fase nuova
				PostoFuga = PostoPerFase[Indice];
				return;
			}
			const float Gradi = GradiPerFase.IsValidIndex(Indice) ? GradiPerFase[Indice] : static_cast<float>(GetActorRotation().Yaw);
			SetActorLocationAndRotation(PostoPerFase[Indice], FRotator(0.f, Gradi, 0.f));
			SguardoIniziale = Gradi;
			if (HasAuthority())
			{
				Sguardo = Gradi;
			}
		}
		return;
	}
	// e' proprio in quella tappa, fermo: si sposta subito al posto della fase nuova (mentre cammina ci arriva da se')
	// (secondo controllo) non mentre parla, e' a terra o scappa: chi scappa torna al posto nuovo, gli altri ci vanno
	// alla prossima tappa
	if (bQuestaCambia && bInFuga && Giornata.IsValidIndex(Tappa))
	{
		PostoFuga = Giornata[Tappa].Dove;
	}
	else if (bQuestaCambia && HasActorBegunPlay() && !bVia && !bCammina && !bATerra && Conversazioni.Num() == 0)
	{
		OnRep_Tappa();
	}
}

void AValdorsoPNG::OnRep_Tappa()
{
	SetActorHiddenInGame(bVia);
	SetActorEnableCollision(!bVia);
	// il corpo MetaHuman e' un attore attaccato: si nasconde con lui
	TArray<AActor*> Attaccati;
	GetAttachedActors(Attaccati, true, true);
	for (AActor* Attaccato : Attaccati)
	{
		if (Attaccato)
		{
			Attaccato->SetActorHiddenInGame(bVia);
		}
	}
	if (bVia || !Giornata.IsValidIndex(Tappa))
	{
		// (14/11) a casa: lascia la panca o il letto
		MettiPosa();
		return;
	}
	// (14/11) cammina verso la tappa: in piedi, il posto lo da' il cammino (MuoviCammino)
	if (bCammina)
	{
		LiberaMobile();
		Posa = PngInPiedi;
		if (Corpo)
		{
			Corpo->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -PngMezzaAltezza), FRotator(0.f, -90.f, 0.f));
		}
		AnimCammino = 0;
		SetActorTickEnabled(true);
		return;
	}
	const FValdorsoTappa& Ora = Giornata[Tappa];
	if (!Ora.Dove.IsZero())
	{
		SetActorLocationAndRotation(Ora.Dove, FRotator(0.f, Ora.Gradi, 0.f));
		SguardoIniziale = Ora.Gradi;
		if (HasAuthority())
		{
			Sguardo = Ora.Gradi;
		}
	}
	// (14/11) seduto sulla panca, a letto, o in piedi (e la sua movenza)
	MettiPosa();
}

void AValdorsoPNG::RimettiMovenza()
{
	bCorre = false;
	// (14/11) mentre cammina l'animazione la sceglie il cammino (MuoviCammino), al prossimo passo
	if (bCammina)
	{
		AnimCammino = 0;
		return;
	}
	if (!Corpo || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	Corpo->SetPlayRate(1.f);
	UAnimSequence* Movenza = nullptr;
	// (14/11) seduto o a letto: le animazioni dei coloni
	if (Posa == PngSeduto)
	{
		Movenza = PngAnimPosa(PngAnimSeduto);
	}
	else if (Posa == PngDorme)
	{
		Movenza = PngAnimPosa(PngAnimDorme);
	}
	else if (Giornata.IsValidIndex(Tappa) && !Giornata[Tappa].Movenza.IsNone()
		&& Giornata[Tappa].Movenza != FName(PngMovenzaPanca) && Giornata[Tappa].Movenza != FName(PngMovenzaLetto))
	{
		const FString Nome = Giornata[Tappa].Movenza.ToString();
		Movenza = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Mixamo/Gesti/%s.%s"), *Nome, *Nome), nullptr,
			LOAD_NoWarn | LOAD_Quiet);
	}
	if (Movenza || Riposo)
	{
		Corpo->PlayAnimation(Movenza ? Movenza : Riposo.Get(), true);
	}
	// (14/11) seduto o a letto la radice dell'animazione resta ferma (come per i coloni, che le suonano come montaggi)
	if (UAnimInstance* Anima = Corpo->GetAnimInstance())
	{
		Anima->SetRootMotionMode(Posa != PngInPiedi ? ERootMotionMode::IgnoreRootMotion : ERootMotionMode::RootMotionFromMontagesOnly);
	}
}

// ------------------------------------------------------------------------- (14/11, "PNG che vivono") panche e letti

void AValdorsoPNG::OnRep_Alzato()
{
	if (HasActorBegunPlay())
	{
		MettiPosa();
	}
}

void AValdorsoPNG::LiberaMobile()
{
	if (HasAuthority())
	{
		if (AValdorsoPanca* Panca = Cast<AValdorsoPanca>(Mobile.Get()))
		{
			if (Panca->Riservati.IsValidIndex(PostoMobile) && Panca->Riservati[PostoMobile].Get() == this)
			{
				Panca->Riservati[PostoMobile].Reset();
			}
		}
		else if (AValdorsoLetto* Letto = Cast<AValdorsoLetto>(Mobile.Get()))
		{
			if (Letto->Riservati.IsValidIndex(PostoMobile) && Letto->Riservati[PostoMobile].Get() == this)
			{
				Letto->Riservati[PostoMobile].Reset();
			}
		}
	}
	Mobile.Reset();
	PostoMobile = INDEX_NONE;
}

void AValdorsoPNG::MettiPosa()
{
	LiberaMobile();
	Posa = PngInPiedi;
	// il corpo al suo posto di sempre (a letto si sposta sotto la capsula)
	if (Corpo)
	{
		Corpo->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -PngMezzaAltezza), FRotator(0.f, -90.f, 0.f));
	}
	// (14/11) mentre cammina e' in piedi: il posto e l'animazione li da' il cammino
	if (bCammina)
	{
		AnimCammino = 0;
		SetActorTickEnabled(true);
		return;
	}
	UWorld* Mondo = GetWorld();
	const FValdorsoTappa* Ora = (!bVia && Giornata.IsValidIndex(Tappa)) ? &Giornata[Tappa] : nullptr;
	const bool bPanca = Ora && Ora->Movenza == FName(PngMovenzaPanca);
	const bool bLetto = Ora && Ora->Movenza == FName(PngMovenzaLetto);
	if (!Mondo || (!bPanca && !bLetto))
	{
		RimettiMovenza();
		return;
	}
	const FVector Vicino = Ora->Dove.IsZero() ? GetActorLocation() : Ora->Dove;
	const float Mezza = PngMezzaAltezza * static_cast<float>(GetActorScale3D().Z);
	float Gradi = static_cast<float>(GetActorRotation().Yaw);
	bool bTrovato = false;
	if (bPanca)
	{
		// il posto (di tutte le panche) piu' vicino a Dove
		AValdorsoPanca* Scelta = nullptr;
		int32 Quale = INDEX_NONE;
		float Meglio = PngRaggioMobile;
		FVector Sedile = FVector::ZeroVector;
		for (TActorIterator<AValdorsoPanca> It(Mondo); It; ++It)
		{
			// (19/11) non le panche di un'altra fase del villaggio (quelle della cappella, nascoste nella chiesa)
			if (It->IsHidden())
			{
				continue;
			}
			for (int32 i = 0; i < It->Posti.Num(); ++i)
			{
				const FVector Punto = It->GetActorTransform().TransformPosition(FVector(It->AvantiPosto, It->Posti[i], 0.f));
				const float Distanza = static_cast<float>(FVector::Dist2D(Punto, Vicino));
				if (Distanza < Meglio && FMath::Abs(Punto.Z - (Vicino.Z - Mezza)) < 200.f)
				{
					Meglio = Distanza;
					Scelta = *It;
					Quale = i;
					Sedile = Punto;
				}
			}
		}
		if (Scelta)
		{
			bTrovato = true;
			Gradi = static_cast<float>(Scelta->GetActorRotation().Yaw);
			Mobile = Scelta;
			PostoMobile = Quale;
			if (bAlzato)
			{
				SetActorLocationAndRotation(Sedile + Scelta->GetActorForwardVector() * PngDavantiPanca + FVector(0.f, 0.f, Mezza), FRotator(0.f, Gradi, 0.f));
			}
			else
			{
				SetActorLocationAndRotation(Sedile + FVector(0.f, 0.f, Mezza), FRotator(0.f, Gradi, 0.f));
				Posa = PngSeduto;
				if (HasAuthority())
				{
					if (Scelta->Riservati.Num() < Scelta->Posti.Num())
					{
						Scelta->Riservati.SetNum(Scelta->Posti.Num());
					}
					Scelta->Riservati[Quale] = this;
				}
			}
		}
	}
	else
	{
		AValdorsoLetto* Scelta = nullptr;
		int32 Quale = INDEX_NONE;
		float Meglio = PngRaggioMobile;
		for (TActorIterator<AValdorsoLetto> It(Mondo); It; ++It)
		{
			for (int32 i = 0; i < It->Posti.Num(); ++i)
			{
				const FVector Punto = It->GetActorTransform().TransformPosition(FVector(It->Posti[i], 0.f, 0.f));
				const float Distanza = static_cast<float>(FVector::Dist2D(Punto, Vicino));
				if (Distanza < Meglio && FMath::Abs(Punto.Z - (Vicino.Z - Mezza)) < 200.f)
				{
					Meglio = Distanza;
					Scelta = *It;
					Quale = i;
				}
			}
		}
		if (Scelta)
		{
			bTrovato = true;
			Mobile = Scelta;
			PostoMobile = Quale;
			// i conti del letto sono fatti per una capsula di 90 cm (senza colono): qui la nostra
			const FVector Alza(0.f, 0.f, Mezza - 90.f);
			if (bAlzato)
			{
				const FTransform Uscita = Scelta->TrasformaUscita(Quale, nullptr);
				Gradi = static_cast<float>(Uscita.Rotator().Yaw);
				SetActorLocationAndRotation(Uscita.GetLocation() + Alza, FRotator(0.f, Gradi, 0.f));
			}
			else
			{
				FTransform Attore;
				FTransform MeshRelativa;
				Scelta->TrasformaSdraiato(Quale, nullptr, Attore, MeshRelativa);
				const FTransform MeshNelMondo = MeshRelativa * Attore;
				Gradi = static_cast<float>(Attore.Rotator().Yaw);
				SetActorLocationAndRotation(Attore.GetLocation() + Alza, FRotator(0.f, Gradi, 0.f));
				if (Corpo)
				{
					Corpo->SetWorldLocationAndRotation(MeshNelMondo.GetLocation(), MeshNelMondo.GetRotation());
				}
				Posa = PngDorme;
				if (HasAuthority())
				{
					if (Scelta->Riservati.Num() < Scelta->Posti.Num())
					{
						Scelta->Riservati.SetNum(Scelta->Posti.Num());
					}
					Scelta->Riservati[Quale] = this;
				}
			}
		}
	}
	if (bTrovato)
	{
		SguardoIniziale = Gradi;
		if (HasAuthority())
		{
			Sguardo = Gradi;
		}
	}
	else
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] PNG %s: nessuna %s entro %.0f cm da (%.0f, %.0f, %.0f): resta in piedi."),
			*GetName(), bPanca ? TEXT("panca") : TEXT("letto"), PngRaggioMobile, Vicino.X, Vicino.Y, Vicino.Z);
	}
	RimettiMovenza();
}

void AValdorsoPNG::Alzati()
{
	if (!HasAuthority() || Posa == PngInPiedi || bAlzato)
	{
		return;
	}
	bAlzato = true;
	ForceNetUpdate();
	MettiPosa();
}

void AValdorsoPNG::UsaPorta(const FValdorsoTappa& Arrivo)
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo || Arrivo.Porta.IsZero())
	{
		return;
	}
	AValdorsoPorta* Scelta = nullptr;
	float Meglio = PngRaggioPorta;
	for (TActorIterator<AValdorsoPorta> It(Mondo); It; ++It)
	{
		// (le imposte delle finestre no)
		if (It->IsA<AValdorsoImposta>())
		{
			continue;
		}
		const float Distanza = static_cast<float>(FVector::Dist(It->GetActorLocation(), Arrivo.Porta));
		if (Distanza < Meglio)
		{
			Meglio = Distanza;
			Scelta = *It;
		}
	}
	if (Scelta)
	{
		Scelta->MettiAperta(Arrivo.bApriPorta, GetActorLocation());
	}
	else
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] PNG %s: nessuna porta entro %.0f cm da (%.0f, %.0f, %.0f)."),
			*GetName(), PngRaggioPorta, Arrivo.Porta.X, Arrivo.Porta.Y, Arrivo.Porta.Z);
	}
}

// ---------------------------------------------------------------------- (14/11, "PNG che camminano") il cammino

double AValdorsoPNG::OraServer() const
{
	const UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return 0.0;
	}
	const AGameStateBase* Gioco = Mondo->GetGameState();
	return Gioco ? Gioco->GetServerWorldTimeSeconds() : Mondo->GetTimeSeconds();
}

float AValdorsoPNG::TempoDelCammino() const
{
	return CamminoFermo >= 0.f ? CamminoFermo : static_cast<float>(FMath::Max(0.0, OraServer() - CamminoInizio));
}

const FValdorsoStrada* AValdorsoPNG::StradaDiAdesso() const
{
	if (!Giornata.IsValidIndex(Tappa) || !Giornata[Tappa].Strade.IsValidIndex(StradaUsata))
	{
		return nullptr;
	}
	const FValdorsoStrada& Strada = Giornata[Tappa].Strade[StradaUsata];
	return Strada.Punti.Num() > 0 ? &Strada : nullptr;
}

bool AValdorsoPNG::PuntoDelCammino(float Tempo, FVector& Dove, float& Gradi, int32& PortaSulPercorso) const
{
	PortaSulPercorso = INDEX_NONE;
	const FValdorsoStrada* Strada = StradaDiAdesso();
	if (!Strada)
	{
		Dove = GetActorLocation();
		Gradi = static_cast<float>(GetActorRotation().Yaw);
		return false;
	}
	const TArray<FVector>& Punti = Strada->Punti;
	float Resto = FMath::Max(0.f, Tempo);
	Gradi = static_cast<float>(GetActorRotation().Yaw);
	for (int32 i = 0; i < Punti.Num(); ++i)
	{
		if (i > 0)
		{
			const FVector Tratto = Punti[i] - Punti[i - 1];
			const float Lungo = static_cast<float>(Tratto.Size2D());
			if (Lungo > 1.f)
			{
				Gradi = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Tratto.Y, Tratto.X)));
			}
			const float Durata = Lungo / PngVelocitaPasso;
			if (Resto < Durata)
			{
				Dove = FMath::Lerp(Punti[i - 1], Punti[i], Durata > 0.f ? Resto / Durata : 1.f);
				return true;
			}
			Resto -= Durata;
		}
		if (Strada->Porte.Contains(i) && i + 1 < Punti.Num())
		{
			if (Resto < PngSostaPorta)
			{
				// sulla soglia, girato verso dove passa
				const FVector Avanti = Punti[i + 1] - Punti[i];
				if (Avanti.SizeSquared2D() > 1.f)
				{
					Gradi = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Avanti.Y, Avanti.X)));
				}
				Dove = Punti[i];
				PortaSulPercorso = i;
				return true;
			}
			Resto -= PngSostaPorta;
		}
	}
	Dove = Punti.Last();
	return false;
}

bool AValdorsoPNG::MuoviCammino(float DeltaSeconds)
{
	if (!bCammina)
	{
		return false;
	}
	FVector Dove;
	float Gradi = 0.f;
	int32 Porta = INDEX_NONE;
	PuntoDelCammino(TempoDelCammino(), Dove, Gradi, Porta);
	// i piedi sulla terra (o sul pavimento): una linea dall'alto, vicino alla quota della strada
	if (UWorld* Mondo = GetWorld())
	{
		const float Mezza = PngMezzaAltezza * static_cast<float>(GetActorScale3D().Z);
		const double Riferimento = bCamminoZ ? FMath::Max(CamminoZ, Dove.Z) : Dove.Z;
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoPngCammino), false, this);
		FCollisionObjectQueryParams Cosa;
		Cosa.AddObjectTypesToQuery(ECC_WorldStatic);
		FHitResult Terra;
		if (Mondo->LineTraceSingleByObjectType(Terra, FVector(Dove.X, Dove.Y, Riferimento + 100.0),
			FVector(Dove.X, Dove.Y, Riferimento - 350.0), Cosa, Parametri))
		{
			Dove.Z = Terra.ImpactPoint.Z + Mezza;
		}
		CamminoZ = Dove.Z;
		bCamminoZ = true;
	}
	SetActorLocation(Dove);
	const bool bFermo = CamminoFermo >= 0.f;
	if (!bFermo)
	{
		// si gira piano verso dove va (non di colpo agli angoli)
		const float Adesso = static_cast<float>(GetActorRotation().Yaw);
		const float Manca = FMath::FindDeltaAngleDegrees(Adesso, Gradi);
		const float Passo = 240.f * DeltaSeconds;
		SetActorRotation(FRotator(0.f, FMath::Abs(Manca) <= Passo ? Gradi : Adesso + FMath::Sign(Manca) * Passo, 0.f));
	}
	// l'animazione: cammina, apre la porta, o sta fermo (parla)
	const uint8 Voluta = bFermo ? 3 : (Porta != INDEX_NONE ? 2 : 1);
	if (Voluta != AnimCammino && Corpo && GetNetMode() != NM_DedicatedServer)
	{
		AnimCammino = Voluta;
		UAnimSequence* Quale = nullptr;
		float Ritmo = 1.f;
		if (Voluta == 1)
		{
			if (!Passeggio)
			{
				Passeggio = LoadObject<UAnimSequence>(nullptr, PngAnimPasseggio, nullptr, LOAD_NoWarn | LOAD_Quiet);
			}
			Quale = Passeggio;
			Ritmo = FMath::Clamp(CVarPngRitmoPasso.GetValueOnGameThread(), 0.3f, 2.f);
		}
		else if (Voluta == 2)
		{
			if (!ApreLaPorta)
			{
				ApreLaPorta = LoadObject<UAnimSequence>(nullptr, PngAnimApre, nullptr, LOAD_NoWarn | LOAD_Quiet);
			}
			Quale = ApreLaPorta;
			if (Quale)
			{
				// tutta l'animazione nella sosta sulla soglia
				Ritmo = FMath::Clamp(Quale->GetPlayLength() / PngSostaPorta, 0.5f, 3.f);
			}
		}
		if (!Quale)
		{
			Quale = Riposo;
		}
		if (Quale)
		{
			Corpo->PlayAnimation(Quale, Voluta != 2);
			Corpo->SetPlayRate(Ritmo);
			if (UAnimInstance* Anima = Corpo->GetAnimInstance())
			{
				Anima->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
			}
		}
	}
	return true;
}

void AValdorsoPNG::OnRep_Cammino()
{
	if (!HasActorBegunPlay())
	{
		return;
	}
	SetActorTickEnabled(true);
	if (bCammina)
	{
		// fermo o ripartito: l'animazione giusta al prossimo passo
		AnimCammino = 0;
		return;
	}
	// arrivato: al suo posto (o a casa), come vuole la tappa
	if (!HasAuthority())
	{
		bCamminoZ = false;
		AnimCammino = 0;
		if (Corpo)
		{
			Corpo->SetPlayRate(1.f);
		}
		OnRep_Tappa();
	}
}

void AValdorsoPNG::ControllaCammino()
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo)
	{
		return;
	}
	const double Adesso = OraServer();
	if (bCammina)
	{
		// si ferma se parla, e' a terra o scappa; riparte quando ha finito
		const bool bFermati = bInFuga || bATerra || Conversazioni.Num() > 0;
		if (bFermati && CamminoFermo < 0.f)
		{
			CamminoFermo = TempoDelCammino();
			ForceNetUpdate();
			OnRep_Cammino();
		}
		else if (!bFermati && CamminoFermo >= 0.f)
		{
			CamminoInizio = Adesso - CamminoFermo;
			CamminoFermo = -1.f;
			ForceNetUpdate();
			OnRep_Cammino();
		}
		FVector Dove;
		float Gradi = 0.f;
		int32 Soglia = INDEX_NONE;
		const bool bAncora = PuntoDelCammino(TempoDelCammino(), Dove, Gradi, Soglia);
		// sulla soglia: apre la porta (e fra poco la richiude dietro di se')
		const FValdorsoStrada* Strada = StradaDiAdesso();
		if (Soglia != INDEX_NONE && Soglia != UltimaPortaAperta && Strada && Strada->Punti.IsValidIndex(Soglia))
		{
			UltimaPortaAperta = Soglia;
			const FVector Punto = Strada->Punti[Soglia];
			AValdorsoPorta* Scelta = nullptr;
			float Meglio = PngRaggioPorta;
			for (TActorIterator<AValdorsoPorta> It(Mondo); It; ++It)
			{
				if (It->IsA<AValdorsoImposta>())
				{
					continue;
				}
				const float Distanza = static_cast<float>(FVector::Dist2D(It->GetActorLocation(), Punto));
				if (Distanza < Meglio && FMath::Abs(It->GetActorLocation().Z - Punto.Z) < 250.f)
				{
					Meglio = Distanza;
					Scelta = *It;
				}
			}
			if (Scelta && !Scelta->Aperta())
			{
				Scelta->MettiAperta(true, Dove);
				FPortaDaChiudere Nuova;
				Nuova.Porta = Scelta;
				Nuova.Alle = Adesso + PngSostaPorta + PngChiudiDopo;
				PorteDaChiudere.Add(Nuova);
			}
		}
		if (!bAncora && CamminoFermo < 0.f)
		{
			FinisciCammino();
		}
	}
	// le porte aperte passando: si richiudono (non se c'e' qualcuno sulla soglia)
	for (int32 i = PorteDaChiudere.Num() - 1; i >= 0; --i)
	{
		AValdorsoPorta* Porta = PorteDaChiudere[i].Porta.Get();
		if (!Porta)
		{
			PorteDaChiudere.RemoveAt(i);
			continue;
		}
		if (Adesso < PorteDaChiudere[i].Alle)
		{
			continue;
		}
		bool bQualcuno = FVector::Dist2D(GetActorLocation(), Porta->GetActorLocation()) < 180.0;
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It && !bQualcuno; ++It)
		{
			const APawn* Colono = It->Get() ? It->Get()->GetPawn() : nullptr;
			bQualcuno = Colono && FVector::Dist2D(Colono->GetActorLocation(), Porta->GetActorLocation()) < 180.0;
		}
		if (bQualcuno)
		{
			PorteDaChiudere[i].Alle = Adesso + 2.0;
			continue;
		}
		Porta->MettiAperta(false, GetActorLocation());
		PorteDaChiudere.RemoveAt(i);
	}
	if (!bCammina && PorteDaChiudere.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(TimerCammino);
	}
}

void AValdorsoPNG::FinisciCammino()
{
	if (!HasAuthority())
	{
		return;
	}
	bCammina = false;
	CamminoFermo = -1.f;
	bCamminoZ = false;
	AnimCammino = 0;
	if (Corpo)
	{
		Corpo->SetPlayRate(1.f);
	}
	bVia = bViaAllArrivo;
	ForceNetUpdate();
	OnRep_Tappa();
	// la porta della tappa (la bottega che apre, la locanda che chiude): non la richiude il cammino
	if (Giornata.IsValidIndex(Tappa))
	{
		const FValdorsoTappa& Arrivo = Giornata[Tappa];
		if (!Arrivo.Porta.IsZero())
		{
			PorteDaChiudere.RemoveAll([&Arrivo](const FPortaDaChiudere& Una)
			{
				return Una.Porta.IsValid() && FVector::Dist(Una.Porta->GetActorLocation(), Arrivo.Porta) < PngRaggioPorta;
			});
		}
		UsaPorta(Arrivo);
	}
}

// ------------------------------------------------------------------------- (14/11, v0.2.4) colpito, a terra, in fuga

bool AValdorsoPNG::Colpito(AActor* Da, float Danno)
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo || bVia)
	{
		return false;
	}
	// (14/11) seduto o a letto: prima salta in piedi
	Alzati();
	const double Adesso = Mondo->GetTimeSeconds();
	APawn* Colono = Cast<APawn>(Da);
	APlayerController* Giocatore = Colono ? Cast<APlayerController>(Colono->GetController()) : nullptr;
	// chi gli stava parlando e lo colpisce: il dialogo si chiude
	if (Giocatore && Conversazioni.Contains(TWeakObjectPtr<APlayerController>(Giocatore)))
	{
		Chiudi(Giocatore, true, TEXT("lo ha colpito"));
	}
	if (Adesso - DannoDa > ValdorsoFiducia::SecondiAccumulo)
	{
		DannoRecente = 0.f;
	}
	DannoRecente += FMath::Max(0.f, Danno);
	DannoDa = Adesso;
	if (!bATerra)
	{
		if (ValdorsoFiducia::Cade(DannoRecente, Danno))
		{
			// (se stava scappando, cade li' dov'e')
			if (bInFuga)
			{
				DoveFuga = GetActorLocation();
			}
			bATerra = true;
			++Cadute;
			DannoRecente = 0.f;
			GetWorldTimerManager().SetTimer(TimerATerra, this, &AValdorsoPNG::Rialza, ValdorsoFiducia::SecondiATerra + 1.25f, false);
			OnRep_Cadute();
		}
		else
		{
			++Barcollate;
			OnRep_Barcollate();
		}
		ForceNetUpdate();
	}
	if (Colono && Giocatore)
	{
		ValdorsoFiducia::Crimine(Colono, this);
	}
	else if (Da)
	{
		GuardaVerso(Da);
		ForseTornaAlPosto();
	}
	return true;
}

void AValdorsoPNG::Rialza()
{
	bATerra = false;
	ForceNetUpdate();
}

void AValdorsoPNG::Spaventa(APawn* Colono, bool bVittima)
{
	UWorld* Mondo = GetWorld();
	// (14/11) chi dorme non vede niente (se lo colpiscono si sveglia: Colpito)
	if (!HasAuthority() || !Mondo || bVia || (Dorme() && !bVittima))
	{
		return;
	}
	Alzati();
	const double Adesso = Mondo->GetTimeSeconds();
	if (Adesso - UltimoGrido >= 6.0)
	{
		UltimoGrido = Adesso;
		Grida(ValdorsoFiducia::Grido(bVittima, Guardia(), FMath::Rand()));
	}
	if (!bVittima && !Guardia() && !bATerra && Conversazioni.Num() == 0 && Colono
		&& FVector::Dist(Colono->GetActorLocation(), GetActorLocation()) <= ValdorsoFiducia::RaggioFuga)
	{
		Fuggi(Colono);
		return;
	}
	if (!bInFuga && Colono)
	{
		GuardaVerso(Colono);
		ForseTornaAlPosto();
	}
}

void AValdorsoPNG::Grida(const FString& Frase)
{
	UWorld* Mondo = GetWorld();
	if (!Mondo || Frase.IsEmpty())
	{
		return;
	}
	const FString Nome = NomeBreve();
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Giocatore = It->Get();
		const APawn* Suo = Giocatore ? Giocatore->GetPawn() : nullptr;
		if (!Suo || FVector::Dist(Suo->GetActorLocation(), GetActorLocation()) > ValdorsoFiducia::RaggioGrida)
		{
			continue;
		}
		if (UValdorsoChatComponent* Chat = UValdorsoChatComponent::Di(Giocatore))
		{
			Chat->ClientSenti(Nome, Frase, false);
		}
	}
}

void AValdorsoPNG::Fuggi(const AActor* Da)
{
	UWorld* Mondo = GetWorld();
	if (!HasAuthority() || !Mondo || !Da || bInFuga)
	{
		return;
	}
	Alzati();
	const FVector Qui = GetActorLocation();
	FVector Lontano = (Qui - Da->GetActorLocation()).GetSafeNormal2D();
	if (Lontano.IsNearlyZero())
	{
		Lontano = -GetActorForwardVector().GetSafeNormal2D();
	}
	FVector Meta = Qui + Lontano * ValdorsoFiducia::DistanzaFuga;
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoPngFuga), false, this);
	Parametri.AddIgnoredActor(Da);
	// un muro (o un altro) in mezzo: si ferma prima
	FHitResult Muro;
	const FVector Su(0.f, 0.f, 25.f);
	if (Mondo->SweepSingleByChannel(Muro, Qui + Su, Meta + Su, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(PngRaggio, PngMezzaAltezza * 0.75f), Parametri))
	{
		Meta = Muro.Location - Su;
	}
	// i piedi sul terreno
	FHitResult Terra;
	if (Mondo->LineTraceSingleByChannel(Terra, Meta + FVector(0.f, 0.f, 200.f), Meta - FVector(0.f, 0.f, 500.f), ECC_Visibility, Parametri))
	{
		Meta.Z = Terra.ImpactPoint.Z + PngMezzaAltezza;
	}
	if (FVector::Dist2D(Meta, Qui) < 100.f)
	{
		GuardaVerso(Da);
		ForseTornaAlPosto();
		return;
	}
	PostoFuga = Qui;
	bRitornoFuga = false;
	DoveFuga = Meta;
	bInFuga = true;
	ForceNetUpdate();
	OnRep_Fuga();
	GetWorldTimerManager().SetTimer(TimerFuga, this, &AValdorsoPNG::RitornaDallaFuga, ValdorsoFiducia::SecondiFuga, false);
}

void AValdorsoPNG::RitornaDallaFuga()
{
	if (!HasAuthority() || !bInFuga)
	{
		return;
	}
	bRitornoFuga = true;
	DoveFuga = PostoFuga;
	ForceNetUpdate();
	OnRep_Fuga();
}

bool AValdorsoPNG::MuoviFuga(float DeltaSeconds)
{
	const FVector Qui = GetActorLocation();
	const FVector Verso = FVector(DoveFuga) - Qui;
	const float Distanza = static_cast<float>(Verso.Size());
	if (Distanza < 5.f)
	{
		SetActorLocation(DoveFuga);
		if (bCorre)
		{
			RimettiMovenza();
		}
		// al suo posto: la fuga e' finita (lo decide il server)
		if (HasAuthority() && bRitornoFuga)
		{
			bRitornoFuga = false;
			bInFuga = false;
			Sguardo = SguardoIniziale;
			ForceNetUpdate();
			OnRep_Fuga();
			OnRep_Sguardo();
		}
		return false;
	}
	const float Passo = FMath::Min(ValdorsoFiducia::VelocitaFuga * DeltaSeconds, Distanza);
	SetActorLocation(Qui + Verso / Distanza * Passo);
	if (Verso.SizeSquared2D() > 1.f)
	{
		SetActorRotation(FRotator(0.f, static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Verso.Y, Verso.X))), 0.f));
	}
	return true;
}

void AValdorsoPNG::OnRep_Fuga()
{
	if (!HasActorBegunPlay())
	{
		return;
	}
	SetActorTickEnabled(true);
	if (!bInFuga)
	{
		// arrivato al suo posto (sul PC: dove l'ha messo il server, se la fuga l'ha vista; chi arriva dopo no)
		if (bVistaFuga || HasAuthority())
		{
			SetActorLocation(DoveFuga);
			RimettiMovenza();
		}
		bVistaFuga = false;
		bSguardoValido = true;
		return;
	}
	bVistaFuga = true;
	if (!Corpo || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!Corsa)
	{
		for (const TCHAR* Percorso : PngCorse)
		{
			Corsa = LoadObject<UAnimSequence>(nullptr, Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (Corsa)
			{
				break;
			}
		}
	}
	if (Corsa)
	{
		Corpo->PlayAnimation(Corsa, true);
		bCorre = true;
	}
}

void AValdorsoPNG::OnRep_Barcollate()
{
	if (!HasActorBegunPlay() || !GetWorld())
	{
		return;
	}
	if (CadutaDa < 0.0)
	{
		BarcolloDa = GetWorld()->GetTimeSeconds();
	}
	SetActorTickEnabled(true);
}

void AValdorsoPNG::OnRep_Cadute()
{
	if (!HasActorBegunPlay() || !GetWorld())
	{
		return;
	}
	CadutaDa = GetWorld()->GetTimeSeconds();
	BarcolloDa = -1.0;
	SetActorTickEnabled(true);
}

bool AValdorsoPNG::AnimaCorpo()
{
	UWorld* Mondo = GetWorld();
	if (!Corpo || !Mondo || GetNetMode() == NM_DedicatedServer)
	{
		CadutaDa = -1.0;
		BarcolloDa = -1.0;
		return false;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	float Angolo = 0.f;
	bool bAncora = false;
	if (CadutaDa >= 0.0)
	{
		// all'indietro in un attimo, giu' per un po', poi su piano (i gradi attorno ai piedi)
		constexpr float Giu = 0.35f;
		constexpr float Su = 0.9f;
		constexpr float Steso = -85.f;
		const float T = static_cast<float>(Adesso - CadutaDa);
		if (T < Giu)
		{
			Angolo = Steso * FMath::InterpEaseIn(0.f, 1.f, T / Giu, 2.f);
			bAncora = true;
		}
		else if (T < Giu + ValdorsoFiducia::SecondiATerra)
		{
			Angolo = Steso;
			bAncora = true;
		}
		else if (T < Giu + ValdorsoFiducia::SecondiATerra + Su)
		{
			Angolo = Steso * (1.f - FMath::InterpEaseInOut(0.f, 1.f, (T - Giu - ValdorsoFiducia::SecondiATerra) / Su, 2.f));
			bAncora = true;
		}
		else
		{
			CadutaDa = -1.0;
		}
	}
	else if (BarcolloDa >= 0.0)
	{
		const float T = static_cast<float>(Adesso - BarcolloDa);
		if (T < 0.12f)
		{
			Angolo = -12.f * (T / 0.12f);
			bAncora = true;
		}
		else if (T < 0.45f)
		{
			Angolo = -12.f * (1.f - (T - 0.12f) / 0.33f);
			bAncora = true;
		}
		else
		{
			BarcolloDa = -1.0;
		}
	}
	if (!bAncora && !bPiegato)
	{
		return false;
	}
	// il manichino guarda la X dell'attore con -90 di imbardata (vedi il costruttore): la piega e' attorno alla Y
	const FQuat Base = FRotator(0.f, -90.f, 0.f).Quaternion();
	Corpo->SetRelativeRotation(FQuat(FVector::YAxisVector, FMath::DegreesToRadians(Angolo)) * Base);
	bPiegato = bAncora;
	return bAncora;
}
