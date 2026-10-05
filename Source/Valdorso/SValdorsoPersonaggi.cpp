// Valdorso - La scelta del personaggio (vedi il .h).

#include "SValdorsoPersonaggi.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoRegole.h"
#include "ValdorsoTemaUI.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoPersonaggi"

void SValdorsoSceltaPersonaggio::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	OnScegli = InArgs._OnScegli;
	OnCrea = InArgs._OnCrea;
	OnCancella = InArgs._OnCancella;
	OnEsci = InArgs._OnEsci;

	FSlateFontInfo FontTitolo = Stile->Caratteri->Titolo(40.f, TEXT("Bold"));
	FontTitolo.LetterSpacing = 60;

	ChildSlot
	[
		SNew(SOverlay)

		// Fondo scuro: il mondo dietro non si vede ancora.
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&Stile->Pieno).ColorAndOpacity(ValdorsoTema::Fondo().CopyWithNewOpacity(0.96f))
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Stile->SfondoPannello)
			.Padding(FMargin(54.f, 40.f))
			[
				SNew(SBox).WidthOverride(640.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Titolo", "Il Registro dei coloni"))
						.Font(FontTitolo)
						.ColorAndOpacity_Lambda([]()
						{
							const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
							return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo * 0.6f));
						})
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 12.f, 0.f, 8.f))
					[
						Stile->Separatore(300.f)
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 20.f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Sottotitolo", "Scegli chi entra nella valle."))
						.Font(Stile->Caratteri->Testo(21.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::TestoSecondario())
					]

					// Le righe dei personaggi (si ridisegnano a ogni risposta del server).
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(Righe, SVerticalBox)
						.IsEnabled_Lambda([this]() { return !bInAttesa; })
					]

					// Il messaggio.
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 10.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return Messaggio; })
						.Font(Stile->Caratteri->Testo(21.f))
						.AutoWrapText(true)
						.Visibility_Lambda([this]() { return Messaggio.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
						.ColorAndOpacity_Lambda([this]()
						{
							if (bInAttesa)
							{
								const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
								return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo));
							}
							return FSlateColor(bErrore ? ValdorsoTema::Brace() : ValdorsoTema::Pergamena());
						})
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(FMargin(0.f, 24.f, 0.f, 0.f))
					[
						Stile->Pulsante(LOCTEXT("Esci", "Torna al menu"),
							FSimpleDelegate::CreateLambda([this]() { OnEsci.ExecuteIfBound(); }), 18.f)
					]
				]
			]
		]
	];

	Ridisegna();
}

void SValdorsoSceltaPersonaggio::Aggiorna(const TArray<FValdorsoPersonaggioBreve>& Nuovi, const FText& Testo, bool bComeErrore)
{
	Elenco = Nuovi;
	Messaggio = Testo;
	bErrore = bComeErrore;
	bInAttesa = false;
	IdDaCancellare.Empty();
	Ridisegna();
}

void SValdorsoSceltaPersonaggio::Attendi(const FText& Testo)
{
	bInAttesa = true;
	bErrore = false;
	Messaggio = Testo;
}

TSharedPtr<SWidget> SValdorsoSceltaPersonaggio::FuocoIniziale() const
{
	if (CampoConferma.IsValid())
	{
		return CampoConferma;
	}
	if (PrimoPulsante.IsValid())
	{
		return PrimoPulsante;
	}
	return CampoNome;
}

void SValdorsoSceltaPersonaggio::Ridisegna()
{
	if (!Righe.IsValid())
	{
		return;
	}
	// Quello che il giocatore aveva scritto resta (se il server ha detto di no, non deve riscriverlo).
	const FText NomeScritto = CampoNome.IsValid() ? CampoNome->GetText() : FText::GetEmpty();
	Righe->ClearChildren();
	CampoNome.Reset();
	CampoConferma.Reset();
	PrimoPulsante.Reset();
	ScrittoPrima = NomeScritto;

	for (const FValdorsoPersonaggioBreve& Personaggio : Elenco)
	{
		Righe->AddSlot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 12.f)) [ RigaPersonaggio(Personaggio) ];
	}
	for (int32 Posto = Elenco.Num(); Posto < ValdorsoRegole::PersonaggiPerAccount; ++Posto)
	{
		Righe->AddSlot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 12.f)) [ Posto == Elenco.Num() ? RigaNuovo() : RigaLibera() ];
	}

	if (TSharedPtr<SWidget> Fuoco = FuocoIniziale())
	{
		FSlateApplication::Get().SetAllUserFocus(Fuoco, EFocusCause::SetDirectly);
	}
}

