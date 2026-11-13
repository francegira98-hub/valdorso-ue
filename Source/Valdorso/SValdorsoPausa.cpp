// Valdorso - La pausa (vedi il .h). Scritto da Claude il 02/11/2026.

#include "SValdorsoPausa.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoTemaUI.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoPausa"

void SValdorsoPausa::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	OnRiprendi = InArgs._OnRiprendi;
	OnImpostazioni = InArgs._OnImpostazioni;
	OnEsci = InArgs._OnEsci;
	OnAnnulla = InArgs._OnAnnulla;

	FSlateFontInfo FontTitolo = Stile->Caratteri->Titolo(30.f, TEXT("Bold"));
	FontTitolo.LetterSpacing = 30;

	// I pulsanti si spengono mentre si esce (resta solo "Resta nella valle").
	const TAttribute<bool> Liberi = TAttribute<bool>::CreateLambda([this]() { return !bInUscita; });
	auto Riga = [](const TSharedRef<SWidget>& Pulsante)
	{
		return SNew(SBox).WidthOverride(400.f)[ Pulsante ];   // (13/11) 340 tagliava "Torna ai personaggi"
	};

	TSharedRef<SVerticalBox> Pulsanti = SNew(SVerticalBox);
	TSharedRef<SButton> Riprendi = Stile->Pulsante(LOCTEXT("Riprendi", "Riprendi"),
		FSimpleDelegate::CreateLambda([this]() { OnRiprendi.ExecuteIfBound(); }), 20.f, Liberi);
	PrimoPulsante = Riprendi;
	Pulsanti->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 5.f)) [ Riga(Riprendi) ];

	// (02/11, passo 6, seconda parte) Le impostazioni: si aprono sopra la pausa.
	TSharedRef<SButton> Impostazioni = Stile->Pulsante(LOCTEXT("Impostazioni", "Impostazioni"),
		FSimpleDelegate::CreateLambda([this]() { OnImpostazioni.ExecuteIfBound(); }), 20.f, Liberi);
	Pulsanti->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 5.f)) [ Riga(Impostazioni) ];

	if (InArgs._bConPersonaggi)
	{
		Pulsanti->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 5.f))
		[
			Riga(Stile->Pulsante(LOCTEXT("Personaggi", "Torna ai personaggi"),
				FSimpleDelegate::CreateLambda([this]() { Esci(EValdorsoUscita::Personaggi); }), 20.f, Liberi))
		];
	}
	Pulsanti->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 5.f))
	[
		Riga(Stile->Pulsante(LOCTEXT("Menu", "Torna al menu"),
			FSimpleDelegate::CreateLambda([this]() { Esci(EValdorsoUscita::Menu); }), 20.f, Liberi))
	];
	Pulsanti->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 5.f))
	[
		Riga(Stile->Pulsante(LOCTEXT("Gioco", "Esci dal gioco"),
			FSimpleDelegate::CreateLambda([this]() { Esci(EValdorsoUscita::Gioco); }), 20.f, Liberi))
	];

	ChildSlot
	[
		SNew(SOverlay)

		// Il velo: il mondo dietro resta visibile, appena scurito (non si ferma).
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&Stile->Velo)
		]

		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Stile->SfondoPannello)
			.Padding(FMargin(50.f, 36.f))
			[
				SNew(SBox).WidthOverride(500.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Titolo", "Una sosta"))
						.Font(FontTitolo)
						.ColorAndOpacity_Lambda([]()
						{
							const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
							return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo * 0.6f));
						})
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 10.f, 0.f, 6.f))
					[
						Stile->Separatore(260.f)
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 18.f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Sottotitolo", "La valle non si ferma: intorno a te il tempo scorre."))
						.Font(Stile->Caratteri->Testo(19.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::TestoSecondario())
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
					]

					+ SVerticalBox::Slot().AutoHeight()
					[
						Pulsanti
					]

					// Il conto alla rovescia (o il motivo dell'annullamento).
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 18.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return bInUscita ? TestoConto() : Messaggio; })
						.Font(Stile->Caratteri->Testo(20.f))
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
						.Visibility_Lambda([this]() { return bInUscita || !Messaggio.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed; })
						.ColorAndOpacity_Lambda([this]()
						{
							if (bInUscita)
							{
								const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
								return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo));
							}
							return FSlateColor(bErrore ? ValdorsoTema::Brace() : ValdorsoTema::Pergamena());
						})
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 12.f, 0.f, 0.f))
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return bInUscita ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							Stile->Pulsante(LOCTEXT("Resta", "Resta nella valle"),
								FSimpleDelegate::CreateLambda([this]() { OnAnnulla.ExecuteIfBound(); }), 18.f)
						]
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 16.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Tasti", "Esc o F10 per tornare al gioco"))
						.Font(Stile->Caratteri->Testo(15.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.6f))
					]
				]
			]
		]
	];
}

void SValdorsoPausa::Esci(EValdorsoUscita Dove)
{
	if (bInUscita)
	{
		return;
	}
	bInUscita = true;
	bConta = false;
	DoveVa = Dove;
	Messaggio = FText::GetEmpty();
	// Il pulsante premuto si spegne: il fuoco passa alla pausa, così Esc ("resto") continua a funzionare.
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::SetDirectly);
	}
	OnEsci.ExecuteIfBound(Dove);
}

void SValdorsoPausa::MostraConto(EValdorsoUscita Dove, int32 Secondi)
{
	bInUscita = true;
	bConta = true;
	DoveVa = Dove;
	FineConto = FPlatformTime::Seconds() + FMath::Max(0, Secondi);
}

void SValdorsoPausa::Annullata(const FText& Motivo)
{
	bInUscita = false;
	bConta = false;
	Messaggio = Motivo;
	bErrore = true;
}

FText SValdorsoPausa::TestoConto() const
{
	if (!bConta)
	{
		return LOCTEXT("Attesa", "Il colono raccoglie le sue cose...");
	}
	const int32 Rimasti = FMath::Max(0, FMath::CeilToInt(static_cast<float>(FineConto - FPlatformTime::Seconds())));
	if (Rimasti <= 0)
	{
		return LOCTEXT("Parte", "Il colono lascia la valle.");
	}
	switch (DoveVa)
	{
	case EValdorsoUscita::Personaggi:
		return FText::Format(LOCTEXT("ContoPersonaggi", "Il colono torna al registro tra {0} secondi. Se vieni colpito o spostato, resti."), Rimasti);
	case EValdorsoUscita::Gioco:
		return FText::Format(LOCTEXT("ContoGioco", "Il colono si mette a dormire: il gioco si chiude tra {0} secondi. Se vieni colpito o spostato, resti."), Rimasti);
	default:
		return FText::Format(LOCTEXT("ContoMenu", "Il colono si mette a dormire: torni al menu tra {0} secondi. Se vieni colpito o spostato, resti."), Rimasti);
	}
}

FReply SValdorsoPausa::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::F10 || Tasto == EKeys::Gamepad_Special_Right || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		// Durante l'uscita il tasto vuol dire "resto"; altrimenti si torna al gioco.
		if (bInUscita)
		{
			OnAnnulla.ExecuteIfBound();
		}
		else
		{
			OnRiprendi.ExecuteIfBound();
		}
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoPausa::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Un clic sul velo non deve togliere il fuoco alla pausa (Esc smetterebbe di funzionare).
	// (Il fuoco va alla pausa stessa: durante l'uscita i pulsanti sono spenti e non lo prenderebbero.)
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

#undef LOCTEXT_NAMESPACE
