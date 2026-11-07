// Valdorso - Il villaggio che cresce a fasi (vedi il .h). Scritto da Claude il 06/11/2026.

#include "ValdorsoFasiVillaggio.h"
#include "Valdorso.h"
#include "ValdorsoRegistroEventi.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"

namespace ValdorsoFasi
{
	bool LeggiEtichetta(const FName& Etichetta, int32& Da, int32& Fino)
	{
		const FString Testo = Etichetta.ToString();
		if (!Testo.StartsWith(Prefisso, ESearchCase::IgnoreCase))
		{
			return false;
		}
		FString Sinistra, Destra;
		if (!Testo.Mid(FCString::Strlen(Prefisso)).TrimStartAndEnd().Split(TEXT("-"), &Sinistra, &Destra))
		{
			return false;
		}
		if (Sinistra.IsEmpty() || Destra.IsEmpty() || !Sinistra.IsNumeric() || !Destra.IsNumeric())
		{
			return false;
		}
		Da = FCString::Atoi(*Sinistra);
		Fino = FCString::Atoi(*Destra);
		// "2-1" o "0-3" non hanno senso: si trattano come etichette sbagliate (la cosa resta com'è).
		return Da >= 1 && Fino >= 0 && (Fino == 0 || Fino >= Da);
	}

	bool CeNellaFase(int32 Fase, int32 Da, int32 Fino)
	{
		return Fase >= Da && (Fino == 0 || Fase <= Fino);
	}

	bool FaseDiAttore(const AActor* Attore, int32& Da, int32& Fino)
	{
		if (!Attore)
		{
			return false;
		}
		for (const FName& Etichetta : Attore->Tags)
		{
			if (LeggiEtichetta(Etichetta, Da, Fino))
			{
				return true;
			}
		}
		return false;
	}
}

namespace
{
	/** Valdorso.Villaggio.Fase 2: cambia la fase del villaggio (solo dove c'è il server: editor, server dedicato). */
	FAutoConsoleCommand ComandoFaseVillaggio(
		TEXT("Valdorso.Villaggio.Fase"),
		TEXT("Mette la fase del villaggio (1 = oggi, 2 = anello di case e palizzata larga, 3 = muro di pietra), per esempio Valdorso.Villaggio.Fase 2. Senza numero dice la fase di adesso."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti)
		{
			for (TObjectIterator<AValdorsoFasiVillaggio> Fasi; Fasi; ++Fasi)
			{
				if (!IsValid(*Fasi) || !Fasi->GetWorld() || !Fasi->GetWorld()->IsGameWorld())
				{
					continue;
				}
				if (Argomenti.Num() > 0 && Fasi->HasAuthority())
				{
					Fasi->ImpostaFase(FCString::Atoi(*Argomenti[0]));
				}
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Villaggio (%s): fase %d"),
					Fasi->HasAuthority() ? TEXT("server") : TEXT("PC"), Fasi->GetFase());
			}
		}));

	/** Mostra o nasconde un attore del villaggio: visibile, con collisione e con le luci accese, oppure niente. */
	void MostraAttoreDellaFase(AActor* Attore, bool bVisibile)
	{
		if (Attore->IsHidden() == !bVisibile && Attore->GetActorEnableCollision() == bVisibile)
		{
			return;
		}
		Attore->SetActorHiddenInGame(!bVisibile);
		Attore->SetActorEnableCollision(bVisibile);
		TInlineComponentArray<ULightComponent*> Luci(Attore);
		for (ULightComponent* Luce : Luci)
		{
			Luce->SetVisibility(bVisibile);
		}
	}

	/** (07/11 sera) La piazza, le strade e il piazzale delle carovane si calpestano: collisione vera, da tutte e due le
	 *  facce, anche se il livello è stato salvato senza (i piedi del colono affondavano nei ciottoli). Vale su server e
	 *  PC, anche per i pezzi che arrivano dopo con World Partition. Vero se l'attore è uno di questi. */
	bool RendiSuoloVillaggioCalpestabile(AActor* Attore)
	{
		bool bSuolo = false;
		TInlineComponentArray<UStaticMeshComponent*> Pezzi(Attore);
		for (UStaticMeshComponent* Pezzo : Pezzi)
		{
			UStaticMesh* Mesh = Pezzo ? Pezzo->GetStaticMesh() : nullptr;
			if (!Mesh)
			{
				continue;
			}
			const FString Nome = Mesh->GetName();
			if (!Nome.StartsWith(TEXT("SM_Villaggio_Piazza")) && !Nome.StartsWith(TEXT("SM_Villaggio_Strade"))
				&& !Nome.StartsWith(TEXT("SM_Villaggio_Carovane")))
			{
				continue;
			}
			bSuolo = true;
			bool bRifai = false;
			if (UBodySetup* Corpo = Mesh->GetBodySetup())
			{
				if (Corpo->CollisionTraceFlag != CTF_UseComplexAsSimple || !Corpo->bDoubleSidedGeometry)
				{
					Corpo->CollisionTraceFlag = CTF_UseComplexAsSimple;
					Corpo->bDoubleSidedGeometry = true;
					bRifai = true;
				}
			}
			if (Pezzo->GetCollisionProfileName() != UCollisionProfile::BlockAll_ProfileName)
			{
				Pezzo->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
				bRifai = true;
			}
			Pezzo->CanCharacterStepUpOn = ECB_Yes;
			// la prima volta che si vede questo pezzo si rifà sempre la sua fisica (la forma della mesh è condivisa: se
			// l'ha già sistemata un altro pezzo, qui i valori sembrano giusti ma la fisica è ancora quella vecchia)
			static const FName Segno(TEXT("ValdorsoCalpestabile"));
			const bool bPrimaVolta = !Pezzo->ComponentHasTag(Segno);
			if (!bRifai && !bPrimaVolta)
			{
				continue;
			}
			Pezzo->ComponentTags.AddUnique(Segno);
			Pezzo->RecreatePhysicsState();
			UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Villaggio: %s calpestabile (%s, collisione %s%s)"), *Nome,
				bRifai ? TEXT("sistemata adesso") : TEXT("era già a posto"),
				Pezzo->IsCollisionEnabled() ? TEXT("accesa") : TEXT("SPENTA"),
				Attore->GetActorEnableCollision() ? TEXT("") : TEXT(", ma l'attore ha la collisione spenta"));
		}
		return bSuolo;
	}
}

