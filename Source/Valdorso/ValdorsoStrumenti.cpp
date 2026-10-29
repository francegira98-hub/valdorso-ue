// Valdorso - Strumenti per gli script dell'editor. Scritto da Claude il 29/10/2026. Vedi ValdorsoStrumenti.h.

#include "ValdorsoStrumenti.h"

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
