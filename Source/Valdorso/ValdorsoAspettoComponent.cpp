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

namespace
{
	TSoftClassPtr<AActor> Testa(const TCHAR* Percorso)
	{
		return TSoftClassPtr<AActor>(FSoftObjectPath(Percorso));
	}
}

UValdorsoAspettoComponent::UValdorsoAspettoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Di serie le teste di prova (si cambiano e si aggiungono nel Blueprint del personaggio).
	// Sono riferimenti "morbidi": il C++ non carica niente finché non serve. Le teste che mancano su un PC
	// si saltano: al loro posto c'è la testa di riserva.
	ClasseMetaHuman = Testa(TEXT("/Game/MetaHumans/MH_Prova/BP_MH_Prova.BP_MH_Prova_C"));
	TesteUomo.Add(ClasseMetaHuman);
	TesteUomo.Add(Testa(TEXT("/Game/Unpacked/MH_Walter/BP_MH_Walter.BP_MH_Walter_C")));
	TesteDonna.Add(Testa(TEXT("/Game/Unpacked/MH_Jelena/BP_MH_Jelena.BP_MH_Jelena_C")));
	AnimazioneCorpo = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/MetaHumans/MH_Prova/ABP_CorpoSegue.ABP_CorpoSegue_C")));
}

void UValdorsoAspettoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UValdorsoAspettoComponent, Aspetto);
	DOREPLIFETIME(UValdorsoAspettoComponent, bDonna);
}

void UValdorsoAspettoComponent::BeginPlay()
{
	Super::BeginPlay();
	bIniziato = true;
	Aggiorna();
}

void UValdorsoAspettoComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TogliCorpo();
	bIniziato = false;
	Super::EndPlay(EndPlayReason);
}

TSoftClassPtr<AActor> UValdorsoAspettoComponent::TestaDaUsare() const
{
	if (!TestaProva.IsNull())
	{
		return TestaProva;
	}
	const TArray<TSoftClassPtr<AActor>>& Teste = bDonna ? TesteDonna : TesteUomo;
	if (Teste.Num() > 0)
	{
		const int32 Scelta = ValdorsoAspetto::Valore(Aspetto, TEXT("Testa")) % Teste.Num();
		if (!Teste[Scelta].IsNull())
		{
			return Teste[Scelta];
		}
	}
	return ClasseMetaHuman;
}

void UValdorsoAspettoComponent::Aggiorna()
{
	// Il server dedicato non disegna niente; e prima di BeginPlay il personaggio non è pronto.
	if (!bIniziato || GetNetMode() == NM_DedicatedServer || !bUsaMetaHuman)
	{
		return;
	}
	const TSoftClassPtr<AActor> Voluta = TestaDaUsare();
	if (!CorpoMetaHuman || Voluta != TestaAttuale)
	{
		TogliCorpo();
		CreaCorpo(Voluta);
	}
	ApplicaVolto();
}

void UValdorsoAspettoComponent::TogliCorpo()
{
	if (CorpoMetaHuman)
	{
		// Il corpo era stato spostato sul manichino: si distrugge a parte, poi il resto del MetaHuman.
		TInlineComponentArray<USkeletalMeshComponent*> Pezzi(CorpoMetaHuman);
		for (USkeletalMeshComponent* Pezzo : Pezzi)
		{
			Pezzo->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		}
		CorpoMetaHuman->Destroy();
		CorpoMetaHuman = nullptr;
	}
	TestaAttuale.Reset();
	Volto.Reset();

	ACharacter* Personaggio = Cast<ACharacter>(GetOwner());
	if (USkeletalMeshComponent* Manichino = Personaggio ? Personaggio->GetMesh() : nullptr)
	{
		Manichino->SetVisibility(true, false);
	}
}

void UValdorsoAspettoComponent::CreaCorpo(const TSoftClassPtr<AActor>& Voluta)
{
	ACharacter* Personaggio = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Manichino = Personaggio ? Personaggio->GetMesh() : nullptr;
	UWorld* Mondo = GetWorld();
	if (!Manichino || !Mondo)
	{
		return;
	}

	TSoftClassPtr<AActor> Usata = Voluta;
	UClass* Classe = Usata.LoadSynchronous();
	if (!Classe && Voluta != ClasseMetaHuman)
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Aspetto: non trovo la testa %s, uso quella di riserva."), *Voluta.ToString());
		Usata = ClasseMetaHuman;
		Classe = Usata.LoadSynchronous();
	}
	if (!Classe)
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Aspetto: non trovo nessun MetaHuman, resta il manichino."));
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
		Volto.Reset();
		return;
	}

	// Il corpo si attacca direttamente al manichino: così Copy Pose From Mesh (Use Attached Parent) copia la sua posa.
	Corpo->AttachToComponent(Manichino, FAttachmentTransformRules::SnapToTargetIncludingScale);
	Corpo->SetAnimInstanceClass(Animazione);
	Corpo->AddTickPrerequisiteComponent(Manichino);
	TestaAttuale = Voluta;

	// Il manichino sparisce ma continua ad animarsi (le ossa servono al corpo MetaHuman e alle abilità).
	Manichino->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Manichino->SetVisibility(false, false);

	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Aspetto: corpo %s sopra il manichino di %s."), *Classe->GetName(), *Personaggio->GetName());

	// L'animazione del volto si prepara al primo aggiornamento: i cursori si scrivono anche al giro dopo.
	Mondo->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UValdorsoAspettoComponent::ApplicaVolto));
}

void UValdorsoAspettoComponent::ImpostaAspetto(const FValdorsoAspetto& Nuovo, const FString& Sesso)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Aspetto = Nuovo;
	bDonna = (Sesso == TEXT("donna"));
	// Sul server con giocatore (gioco locale, prova nell'editor) OnRep non arriva da sé.
	Aggiorna();
}

void UValdorsoAspettoComponent::OnRep_Aspetto()
{
	Aggiorna();
}

void UValdorsoAspettoComponent::ProvaCursore(FName Id, float Valore)
{
	CursoriProva.Add(Id, FMath::Clamp(Valore, -1.f, 1.f));
	ApplicaVolto();
}

void UValdorsoAspettoComponent::ProvaTesta(int32 Numero, bool bDiDonna)
{
	const TArray<TSoftClassPtr<AActor>>& Teste = bDiDonna ? TesteDonna : TesteUomo;
	if (!Teste.IsValidIndex(Numero))
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Aspetto: non c'è la testa %d (%s): ce ne sono %d."),
			Numero, bDiDonna ? TEXT("donna") : TEXT("uomo"), Teste.Num());
		return;
	}
	TestaProva = Teste[Numero];
	Aggiorna();
}

void UValdorsoAspettoComponent::ApplicaVolto()
{
	USkeletalMeshComponent* Faccia = Volto.Get();
	UAnimInstance* Animazione = Faccia ? Faccia->GetPostProcessInstance() : nullptr;
	if (!Animazione)
	{
		return;
	}

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
		}
		else if (const FFloatProperty* Singolo = CastField<FFloatProperty>(Variabile))
		{
			Singolo->SetPropertyValue_InContainer(Animazione, Valore);
		}
	}
}
