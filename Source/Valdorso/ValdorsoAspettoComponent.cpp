// Valdorso - L'aspetto del colono nel mondo. Scritto da Claude il 28/10/2026. Vedi ValdorsoAspettoComponent.h.

#include "ValdorsoAspettoComponent.h"
#include "Valdorso.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GroomComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
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

	// (29/10) I colori dei capelli, nell'ordine delle scelte della voce "ColoreCapelli" (12). Sono i parametri del
	// materiale dei capelli MetaHuman: melanina (0 biondo chiaro, 1 nero), rossore e quanto bianco (i capelli grigi).
	// La prima scelta lascia il colore che la testa ha già.
	struct FColoreCapelli
	{
		const TCHAR* Nome;
		float Melanina;
		float Rossore;
		float Bianco;
	};
	const FColoreCapelli ColoriCapelli[] = {
		{ TEXT("Come nasce"), 0.f, 0.f, 0.f },
		{ TEXT("Nero corvino"), 0.95f, 0.10f, 0.f },
		{ TEXT("Castano scuro"), 0.80f, 0.20f, 0.f },
		{ TEXT("Castano"), 0.62f, 0.25f, 0.f },
		{ TEXT("Castano chiaro"), 0.47f, 0.30f, 0.f },
		{ TEXT("Biondo cenere"), 0.30f, 0.12f, 0.f },
		{ TEXT("Biondo dorato"), 0.22f, 0.35f, 0.f },
		{ TEXT("Biondo chiaro"), 0.10f, 0.18f, 0.f },
		{ TEXT("Rosso fuoco"), 0.28f, 0.90f, 0.f },
		{ TEXT("Rame scuro"), 0.50f, 0.70f, 0.f },
		{ TEXT("Brizzolato"), 0.75f, 0.20f, 0.45f },
		{ TEXT("Bianco"), 0.30f, 0.05f, 0.95f },
	};
	constexpr int32 ColoriCapelliNumero = static_cast<int32>(UE_ARRAY_COUNT(ColoriCapelli));

	bool PeloColorabile(const UActorComponent* Pezzo)
	{
		const FString Nome = Pezzo->GetName();
		return Nome.StartsWith(TEXT("Hair")) || Nome.StartsWith(TEXT("Eyebrows")) || Nome.StartsWith(TEXT("Beard"))
			|| Nome.StartsWith(TEXT("Mustache"));
	}
}

