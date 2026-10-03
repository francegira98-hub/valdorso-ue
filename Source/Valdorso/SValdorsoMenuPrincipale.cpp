// Valdorso - Il menu principale "Oro e brace" (vedi il .h).

#include "SValdorsoMenuPrincipale.h"
#include "ValdorsoTemaUI.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "ValdorsoMenu"

// ---------------------------------------------------------------------------------------------
// Le braci
// ---------------------------------------------------------------------------------------------

void SValdorsoBraci::Construct(const FArguments& InArgs)
{
	Caso.Initialize(static_cast<int32>(FPlatformTime::Cycles()));
	Pennello = FSlateRoundedBoxBrush(FLinearColor::White, 100.f);

	Braci.SetNum(FMath::Max(0, InArgs._Numero));
	for (FBrace& Brace : Braci)
	{
		Rinasci(Brace, true);
	}

	SetVisibility(EVisibility::HitTestInvisible);
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoBraci::Aggiorna));
}

void SValdorsoBraci::Rinasci(FBrace& Brace, bool bAllInizio)
{
	Brace.Posizione.X = Caso.FRand();
	Brace.Posizione.Y = bAllInizio ? Caso.FRand() : 1.03f;
	Brace.Salita = Caso.FRandRange(0.025f, 0.08f);
	Brace.Ondeggio = Caso.FRandRange(0.004f, 0.016f);
	Brace.Fase = Caso.FRandRange(0.f, 2.f * PI);
	Brace.Dimensione = Caso.FRandRange(1.6f, 4.2f);
	Brace.Vita = Caso.FRandRange(6.f, 13.f);
	Brace.Eta = bAllInizio ? Caso.FRandRange(0.f, Brace.Vita) : 0.f;
	Brace.Colore = FMath::Lerp(ValdorsoTema::Brace(), ValdorsoTema::OroChiaro(), Caso.FRandRange(0.f, 0.55f));
}

EActiveTimerReturnType SValdorsoBraci::Aggiorna(double Tempo, float Delta)
{
	Delta = FMath::Min(Delta, 0.1f);
	for (FBrace& Brace : Braci)
	{
		Brace.Eta += Delta;
		Brace.Posizione.Y -= Brace.Salita * Delta;
		Brace.Posizione.X += FMath::Sin(Brace.Eta * 1.3f + Brace.Fase) * Brace.Ondeggio * Delta;
		if (Brace.Eta > Brace.Vita || Brace.Posizione.Y < -0.05f)
		{
			Rinasci(Brace, false);
		}
	}
	return EActiveTimerReturnType::Continue;
}

int32 SValdorsoBraci::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Misura = FVector2f(AllottedGeometry.GetLocalSize());
	const FLinearColor Tinta = InWidgetStyle.GetColorAndOpacityTint();

	for (const FBrace& Brace : Braci)
	{
		// Si accende nel primo secondo, si spegne negli ultimi due, e tremola come il fuoco.
		const float Entrata = FMath::Clamp(Brace.Eta / 1.f, 0.f, 1.f);
		const float Uscita = FMath::Clamp((Brace.Vita - Brace.Eta) / 2.f, 0.f, 1.f);
		const float Tremolio = 0.7f + 0.3f * FMath::Sin(Brace.Eta * 9.f + Brace.Fase);
		const float Alfa = Entrata * Uscita * Tremolio;
		if (Alfa <= 0.01f)
		{
			continue;
		}

		const FVector2f Centro(Brace.Posizione.X * Misura.X, Brace.Posizione.Y * Misura.Y);

		// L'alone, grande e tenue.
		const float Alone = Brace.Dimensione * 4.f;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(Alone, Alone), FSlateLayoutTransform(Centro - FVector2f(Alone * 0.5f, Alone * 0.5f))),
			&Pennello, ESlateDrawEffect::None, Brace.Colore.CopyWithNewOpacity(Alfa * 0.13f) * Tinta);

		// Il nucleo, piccolo e vivo.
		const float Nucleo = Brace.Dimensione;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(Nucleo, Nucleo), FSlateLayoutTransform(Centro - FVector2f(Nucleo * 0.5f, Nucleo * 0.5f))),
			&Pennello, ESlateDrawEffect::None, Brace.Colore.CopyWithNewOpacity(Alfa) * Tinta);
	}
	return LayerId + 1;
}

// ---------------------------------------------------------------------------------------------
// La sfumatura
// ---------------------------------------------------------------------------------------------

