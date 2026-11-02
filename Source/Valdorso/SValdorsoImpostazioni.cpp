// Valdorso - La schermata delle impostazioni (vedi il .h). Scritto da Claude il 02/11/2026.

#include "SValdorsoImpostazioni.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoTemaUI.h"
#include "HAL/PlatformTime.h"
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

#define LOCTEXT_NAMESPACE "ValdorsoImpostazioni"

namespace
{
	FText Percento(float Valore)
	{
		return FText::Format(LOCTEXT("Percento", "{0}%"), FMath::RoundToInt(Valore * 100.f));
	}

	FText SiNo(bool bValore)
	{
		return bValore ? LOCTEXT("Si", "Sì") : LOCTEXT("No", "No");
	}

	/** Un passo di 0,1 tra Minimo e Massimo (arrotondato, così non si accumulano gli errori). */
	float Passo01(float Valore, int32 Verso, float Minimo, float Massimo)
	{
		return FMath::Clamp(FMath::RoundToFloat((Valore + 0.1f * Verso) * 20.f) / 20.f, Minimo, Massimo);
	}

	const float Grandezze[] = { 1.f, 1.15f, 1.3f };

	int32 IndiceGrandezza(float Valore)
	{
		int32 Migliore = 0;
		for (int32 i = 1; i < static_cast<int32>(UE_ARRAY_COUNT(Grandezze)); ++i)
		{
			if (FMath::Abs(Grandezze[i] - Valore) < FMath::Abs(Grandezze[Migliore] - Valore))
			{
				Migliore = i;
			}
		}
		return Migliore;
	}
}

void SValdorsoImpostazioni::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	OnChiudi = InArgs._OnChiudi;
	Bozza = ValdorsoImpostazioni::Correnti();

	FSlateFontInfo FontTitolo = Stile->Caratteri->Titolo(30.f, TEXT("Bold"));
	FontTitolo.LetterSpacing = 30;

	TSharedRef<SVerticalBox> Voci = SNew(SVerticalBox);

	AggiungiTitolo(Voci, LOCTEXT("Grafica", "Grafica"));
	AggiungiVoce(Voci, LOCTEXT("Qualita", "Qualità"),
		[this]()
		{
			switch (Bozza.Qualita)
			{
			case 0: return LOCTEXT("Bassa", "Bassa");
			case 1: return LOCTEXT("Media", "Media");
			case 2: return LOCTEXT("Alta", "Alta");
			case 3: return LOCTEXT("Epica", "Epica");
			default: return LOCTEXT("Automatica", "Automatica");
			}
		},
		[this](int32 Verso) { Bozza.Qualita = FMath::Clamp(Bozza.Qualita + Verso, -1, 3); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("Finestra", "Finestra"),
		[this]()
		{
			switch (Bozza.Finestra)
			{
			case 0: return LOCTEXT("SchermoIntero", "Schermo intero");
			case 2: return LOCTEXT("InFinestra", "In finestra");
			default: return LOCTEXT("SenzaBordi", "Senza bordi");
			}
		},
		[this](int32 Verso) { Bozza.Finestra = FMath::Clamp(Bozza.Finestra + Verso, 0, 2); Applica(); });

	AggiungiTitolo(Voci, LOCTEXT("Audio", "Audio"));
	AggiungiVoce(Voci, LOCTEXT("VolumeGenerale", "Volume generale"),
		[this]() { return Percento(Bozza.VolumeGenerale); },
		[this](int32 Verso) { Bozza.VolumeGenerale = Passo01(Bozza.VolumeGenerale, Verso, 0.f, 1.f); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("VolumeMusica", "Musica"),
		[this]() { return Percento(Bozza.VolumeMusica); },
		[this](int32 Verso) { Bozza.VolumeMusica = Passo01(Bozza.VolumeMusica, Verso, 0.f, 1.f); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("VolumeEffetti", "Effetti"),
		[this]() { return Percento(Bozza.VolumeEffetti); },
		[this](int32 Verso) { Bozza.VolumeEffetti = Passo01(Bozza.VolumeEffetti, Verso, 0.f, 1.f); Applica(); });

	AggiungiTitolo(Voci, LOCTEXT("Controlli", "Controlli"));
	AggiungiVoce(Voci, LOCTEXT("Sensibilita", "Sensibilità della telecamera"),
		[this]() { return Percento(Bozza.Sensibilita); },
		[this](int32 Verso) { Bozza.Sensibilita = Passo01(Bozza.Sensibilita, Verso, 0.3f, 3.f); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("InvertiY", "Inverti su e giù"),
		[this]() { return SiNo(Bozza.bInvertiY); },
		[this](int32) { Bozza.bInvertiY = !Bozza.bInvertiY; Applica(); });

	AggiungiTitolo(Voci, LOCTEXT("Accessibilita", "Accessibilità"));
	AggiungiVoce(Voci, LOCTEXT("GrandezzaTesto", "Grandezza delle scritte"),
		[this]()
		{
			switch (IndiceGrandezza(Bozza.GrandezzaTesto))
			{
			case 1: return LOCTEXT("Grandi", "Grandi");
			case 2: return LOCTEXT("MoltoGrandi", "Molto grandi");
			default: return LOCTEXT("Normali", "Normali");
			}
		},
		[this](int32 Verso)
		{
			Bozza.GrandezzaTesto = Grandezze[FMath::Clamp(IndiceGrandezza(Bozza.GrandezzaTesto) + Verso, 0, 2)];
			Applica();
		});
	AggiungiVoce(Voci, LOCTEXT("MenoMovimenti", "Meno movimento sullo schermo"),
		[this]() { return SiNo(Bozza.bMenoMovimenti); },
		[this](int32) { Bozza.bMenoMovimenti = !Bozza.bMenoMovimenti; Applica(); });
	AggiungiVoce(Voci, LOCTEXT("Daltonismo", "Colori per il daltonismo"),
		[this]()
		{
			switch (Bozza.Daltonismo)
			{
			case 1: return LOCTEXT("Deuteranopia", "Rosso-verde (deuteranopia)");
			case 2: return LOCTEXT("Protanopia", "Rosso-verde (protanopia)");
			case 3: return LOCTEXT("Tritanopia", "Blu-giallo (tritanopia)");
			default: return LOCTEXT("Nessuno", "Nessuno");
			}
		},
		[this](int32 Verso) { Bozza.Daltonismo = (Bozza.Daltonismo + Verso + 4) % 4; Applica(); });

	TSharedRef<SButton> Fatto = Stile->Pulsante(LOCTEXT("Fatto", "Fatto"),
		FSimpleDelegate::CreateLambda([this]() { OnChiudi.ExecuteIfBound(); }), 20.f);

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
			.BorderImage(&Stile->SfondoPannello)
			.Padding(FMargin(46.f, 32.f))
			[
				SNew(SBox).WidthOverride(720.f).MaxDesiredHeight(860.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Titolo", "Impostazioni"))
						.Font(FontTitolo)
						.ColorAndOpacity_Lambda([]()
						{
							const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
							return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo * 0.6f));
						})
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 10.f, 0.f, 4.f))
					[
						Stile->Separatore(300.f)
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 10.f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Sottotitolo", "Valgono subito e restano su questo PC."))
						.Font(Stile->Caratteri->Testo(18.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::TestoSecondario())
					]

					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							Voci
						]
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 18.f, 0.f, 0.f))
					[
						SNew(SBox).WidthOverride(240.f)[ Fatto ]
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 10.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Tasti", "Clic (o Invio) sulle frecce per cambiare · Esc o F10 per tornare"))
						.Font(Stile->Caratteri->Testo(15.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.6f))
					]
				]
			]
		]
	];
}

