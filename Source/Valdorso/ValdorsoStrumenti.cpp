// Valdorso - Strumenti per gli script dell'editor. Scritto da Claude il 29/10/2026. Vedi ValdorsoStrumenti.h.

#include "ValdorsoStrumenti.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#include "WorldPartition/WorldPartition.h"

#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

void UValdorsoStrumenti::CostruisciSubito(UObject* Asset)
{
#if WITH_EDITOR
	if (!Asset)
	{
		return;
	}
	// "È cambiato": l'asset si ricostruisce (il legame dei capelli rifà il trasferimento sul volto).
	Asset->Modify();
	Asset->PostEditChange();
	// E si aspetta che la costruzione in sottofondo finisca.
	FAssetCompilingManager::Get().FinishAllCompilation();
#endif
}

namespace
{
	/** La proprieta' intera "LoadingRange" di un oggetto (nullptr se non c'e'). */
	FIntProperty* StrumentiRaggioDi(const UObject* Oggetto)
	{
		return Oggetto ? FindFProperty<FIntProperty>(Oggetto->GetClass(), TEXT("LoadingRange")) : nullptr;
	}

	/** Porta il raggio di un oggetto (una partizione) ad almeno Minimo. */
	void StrumentiAlmeno(UObject* Oggetto, int32 Minimo, const FString& Nome, TArray<FString>& Fatto)
	{
		FIntProperty* Raggio = StrumentiRaggioDi(Oggetto);
		if (!Raggio)
		{
			return;
		}
		const int32 Vecchio = Raggio->GetPropertyValue_InContainer(Oggetto);
		if (Vecchio >= Minimo)
		{
			Fatto.Add(FString::Printf(TEXT("%s: distanza di caricamento %d cm (resta)"), *Nome, Vecchio));
			return;
		}
#if WITH_EDITOR
		Oggetto->Modify();
#endif
		Raggio->SetPropertyValue_InContainer(Oggetto, Minimo);
#if WITH_EDITOR
		FPropertyChangedEvent Cambio(Raggio);
		Oggetto->PostEditChangeProperty(Cambio);
#endif
		Fatto.Add(FString::Printf(TEXT("%s: distanza di caricamento da %d a %d cm"), *Nome, Vecchio, Minimo));
	}
}

TArray<FString> UValdorsoStrumenti::DistanzaCaricamento(UObject* Mondo, int32 Minimo)
{
	TArray<FString> Fatto;
	UWorld* World = Mondo ? Mondo->GetWorld() : nullptr;
	UWorldPartition* Partizione = World ? World->GetWorldPartition() : nullptr;
	if (!Partizione)
	{
		return Fatto;
	}
	// il runtime hash: per nome, come lo vede il pannello (in 5.8 da Python non si legge)
	UObject* Hash = nullptr;
	for (TFieldIterator<FObjectPropertyBase> It(Partizione->GetClass()); It; ++It)
	{
		UObject* Valore = It->GetObjectPropertyValue_InContainer(Partizione);
		const FString Classe = Valore ? Valore->GetClass()->GetName() : FString();
		if (Classe.Contains(TEXT("RuntimeHash")) || Classe.Contains(TEXT("SpatialHash")))
		{
			Hash = Valore;
			break;
		}
	}
	if (!Hash)
	{
		return Fatto;
	}
	// 1. il Runtime Hash Set (5.8): RuntimePartitions[0].MainPartition (o MainLayer) -> LoadingRange
	if (FArrayProperty* Partizioni = FindFProperty<FArrayProperty>(Hash->GetClass(), TEXT("RuntimePartitions")))
	{
		const FStructProperty* Dentro = CastField<FStructProperty>(Partizioni->Inner);
		FScriptArrayHelper Elenco(Partizioni, Partizioni->ContainerPtrToValuePtr<void>(Hash));
		if (Dentro && Elenco.Num() > 0)
		{
			void* Prima = Elenco.GetRawPtr(0);
			for (TFieldIterator<FObjectPropertyBase> It(Dentro->Struct); It; ++It)
			{
				UObject* Valore = It->GetObjectPropertyValue_InContainer(Prima);
				if (StrumentiRaggioDi(Valore))
				{
					StrumentiAlmeno(Valore, Minimo, FString::Printf(TEXT("partizione principale (%s)"), *It->GetName()), Fatto);
				}
			}
		}
	}
	// 2. lo Spatial Hash: Grids[0].LoadingRange
	if (FArrayProperty* Griglie = FindFProperty<FArrayProperty>(Hash->GetClass(), TEXT("Grids")))
	{
		const FStructProperty* Dentro = CastField<FStructProperty>(Griglie->Inner);
		FScriptArrayHelper Elenco(Griglie, Griglie->ContainerPtrToValuePtr<void>(Hash));
		FIntProperty* Raggio = Dentro ? FindFProperty<FIntProperty>(Dentro->Struct, TEXT("LoadingRange")) : nullptr;
		if (Raggio && Elenco.Num() > 0)
		{
			void* Prima = Elenco.GetRawPtr(0);
			const int32 Vecchio = Raggio->GetPropertyValue_InContainer(Prima);
			if (Vecchio < Minimo)
			{
#if WITH_EDITOR
				Hash->Modify();
#endif
				Raggio->SetPropertyValue_InContainer(Prima, Minimo);
				Fatto.Add(FString::Printf(TEXT("griglia principale: distanza di caricamento da %d a %d cm"), Vecchio, Minimo));
			}
			else
			{
				Fatto.Add(FString::Printf(TEXT("griglia principale: distanza di caricamento %d cm (resta)"), Vecchio));
			}
		}
	}
	return Fatto;
}
