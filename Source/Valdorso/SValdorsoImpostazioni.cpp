// Valdorso - La schermata delle impostazioni (vedi il .h). Scritto da Claude il 02/11/2026.

#include "SValdorsoImpostazioni.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoTemaUI.h"
#include "ValdorsoTasti.h"
#include "Framework/Application/SlateApplication.h"
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
	// (10/11) quanto lontano si vede bene (ValdorsoImpostazioni.h)
	AggiungiVoce(Voci, LOCTEXT("Distanza", "Distanza visiva"),
		[this]()
		{
			switch (Bozza.Distanza)
			{
			case 0: return LOCTEXT("DistanzaVicina", "Vicina");
			case 1: return LOCTEXT("DistanzaNormale", "Normale");
			case 3: return LOCTEXT("DistanzaLontanissima", "Lontanissima");
			default: return LOCTEXT("DistanzaLontana", "Lontana");
			}
		},
		[this](int32 Verso) { Bozza.Distanza = FMath::Clamp(Bozza.Distanza + Verso, 0, 3); Applica(); });

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

	// (11/11, v0.1.5) la corsa "premi una volta" e i tasti da cambiare (ValdorsoTasti.h)
	AggiungiVoce(Voci, LOCTEXT("CorsaAlterna", "Corsa: premi una volta (non tenere premuto)"),
		[this]() { return SiNo(Bozza.bCorsaAlterna); },
		[this](int32) { Bozza.bCorsaAlterna = !Bozza.bCorsaAlterna; Applica(); });

	AggiungiTitolo(Voci, LOCTEXT("TitoloTasti", "Tasti (clic sulla freccia, poi premi il tasto nuovo)"));
	for (int32 k = 0; k < static_cast<int32>(EValdorsoTasto::Numero); ++k)
	{
		const EValdorsoTasto Azione = static_cast<EValdorsoTasto>(k);
		AggiungiVoce(Voci, ValdorsoTasti::Nome(Azione),
			[this, k, Azione]()
			{
				return TastoInAttesa == k ? LOCTEXT("PremiTasto", "Premi un tasto... (Esc: lascia)") : ValdorsoTasti::NomeTasto(ValdorsoTasti::Tasto(Azione));
			},
			[this, k](int32)
			{
				TastoInAttesa = k;
				if (FSlateApplication::IsInitialized())
				{
					FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::SetDirectly);
				}
			});
	}
	AggiungiVoce(Voci, LOCTEXT("TastiDiSerie", "Tutti i tasti come di serie"),
		[]() { return ValdorsoTasti::CorpoCambiato() ? LOCTEXT("Rimetti", "rimetti") : LOCTEXT("GiaDiSerie", "-"); },
		[this](int32) { TastoInAttesa = -1; ValdorsoTasti::RimettiDiSerie(); });

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
	// (11/11, v0.1.5) le scosse della telecamera e i sottotitoli dei suoni
	AggiungiVoce(Voci, LOCTEXT("Scosse", "Quanto trema la telecamera ai colpi"),
		[this]() { return Percento(Bozza.Scosse); },
		[this](int32 Verso) { Bozza.Scosse = FMath::Clamp(FMath::RoundToFloat((Bozza.Scosse + 0.25f * Verso) * 4.f) / 4.f, 0.f, 1.f); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("Sottotitoli", "Scritte dei suoni importanti"),
		[this]() { return SiNo(Bozza.bSottotitoli); },
		[this](int32) { Bozza.bSottotitoli = !Bozza.bSottotitoli; Applica(); });

	// (11/11, v0.1.5) l'interfaccia su misura: dove stanno le barre, la bussola e la chat (ValdorsoInterfaccia.h)
	AggiungiTitolo(Voci, LOCTEXT("Interfaccia", "Interfaccia (barre, bussola, chat)"));
	const auto Pixel = [](int32 Valore) { return FText::Format(LOCTEXT("Pixel", "{0} px"), FText::AsNumber(Valore)); };
	AggiungiVoce(Voci, LOCTEXT("BarreX", "Barre: a sinistra / a destra"),
		[this, Pixel]() { return Pixel(Bozza.BarreX); },
		[this](int32 Verso) { Bozza.BarreX = FMath::Clamp(Bozza.BarreX + 40 * Verso, -1200, 1200); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("BarreY", "Barre: piu' su / piu' giu'"),
		[this, Pixel]() { return Pixel(Bozza.BarreY); },
		[this](int32 Verso) { Bozza.BarreY = FMath::Clamp(Bozza.BarreY + 40 * Verso, -900, 0); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("BussolaX", "Bussola: a sinistra / a destra"),
		[this, Pixel]() { return Pixel(Bozza.BussolaX); },
		[this](int32 Verso) { Bozza.BussolaX = FMath::Clamp(Bozza.BussolaX + 40 * Verso, -1200, 1200); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("BussolaY", "Bussola: piu' su / piu' giu'"),
		[this, Pixel]() { return Pixel(Bozza.BussolaY); },
		[this](int32 Verso) { Bozza.BussolaY = FMath::Clamp(Bozza.BussolaY + 40 * Verso, 0, 900); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("ChatX", "Chat: a sinistra / a destra"),
		[this, Pixel]() { return Pixel(Bozza.ChatX); },
		[this](int32 Verso) { Bozza.ChatX = FMath::Clamp(Bozza.ChatX + 40 * Verso, 0, 1600); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("ChatY", "Chat: piu' su / piu' giu'"),
		[this, Pixel]() { return Pixel(Bozza.ChatY); },
		[this](int32 Verso) { Bozza.ChatY = FMath::Clamp(Bozza.ChatY + 40 * Verso, -900, 300); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("GrandezzaHud", "Grandezza di barre, bussola e chat"),
		[this]() { return Percento(Bozza.GrandezzaHud); },
		[this](int32 Verso) { Bozza.GrandezzaHud = FMath::Clamp(FMath::RoundToFloat((Bozza.GrandezzaHud + 0.1f * Verso) * 10.f) / 10.f, 0.8f, 1.4f); Applica(); });
	AggiungiVoce(Voci, LOCTEXT("HudDiSerie", "Interfaccia come di serie"),
		[]() { return LOCTEXT("Rimetti2", "rimetti"); },
		[this](int32)
		{
			Bozza.BarreX = Bozza.BarreY = Bozza.BussolaX = Bozza.BussolaY = Bozza.ChatX = Bozza.ChatY = 0;
			Bozza.GrandezzaHud = 1.f;
			Applica();
		});

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

FReply SValdorsoImpostazioni::Cattura(const FKey& Tasto)
{
	if (TastoInAttesa < 0)
	{
		return FReply::Unhandled();
	}
	if (Tasto == EKeys::Escape || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		// (con il controller: il tasto B/cerchio annulla, i tasti del controller non si cambiano da qui)
		TastoInAttesa = -1;
		return FReply::Handled();
	}
	// un tasto riservato (la pausa, la barra 1-8...) non si prende: si aspetta un altro tasto
	if (ValdorsoTasti::Imposta(static_cast<EValdorsoTasto>(TastoInAttesa), Tasto))
	{
		TastoInAttesa = -1;
	}
	return FReply::Handled();
}

FReply SValdorsoImpostazioni::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	// (11/11, v0.1.5) si sta aspettando il tasto nuovo di un'azione
	if (TastoInAttesa >= 0)
	{
		return Cattura(Tasto);
	}
	if (Tasto == EKeys::Escape || Tasto == EKeys::F10 || Tasto == EKeys::Gamepad_Special_Right || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoImpostazioni::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (TastoInAttesa >= 0)
	{
		return Cattura(MouseEvent.GetEffectingButton());
	}
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

#undef LOCTEXT_NAMESPACE