void SValdorsoImpostazioni::AggiungiTitolo(const TSharedRef<SVerticalBox>& Dove, const FText& Titolo)
{
	Dove->AddSlot().AutoHeight().Padding(FMargin(0.f, 14.f, 0.f, 4.f))
	[
		SNew(STextBlock)
		.Text(Titolo)
		.Font(Stile->Caratteri->Titolo(19.f, TEXT("Bold")))
		.ColorAndOpacity(ValdorsoTema::Brace())
	];
}

void SValdorsoImpostazioni::AggiungiVoce(const TSharedRef<SVerticalBox>& Dove, const FText& Nome, TFunction<FText()> Valore, TFunction<void(int32)> Passo)
{
	TSharedRef<SButton> Meno = Stile->Pulsante(FText::FromString(TEXT("<")),
		FSimpleDelegate::CreateLambda([Passo]() { Passo(-1); }), 16.f);
	TSharedRef<SButton> Piu = Stile->Pulsante(FText::FromString(TEXT(">")),
		FSimpleDelegate::CreateLambda([Passo]() { Passo(1); }), 16.f);
	if (!PrimoPulsante.IsValid())
	{
		PrimoPulsante = Piu;
	}
	Dove->AddSlot().AutoHeight().Padding(FMargin(0.f, 3.f))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(Nome)
			.Font(Stile->Caratteri->Testo(20.f))
			.ColorAndOpacity(ValdorsoTema::Pergamena())
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Meno
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(260.f).HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Valore]() { return Valore(); })
				.Font(Stile->Caratteri->Titolo(16.f, TEXT("Bold")))
				.ColorAndOpacity(ValdorsoTema::OroChiaro())
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Piu
		]
	];
}

void SValdorsoImpostazioni::Applica()
{
	ValdorsoImpostazioni::Cambia(Bozza);
}

FReply SValdorsoImpostazioni::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::F10 || Tasto == EKeys::Gamepad_Special_Right || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoImpostazioni::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

#undef LOCTEXT_NAMESPACE
