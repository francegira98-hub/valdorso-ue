// Valdorso - Il nodo "Volto Valdorso" come appare nell'AnimGraph. Scritto da Claude il 17/10/2026.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimNode_VoltoValdorso.h"
#include "AnimGraphNode_VoltoValdorso.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_VoltoValdorso : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Impostazioni")
	FAnimNode_VoltoValdorso Node;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;

protected:
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
};
