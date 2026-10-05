// Valdorso - La firma vera (passo 4.2b, idea di Claude approvata da Fra il 05/10/2026). Scritto da Claude il 05/10/2026.
//
// Un riquadro dove il giocatore disegna la sua firma: con il mouse (tieni premuto il tasto sinistro e scrivi) o con il
// controller (lo stick sinistro muove la penna, A tenuto premuto la appoggia sulla carta). La firma si salva come
// tratti di penna (ValdorsoRegistro::LeggiFirma) e si può ridisegnare uguale ovunque: Registro, Patto, contratti,
// lettere al futuro, testamento. "@" è la firma con il nome, scritta in calligrafia.
// Con bModificabile falso il riquadro mostra soltanto la firma.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Fonts/SlateFontInfo.h"

DECLARE_DELEGATE_OneParam(FValdorsoSuFirma, const FString& /*Firma*/);

class VALDORSO_API SValdorsoFirma : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoFirma) : _bModificabile(true), _Grandezza(FVector2D(520.f, 130.f)) {}
		/** La firma di partenza (vuota, "@" o i tratti). */
		SLATE_ARGUMENT(FString, Firma)
		/** Il nome da scrivere in calligrafia quando la firma è "@". */
		SLATE_ARGUMENT(FString, Nome)
		SLATE_ARGUMENT(FSlateFontInfo, FontNome)
		SLATE_ARGUMENT(bool, bModificabile)
		SLATE_ARGUMENT(FVector2D, Grandezza)
		/** Ogni volta che un tratto finisce (o la firma cambia): la firma nuova. */
		SLATE_EVENT(FValdorsoSuFirma, OnCambia)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Cambia la firma da fuori ("" per cancellare, "@" per firmare con il nome). Non chiama OnCambia. */
	void ImpostaFirma(const FString& Nuova);

	/** La firma come testo, pronta per il server. */
	FString Firma() const;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return Grandezza; }

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool SupportsKeyboardFocus() const override { return bModificabile; }
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
	/** Se il fuoco o il mouse se ne vanno a metà tratto, il tratto si chiude (e la penna si alza). */
	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

private:
	/** Un punto nuovo (da 0 a 1 nel riquadro) nel tratto di adesso, se si è mosso abbastanza. */
	void AggiungiPunto(const FVector2f& Punto, bool bSempre = false);
	void IniziaTratto(const FVector2f& Punto);
	void FinisciTratto();
	int32 ContaPunti() const;

	/** I tratti, con i punti da 0 a 1 (x verso destra, y verso il basso). */
	TArray<TArray<FVector2f>> Tratti;
	bool bColNome = false;
	bool bDisegnando = false;

	FString Nome;
	FSlateFontInfo FontNome;
	bool bModificabile = true;
	FVector2D Grandezza = FVector2D(520.f, 130.f);
	FValdorsoSuFirma OnCambia;

	/** La penna del controller: dove sta e come la spinge lo stick. */
	FVector2f Penna = FVector2f(0.1f, 0.6f);
	FVector2f Spinta = FVector2f::ZeroVector;
	bool bPennaGiu = false;
	bool bPennaVista = false;
};