int32 SValdorsoSfumatura::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Misura = AllottedGeometry.GetLocalSize();
	const FLinearColor Fondo = ValdorsoTema::Fondo();

	// Da sinistra: scuro dove stanno titolo e pulsanti, poi la scena libera a destra.
	TArray<FSlateGradientStop> Orizzontale;
	Orizzontale.Add(FSlateGradientStop(FVector2D(0.0, 0.0), Fondo.CopyWithNewOpacity(0.92f)));
	Orizzontale.Add(FSlateGradientStop(FVector2D(Misura.X * 0.36, 0.0), Fondo.CopyWithNewOpacity(0.72f)));
	Orizzontale.Add(FSlateGradientStop(FVector2D(Misura.X * 0.62, 0.0), Fondo.CopyWithNewOpacity(0.f)));
	FSlateDrawElement::MakeGradient(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Orizzontale, Orient_Vertical);

	// Dal basso: un velo per le voci della locanda e la versione.
	TArray<FSlateGradientStop> Verticale;
	Verticale.Add(FSlateGradientStop(FVector2D(0.0, Misura.Y * 0.72), Fondo.CopyWithNewOpacity(0.f)));
	Verticale.Add(FSlateGradientStop(FVector2D(0.0, Misura.Y), Fondo.CopyWithNewOpacity(0.8f)));
	FSlateDrawElement::MakeGradient(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Verticale, Orient_Horizontal);

	return LayerId + 1;
}

// ---------------------------------------------------------------------------------------------
// Il menu
// ---------------------------------------------------------------------------------------------

