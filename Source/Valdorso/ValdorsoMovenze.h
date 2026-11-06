// Valdorso - Le movenze del colono sulla scala e in acqua (v0.1.3). Scritto da Claude il 06/11/2026.
//
// Come funziona:
// - Sulla scala a pioli (UValdorsoScalataComponent) e quando nuota (MOVE_Swimming) il colono fa l'animazione giusta:
//   sulla scala sale, scende o sta fermo aggrappato secondo la velocità in su o in giù; in acqua nuota avanti, di lato
//   o sta a galla secondo dove va. Le animazioni sono della Free Animation Library di Fab (voxel vision), adattate al
//   nostro manichino da Content/Python/nuovi/porta_movenze.py in /Game/Fab/FreeAnimationLibrary/Movenze.
// - È solo aspetto: lo fa ogni PC da sé (anche per gli altri coloni, dalla velocità e dallo stato che arrivano in rete),
//   mai il server dedicato. Usa lo slot DefaultSlot come il dormire, in giro continuo, e sfuma da una all'altra.
// - Se le animazioni non ci sono (script non lanciato), non fa niente.
// - (06/11, dopo la prova di Fra) Mentre nuota il colono si stende: la capsula resta in piedi (è il movimento di
//   Unreal), quindi si inclina il corpo in avanti intorno al centro della capsula, fino a 75 gradi meno quanto è già
//   steso nell'animazione (misurato da testa e bacino). A galla resta dritto. Per i coloni degli altri lo spostamento
//   passa da CacheInitialMeshOffset, come per il letto, così la rete non lo annulla.
// Comandi di console (per le prove): Valdorso.Movenze.Scala 1.0 (velocità dell'animazione della scala),
// Valdorso.Movenze.Nuoto 1.0 (velocità del nuoto), Valdorso.Movenze.Steso 75 (quanto si stende nuotando, gradi),
// Valdorso.Movenze.StesoSu 0 (cm: alza o abbassa il corpo steso), Valdorso.Movenze.Diagnosi 1 (scrive ogni cambio).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ValdorsoMovenze.generated.h"

class UAnimMontage;
class UAnimSequence;

/** Quale movenza fa il colono adesso. */
UENUM()
enum class EValdorsoMovenza : uint8
{
	Nessuna,
	ScalaSu,
	ScalaGiu,
	ScalaFerma,
	NuotoAvanti,
	NuotoSinistra,
	NuotoDestra,
	NuotoFermo
};

namespace ValdorsoMovenze
{
	/** Sulla scala: dalla velocità in su (cm/s). */
	VALDORSO_API EValdorsoMovenza Scala(float VelocitaZ);

	/** In acqua: dalla velocità vista dal colono (X avanti, Y a destra, cm/s). */
	VALDORSO_API EValdorsoMovenza Nuoto(const FVector& VelocitaLocale);

	/** Quanto inclinare il corpo (gradi) per stenderlo: Voluta meno quanto è già inclinato nell'animazione (Gia),
	 *  mai all'indietro. */
	VALDORSO_API float Inclinazione(float Voluta, float Gia);

	/** Il corpo inclinato di Gradi in avanti intorno a Perno (tutto visto dal colono: X avanti, Z su). */
	VALDORSO_API FTransform Inclinato(const FTransform& Base, float Gradi, const FVector& Perno);
}

UCLASS(ClassGroup = (Valdorso), meta = (BlueprintSpawnableComponent))
class VALDORSO_API UValdorsoMovenzeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UValdorsoMovenzeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	/** Le animazioni, una per movenza (nullptr se manca). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequence>> Animazioni;

	/** Il montaggio che sta andando adesso (fatto al volo dall'animazione). */
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> Montaggio;

	EValdorsoMovenza Adesso = EValdorsoMovenza::Nessuna;
	bool bCaricate = false;

	void Carica();
	void Cambia(EValdorsoMovenza Nuova, float Ritmo);

	/** Stende o raddrizza il corpo verso Voluta (gradi), sfumando. */
	void Stendi(float Voluta, float DeltaTime);

	/** Dove stava il corpo prima di inclinarlo (relativo al colono). */
	FVector BasePosto = FVector::ZeroVector;
	FRotator BaseVerso = FRotator::ZeroRotator;
	bool bBaseSalvata = false;
	float Inclinazione = 0.f;
};
