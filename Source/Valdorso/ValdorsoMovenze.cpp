// Valdorso - Le movenze del colono sulla scala e in acqua (vedi il .h). Scritto da Claude il 06/11/2026.

#include "ValdorsoMovenze.h"
#include "Valdorso.h"
#include "ValdorsoScala.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<float> CVarMovenzeScala(
		TEXT("Valdorso.Movenze.Scala"),
		1.f,
		TEXT("Velocità dell'animazione della scala a pioli (1 = come è fatta, alla velocità di salita di serie)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarMovenzeNuoto(
		TEXT("Valdorso.Movenze.Nuoto"),
		1.f,
		TEXT("Velocità dell'animazione del nuoto (1 = come è fatta)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarMovenzeSteso(
		TEXT("Valdorso.Movenze.Steso"),
		75.f,
		TEXT("Quanto si stende il colono quando nuota (gradi in avanti, 0 = dritto)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarMovenzeStesoSu(
		TEXT("Valdorso.Movenze.StesoSu"),
		0.f,
		TEXT("Quanto alzare (cm, negativo = abbassare) il corpo steso quando nuota."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarMovenzeDiagnosi(
		TEXT("Valdorso.Movenze.Diagnosi"),
		0,
		TEXT("1: scrive nel registro ogni cambio di movenza e quanto è inclinato il corpo."),
		ECVF_Default);

	/** Dove stanno le animazioni (porta_movenze.py), nell'ordine di EValdorsoMovenza (la prima, Nessuna, non c'è). */
	const TCHAR* const NomiMovenze[] = {
		nullptr,
		TEXT("Scala_Su"),
		TEXT("Scala_Giu"),
		TEXT("Scala_Ferma"),
		TEXT("Nuoto_Avanti"),
		TEXT("Nuoto_Sinistra"),
		TEXT("Nuoto_Destra"),
		TEXT("Nuoto_Fermo"),
	};
	constexpr int32 NumeroMovenze = 8;
	static_assert(UE_ARRAY_COUNT(NomiMovenze) == NumeroMovenze, "una animazione per ogni EValdorsoMovenza");
	const TCHAR* CartellaMovenze = TEXT("/Game/Fab/FreeAnimationLibrary/Movenze/");

	/** La velocità di salita di serie della scala (AValdorsoScala::Velocita), per il ritmo dell'animazione. */
	constexpr float VelocitaScalaDiSerie = 140.f;
	constexpr float SfumaMovenze = 0.25f;
}

namespace ValdorsoMovenze
{
	EValdorsoMovenza Scala(float VelocitaZ)
	{
		if (VelocitaZ > 20.f)
		{
			return EValdorsoMovenza::ScalaSu;
		}
		if (VelocitaZ < -20.f)
		{
			return EValdorsoMovenza::ScalaGiu;
		}
		return EValdorsoMovenza::ScalaFerma;
	}

	EValdorsoMovenza Nuoto(const FVector& V)
	{
		const float Avanti = static_cast<float>(V.X);
		const float Destra = static_cast<float>(V.Y);
		if (FMath::Sqrt(Avanti * Avanti + Destra * Destra) < 40.f)
		{
			return EValdorsoMovenza::NuotoFermo;
		}
		// di lato solo se va soprattutto di lato (il colono di solito si gira verso dove nuota)
		if (FMath::Abs(Destra) > FMath::Abs(Avanti) * 1.2f)
		{
			return Destra > 0.f ? EValdorsoMovenza::NuotoDestra : EValdorsoMovenza::NuotoSinistra;
		}
		return EValdorsoMovenza::NuotoAvanti;
	}
}

namespace ValdorsoMovenze
{
	float Inclinazione(float Voluta, float Gia)
	{
		return FMath::Clamp(Voluta - Gia, 0.f, 90.f);
	}

	FTransform Inclinato(const FTransform& Base, float Gradi, const FVector& Perno)
	{
		// in avanti = la testa (Z) va verso X: rotazione intorno a Y
		const FQuat Giro(FVector(0.0, 1.0, 0.0), FMath::DegreesToRadians(Gradi));
		FTransform Fuori = Base;
		Fuori.SetRotation(Giro * Base.GetRotation());
		Fuori.SetLocation(Perno + Giro.RotateVector(Base.GetLocation() - Perno));
		return Fuori;
	}
}

UValdorsoMovenzeComponent::UValdorsoMovenzeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;
	SetIsReplicatedByDefault(false);
}

void UValdorsoMovenzeComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		// il server dedicato non disegna: niente movenze
		SetComponentTickEnabled(false);
	}
}

