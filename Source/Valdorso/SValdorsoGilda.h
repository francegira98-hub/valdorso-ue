// Valdorso - La schermata della Gilda: la bacheca degli incarichi e il bancone di Lucia (v0.1.4, 10/11/2026). Scritto
// da Claude. Vedi ValdorsoGilda.h e ValdorsoGildaDati.h.
//
// La bacheca: i fogli appesi, ognuno con il sigillo di ceralacca del grado (rosso Comune, Bronzo, Argento, Oro), il
// titolo, la paga e la scadenza (gialli quelli che scadono presto). Clic su un foglio: a destra si legge tutto, e
// "Stacca il foglio" (il server controlla grado, posti e zaino).
// Il bancone di Lucia: i fogli che hai preso, con cosa manca. "Fatti assegnare" (con la cauzione per Argento e Oro),
// "Compiuto" quando il lavoro e' fatto, "Rinuncia" (due clic: da assegnato costa punti).
// Si chiude con Esc o E, o allontanandosi.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "ValdorsoGildaDati.h"

class APlayerController;
class FValdorsoCaratteri;
class SBox;
class UValdorsoGildaComponent;
class UValdorsoZainoComponent;

class VALDORSO_API SValdorsoGilda : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoGilda) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(bool, Bancone)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	bool EBancone() const { return bBancone; }

	/** Rifa' il contenuto (sono arrivati fogli nuovi, o la Gilda e' cambiata). */
	void Rifai();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedRef<SWidget> Bacheca();
	TSharedRef<SWidget> Bancone();
	TSharedRef<SWidget> Foglio(const FValdorsoIncarico& Incarico, int64 Adesso);
	TSharedRef<SWidget> Dettaglio(int64 Adesso);
	TSharedRef<SWidget> RigaPreso(const FValdorsoIncaricoPreso& Preso, int64 Adesso);
	TSharedRef<SWidget> Sigillo(EValdorsoGrado Grado, float Lato);
	FText TestoGrado() const;

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoGildaComponent> Gilda;
	TWeakObjectPtr<UValdorsoZainoComponent> Zaino;
	bool bBancone = false;
	FSimpleDelegate OnChiudi;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	TSharedPtr<SBox> Contenuto;
	FSlateBrush Carta;
	FSlateBrush Velo;
	FSlateBrush Riquadro;
	FSlateBrush RiquadroGiallo;
	FSlateBrush RiquadroScelto;
	FSlateBrush Assi;
	FSlateBrush Sigilli[4];
	/** Il foglio scelto sulla bacheca (0 = nessuno), e quello a cui si sta rinunciando (secondo clic). */
	int32 Scelto = 0;
	int32 RinunciaChiesta = 0;
	double RinunciaFino = 0.0;
	FVector Partenza = FVector::ZeroVector;
	/** Un clic ha cambiato la scelta: si rifa' al prossimo tick (non dentro il clic). */
	bool bDaRifare = false;
	int32 VistoGilda = -1;
	int32 VistoZaino = -1;
};

namespace ValdorsoGildaSchermo
{
	/** Vero se su questo PC la schermata della Gilda e' aperta. */
	VALDORSO_API bool Aperta(const APlayerController* Controllore);

	/** Apre la bacheca (bBancone falso) o il bancone di Lucia, se non c'e' altro aperto. */
	VALDORSO_API bool Apri(APlayerController* Controllore, bool bBancone);

	/** Rifa' la schermata aperta (i fogli nuovi). */
	VALDORSO_API void Rifai(APlayerController* Controllore);

	/** La chiude e torna al gioco. */
	VALDORSO_API void Chiudi(APlayerController* Controllore);
}
