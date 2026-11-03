// Valdorso - La bacheca letta (vedi il .h). Scritto da Claude il 03/11/2026.

#include "SValdorsoBacheca.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoTemaUI.h"
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

#define LOCTEXT_NAMESPACE "ValdorsoBacheca"

void SValdorsoBacheca::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	OnChiudi = InArgs._OnChiudi;
	// La carta della bacheca: pergamena chiara con il bordo d'oro scuro.
	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 4.f, ValdorsoTema::OroScuro(), 2.f);

	FSlateFontInfo FontTitolo = Stile->Caratteri->Titolo(26.f, TEXT("Bold"));
	FontTitolo.LetterSpacing = 20;

	TSharedRef<SVerticalBox> Fogli = SNew(SVerticalBox);
	if (InArgs._Avvisi.Num() == 0)
	{
		Fogli->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Vuota", "Non c'è appeso niente. Solo i buchi dei chiodi."))
			.Font(Stile->Caratteri->Testo(20.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
		];
	}
	for (int32 i = 0; i < InArgs._Avvisi.Num(); ++i)
	{
		if (i > 0)
		{
			Fogli->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 8.f))
			[
				Stile->Separatore(160.f, ValdorsoTema::OroScuro())
			];
		}
		Fogli->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(InArgs._Avvisi[i]))
			.Font(Stile->Caratteri->Testo(20.f))
			.ColorAndOpacity(ValdorsoTema::Inchiostro())
			.AutoWrapText(true)
		];
	}

	TSharedRef<SButton> Chiudi = Stile->Pulsante(LOCTEXT("Chiudi", "Chiudi"),
		FSimpleDelegate::CreateLambda([this]() { OnChiudi.ExecuteIfBound(); }), 18.f);
	PulsanteChiudi = Chiudi;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&Stile->Velo)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(20.f, 30.f))
		[
			SNew(SBorder)
			.BorderImage(&Carta)
			.Padding(FMargin(44.f, 30.f))
			[
				SNew(SBox).WidthOverride(620.f).MaxDesiredHeight(760.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(InArgs._Titolo.IsEmpty() ? FString(TEXT("Bacheca")) : InArgs._Titolo))
						.Font(FontTitolo)
						.ColorAndOpacity(ValdorsoTema::Rubrica())
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
							Fogli
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 18.f, 0.f, 0.f))
					[
						SNew(SBox).WidthOverride(200.f)[ Chiudi ]
					]
				]
			]
		]
	];
}

FReply SValdorsoBacheca::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::F10 || Tasto == EKeys::E || Tasto == EKeys::Gamepad_FaceButton_Right
		|| Tasto == EKeys::Gamepad_Special_Right)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoBacheca::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

#undef LOCTEXT_NAMESPACE
