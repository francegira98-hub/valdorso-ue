// Valdorso - Il dialogo con un PNG sulla pergamena (vedi il .h). Scritto da Claude il 07/11/2026.
// Unity build: le cose del namespace anonimo cominciano con DialogoUI, così non si scontrano con gli altri file.

#include "SValdorsoDialogo.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoRune.h"
#include "ValdorsoFiducia.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoDialogo"

namespace
{
	/** Una battuta tra parentesi è un gesto ("(Gaspare sputa un filo d'erba.)"): si scrive in corsivo, senza parentesi. */
	bool DialogoUIGesto(const FString& Riga, FString& Fuori)
	{
		const FString Pulita = Riga.TrimStartAndEnd();
		if (Pulita.Len() >= 2 && Pulita.StartsWith(TEXT("(")) && Pulita.EndsWith(TEXT(")")))
		{
			Fuori = Pulita.Mid(1, Pulita.Len() - 2).TrimStartAndEnd();
			return true;
		}
		return false;
	}
}

void SValdorsoDialogo::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	OnScelta = InArgs._OnScelta;
	// La stessa carta della bacheca: pergamena chiara con il bordo d'oro scuro.
	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 4.f, ValdorsoTema::OroScuro(), 2.f);
	// Un velo leggero: il PNG dietro la pergamena si deve ancora vedere.
	VeloLeggero = FSlateColorBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.3f));

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&VeloLeggero)
		]
		// A destra, a metà altezza: a sinistra resta chi parla.
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(FMargin(20.f, 30.f, 70.f, 30.f))
		[
			SNew(SBorder)
			.BorderImage(&Carta)
			.Padding(FMargin(40.f, 28.f))
			[
				SAssignNew(Contenuto, SBox).WidthOverride(640.f).MaxDesiredHeight(820.f)
			]
		]
	];
	Mostra(InArgs._Battuta);
}

TSharedRef<SWidget> SValdorsoDialogo::PulsanteRisposta(int32 Indice, const FString& Testo)
{
	// Come i pulsanti di FValdorsoStileAccesso, ma la scritta va a capo (le risposte possono essere lunghe).
	TSharedRef<TWeakPtr<SButton>> Riferimento = MakeShared<TWeakPtr<SButton>>();
	const FText Scritta = FText::FromString(FString::Printf(TEXT("%d.   %s"), Indice + 1, *Testo));
	TSharedRef<SButton> Nuovo = SNew(SButton)
		.ButtonStyle(&Stile->StilePulsante)
		.IsEnabled_Lambda([this]() { return !bInAttesa; })
		.IsFocusable(true)
		.HAlign(HAlign_Left)
		.OnClicked_Lambda([this, Indice]()
		{
			Scegli(Indice);
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(Scritta)
			.Font(Stile->Caratteri->Testo(19.f))
			.AutoWrapText(true)
			.ColorAndOpacity_Lambda([Riferimento]()
			{
				const TSharedPtr<SButton> P = Riferimento->Pin();
				if (P.IsValid() && !P->IsEnabled())
				{
					return FSlateColor(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.4f));
				}
				const bool bAttivo = P.IsValid() && (P->IsHovered() || P->HasKeyboardFocus());
				return FSlateColor(bAttivo ? ValdorsoTema::OroChiaro() : ValdorsoTema::Oro());
			})
		];
	*Riferimento = Nuovo;
	return Nuovo;
}