TSharedRef<SWidget> SValdorsoSceltaPersonaggio::RigaPersonaggio(const FValdorsoPersonaggioBreve& Personaggio)
{
	const FString Id = Personaggio.Id;
	const FString Nome = Personaggio.Nome;

	// "Ultima volta il 04/10/2026 · 2 ore e 15 minuti nella valle"
	const int64 MinutiTotali = Personaggio.TempoDiGioco / 60;
	const FString Tempo = MinutiTotali >= 60
		? FString::Printf(TEXT("%lld ore e %lld minuti nella valle"), MinutiTotali / 60, MinutiTotali % 60)
		: FString::Printf(TEXT("%lld minuti nella valle"), MinutiTotali);
	const FString Quando = Personaggio.UltimoGioco > 0
		? TEXT("Ultima volta il ") + FDateTime::FromUnixTimestamp(Personaggio.UltimoGioco).ToString(TEXT("%d/%m/%Y")) + TEXT(" · ") + Tempo
		: FString(TEXT("Non è ancora entrato nella valle"));

	FSlateFontInfo FontNome = Stile->Caratteri->Titolo(26.f, TEXT("Bold"));
	FontNome.LetterSpacing = 40;

	TSharedRef<SVerticalBox> Contenuto = SNew(SVerticalBox);

	TSharedRef<SButton> Entra = Stile->Pulsante(LOCTEXT("Entra", "Entra"),
		FSimpleDelegate::CreateLambda([this, Id]() { if (!bInAttesa) { OnScegli.ExecuteIfBound(Id); } }), 20.f);
	if (!PrimoPulsante.IsValid())
	{
		PrimoPulsante = Entra;
	}

	Contenuto->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(Nome)).Font(FontNome).ColorAndOpacity(ValdorsoTema::Oro())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 2.f, 0.f, 0.f))
			[
				SNew(STextBlock).Text(FText::FromString(Quando))
				.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::TestoSecondario())
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(12.f, 0.f, 0.f, 0.f))
		[
			SNew(SButton)
			.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
			.Visibility(IdDaCancellare == Id ? EVisibility::Collapsed : EVisibility::Visible)
			.OnClicked_Lambda([this, Id]()
			{
				IdDaCancellare = Id;
				Messaggio = FText::GetEmpty();
				Ridisegna();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(LOCTEXT("Cancella", "Cancella"))
				.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::Brace().CopyWithNewOpacity(0.8f))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(16.f, 0.f, 0.f, 0.f))
		[
			Entra
		]
	];

	// La conferma della cancellazione: si riscrive il nome.
	if (IdDaCancellare == Id)
	{
		CampoConferma = Stile->Campo(FText::FromString(Nome), false,
			FOnTextCommitted::CreateLambda([this, Id, Nome](const FText& Testo, ETextCommit::Type Tipo)
			{
				// (05/10) Solo con il nome giusto, come il server (spazi ai lati e maiuscole non contano).
				if (Tipo == ETextCommit::OnEnter && !bInAttesa && Testo.ToString().TrimStartAndEnd().Equals(Nome, ESearchCase::IgnoreCase))
				{
					OnCancella.ExecuteIfBound(Id, Testo.ToString());
				}
			}));
		TSharedPtr<SEditableTextBox> Campo = CampoConferma;
		Contenuto->AddSlot().AutoHeight().Padding(FMargin(0.f, 12.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("ConfermaTesto", "Cancellare {0} è per sempre: ricordi, luogo e tutto ciò che ha. Il suo nome resterà riservato per 30 giorni. Per confermare, scrivi il suo nome."), FText::FromString(Nome)))
			.Font(Stile->Caratteri->Testo(18.f))
			.ColorAndOpacity(ValdorsoTema::Pergamena())
			.AutoWrapText(true)
		];
		Contenuto->AddSlot().AutoHeight().Padding(FMargin(0.f, 8.f, 0.f, 0.f)) [ CampoConferma.ToSharedRef() ];
		Contenuto->AddSlot().AutoHeight().Padding(FMargin(0.f, 10.f, 0.f, 0.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Left)
			[
				Stile->Pulsante(LOCTEXT("Annulla", "Annulla"), FSimpleDelegate::CreateLambda([this]()
				{
					IdDaCancellare.Empty();
					Ridisegna();
				}), 18.f)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Stile->Pulsante(LOCTEXT("CancellaSempre", "Cancella per sempre"), FSimpleDelegate::CreateLambda([this, Id, Campo]()
				{
					if (!bInAttesa && Campo.IsValid())
					{
						OnCancella.ExecuteIfBound(Id, Campo->GetText().ToString());
					}
				}), 18.f,
				// (05/10) Acceso solo quando il nome scritto è quello giusto.
				TAttribute<bool>::CreateLambda([this, Campo, Nome]()
				{
					return !bInAttesa && Campo.IsValid() && Campo->GetText().ToString().TrimStartAndEnd().Equals(Nome, ESearchCase::IgnoreCase);
				}))
			]
		];
	}

	return SNew(SBorder)
		.BorderImage(&Stile->SfondoCampo)
		.Padding(FMargin(20.f, 14.f))
		[
			Contenuto
		];
}