AValdorsoFasiVillaggio::AValdorsoFasiVillaggio()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
}

void AValdorsoFasiVillaggio::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdorsoFasiVillaggio, Fase);
}

void AValdorsoFasiVillaggio::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		const int32 Salvata = LeggiFaseSalvata();
		Fase = FMath::Clamp(Salvata > 0 ? Salvata : FaseIniziale, 1, FMath::Max(1, FaseMassima));
	}
	ManiglioLivelli = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &AValdorsoFasiVillaggio::QuandoArrivaLivello);
	Applica();
	// Un secondo ripasso poco dopo: qualche pezzo della valle può arrivare insieme a questo attore.
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().SetTimer(TimerRipasso, FTimerDelegate::CreateWeakLambda(this, [this]() { Applica(); }), 2.f, false);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Villaggio: fase %d (%s)"), Fase, HasAuthority() ? TEXT("server") : TEXT("PC"));
}

void AValdorsoFasiVillaggio::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FWorldDelegates::LevelAddedToWorld.Remove(ManiglioLivelli);
	if (UWorld* Mondo = GetWorld())
	{
		Mondo->GetTimerManager().ClearTimer(TimerRipasso);
	}
	Super::EndPlay(EndPlayReason);
}

void AValdorsoFasiVillaggio::OnRep_Fase()
{
	Applica();
}

void AValdorsoFasiVillaggio::ImpostaFase(int32 NuovaFase)
{
	if (!HasAuthority())
	{
		return;
	}
	const int32 Prima = Fase;
	Fase = FMath::Clamp(NuovaFase, 1, FMath::Max(1, FaseMassima));
	if (Fase == Prima)
	{
		return;
	}
	SalvaFase();
	ValdorsoRegistroEventi::Annota(TEXT("FaseVillaggio"), this, nullptr, GetActorLocation(),
		FString::Printf(TEXT("da %d a %d"), Prima, Fase));
	ForceNetUpdate();
	Applica();
}

void AValdorsoFasiVillaggio::QuandoArrivaLivello(ULevel* Livello, UWorld* Mondo)
{
	if (Mondo == GetWorld() && Livello)
	{
		Applica(Livello);
	}
}

void AValdorsoFasiVillaggio::Applica(ULevel* SoloLivello)
{
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}
	int32 Mostrate = 0;
	int32 Nascoste = 0;
	auto Sistema = [&](AActor* Attore)
	{
		int32 Da = 1;
		int32 Fino = 0;
		if (!IsValid(Attore))
		{
			return;
		}
		RendiSuoloVillaggioCalpestabile(Attore);
		if (!ValdorsoFasi::FaseDiAttore(Attore, Da, Fino))
		{
			return;
		}
		const bool bVisibile = ValdorsoFasi::CeNellaFase(Fase, Da, Fino);
		MostraAttoreDellaFase(Attore, bVisibile);
		++(bVisibile ? Mostrate : Nascoste);
	};
	if (SoloLivello)
	{
		for (AActor* Attore : SoloLivello->Actors)
		{
			Sistema(Attore);
		}
	}
	else
	{
		for (TActorIterator<AActor> It(Mondo); It; ++It)
		{
			Sistema(*It);
		}
	}
	if (Mostrate + Nascoste > 0)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Villaggio: fase %d, %d cose mostrate e %d nascoste%s"),
			Fase, Mostrate, Nascoste, SoloLivello ? TEXT(" (pezzo arrivato)") : TEXT(""));
	}
}

FString AValdorsoFasiVillaggio::FileFase()
{
	return FPaths::ProjectSavedDir() / TEXT("Server") / TEXT("Villaggio.json");
}

int32 AValdorsoFasiVillaggio::LeggiFaseSalvata() const
{
	FString Testo;
	if (!FFileHelper::LoadFileToString(Testo, *FileFase()))
	{
		return 0;
	}
	// Il file è piccolo e lo scriviamo noi: {"fase": 2}
	int32 Inizio = INDEX_NONE;
	if (!Testo.FindChar(TEXT(':'), Inizio))
	{
		return 0;
	}
	FString Numero;
	for (int32 i = Inizio + 1; i < Testo.Len(); ++i)
	{
		if (FChar::IsDigit(Testo[i]))
		{
			Numero.AppendChar(Testo[i]);
		}
		else if (!Numero.IsEmpty())
		{
			break;
		}
	}
	return Numero.IsEmpty() ? 0 : FCString::Atoi(*Numero);
}

void AValdorsoFasiVillaggio::SalvaFase() const
{
	const FString Percorso = FileFase();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Percorso), true);
	const FString Testo = FString::Printf(TEXT("{\"fase\": %d, \"quando\": \"%s\"}\n"), Fase, *FDateTime::UtcNow().ToIso8601());
	if (!FFileHelper::SaveStringToFile(Testo, *Percorso, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Villaggio: non riesco a salvare la fase in %s"), *Percorso);
	}
}
