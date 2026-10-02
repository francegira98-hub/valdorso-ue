// Valdorso - Il registro eventi del mondo (vedi il .h).

#include "ValdorsoRegistroEventi.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Valdorso.h"

namespace ValdorsoRegistroEventi
{
	void Annota(const FString& Tipo, const AActor* Chi, const AActor* Bersaglio, const FVector& Dove, const FString& Dettagli)
	{
		// Solo il server annota: il PC di un giocatore non decide cosa è successo nel mondo.
		if (Chi == nullptr || !Chi->HasAuthority())
		{
			return;
		}

		const UWorld* Mondo = Chi->GetWorld();
		const float Quando = Mondo ? Mondo->GetTimeSeconds() : 0.f;

		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Registro: %s | chi %s | bersaglio %s | dove %s | quando %.2f s | %s"),
			*Tipo,
			*GetNameSafe(Chi),
			Bersaglio ? *GetNameSafe(Bersaglio) : TEXT("nessuno"),
			*Dove.ToCompactString(),
			Quando,
			*Dettagli);
	}
}