TSharedRef<SWidget> SValdorsoSceltaPersonaggio::RigaNuovo()
{
	CampoNome = Stile->Campo(LOCTEXT("SugNome", "Il nome del tuo nuovo personaggio"), false,
		FOnTextCommitted::CreateLambda([this](const FText&, ETextCommit::Type Tipo)
		{
			if (Tipo == ETextCommit::OnEnter)
			{
				Crea();
			}
		}));
	CampoNome->SetText(ScrittoPrima);

	return SNew(SBorder)
		.BorderImage(&Stile->SfondoCampo)
		.Padding(FMargin(20.f, 14.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(2.f, 0.f, 0.f, 6.f))
			[
				Stile->Etichetta(LOCTEXT("EtNuovo", "UN NUOVO COLONO"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 6.f))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SpiegaNome", "Da 3 a 20 lettere, anche accentate; spazi, apostrofi e trattini tra le parole. Il nome è unico nella valle."))
				.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::TestoSecondario())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					CampoNome.ToSharedRef()
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(14.f, 0.f, 0.f, 0.f))
				[
					Stile->Pulsante(LOCTEXT("Crea", "Scrivi nel registro"), FSimpleDelegate::CreateSP(this, &SValdorsoSceltaPersonaggio::Crea), 18.f)
				]
			]
		];
}

TSharedRef<SWidget> SValdorsoSceltaPersonaggio::RigaLibera()
{
	return SNew(SBorder)
		.BorderImage(&Stile->SfondoCampo)
		.Padding(FMargin(20.f, 14.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Libero", "Posto libero"))
			.Font(Stile->Caratteri->Testo(19.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.5f))
		];
}

void SValdorsoSceltaPersonaggio::Crea()
{
	if (bInAttesa || !CampoNome.IsValid())
	{
		return;
	}
	// Le stesse regole del server, per avvisare subito (il server ricontrolla comunque).
	const FString Nome = ValdorsoRegole::NormalizzaNomePersonaggio(CampoNome->GetText().ToString());
	const FString Problema = ValdorsoRegole::ProblemaNomePersonaggio(Nome);
	if (!Problema.IsEmpty())
	{
		Messaggio = FText::FromString(Problema);
		bErrore = true;
		return;
	}
	OnCrea.ExecuteIfBound(Nome);
}

#undef LOCTEXT_NAMESPACE