void UValdorsoMovenzeComponent::Carica()
{
	bCaricate = true;
	Animazioni.SetNum(NumeroMovenze);
	int32 Trovate = 0;
	for (int32 i = 1; i < NumeroMovenze; ++i)
	{
		const FString Percorso = FString::Printf(TEXT("%s%s.%s"), CartellaMovenze, NomiMovenze[i], NomiMovenze[i]);
		Animazioni[i] = LoadObject<UAnimSequence>(nullptr, *Percorso, nullptr, LOAD_NoWarn);
		Trovate += Animazioni[i] ? 1 : 0;
	}
	if (Trovate < NumeroMovenze - 1)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Movenze: trovate %d animazioni su %d in %s (lanciare Content/Python/nuovi/porta_movenze.py)"),
			Trovate, NumeroMovenze - 1, CartellaMovenze);
	}
}

void UValdorsoMovenzeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bCaricate)
	{
		Carica();
	}
	const ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (!Colono)
	{
		return;
	}
	const FVector Velocita = Colono->GetVelocity();
	const UValdorsoScalataComponent* Scalata = Colono->FindComponentByClass<UValdorsoScalataComponent>();
	const UCharacterMovementComponent* Movimento = Colono->GetCharacterMovement();

	EValdorsoMovenza Voluta = EValdorsoMovenza::Nessuna;
	float Ritmo = 1.f;
	float Steso = 0.f;
	if (Scalata && Scalata->Sale())
	{
		Voluta = ValdorsoMovenze::Scala(static_cast<float>(Velocita.Z));
		if (Voluta != EValdorsoMovenza::ScalaFerma)
		{
			Ritmo = CVarMovenzeScala.GetValueOnGameThread() * FMath::Clamp(FMath::Abs(static_cast<float>(Velocita.Z)) / VelocitaScalaDiSerie, 0.3f, 2.f);
		}
	}
	else if (Movimento && Movimento->IsSwimming())
	{
		const FVector Locale = Colono->GetActorRotation().UnrotateVector(FVector(Velocita.X, Velocita.Y, 0.0));
		Voluta = ValdorsoMovenze::Nuoto(Locale);
		if (Voluta != EValdorsoMovenza::NuotoFermo)
		{
			Steso = CVarMovenzeSteso.GetValueOnGameThread() * (Voluta == EValdorsoMovenza::NuotoAvanti ? 1.f : 0.8f);
			const float Massima = FMath::Max(100.f, Movimento->MaxSwimSpeed);
			Ritmo = CVarMovenzeNuoto.GetValueOnGameThread() * FMath::Clamp(static_cast<float>(Locale.Size()) / Massima, 0.6f, 1.4f);
		}
	}
	Cambia(Voluta, FMath::Max(0.05f, Ritmo));
	Stendi(Montaggio ? Steso : 0.f, DeltaTime);
}

