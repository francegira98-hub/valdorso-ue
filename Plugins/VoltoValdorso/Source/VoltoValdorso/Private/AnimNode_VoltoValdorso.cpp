// Valdorso - Il nodo "Volto Valdorso" dell'AnimGraph. Scritto da Claude il 17/10/2026.

#include "AnimNode_VoltoValdorso.h"
#include "VoltoValdorsoRiga.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogVoltoValdorso, Log, All);

FAnimNode_VoltoValdorso::FAnimNode_VoltoValdorso()
{
}

void FAnimNode_VoltoValdorso::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	FAnimNode_SkeletalControlBase::OnInitializeAnimInstance(InProxy, InAnimInstance);
	Prepara(InAnimInstance);
}

void FAnimNode_VoltoValdorso::Prepara(const UAnimInstance* InAnimInstance)
{
	NomiCursori.Reset();
	ValoriCursori.Reset();
	VariabiliCursori.Reset();
	Ossa.Reset();
	TabellaPreparata = Tabella;
	ClassePreparata = InAnimInstance ? InAnimInstance->GetClass() : nullptr;

	if (!Tabella)
	{
		return;
	}
	if (Tabella->GetRowStruct() != FVoltoValdorsoRiga::StaticStruct())
	{
		UE_LOG(LogVoltoValdorso, Warning, TEXT("Volto Valdorso: la tabella %s non è di tipo VoltoValdorsoRiga."), *Tabella->GetName());
		return;
	}

	const FReferenceSkeleton* Scheletro = nullptr;
	if (InAnimInstance)
	{
		if (const USkeletalMeshComponent* Componente = InAnimInstance->GetSkelMeshComponent())
		{
			if (const USkeletalMesh* Mesh = Componente->GetSkeletalMeshAsset())
			{
				Scheletro = &Mesh->GetRefSkeleton();
			}
		}
	}

	TMap<FName, int32> IndiceOsso;
	for (const TPair<FName, uint8*>& Coppia : Tabella->GetRowMap())
	{
		const FVoltoValdorsoRiga* Riga = reinterpret_cast<const FVoltoValdorsoRiga*>(Coppia.Value);
		if (!Riga || Riga->Cursore.IsNone() || Riga->Osso.IsNone())
		{
			continue;
		}
		if (Scheletro && Scheletro->FindBoneIndex(Riga->Osso) == INDEX_NONE)
		{
			UE_LOG(LogVoltoValdorso, Warning, TEXT("Volto Valdorso: riga %s, l'osso %s non esiste in questa mesh."), *Coppia.Key.ToString(), *Riga->Osso.ToString());
			continue;
		}

		const int32 Cursore = NomiCursori.AddUnique(Riga->Cursore);

		int32* Trovato = IndiceOsso.Find(Riga->Osso);
		if (!Trovato)
		{
			FOssoVolto Nuovo;
			Nuovo.Osso = FBoneReference(Riga->Osso);
			Trovato = &IndiceOsso.Add(Riga->Osso, Ossa.Add(MoveTemp(Nuovo)));
		}

		FEffetto Effetto;
		Effetto.Cursore = Cursore;
		Effetto.Spostamento = Riga->SpostamentoPerUnita;
		Effetto.Scala = Riga->ScalaPerUnita;
		Ossa[*Trovato].Effetti.Add(Effetto);
	}

	ValoriCursori.SetNumZeroed(NomiCursori.Num());
	VariabiliCursori.SetNumZeroed(NomiCursori.Num());

	if (ClassePreparata)
	{
		for (int32 i = 0; i < NomiCursori.Num(); ++i)
		{
			const FProperty* Variabile = ClassePreparata->FindPropertyByName(NomiCursori[i]);
			if (Variabile && (Variabile->IsA<FDoubleProperty>() || Variabile->IsA<FFloatProperty>()))
			{
				VariabiliCursori[i] = Variabile;
			}
			else
			{
				UE_LOG(LogVoltoValdorso, Warning, TEXT("Volto Valdorso: nell'Anim Blueprint manca la variabile Float \"%s\" (il cursore resta a 0)."), *NomiCursori[i].ToString());
			}
		}
	}

	UE_LOG(LogVoltoValdorso, Log, TEXT("Volto Valdorso: %d cursori, %d ossa dalla tabella %s."), NomiCursori.Num(), Ossa.Num(), *Tabella->GetName());
}

