// Valdorso - Sedersi (vedi il .h). Scritto da Claude il 03/11/2026; diagnosi e prove della panca aggiunte la sera del 03/11.

#include "ValdorsoSeduta.h"
#include "Valdorso.h"
#include "ValdorsoRegistroEventi.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/ObjectKey.h"

#define LOCTEXT_NAMESPACE "ValdorsoSeduta"

namespace
{
	const TCHAR* PancaCartella = TEXT("/Game/Fab/GameAnimationSample/Panca/");

	// (03/11) Comandi di console per trovare il difetto della panca (si cambiano anche mentre si gioca, senza compilare).
	TAutoConsoleVariable<int32> CVarPancaDiagnosi(TEXT("Valdorso.Panca.Diagnosi"), 1,
		TEXT("1 = scrive nel log dove sta il colono a ogni fase della panca (partenza, movimento dell'animazione, arrivo)."));
	TAutoConsoleVariable<int32> CVarPancaFermo(TEXT("Valdorso.Panca.Fermo"), 0,
		TEXT("1 = prova: il colono va subito al posto e l'animazione gira sul posto (la capsula non segue il root motion)."));
	TAutoConsoleVariable<float> CVarPancaCorrezione(TEXT("Valdorso.Panca.Correzione"), 0.f,
		TEXT("Prova: gradi da aggiungere alla mesh mentre il colono è sulla panca (es. 90 o -90), se le animazioni guardano storto."));

	FString PancaTesto(const FTransform& T)
	{
		const FVector L = T.GetLocation();
		return FString::Printf(TEXT("(%.0f, %.0f, %.0f) yaw %.0f"), L.X, L.Y, L.Z, T.Rotator().Yaw);
	}

	/** Le animazioni per sedersi: 0-1 da davanti, 2-3 da sinistra, 4-5 da destra, 6-7 da dietro a sinistra, 8-9 da dietro a destra. */
	const TCHAR* PancaEntrate[] = {
		TEXT("Siede_F_Lfoot"), TEXT("Siede_F_Rfoot"),
		TEXT("Siede_L90_Lfoot"), TEXT("Siede_L90_Rfoot"),
		TEXT("Siede_R90_Lfoot"), TEXT("Siede_R90_Rfoot"),
		TEXT("Siede_BL_Lfoot"), TEXT("Siede_BL_Rfoot"),
		TEXT("Siede_BR_Lfoot"), TEXT("Siede_BR_Rfoot"),
	};
	constexpr int32 PancaNumeroEntrate = static_cast<int32>(UE_ARRAY_COUNT(PancaEntrate));

	UAnimSequence* PancaAnimazione(const TCHAR* Nome)
	{
		const FString Percorso = FString::Printf(TEXT("%s%s.%s"), PancaCartella, Nome, Nome);
		UAnimSequence* Sequenza = LoadObject<UAnimSequence>(nullptr, *Percorso);
		if (!Sequenza)
		{
			UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Panca: manca l'animazione %s (lanciare porta_panca_da_gasp.py nel GameAnimationSample)."), *Percorso);
			return nullptr;
		}
		// Il movimento lo prende la capsula (la seguiamo noi): la mesh resta ferma sulla capsula.
		Sequenza->bEnableRootMotion = true;
		Sequenza->RootMotionRootLock = ERootMotionRootLock::RefPose;
		return Sequenza;
	}

	/** Il movimento dell'animazione tra due istanti (secondi). */
	FTransform PancaMovimento(const UAnimSequence* Sequenza, double Da, double A)
	{
		if (!Sequenza)
		{
			return FTransform::Identity;
		}
		const double Durata = static_cast<double>(Sequenza->GetPlayLength());
		FAnimExtractContext Contesto;
		return Sequenza->ExtractRootMotionFromRange(FMath::Clamp(Da, 0.0, Durata), FMath::Clamp(A, 0.0, Durata), Contesto);
	}