void SValdorsoMenuPrincipale::Construct(const FArguments& InArgs)
{
	OnEntra = InArgs._OnEntra;
	OnEsci = InArgs._OnEsci;
	Caratteri = MakeShared<FValdorsoCaratteri>();

	Pieno = FSlateColorBrush(FLinearColor::White);
	SfondoPannello = FSlateRoundedBoxBrush(ValdorsoTema::Pannelli().CopyWithNewOpacity(0.96f), 6.f,
		ValdorsoTema::Oro().CopyWithNewOpacity(0.55f), 1.f);

	StilePulsante = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateRoundedBoxBrush(ValdorsoTema::PulsanteSopra().CopyWithNewOpacity(0.55f), 3.f))
		.SetPressed(FSlateRoundedBoxBrush(ValdorsoTema::PulsantePremuto().CopyWithNewOpacity(0.75f), 3.f))
		.SetDisabled(FSlateNoResource())
		.SetNormalPadding(FMargin(14.f, 6.f))
		.SetPressedPadding(FMargin(14.f, 7.f, 14.f, 5.f));

	// Una voce della locanda diversa a ogni avvio (più avanti arriveranno dal server, dal registro eventi).
	const TArray<FText> Voci = {
		LOCTEXT("Voce1", "Dicono che di notte le rune delle rovine si accendano da sole."),
		LOCTEXT("Voce2", "Un pastore giura di aver visto un'ombra enorme passare davanti alla luna."),
		LOCTEXT("Voce3", "Al posto di guardia del passo non lasciano uscire nessuno. Nemmeno le lettere."),
		LOCTEXT("Voce4", "Bruno, alla locanda, dice che il frammento del tempio ha saltato un battito."),
		LOCTEXT("Voce5", "Gaspare il cacciatore parla di un'ombra d'orso tra gli alberi, nelle notti di nebbia."),
		LOCTEXT("Voce6", "Qualcuno ha sentito la campana del tempio suonare da sola, dopo mezzanotte.")
	};
	const FText Voce = Voci[FMath::RandRange(0, Voci.Num() - 1)];

	// I due pulsanti che ricevono il fuoco del controller: "Entra nella valle" e "Chiudi" del pannello.
	const TSharedRef<SButton> PulsanteEntra = CreaPulsante(LOCTEXT("Entra", "Entra nella valle"),
		FSimpleDelegate::CreateLambda([this]() { OnEntra.ExecuteIfBound(); }), 32.f);
	const TSharedRef<SButton> PulsanteChiudiPannello = CreaPulsante(LOCTEXT("Chiudi", "Chiudi"),
		FSimpleDelegate::CreateSP(this, &SValdorsoMenuPrincipale::ChiudiPannello), 24.f);
	PrimoPulsante = PulsanteEntra;
	PulsanteChiudi = PulsanteChiudiPannello;

	FSlateFontInfo FontTitolo = Caratteri->Titolo(112.f, TEXT("Black"));
	FontTitolo.LetterSpacing = 160;

	FSlateFontInfo FontEtichetta = Caratteri->Titolo(15.f, TEXT("Bold"));
	FontEtichetta.LetterSpacing = 220;

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SNew(SValdorsoSfumatura)
		]

		+ SOverlay::Slot()
		[
			SNew(SValdorsoBraci).Numero(80)
		]

		// Titolo, motto e pulsanti.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FMargin(130.f, 0.f, 0.f, 70.f))
		[
			SNew(SVerticalBox)
			.IsEnabled_Lambda([this]() { return !PannelloAperto(); })

			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Titolo", "VALDORSO"))
				.Font(FontTitolo)
				.ColorAndOpacity_Lambda([]()
				{
					const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
					return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo * 0.7f));
				})
				.ShadowOffset(FVector2D(0.f, 3.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f))
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(6.f, 2.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Motto", "Ogni gesto lascia un segno. Scegli chi diventare."))
				.Font(Caratteri->Testo(27.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::TestoSecondario())
				.ShadowOffset(FVector2D(0.f, 2.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f))
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(4.f, 26.f, 0.f, 30.f))
			[
				CreaSeparatore(460.f)
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f))
			[
				PulsanteEntra
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f))
			[
				CreaPulsante(LOCTEXT("Impostazioni", "Impostazioni"),
					FSimpleDelegate::CreateSP(this, &SValdorsoMenuPrincipale::MostraImpostazioni))
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f))
			[
				CreaPulsante(LOCTEXT("Riconoscimenti", "Riconoscimenti"),
					FSimpleDelegate::CreateSP(this, &SValdorsoMenuPrincipale::MostraRiconoscimenti))
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f))
			[
				CreaPulsante(LOCTEXT("Esci", "Esci"),
					FSimpleDelegate::CreateLambda([this]() { OnEsci.ExecuteIfBound(); }))
			]
		]

		// Le voci della locanda.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(136.f, 0.f, 0.f, 58.f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("VociTitolo", "VOCI DALLA LOCANDA"))
				.Font(FontEtichetta)
				.ColorAndOpacity(ValdorsoTema::Oro().CopyWithNewOpacity(0.85f))
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 6.f, 0.f, 0.f))
			[
				SNew(SBox).WidthOverride(720.f)
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("VoceFormato", "«{0}»"), Voce))
					.Font(Caratteri->Testo(22.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::Pergamena().CopyWithNewOpacity(0.85f))
					.AutoWrapText(true)
				]
			]
		]

		// La versione.
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 64.f, 54.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Versione", "v0.1.2 · la valle in costruzione"))
			.Font(Caratteri->Testo(19.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.75f))
		]

		// Il pannello (Impostazioni, Riconoscimenti).
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.Visibility_Lambda([this]() { return VisibilitaPannello; })
			.BorderImage(&SfondoPannello)
			.Padding(FMargin(52.f, 38.f))
			[
				SNew(SBox).WidthOverride(780.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return TitoloPannello; })
						.Font(Caratteri->Titolo(36.f, TEXT("Bold")))
						.ColorAndOpacity(ValdorsoTema::Oro())
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(2.f, 14.f, 0.f, 22.f))
					[
						CreaSeparatore(320.f)
					]

					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return TestoPannello; })
						.Font(Caratteri->Testo(23.f))
						.ColorAndOpacity(ValdorsoTema::Pergamena())
						.AutoWrapText(true)
						.LineHeightPercentage(1.1f)
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(FMargin(0.f, 30.f, 0.f, 0.f))
					[
						PulsanteChiudiPannello
					]
				]
			]
		]
	];

	// Il menu appare piano piano, come la valle che esce dalla nebbia.
	SetRenderOpacity(0.f);
	Inizio = FPlatformTime::Seconds();
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoMenuPrincipale::Anima));
}

TSharedRef<SButton> SValdorsoMenuPrincipale::CreaPulsante(const FText& Testo, FSimpleDelegate Azione, float Dimensione)
{
	// Il pulsante si "accende" quando il mouse ci passa sopra o quando ha il fuoco del controller.
	TSharedRef<TWeakPtr<SButton>> Riferimento = MakeShared<TWeakPtr<SButton>>();
	auto Attivo = [Riferimento]()
	{
		const TSharedPtr<SButton> Pulsante = Riferimento->Pin();
		return Pulsante.IsValid() && Pulsante->IsEnabled() && (Pulsante->IsHovered() || Pulsante->HasKeyboardFocus());
	};

	TSharedRef<SButton> Pulsante = SNew(SButton)
		.ButtonStyle(&StilePulsante)
		.IsFocusable(true)
		.OnClicked_Lambda([Azione]() { Azione.ExecuteIfBound(); return FReply::Handled(); })
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(0.f, 0.f, 16.f, 0.f))
			[
				SNew(SBox).WidthOverride(9.f).HeightOverride(9.f)
				[
					SNew(SImage)
					.Image(&Pieno)
					.ColorAndOpacity_Lambda([Attivo]()
					{
						return FSlateColor(ValdorsoTema::Brace().CopyWithNewOpacity(Attivo() ? 1.f : 0.f));
					})
					.RenderTransform(TransformCast<FSlateRenderTransform>(FQuat2D(FMath::DegreesToRadians(45.f))))
					.RenderTransformPivot(FVector2D(0.5f, 0.5f))
				]
			]

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Testo)
				.Font(Caratteri->Titolo(Dimensione, TEXT("Bold")))
				.ColorAndOpacity_Lambda([Attivo]()
				{
					return FSlateColor(Attivo() ? ValdorsoTema::Oro() : ValdorsoTema::Pergamena());
				})
				.ShadowOffset(FVector2D(0.f, 2.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f))
			]
		];

	*Riferimento = Pulsante;
	return Pulsante;
}

