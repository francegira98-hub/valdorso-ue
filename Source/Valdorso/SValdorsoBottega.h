// Valdorso - La schermata di una bottega (v0.1.4, 10/11/2026). Scritto da Claude. Vedi ValdorsoBotteghe.h.
//
// Una pergamena come lo zaino: il nome della bottega, gli Aureus, e una riga per ricetta: il disegno di quello che si
// ottiene, il suo nome, quello che serve (in rosso quello che manca) e il pulsante ("Compra", "Concia", "Cuoci",
// "Vendi"). Il pulsante chiede al server (UValdorsoZainoComponent::ServerRicetta); lo zaino che torna aggiorna le righe.
// Si chiude con Esc, E o I, o allontanandosi dal banco.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "ValdorsoBotteghe.h"

class APlayerController;
class FValdorsoCaratteri;
class UValdorsoZainoComponent;
struct FValdorsoRicetta;

class VALDORSO_API SValdorsoBottega : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoBottega) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(TWeakObjectPtr<UValdorsoZainoComponent>, Zaino)
		SLATE_ARGUMENT(EValdorsoTipoBanco, Banco)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	EValdorsoTipoBanco GetBanco() const { return Banco; }

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedRef<SWidget> Riga(const FValdorsoRicetta* Ricetta);
	/** Perche' non si puo' fare (vuoto = si puo'), rifatto solo quando lo zaino cambia. */
	FText MancaDi(const FValdorsoRicetta* Ricetta) const;
	FText TestoMonete() const;

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoZainoComponent> Zaino;
	EValdorsoTipoBanco Banco = EValdorsoTipoBanco::Fuoco;
	FSimpleDelegate OnChiudi;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FSlateBrush Carta;
	FSlateBrush Velo;
	FSlateBrush Riquadro;
	/** Dove era il colono quando si e' aperta (allontanandosi si chiude). */
	FVector Partenza = FVector::ZeroVector;
	mutable TMap<const FValdorsoRicetta*, FText> Mancanze;
	mutable int32 MancanzeDi = -1;
};

namespace ValdorsoBottegaSchermo
{
	/** Vero se su questo PC una bottega e' aperta. */
	VALDORSO_API bool Aperta(const APlayerController* Controllore);

	/** Apre la bottega di quel banco (se non c'e' altro aperto). */
	VALDORSO_API bool Apri(APlayerController* Controllore, EValdorsoTipoBanco Banco);

	/** La chiude e torna al gioco. */
	VALDORSO_API void Chiudi(APlayerController* Controllore);
}