	/** Il movimento dell'animazione dall'inizio fino a Tempo (secondi). */
	FTransform PancaMovimentoFino(const UAnimSequence* Sequenza, double Tempo)
	{
		return PancaMovimento(Sequenza, 0.0, Tempo);
	}

	constexpr double PancaPasso = 1.0 / 30.0;   // si guardano le animazioni a 30 fotogrammi al secondo

	/**
	 * (03/11) Da quando in poi la radice dell'animazione non si muove più (meno di 1 cm e 1 grado fino alla fine).
	 * Le animazioni della panca del GASP finiscono con molti secondi fermi (Siede 11 s, Alza 6,7 s): la fase finisce
	 * poco dopo questo istante, non alla fine della sequenza.
	 */
	double PancaFineMovimento(const UAnimSequence* Sequenza)
	{
		static TMap<FObjectKey, double> Gia;   // (le sequenze non cambiano mentre si gioca)
		if (!Sequenza)
		{
			return 0.0;
		}
		if (const double* Trovato = Gia.Find(FObjectKey(Sequenza)))
		{
			return *Trovato;
		}
		const double Durata = static_cast<double>(Sequenza->GetPlayLength());
		double Fine = Durata;
		for (double T = Durata - PancaPasso; T > 0.0; T -= PancaPasso)
		{
			const FTransform Resto = PancaMovimento(Sequenza, T, Durata);
			if (Resto.GetTranslation().Size() > 1.0 || FMath::Abs(Resto.Rotator().Yaw) > 1.0)
			{
				break;
			}
			Fine = T;
		}
		Gia.Add(FObjectKey(Sequenza), Fine);
		return Fine;
	}

	/**
	 * (03/11) Da che punto dell'animazione partire. Le animazioni per sedersi del GASP cominciano a 4-5 metri dalla panca
	 * (era il "balletto": il colono veniva portato là e tornava indietro camminando). Si cerca il fotogramma in cui il
	 * personaggio dell'animazione sta più vicino a dove sta davvero il colono, e girato come lui.
	 */
	double PancaScegliAvvio(const UAnimSequence* Sequenza, const FTransform& MeshSeduta, const FTransform& MeshAdesso, double FineMovimento)
	{
		const double Durata = static_cast<double>(Sequenza->GetPlayLength());
		double Migliore = 0.0;
		double Costo = TNumericLimits<double>::Max();
		for (double T = 0.0; T <= FineMovimento; T += PancaPasso)
		{
			// Solo i fotogrammi in cui c'è ancora da muoversi (almeno 40 cm o 45 gradi): sennò, arrivando da dietro,
			// vinceva il fotogramma già seduto (stessa direzione, mezzo metro) e il colono si sedeva senza animazione.
			const FTransform Resto = PancaMovimento(Sequenza, T, Durata);
			if (Resto.GetTranslation().Size2D() < 40.0 && FMath::Abs(Resto.Rotator().Yaw) < 45.0)
			{
				break;   // da qui in poi l'animazione si siede sul posto
			}
			const FTransform Inizio = Resto.Inverse() * MeshSeduta;
			const double Distanza = FVector::Dist2D(Inizio.GetLocation(), MeshAdesso.GetLocation());
			const double Gradi = FMath::Abs(FRotator::NormalizeAxis(Inizio.Rotator().Yaw - MeshAdesso.Rotator().Yaw));
			const double Questo = Distanza + 0.5 * Gradi;   // mezzo centimetro per grado
			if (Questo < Costo)
			{
				Costo = Questo;
				Migliore = T;
			}
		}
		return Migliore;
	}

	/** Secondi per assorbire la differenza tra il colono e l'animazione: 0,4 s, di più se la differenza è grande (1,5 m al secondo). */
	double PancaSfumatura(const FVector& Scarto)
	{
		return FMath::Max(0.4, Scarto.Size2D() / 150.0);
	}
}

// ------------------------------------------------------------------------------------------------
// Il componente sul colono
// ------------------------------------------------------------------------------------------------

UValdorsoSedutaComponent::UValdorsoSedutaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UValdorsoSedutaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UValdorsoSedutaComponent, Stato);
}

