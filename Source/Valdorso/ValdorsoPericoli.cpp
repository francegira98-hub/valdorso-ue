// Valdorso - I pericoli della valle: equilibrio, morte e rinascita (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoPericoli.h"
#include "ValdorsoScherma.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoTerritorio.h"
#include "ValdorsoModoFoto.h"
#include "Valdorso.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoPericoli"

namespace ValdorsoPericoli
{
	bool PerdeEquilibrio(float Equilibrio)
	{
		return Equilibrio <= 0.5f;
	}

	bool Caduta(float Salute)
	{
		return Salute <= KINDA_SMALL_NUMBER;
	}

	FVector VersoIngresso(const FVector& Pietra, const FVector& CentroTempio)
	{
		const FVector Piano(CentroTempio.X - Pietra.X, CentroTempio.Y - Pietra.Y, 0.0);
		if (Piano.SizeSquared() < FMath::Square(50.0))
		{
			return FVector(0.0, 1.0, 0.0);
		}
		return Piano.GetSafeNormal();
	}

	FTransform DavantiAllAltare(const FVector& Pietra, const FVector& Verso, float MezzaAltezza)
	{
		FVector Piano(Verso.X, Verso.Y, 0.0);
		Piano = Piano.IsNearlyZero() ? FVector(0.0, 1.0, 0.0) : Piano.GetSafeNormal();
		FVector Dove = Pietra + Piano * DistanzaDallAltare;
		// i piedi sul pavimento (sotto l'altare), il centro della capsula più su, e 2 cm d'aria per non incastrarsi
		Dove.Z = Pietra.Z - AltezzaAltare + MezzaAltezza + 2.0;
		return FTransform(Piano.Rotation(), Dove);
	}
}

namespace
{
	/**
	 * I coloni su cui vale un comando di console: quello con quel nome; senza nome il proprio (da soli o ospitando) o
	 * tutti (server dedicato). Solo dove c'è l'autorità. Restituisce quanti ne ha trovati.
	 */
	int32 PericoliPerOgniColono(UWorld* Mondo, const FString& Nome, TFunctionRef<void(UValdorsoPericoliComponent&)> Fai)
	{
		if (!Mondo)
		{
			return 0;
		}
		TArray<UValdorsoPericoliComponent*> Scelti;
		TArray<UValdorsoPericoliComponent*> Tutti;
		UValdorsoPericoliComponent* Mio = nullptr;
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Controllore = It->Get();
			if (!Controllore || !Controllore->HasAuthority())
			{
				continue;
			}
			UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Controllore->GetPawn());
			if (!Pericoli)
			{
				continue;
			}
			if (!Nome.IsEmpty())
			{
				if (Controllore->PlayerState && Controllore->PlayerState->GetPlayerName().Equals(Nome, ESearchCase::IgnoreCase))
				{
					Scelti.Add(Pericoli);
				}
				continue;
			}
			Tutti.Add(Pericoli);
			if (!Mio && Controllore->IsLocalController())
			{
				Mio = Pericoli;
			}
		}
		if (Nome.IsEmpty())
		{
			if (Mio)
			{
				Scelti.Add(Mio);
			}
			else
			{
				Scelti = Tutti;
			}
		}
		for (UValdorsoPericoliComponent* Pericoli : Scelti)
		{
			Fai(*Pericoli);
		}
		return Scelti.Num();
	}

	/** Il nome scritto dopo i primi Salta argomenti (i nomi dei personaggi possono avere spazi). */
	FString PericoliNomeDa(const TArray<FString>& Argomenti, int32 Salta)
	{
		TArray<FString> Resto;
		for (int32 i = Salta; i < Argomenti.Num(); ++i)
		{
			Resto.Add(Argomenti[i]);
		}
		return FString::Join(Resto, TEXT(" ")).TrimStartAndEnd();
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoPericoliFerisci(
		TEXT("Valdorso.Ferisci"),
		TEXT("Per le prove: toglie salute al colono, Valdorso.Ferisci <quantità> [nome] (negativo cura; sul server, o da soli)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			if (Argomenti.Num() < 1)
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Pericoli: serve la quantità, per esempio Valdorso.Ferisci 30"));
				return;
			}
			const float Quantita = FCString::Atof(*Argomenti[0]);
			const int32 Trovati = PericoliPerOgniColono(Mondo, PericoliNomeDa(Argomenti, 1),
				[Quantita](UValdorsoPericoliComponent& Pericoli) { Pericoli.Ferisci(Quantita); });
			const FString Esito = Trovati > 0 ? FString::Printf(TEXT("ferita di %.0f a %d coloni"), Quantita, Trovati)
				: FString(TEXT("nessun colono (il comando vale sul server, o giocando da soli)"));
			UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Pericoli: %s"), *Esito);
		}));

	FAutoConsoleCommandWithWorldAndArgs ComandoPericoliMuori(
		TEXT("Valdorso.Muori"),
		TEXT("Per le prove: il colono cade subito e dopo qualche secondo rinasce al Tempio del Cuore, Valdorso.Muori [nome] (sul server, o da soli)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const int32 Trovati = PericoliPerOgniColono(Mondo, PericoliNomeDa(Argomenti, 0),
				[](UValdorsoPericoliComponent& Pericoli) { Pericoli.Muori(); });
			const FString Esito = Trovati > 0 ? FString::Printf(TEXT("%d coloni caduti"), Trovati)
				: FString(TEXT("nessun colono (il comando vale sul server, o giocando da soli)"));
			UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Pericoli: %s"), *Esito);
		}));
}