FString UValdorsoAspettoComponent::NomeColoreCapelli(int32 Scelta)
{
	return ColoriCapelli[FMath::Abs(Scelta) % ColoriCapelliNumero].Nome;
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
	// (28/10) Le teste montate da crea_teste_valdorso.py. Nuove teste: in fondo, per non cambiare quelle già scelte.
	TesteUomo.Add(Testa(TEXT("/Game/Unpacked/MH_Lorenzo/BP_MH_Lorenzo.BP_MH_Lorenzo_C")));
	TesteUomo.Add(Testa(TEXT("/Game/Unpacked/MH_Dominic/BP_MH_Dominic.BP_MH_Dominic_C")));
	TesteUomo.Add(Testa(TEXT("/Game/Unpacked/MH_Mikel/BP_MH_Mikel.BP_MH_Mikel_C")));
	TesteUomo.Add(Testa(TEXT("/Game/Unpacked/MH_Omari/BP_MH_Omari.BP_MH_Omari_C")));
	TesteDonna.Add(Testa(TEXT("/Game/Unpacked/MH_Jelena/BP_MH_Jelena.BP_MH_Jelena_C")));
	TesteDonna.Add(Testa(TEXT("/Game/Unpacked/MH_Grace/BP_MH_Grace.BP_MH_Grace_C")));
	TesteDonna.Add(Testa(TEXT("/Game/Unpacked/MH_Etta/BP_MH_Etta.BP_MH_Etta_C")));
	TesteDonna.Add(Testa(TEXT("/Game/Unpacked/MH_Celeste/BP_MH_Celeste.BP_MH_Celeste_C")));
	TesteDonna.Add(Testa(TEXT("/Game/Unpacked/MH_Sunita/BP_MH_Sunita.BP_MH_Sunita_C")));
	AnimazioneVolto = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/MetaHumans/Common/Face/ABP_Face_PostProcess.ABP_Face_PostProcess_C")));
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

USkeletalMeshComponent* UValdorsoAspettoComponent::GetManichino() const
{
	if (Supporto.IsValid())
	{
		return Supporto.Get();
	}
	const ACharacter* Personaggio = Cast<ACharacter>(GetOwner());
	return Personaggio ? Personaggio->GetMesh() : nullptr;
}

void UValdorsoAspettoComponent::UsaSupporto(USkeletalMeshComponent* Manichino, USceneCaptureComponent2D* Ritratto)
{
	TogliCorpo();
	Supporto = Manichino;
	SoloRitratto = Ritratto;
	Aggiorna();
}

int32 UValdorsoAspettoComponent::TesteDisponibili(bool bPerDonna)
{
	const UValdorsoAspettoComponent* Serie = GetDefault<UValdorsoAspettoComponent>();
	return FMath::Max(1, (bPerDonna ? Serie->TesteDonna : Serie->TesteUomo).Num());
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
	// Si rifà solo se la testa è cambiata (anche se l'ultima volta non si è potuta caricare: niente nuovi tentativi
	// a ogni movimento di un cursore).
	if (Voluta != TestaAttuale || (!CorpoMetaHuman && TestaAttuale.IsNull()))
	{
		TogliCorpo();
		CreaCorpo(Voluta);
		ColoreCapelliMesso = INDEX_NONE;
	}
	ApplicaVolto();
	ApplicaColori();
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
		if (USceneCaptureComponent2D* Ritratto = SoloRitratto.Get())
		{
			Ritratto->ShowOnlyActors.Remove(CorpoMetaHuman);
		}
		CorpoMetaHuman->Destroy();
		CorpoMetaHuman = nullptr;
	}
	TestaAttuale.Reset();
	Volto.Reset();

	if (USkeletalMeshComponent* Manichino = GetManichino())
	{
		Manichino->SetVisibility(true, false);
	}
}

void UValdorsoAspettoComponent::CreaCorpo(const TSoftClassPtr<AActor>& Voluta)
{
	// Ricordata anche se non riesce, così non si riprova a ogni aggiornamento.
	TestaAttuale = Voluta;
	AActor* Personaggio = GetOwner();
	USkeletalMeshComponent* Manichino = GetManichino();
	UWorld* Mondo = GetWorld();
	if (!Personaggio || !Manichino || !Mondo)
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

	// Il volto usa sempre la nostra animazione del volto, con il nodo dei cursori.
	if (USkeletalMeshComponent* Faccia = Volto.Get())
	{
		if (UClass* AnimVolto = AnimazioneVolto.LoadSynchronous())
		{
			Faccia->SetOverridePostProcessAnimBP(AnimVolto);
		}
		else
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Aspetto: non trovo l'animazione del volto %s (i cursori non si vedranno)."), *AnimazioneVolto.ToString());
		}
	}

	// Il corpo si attacca direttamente al manichino: così Copy Pose From Mesh (Use Attached Parent) copia la sua posa.
	Corpo->AttachToComponent(Manichino, FAttachmentTransformRules::SnapToTargetIncludingScale);
	Corpo->SetAnimInstanceClass(Animazione);
	Corpo->AddTickPrerequisiteComponent(Manichino);
	// Nel palco del Registro il MetaHuman lo vede solo la telecamera del ritratto.
	if (USceneCaptureComponent2D* Ritratto = SoloRitratto.Get())
	{
		TInlineComponentArray<UPrimitiveComponent*> Forme(CorpoMetaHuman);
		for (UPrimitiveComponent* Forma : Forme)
		{
			// (Non "visibile solo alla telecamera": così i capelli e il volto non comparivano. Il palco è lontano
			// sopra la mappa: la telecamera del gioco non lo vede comunque.)
			// Per il primo piano: sempre il dettaglio più alto, e sempre animato (anche se lo vede solo il ritratto).
			if (USkeletalMeshComponent* Pelle = Cast<USkeletalMeshComponent>(Forma))
			{
				Pelle->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
				Pelle->SetForcedLOD(1);
			}
			// I capelli nel ritratto: le ciocche a schede (LOD 1), più sicure nella telecamera del palco.
			if (UGroomComponent* Capelli = Cast<UGroomComponent>(Forma))
			{
				Capelli->SetForcedLOD(1);
			}
		}
		// LODSync cambierebbe da solo il dettaglio di tutti i pezzi: nel ritratto lo si spegne, valgono i LOD qui sopra.
		TArray<UActorComponent*> Altri;
		CorpoMetaHuman->GetComponents(Altri);
		for (UActorComponent* Pezzo : Altri)
		{
			if (Pezzo && Pezzo->GetClass()->GetName().Contains(TEXT("LODSync")))
			{
				Pezzo->SetComponentTickEnabled(false);
			}
		}
		Ritratto->ShowOnlyActors.AddUnique(CorpoMetaHuman);
	}

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
	ApplicaColori();
}