double UValdorsoSedutaComponent::Adesso() const
{
	const UWorld* Mondo = GetWorld();
	if (const AGameStateBase* Gioco = Mondo ? Mondo->GetGameState() : nullptr)
	{
		return Gioco->GetServerWorldTimeSeconds();
	}
	return Mondo ? Mondo->GetTimeSeconds() : 0.0;
}

UAnimSequence* UValdorsoSedutaComponent::AnimazioneDellaFase() const
{
	switch (Stato.Fase)
	{
	case 1: return PancaAnimazione(PancaEntrate[FMath::Clamp<int32>(Stato.Entrata, 0, PancaNumeroEntrate - 1)]);
	case 2: return PancaAnimazione(TEXT("Seduto"));
	case 3: return PancaAnimazione(TEXT("Alza_InPiedi"));
	default: return nullptr;
	}
}

void UValdorsoSedutaComponent::Siedi(AValdorsoPanca* Panca, int32 Posto)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || !Colono->HasAuthority() || !Panca || Occupato() || !Panca->Posti.IsValidIndex(Posto))
	{
		return;
	}
	const FTransform Seduto = Panca->TrasformaSeduto(Posto, Colono);

	// L'animazione dalla parte da cui arriva: l'angolo tra "davanti al posto" e "dove sta il colono".
	const FVector DalPosto = Seduto.InverseTransformVectorNoScale(Colono->GetActorLocation() - Seduto.GetLocation());
	const float Angolo = FMath::RadiansToDegrees(FMath::Atan2(DalPosto.Y, DalPosto.X));
	int32 Gruppo = 0;                                   // da davanti
	if (Angolo > 50.f) { Gruppo = Angolo > 130.f ? 4 : 2; }       // da destra (dietro a destra oltre 130)
	else if (Angolo < -50.f) { Gruppo = Angolo < -130.f ? 3 : 1; } // da sinistra (dietro a sinistra oltre 130)
	static const int32 PrimaEntrata[] = { 0, 2, 4, 6, 8 };
	const int32 Indice = PrimaEntrata[Gruppo] + (FMath::RandBool() ? 1 : 0);
	UAnimSequence* Sequenza = PancaAnimazione(PancaEntrate[Indice]);
	if (!Sequenza)
	{
		return;
	}

	// Da dove partire: alla fine dell'animazione la mesh deve trovarsi dove la vuole il posto.
	// La mesh come starà sulla panca (con la correzione di prova, se c'è).
	const float Correzione = CVarPancaCorrezione.GetValueOnGameThread();
	FTransform Relativa = Colono->GetMesh()->GetRelativeTransform();
	const FRotator Base = RotazioneBase();
	Relativa.SetRotation(FRotator(Base.Pitch, Base.Yaw + Correzione, Base.Roll).Quaternion());
	const FTransform MeshSeduta = Relativa * Seduto;
	const FTransform MeshAdesso = Relativa * Colono->GetActorTransform();
	const bool bFermo = CVarPancaFermo.GetValueOnGameThread() != 0;
	const double FineMovimento = PancaFineMovimento(Sequenza);
	const double Avvio = bFermo ? 0.0 : PancaScegliAvvio(Sequenza, MeshSeduta, MeshAdesso, FineMovimento);
	const FTransform Movimento = PancaMovimento(Sequenza, Avvio, Sequenza->GetPlayLength());

	Panca->Occupanti.SetNum(Panca->Posti.Num());
	Panca->Occupanti[Posto] = Colono;
	Stato.Panca = Panca;
	Stato.Posto = static_cast<uint8>(Posto);
	Stato.Entrata = static_cast<uint8>(Indice);
	Stato.Correzione = Correzione;
	Stato.bFermo = bFermo;
	Stato.Partenza = bFermo ? MeshSeduta : Movimento.Inverse() * MeshSeduta;
	Stato.Avvio = static_cast<float>(Avvio);
	// La fase finisce un secondo e mezzo dopo che la radice si è fermata (il corpo si assesta), non dopo i secondi fermi in fondo.
	Stato.Fine = static_cast<float>(FMath::Min(static_cast<double>(Sequenza->GetPlayLength()), FineMovimento + 1.5));
	// La piccola differenza tra dove sta il colono e dove comincia l'animazione si assorbe nei primi decimi di secondo.
	Stato.Scarto = bFermo ? FVector::ZeroVector : MeshAdesso.GetLocation() - Stato.Partenza.GetLocation();
	Stato.ScartoYaw = bFermo ? 0.f : static_cast<float>(FRotator::NormalizeAxis(MeshAdesso.Rotator().Yaw - Stato.Partenza.Rotator().Yaw));
	if (CVarPancaDiagnosi.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Panca: %s si siede con %s (arriva da %.0f gradi), durata %.2f s, fermo %d, correzione %.0f"),
			*Colono->GetName(), PancaEntrate[Indice], Angolo, Sequenza->GetPlayLength(), bFermo ? 1 : 0, Correzione);
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Panca:   colono adesso %s | posto (attore) %s | mesh relativa %s"),
			*PancaTesto(Colono->GetActorTransform()), *PancaTesto(Seduto), *PancaTesto(Relativa));
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Panca:   movimento dell'animazione %s | mesh seduta %s | mesh alla partenza %s"),
			*PancaTesto(Movimento), *PancaTesto(MeshSeduta), *PancaTesto(Stato.Partenza));
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Panca:   parte da %.2f s, finisce a %.2f s, scarto %.0f cm e %.0f gradi"),
			Stato.Avvio, Stato.Fine, Stato.Scarto.Size(), Stato.ScartoYaw);
	}
	ValdorsoRegistroEventi::Annota(TEXT("Siede"), Colono, Panca, Seduto.GetLocation());
	CambiaFase(1);
}

