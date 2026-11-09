// Valdorso - Le armi in mano (v0.1.4 passo 4.5, provvisorio, 09/11/2026). Scritto da Claude. Vedi ValdorsoArmi.h.

#include "ValdorsoArmi.h"
#include "Valdorso.h"
#include "ValdorsoAspettoComponent.h"
#include "ValdorsoOggetti.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// Come stanno in mano: valori provvisori (il manichino di Epic: hand_r con la X verso le dita). Si sistemano in gioco
	// con Valdorso.Arma.Presa; poi Claude scrive qui quelli giusti.
	FVector ArmiPosDestra(9.f, 3.f, -1.f);
	FRotator ArmiRotDestra(0.f, 0.f, 90.f);
	FVector ArmiPosSinistra(-8.f, -4.f, 1.f);
	FRotator ArmiRotSinistra(0.f, 90.f, 0.f);

	void ArmiRimettiTutte(UWorld* Mondo)
	{
		if (!Mondo)
		{
			return;
		}
		for (TActorIterator<ACharacter> It(Mondo); It; ++It)
		{
			if (UValdorsoArmiComponent* Armi = UValdorsoArmiComponent::Di(*It))
			{
				Armi->Mostra();
			}
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoArmaPresa(
		TEXT("Valdorso.Arma.Presa"),
		TEXT("Valdorso.Arma.Presa destra|sinistra <x> <y> <z> <pitch> <yaw> <roll>: come sta l'arma nella mano (cm e gradi, rispetto all'osso). Senza numeri: scrive quelli di adesso."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const bool bSinistra = Argomenti.Num() > 0 && Argomenti[0].StartsWith(TEXT("s"), ESearchCase::IgnoreCase);
			FVector& Pos = bSinistra ? ArmiPosSinistra : ArmiPosDestra;
			FRotator& Rot = bSinistra ? ArmiRotSinistra : ArmiRotDestra;
			if (Argomenti.Num() >= 7)
			{
				Pos = FVector(FCString::Atof(*Argomenti[1]), FCString::Atof(*Argomenti[2]), FCString::Atof(*Argomenti[3]));
				Rot = FRotator(FCString::Atof(*Argomenti[4]), FCString::Atof(*Argomenti[5]), FCString::Atof(*Argomenti[6]));
				ArmiRimettiTutte(Mondo);
			}
			UE_LOG(LogValdorso, Display, TEXT("Valdorso.Arma.Presa %s %.1f %.1f %.1f %.1f %.1f %.1f  (da mandare a Claude)"),
				bSinistra ? TEXT("sinistra") : TEXT("destra"), Pos.X, Pos.Y, Pos.Z, Rot.Pitch, Rot.Yaw, Rot.Roll);
		}));

	// (09/11) Per trovare in fretta il verso giusto: sei direzioni della lama (o della faccia dello scudo) e quattro giri.
	FAutoConsoleCommandWithWorldAndArgs ComandoArmaProva(
		TEXT("Valdorso.Arma.Prova"),
		TEXT("Valdorso.Arma.Prova destra|sinistra <1-6> [giro 0-3]: prova una delle sei direzioni dell'arma nella mano (e quanto e' girata)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const bool bSinistra = Argomenti.Num() > 0 && Argomenti[0].StartsWith(TEXT("s"), ESearchCase::IgnoreCase);
			const int32 Quale = Argomenti.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Argomenti[1]), 1, 6) : 1;
			const int32 Giro = Argomenti.Num() > 2 ? FMath::Clamp(FCString::Atoi(*Argomenti[2]), 0, 3) : 0;
			static const FRotator Direzioni[6] = { FRotator(0.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), FRotator(0.f, 90.f, 0.f),
				FRotator(0.f, -90.f, 0.f), FRotator(90.f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f) };
			FRotator& Rot = bSinistra ? ArmiRotSinistra : ArmiRotDestra;
			// prima il giro intorno alla lama, poi la direzione
			Rot = (FQuat(Direzioni[Quale - 1]) * FQuat(FRotator(0.f, 0.f, 90.f * Giro))).Rotator();
			ArmiRimettiTutte(Mondo);
			const FVector& Pos = bSinistra ? ArmiPosSinistra : ArmiPosDestra;
			UE_LOG(LogValdorso, Display, TEXT("Valdorso.Arma.Presa %s %.1f %.1f %.1f %.1f %.1f %.1f  (prova %d, giro %d)"),
				bSinistra ? TEXT("sinistra") : TEXT("destra"), Pos.X, Pos.Y, Pos.Z, Rot.Pitch, Rot.Yaw, Rot.Roll, Quale, Giro);
		}));
}

namespace ValdorsoArmi
{
	FString PercorsoModello(FName Id)
	{
		const FString Nome = FString::Printf(TEXT("SM_Arma_%s"), *Id.ToString());
		return FString::Printf(TEXT("/Game/Valle/Pericoli/Armi/%s.%s"), *Nome, *Nome);
	}

	FTransform Presa(bool bSinistra)
	{
		return bSinistra ? FTransform(ArmiRotSinistra, ArmiPosSinistra) : FTransform(ArmiRotDestra, ArmiPosDestra);
	}

	FName Osso(bool bSinistra)
	{
		return bSinistra ? FName(TEXT("hand_l")) : FName(TEXT("hand_r"));
	}
}

