// Valdorso - La schermata d'avvio (vedi il .h).

#include "SValdorsoAvvio.h"
#include "SValdorsoMenuPrincipale.h"
#include "ValdorsoTemaUI.h"
#include "ValdorsoSuoni.h"
#include "ValdorsoImpostazioni.h"
#include "ValdorsoVersione.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Brushes/SlateNoResource.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/AudioComponent.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "ValdorsoAvvio"

namespace
{
	/** Parte piano e finisce piano. */
	float MorbidoAvvio(float A)
	{
		A = FMath::Clamp(A, 0.f, 1.f);
		return A * A * (3.f - 2.f * A);
	}
}

void SValdorsoAvvio::Construct(const FArguments& InArgs)
{
	OnFine = InArgs._OnFine;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Pieno = FSlateColorBrush(FLinearColor::White);
	Vuoto = FSlateNoResource();
	bMenoMovimenti = ValdorsoImpostazioni::Correnti().bMenoMovimenti;

	FSlateFontInfo FontLogo = Caratteri->Titolo(104.f, TEXT("Black"));
	FontLogo.LetterSpacing = 180;
	FSlateFontInfo FontTitolo = Caratteri->Titolo(132.f, TEXT("Black"));
	FontTitolo.LetterSpacing = 200;
	FSlateFontInfo FontEtichetta = Caratteri->Titolo(17.f, TEXT("Bold"));
	FontEtichetta.LetterSpacing = 260;

	const FText TestoVersione = FText::Format(LOCTEXT("Versione", "{0} · la valle in costruzione"), FText::FromString(ValdorsoVersione::PerScritte()));

	ChildSlot
	[
		SNew(SOverlay)

		// Il nero: pieno per il logo e l'avviso, poi si apre sulla scena del menu.
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(&Pieno)
			.ColorAndOpacity_Lambda([this]() { return FSlateColor(ValdorsoTema::Fondo().CopyWithNewOpacity(OpacitaNero())); })
		]

		// Le braci che salgono, solo sul titolo (sopra la scena).
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(&Vuoto)
			.Padding(0.f)
			.Visibility_Lambda([this]() { return OpacitaTitolo() > 0.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this]() { return FLinearColor(1.f, 1.f, 1.f, OpacitaTitolo()); })
			[
				SNew(SValdorsoBraci).Numero(60)
			]
		]

		// 1. Il logo.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Vuoto)
			.Padding(0.f)
			.Visibility_Lambda([this]() { return OpacitaLogo() > 0.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this]() { return FLinearColor(1.f, 1.f, 1.f, OpacitaLogo()); })
			[
			SNew(SVerticalBox)

			// il rombo di brace che batte
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 34.f))
			[
				// una scatola fissa di 40 px, con dentro il rombo che si gonfia a ogni colpo (il resto non si sposta)
				SNew(SBox).WidthOverride(40.f).HeightOverride(40.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride_Lambda([this]() { return FOptionalSize(22.f * (1.f + 0.35f * Pulsa())); })
					.HeightOverride_Lambda([this]() { return FOptionalSize(22.f * (1.f + 0.35f * Pulsa())); })
					[
						SNew(SImage)
						.Image(&Pieno)
						.ColorAndOpacity_Lambda([this]()
						{
							return FSlateColor(FMath::Lerp(ValdorsoTema::Brace(), ValdorsoTema::OroChiaro(), Pulsa() * 0.8f));
						})
						.RenderTransform(TransformCast<FSlateRenderTransform>(FQuat2D(FMath::DegreesToRadians(45.f))))
						.RenderTransformPivot(FVector2D(0.5f, 0.5f))
					]
				]
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Logo", "VALDORSO"))
				.Font(FontLogo)
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Pulsa() * 0.6f)); })
				.ShadowOffset(FVector2D(0.f, 3.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f))
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 18.f, 0.f, 16.f))
			[
				CreaSeparatore(420.f)
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Anno", "Anno 312 della Corona"))
				.Font(Caratteri->Testo(28.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::TestoSecondario())
			]
			]
		]

		// 2. Prima di partire.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Vuoto)
			.Padding(0.f)
			.Visibility_Lambda([this]() { return OpacitaAvviso() > 0.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this]() { return FLinearColor(1.f, 1.f, 1.f, OpacitaAvviso()); })
			[
			SNew(SBox)
			.WidthOverride(900.f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PrimaDiPartire", "PRIMA DI PARTIRE"))
					.Font(FontEtichetta)
					.ColorAndOpacity(ValdorsoTema::Oro())
				]

				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 14.f, 0.f, 30.f))
				[
					CreaSeparatore(300.f)
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("InCostruzione",
						"Valdorso è un mondo in costruzione: questa è la {0}. Quello che vedi cambierà ancora, e ogni prova "
						"aiuta a farlo crescere."), FText::FromString(ValdorsoVersione::PerScritte())))
					.Font(Caratteri->Testo(25.f))
					.ColorAndOpacity(ValdorsoTema::Pergamena())
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
					.LineHeightPercentage(1.12f)
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 26.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Salute",
						"Avviso per la salute. Nel gioco alcune luci battono e lampeggiano (il Cuore, le rune, i lampi dei "
						"temporali). Chi soffre di epilessia fotosensibile o di forti emicranie faccia attenzione; se senti "
						"malessere, fermati e riposa."))
					.Font(Caratteri->Testo(22.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::TestoSecondario())
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
					.LineHeightPercentage(1.1f)
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 18.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Accessibilita",
						"Grandezza delle scritte, meno movimenti e colori per daltonici: nelle Impostazioni, alla voce Accessibilità."))
					.Font(Caratteri->Testo(20.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.8f))
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
				]

				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 44.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Continua", "Premi un tasto per continuare"))
					.Font(Caratteri->Titolo(16.f, TEXT("Regular")))
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(ValdorsoTema::Oro().CopyWithNewOpacity(0.55f + 0.35f * Pulsa())); })
				]
			]
			]
		]

		// 3. Il titolo, sopra la scena.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 0.f, 60.f))
		[
			SNew(SBorder)
			.BorderImage(&Vuoto)
			.Padding(0.f)
			.Visibility_Lambda([this]() { return OpacitaTitolo() > 0.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this]() { return FLinearColor(1.f, 1.f, 1.f, OpacitaTitolo()); })
			[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Titolo", "VALDORSO"))
				.Font(FontTitolo)
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Pulsa() * 0.7f)); })
				.ShadowOffset(FVector2D(0.f, 4.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f))
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 6.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Motto", "Ogni gesto lascia un segno."))
				.Font(Caratteri->Testo(30.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::TestoSecondario())
				.ShadowOffset(FVector2D(0.f, 2.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f))
			]
			]
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 110.f))
		[
			SNew(STextBlock)
			.Visibility_Lambda([this]() { return OpacitaTitolo() > 0.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.Text(LOCTEXT("PremiUnTasto", "Premi un tasto per entrare"))
			.Font(Caratteri->Titolo(24.f, TEXT("Bold")))
			.ColorAndOpacity_Lambda([this]()
			{
				// pulsa piano con il battito, e compare dopo il titolo
				const float Dopo = MorbidoAvvio((Trascorso() - 1.2f) / 0.8f);
				const float Alfa = Adesso == EMomento::Uscita ? OpacitaTitolo() : Dopo * (0.55f + 0.45f * Pulsa());
				return FSlateColor(ValdorsoTema::Pergamena().CopyWithNewOpacity(Alfa));
			})
			.ShadowOffset(FVector2D(0.f, 2.f))
			.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f))
		]

		// La versione, in basso a destra (come nel menu).
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 64.f, 54.f))
		[
			SNew(STextBlock)
			.Visibility(EVisibility::HitTestInvisible)
			.Text(TestoVersione)
			.Font(Caratteri->Testo(19.f, TEXT("Italic")))
			.ColorAndOpacity_Lambda([this]() { return FSlateColor(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.7f * FMath::Max(OpacitaTitolo(), OpacitaAvviso()))); })
		]
	];

	VaiA(EMomento::Logo);
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoAvvio::Anima));
}