void UValdorsoAspettoComponent::ProvaTesta(int32 Numero, bool bDiDonna)
{
	if (Numero < 0)
	{
		TestaProva.Reset();
		Aggiorna();
		return;
	}
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

void UValdorsoAspettoComponent::ApplicaColori()
{
	if (!CorpoMetaHuman)
	{
		return;
	}

	// La carnagione: più scura o più chiara della pelle della testa (la luminosità della pelle "cotta" dal Creator).
	// Va sul volto e sul corpo insieme, così il collo non si vede.
	const float* ProvaPelle = CursoriProva.Find(TEXT("Carnagione"));
	const float Pelle = ProvaPelle ? *ProvaPelle : ValdorsoAspetto::Cursore(Aspetto, TEXT("Carnagione"));
	const float Luce = Pelle < 0.f ? 1.f + 0.55f * Pelle : 1.f + 0.30f * Pelle;
	const float Colore = Pelle < 0.f ? -0.12f * Pelle : 0.f;
	TInlineComponentArray<USkeletalMeshComponent*> Pezzi(CorpoMetaHuman);
	for (USkeletalMeshComponent* Pezzo : Pezzi)
	{
		if (Pezzo->GetFName() == TEXT("Face") || Pezzo->GetFName() == TEXT("Body"))
		{
			Pezzo->SetScalarParameterValueOnMaterials(TEXT("Basecolor Global Value Post-Bake"), Luce);
			Pezzo->SetScalarParameterValueOnMaterials(TEXT("Basecolor Global Saturation Post-Bake"), Colore);
		}
	}

	// Il colore dei capelli (anche sopracciglia, barba e baffi): si cambia solo quando cambia la scelta.
	const int32 Scelta = ValdorsoAspetto::Valore(Aspetto, TEXT("ColoreCapelli")) % ColoriCapelliNumero;
	if (Scelta == ColoreCapelliMesso)
	{
		return;
	}
	ColoreCapelliMesso = Scelta;
	const FColoreCapelli& Tinta = ColoriCapelli[Scelta];
	TInlineComponentArray<UGroomComponent*> Peli(CorpoMetaHuman);
	for (UGroomComponent* Pelo : Peli)
	{
		if (!PeloColorabile(Pelo))
		{
			continue;
		}
		if (Scelta == 0)
		{
			// Come nasce: si tolgono i colori messi prima (torna quello del materiale della testa).
			for (int32 i = 0; i < Pelo->GetNumMaterials(); ++i)
			{
				if (UMaterialInstanceDynamic* Dinamico = Cast<UMaterialInstanceDynamic>(Pelo->GetMaterial(i)))
				{
					Dinamico->ClearParameterValues();
				}
			}
			continue;
		}
		Pelo->SetScalarParameterValueOnMaterials(TEXT("hairMelanin"), Tinta.Melanina);
		Pelo->SetScalarParameterValueOnMaterials(TEXT("hairRedness"), Tinta.Rossore);
		Pelo->SetScalarParameterValueOnMaterials(TEXT("WhiteAmount"), Tinta.Bianco);
		// Niente tinta artificiale (qualche testa del Creator ha i capelli tinti).
		Pelo->SetVectorParameterValueOnMaterials(TEXT("hairDye"), FVector(1.f, 1.f, 1.f));
	}
}
