// Valdorso - Dormire (vedi il .h). Scritto da Claude il 05/11/2026.

#include "ValdorsoLetto.h"
#include "Valdorso.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoSogni.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/Layout/SBorder.h"

#define LOCTEXT_NAMESPACE "ValdorsoLetto"

namespace
{
	const TCHAR* LettoAnimazioneDorme = TEXT("/Game/Mixamo/Dormire/Dorme.Dorme");

	// (05/11) Come sta il corpo nell'animazione "Laying Sleeping" di Mixamo: sdraiato lungo l'asse Y della mesh, con il
	// bacino a 103 cm dalla radice. Numeri provati da Fra sul letto dei coloni con Valdorso.Letto.Y 90 e .Z 6 (partendo
	// da Y -8, Z -88). Valgono per tutti i letti; YawCorpo e RitoccoCorpo del letto sono ritocchi in più per il singolo
	// letto. Se si cambia animazione, si rifanno questi numeri.
	constexpr float LettoYawAnimazione = -90.f;
	const FVector LettoRitoccoAnimazione(0.f, 82.f, -82.f);

	/** Mezzo secondo per scurire lo schermo: a schermo nero il server sposta il colono. */
	constexpr double LettoMezzoVelo = 0.5;

	// Comandi di console per sistemare a occhio il corpo sdraiato (si sommano ai valori del letto; valgono dalla
	// prossima volta che il colono si corica). Quando i numeri vanno bene, si scrivono nel Blueprint del letto.
	TAutoConsoleVariable<float> CVarLettoYaw(TEXT("Valdorso.Letto.Yaw"), 0.f, TEXT("Gradi in più per il corpo sdraiato."));
	TAutoConsoleVariable<float> CVarLettoX(TEXT("Valdorso.Letto.X"), 0.f, TEXT("Cm in più per il corpo sdraiato, di traverso al letto."));
	TAutoConsoleVariable<float> CVarLettoY(TEXT("Valdorso.Letto.Y"), 0.f, TEXT("Cm in più per il corpo sdraiato, lungo il letto."));
	TAutoConsoleVariable<float> CVarLettoZ(TEXT("Valdorso.Letto.Z"), 0.f, TEXT("Cm in più per il corpo sdraiato, in altezza."));

	FString LettoTesto(const FTransform& T)
	{
		const FVector L = T.GetLocation();
		return FString::Printf(TEXT("(%.0f, %.0f, %.0f) yaw %.0f"), L.X, L.Y, L.Z, T.Rotator().Yaw);
	}

	UAnimSequence* LettoAnimazione()
	{
		UAnimSequence* Sequenza = LoadObject<UAnimSequence>(nullptr, LettoAnimazioneDorme);
		if (!Sequenza)
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Letto: manca l'animazione %s (lanciare porta_dormire_da_mixamo.py)."), LettoAnimazioneDorme);
			return nullptr;
		}
		// Il colono sta fermo sul letto: la radice resta dov'è (niente scivolate).
		Sequenza->bEnableRootMotion = true;
		Sequenza->RootMotionRootLock = ERootMotionRootLock::RefPose;
		return Sequenza;
	}
}

// ------------------------------------------------------------------------------------------------
// Il componente sul colono
// ------------------------------------------------------------------------------------------------

UValdorsoSonnoComponent::UValdorsoSonnoComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UValdorsoSonnoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UValdorsoSonnoComponent, Stato);
}

double UValdorsoSonnoComponent::Adesso() const
{
	const UWorld* Mondo = GetWorld();
	if (const AGameStateBase* Gioco = Mondo ? Mondo->GetGameState() : nullptr)
	{
		return Gioco->GetServerWorldTimeSeconds();
	}
	return Mondo ? Mondo->GetTimeSeconds() : 0.0;
}