// ------------------------------------------------------------------------------------------------- il componente

UValdorsoPericoliComponent::UValdorsoPericoliComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UValdorsoPericoliComponent* UValdorsoPericoliComponent::Di(const AActor* Corpo)
{
	return Corpo ? Corpo->FindComponentByClass<UValdorsoPericoliComponent>() : nullptr;
}

void UValdorsoPericoliComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// a tutti: anche gli altri devono vedere il colono a terra
	DOREPLIFETIME(UValdorsoPericoliComponent, bCaduto);
}

void UValdorsoPericoliComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Proprietario = GetOwner();
	if (!Proprietario || !Proprietario->HasAuthority())
	{
		return;
	}
	// Solo il server ascolta le statistiche (e decide cadute e barcollamenti).
	if (UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Proprietario))
	{
		ManigliaSalute = Abilita->GetGameplayAttributeValueChangeDelegate(UValdorsoAttributeSet::GetSaluteAttribute())
			.AddUObject(this, &UValdorsoPericoliComponent::QuandoCambiaSalute);
		ManigliaEquilibrio = Abilita->GetGameplayAttributeValueChangeDelegate(UValdorsoAttributeSet::GetEquilibrioAttribute())
			.AddUObject(this, &UValdorsoPericoliComponent::QuandoCambiaEquilibrio);
	}
}

void UValdorsoPericoliComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (ManigliaSalute.IsValid())
		{
			Abilita->GetGameplayAttributeValueChangeDelegate(UValdorsoAttributeSet::GetSaluteAttribute()).Remove(ManigliaSalute);
		}
		if (ManigliaEquilibrio.IsValid())
		{
			Abilita->GetGameplayAttributeValueChangeDelegate(UValdorsoAttributeSet::GetEquilibrioAttribute()).Remove(ManigliaEquilibrio);
		}
	}
	ManigliaSalute.Reset();
	ManigliaEquilibrio.Reset();
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerRinascita);
		Mondo->GetTimerManager().ClearTimer(TimerBarcollo);
		Mondo->GetTimerManager().ClearTimer(TimerPosaMorte);
	}
	Super::EndPlay(EndPlayReason);
}

// --- Sul server --------------------------------------------------------------------------------------

void UValdorsoPericoliComponent::QuandoCambiaSalute(const FOnAttributeChangeData& Dati)
{
	// chi ha dato il colpo (se è arrivato da un effetto, per esempio il pugno)
	if (Dati.GEModData)
	{
		if (AActor* Chi = Dati.GEModData->EffectSpec.GetEffectContext().GetOriginalInstigator(); Chi && Chi != GetOwner())
		{
			UltimoColpevole = Chi;
		}
	}
	if (!bCaduto && ValdorsoPericoli::Caduta(Dati.NewValue))
	{
		Cadi(Dati.GEModData ? TEXT("salute a zero (colpi)") : TEXT("salute a zero"));
	}
}