USkeletalMeshComponent* UValdorsoArmiComponent::Mano() const
{
	// (09/11) le mani che si vedono: quelle del corpo MetaHuman (ha altre misure del manichino), se c'e'
	const ACharacter* Corpo = Cast<ACharacter>(GetOwner());
	if (const UValdorsoAspettoComponent* Aspetto = GetOwner() ? GetOwner()->FindComponentByClass<UValdorsoAspettoComponent>() : nullptr)
	{
		USkeletalMeshComponent* Visibile = Aspetto->GetCorpoVisibile();
		if (Visibile && Visibile->DoesSocketExist(ValdorsoArmi::Osso(false)))
		{
			return Visibile;
		}
	}
	return Corpo ? Corpo->GetMesh() : nullptr;
}

void UValdorsoArmiComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// il corpo MetaHuman arriva (o cambia) dopo: le armi passano nelle sue mani
	USkeletalMeshComponent* Dove = Mano();
	const bool bSpostare = (InDestra && InDestra->IsVisible() && InDestra->GetAttachParent() != Dove)
		|| (InSinistra && InSinistra->IsVisible() && InSinistra->GetAttachParent() != Dove);
	if (bSpostare)
	{
		Mostra();
	}
}

UValdorsoArmiComponent::UValdorsoArmiComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;
	SetIsReplicatedByDefault(true);
}

UValdorsoArmiComponent* UValdorsoArmiComponent::Di(const AActor* Chi)
{
	return Chi ? Chi->FindComponentByClass<UValdorsoArmiComponent>() : nullptr;
}

void UValdorsoArmiComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UValdorsoArmiComponent, Destra);
	DOREPLIFETIME(UValdorsoArmiComponent, Sinistra);
}

void UValdorsoArmiComponent::BeginPlay()
{
	Super::BeginPlay();
	Mostra();
}

void UValdorsoArmiComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (TObjectPtr<UStaticMeshComponent>* Parte : { &InDestra, &InSinistra })
	{
		if (*Parte)
		{
			(*Parte)->DestroyComponent();
			*Parte = nullptr;
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UValdorsoArmiComponent::Imposta(FName NuovaDestra, FName NuovaSinistra)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || (NuovaDestra == Destra && NuovaSinistra == Sinistra))
	{
		return;
	}
	Destra = NuovaDestra;
	Sinistra = NuovaSinistra;
	// sul server (o giocando da soli) OnRep non arriva: si mostra subito
	Mostra();
}

const FValdorsoTipoOggetto* UValdorsoArmiComponent::Arma() const
{
	const FValdorsoTipoOggetto* Tipo = Destra.IsNone() ? nullptr : ValdorsoOggetti::Trova(Destra);
	return Tipo && Tipo->Posto == EValdorsoPosto::ManoDestra ? Tipo : nullptr;
}

const FValdorsoTipoOggetto* UValdorsoArmiComponent::Scudo() const
{
	const FValdorsoTipoOggetto* Tipo = Sinistra.IsNone() ? nullptr : ValdorsoOggetti::Trova(Sinistra);
	return Tipo && Tipo->Posto == EValdorsoPosto::ManoSinistra ? Tipo : nullptr;
}

void UValdorsoArmiComponent::OnRep_Armi()
{
	Mostra();
}

void UValdorsoArmiComponent::Mostra()
{
	// un server senza finestra non disegna niente
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	InDestra = Pezzo(InDestra, Arma() ? Destra : NAME_None, false);
	InSinistra = Pezzo(InSinistra, Scudo() ? Sinistra : NAME_None, true);
}

UStaticMeshComponent* UValdorsoArmiComponent::Pezzo(UStaticMeshComponent* Attuale, FName Id, bool bSinistra)
{
	USkeletalMeshComponent* Manichino = Mano();
	UStaticMesh* Modello = nullptr;
	if (!Id.IsNone() && Manichino)
	{
		const FString Percorso = ValdorsoArmi::PercorsoModello(Id);
		Modello = LoadObject<UStaticMesh>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Modello)
		{
			// (09/11) l'importazione dei .glb mette il modello in una sottocartella: Armi/SM_Arma_X/StaticMeshes/SM_Arma_X
			const FString Nome = FString::Printf(TEXT("SM_Arma_%s"), *Id.ToString());
			const FString Dentro = FString::Printf(TEXT("/Game/Valle/Pericoli/Armi/%s/StaticMeshes/%s.%s"), *Nome, *Nome, *Nome);
			Modello = LoadObject<UStaticMesh>(nullptr, *Dentro, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		if (!Modello)
		{
			UE_LOG(LogValdorso, Warning, TEXT("Armi: manca il modello %s (Menu Valdorso -> Pericoli: importa i pericoli)."), *Percorso);
		}
	}
	if (!Modello)
	{
		if (Attuale)
		{
			Attuale->SetVisibility(false);
			Attuale->SetStaticMesh(nullptr);
		}
		return Attuale;
	}
	if (!Attuale)
	{
		Attuale = NewObject<UStaticMeshComponent>(GetOwner());
		Attuale->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Attuale->SetGenerateOverlapEvents(false);
		Attuale->SetCanEverAffectNavigation(false);
		Attuale->SetCastShadow(true);
		Attuale->SetIsReplicated(false);
		Attuale->RegisterComponent();
	}
	Attuale->SetStaticMesh(Modello);
	Attuale->AttachToComponent(Manichino, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ValdorsoArmi::Osso(bSinistra));
	Attuale->SetRelativeTransform(ValdorsoArmi::Presa(bSinistra));
	Attuale->SetVisibility(true);
	return Attuale;
}