void UValdorsoSonnoComponent::Corica(AValdorsoLetto* Letto, int32 Posto)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || !Colono->HasAuthority() || !Letto || Occupato() || !Letto->Posti.IsValidIndex(Posto))
	{
		return;
	}
	Letto->Occupanti.SetNum(Letto->Posti.Num());
	Letto->Occupanti[Posto] = Colono;
	Stato.Letto = Letto;
	Stato.Posto = static_cast<uint8>(Posto);
	Letto->TrasformaSdraiato(Posto, Colono, Stato.Dove, Stato.MeshRelativa);
	Stato.Uscita = Letto->TrasformaUscita(Posto, Colono);
	// Se accanto al letto c'è un muro o un mobile, il motore cerca lì vicino un punto dove la capsula sta.
	FVector Luogo = Stato.Uscita.GetLocation();
	FRotator Verso = Stato.Uscita.Rotator();
	if (GetWorld() && GetWorld()->FindTeleportSpot(Colono, Luogo, Verso))
	{
		Stato.Uscita.SetLocation(Luogo);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Letto: %s si corica nel posto %d: sdraiato %s, si rialzerà in %s"),
		*Colono->GetName(), Posto, *LettoTesto(Stato.Dove), *LettoTesto(Stato.Uscita));
	ValdorsoRegistroEventi::Annota(TEXT("Dorme"), Colono, Letto, Stato.Dove.GetLocation());
	CambiaFase(1);
}

void UValdorsoSonnoComponent::Alzati()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || !Colono->HasAuthority() || Stato.Fase != 2)
	{
		return;
	}
	ValdorsoRegistroEventi::Annota(TEXT("SiSveglia"), Colono, Stato.Letto, Colono->GetActorLocation());
	CambiaFase(3);
}

void UValdorsoSonnoComponent::ChiediDiAlzarti()
{
	if (Stato.Fase != 2 || !GetWorld())
	{
		return;
	}
	const double Ora = GetWorld()->GetTimeSeconds();
	if (Ora - UltimaRichiesta < 0.5)
	{
		return;
	}
	UltimaRichiesta = Ora;
	ServerAlzati();
}

void UValdorsoSonnoComponent::ServerAlzati_Implementation()
{
	Alzati();
}

void UValdorsoSonnoComponent::CambiaFase(uint8 Nuova)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	Stato.Fase = Nuova;
	Stato.Inizio = Adesso();
	if (Nuova == 0)
	{
		if (Stato.Letto)
		{
			Stato.Letto->Libera(Colono);
		}
		Stato.Letto = nullptr;
	}
	if (Colono)
	{
		Colono->ForceNetUpdate();
	}
	OnRep_Stato();
	// (11/11, v0.1.6) appena addormentato: il server decide se sogna (ValdorsoSogni.h)
	if (Nuova == 2 && Colono && Colono->HasAuthority())
	{
		ValdorsoSogni::Addormentato(Colono);
	}
}

void UValdorsoSonnoComponent::ClientSogno_Implementation(FName Id)
{
	ValdorsoSogni::Mostra(GetWorld(), Id);
}

void UValdorsoSonnoComponent::OnRep_Stato()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono)
	{
		return;
	}
	// (11/11, v0.1.6) chi si sveglia interrompe il sogno
	if ((Stato.Fase == 3 || Stato.Fase == 0) && Colono->IsLocallyControlled())
	{
		ValdorsoSogni::Chiudi();
	}
	// Un cambio vero: fase diversa, o stessa fase ricominciata (per esempio un altro letto, se la rete ha perso i passi in mezzo).
	const uint8 Prima = FaseVista;
	const bool bNuova = Stato.Fase != FaseVista || Stato.Inizio != InizioVisto;
	UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement();
	UCapsuleComponent* Capsula = Colono->GetCapsuleComponent();

	if (Stato.Fase != 0)
	{
		// Si corica, dorme o si alza: niente movimento normale, e il letto non fa da muro alla capsula.
		if (Movimento && Movimento->MovementMode != MOVE_None)
		{
			Movimento->StopMovementImmediately();
			Movimento->DisableMovement();
		}
		if (Capsula && Stato.Letto && LettoIgnorato.Get() != Stato.Letto)
		{
			if (LettoIgnorato.IsValid())
			{
				Capsula->IgnoreActorWhenMoving(LettoIgnorato.Get(), false);
			}
			Capsula->IgnoreActorWhenMoving(Stato.Letto, true);
			LettoIgnorato = Stato.Letto;
		}
		SetComponentTickEnabled(true);
	}
	if (Stato.Fase == 2 && bNuova)
	{
		// A schermo nero: il colono va sul letto.
		Colono->SetActorLocationAndRotation(Stato.Dove.GetLocation(), Stato.Dove.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
		MettiMesh(true);
	}
	if (Stato.Fase == 0 && bNuova && Prima != 0)
	{
		// A schermo nero: il colono si ritrova in piedi accanto al letto.
		Colono->SetActorLocationAndRotation(Stato.Uscita.GetLocation(), Stato.Uscita.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
		MettiMesh(false);
		if (Movimento && Movimento->MovementMode == MOVE_None)
		{
			Movimento->SetMovementMode(MOVE_Walking);
		}
		if (Capsula && LettoIgnorato.IsValid())
		{
			Capsula->IgnoreActorWhenMoving(LettoIgnorato.Get(), false);
		}
		LettoIgnorato.Reset();
	}

	// Il velo nero, solo sullo schermo di chi dorme.
	if (Colono->IsLocallyControlled())
	{
		if ((Stato.Fase == 1 || Stato.Fase == 3) && bNuova)
		{
			MostraVelo();
		}
		else if ((Stato.Fase == 2 || Stato.Fase == 0) && Velo.IsValid() && VeloFine == 0.0)
		{
			VeloFine = FPlatformTime::Seconds();   // spostato: il velo si schiarisce
		}
	}
	FaseVista = Stato.Fase;
	InizioVisto = Stato.Inizio;
	SuonaFase();
	if (Stato.Fase == 0 && !Velo.IsValid())
	{
		SetComponentTickEnabled(false);
	}
}

void UValdorsoSonnoComponent::SuonaFase()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anima = Colono && Colono->GetMesh() ? Colono->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anima || (FaseSuonata == Stato.Fase && InizioSuonato == Stato.Inizio))
	{
		return;   // niente Anim Instance (si riprova al prossimo fotogramma) o già fatto
	}
	if (Stato.Fase == 2)
	{
		UAnimSequence* Sequenza = LettoAnimazione();
		if (!Sequenza)
		{
			return;
		}
		// In ciclo, dall'inizio (respira: chi arriva dopo non nota la differenza).
		Anima->PlaySlotAnimationAsDynamicMontage(Sequenza, TEXT("DefaultSlot"), 0.f, 0.2f, 1.f, 100000);
	}
	else if (Stato.Fase == 0)
	{
		Anima->StopSlotAnimation(0.f, TEXT("DefaultSlot"));
	}
	FaseSuonata = Stato.Fase;
	InizioSuonato = Stato.Inizio;
}

