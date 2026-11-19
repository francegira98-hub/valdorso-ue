// Valdorso - Il contratto da firmare: quello che si vede (18/11/2026, chiesto da Fra). Scritto da Claude.
//
// Una pergamena al centro dello schermo, come quella del Registro: in cima chi lo emette, il titolo, le clausole una
// sotto l'altra, e in fondo il riquadro dove si firma davvero (SValdorsoFirma: con il mouse, o con lo stick e A).
// Sotto il foglio: "Firma e consegna" (si accende quando la firma c'e'), "Come sul Registro" (mette la firma che il
// colono ha gia' fatto sul Registro), "Cancella" e "Non firmo". Esc: non firmo.
// La firma parte verso il server (UValdorsoZainoComponent::ServerFirmaContratto), che decide: il contratto vale solo
// vicino a chi lo porge e per pochi minuti.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Texture2D.h"
#include "ValdorsoContratto.h"

class APlayerController;
class FValdorsoStileAccesso;
class SValdorsoFirma;

class VALDORSO_API SValdorsoContratto : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoContratto) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(FValdorsoContrattoVista, Vista)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/** Il riquadro della firma (per dargli il fuoco quando si apre). */
	TSharedPtr<SWidget> RiquadroFirma() const;

private:
	/** Manda la firma al server e chiude. */
	void Consegna();
	void ComeSulRegistro();
	void Cancella();

	TWeakObjectPtr<APlayerController> Controllore;
	FValdorsoContrattoVista Vista;
	FSimpleDelegate OnChiudi;
	TSharedPtr<FValdorsoStileAccesso> Stile;
	TSharedPtr<SValdorsoFirma> Firma;
	/** La firma di adesso ("" = ancora niente). */
	FString FirmaDiAdesso;
	TStrongObjectPtr<UTexture2D> TexturePergamena;
	FSlateBrush Pergamena;
	FSlateBrush Velo;
};

namespace ValdorsoContrattoSchermo
{
	/** Vero se su questo PC e' aperto un contratto. */
	VALDORSO_API bool Aperto(const APlayerController* Controllore);

	/** Apre il contratto. Falso se c'e' altro aperto (un dialogo che si sta chiudendo): chi chiama riprova. */
	VALDORSO_API bool Apri(APlayerController* Controllore, const FValdorsoContrattoVista& Vista);

	/** Lo chiude e torna al gioco. */
	VALDORSO_API void Chiudi(APlayerController* Controllore);
}
