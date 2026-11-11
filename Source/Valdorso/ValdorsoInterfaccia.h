// Valdorso - L'interfaccia su misura e i sottotitoli dei suoni (v0.1.5 "Crescere", accessibilita', 11/11/2026). Scritto da
// Claude. Dalla Roadmap ("Interfaccia su misura: spostare e ingrandire barre, barra delle azioni, bussola e chat";
// "sottotitoli dei suoni importanti") e da Fra il 10/11 ("HUD spostabile", "sottotitoli dei suoni").
//
// Come funziona:
// - Sposta(Pezzo) e' lo spostamento (e la grandezza) che un pezzo dell'interfaccia prende dalle Impostazioni (Interfaccia:
//   barre, bussola, chat): lo applicano le barre (ValdorsoBarre.cpp), la bussola (ValdorsoTaccuino.cpp) e la chat
//   (ValdorsoChat.cpp). Vale subito, mentre si sposta.
// - I sottotitoli: quando sul PC suona un suono importante (un ululato, un ringhio, un colpo, la campana, le ali del
//   drago...), in basso compare una riga tra parentesi con da dove viene ("[Un ululato, davanti a te]"). Al massimo tre
//   righe, quattro secondi ciascuna; lo stesso suono di nuovo rinnova la riga invece di aggiungerne un'altra. Si spengono
//   nelle Impostazioni (Accessibilita'). Il nome della scritta viene dal nome del suono (S_Lupo_Ululato...).

#pragma once

#include "CoreMinimal.h"
#include "Rendering/SlateRenderTransform.h"
#include "Misc/Attribute.h"

class USoundBase;
class UWorld;

/** I pezzi dell'interfaccia che si spostano. */
enum class EValdorsoPezzoHud : uint8
{
	Barre,
	Bussola,
	Chat,
};

namespace ValdorsoInterfaccia
{
	/** Lo spostamento e la grandezza di un pezzo, da mettere sul widget (SetRenderTransform): si legge a ogni disegno. */
	VALDORSO_API TAttribute<TOptional<FSlateRenderTransform>> Sposta(EValdorsoPezzoHud Pezzo);

	/** La scritta di un suono ("Un ululato"), vuota se quel suono non ne ha una. */
	VALDORSO_API FText ScrittaDelSuono(const FString& NomeSuono);

	/** Da dove viene, rispetto a chi guarda ("davanti a te", "dietro di te", "a sinistra", "a destra"); Avanti in piano. */
	VALDORSO_API FText DaDove(const FVector& Occhio, const FVector& Avanti, const FVector& Suono);

	/** Un suono e' suonato su questo PC in quel punto (fino a Lontano cm): se e' importante e vicino, la sua scritta. */
	VALDORSO_API void SuonoSentito(UWorld* Mondo, const USoundBase* Suono, const FVector& Dove, float Lontano);

	/** Una scritta di suono gia' pronta (per i suoni senza un asset con il nome giusto: la voce del Cuore). */
	VALDORSO_API void Sottotitolo(UWorld* Mondo, const FText& Scritta, const FVector& Dove, bool bVicino = false);
}