void UValdorsoSonnoComponent::MostraVelo()
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}
	TogliVelo();
	VeloInizio = FPlatformTime::Seconds();
	VeloFine = 0.0;
	static FSlateColorBrush Bianco(FLinearColor::White);
	TWeakObjectPtr<UValdorsoSonnoComponent> Debole(this);
	Velo = SNew(SBorder)
		.BorderImage(&Bianco)
		.Visibility(EVisibility::HitTestInvisible)
		.BorderBackgroundColor_Lambda([Debole]()
		{
			const UValdorsoSonnoComponent* Questo = Debole.Get();
			if (!Questo)
			{
				return FSlateColor(FLinearColor::Transparent);
			}
			const double Ora = FPlatformTime::Seconds();
			// Si scurisce in 0,4 s, resta nero finché il colono non è stato spostato, poi si schiarisce in mezzo secondo.
			const double Buio = Questo->VeloFine == 0.0
				? FMath::Clamp((Ora - Questo->VeloInizio) / 0.4, 0.0, 1.0)
				: 1.0 - FMath::Clamp((Ora - Questo->VeloFine) / 0.5, 0.0, 1.0);
			return FSlateColor(FLinearColor(0.f, 0.f, 0.f, static_cast<float>(Buio)));
		});
	Viewport->AddViewportWidgetContent(Velo.ToSharedRef(), 1000);
}

void UValdorsoSonnoComponent::TogliVelo()
{
	if (Velo.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Velo.ToSharedRef());
		}
		Velo.Reset();
	}
}

void UValdorsoSonnoComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* Colono = Cast<ACharacter>(GetOwner());

	// Sul server: a schermo nero (dopo mezzo secondo) si passa alla fase dopo.
	if (Colono && Colono->HasAuthority() && (Stato.Fase == 1 || Stato.Fase == 3) && Adesso() - Stato.Inizio >= LettoMezzoVelo)
	{
		CambiaFase(Stato.Fase == 1 ? 2 : 0);
	}

	// Se l'animazione non è partita (Anim Instance non ancora pronta, per chi arriva dopo), si riprova; e la mesh resta
	// sdraiata (sugli altri PC lo "smoothing" del movimento la rimetterebbe dritta).
	if (Stato.Fase == 2)
	{
		SuonaFase();
		MettiMesh(true);
	}

	// Sul PC: il velo si toglie quando si è schiarito (o dopo 4 s, se qualcosa è andato storto).
	if (Velo.IsValid())
	{
		const double Ora = FPlatformTime::Seconds();
		if ((VeloFine > 0.0 && Ora - VeloFine > 0.55) || Ora - VeloInizio > 4.0)
		{
			TogliVelo();
		}
	}
	if (Stato.Fase == 0 && !Velo.IsValid())
	{
		SetComponentTickEnabled(false);
	}
}

