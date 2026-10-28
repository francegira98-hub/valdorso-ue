// Valdorso - Il nodo "Volto Valdorso" dell'AnimGraph. Scritto da Claude il 17/10/2026.
//
// Al posto di tanti nodi Transform (Modify) Bone fatti a mano, questo nodo legge una tabella (righe
// FVoltoValdorsoRiga: cursore -> osso -> spostamento e scala per unità) e muove tutte le ossa del volto insieme.
// I valori dei cursori (-1..1) li prende dalle variabili Float dell'Anim Blueprint che hanno lo stesso nome del
// cursore (NasoLunghezza, Orecchie, ...). Una voce nuova = una riga nella tabella + una variabile Float.
// Le ossa si muovono come con Transform (Modify) Bone in Add to Existing e Component Space: le figlie seguono.

#pragma once

#include "CoreMinimal.h"
#include "BoneContainer.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_VoltoValdorso.generated.h"

class UDataTable;
class UAnimInstance;

USTRUCT(BlueprintInternalUseOnly)
struct VOLTOVALDORSO_API FAnimNode_VoltoValdorso : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	// La tabella dei cursori (righe di tipo VoltoValdorsoRiga), es. DT_VoltoValdorso.
	UPROPERTY(EditAnywhere, Category = "Volto")
	TObjectPtr<UDataTable> Tabella = nullptr;

	// Limita i valori dei cursori tra -1 e 1 (consigliato: è il campo del catalogo).
	UPROPERTY(EditAnywhere, Category = "Volto")
	bool bLimitaTraMenoUnoEUno = true;

	FAnimNode_VoltoValdorso();

	// FAnimNode_Base
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual bool HasPreUpdate() const override { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	// FAnimNode_SkeletalControlBase
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

private:
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

	// Prepara cursori e ossa dalla tabella e trova le variabili dell'Anim Blueprint (sul thread del gioco).
	void Prepara(const UAnimInstance* InAnimInstance);

	struct FEffetto
	{
		int32 Cursore = INDEX_NONE;
		FVector Spostamento = FVector::ZeroVector;
		FVector Scala = FVector::ZeroVector;
	};

	struct FOssoVolto
	{
		FBoneReference Osso;
		TArray<FEffetto> Effetti;
	};

	TArray<FName> NomiCursori;
	TArray<float> ValoriCursori;
	TArray<const FProperty*> VariabiliCursori;
	TArray<FOssoVolto> Ossa;
	const UClass* ClassePreparata = nullptr;
	const UDataTable* TabellaPreparata = nullptr;
	TArray<FBoneTransform> Appoggio;
};
