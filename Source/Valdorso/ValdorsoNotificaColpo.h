// Valdorso - La notifica "Valdorso: Colpo".
// Si mette nel montaggio di un attacco, nel fotogramma in cui il colpo arriva (per il pugno: il braccio disteso).
// Quando l'animazione passa di lì, avvisa l'abilità con l'evento Valdorso.Evento.Colpo,
// e l'abilità (sul server) guarda chi è stato colpito.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ValdorsoNotificaColpo.generated.h"

UCLASS(meta = (DisplayName = "Valdorso: Colpo"))
class VALDORSO_API UValdorsoNotificaColpo : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