void UValdorsoSedutaComponent::Alzati()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || !Colono->HasAuthority() || Stato.Fase != 2)
	{
		return;
	}
	Stato.Partenza = Colono->GetMesh()->GetRelativeTransform() * Colono->GetActorTransform();
	const UAnimSequence* Sequenza = PancaAnimazione(TEXT("Alza_InPiedi"));
	Stato.Avvio = 0.f;
	Stato.Scarto = FVector::ZeroVector;
	Stato.ScartoYaw = 0.f;
	// Alzarsi finisce un secondo dopo che i piedi si sono fermati (l'animazione ha più di 3 s fermi in fondo).
	Stato.Fine = Sequenza ? static_cast<float>(FMath::Min(static_cast<double>(Sequenza->GetPlayLength()), PancaFineMovimento(Sequenza) + 1.0)) : 0.f;
	if (CVarPancaDiagnosi.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Panca: %s si alza, finisce a %.2f s, movimento dell'animazione %s, mesh alla partenza %s"),
			*Colono->GetName(), Stato.Fine,
			*PancaTesto(PancaMovimentoFino(Sequenza, Sequenza ? Sequenza->GetPlayLength() : 0.0)), *PancaTesto(Stato.Partenza));
	}
	CambiaFase(3);
}

void UValdorsoSedutaComponent::ChiediDiAlzarti()
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

void UValdorsoSedutaComponent::ServerAlzati_Implementation()
{
	Alzati();
}

void UValdorsoSedutaComponent::CambiaFase(uint8 Nuova)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (Nuova == 2 || Nuova == 0)
	{
		Diagnosi(Nuova == 2 ? TEXT("seduto (fine di Siede)") : TEXT("in piedi (fine di Alza)"));
	}
	Stato.Fase = Nuova;
	Stato.Inizio = Adesso();
	if (Nuova == 2 || Nuova == 0)
	{
		Stato.Avvio = 0.f;
		Stato.Fine = 0.f;
		Stato.Scarto = FVector::ZeroVector;
		Stato.ScartoYaw = 0.f;
	}
	if (Nuova == 0)
	{
		if (Stato.Panca)
		{
			Stato.Panca->Libera(Colono);
		}
		Stato.Panca = nullptr;
	}
	if (Colono)
	{
		Colono->ForceNetUpdate();
	}
	OnRep_Stato();
}