void UValdorsoPericoliComponent::QuandoCambiaEquilibrio(const FOnAttributeChangeData& Dati)
{
	if (bCaduto || bBarcolla || !ValdorsoPericoli::PerdeEquilibrio(Dati.NewValue))
	{
		return;
	}
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	UWorld* Mondo = GetWorld();
	if (!Colono || !Mondo)
	{
		return;
	}
	bBarcolla = true;
	// (11/11, v0.1.5) la Tempra: chi si rialza prima (75) barcolla la meta' (ValdorsoScherma.h)
	const float Barcollo = ValdorsoScherma::Talento(Colono, FName(TEXT("Tempra")), 3) ? SecondiBarcollo * 0.5f : SecondiBarcollo;
	// Lo stordimento che c'è già (ClientAvviso): il colono non si muove per un istante e lo legge in alto.
	if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(Colono->GetController()))
	{
		Controllore->ClientAvviso(LOCTEXT("Barcolli", "Perdi l'equilibrio!"), Barcollo + 0.5f, Barcollo);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Pericoli: %s barcolla"), *Colono->GetName());
	Mondo->GetTimerManager().SetTimer(TimerBarcollo, this, &UValdorsoPericoliComponent::FineBarcollo, Barcollo, false);
}

void UValdorsoPericoliComponent::FineBarcollo()
{
	bBarcolla = false;
	if (bCaduto)
	{
		return;
	}
	// passato il barcollare, l'equilibrio torna pieno
	if (UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetEquilibrioAttribute(),
			Abilita->GetNumericAttribute(UValdorsoAttributeSet::GetEquilibrioMaxAttribute()));
	}
}

void UValdorsoPericoliComponent::Ferisci(float Quantita)
{
	AActor* Proprietario = GetOwner();
	UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Proprietario);
	if (!Proprietario || !Proprietario->HasAuthority() || !Abilita || bCaduto)
	{
		return;
	}
	const float Massimo = Abilita->GetNumericAttribute(UValdorsoAttributeSet::GetSaluteMaxAttribute());
	const float Prima = Abilita->GetNumericAttribute(UValdorsoAttributeSet::GetSaluteAttribute());
	// (la caduta, se la salute arriva a zero, la fa partire QuandoCambiaSalute)
	Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute(), FMath::Clamp(Prima - Quantita, 0.f, Massimo));
}

void UValdorsoPericoliComponent::Muori()
{
	AActor* Proprietario = GetOwner();
	if (!Proprietario || !Proprietario->HasAuthority() || bCaduto)
	{
		return;
	}
	UltimoColpevole.Reset();
	if (UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Proprietario))
	{
		Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute(), 0.f);
	}
	// se la salute era già a zero (o il colono non ha statistiche) la caduta parte da qui
	if (!bCaduto)
	{
		Cadi(TEXT("Valdorso.Muori (prova)"));
	}
}

void UValdorsoPericoliComponent::Cadi(const FString& Come)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	UWorld* Mondo = GetWorld();
	if (!Colono || !Mondo || bCaduto)
	{
		return;
	}
	bCaduto = true;
	bBarcolla = false;
	Mondo->GetTimerManager().ClearTimer(TimerBarcollo);
	MostraCaduta();

	AActor* Colpevole = UltimoColpevole.Get();
	ValdorsoRegistroEventi::Annota(TEXT("Morte"), Colono, Colpevole, Colono->GetActorLocation(),
		Colpevole ? FString::Printf(TEXT("%s; ultimo colpo dal bersaglio"), *Come) : Come);
	Mondo->GetTimerManager().SetTimer(TimerRinascita, this, &UValdorsoPericoliComponent::Rinasci, SecondiRinascita, false);
}

