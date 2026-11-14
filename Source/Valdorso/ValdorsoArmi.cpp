// Valdorso - Le armi in mano (v0.1.4 passo 4.5, provvisorio, 09/11/2026). Scritto da Claude. Vedi ValdorsoArmi.h.

#include "ValdorsoArmi.h"
#include "Valdorso.h"
#include "ValdorsoAspettoComponent.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoQualita.h"
#include "Animation/AnimSequenceBase.h"
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
	// I quattro posti: le due mani, il fianco (la spada nel fodero) e la schiena (lo scudo). Valori di serie: provati da Fra
	// il 09/11 (destra: prova 5 giro 1; sinistra: prova 5); si sistemano in gioco con Valdorso.Arma.Prova / Sposta / Presa.
	// (14/11, v0.2.3) il quinto: l'arco, nella sinistra
	const TCHAR* const ArmiNomiPosti[5] = { TEXT("destra"), TEXT("sinistra"), TEXT("fianco"), TEXT("schiena"), TEXT("arco") };
	const TCHAR* const ArmiOssi[5] = { TEXT("hand_r"), TEXT("hand_l"), TEXT("pelvis"), TEXT("spine_05"), TEXT("hand_l") };

	FRotator ArmiDirezione(int32 Quale, int32 Giro)
	{
		static const FRotator Direzioni[6] = { FRotator(0.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), FRotator(0.f, 90.f, 0.f),
			FRotator(0.f, -90.f, 0.f), FRotator(90.f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f) };
		// prima il giro intorno alla lama, poi la direzione
		return (FQuat(Direzioni[FMath::Clamp(Quale, 1, 6) - 1]) * FQuat(FRotator(0.f, 0.f, 90.f * FMath::Clamp(Giro, 0, 3)))).Rotator();
	}

	// (14/11) l'arco: in piedi nella sinistra (da provare in gioco con Valdorso.Arma.Prova arco <1-6> [giro])
	FVector ArmiPos[5] = { FVector(9.f, 3.f, -1.f), FVector(-8.f, -4.f, 1.f), FVector(0.f, -18.f, 12.f), FVector(-5.f, 20.f, 0.f), FVector(-8.f, -3.f, 0.f) };
	FRotator ArmiRot[5] = { ArmiDirezione(5, 1), ArmiDirezione(5, 0), ArmiDirezione(6, 0), ArmiDirezione(3, 0), ArmiDirezione(5, 1) };

	/** Il posto dal suo nome ("destra", "s", "fianco"...); -1 se non c'e'. */
	int32 ArmiPosto(const TArray<FString>& Argomenti)
	{
		if (Argomenti.Num() == 0)
		{
			return -1;
		}
		for (int32 i = 0; i < ValdorsoArmi::NumeroPosti; ++i)
		{
			if (FString(ArmiNomiPosti[i]).StartsWith(Argomenti[0], ESearchCase::IgnoreCase))
			{
				return i;
			}
		}
		return -1;
	}

	void ArmiScrivi(int32 Posto, const TCHAR* Come)
	{
		const FVector& Pos = ArmiPos[Posto];
		const FRotator& Rot = ArmiRot[Posto];
		UE_LOG(LogValdorso, Display, TEXT("Valdorso.Arma.Presa %s %.1f %.1f %.1f %.1f %.1f %.1f  (%s: da mandare a Claude)"),
			ArmiNomiPosti[Posto], Pos.X, Pos.Y, Pos.Z, Rot.Pitch, Rot.Yaw, Rot.Roll, Come);
	}

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

	const TCHAR* ArmiSpiega = TEXT(" (posti: destra, sinistra, fianco, schiena, arco)");

	// (14/11, v0.2.3) le animazioni dei colpi per famiglia: 0 = sempre il pugno (se qualcosa si vede storto)
	TAutoConsoleVariable<int32> CVarArmiAnimazioni(
		TEXT("Valdorso.Armi.Animazioni"),
		1,
		TEXT("1 = ogni famiglia d'arma colpisce con la sua animazione (Greystone); 0 = tutte con il gesto del pugno."));

	FAutoConsoleCommandWithWorldAndArgs ComandoArmaPresa(
		TEXT("Valdorso.Arma.Presa"),
		TEXT("Valdorso.Arma.Presa <posto> <x> <y> <z> <pitch> <yaw> <roll>: come sta l'arma (cm e gradi, rispetto all'osso). Senza numeri: scrive quelli di adesso. Posti: destra, sinistra, fianco, schiena."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const int32 Posto = ArmiPosto(Argomenti);
			if (Posto < 0)
			{
				UE_LOG(LogValdorso, Display, TEXT("Valdorso.Arma.Presa: quale posto?%s"), ArmiSpiega);
				return;
			}
			if (Argomenti.Num() >= 7)
			{
				ArmiPos[Posto] = FVector(FCString::Atof(*Argomenti[1]), FCString::Atof(*Argomenti[2]), FCString::Atof(*Argomenti[3]));
				ArmiRot[Posto] = FRotator(FCString::Atof(*Argomenti[4]), FCString::Atof(*Argomenti[5]), FCString::Atof(*Argomenti[6]));
				ArmiRimettiTutte(Mondo);
			}
			ArmiScrivi(Posto, TEXT("presa"));
		}));

	// (09/11) Per trovare in fretta il verso giusto: sei direzioni della lama (o della faccia dello scudo) e quattro giri.
	FAutoConsoleCommandWithWorldAndArgs ComandoArmaProva(
		TEXT("Valdorso.Arma.Prova"),
		TEXT("Valdorso.Arma.Prova <posto> <1-6> [giro 0-3]: prova una delle sei direzioni dell'arma (e quanto e' girata). Posti: destra, sinistra, fianco, schiena."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const int32 Posto = ArmiPosto(Argomenti);
			if (Posto < 0)
			{
				UE_LOG(LogValdorso, Display, TEXT("Valdorso.Arma.Prova: quale posto?%s"), ArmiSpiega);
				return;
			}
			const int32 Quale = Argomenti.Num() > 1 ? FCString::Atoi(*Argomenti[1]) : 1;
			const int32 Giro = Argomenti.Num() > 2 ? FCString::Atoi(*Argomenti[2]) : 0;
			ArmiRot[Posto] = ArmiDirezione(Quale, Giro);
			ArmiRimettiTutte(Mondo);
			ArmiScrivi(Posto, TEXT("prova"));
		}));

	// (09/11) Spostare un poco l'arma (cm, lungo gli assi dell'osso): Valdorso.Arma.Sposta destra 0 0 -3
	FAutoConsoleCommandWithWorldAndArgs ComandoArmaSposta(
		TEXT("Valdorso.Arma.Sposta"),
		TEXT("Valdorso.Arma.Sposta <posto> <dx> <dy> <dz>: sposta l'arma di qualche cm (lungo gli assi dell'osso). Posti: destra, sinistra, fianco, schiena."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const int32 Posto = ArmiPosto(Argomenti);
			if (Posto < 0 || Argomenti.Num() < 4)
			{
				UE_LOG(LogValdorso, Display, TEXT("Valdorso.Arma.Sposta <posto> <dx> <dy> <dz>%s"), ArmiSpiega);
				return;
			}
			ArmiPos[Posto] += FVector(FCString::Atof(*Argomenti[1]), FCString::Atof(*Argomenti[2]), FCString::Atof(*Argomenti[3]));
			ArmiRimettiTutte(Mondo);
			ArmiScrivi(Posto, TEXT("sposta"));
		}));
}

