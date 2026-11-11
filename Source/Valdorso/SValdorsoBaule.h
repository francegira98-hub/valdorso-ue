// Valdorso - La schermata del baule del colono (v0.2.0, 12/11/2026). Scritto da Claude. Vedi ValdorsoBaule.h.
//
// Una pergamena come la bottega: a sinistra quello che hai nello zaino (con il peso), a destra quello che c'e' nel
// baule; su ogni riga il disegno dell'oggetto, il nome, quanti, e il pulsante: "Metti" (nel baule) o "Prendi" (nello
// zaino). Il pulsante chiede al server; lo zaino che torna rifa' le righe. Si chiude con Esc, E o I, o allontanandosi.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class APlayerController;
class FValdorsoCaratteri;
class SScrollBox;
class UValdorsoZainoComponent;
struct FValdorsoPostoZaino;

class VALDORSO_API SValdorsoBaule : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoBaule) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(TWeakObjectPtr<UValdorsoZainoComponent>, Zaino)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/** Rifa' le due liste (quando lo zaino cambia). */
	void Rifai();
	TSharedRef<SWidget> Riga(const FValdorsoPostoZaino& Posto, int32 Indice, bool bNelBaule);
	TSharedRef<SWidget> Colonna(const FText& Titolo, TAttribute<FText> Sotto, TSharedRef<SScrollBox> Lista);

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoZainoComponent> Zaino;
	FSimpleDelegate OnChiudi;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	TSharedPtr<SScrollBox> ListaZaino;
	TSharedPtr<SScrollBox> ListaBaule;
	FSlateBrush Carta;
	FSlateBrush Velo;
	FSlateBrush Riquadro;
	FVector Partenza = FVector::ZeroVector;
	int32 RifattoA = -1;
};

namespace ValdorsoBauleSchermo
{
	/** Vero se su questo PC il baule e' aperto. */
	VALDORSO_API bool Aperto(const APlayerController* Controllore);

	/** Apre il baule (se non c'e' altro aperto). */
	VALDORSO_API bool Apri(APlayerController* Controllore);

	/** Lo chiude e torna al gioco. */
	VALDORSO_API void Chiudi(APlayerController* Controllore);
}
