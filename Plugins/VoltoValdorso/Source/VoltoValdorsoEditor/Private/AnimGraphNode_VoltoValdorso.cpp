// Valdorso - Il nodo "Volto Valdorso" come appare nell'AnimGraph. Scritto da Claude il 17/10/2026.

#include "AnimGraphNode_VoltoValdorso.h"

#define LOCTEXT_NAMESPACE "VoltoValdorso"

FText UAnimGraphNode_VoltoValdorso::GetControllerDescription() const
{
	return LOCTEXT("Descrizione", "Volto Valdorso");
}

FText UAnimGraphNode_VoltoValdorso::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (Node.Tabella && TitleType != ENodeTitleType::ListView && TitleType != ENodeTitleType::MenuTitle)
	{
		return FText::Format(LOCTEXT("TitoloConTabella", "Volto Valdorso\nTabella: {0}"), FText::FromString(Node.Tabella->GetName()));
	}
	return GetControllerDescription();
}

FText UAnimGraphNode_VoltoValdorso::GetTooltipText() const
{
	return LOCTEXT("Suggerimento", "Applica i cursori del volto leggendo la tabella: ogni riga sposta o scala un osso in base alla variabile Float con il nome del cursore.");
}

#undef LOCTEXT_NAMESPACE
