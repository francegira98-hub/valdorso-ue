// Valdorso - I PNG della valle (vedi il .h). Scritto da Claude il 07/11/2026.
// Unity build: le cose del namespace anonimo cominciano con Png, così non si scontrano con gli altri file.

#include "ValdorsoPNG.h"
#include "ValdorsoGrottaNord.h"
#include "ValdorsoRiti.h"
#include "ValdorsoStorie.h"
#include "ValdorsoVoci.h"
#include "ValdorsoGilda.h"
#include "ValdorsoChat.h"
#include "ValdorsoCielo.h"
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
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
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
			if (Zaino && !Segno.IsNone() && !HaSegno(IdDialogo, Segno) && ValdorsoOggetti::DonoDelSegno(IdDialogo, Segno, Dono, Quanti))
			{
				Zaino->Dai(Dono, Quanti);
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
		UValdorsoZainoComponent* Zaino = nullptr;
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
		if (Giornata.Num() > 0 || AttoMinimo > 0)
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
}

void AValdorsoPNG::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerRitorno);
		Mondo->GetTimerManager().ClearTimer(TimerSaluti);
		Mondo->GetTimerManager().ClearTimer(TimerGiornata);
	}
	Conversazioni.Reset();
	Super::EndPlay(EndPlayReason);
}

const FValdorsoDialogo* AValdorsoPNG::DatiDialogo() const
{
	return ValdorsoDialoghi::Trova(Dialogo);
}

FText AValdorsoPNG::TestoInterazione(const APawn* Chi) const
{
	if (bVia)
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
	if (!HasAuthority() || !Chi || bVia)
	{
		return;
	}
	const FValdorsoDialogo* Dati = DatiDialogo();
	APlayerController* Controllore = Cast<APlayerController>(Chi->GetController());
	if (!Dati || !Controllore)
	{
		return;
	}
	// Via le conversazioni di chi se n'è andato dal server.
	for (auto It = Conversazioni.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	// (08/11) La memoria di questo colono: nel file del suo personaggio (o, senza, per nome).
	FPngMemoriaColono Memoria(Chi);
	const bool bGiaParlato = Memoria.GiaParlato(Dati->Id);
	Memoria.SegnaParlato(Dati->Id);

	FConversazione Nuova;
	Nuova.Nodo = ValdorsoDialoghi::NodoIniziale(*Dati, bGiaParlato);
	Nuova.Passo = ProssimoPasso++;
	Conversazioni.Add(TWeakObjectPtr<APlayerController>(Controllore), Nuova);
	PngSegnaNodo(*Dati, Nuova.Nodo, Memoria);

	ValdorsoRegistroEventi::Annota(TEXT("Dialogo"), Chi, this, GetActorLocation(),
		FString::Printf(TEXT("parla con %s (%s%s)"), *Dati->NomeBreve, bGiaParlato ? TEXT("di nuovo") : TEXT("la prima volta"),
			Memoria.NelFile() ? TEXT("") : TEXT(", senza personaggio salvato")));

	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerRitorno);
	}
	GuardaVerso(Chi);
	Manda(Controllore, ValdorsoDialoghi::Battuta(*Dati, Nuova.Nodo, Nuova.Passo, Memoria));
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
	const FValdorsoDialogo* Dati = DatiDialogo();
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
		return;
	}
	Aperta->Nodo = Prossimo;
	Aperta->Passo = ProssimoPasso++;
	GuardaVerso(Colono);
	Manda(Chi, ValdorsoDialoghi::Battuta(*Dati, Aperta->Nodo, Aperta->Passo, Memoria));
	if (Bancone)
	{
		Bancone->ApriBancone(this);
	}
}

void AValdorsoPNG::Manda(APlayerController* Chi, const FValdorsoBattuta& Battuta)
{
	if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
	{
		// (11/11, v0.1.6) i segnaposto: le voci vere della locanda ({voci}) e le righe dell'Atto I (ValdorsoVoci.h)
		FValdorsoBattuta Piena = Battuta;
		ValdorsoVoci::Riempi(GetWorld(), Piena.Righe);
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
	Conversazioni.Remove(Chiave);
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
	if (!HasAuthority() || !Chi)
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
	if (!bSguardoValido)
	{
		SetActorTickEnabled(false);
		return;
	}
	const float Adesso = static_cast<float>(GetActorRotation().Yaw);
	const float Manca = FMath::FindDeltaAngleDegrees(Adesso, Sguardo);
	const float Passo = VelocitaGiro * DeltaSeconds;
	if (FMath::Abs(Manca) <= Passo)
	{
		SetActorRotation(FRotator(0.f, Sguardo, 0.f));
		SetActorTickEnabled(false);
		return;
	}
	SetActorRotation(FRotator(0.f, Adesso + FMath::Sign(Manca) * Passo, 0.f));
}

// --- (10/11) I saluti a chi passa ----------------------------------------------------------------------------------

void AValdorsoPNG::Saluta()
{
	UWorld* Mondo = GetWorld();
	const FValdorsoDialogo* Dati = DatiDialogo();
	if (!HasAuthority() || !Mondo || !Dati || bVia)
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
		const FString Frase = ValdorsoDialoghi::Saluto(Dati->Id, Ora, FMath::Rand());
		Salutati.Add(TWeakObjectPtr<APlayerController>(Giocatore), Adesso);
		if (Frase.IsEmpty())
		{
			continue;
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
	if (!HasAuthority() || !Mondo || Conversazioni.Num() > 0)
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
	const bool bVuoleVia = ValdorsoVoci::Atto(this) < AttoMinimo || (Giornata.IsValidIndex(Voluta) && Giornata[Voluta].bAssente);
	if (Voluta == Tappa && bVuoleVia == bVia)
	{
		VuoleCambiareDa = -1.0;
		bGiornataPartita = true;
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
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
	VuoleCambiareDa = -1.0;
	bGiornataPartita = true;
	Tappa = Voluta;
	bVia = bVuoleVia;
	ForceNetUpdate();
	OnRep_Tappa();
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
	if (Corpo && GetNetMode() != NM_DedicatedServer)
	{
		UAnimSequence* Movenza = nullptr;
		if (!Ora.Movenza.IsNone())
		{
			const FString Nome = Ora.Movenza.ToString();
			Movenza = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Mixamo/Gesti/%s.%s"), *Nome, *Nome), nullptr,
				LOAD_NoWarn | LOAD_Quiet);
		}
		if (Movenza || Riposo)
		{
			Corpo->PlayAnimation(Movenza ? Movenza : Riposo.Get(), true);
		}
	}
}
