// Valdorso - Le Rovine degli Antichi (v0.1.3): le rune che di notte si accendono e le iscrizioni. Scritto da Claude
// il 06/11/2026. Le rovine (la cinta spezzata, la colonnata, il cerchio delle cinque pietre, il tumulo con la lastra della
// cripta) le fa Tools/Generatori/villaggio_rovine.py e le mette crea_villaggio.py.
//
// - AValdorsoRuneNotte (uno per mondo, lo fa nascere la modalità di gioco; lavora solo sui PC): cerca le cose con
//   l'etichetta "ValdorsoRune" (le mette crea_villaggio.py), e di notte accende le parti fatte con il materiale delle
//   rune (MI_Villaggio_Runa: il suo parametro "Luce") e le loro luci, a tempo con il battito del Cuore: due colpi e
//   una pausa. Di giorno sono spente. Le cerca di nuovo ogni tanto (con World Partition arrivano quando ci si avvicina).
// - AValdorsoIscrizione: una bacheca senza la tavola di legno (si vede la pietra delle rovine): si legge con E
//   ("Leggi l'iscrizione") e mostra il testo come la bacheca.
// Comando di console: Valdorso.Rune.Accese 1 le accende anche di giorno (per le prove; 0 di serie).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoInterazione.h"
#include "ValdorsoRovine.generated.h"

class AValdorsoCielo;
class ULightComponent;
class UMaterialInstanceDynamic;

namespace ValdorsoRune
{
	/** Il battito delle rune (0..1): due colpi e una pausa ogni 1,6 secondi, come il frammento del tempio. */
	VALDORSO_API float Battito(float Secondi);

	/** Quanto brillano le rune (il parametro Luce del materiale): Notte 0..1 e il battito. */
	VALDORSO_API float Luce(float Notte, float Battito);
}

UCLASS()
class VALDORSO_API AValdorsoRuneNotte : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoRuneNotte();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FLuceRuna
	{
		TWeakObjectPtr<ULightComponent> Luce;
		float Intensita = 0.f;
	};

	void Cerca();

	TSet<TWeakObjectPtr<AActor>> Trovati;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Materiali;
	TArray<FLuceRuna> Luci;
	TWeakObjectPtr<AValdorsoCielo> Cielo;
	double ProssimaRicerca = 0.0;
	float Notte = 0.f;
};

/** Un'iscrizione sulle rovine: si legge come la bacheca, ma senza la tavola di legno. */
UCLASS()
class VALDORSO_API AValdorsoIscrizione : public AValdorsoBacheca
{
	GENERATED_BODY()

public:
	AValdorsoIscrizione();

	virtual FText TestoInterazione(const APawn* Chi) const override;
};