void UValdorsoSedutaComponent::OnRep_Stato()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono)
	{
		return;
	}
	if (CVarPancaDiagnosi.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Panca: %s riceve la fase %d (%s), panca %s"), *Colono->GetName(), Stato.Fase,
			Colono->HasAuthority() ? TEXT("server") : TEXT("PC"), Stato.Panca ? TEXT("sì") : TEXT("no"));
	}
	UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement();
	UCapsuleComponent* Capsula = Colono->GetCapsuleComponent();
	if (Stato.Fase != 0)
	{
		// Seduto o in movimento verso la panca: il movimento normale si spegne e la panca non fa da muro alla capsula.
		if (Movimento && Movimento->MovementMode != MOVE_None)
		{
			Movimento->StopMovementImmediately();
			Movimento->DisableMovement();
		}
		if (Capsula && Stato.Panca)
		{
			// Se (per un pacchetto perso) si passa da una panca all'altra senza "in piedi", la vecchia torna un ostacolo.
			if (PancaIgnorata.IsValid() && PancaIgnorata.Get() != Stato.Panca)
			{
				Capsula->IgnoreActorWhenMoving(PancaIgnorata.Get(), false);
			}
			Capsula->IgnoreActorWhenMoving(Stato.Panca, true);
			PancaIgnorata = Stato.Panca;
		}
		ApplicaCorrezione(Stato.Correzione);
		SetComponentTickEnabled(true);
	}
	else
	{
		if (Movimento && Movimento->MovementMode == MOVE_None)
		{
			Movimento->SetMovementMode(MOVE_Walking);
		}
		// Di nuovo in piedi: la panca torna a essere un ostacolo e la mesh torna dritta.
		if (Capsula && PancaIgnorata.IsValid())
		{
			Capsula->IgnoreActorWhenMoving(PancaIgnorata.Get(), false);
		}
		PancaIgnorata.Reset();
		// Togliendo la correzione dalla mesh si gira l'attore degli stessi gradi: il corpo resta dov'è, senza scatti.
		if (CorrezioneMessa != 0.f && (Colono->HasAuthority() || Colono->IsLocallyControlled()))
		{
			const FRotator Verso = Colono->GetActorRotation();
			Colono->SetActorRotation(FRotator(0.f, Verso.Yaw + CorrezioneMessa, 0.f), ETeleportType::TeleportPhysics);
		}
		ApplicaCorrezione(0.f);
		SetComponentTickEnabled(false);
	}
	SuonaFase();
}

void UValdorsoSedutaComponent::SuonaFase()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anima = Colono && Colono->GetMesh() ? Colono->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anima || (FaseSuonata == Stato.Fase && InizioSuonato == Stato.Inizio))
	{
		return;
	}
	FaseSuonata = Stato.Fase;
	InizioSuonato = Stato.Inizio;
	if (Stato.Fase == 0)
	{
		Anima->StopSlotAnimation(0.3f, TEXT("DefaultSlot"));
		return;
	}
	UAnimSequence* Sequenza = AnimazioneDellaFase();
	if (!Sequenza)
	{
		return;
	}
	// Chi arriva a metà (o con il ritardo della rete) parte dal punto giusto.
	const float Gia = static_cast<float>(FMath::Max(0.0, Adesso() - Stato.Inizio));
	const int32 Giri = Stato.Fase == 2 ? 100000 : 1;
	// Sedersi parte dal fotogramma scelto dal server (Avvio), non dall'inizio della sequenza.
	const float Punto = Stato.Fase == 2 ? FMath::Fmod(Gia, FMath::Max(0.1f, Sequenza->GetPlayLength())) : FMath::Min(Stato.Avvio + Gia, Sequenza->GetPlayLength());
	// Il seduto entra con una sfumatura lunga: sedersi finisce prima della fine della sequenza e il corpo si sta ancora assestando.
	const float Entrata = Stato.Fase == 1 ? 0.2f : (Stato.Fase == 2 ? 0.4f : 0.15f);
	Anima->PlaySlotAnimationAsDynamicMontage(Sequenza, TEXT("DefaultSlot"), Entrata, 0.25f, 1.f, Giri, -1.f, Punto);
}

