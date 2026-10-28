// Valdorso - L'aspetto del colono nel mondo. Scritto da Claude il 28/10/2026. Vedi ValdorsoAspettoComponent.h.

#include "ValdorsoAspettoComponent.h"
#include "Valdorso.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

UValdorsoAspettoComponent::UValdorsoAspettoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Di serie la testa di prova e la sua animazione del corpo (si cambiano nel Blueprint del personaggio).
	// Sono riferimenti "morbidi": il C++ non carica niente finché il gioco non parte.
	ClasseMetaHuman = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/MetaHumans/MH_Prova/BP_MH_Prova.BP_MH_Prova_C")));
	AnimazioneCorpo = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/MetaHumans/MH_Prova/ABP_CorpoSegue.ABP_CorpoSegue_C")));
}

void UValdorsoAspettoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UValdorsoAspettoComponent, Aspetto);
}

void UValdorsoAspettoComponent::BeginPlay()
{
	Super::BeginPlay();

	// Il server dedicato non disegna niente: niente corpo MetaHuman.
	if (GetNetMode() == NM_DedicatedServer || !bUsaMetaHuman)
	{
		return;
	}
	CreaCorpo();
}

void UValdorsoAspettoComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CorpoMetaHuman)
	{
		CorpoMetaHuman->Destroy();
		CorpoMetaHuman = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UValdorsoAspettoComponent::CreaCorpo()
{
	ACharacter* Personaggio = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Manichino = Personaggio ? Personaggio->GetMesh() : nullptr;
	UWorld* Mondo = GetWorld();
	if (!Manichino || !Mondo)
	{
		return;
	}

	UClass* Classe = ClasseMetaHuman.LoadSynchronous();
	if (!Classe)
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Aspetto: non trovo il MetaHuman %s, resta il manichino."), *ClasseMetaHuman.ToString());
		return;
	}
	UClass* Animazione = AnimazioneCorpo.LoadSynchronous();
	if (!Animazione)
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Aspetto: non trovo l'animazione del corpo %s, resta il manichino."), *AnimazioneCorpo.ToString());
		return;
	}

	FActorSpawnParameters Parametri;
	Parametri.Owner = Personaggio;
	Parametri.ObjectFlags |= RF_Transient;
	Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CorpoMetaHuman = Mondo->SpawnActor<AActor>(Classe, Manichino->GetComponentTransform(), Parametri);
	if (!CorpoMetaHuman)
	{
		return;
	}
	CorpoMetaHuman->SetActorEnableCollision(false);
	CorpoMetaHuman->AttachToActor(Personaggio, FAttachmentTransformRules::KeepWorldTransform);

	USkeletalMeshComponent* Corpo = nullptr;
	TInlineComponentArray<USkeletalMeshComponent*> Pezzi(CorpoMetaHuman);
	for (USkeletalMeshComponent* Pezzo : Pezzi)
	{
		if (Pezzo->GetFName() == TEXT("Body"))
		{
			Corpo = Pezzo;
		}
		else if (Pezzo->GetFName() == TEXT("Face"))
		{
			Volto = Pezzo;
		}
	}
	if (!Corpo)
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Aspetto: %s non ha il componente Body, resta il manichino."), *Classe->GetName());
		CorpoMetaHuman->Destroy();
		CorpoMetaHuman = nullptr;
		return;
	}

	// Il corpo si attacca direttamente al manichino: così Copy Pose From Mesh (Use Attached Parent) copia la sua posa.
	Corpo->AttachToComponent(Manichino, FAttachmentTransformRules::SnapToTargetIncludingScale);
	Corpo->SetAnimInstanceClass(Animazione);
	Corpo->AddTickPrerequisiteComponent(Manichino);

	// Il manichino sparisce ma continua ad animarsi (le ossa servono al corpo MetaHuman e alle abilità).
	Manichino->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Manichino->SetVisibility(false, false);

	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Aspetto: corpo %s sopra il manichino di %s."), *Classe->GetName(), *Personaggio->GetName());

	// L'animazione del volto si prepara al primo aggiornamento: i cursori si scrivono subito e al giro dopo.
	ApplicaVolto();
	Mondo->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UValdorsoAspettoComponent::ApplicaVolto));
}

void UValdorsoAspettoComponent::ImpostaAspetto(const FValdorsoAspetto& Nuovo)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Aspetto = Nuovo;
	// Sul server con giocatore (gioco locale, prova nell'editor) OnRep non arriva da sé.
	ApplicaVolto();
}

void UValdorsoAspettoComponent::OnRep_Aspetto()
{
	ApplicaVolto();
}

void UValdorsoAspettoComponent::ProvaCursore(FName Id, float Valore)
{
	CursoriProva.Add(Id, FMath::Clamp(Valore, -1.f, 1.f));
	ApplicaVolto();
}

void UValdorsoAspettoComponent::ApplicaVolto()
{
	USkeletalMeshComponent* Faccia = Volto.Get();
	UAnimInstance* Animazione = Faccia ? Faccia->GetPostProcessInstance() : nullptr;
	if (!Animazione)
	{
		return;
	}

	int32 Scritti = 0;
	for (const FValdorsoVoceAspetto& Voce : ValdorsoAspetto::Voci())
	{
		if (Voce.Gruppo != EValdorsoGruppoAspetto::Volto || !Voce.bCursore)
		{
			continue;
		}
		const FName Nome(Voce.Id);
		const float* Prova = CursoriProva.Find(Nome);
		const float Valore = Prova ? *Prova : ValdorsoAspetto::Cursore(Aspetto, Voce.Id);

		const FProperty* Variabile = Animazione->GetClass()->FindPropertyByName(Nome);
		if (const FDoubleProperty* Doppio = CastField<FDoubleProperty>(Variabile))
		{
			Doppio->SetPropertyValue_InContainer(Animazione, Valore);
			++Scritti;
		}
		else if (const FFloatProperty* Singolo = CastField<FFloatProperty>(Variabile))
		{
			Singolo->SetPropertyValue_InContainer(Animazione, Valore);
			++Scritti;
		}
	}
	UE_LOG(LogValdorso, Verbose, TEXT("[Valdorso] Aspetto: %d cursori del volto applicati."), Scritti);
}