void UValdorsoSonnoComponent::MettiMesh(bool bSdraiata)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Colono ? Colono->GetMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}
	if (!bMeshBaseLetta)
	{
		MeshBasePosto = Colono->GetBaseTranslationOffset();
		MeshBaseVerso = Colono->GetBaseRotationOffsetRotator();
		bMeshBaseLetta = true;
	}
	const FVector Posto = bSdraiata ? Stato.MeshRelativa.GetLocation() : MeshBasePosto;
	const FRotator Verso = bSdraiata ? Stato.MeshRelativa.Rotator() : MeshBaseVerso;
	if (!Mesh->GetRelativeLocation().Equals(Posto, 0.1f) || !Mesh->GetRelativeRotation().Equals(Verso, 0.1f))
	{
		Mesh->SetRelativeLocationAndRotation(Posto, Verso);
		Colono->CacheInitialMeshOffset(Posto, Verso);
	}
}

void UValdorsoSonnoComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TogliVelo();
	if (Stato.Letto && GetOwner() && GetOwner()->HasAuthority())
	{
		Stato.Letto->Libera(Cast<APawn>(GetOwner()));
	}
	Super::EndPlay(EndPlayReason);
}

// ------------------------------------------------------------------------------------------------
// Il letto
// ------------------------------------------------------------------------------------------------

AValdorsoLetto::AValdorsoLetto()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	Posti = { 0.f };

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	Zona = CreateDefaultSubobject<UBoxComponent>(TEXT("Zona"));
	Zona->SetupAttachment(RootComponent);
	Zona->SetBoxExtent(FVector(70.f, 110.f, 30.f));
	Zona->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	// Solo per essere trovata con E: non ferma nessuno e non ferma la vista o la telecamera.
	Zona->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Zona->SetCollisionObjectType(ECC_WorldDynamic);
	Zona->SetCollisionResponseToAllChannels(ECR_Overlap);
	Zona->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Zona->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Zona->SetGenerateOverlapEvents(false);
	Zona->SetHiddenInGame(true);
}

void AValdorsoLetto::BeginPlay()
{
	Super::BeginPlay();
	Occupanti.SetNum(Posti.Num());
	// La zona si adatta al modello del Blueprint (i pezzi di Fab), se c'è.
	FBox Scatola(ForceInit);
	TArray<UPrimitiveComponent*> Pezzi;
	GetComponents<UPrimitiveComponent>(Pezzi);
	for (const UPrimitiveComponent* Pezzo : Pezzi)
	{
		if (Pezzo && Pezzo != Zona.Get() && Pezzo->IsRegistered())
		{
			// La scatola di ogni pezzo nel sistema del letto (giusta anche se il letto è girato).
			Scatola += Pezzo->CalcBounds(Pezzo->GetComponentTransform().GetRelativeTransform(GetActorTransform())).GetBox();
		}
	}
	if (Scatola.IsValid)
	{
		Zona->SetRelativeLocation(Scatola.GetCenter());
		Zona->SetBoxExtent(Scatola.GetExtent().ComponentMax(FVector(20.f)));
		// (05/11) Un letto largo (più di 1,9 m) con un solo posto al centro diventa un letto da due.
		const float Larghezza = Scatola.GetSize().X;
		if (Posti.Num() == 1 && FMath::IsNearlyZero(Posti[0]) && Larghezza > 190.f)
		{
			Posti = { -Larghezza * 0.22f, Larghezza * 0.22f };
			Occupanti.SetNum(Posti.Num());
		}
	}
}

