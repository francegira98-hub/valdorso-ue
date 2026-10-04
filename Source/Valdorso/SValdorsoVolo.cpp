// Valdorso - Le scritte sopra il volo verso la valle (vedi il .h).

#include "SValdorsoVolo.h"
#include "ValdorsoTemaUI.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "InputCoreTypes.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "ValdorsoVolo"

void SValdorsoVolo::Construct(const FArguments& InArgs)
{
	OnSalta = InArgs._OnSalta;
	Aiuto = LOCTEXT("Salta", "ESC  SALTA");
	Caratteri = MakeShared<FValdorsoCaratteri>();

	FSlateFontInfo FontTitolo = Caratteri->Titolo(58.f, TEXT("Regular"));
	FontTitolo.LetterSpacing = 140;

	FSlateFontInfo FontAiuto = Caratteri->Titolo(14.f, TEXT("Bold"));
	FontAiuto.LetterSpacing = 200;

	ChildSlot
	[
		SNew(SOverlay)

		// "La valle ti accoglie": batte con il Cuore, più forte del solito.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 0.f, 120.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Accoglie", "La valle ti accoglie"))
			.Font(FontTitolo)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity_Lambda([this]()
			{
				const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
				const FLinearColor Colore = FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo);
				return FSlateColor(Colore.CopyWithNewOpacity(OpacitaTitolo * (0.82f + 0.18f * Colpo)));
			})
			.ShadowOffset(FVector2D(0.f, 3.f))
			.ShadowColorAndOpacity_Lambda([this]() { return FLinearColor(0.f, 0.f, 0.f, 0.85f * OpacitaTitolo); })
		]

		// La riga piccola: sotto il titolo, sul nero, mentre ci si collega.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 110.f))
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return Riga; })
			.Font(Caratteri->Testo(24.f, TEXT("Italic")))
			.ColorAndOpacity_Lambda([this]()
			{
				// Respira piano, così si capisce che il gioco non è fermo.
				const float Respiro = 0.65f + 0.35f * FMath::Sin(static_cast<float>(FPlatformTime::Seconds()) * 2.2f);
				return FSlateColor(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(OpacitaRiga * Respiro));
			})
		]

		// "Esc  salta", in basso a destra, quando il volo si può saltare.
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 60.f, 48.f))
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return Aiuto; })
			.Font(FontAiuto)
			.ColorAndOpacity_Lambda([this]()
			{
				if (!bSaltabile)
				{
					return FSlateColor(FLinearColor::Transparent);
				}
				// Compare piano dopo un secondo, per non distrarre dall'inizio del volo.
				const double Ora = FPlatformTime::Seconds();
				if (InizioAiuto < 0.0)
				{
					InizioAiuto = Ora;
				}
				const float Comparsa = FMath::Clamp(static_cast<float>(Ora - InizioAiuto - 1.0), 0.f, 1.f);
				return FSlateColor(ValdorsoTema::Oro().CopyWithNewOpacity(0.55f * Comparsa));
			})
		]
	];

	SetVisibility(EVisibility::Visible);
}

FReply SValdorsoVolo::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	if (bSaltabile && !InKeyEvent.IsRepeat()
		&& (Tasto == EKeys::Escape || Tasto == EKeys::Gamepad_FaceButton_Right || Tasto == EKeys::Gamepad_Special_Right))
	{
		bSaltabile = false;
		OnSalta.ExecuteIfBound();
		return FReply::Handled();
	}
	// Il resto passa oltre (per esempio il tasto della console); il menu dietro non ha più il fuoco.
	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