namespace ValdorsoArmi
{
	FString PercorsoModello(FName Id)
	{
		const FString Nome = FString::Printf(TEXT("SM_Arma_%s"), *Id.ToString());
		return FString::Printf(TEXT("/Game/Valle/Pericoli/Armi/%s.%s"), *Nome, *Nome);
	}

	FTransform Presa(int32 Posto)
	{
		const int32 Quale = FMath::Clamp(Posto, 0, NumeroPosti - 1);
		return FTransform(ArmiRot[Quale], ArmiPos[Quale]);
	}

	FName Osso(int32 Posto)
	{
		return FName(ArmiOssi[FMath::Clamp(Posto, 0, NumeroPosti - 1)]);
	}

	UAnimSequenceBase* AnimazioneColpo(FName IdArma, int32 Numero, float& OutIstante, float& OutVelocita)
	{
		OutIstante = 0.5f;
		OutVelocita = 1.f;
		if (CVarArmiAnimazioni.GetValueOnGameThread() == 0)
		{
			return nullptr;
		}
		// quale animazione, quando arriva il colpo (frazione della durata) e quanto in fretta
		const TCHAR* Nome = nullptr;
		switch (ValdorsoQualita::Famiglia(IdArma))
		{
		case EValdorsoFamigliaArma::Pugnali: Nome = TEXT("Spada_Rapido"); OutIstante = 0.45f; OutVelocita = 1.2f; break;
		case EValdorsoFamigliaArma::Spade:
			Nome = (Numero % 2) == 0 ? TEXT("Spada_Rapido") : TEXT("Spada_Medio");
			OutIstante = (Numero % 2) == 0 ? 0.45f : 0.42f;
			OutVelocita = (Numero % 2) == 0 ? 1.05f : 1.15f;
			break;
		case EValdorsoFamigliaArma::Asce: Nome = TEXT("Spada_Pesante"); OutIstante = 0.55f; OutVelocita = 0.95f; break;
		case EValdorsoFamigliaArma::Mazze: Nome = TEXT("Spada_Dall_Alto"); OutIstante = 0.55f; OutVelocita = 0.9f; break;
		case EValdorsoFamigliaArma::Aste: Nome = TEXT("Spada_Medio"); OutIstante = 0.42f; OutVelocita = 1.1f; break;
		case EValdorsoFamigliaArma::Scudo: Nome = TEXT("Spada_Parata"); OutIstante = 0.35f; OutVelocita = 1.3f; break;
		default: return nullptr;
		}
		const FString Percorso = FString::Printf(TEXT("/Game/Fab/Armi/Spada/%s.%s"), Nome, Nome);
		return LoadObject<UAnimSequenceBase>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

USkeletalMeshComponent* UValdorsoArmiComponent::Mano() const
{
	// (09/11) le mani che si vedono: quelle del corpo MetaHuman (ha altre misure del manichino), se c'e'
	const ACharacter* Corpo = Cast<ACharacter>(GetOwner());
	if (const UValdorsoAspettoComponent* Aspetto = GetOwner() ? GetOwner()->FindComponentByClass<UValdorsoAspettoComponent>() : nullptr)
	{
		USkeletalMeshComponent* Visibile = Aspetto->GetCorpoVisibile();
		if (Visibile && Visibile->DoesSocketExist(ValdorsoArmi::Osso(ValdorsoArmi::PostoDestra)))
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
	DOREPLIFETIME(UValdorsoArmiComponent, bRiposte);
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
	// prendere in mano un'arma nuova dallo zaino la sfodera
	if ((NuovaDestra != Destra && !NuovaDestra.IsNone()) || (NuovaSinistra != Sinistra && !NuovaSinistra.IsNone()))
	{
		bRiposte = false;
		bRiposteMostrate = false;
	}
	Destra = NuovaDestra;
	Sinistra = NuovaSinistra;
	// sul server (o giocando da soli) OnRep non arriva: si mostra subito
	Mostra();
}

const FValdorsoTipoOggetto* UValdorsoArmiComponent::ArmaPortata() const
{
	const FValdorsoTipoOggetto* Tipo = Destra.IsNone() ? nullptr : ValdorsoOggetti::Trova(Destra);
	return Tipo && Tipo->Posto == EValdorsoPosto::ManoDestra ? Tipo : nullptr;
}

const FValdorsoTipoOggetto* UValdorsoArmiComponent::ScudoPortato() const
{
	const FValdorsoTipoOggetto* Tipo = Sinistra.IsNone() ? nullptr : ValdorsoOggetti::Trova(Sinistra);
	return Tipo && Tipo->Posto == EValdorsoPosto::ManoSinistra ? Tipo : nullptr;
}

const FValdorsoTipoOggetto* UValdorsoArmiComponent::Arma() const
{
	return bRiposte ? nullptr : ArmaPortata();
}

const FValdorsoTipoOggetto* UValdorsoArmiComponent::Scudo() const
{
	return bRiposte ? nullptr : ScudoPortato();
}

void UValdorsoArmiComponent::ServerAlterna_Implementation()
{
	if (!ArmaPortata() && !ScudoPortato())
	{
		return;
	}
	const UWorld* Mondo = GetWorld();
	const double OraMondo = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	if (OraMondo < ProssimoCambio)
	{
		return;
	}
	ProssimoCambio = OraMondo + 0.6;
	bRiposte = !bRiposte;
	// sul server (o da soli) OnRep non arriva
	OnRep_Armi();
}

void UValdorsoArmiComponent::OnRep_Armi()
{
	// il suono del fodero (o delle cinghie dello scudo) quando si ripone o si sfodera, non quando si entra in gioco
	const bool bCambiato = bRiposte != bRiposteMostrate;
	bRiposteMostrate = bRiposte;
	if (bCambiato && HasBegunPlay() && GetOwner() && GetNetMode() != NM_DedicatedServer)
	{
		const TCHAR* Nome = bRiposte ? TEXT("S_Fodero_Riponi") : TEXT("S_Fodero_Sfodera");
		ValdorsoCombattimento::Suona(GetWorld(), ValdorsoCombattimento::SuonoPericoli(Nome), GetOwner()->GetActorLocation(), 1500.f,
			1.f, FMath::FRandRange(0.95f, 1.05f));
	}
	Mostra();
}

void UValdorsoArmiComponent::Mostra()
{
	// un server senza finestra non disegna niente
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// riposte: la spada nel fodero al fianco, lo scudo dietro la schiena; (14/11) l'arco nella sinistra, riposto sulla schiena
	const FValdorsoTipoOggetto* Portata = ArmaPortata();
	const bool bArco = Portata && Portata->bDueMani;
	InDestra = Pezzo(InDestra, Portata ? Destra : NAME_None, bArco ? (bRiposte ? ValdorsoArmi::PostoSchiena : ValdorsoArmi::PostoArco)
		: (bRiposte ? ValdorsoArmi::PostoFianco : ValdorsoArmi::PostoDestra));
	InSinistra = Pezzo(InSinistra, ScudoPortato() ? Sinistra : NAME_None, bRiposte ? ValdorsoArmi::PostoSchiena : ValdorsoArmi::PostoSinistra);
}

UStaticMeshComponent* UValdorsoArmiComponent::Pezzo(UStaticMeshComponent* Attuale, FName Id, int32 Posto)
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
	Attuale->AttachToComponent(Manichino, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ValdorsoArmi::Osso(Posto));
	Attuale->SetRelativeTransform(ValdorsoArmi::Presa(Posto));
	Attuale->SetVisibility(true);
	return Attuale;
}