void UValdorsoMovenzeComponent::Stendi(float Voluta, float DeltaTime)
{
	ACharacter* Colono = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Colono ? Colono->GetMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}
	if (Voluta <= 0.f && Inclinazione <= 0.f)
	{
		return;                                       // dritto, e lo era già: non si tocca niente (il letto, la panca)
	}
	if (!bBaseSalvata)
	{
		BasePosto = Colono->GetBaseTranslationOffset();
		BaseVerso = Colono->GetBaseRotationOffsetRotator();
		bBaseSalvata = true;
	}
	// quanto è già steso il corpo nell'animazione: dal bacino alla testa, nello spazio della mesh (Z su)
	float Gia = 0.f;
	if (Voluta > 0.f)
	{
		const FVector Bacino = Mesh->GetSocketTransform(TEXT("pelvis"), RTS_Component).GetLocation();
		const FVector Testa = Mesh->GetSocketTransform(TEXT("head"), RTS_Component).GetLocation();
		const FVector Su = (Testa - Bacino).GetSafeNormal();
		if (!Su.IsNearlyZero())
		{
			Gia = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(Su.Z), -1.f, 1.f)));
		}
	}
	const float Obiettivo = ValdorsoMovenze::Inclinazione(Voluta, Gia);
	Inclinazione = FMath::FInterpTo(Inclinazione, Obiettivo, DeltaTime, 4.f);
	if (Obiettivo <= 0.f && Inclinazione < 0.5f)
	{
		Inclinazione = 0.f;
	}
	const float Peso = FMath::Clamp(Inclinazione / FMath::Max(1.f, CVarMovenzeSteso.GetValueOnGameThread()), 0.f, 1.f);
	const FTransform Base(BaseVerso, BasePosto);
	FTransform Nuovo = ValdorsoMovenze::Inclinato(Base, Inclinazione, FVector::ZeroVector);
	Nuovo.AddToTranslation(FVector(0.0, 0.0, CVarMovenzeStesoSu.GetValueOnGameThread() * Peso));
	// la mesh si sposta subito; la base serve allo smoothing di rete dei coloni degli altri (come il letto)
	Mesh->SetRelativeLocationAndRotation(Nuovo.GetLocation(), Nuovo.Rotator());
	Colono->CacheInitialMeshOffset(Nuovo.GetLocation(), Nuovo.Rotator());
	if (Inclinazione <= 0.f)
	{
		Mesh->SetRelativeLocationAndRotation(BasePosto, BaseVerso);
		Colono->CacheInitialMeshOffset(BasePosto, BaseVerso);   // di nuovo dritto, esattamente com'era
		bBaseSalvata = false;
	}
	if (CVarMovenzeDiagnosi.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Movenze: %s, nell'animazione già steso %.0f gradi, inclinato %.0f (obiettivo %.0f)"),
			*Colono->GetName(), Gia, Inclinazione, Obiettivo);
	}
}

void UValdorsoMovenzeComponent::Cambia(EValdorsoMovenza Nuova, float Ritmo)
{
	const ACharacter* Colono = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Colono ? Colono->GetMesh() : nullptr;
	UAnimInstance* Anima = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!Anima)
	{
		return;
	}
	const int32 Indice = static_cast<int32>(Nuova);
	UAnimSequence* Sequenza = Animazioni.IsValidIndex(Indice) ? Animazioni[Indice].Get() : nullptr;
	if (Nuova != EValdorsoMovenza::Nessuna && !Sequenza)
	{
		Nuova = EValdorsoMovenza::Nessuna;          // l'animazione manca: si resta come prima
	}

	// Il nostro montaggio può essere stato fermato da un altro (il dormire, un colpo): allora si riparte.
	const bool bVa = Montaggio && Anima->Montage_IsPlaying(Montaggio);
	if (Nuova == Adesso && (Nuova == EValdorsoMovenza::Nessuna || bVa))
	{
		if (bVa)
		{
			Anima->Montage_SetPlayRate(Montaggio, Ritmo);
		}
		return;
	}
	if (Nuova == EValdorsoMovenza::Nessuna)
	{
		if (bVa)
		{
			Anima->Montage_Stop(SfumaMovenze, Montaggio);
		}
		Montaggio = nullptr;
	}
	else
	{
		// in giro continuo (tanti giri): lo ferma il cambio di movenza
		Montaggio = Anima->PlaySlotAnimationAsDynamicMontage(Sequenza, TEXT("DefaultSlot"), SfumaMovenze, SfumaMovenze, Ritmo, 100000);
	}
	if (CVarMovenzeDiagnosi.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Movenze: %s da %d a %d (ritmo %.2f)"), *GetOwner()->GetName(),
			static_cast<int32>(Adesso), static_cast<int32>(Nuova), Ritmo);
	}
	Adesso = Nuova;
}