FTransform UValdorsoPericoliComponent::PuntoDiRinascita(const ACharacter* Colono) const
{
	UWorld* Mondo = GetWorld();
	const float MezzaAltezza = Colono->GetCapsuleComponent() ? Colono->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.f;
	const FValdorsoLuogo* Tempio = ValdorsoLuoghi::Trova(TEXT("Tempio del Cuore"));

	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoRinascita), false, Colono);
	const FCollisionObjectQueryParams Statici(ECC_WorldStatic);
	// Il pavimento sotto un punto (una linea dall'alto, ma sotto il tetto): vero se l'ha trovato.
	auto SulPavimento = [Mondo, &Parametri, &Statici, MezzaAltezza](FVector& Punto, float Sopra, float Sotto)
	{
		FHitResult Colpo;
		if (Mondo && Mondo->LineTraceSingleByObjectType(Colpo, Punto + FVector(0.0, 0.0, Sopra), Punto - FVector(0.0, 0.0, Sotto), Statici, Parametri))
		{
			Punto.Z = Colpo.ImpactPoint.Z + MezzaAltezza + 2.0;
			return true;
		}
		return false;
	};

	// 1. Davanti all'altare con la pietra di Val d'Orso, girato verso l'ingresso.
	AValdorsoPietraVillaggio* Pietra = nullptr;
	if (Mondo)
	{
		TActorIterator<AValdorsoPietraVillaggio> Prima(Mondo);
		Pietra = Prima ? *Prima : nullptr;
	}
	if (Pietra)
	{
		const FVector Dove = Pietra->GetActorLocation();
		const FVector Centro = Tempio ? Tempio->Posizione : Dove + FVector(0.0, 1000.0, 0.0);
		FTransform Punto = ValdorsoPericoli::DavantiAllAltare(Dove, ValdorsoPericoli::VersoIngresso(Dove, Centro), MezzaAltezza);
		Parametri.AddIgnoredActor(Pietra);
		FVector Piedi = Punto.GetLocation();
		SulPavimento(Piedi, 80.f, 300.f);
		Punto.SetLocation(Piedi);
		return Punto;
	}
	// 2. Nel luogo "Tempio del Cuore" (il centro della navata), girato verso sud (la piazza).
	if (Tempio)
	{
		FVector Piedi = Tempio->Posizione + FVector(0.0, 0.0, MezzaAltezza + 2.0);
		SulPavimento(Piedi, 300.f, 600.f);
		return FTransform(FRotator(0.0, 90.0, 0.0), Piedi);
	}
	// 3. Il punto di partenza della mappa.
	AGameModeBase* Modalita = Mondo ? Mondo->GetAuthGameMode() : nullptr;
	if (AActor* Partenza = Modalita ? Modalita->FindPlayerStart(Colono->GetController()) : nullptr)
	{
		return FTransform(FRotator(0.0, Partenza->GetActorRotation().Yaw, 0.0), Partenza->GetActorLocation());
	}
	return Colono->GetActorTransform();
}

void UValdorsoPericoliComponent::Rinasci()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || !bCaduto)
	{
		return;
	}
	// prima si rialza (la mesh torna sotto la capsula), poi va al tempio
	bCaduto = false;
	MostraRialzata();

	const FTransform Punto = PuntoDiRinascita(Colono);
	const FRotator Verso(0.0, Punto.Rotator().Yaw, 0.0);
	if (!Colono->TeleportTo(Punto.GetLocation(), Verso, false, false))
	{
		Colono->TeleportTo(Punto.GetLocation(), Verso, false, true);
	}
	if (AController* Controllore = Colono->GetController())
	{
		Controllore->SetControlRotation(Verso);
		if (APlayerController* Giocatore = Cast<APlayerController>(Controllore))
		{
			Giocatore->ClientSetRotation(Verso, true);
		}
	}

	// tutte le statistiche piene
	if (UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Colono))
	{
		auto Riempi = [Abilita](const FGameplayAttribute& Valore, const FGameplayAttribute& Massimo)
		{
			Abilita->SetNumericAttributeBase(Valore, Abilita->GetNumericAttribute(Massimo));
		};
		Riempi(UValdorsoAttributeSet::GetSaluteAttribute(), UValdorsoAttributeSet::GetSaluteMaxAttribute());
		Riempi(UValdorsoAttributeSet::GetStaminaAttribute(), UValdorsoAttributeSet::GetStaminaMaxAttribute());
		Riempi(UValdorsoAttributeSet::GetManaAttribute(), UValdorsoAttributeSet::GetManaMaxAttribute());
		Riempi(UValdorsoAttributeSet::GetEquilibrioAttribute(), UValdorsoAttributeSet::GetEquilibrioMaxAttribute());
	}
	UltimoColpevole.Reset();

	ValdorsoRegistroEventi::Annota(TEXT("Rinascita"), Colono, nullptr, Colono->GetActorLocation(), TEXT("al Tempio del Cuore"));
	// il personaggio si salva subito: rinato al tempio, con le statistiche piene
	if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(Colono->GetController()))
	{
		Controllore->SalvaPersonaggio();
	}
}

