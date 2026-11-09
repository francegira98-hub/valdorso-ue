// Valdorso - La tessera del colono: quello che si vede (09/11/2026, chiesta da Fra). Scritto da Claude.
//
// Un foglio di carta pesante, come un documento vero: in alto "Corona di Aurelia · Valle dell'Orso", a sinistra il
// ritratto (la faccia del colono, fotografata in quel momento dal suo corpo nella valle, in toni seppia) con sotto il
// numero; a destra nome, età e provenienza, il vecchio mestiere, la fede, la qualifica e dove è stata rilasciata; in
// basso la firma vera (quella del Registro) e il timbro rosso della capitana. Si chiude con Esc, E, I o Invio (X sul controller la mostra).
// La propria tessera ha in fondo "Mostrala a chi hai davanti" (UValdorsoZainoComponent::ServerMostraTessera); quella
// che un altro ti mostra dice "<nome> ti mostra la sua tessera".
// La foto: una telecamera (SceneCapture) davanti alla testa del colono, solo lui e i pezzi attaccati (il MetaHuman),
// senza luci del mondo (così di notte si vede uguale). Se il colono non c'è (troppo lontano), al posto della faccia
// una sagoma.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
// (09/11) TStrongObjectPtr vuole i tipi completi (chi include questo file li deve conoscere): non bastano le dichiarazioni
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ValdorsoTessera.h"

class APawn;
class APlayerController;
class FValdorsoStileAccesso;
class USceneCaptureComponent2D;
class UGroomComponent;

/** Il timbro rotondo della capitana: due cerchi rossi, una stella e il testo nel mezzo. */
class VALDORSO_API SValdorsoTimbro : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoTimbro) {}
		SLATE_ARGUMENT(FString, Testo)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(118.f, 118.f); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	FString Testo;
	TSharedPtr<class FValdorsoCaratteri> Caratteri;
};

class VALDORSO_API SValdorsoTessera : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoTessera) : _bPropria(true) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(FValdorsoTesseraVista, Vista)
		/** Il colono della tessera (per la foto; può mancare). */
		SLATE_ARGUMENT(TWeakObjectPtr<APawn>, Di)
		SLATE_ARGUMENT(bool, bPropria)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SValdorsoTessera();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	/** La foto del colono: prepara la telecamera e scatta (di nuovo dopo un attimo, quando il volto è caricato). */
	void Fotografa();
	EActiveTimerReturnType Scatta(double Tempo, float Delta);
	/** Legge la pellicola e stampa la foto in seppia (lo sfondo diventa carta). Falso se la pellicola è vuota. */
	bool Sviluppa();
	/** Via la telecamera, e i capelli tornano come prima. */
	void FineScatti();
	/** Mostra la tessera a chi si ha davanti (il server sceglie) e chiude. */
	void Mostra();
	/** Una riga "Etichetta: valore" della tessera. */
	TSharedRef<SWidget> Riga(const FText& Etichetta, const FString& Valore, bool bForte = false);

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<APawn> Di;
	FValdorsoTesseraVista Vista;
	bool bPropria = true;
	FSimpleDelegate OnChiudi;
	TSharedPtr<FValdorsoStileAccesso> Stile;
	FSlateBrush Carta;
	FSlateBrush Velo;
	FSlateBrush Cornice;
	FSlateBrush Foto;
	TStrongObjectPtr<UTextureRenderTarget2D> Pellicola;
	TStrongObjectPtr<UTexture2D> Stampa;
	TWeakObjectPtr<USceneCaptureComponent2D> Obiettivo;
	/** I capelli messi a ciocche per gli scatti. */
	TArray<TWeakObjectPtr<UGroomComponent>> Capelli;
	int32 ScattiFatti = 0;
	bool bFotoPronta = false;
};

namespace ValdorsoTesseraSchermo
{
	/** Vero se su questo PC è aperta una tessera. */
	VALDORSO_API bool Aperta(const APlayerController* Controllore);

	/** Apre una tessera (la propria o quella mostrata da Di). Se c'è altro aperto (non lo zaino), solo un avviso. */
	VALDORSO_API void Apri(APlayerController* Controllore, const FValdorsoTesseraVista& Vista, APawn* Di, bool bPropria);

	/** La chiude e torna al gioco. */
	VALDORSO_API void Chiudi(APlayerController* Controllore);
}