TSharedRef<SWidget> SValdorsoMenuPrincipale::CreaDiamante(float Lato, const FLinearColor& Colore)
{
	return SNew(SBox).WidthOverride(Lato).HeightOverride(Lato)
		[
			SNew(SImage)
			.Image(&Pieno)
			.ColorAndOpacity(Colore)
			.RenderTransform(TransformCast<FSlateRenderTransform>(FQuat2D(FMath::DegreesToRadians(45.f))))
			.RenderTransformPivot(FVector2D(0.5f, 0.5f))
		];
}

TSharedRef<SWidget> SValdorsoMenuPrincipale::CreaSeparatore(float Larghezza)
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
				CreaDiamante(8.f, ValdorsoTema::Oro())
			]

			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(1.f) [ SNew(SImage).Image(&Pieno).ColorAndOpacity(Linea) ]
			]
		];
}

void SValdorsoMenuPrincipale::MostraPannello(const FText& Titolo, const FText& Testo)
{
	TitoloPannello = Titolo;
	TestoPannello = Testo;
	VisibilitaPannello = EVisibility::Visible;
	if (PulsanteChiudi.IsValid())
	{
		FSlateApplication::Get().SetAllUserFocus(PulsanteChiudi, EFocusCause::SetDirectly);
	}
}

void SValdorsoMenuPrincipale::ChiudiPannello()
{
	VisibilitaPannello = EVisibility::Collapsed;
	if (PrimoPulsante.IsValid())
	{
		FSlateApplication::Get().SetAllUserFocus(PrimoPulsante, EFocusCause::SetDirectly);
	}
}

void SValdorsoMenuPrincipale::MostraImpostazioni()
{
	MostraPannello(LOCTEXT("ImpTitolo", "Impostazioni"),
		LOCTEXT("ImpTesto", "Grafica, tasti e suoni arrivano con il passo 6 di questa tappa: preset grafici con una prova automatica al primo avvio, tasti ridefinibili, volumi separati per musica, ambiente e voci."));
}

void SValdorsoMenuPrincipale::MostraRiconoscimenti()
{
	MostraPannello(LOCTEXT("RicTitolo", "Riconoscimenti"),
		LOCTEXT("RicTesto",
			"Valdorso è ideato e diretto da Fra.\n\n"
			"Fatto con Unreal Engine 5.8 di Epic Games, con il manichino e le animazioni del Game Animation Sample di Epic.\n"
			"Capriola da Mixamo (Adobe).\n"
			"Orso: \"Animated Bear 3D Model\" di AnimalMesh 3D, da Sketchfab, licenza CC BY 4.0 (creativecommons.org/licenses/by/4.0); materiali e misure cambiati per Valdorso.\n"
			"Texture di pietra, roccia e terreno e cielo notturno da Poly Haven (CC0).\n"
			"Caratteri Cinzel ed EB Garamond, licenza SIL Open Font License 1.1.\n"
			"Codice scritto con l'aiuto di Claude, di Anthropic.\n\n"
			"Grazie alla community del Discord di Valdorso."));
}

FReply SValdorsoMenuPrincipale::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// Esc o il tasto "indietro" del controller chiudono il pannello.
	if (PannelloAperto() && (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right))
	{
		ChiudiPannello();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

EActiveTimerReturnType SValdorsoMenuPrincipale::Anima(double Tempo, float Delta)
{
	// Comparsa in 1,5 secondi; poi il timer resta acceso perché il titolo pulsa con il battito.
	const float Trascorso = static_cast<float>(FPlatformTime::Seconds() - Inizio);
	SetRenderOpacity(FMath::Clamp(Trascorso / 1.5f, 0.f, 1.f));
	return EActiveTimerReturnType::Continue;
}

#undef LOCTEXT_NAMESPACE