void FAnimNode_VoltoValdorso::PreUpdate(const UAnimInstance* InAnimInstance)
{
	FAnimNode_SkeletalControlBase::PreUpdate(InAnimInstance);

	if (!InAnimInstance)
	{
		return;
	}
	if (InAnimInstance->GetClass() != ClassePreparata || Tabella != TabellaPreparata)
	{
		// Cambiati classe o tabella (succede nell'editor): le ossa si ripreparano alla prossima inizializzazione.
		// Per ora azzeriamo i valori, così non si muove niente di sbagliato.
		for (float& Valore : ValoriCursori)
		{
			Valore = 0.f;
		}
		return;
	}

	for (int32 i = 0; i < VariabiliCursori.Num(); ++i)
	{
		float Valore = 0.f;
		if (const FProperty* Variabile = VariabiliCursori[i])
		{
			if (const FDoubleProperty* Doppio = CastField<FDoubleProperty>(Variabile))
			{
				Valore = static_cast<float>(Doppio->GetPropertyValue_InContainer(InAnimInstance));
			}
			else if (const FFloatProperty* Singolo = CastField<FFloatProperty>(Variabile))
			{
				Valore = Singolo->GetPropertyValue_InContainer(InAnimInstance);
			}
		}
		ValoriCursori[i] = bLimitaTraMenoUnoEUno ? FMath::Clamp(Valore, -1.f, 1.f) : Valore;
	}
}

void FAnimNode_VoltoValdorso::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	for (FOssoVolto& Osso : Ossa)
	{
		Osso.Osso.Initialize(RequiredBones);
	}

	// In ordine dalla radice verso le punte, come vuole la posa nello spazio del componente.
	Ossa.Sort([&RequiredBones](const FOssoVolto& A, const FOssoVolto& B)
	{
		return A.Osso.GetCompactPoseIndex(RequiredBones).GetInt() < B.Osso.GetCompactPoseIndex(RequiredBones).GetInt();
	});
}

bool FAnimNode_VoltoValdorso::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	return Ossa.Num() > 0 && ValoriCursori.Num() > 0;
}

void FAnimNode_VoltoValdorso::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	const FBoneContainer& Contenitore = Output.Pose.GetPose().GetBoneContainer();

	// Un osso alla volta: così, se si muovono un padre e un figlio, il figlio parte dalla posizione nuova del padre
	// (come con una fila di nodi Transform (Modify) Bone). OutBoneTransforms resta vuoto: la posa è già aggiornata.
	for (const FOssoVolto& Osso : Ossa)
	{
		if (!Osso.Osso.IsValidToEvaluate(Contenitore))
		{
			continue;
		}

		FVector Spostamento = FVector::ZeroVector;
		FVector Scala = FVector::OneVector;
		bool bCambia = false;
		for (const FEffetto& Effetto : Osso.Effetti)
		{
			const float Valore = ValoriCursori.IsValidIndex(Effetto.Cursore) ? ValoriCursori[Effetto.Cursore] : 0.f;
			if (FMath::IsNearlyZero(Valore))
			{
				continue;
			}
			Spostamento += Effetto.Spostamento * Valore;
			Scala *= (FVector::OneVector + Effetto.Scala * Valore);
			bCambia = true;
		}
		if (!bCambia)
		{
			continue;
		}

		const FCompactPoseBoneIndex Indice = Osso.Osso.GetCompactPoseIndex(Contenitore);
		FTransform Trasformazione = Output.Pose.GetComponentSpaceTransform(Indice);
		Trasformazione.AddToTranslation(Spostamento);
		// La scala non scende mai sotto l'1%: un osso a scala 0 rovinerebbe la posa.
		Trasformazione.SetScale3D(Trasformazione.GetScale3D() * FVector::Max(Scala, FVector(0.01)));

		Appoggio.Reset();
		Appoggio.Add(FBoneTransform(Indice, Trasformazione));
		Output.Pose.LocalBlendCSBoneTransforms(Appoggio, ActualAlpha);
	}
}

void FAnimNode_VoltoValdorso::GatherDebugData(FNodeDebugData& DebugData)
{
	FString Riga = DebugData.GetNodeName(this);
	Riga += FString::Printf(TEXT("(cursori: %d, ossa: %d)"), NomiCursori.Num(), Ossa.Num());
	DebugData.AddDebugItem(Riga);
	ComponentPose.GatherDebugData(DebugData);
}