void UValdorsoSedutaComponent::SeguiAnimazione()
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || (Stato.Fase != 1 && Stato.Fase != 3))
	{
		return;
	}
	const UAnimSequence* Sequenza = AnimazioneDellaFase();
	const double Passato = Adesso() - Stato.Inizio;
	const double Tempo = Stato.Avvio + Passato;   // il punto dell'animazione
	// Mesh nel mondo adesso = movimento fatto finora (da Avvio), a partire da dove stava all'inizio; l'attore sta "sopra" la mesh.
	FTransform Mesh = Stato.bFermo ? Stato.Partenza : PancaMovimento(Sequenza, Stato.Avvio, Tempo) * Stato.Partenza;
	// Nei primi decimi di secondo si assorbe lo scarto tra dove stava il colono e dove comincia l'animazione.
	const double Resta = 1.0 - FMath::Clamp(Passato / PancaSfumatura(Stato.Scarto), 0.0, 1.0);
	if (Resta > 0.0)
	{
		Mesh.AddToTranslation(Stato.Scarto * Resta);
		Mesh.SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(Stato.ScartoYaw * Resta)) * Mesh.GetRotation());
	}
	const FTransform Attore = Colono->GetMesh()->GetRelativeTransform().Inverse() * Mesh;
	Colono->SetActorLocationAndRotation(Attore.GetLocation(), FRotator(0.f, Attore.Rotator().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);

	// Sul server: a fine movimento (Fine, o la fine della sequenza) si passa alla fase dopo.
	const double Fine = Stato.Fine > 0.f ? static_cast<double>(Stato.Fine) : (Sequenza ? static_cast<double>(Sequenza->GetPlayLength()) : 0.0);
	if (Colono->HasAuthority() && Sequenza && Tempo >= Fine)
	{
		// Prova "fermo": alzandosi la capsula è rimasta sul posto; la si porta 60 cm davanti, fuori dalla panca.
		if (Stato.bFermo && Stato.Fase == 3)
		{
			const FVector Davanti = Stato.Panca ? Stato.Panca->GetActorForwardVector() : Colono->GetActorForwardVector();
			Colono->SetActorLocation(Colono->GetActorLocation() + Davanti * 60.f, false, nullptr, ETeleportType::TeleportPhysics);
		}
		CambiaFase(Stato.Fase == 1 ? 2 : 0);
	}
}

void UValdorsoSedutaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// La correzione si rimette a ogni fotogramma: sugli altri PC lo "smoothing" del movimento riscrive la rotazione della mesh.
	if (Stato.Fase != 0)
	{
		ApplicaCorrezione(Stato.Correzione);
	}
	SeguiAnimazione();
	// Se l'animazione non è partita (per esempio l'Anim Instance non c'era ancora), si riprova.
	SuonaFase();
}

void UValdorsoSedutaComponent::BeginPlay()
{
	Super::BeginPlay();
	// Chi arriva dopo (o rientra nel raggio della rete) mentre il colono è già sulla panca: la correzione va messa adesso.
	ApplicaCorrezione(Stato.Fase != 0 ? Stato.Correzione : 0.f);
}

FRotator UValdorsoSedutaComponent::RotazioneBase()
{
	// Letta una volta sola, prima di ogni correzione: è la rotazione "di serie" della mesh sulla capsula.
	if (!bRotazioneMeshLetta)
	{
		if (const ACharacter* Colono = Cast<ACharacter>(GetOwner()))
		{
			RotazioneMeshBase = Colono->GetBaseRotationOffsetRotator();
			bRotazioneMeshLetta = true;
		}
	}
	return RotazioneMeshBase;
}