void SValdorsoDialogo::Mostra(const FValdorsoBattuta& Battuta)
{
	bInAttesa = false;
	bFine = Battuta.bFine || Battuta.Scelte.Num() == 0;
	NumeroScelte = bFine ? 0 : FMath::Min(Battuta.Scelte.Num(), ValdorsoDialoghi::MassimoScelte);

	FSlateFontInfo FontTitolo = Stile->Caratteri->Titolo(26.f, TEXT("Bold"));
	FontTitolo.LetterSpacing = 20;

	// Le battute del PNG: parole, gesti in corsivo, rune disegnate.
	TSharedRef<SVerticalBox> Battute = SNew(SVerticalBox);
	for (const FString& Riga : Battuta.Righe)
	{
		TArray<TArray<EValdorsoRuna>> Parole;
		if (ValdorsoRune::LeggiRiga(Riga, Parole))
		{
			FString Didascalia;
			int32 Barra = INDEX_NONE;
			if (Riga.FindChar(TEXT('|'), Barra))
			{
				Didascalia = Riga.Mid(Barra + 1).TrimStartAndEnd();
			}
			TSharedRef<SHorizontalBox> Fila = SNew(SHorizontalBox);
			for (const TArray<EValdorsoRuna>& Parola : Parole)
			{
				Fila->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(10.f, 4.f))
				[
					SNew(SValdorsoRuna)
					.Parola(Parola)
					.Altezza(Parole.Num() > 3 ? 36.f : 46.f)
					.Spessore(3.f)
					.Colore(ValdorsoTema::Rubrica())
				];
			}
			Battute->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 4.f, 0.f, 2.f))
			[
				Fila
			];
			if (!Didascalia.IsEmpty())
			{
				Battute->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 10.f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(Didascalia))
					.Font(Stile->Caratteri->Testo(16.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
				];
			}
			continue;
		}
		FString Gesto;
		const bool bGesto = DialogoUIGesto(Riga, Gesto);
		Battute->AddSlot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 10.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(bGesto ? Gesto : Riga))
			.Font(Stile->Caratteri->Testo(bGesto ? 18.f : 20.f, bGesto ? TEXT("Italic") : TEXT("Regular")))
			.ColorAndOpacity(bGesto ? ValdorsoTema::InchiostroTenue() : ValdorsoTema::Inchiostro())
			.AutoWrapText(true)
		];
	}

	// Le risposte (o, alla fine, "Congedati").
	TSharedRef<SVerticalBox> Risposte = SNew(SVerticalBox);
	PrimoPulsante.Reset();
	if (bFine)
	{
		TSharedRef<SButton> Congedati = Stile->Pulsante(LOCTEXT("Congedati", "Congedati"),
			FSimpleDelegate::CreateLambda([this]() { Scegli(INDEX_NONE); }), 18.f);
		PrimoPulsante = Congedati;
		Risposte->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(220.f)[ Congedati ]
		];
	}
	else
	{
		for (int32 i = 0; i < NumeroScelte; ++i)
		{
			TSharedRef<SWidget> Pulsante = PulsanteRisposta(i, Battuta.Scelte[i]);
			if (!PrimoPulsante.IsValid())
			{
				PrimoPulsante = Pulsante;
			}
			Risposte->AddSlot().AutoHeight().Padding(FMargin(0.f, 3.f))
			[
				Pulsante
			];
		}
	}

	const FText Aiuto = bFine
		? LOCTEXT("AiutoFine", "Invio o Esc per congedarti")
		: LOCTEXT("Aiuto", "1-4 per rispondere  ·  Esc per andare via");

	if (!Contenuto.IsValid())
	{
		return;
	}
	Contenuto->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Battuta.Chi.IsEmpty() ? FString(TEXT("...")) : Battuta.Chi))
			.Font(FontTitolo)
			.ColorAndOpacity(ValdorsoTema::Rubrica())
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
		]
		// (14/11, v0.2.4) quanto si fida di te, a parole (niente numeri: ValdorsoFiducia.h)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 2.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Visibility(Battuta.Fiducia.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
			.Text(FText::FromString(Battuta.Fiducia))
			.Font(Stile->Caratteri->Testo(15.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoFiducia::Colore(static_cast<EValdorsoFiducia>(FMath::Min<uint8>(Battuta.GradoFiducia,
				static_cast<uint8>(EValdorsoFiducia::DiFamiglia)))))
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 8.f, 0.f, 16.f))
		[
			Stile->Separatore(260.f, ValdorsoTema::OroScuro())
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				Battute
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 6.f, 0.f, 12.f))
		[
			Stile->Separatore(160.f, ValdorsoTema::OroScuro())
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Risposte
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 14.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(Aiuto)
			.Font(Stile->Caratteri->Testo(14.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
		]
	);
}

void SValdorsoDialogo::Scegli(int32 Indice)
{
	if (Indice != INDEX_NONE)
	{
		if (bInAttesa || bFine || Indice < 0 || Indice >= NumeroScelte)
		{
			return;
		}
		// Si aspetta la battuta dopo dal server (i pulsanti si spengono: niente risposte doppie).
		bInAttesa = true;
	}
	OnScelta.ExecuteIfBound(Indice);
}

FReply SValdorsoDialogo::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::F10 || Tasto == EKeys::Gamepad_FaceButton_Right || Tasto == EKeys::Gamepad_Special_Right)
	{
		Scegli(INDEX_NONE);
		return FReply::Handled();
	}
	if (bFine)
	{
		if (Tasto == EKeys::E || Tasto == EKeys::Enter || Tasto == EKeys::SpaceBar || Tasto == EKeys::One || Tasto == EKeys::NumPadOne)
		{
			Scegli(INDEX_NONE);
		}
		return FReply::Handled();
	}
	const FKey Numeri[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
	const FKey Tastierino[] = { EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour };
	for (int32 i = 0; i < ValdorsoDialoghi::MassimoScelte; ++i)
	{
		if (Tasto == Numeri[i] || Tasto == Tastierino[i])
		{
			Scegli(i);
			return FReply::Handled();
		}
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoDialogo::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

#undef LOCTEXT_NAMESPACE
