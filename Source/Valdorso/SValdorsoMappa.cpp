// Valdorso - La mappa di carta (vedi il .h). Scritto da Claude il 06/11/2026.

#include "SValdorsoMappa.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoTemaUI.h"
#include "Engine/Texture2D.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoMappa"

const TCHAR* SValdorsoMappa::PercorsoTexture = TEXT("/Game/UI/Mappa/T_MappaValle.T_MappaValle");

void SValdorsoMappa::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	OnChiudi = InArgs._OnChiudi;
	Texture.Reset(LoadObject<UTexture2D>(nullptr, PercorsoTexture));
	if (Texture.IsValid())
	{
		Foglio.SetResourceObject(Texture.Get());
		Foglio.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		Foglio.DrawAs = ESlateBrushDrawType::Image;
	}

	TSharedRef<SWidget> Contenuto = Texture.IsValid()
		? StaticCastSharedRef<SWidget>(
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.RenderTransform(this, &SValdorsoMappa::Trasformazione)
			.RenderTransformPivot(FVector2D(0.5f, 0.5f))
			[
				SNew(SImage).Image(&Foglio)
			])
		: StaticCastSharedRef<SWidget>(
			SNew(STextBlock)
			.Text(LOCTEXT("Manca", "La mappa non c'è ancora: lanciare Content/Python/nuovi/crea_mappa.py nell'editor."))
			.Font(Stile->Caratteri->Testo(22.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::Pergamena()));

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&Stile->Velo)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(30.f, 30.f, 30.f, 70.f))
		[
			Contenuto
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(0.f, 0.f, 0.f, 22.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Aiuto", "Rotella: avvicina  ·  Trascina: sposta  ·  M: chiudi"))
			.Font(Stile->Caratteri->Testo(18.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::TestoSecondario())
		]
	];
}

TOptional<FSlateRenderTransform> SValdorsoMappa::Trasformazione() const
{
	return FSlateRenderTransform(FScale2f(Zoom), FVector2f(Spostamento));
}

FReply SValdorsoMappa::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::M || Tasto == EKeys::Escape || Tasto == EKeys::F10 || Tasto == EKeys::E
		|| Tasto == EKeys::Gamepad_FaceButton_Right || Tasto == EKeys::Gamepad_Special_Left)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoMappa::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bTrascina = true;
		return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
	}
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SValdorsoMappa::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bTrascina && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bTrascina = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SValdorsoMappa::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bTrascina)
	{
		// Lo spostamento è in pixel dello schermo divisi la scala dell'interfaccia (come il resto del widget).
		Spostamento += FVector2D(MouseEvent.GetCursorDelta()) / FMath::Max(0.01, static_cast<double>(MyGeometry.Scale));
		const float Limite = 900.f * Zoom;
		Spostamento.X = FMath::Clamp(Spostamento.X, -Limite, Limite);
		Spostamento.Y = FMath::Clamp(Spostamento.Y, -Limite, Limite);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SValdorsoMappa::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const float Prima = Zoom;
	Zoom = FMath::Clamp(Zoom * (MouseEvent.GetWheelDelta() > 0.f ? 1.2f : 1.f / 1.2f), 1.f, 4.f);
	if (FMath::IsNearlyEqual(Zoom, 1.f))
	{
		Spostamento = FVector2D::ZeroVector;
	}
	else
	{
		Spostamento *= Zoom / Prima;
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