void UValdorsoSedutaComponent::ApplicaCorrezione(float Gradi)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || !Colono->GetMesh())
	{
		return;
	}
	if (Gradi == 0.f && CorrezioneMessa == 0.f)
	{
		return; // niente da fare: la mesh è già quella di serie (e la si lascia allo smoothing del movimento)
	}
	const FRotator Base = RotazioneBase();
	const FRotator Voluta(Base.Pitch, Base.Yaw + Gradi, Base.Roll);
	if (!Colono->GetMesh()->GetRelativeRotation().Equals(Voluta, 0.01f))
	{
		Colono->GetMesh()->SetRelativeRotation(Voluta);
	}
	// Anche lo smoothing della rete deve conoscerla, sennò la toglie.
	Colono->CacheInitialMeshOffset(Colono->GetBaseTranslationOffset(), Voluta);
	CorrezioneMessa = Gradi;
}

void UValdorsoSedutaComponent::Diagnosi(const TCHAR* Quando) const
{
	const ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono || !Colono->GetMesh() || CVarPancaDiagnosi.GetValueOnGameThread() == 0)
	{
		return;
	}
	const FTransform Mesh = Colono->GetMesh()->GetComponentTransform();
	FString Atteso = TEXT("-");
	if (Stato.Panca && Stato.Panca->Posti.IsValidIndex(Stato.Posto))
	{
		Atteso = PancaTesto(Stato.Panca->TrasformaSeduto(Stato.Posto, Colono));
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Panca: %s %s: attore %s, mesh %s, posto voluto (attore) %s, fase %d dopo %.2f s"),
		*Colono->GetName(), Quando, *PancaTesto(Colono->GetActorTransform()), *PancaTesto(Mesh), *Atteso,
		Stato.Fase, Adesso() - Stato.Inizio);
}

void UValdorsoSedutaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Stato.Panca && GetOwner() && GetOwner()->HasAuthority())
	{
		Stato.Panca->Libera(Cast<APawn>(GetOwner()));
	}
	Super::EndPlay(EndPlayReason);
}

// ------------------------------------------------------------------------------------------------
// La panca
// ------------------------------------------------------------------------------------------------

AValdorsoPanca::AValdorsoPanca()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	Posti = { -45.f, 45.f };

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Semplice(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	auto Pezzo = [&](const TCHAR* Nome, const FVector& Scala, const FVector& Posto)
	{
		UStaticMeshComponent* Nuovo = CreateDefaultSubobject<UStaticMeshComponent>(Nome);
		Nuovo->SetupAttachment(RootComponent);
		if (Cubo.Succeeded())
		{
			Nuovo->SetStaticMesh(Cubo.Object);
		}
		if (Semplice.Succeeded())
		{
			Nuovo->SetMaterial(0, Semplice.Object);
		}
		Nuovo->SetRelativeScale3D(Scala);
		Nuovo->SetRelativeLocation(Posto);
		Nuovo->SetCollisionProfileName(TEXT("BlockAll"));
		Nuovo->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		return Nuovo;
	};
	// Seduta 180 × 45 × 6 cm, a 45 cm da terra; quattro gambe di 8 × 8 × 42 cm.
	Seduta = Pezzo(TEXT("Seduta"), FVector(0.45f, 1.8f, 0.06f), FVector(0.f, 0.f, 45.f));
	Gambe.Add(Pezzo(TEXT("Gamba1"), FVector(0.08f, 0.08f, 0.42f), FVector(15.f, -80.f, 21.f)));
	Gambe.Add(Pezzo(TEXT("Gamba2"), FVector(0.08f, 0.08f, 0.42f), FVector(-15.f, -80.f, 21.f)));
	Gambe.Add(Pezzo(TEXT("Gamba3"), FVector(0.08f, 0.08f, 0.42f), FVector(15.f, 80.f, 21.f)));
	Gambe.Add(Pezzo(TEXT("Gamba4"), FVector(0.08f, 0.08f, 0.42f), FVector(-15.f, 80.f, 21.f)));
}