// --- Su ogni PC: il corpo che cade e si rialza ---------------------------------------------------------

void UValdorsoPericoliComponent::OnRep_Caduto()
{
	if (bCaduto)
	{
		MostraCaduta();
	}
	else
	{
		MostraRialzata();
	}
}

void UValdorsoPericoliComponent::MostraCaduta()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || bCadutaMostrata)
	{
		return;
	}
	bCadutaMostrata = true;

	// fermo: niente movimento (sul server e su ogni PC)
	if (UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement())
	{
		Movimento->StopMovementImmediately();
		Movimento->DisableMovement();
	}
	// il giocatore di questo colono: via i tasti del corpo e la modalità foto
	if (Colono->IsLocallyControlled())
	{
		if (APlayerController* Giocatore = Cast<APlayerController>(Colono->GetController()))
		{
			ValdorsoFoto::Chiudi(Giocatore);
			Colono->DisableInput(Giocatore);
			bTastiSpenti = true;
		}
	}
	// sul server dedicato nessuno guarda: niente animazione né fisica
	if (Colono->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	USkeletalMeshComponent* Corpo = Colono->GetMesh();
	if (!Corpo)
	{
		return;
	}
	// il montaggio della morte, se c'è: si ferma sull'ultimo fotogramma
	if (MontaggioMorte && Corpo->GetAnimInstance())
	{
		const float Durata = Colono->PlayAnimMontage(MontaggioMorte);
		if (Durata > 0.f)
		{
			if (UWorld* Mondo = GetWorld())
			{
				// prima che il montaggio cominci a sfumare via (dopo non è più "attivo" e la pausa non lo ferma più)
				const float Sfumatura = MontaggioMorte->bEnableAutoBlendOut ? MontaggioMorte->BlendOut.GetBlendTime() : 0.f;
				Mondo->GetTimerManager().SetTimer(TimerPosaMorte, this, &UValdorsoPericoliComponent::FermaPosaMorte,
					FMath::Max(0.05f, Durata - Sfumatura - 0.05f), false);
			}
			return;
		}
	}
	// altrimenti la bambola di pezza: la fisica del manichino (il corpo MetaHuman ne copia la posa)
	if (!bMeshLetta)
	{
		MeshRelativa = Corpo->GetRelativeTransform();
		ProfiloMesh = Corpo->GetCollisionProfileName();
		bMeshLetta = true;
	}
	Corpo->SetCollisionProfileName(FName(TEXT("Ragdoll")));
	Corpo->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Corpo->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Corpo->SetAllBodiesSimulatePhysics(true);
	Corpo->SetSimulatePhysics(true);
	Corpo->WakeAllRigidBodies();
	bBambola = true;
}

void UValdorsoPericoliComponent::FermaPosaMorte()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Corpo = Colono ? Colono->GetMesh() : nullptr;
	if (bCaduto && MontaggioMorte && Corpo && Corpo->GetAnimInstance())
	{
		Corpo->GetAnimInstance()->Montage_Pause(MontaggioMorte);
	}
}

void UValdorsoPericoliComponent::MostraRialzata()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || !bCadutaMostrata)
	{
		return;
	}
	bCadutaMostrata = false;
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerPosaMorte);
	}
	USkeletalMeshComponent* Corpo = Colono->GetMesh();
	if (Corpo && MontaggioMorte)
	{
		Colono->StopAnimMontage(MontaggioMorte);
	}
	// la bambola di pezza torna mesh: niente fisica, collisioni di prima, di nuovo sotto la capsula
	if (Corpo && bBambola)
	{
		Corpo->SetSimulatePhysics(false);
		Corpo->SetAllBodiesSimulatePhysics(false);
		if (bMeshLetta)
		{
			Corpo->SetCollisionProfileName(ProfiloMesh);
		}
		Corpo->AttachToComponent(Colono->GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Corpo->SetRelativeTransform(MeshRelativa);
	}
	bBambola = false;
	if (UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement())
	{
		Movimento->SetMovementMode(MOVE_Walking);
	}
	if (bTastiSpenti)
	{
		if (APlayerController* Giocatore = Cast<APlayerController>(Colono->GetController()))
		{
			Colono->EnableInput(Giocatore);
		}
		bTastiSpenti = false;
	}
}

#undef LOCTEXT_NAMESPACE