SValdorsoAvvio::~SValdorsoAvvio()
{
	if (Battito.IsValid())
	{
		ValdorsoSuoni::Sfuma(Battito.Get(), 0.2f);
		Battito.Reset();
	}
}

TSharedRef<SWidget> SValdorsoAvvio::CreaSeparatore(float Larghezza)
{
	const FLinearColor Linea = ValdorsoTema::Oro().CopyWithNewOpacity(0.55f);
	return SNew(SBox).WidthOverride(Larghezza).HeightOverride(14.f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(1.f) [ SNew(SImage).Image(&Pieno).ColorAndOpacity(Linea) ]
			]

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(12.f, 0.f))
			[
				SNew(SBox).WidthOverride(8.f).HeightOverride(8.f)
				[
					SNew(SImage)
					.Image(&Pieno)
					.ColorAndOpacity(ValdorsoTema::Oro())
					.RenderTransform(TransformCast<FSlateRenderTransform>(FQuat2D(FMath::DegreesToRadians(45.f))))
					.RenderTransformPivot(FVector2D(0.5f, 0.5f))
				]
			]

			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(1.f) [ SNew(SImage).Image(&Pieno).ColorAndOpacity(Linea) ]
			]
		];
}

void SValdorsoAvvio::VaiA(EMomento Nuovo)
{
	Adesso = Nuovo;
	InizioMomento = FPlatformTime::Seconds();
	if (Nuovo == EMomento::Logo)
	{
		// Un battito solo, piano (nei test e sul server non suona niente).
		// (ValdorsoSuoni::Suona applica già il volume degli effetti delle impostazioni)
		Battito.Reset(ValdorsoSuoni::Suona(TEXT("S_Battito"), 0.55f));
	}
}

