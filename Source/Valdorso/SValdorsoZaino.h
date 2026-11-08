// Valdorso - Lo zaino: quello che si vede (v0.1.4, seconda parte). Scritto da Claude il 08/11/2026.
//
// SValdorsoZaino        la pergamena dello zaino (tasto I): il titolo, il peso ("6,2 / 30 kg", rosso oltre la
//                       portata), gli Aureus, la griglia, a destra l'oggetto sotto il mouse (nome, due righe, peso,
//                       sorsi) e in fondo come si usa. Si chiude con I, Esc o Tab.
// SValdorsoGrigliaZaino la griglia 6 x 4 e, sotto, gli 8 tasti della barra delle azioni, tutto disegnato a penna:
//                       - clic su un oggetto: lo prendi in mano (segue il mouse); clic su un altro posto: lo metti lì
//                         (sulla stessa pila si unisce, su un altro oggetto si scambia); clic su un tasto 1-8: va nella
//                         barra; clic fuori o Esc: lo rimetti giù.
//                       - clic destro su un oggetto: lo usi (mangi, bevi, ti curi, leggi); su un tasto: lo vuoti.
//                       - col mouse sopra un oggetto: i numeri 1-8 lo mettono in quel tasto; Canc due volte lo getta.
// Il server decide tutto (UValdorsoZainoComponent): qui si chiede e si mostra quello che arriva.
// ValdorsoDisegnoOggetti::Disegna disegna un oggetto (lo usa anche la barra delle azioni, SValdorsoBarre).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class APlayerController;
class FValdorsoCaratteri;
class UValdorsoZainoComponent;
struct FGeometry;
class FSlateWindowElementList;

namespace ValdorsoDisegnoOggetti
{
	/** Disegna l'oggetto Id a penna dentro un quadrato di lato Lato (pixel) centrato in Centro. */
	VALDORSO_API void Disegna(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria, FName Id,
		const FVector2f& Centro, float Lato, const FLinearColor& Colore, float Spessore);

	/** Disegna l'Aureus. */
	VALDORSO_API void DisegnaMoneta(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria,
		const FVector2f& Centro, float Lato, const FLinearColor& Colore, float Spessore);
}

class VALDORSO_API SValdorsoGrigliaZaino : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoGrigliaZaino) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UValdorsoZainoComponent>, Zaino)
		/** Chi tiene la tastiera (la pergamena): dopo un clic la riprende, così 1-8, Canc ed Esc funzionano ancora. */
		SLATE_ARGUMENT(TWeakPtr<SWidget>, Fuoco)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static constexpr float LatoPosto = 72.f;
	static constexpr float Spazio = 8.f;
	static constexpr float LatoTasto = 54.f;
	static constexpr float SopraTasti = 34.f;

	/** Il posto da descrivere a destra: quello in mano, se no quello sotto il mouse (INDEX_NONE: niente). */
	int32 PostoMostrato() const;
	/** Il posto sotto il mouse (INDEX_NONE se no). */
	int32 PostoSotto() const { return Sotto; }
	/** Il posto in mano (INDEX_NONE se niente). */
	int32 PostoInMano() const { return InMano; }
	/** Rimette giù quello che si ha in mano; falso se non c'era niente. */
	bool Posa();

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

private:
	/** Dove sta un posto della griglia e un tasto della barra (in alto a sinistra, pixel). */
	static FVector2f AngoloPosto(int32 Posto);
	static FVector2f AngoloTasto(int32 Tasto);
	/** Cosa c'è sotto un punto: un posto (0..23), un tasto (100 + 0..7) o INDEX_NONE. */
	static int32 Colpito(const FVector2f& Punto);
	/** Un clic gestito: la tastiera torna alla pergamena. */
	FReply Gestito() const;

	TWeakObjectPtr<UValdorsoZainoComponent> Zaino;
	TWeakPtr<SWidget> Fuoco;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	FSlateBrush Casella;
	int32 Sotto = INDEX_NONE;
	int32 TastoSotto = INDEX_NONE;
	int32 InMano = INDEX_NONE;
	FVector2f Mouse = FVector2f::ZeroVector;
	bool bMouseDentro = false;
};

class VALDORSO_API SValdorsoZaino : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoZaino) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controllore)
		SLATE_ARGUMENT(TWeakObjectPtr<UValdorsoZainoComponent>, Zaino)
		SLATE_EVENT(FSimpleDelegate, OnChiudi)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	FText TestoPeso() const;
	FSlateColor ColorePeso() const;
	FText NomeMostrato() const;
	FText DescrizioneMostrata() const;
	FText DettagliMostrati() const;
	FText Suggerimento() const;

	TWeakObjectPtr<APlayerController> Controllore;
	TWeakObjectPtr<UValdorsoZainoComponent> Zaino;
	FSimpleDelegate OnChiudi;
	TSharedPtr<FValdorsoCaratteri> Caratteri;
	TSharedPtr<SValdorsoGrigliaZaino> Griglia;
	FSlateBrush Carta;
	FSlateBrush Velo;
	/** Canc va premuto due volte sullo stesso posto: il primo è questo, fino a questo momento. */
	int32 GettaPosto = INDEX_NONE;
	double GettaFino = 0.0;
};