void AValdorsoLetto::TrasformaSdraiato(int32 Posto, const APawn* Chi, FTransform& Attore, FTransform& MeshRelativa) const
{
	const ACharacter* Colono = Cast<ACharacter>(Chi);
	const float Traverso = Posti.IsValidIndex(Posto) ? Posti[Posto] : 0.f;
	const FVector Locale = FVector(Traverso, 0.f, AltezzaMaterasso) + LettoRitoccoAnimazione + RitoccoCorpo
		+ FVector(CVarLettoX.GetValueOnGameThread(), CVarLettoY.GetValueOnGameThread(), CVarLettoZ.GetValueOnGameThread());
	// La mesh del colono (i piedi del manichino) sul materasso, girata lungo il letto.
	const float Gradi = GetActorRotation().Yaw + 90.f + LettoYawAnimazione + YawCorpo + CVarLettoYaw.GetValueOnGameThread();
	const FTransform Mesh(FRotator(0.f, Gradi, 0.f), GetActorTransform().TransformPosition(Locale));
	const FTransform Relativa = Colono && Colono->GetMesh() ? Colono->GetMesh()->GetRelativeTransform() : FTransform(FRotator(0.f, -90.f, 0.f), FVector(0.f, 0.f, -90.f));
	// (05/11) L'attore (capsula e telecamera) resta in piedi sul materasso, girato come il corpo; è la mesh che si
	// abbassa e si sdraia sotto di lui. Così la telecamera non finisce dentro il letto.
	const FTransform Dritto = Relativa.Inverse() * Mesh;
	const float MezzaAltezza = Colono && Colono->GetCapsuleComponent() ? Colono->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector Sopra = GetActorTransform().TransformPosition(FVector(Traverso, 0.f, AltezzaMaterasso)) + FVector(0.f, 0.f, MezzaAltezza);
	Attore = FTransform(FRotator(0.f, Dritto.Rotator().Yaw, 0.f), Sopra);
	MeshRelativa = Mesh.GetRelativeTransform(Attore);
}

FTransform AValdorsoLetto::TrasformaUscita(int32 Posto, const APawn* Chi) const
{
	const ACharacter* Colono = Cast<ACharacter>(Chi);
	const float Traverso = Posti.IsValidIndex(Posto) ? Posti[Posto] : 0.f;
	// Si scende dalla parte del proprio posto (nei letti grandi, chi sta a sinistra scende a sinistra).
	const float Lato = Traverso < 0.f ? -1.f : 1.f;
	const float MezzaAltezza = Colono && Colono->GetCapsuleComponent() ? Colono->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector Luogo = GetActorTransform().TransformPosition(FVector(Lato * DistanzaUscita, 0.f, 0.f)) + FVector(0.f, 0.f, MezzaAltezza + 2.f);
	return FTransform(FRotator(0.f, GetActorRotation().Yaw + (Lato > 0.f ? 0.f : 180.f), 0.f), Luogo);
}

int32 AValdorsoLetto::PostoLibero(const APawn* Chi) const
{
	int32 Migliore = INDEX_NONE;
	float Distanza = TNumericLimits<float>::Max();
	for (int32 i = 0; i < Posti.Num(); ++i)
	{
		if (Occupanti.IsValidIndex(i) && Occupanti[i].IsValid())
		{
			continue;
		}
		const float D = FVector::Dist2D(GetActorTransform().TransformPosition(FVector(Posti[i], 0.f, 0.f)), Chi->GetActorLocation());
		if (D < Distanza)
		{
			Distanza = D;
			Migliore = i;
		}
	}
	return Migliore;
}

void AValdorsoLetto::Libera(const APawn* Chi)
{
	for (TWeakObjectPtr<APawn>& Occupante : Occupanti)
	{
		if (Occupante.Get() == Chi)
		{
			Occupante.Reset();
		}
	}
}

FVector AValdorsoLetto::PuntoInterazione() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 50.f);
}

FText AValdorsoLetto::TestoInterazione(const APawn* Chi) const
{
	const UValdorsoSonnoComponent* Componente = Chi ? Chi->FindComponentByClass<UValdorsoSonnoComponent>() : nullptr;
	if (!Componente)
	{
		return FText::GetEmpty();
	}
	if (Componente->Occupato())
	{
		return Componente->Dorme() && Componente->GetLetto() == this ? LOCTEXT("Alzati", "Alzati") : FText::GetEmpty();
	}
	return LOCTEXT("Dormi", "Dormi");
}

void AValdorsoLetto::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	UValdorsoSonnoComponent* Componente = Chi->FindComponentByClass<UValdorsoSonnoComponent>();
	if (!Componente)
	{
		return;
	}
	if (Componente->Occupato())
	{
		if (Componente->Dorme() && Componente->GetLetto() == this)
		{
			Componente->Alzati();
		}
		return;
	}
	Occupanti.SetNum(Posti.Num());
	const int32 Posto = PostoLibero(Chi);
	if (Posto != INDEX_NONE)
	{
		Componente->Corica(this, Posto);
	}
}

#undef LOCTEXT_NAMESPACE