void AValdorsoPanca::BeginPlay()
{
	Super::BeginPlay();
	Occupanti.SetNum(Posti.Num());
	auto Colora = [](UStaticMeshComponent* Pezzo, const FLinearColor& Colore)
	{
		if (Pezzo && Pezzo->GetStaticMesh() && Pezzo->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/")))
		{
			if (UMaterialInstanceDynamic* Legno = Pezzo->CreateDynamicMaterialInstance(0))
			{
				Legno->SetVectorParameterValue(TEXT("Color"), Colore);
			}
		}
	};
	Colora(Seduta, FLinearColor(0.36f, 0.22f, 0.11f));
	for (UStaticMeshComponent* Gamba : Gambe)
	{
		Colora(Gamba, FLinearColor(0.2f, 0.12f, 0.06f));
	}
}

FTransform AValdorsoPanca::TrasformaSeduto(int32 Posto, const APawn* Chi) const
{
	const float Lungo = Posti.IsValidIndex(Posto) ? Posti[Posto] : 0.f;
	const ACharacter* Colono = Cast<ACharacter>(Chi);
	const float MezzaAltezza = Colono && Colono->GetCapsuleComponent() ? Colono->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.f;
	// L'attore del colono: a terra (più mezza capsula), sul posto, girato come la panca (guarda davanti).
	const FVector Luogo = GetActorTransform().TransformPosition(FVector(AvantiPosto, Lungo, 0.f)) + FVector(0.f, 0.f, MezzaAltezza);
	return FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), Luogo);
}

int32 AValdorsoPanca::PostoLibero(const APawn* Chi) const
{
	int32 Migliore = INDEX_NONE;
	float Distanza = TNumericLimits<float>::Max();
	for (int32 i = 0; i < Posti.Num(); ++i)
	{
		if (Occupanti.IsValidIndex(i) && Occupanti[i].IsValid())
		{
			continue;
		}
		const float D = FVector::Dist2D(TrasformaSeduto(i, Chi).GetLocation(), Chi->GetActorLocation());
		if (D < Distanza)
		{
			Distanza = D;
			Migliore = i;
		}
	}
	return Migliore;
}

void AValdorsoPanca::Libera(const APawn* Chi)
{
	for (TWeakObjectPtr<APawn>& Occupante : Occupanti)
	{
		if (Occupante.Get() == Chi)
		{
			Occupante.Reset();
		}
	}
}

FVector AValdorsoPanca::PuntoInterazione() const
{
	// Il centro della seduta: davanti e dietro vanno bene (ci si può sedere anche arrivando da dietro).
	return GetActorLocation() + FVector(0.f, 0.f, 50.f);
}

FText AValdorsoPanca::TestoInterazione(const APawn* Chi) const
{
	const UValdorsoSedutaComponent* Componente = Chi ? Chi->FindComponentByClass<UValdorsoSedutaComponent>() : nullptr;
	if (!Componente)
	{
		return FText::GetEmpty();
	}
	if (Componente->Occupato())
	{
		// "Alzati" solo quando è seduto del tutto: mentre si siede o si alza non c'è niente da premere.
		return Componente->Seduto() && Componente->GetPanca() == this ? LOCTEXT("Alzati", "Alzati") : FText::GetEmpty();
	}
	// I posti occupati li sa il server; sul PC si guarda chi è già seduto qui.
	return LOCTEXT("Siediti", "Siediti");
}

void AValdorsoPanca::Interagisci(APawn* Chi)
{
	if (!HasAuthority() || !Chi)
	{
		return;
	}
	UValdorsoSedutaComponent* Componente = Chi->FindComponentByClass<UValdorsoSedutaComponent>();
	if (!Componente)
	{
		return;
	}
	if (Componente->Occupato())
	{
		if (Componente->GetPanca() == this)
		{
			Componente->Alzati();
		}
		return;
	}
	Occupanti.SetNum(Posti.Num());
	const int32 Posto = PostoLibero(Chi);
	if (Posto != INDEX_NONE)
	{
		Componente->Siedi(this, Posto);
	}
}

#undef LOCTEXT_NAMESPACE