void SValdorsoAvvio::Avanza()
{
	// sul titolo si aspetta che il nero si sia aperto e la scritta sia comparsa (niente salti a scatto)
	const float Minimo = Adesso == EMomento::Titolo ? 2.0f : AttesaMinima;
	if (Trascorso() < Minimo)
	{
		return;
	}
	switch (Adesso)
	{
	case EMomento::Logo:   VaiA(EMomento::Avviso); break;
	case EMomento::Avviso: VaiA(EMomento::Titolo); break;
	case EMomento::Titolo: VaiA(EMomento::Uscita); break;
	default: break;
	}
}

FReply SValdorsoAvvio::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// un tasto tenuto premuto non salta tutto
	if (!InKeyEvent.IsRepeat())
	{
		Avanza();
	}
	return FReply::Handled();
}

FReply SValdorsoAvvio::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Avanza();
	return FReply::Handled();
}

EActiveTimerReturnType SValdorsoAvvio::Anima(double Tempo, float Delta)
{
	const float T = Trascorso();
	if (Adesso == EMomento::Logo && T >= DurataLogo)
	{
		VaiA(EMomento::Avviso);
	}
	else if (Adesso == EMomento::Avviso && T >= DurataAvviso)
	{
		VaiA(EMomento::Titolo);
	}
	else if (Adesso == EMomento::Uscita && T >= DurataUscita && !bFinito)
	{
		bFinito = true;
		OnFine.ExecuteIfBound();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

float SValdorsoAvvio::Trascorso() const
{
	return static_cast<float>(FPlatformTime::Seconds() - InizioMomento);
}

float SValdorsoAvvio::OpacitaNero() const
{
	switch (Adesso)
	{
	case EMomento::Logo:
	case EMomento::Avviso: return 1.f;
	case EMomento::Titolo: return 1.f - MorbidoAvvio(Trascorso() / 1.4f);
	default: return 0.f;
	}
}

float SValdorsoAvvio::OpacitaLogo() const
{
	if (Adesso != EMomento::Logo)
	{
		return 0.f;
	}
	const float T = Trascorso();
	return MorbidoAvvio(T / 1.1f) * MorbidoAvvio((DurataLogo - T) / 0.6f);
}

float SValdorsoAvvio::OpacitaAvviso() const
{
	return Adesso == EMomento::Avviso ? MorbidoAvvio(Trascorso() / 0.6f) : 0.f;
}

float SValdorsoAvvio::OpacitaTitolo() const
{
	if (Adesso == EMomento::Titolo)
	{
		return MorbidoAvvio((Trascorso() - 0.4f) / 1.0f);
	}
	if (Adesso == EMomento::Uscita)
	{
		return 1.f - MorbidoAvvio(Trascorso() / DurataUscita);
	}
	return 0.f;
}

float SValdorsoAvvio::Pulsa() const
{
	return bMenoMovimenti ? 0.35f : ValdorsoTema::Battito(FPlatformTime::Seconds());
}

#undef LOCTEXT_NAMESPACE
