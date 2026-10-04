// Valdorso - Il Registro di Val d'Orso a schermo (vedi il .h).
// Attenzione ai nomi (unity build): le funzioni del namespace anonimo cominciano tutte con RegSchermo, così non si
// scontrano con quelle degli altri file; le variabili locali non si chiamano come le funzioni di ValdorsoRegistro
// (Voci, Domanda, Risposta, Testo, Problema) né come i campi della classe.

#include "SValdorsoRegistro.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoRegole.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoRegistroSchermo"

namespace
{
	/** Quanto dura il nero dopo la firma, prima che il personaggio entri. */
	constexpr float RegSchermoSecondiFinale = 4.5f;

	/** Oltre questo numero di risposte, la pagina le mette su due colonne. */
	constexpr int32 RegSchermoMassimoUnaColonna = 7;

	using FRegSchermoPagina = SValdorsoRegistroColono::EPagina;

	/** La domanda di una pagina con una sola domanda a scelta (Numero per le altre). */
	ValdorsoRegistro::EDomanda RegSchermoDomandaDellaPagina(FRegSchermoPagina Quale)
	{
		switch (Quale)
		{
		case FRegSchermoPagina::Fede: return ValdorsoRegistro::EDomanda::Fede;
		case FRegSchermoPagina::Origine: return ValdorsoRegistro::EDomanda::Origine;
		case FRegSchermoPagina::Mestiere: return ValdorsoRegistro::EDomanda::Mestiere;
		case FRegSchermoPagina::Motivo: return ValdorsoRegistro::EDomanda::Motivo;
		case FRegSchermoPagina::Ricordo: return ValdorsoRegistro::EDomanda::Ricordo;
		case FRegSchermoPagina::Paura: return ValdorsoRegistro::EDomanda::Paura;
		case FRegSchermoPagina::Richiamo: return ValdorsoRegistro::EDomanda::RichiamoMagia;
		default: return ValdorsoRegistro::EDomanda::Numero;
		}
	}

	/** La pagina in cui si risponde a una domanda. */
	FRegSchermoPagina RegSchermoPaginaDellaDomanda(ValdorsoRegistro::EDomanda Quale)
	{
		switch (Quale)
		{
		case ValdorsoRegistro::EDomanda::Sesso: return FRegSchermoPagina::ChiSei;
		case ValdorsoRegistro::EDomanda::Fede: return FRegSchermoPagina::Fede;
		case ValdorsoRegistro::EDomanda::Origine: return FRegSchermoPagina::Origine;
		case ValdorsoRegistro::EDomanda::Mestiere: return FRegSchermoPagina::Mestiere;
		case ValdorsoRegistro::EDomanda::Motivo: return FRegSchermoPagina::Motivo;
		case ValdorsoRegistro::EDomanda::Ricordo: return FRegSchermoPagina::Ricordo;
		case ValdorsoRegistro::EDomanda::Paura: return FRegSchermoPagina::Paura;
		case ValdorsoRegistro::EDomanda::RichiamoMagia: return FRegSchermoPagina::Richiamo;
		case ValdorsoRegistro::EDomanda::Numero: return FRegSchermoPagina::Racconto;
		default: return FRegSchermoPagina::Carattere;
		}
	}

	/** Le quattro coppie del carattere, nell'ordine della pagina. */
	const ValdorsoRegistro::EDomanda RegSchermoCarattere[] = {
		ValdorsoRegistro::EDomanda::Onesta, ValdorsoRegistro::EDomanda::Coraggio,
		ValdorsoRegistro::EDomanda::Devozione, ValdorsoRegistro::EDomanda::Animo
	};
}

void SValdorsoRegistroColono::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	OnSalva = InArgs._OnSalva;
	OnFirmato = InArgs._OnFirmato;
	OnTorna = InArgs._OnTorna;
	Nome = InArgs._NomePersonaggio;
	Bozza = InArgs._Registro;
	Dadi.Initialize(static_cast<int32>(FPlatformTime::Cycles()));

	// Quello che c'è già sul server non si rimanda.
	Inviata = Bozza;
	bInviataValida = true;

	// Un racconto salvato diverso da quello che il sacerdote scriverebbe è stato ritoccato dal giocatore.
	bRaccontoToccato = !Bozza.Racconto.IsEmpty()
		&& !Bozza.Racconto.Equals(ValdorsoRegistro::ComponiRacconto(Bozza, Nome), ESearchCase::CaseSensitive);

	// L'età parte da 25 (si vede subito e si cambia con i pulsanti).
	if (Bozza.Eta < ValdorsoRegistro::EtaMinima || Bozza.Eta > ValdorsoRegistro::EtaMassima)
	{
		Bozza.Eta = 25;
	}

	// Le risposte: chiare come i pulsanti, con il bordo d'oro pieno quando sono scelte.
	StileVoce = Stile->StilePulsante;
	StileVoce.SetNormalPadding(FMargin(16.f, 7.f)).SetPressedPadding(FMargin(16.f, 8.f, 16.f, 6.f));
	StileVoceScelta = StileVoce;
	StileVoceScelta
		.SetNormal(FSlateRoundedBoxBrush(ValdorsoTema::PulsanteSopra(), 3.f, ValdorsoTema::Oro(), 1.5f))
		.SetHovered(FSlateRoundedBoxBrush(ValdorsoTema::PulsantePremuto(), 3.f, ValdorsoTema::OroChiaro(), 1.5f));

	FSlateFontInfo FontNome = Stile->Caratteri->Titolo(30.f, TEXT("Bold"));
	FontNome.LetterSpacing = 50;

	// I dodici rombi in cima: la pagina di adesso è chiara, quelle complete sono d'oro.
	TSharedRef<SHorizontalBox> Rombi = SNew(SHorizontalBox);
	for (int32 i = 0; i < static_cast<int32>(EPagina::Numero); ++i)
	{
		const EPagina Questa = static_cast<EPagina>(i);
		Rombi->AddSlot().AutoWidth().Padding(FMargin(5.f, 0.f))
		[
			Stile->Diamante(7.f, TAttribute<FSlateColor>::CreateLambda([this, Questa]()
			{
				if (Questa == Pagina)
				{
					return FSlateColor(ValdorsoTema::OroChiaro());
				}
				return FSlateColor(PaginaCompleta(Questa) ? ValdorsoTema::Oro() : ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.3f));
			}))
		];
	}

	TSharedRef<SButton> Prossimo = Stile->Pulsante(
		TAttribute<FText>::CreateLambda([this]()
		{
			return Pagina == EPagina::Firma ? LOCTEXT("Firma", "Firma il registro") : LOCTEXT("Avanti", "Avanti");
		}),
		FSimpleDelegate::CreateLambda([this]()
		{
			if (Pagina == EPagina::Firma)
			{
				Firma();
			}
			else
			{
				Avanti();
			}
		}),
		20.f,
		TAttribute<bool>::CreateLambda([this]()
		{
			if (bInFirma || bFinale)
			{
				return false;
			}
			if (Pagina == EPagina::Firma)
			{
				return ValdorsoRegistro::Problema(PerLaFirma(), true).IsEmpty();
			}
			return PaginaCompleta(Pagina);
		}));
	PulsanteAvanti = Prossimo;

	TSharedRef<SButton> DadiPagina = Stile->Pulsante(LOCTEXT("DadiPagina", "I dadi del destino"),
		FSimpleDelegate::CreateSP(this, &SValdorsoRegistroColono::TiraDadiPagina), 16.f,
		TAttribute<bool>::CreateLambda([this]() { return !bInFirma && !bFinale && !bInUscita; }));
	DadiPagina->SetVisibility(TAttribute<EVisibility>::CreateLambda([this]()
	{
		return Pagina <= EPagina::Richiamo ? EVisibility::Visible : EVisibility::Collapsed;
	}));
	DadiPagina->SetToolTipText(LOCTEXT("DadiPaginaAiuto", "Una risposta a caso per questa pagina (Y sul controller)."));

	auto Collegamento = [this](const FText& Scritta, FSimpleDelegate Azione)
	{
		return SNew(SButton)
			.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
			.IsEnabled_Lambda([this]() { return !bInFirma && !bFinale && !bInUscita; })
			.OnClicked_Lambda([Azione]() { Azione.ExecuteIfBound(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(Scritta)
				.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::TestoSecondario())
			];
	};

	ChildSlot
	[
		SNew(SOverlay)

		// Fondo scuro (al passo 4.2b qui dietro ci sarà il palco con il ritratto).
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&Stile->Pieno).ColorAndOpacity(ValdorsoTema::Fondo())
		]

		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)

			// La pergamena, a sinistra.
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(70.f, 30.f, 0.f, 30.f))
			[
				SNew(SBorder)
				.BorderImage(&Stile->SfondoPannello)
				.Padding(FMargin(44.f, 30.f))
				[
					SNew(SBox).WidthOverride(680.f)
					[
						SNew(SVerticalBox)

						// Intestazione e pagina.
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
							[
								Stile->Etichetta(LOCTEXT("Intestazione", "IL REGISTRO DI VAL D'ORSO"))
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									return FText::Format(LOCTEXT("NumeroPagina", "Pagina {0} di {1}"),
										FText::AsNumber(static_cast<int32>(Pagina) + 1), FText::AsNumber(static_cast<int32>(EPagina::Numero)));
								})
								.Font(Stile->Caratteri->Testo(16.f, TEXT("Italic")))
								.ColorAndOpacity(ValdorsoTema::TestoSecondario())
							]
						]

						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 12.f, 0.f, 0.f))
						[
							Rombi
						]

						// Il nome del colono.
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 14.f, 0.f, 0.f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(Nome))
							.Font(FontNome)
							.ColorAndOpacity_Lambda([]()
							{
								const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
								return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo * 0.5f));
							})
						]

						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 6.f, 0.f, 10.f))
						[
							Stile->Separatore(300.f)
						]

						// La frase della storia che apre la pagina.
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 14.f))
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return FrasePagina(Pagina); })
							.Font(Stile->Caratteri->Testo(19.f, TEXT("Italic")))
							.ColorAndOpacity(ValdorsoTema::TestoSecondario())
							.Justification(ETextJustify::Center)
							.AutoWrapText(true)
						]

						// La domanda.
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 12.f))
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return TitoloPagina(Pagina); })
							.Font(Stile->Caratteri->Testo(24.f, TEXT("SemiBold")))
							.ColorAndOpacity(ValdorsoTema::Pergamena())
							.AutoWrapText(true)
						]

						// Il contenuto della pagina (si ridisegna a ogni cambio di pagina).
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).MaxDesiredHeight(470.f)
							[
								SNew(SScrollBox)
								+ SScrollBox::Slot()
								[
									SAssignNew(Corpo, SBox)
								]
							]
						]

						// Il messaggio.
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 10.f, 0.f, 0.f))
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return Messaggio; })
							.Font(Stile->Caratteri->Testo(19.f))
							.AutoWrapText(true)
							.Visibility_Lambda([this]() { return Messaggio.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
							.ColorAndOpacity_Lambda([this]()
							{
								if (bInFirma)
								{
									const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
									return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo));
								}
								return FSlateColor(bErrore ? ValdorsoTema::Brace() : ValdorsoTema::Pergamena());
							})
						]

						// Indietro, i dadi, Avanti (o Firma).
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 20.f, 0.f, 0.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								Stile->Pulsante(LOCTEXT("Indietro", "Indietro"),
									FSimpleDelegate::CreateSP(this, &SValdorsoRegistroColono::Indietro), 18.f,
									TAttribute<bool>::CreateLambda([this]() { return Pagina != EPagina::ChiSei && !bInFirma && !bFinale && !bInUscita; }))
							]
							+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								DadiPagina
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								Prossimo
							]
						]

						// Tutto ai dadi, Torna ai personaggi.
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 12.f, 0.f, 0.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[
								Collegamento(LOCTEXT("Torna", "Torna ai personaggi"), FSimpleDelegate::CreateSP(this, &SValdorsoRegistroColono::Torna))
							]
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNullWidget::NullWidget
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								Collegamento(LOCTEXT("DadiTutto", "Lascia decidere ai dadi tutto il resto"), FSimpleDelegate::CreateSP(this, &SValdorsoRegistroColono::TiraDadiTutto))
							]
						]
					]
				]
			]

			// A destra, per ora, il frammento che batte (il palco arriva al passo 4.2b).
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					Stile->Diamante(64.f, TAttribute<FSlateColor>::CreateLambda([]()
					{
						const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
						return FSlateColor(FMath::Lerp(ValdorsoTema::Brace().CopyWithNewOpacity(0.55f), ValdorsoTema::OroChiaro(), Colpo));
					}))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 46.f, 0.f, 0.f))
				[
					Stile->Etichetta(LOCTEXT("Cuore", "IL CUORE BATTE ANCORA"))
				]
			]
		]

		// Dopo la firma: il nero e la frase.
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(&Stile->Pieno)
			.Visibility_Lambda([this]() { return bFinale ? EVisibility::Visible : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this]()
			{
				const float Passati = static_cast<float>(FPlatformTime::Seconds() - InizioFinale);
				return FSlateColor(FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(Passati / 1.2f, 0.f, 1.f)));
			})
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(80.f, 0.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Finale", "Il frammento batte più forte, per un istante. La valle ti ha sentito."))
			.Font(Stile->Caratteri->Testo(30.f, TEXT("Italic")))
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
			.Visibility_Lambda([this]() { return bFinale ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this]()
			{
				const float Passati = static_cast<float>(FPlatformTime::Seconds() - InizioFinale);
				const float Luce = FMath::Clamp((Passati - 1.0f) / 1.0f, 0.f, 1.f);
				const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
				return FSlateColor(FMath::Lerp(ValdorsoTema::Pergamena(), ValdorsoTema::OroChiaro(), Colpo * 0.6f).CopyWithNewOpacity(Luce));
			})
		]
	];

	// Si riparte dalla prima domanda senza risposta (o dal racconto, se ci sono tutte).
	VaiA(RegSchermoPaginaDellaDomanda(ValdorsoRegistro::PrimaMancante(Bozza)));
}

// ------------------------------------------------------------------------------------------------
// Pagine e navigazione
// ------------------------------------------------------------------------------------------------

void SValdorsoRegistroColono::VaiA(EPagina Nuova)
{
	Pagina = Nuova;
	Messaggio = FText::GetEmpty();
	bErrore = false;
	if (Pagina == EPagina::Racconto && (!bRaccontoToccato || Bozza.Racconto.IsEmpty()))
	{
		// Il sacerdote riscrive il racconto con le risposte di adesso (finché il giocatore non lo ritocca).
		Bozza.Racconto = ValdorsoRegistro::ComponiRacconto(Bozza, Nome);
		bRaccontoToccato = false;
	}
	Ridisegna();
}

void SValdorsoRegistroColono::Avanti()
{
	if (bInFirma || bFinale || bInUscita || Pagina >= EPagina::Firma || !PaginaCompleta(Pagina))
	{
		return;
	}
	const FText Difetto = SalvaBozza();
	VaiA(static_cast<EPagina>(static_cast<int32>(Pagina) + 1));
	if (!Difetto.IsEmpty())
	{
		MostraMessaggio(Difetto, true);
	}
}

void SValdorsoRegistroColono::Indietro()
{
	if (bInFirma || bFinale || bInUscita || Pagina == EPagina::ChiSei)
	{
		return;
	}
	const FText Difetto = SalvaBozza();
	VaiA(static_cast<EPagina>(static_cast<int32>(Pagina) - 1));
	if (!Difetto.IsEmpty())
	{
		MostraMessaggio(Difetto, true);
	}
}

void SValdorsoRegistroColono::Torna()
{
	if (bInFirma || bFinale || bInUscita)
	{
		return;
	}
	const FText Difetto = SalvaBozza();
	if (!Difetto.IsEmpty())
	{
		// Prima di uscire si sistema il testo, altrimenti le ultime risposte andrebbero perse.
		MostraMessaggio(Difetto, true);
		return;
	}
	// Una volta sola: lo schermo resta spento finché il server non rimanda l'elenco.
	bInUscita = true;
	MostraMessaggio(LOCTEXT("Uscendo", "Il sacerdote chiude il registro..."), false);
	OnTorna.ExecuteIfBound();
}

void SValdorsoRegistroColono::Ridisegna()
{
	if (!Corpo.IsValid())
	{
		return;
	}
	PrimoFuoco.Reset();
	CampoTesto.Reset();
	Corpo->SetContent(ContenutoPagina());

	TSharedPtr<SWidget> Fuoco = FuocoIniziale();
	if (Fuoco.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocus(Fuoco, EFocusCause::SetDirectly);
	}
}

TSharedPtr<SWidget> SValdorsoRegistroColono::FuocoIniziale() const
{
	if (PrimoFuoco.IsValid())
	{
		return PrimoFuoco;
	}
	return PulsanteAvanti;
}

bool SValdorsoRegistroColono::PaginaCompleta(EPagina Quale) const
{
	switch (Quale)
	{
	case EPagina::ChiSei:
		return !Bozza.Sesso.IsEmpty() && Bozza.Eta >= ValdorsoRegistro::EtaMinima && Bozza.Eta <= ValdorsoRegistro::EtaMassima;
	case EPagina::Carattere:
		for (const ValdorsoRegistro::EDomanda Coppia : RegSchermoCarattere)
		{
			if (ValdorsoRegistro::Risposta(Bozza, Coppia).IsEmpty())
			{
				return false;
			}
		}
		return true;
	case EPagina::Racconto:
		return !Bozza.Racconto.TrimStartAndEnd().IsEmpty()
			&& ValdorsoRegole::ProblemaTestoLibero(Bozza.Racconto, ValdorsoRegistro::MassimoRacconto).IsEmpty();
	case EPagina::Storia:
		return ValdorsoRegole::ProblemaTestoLibero(Bozza.Storia, ValdorsoRegistro::MassimoStoria).IsEmpty();
	case EPagina::Firma:
	case EPagina::Numero:
		return false;
	default:
		return !ValdorsoRegistro::Risposta(Bozza, RegSchermoDomandaDellaPagina(Quale)).IsEmpty();
	}
}

FValdorsoRegistro SValdorsoRegistroColono::PerLaFirma() const
{
	FValdorsoRegistro Pronto = Bozza;
	Pronto.Racconto = Pronto.Racconto.TrimStartAndEnd();
	Pronto.Storia = Pronto.Storia.TrimStartAndEnd();
	if (Pronto.Racconto.IsEmpty())
	{
		Pronto.Racconto = ValdorsoRegistro::ComponiRacconto(Pronto, Nome);
	}
	return Pronto;
}

FText SValdorsoRegistroColono::FrasePagina(EPagina Quale) const
{
	switch (Quale)
	{
	case EPagina::ChiSei: return LOCTEXT("FraseChiSei", "Per ultimi vennero gli uomini, fragili e brevi, ma gli unici capaci di scegliere chi diventare.");
	case EPagina::Fede: return LOCTEXT("FraseFede", "Nessun colono pronuncia davanti al sacerdote i nomi che non si pronunciano.");
	case EPagina::Origine: return LOCTEXT("FraseOrigine", "La Corona ha chiamato coloni da ogni angolo di Aurelia: la valle dell'Orso è di nuovo aperta.");
	case EPagina::Mestiere: return LOCTEXT("FraseMestiere", "Nella valle nessuno è più quello che era. Ma le mani ricordano.");
	case EPagina::Motivo: return LOCTEXT("FraseMotivo", "Per secoli nessuno è salito fin quassù. Chi arriva ora, arriva per una ragione.");
	case EPagina::Ricordo: return LOCTEXT("FraseRicordo", "Nella valle si arriva con poco: quel poco deve valere molto.");
	case EPagina::Paura: return LOCTEXT("FrasePaura", "Il sacerdote ha visto tremare uomini più grandi di te. Non ride di nessuna paura.");
	case EPagina::Carattere: return LOCTEXT("FraseCarattere", "Il registro non giudica: annota. Un giorno le tue azioni potranno correggere queste righe.");
	case EPagina::Richiamo: return LOCTEXT("FraseRichiamo", "Quando il Cuore batte, certe fiamme si piegano verso qualcuno. Non verso tutti.");
	case EPagina::Racconto: return LOCTEXT("FraseRacconto", "Il sacerdote intinge la penna e scrive, lento, il tuo arrivo.");
	case EPagina::Storia: return LOCTEXT("FraseStoria", "Quello che il sacerdote non sa, puoi scriverlo tu.");
	case EPagina::Firma: return LOCTEXT("FraseFirma", "Una firma, e la valle saprà il tuo nome.");
	default: return FText::GetEmpty();
	}
}

FText SValdorsoRegistroColono::TitoloPagina(EPagina Quale) const
{
	switch (Quale)
	{
	case EPagina::ChiSei: return LOCTEXT("TitoloChiSei", "Chi sei?");
	case EPagina::Carattere: return LOCTEXT("TitoloCarattere", "Com'è il tuo carattere?");
	case EPagina::Racconto: return LOCTEXT("TitoloRacconto", "Il racconto del sacerdote");
	case EPagina::Storia: return LOCTEXT("TitoloStoria", "La tua storia");
	case EPagina::Firma: return LOCTEXT("TitoloFirma", "Firma il registro");
	default: return FText::FromString(ValdorsoRegistro::Domanda(RegSchermoDomandaDellaPagina(Quale), Bozza.Sesso));
	}
}

TSharedRef<SWidget> SValdorsoRegistroColono::ContenutoPagina()
{
	switch (Pagina)
	{
	case EPagina::ChiSei: return PaginaChiSei();
	case EPagina::Carattere: return PaginaCarattere();
	case EPagina::Racconto: return PaginaTesto(true);
	case EPagina::Storia: return PaginaTesto(false);
	case EPagina::Firma: return PaginaFirma();
	default: return Scelte(RegSchermoDomandaDellaPagina(Pagina), false);
	}
}

// ------------------------------------------------------------------------------------------------
// I pezzi delle pagine
// ------------------------------------------------------------------------------------------------

TSharedRef<SButton> SValdorsoRegistroColono::PulsanteVoce(const FText& Scritta, TFunction<bool()> Scelta, TFunction<void()> Azione, float Dimensione)
{
	// La risposta scelta ha il bordo d'oro pieno: a ogni scelta la pagina si ridisegna (e il fuoco torna sulla scelta).
	return SNew(SButton)
		.ButtonStyle(Scelta() ? &StileVoceScelta : &StileVoce)
		.IsFocusable(true)
		.HAlign(HAlign_Left)
		.IsEnabled_Lambda([this]() { return !bInFirma && !bFinale && !bInUscita; })
		.OnClicked_Lambda([this, Azione]()
		{
			if (!bInFirma && !bFinale && !bInUscita)
			{
				Azione();
				Messaggio = FText::GetEmpty();
				bErrore = false;
				Ridisegna();
			}
			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(0.f, 0.f, 12.f, 0.f))
			[
				Stile->Diamante(8.f, TAttribute<FSlateColor>::CreateLambda([Scelta]()
				{
					return FSlateColor(Scelta() ? ValdorsoTema::OroChiaro() : ValdorsoTema::Oro().CopyWithNewOpacity(0.18f));
				}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Scritta)
				.Font(Stile->Caratteri->Testo(Dimensione))
				.AutoWrapText(true)
				.ColorAndOpacity_Lambda([Scelta]()
				{
					return FSlateColor(Scelta() ? ValdorsoTema::OroChiaro() : ValdorsoTema::Pergamena());
				})
			]
		];
}

TSharedRef<SWidget> SValdorsoRegistroColono::Scelte(ValdorsoRegistro::EDomanda Quale, bool bCompatte)
{
	const TArrayView<const FValdorsoVoceRegistro> Possibili = ValdorsoRegistro::Voci(Quale);
	const bool bDueColonne = bCompatte || Possibili.Num() > RegSchermoMassimoUnaColonna;
	TSharedRef<SUniformGridPanel> Griglia = SNew(SUniformGridPanel).SlotPadding(FMargin(4.f));
	TSharedRef<SVerticalBox> Colonna = SNew(SVerticalBox);

	for (int32 i = 0; i < Possibili.Num(); ++i)
	{
		const FString Chiave = Possibili[i].Chiave;
		const FText Scritta = FText::FromString(ValdorsoRegistro::Testo(Quale, Chiave, Bozza.Sesso));
		TSharedRef<SButton> Voce = PulsanteVoce(Scritta,
			[this, Quale, Chiave]() { return ValdorsoRegistro::Risposta(Bozza, Quale) == Chiave; },
			[this, Quale, Chiave]() { ValdorsoRegistro::Risposta(Bozza, Quale) = Chiave; },
			bCompatte ? 20.f : 21.f);

		// Il fuoco va sulla risposta scelta, o sulla prima.
		if (ValdorsoRegistro::Risposta(Bozza, Quale) == Chiave || (!PrimoFuoco.IsValid() && i == 0))
		{
			PrimoFuoco = Voce;
		}

		if (bDueColonne)
		{
			Griglia->AddSlot(i % 2, i / 2) [ Voce ];
		}
		else
		{
			Colonna->AddSlot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 8.f)) [ Voce ];
		}
	}
	if (bDueColonne)
	{
		return Griglia;
	}
	return Colonna;
}

TSharedRef<SWidget> SValdorsoRegistroColono::PaginaChiSei()
{
	TSharedRef<SWidget> Sesso = Scelte(ValdorsoRegistro::EDomanda::Sesso, true);
	auto PulsanteEta = [this](const TCHAR* Scritta, int32 Di)
	{
		return Stile->Pulsante(FText::FromString(Scritta),
			FSimpleDelegate::CreateLambda([this, Di]() { CambiaEta(Di); }), 16.f,
			TAttribute<bool>::CreateLambda([this, Di]()
			{
				const bool bSiPuo = Di < 0 ? Bozza.Eta > ValdorsoRegistro::EtaMinima : Bozza.Eta < ValdorsoRegistro::EtaMassima;
				return bSiPuo && !bInFirma && !bFinale && !bInUscita;
			}));
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Sesso
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 22.f, 0.f, 8.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("QuantiAnni", "Quanti anni hai?"))
			.Font(Stile->Caratteri->Testo(22.f, TEXT("SemiBold")))
			.ColorAndOpacity(ValdorsoTema::Pergamena())
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(0.f, 0.f, 6.f, 0.f)) [ PulsanteEta(TEXT("-5"), -5) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(0.f, 0.f, 6.f, 0.f)) [ PulsanteEta(TEXT("-1"), -1) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::Format(LOCTEXT("Anni", "{0} anni"), FText::AsNumber(Bozza.Eta)); })
				.Font(Stile->Caratteri->Titolo(26.f, TEXT("Bold")))
				.ColorAndOpacity(ValdorsoTema::OroChiaro())
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(6.f, 0.f, 0.f, 0.f)) [ PulsanteEta(TEXT("+1"), 1) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(6.f, 0.f, 0.f, 0.f)) [ PulsanteEta(TEXT("+5"), 5) ]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("SpiegaEta", "Da {0} a {1} anni. La corona accoglie solo coloni adulti."),
				FText::AsNumber(ValdorsoRegistro::EtaMinima), FText::AsNumber(ValdorsoRegistro::EtaMassima)))
			.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::TestoSecondario())
		];
}

void SValdorsoRegistroColono::CambiaEta(int32 Di)
{
	Bozza.Eta = FMath::Clamp(Bozza.Eta + Di, ValdorsoRegistro::EtaMinima, ValdorsoRegistro::EtaMassima);
}

TSharedRef<SWidget> SValdorsoRegistroColono::PaginaCarattere()
{
	TSharedRef<SVerticalBox> Coppie = SNew(SVerticalBox);
	TSharedPtr<SWidget> FuocoPrimaCoppia;
	TSharedPtr<SWidget> FuocoMancante;
	for (const ValdorsoRegistro::EDomanda Coppia : RegSchermoCarattere)
	{
		Coppie->AddSlot().AutoHeight().Padding(FMargin(0.f, 6.f, 0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(ValdorsoRegistro::Domanda(Coppia, Bozza.Sesso)))
			.Font(Stile->Caratteri->Testo(19.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::TestoSecondario())
		];
		PrimoFuoco.Reset();
		Coppie->AddSlot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 6.f)) [ Scelte(Coppia, true) ];
		if (!FuocoPrimaCoppia.IsValid())
		{
			FuocoPrimaCoppia = PrimoFuoco;
		}
		if (!FuocoMancante.IsValid() && ValdorsoRegistro::Risposta(Bozza, Coppia).IsEmpty())
		{
			FuocoMancante = PrimoFuoco;
		}
	}
	// Il fuoco va sulla prima coppia ancora senza risposta (o sulla prima, se ci sono tutte).
	PrimoFuoco = FuocoMancante.IsValid() ? FuocoMancante : FuocoPrimaCoppia;
	return Coppie;
}

TSharedRef<SWidget> SValdorsoRegistroColono::PaginaTesto(bool bRacconto)
{
	const int32 Massimo = bRacconto ? ValdorsoRegistro::MassimoRacconto : ValdorsoRegistro::MassimoStoria;
	const FText Spiegazione = bRacconto
		? LOCTEXT("SpiegaRacconto", "Il sacerdote ha scritto così il tuo arrivo, con le tue risposte. Puoi leggerlo e ritoccarlo con parole tue: è quello che resterà nel registro.")
		: LOCTEXT("SpiegaStoria", "Facoltativa. Scrivi con parole tue chi eri prima di arrivare nella valle, in gioco di ruolo e coerente con il mondo di Valdorso: niente nomi o fatti del mondo reale, niente indirizzi web. Lo staff può chiederti di correggerla. Si legge esaminando il personaggio da vicino.");

	TSharedRef<SMultiLineEditableTextBox> Campo = SNew(SMultiLineEditableTextBox)
		.Style(&Stile->StileCampo)
		.Font(Stile->Caratteri->Testo(20.f))
		.ForegroundColor(FSlateColor(ValdorsoTema::Pergamena()))
		.AutoWrapText(true)
		.Text(FText::FromString(bRacconto ? Bozza.Racconto : Bozza.Storia))
		.HintText(bRacconto ? FText::GetEmpty() : LOCTEXT("SugStoria", "Mio padre forgiava spade a Torre Grigia; io ho imparato a tacere..."))
		.IsEnabled_Lambda([this]() { return !bInFirma && !bFinale && !bInUscita; })
		.OnTextChanged_Lambda([this, bRacconto](const FText& Nuovo)
		{
			if (bRacconto)
			{
				Bozza.Racconto = Nuovo.ToString();
				bRaccontoToccato = true;
			}
			else
			{
				Bozza.Storia = Nuovo.ToString();
			}
		});
	CampoTesto = Campo;
	PrimoFuoco = Campo;

	TSharedRef<SHorizontalBox> Sotto = SNew(SHorizontalBox);
	if (bRacconto)
	{
		Sotto->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			Stile->Pulsante(LOCTEXT("Riscrivi", "Riscrivi dalle risposte"), FSimpleDelegate::CreateLambda([this]()
			{
				Bozza.Racconto = ValdorsoRegistro::ComponiRacconto(Bozza, Nome);
				if (CampoTesto.IsValid())
				{
					CampoTesto->SetText(FText::FromString(Bozza.Racconto));
				}
				bRaccontoToccato = false;
			}), 15.f, TAttribute<bool>::CreateLambda([this]() { return bRaccontoToccato && !bInFirma && !bFinale && !bInUscita; }))
		];
	}
	Sotto->AddSlot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text_Lambda([this, bRacconto, Massimo]()
		{
			const FString& Scritto = bRacconto ? Bozza.Racconto : Bozza.Storia;
			const FString Difetto = ValdorsoRegole::ProblemaTestoLibero(Scritto, Massimo);
			if (!Difetto.IsEmpty())
			{
				return FText::FromString(Difetto);
			}
			if (bRacconto && Scritto.TrimStartAndEnd().IsEmpty())
			{
				return LOCTEXT("RaccontoVuoto", "Il racconto non può restare vuoto: riscrivilo dalle risposte.");
			}
			return FText::Format(LOCTEXT("Conta", "{0} / {1}"), FText::AsNumber(Scritto.Len()), FText::AsNumber(Massimo));
		})
		.Font(Stile->Caratteri->Testo(16.f, TEXT("Italic")))
		.AutoWrapText(true)
		.Justification(ETextJustify::Right)
		.ColorAndOpacity_Lambda([this, bRacconto]()
		{
			return FSlateColor(PaginaCompleta(bRacconto ? EPagina::Racconto : EPagina::Storia) ? ValdorsoTema::TestoSecondario() : ValdorsoTema::Brace());
		})
	];

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 10.f))
		[
			SNew(STextBlock)
			.Text(Spiegazione)
			.Font(Stile->Caratteri->Testo(18.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::TestoSecondario())
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(bRacconto ? 290.f : 270.f)
			[
				Campo
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
		[
			Sotto
		];
}

TSharedRef<SWidget> SValdorsoRegistroColono::PaginaFirma()
{
	TSharedRef<SVerticalBox> Righe = SNew(SVerticalBox);

	// Una riga per pagina: si clicca per tornare a cambiarla.
	auto Riga = [this, &Righe](const FText& Etichetta, const FString& Valore, EPagina Dove)
	{
		const bool bManca = Valore.IsEmpty();
		Righe->AddSlot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 4.f))
		[
			SNew(SButton)
			.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
			.IsEnabled_Lambda([this]() { return !bInFirma && !bFinale && !bInUscita; })
			.OnClicked_Lambda([this, Dove]() { VaiA(Dove); return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					SNew(SBox).WidthOverride(190.f)
					[
						SNew(STextBlock).Text(Etichetta)
						.Font(Stile->Caratteri->Testo(18.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::TestoSecondario())
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Text(bManca ? LOCTEXT("Manca", "manca: tocca qui per rispondere") : FText::FromString(Valore))
					.Font(Stile->Caratteri->Testo(18.f))
					.AutoWrapText(true)
					.ColorAndOpacity(bManca ? ValdorsoTema::Brace() : ValdorsoTema::Pergamena())
				]
			]
		];
	};

	auto Scelta = [this](ValdorsoRegistro::EDomanda Quale)
	{
		return ValdorsoRegistro::Testo(Quale, ValdorsoRegistro::Risposta(Bozza, Quale), Bozza.Sesso);
	};

	Riga(LOCTEXT("RigaChi", "Chi sei"), Bozza.Sesso.IsEmpty() ? FString()
		: FString::Printf(TEXT("%s, %d anni"), *Scelta(ValdorsoRegistro::EDomanda::Sesso), Bozza.Eta), EPagina::ChiSei);
	Riga(LOCTEXT("RigaFede", "La fede"), Scelta(ValdorsoRegistro::EDomanda::Fede), EPagina::Fede);
	Riga(LOCTEXT("RigaOrigine", "Da dove vieni"), Scelta(ValdorsoRegistro::EDomanda::Origine), EPagina::Origine);
	Riga(LOCTEXT("RigaMestiere", "Cosa facevi"), Scelta(ValdorsoRegistro::EDomanda::Mestiere), EPagina::Mestiere);
	Riga(LOCTEXT("RigaMotivo", "Perché sei qui"), Scelta(ValdorsoRegistro::EDomanda::Motivo), EPagina::Motivo);
	Riga(LOCTEXT("RigaRicordo", "Cosa porti"), Scelta(ValdorsoRegistro::EDomanda::Ricordo), EPagina::Ricordo);
	Riga(LOCTEXT("RigaPaura", "Cosa temi"), Scelta(ValdorsoRegistro::EDomanda::Paura), EPagina::Paura);

	TArray<FString> Tratti;
	for (const ValdorsoRegistro::EDomanda Coppia : RegSchermoCarattere)
	{
		const FString Tratto = Scelta(Coppia);
		if (!Tratto.IsEmpty())
		{
			Tratti.Add(Tratto);
		}
	}
	Riga(LOCTEXT("RigaCarattere", "Il carattere"), Tratti.Num() == static_cast<int32>(UE_ARRAY_COUNT(RegSchermoCarattere)) ? FString::Join(Tratti, TEXT(", ")) : FString(), EPagina::Carattere);
	Riga(LOCTEXT("RigaRichiamo", "Il richiamo"), Scelta(ValdorsoRegistro::EDomanda::RichiamoMagia), EPagina::Richiamo);
	Riga(LOCTEXT("RigaStoria", "La tua storia"), Bozza.Storia.TrimStartAndEnd().IsEmpty()
		? FString(TEXT("non scritta (facoltativa)")) : FString::Printf(TEXT("scritta, %d caratteri"), Bozza.Storia.Len()), EPagina::Storia);

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Righe
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 14.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				const FString Difetto = ValdorsoRegistro::Problema(PerLaFirma(), true);
				if (!Difetto.IsEmpty())
				{
					return FText::FromString(Difetto);
				}
				return LOCTEXT("SpiegaFirma", "Con la firma le risposte non cambiano più: da qui in avanti cambierai vivendo.");
			})
			.Font(Stile->Caratteri->Testo(19.f, TEXT("Italic")))
			.AutoWrapText(true)
			.ColorAndOpacity_Lambda([this]()
			{
				return FSlateColor(ValdorsoRegistro::Problema(PerLaFirma(), true).IsEmpty() ? ValdorsoTema::Pergamena() : ValdorsoTema::Brace());
			})
		];
}

// ------------------------------------------------------------------------------------------------
// Dadi, bozza, firma
// ------------------------------------------------------------------------------------------------

void SValdorsoRegistroColono::TiraDadiPagina()
{
	// Racconto, storia e firma non hanno dadi.
	if (bInFirma || bFinale || bInUscita || Pagina > EPagina::Richiamo)
	{
		return;
	}
	switch (Pagina)
	{
	case EPagina::ChiSei:
		ValdorsoRegistro::CasualeUna(Bozza, ValdorsoRegistro::EDomanda::Sesso, Dadi);
		Bozza.Eta = Dadi.RandRange(ValdorsoRegistro::EtaMinima, 40);
		break;
	case EPagina::Carattere:
		for (const ValdorsoRegistro::EDomanda Coppia : RegSchermoCarattere)
		{
			ValdorsoRegistro::CasualeUna(Bozza, Coppia, Dadi);
		}
		break;
	default:
		if (RegSchermoDomandaDellaPagina(Pagina) != ValdorsoRegistro::EDomanda::Numero)
		{
			ValdorsoRegistro::CasualeUna(Bozza, RegSchermoDomandaDellaPagina(Pagina), Dadi);
		}
		break;
	}
	Ridisegna();
	MostraMessaggio(LOCTEXT("DadiFatto", "I dadi hanno deciso. Puoi cambiare la risposta, o tirarli di nuovo."), false);
}

void SValdorsoRegistroColono::TiraDadiTutto()
{
	if (bInFirma || bFinale || bInUscita)
	{
		return;
	}
	// Solo le risposte che mancano: quelle già scelte restano.
	ValdorsoRegistro::Casuale(Bozza, Dadi, false);
	const FText Difetto = SalvaBozza();
	VaiA(EPagina::Racconto);
	MostraMessaggio(Difetto.IsEmpty()
		? LOCTEXT("DadiTuttoFatto", "I dadi hanno deciso il resto. Leggi cosa ha scritto il sacerdote: puoi sempre tornare indietro.")
		: Difetto, !Difetto.IsEmpty());
}

FText SValdorsoRegistroColono::SalvaBozza()
{
	if (bInFirma || bFinale || bInUscita)
	{
		return FText::GetEmpty();
	}
	// Una bozza con un testo non accettabile non parte (il server la rifiuterebbe): si restituisce il motivo,
	// che chi chiama mostra dopo aver cambiato pagina.
	const FString Difetto = ValdorsoRegistro::Problema(Bozza, false);
	if (!Difetto.IsEmpty())
	{
		return FText::Format(LOCTEXT("BozzaNonSalvata", "Le risposte non sono state salvate: {0}"), FText::FromString(Difetto));
	}
	if (bInviataValida && ValdorsoRegistro::StesseRisposte(Bozza, Inviata))
	{
		return FText::GetEmpty();
	}
	Inviata = Bozza;
	bInviataValida = true;
	OnSalva.ExecuteIfBound(Bozza, false);
	return FText::GetEmpty();
}

void SValdorsoRegistroColono::Firma()
{
	if (bInFirma || bFinale || bInUscita)
	{
		return;
	}
	const FValdorsoRegistro Pronto = PerLaFirma();
	const FString Difetto = ValdorsoRegistro::Problema(Pronto, true);
	if (!Difetto.IsEmpty())
	{
		const ValdorsoRegistro::EDomanda Manca = ValdorsoRegistro::PrimaMancante(Pronto);
		if (Manca != ValdorsoRegistro::EDomanda::Numero)
		{
			VaiA(RegSchermoPaginaDellaDomanda(Manca));
		}
		else
		{
			// Non manca niente: è un testo da sistemare (il racconto o la storia).
			const bool bRacconto = !ValdorsoRegole::ProblemaTestoLibero(Pronto.Racconto, ValdorsoRegistro::MassimoRacconto).IsEmpty();
			VaiA(bRacconto ? EPagina::Racconto : EPagina::Storia);
		}
		MostraMessaggio(FText::FromString(Difetto), true);
		return;
	}
	Bozza = Pronto;
	bInFirma = true;
	MostraMessaggio(LOCTEXT("Firmando", "Il sacerdote firma il registro..."), false);
	OnSalva.ExecuteIfBound(Pronto, true);
}

void SValdorsoRegistroColono::Esito(const FString& Avviso, bool bRiuscito, const FValdorsoRegistro& Salvato)
{
	if (bFinale)
	{
		return;
	}
	if (Salvato.bFirmato)
	{
		// Firmato adesso (o già firmato prima, da un'altra finestra): il personaggio può entrare.
		AvviaFinale();
		return;
	}
	if (bInFirma && !bRiuscito)
	{
		// (Una bozza partita prima della firma risponde "riuscito" senza firma: non è un errore, si aspetta la firma.)
		bInFirma = false;
		MostraMessaggio(Avviso.IsEmpty() ? LOCTEXT("NonFirmato", "Il registro non è stato firmato: riprova.") : FText::FromString(Avviso), true);
		return;
	}
	if (!bRiuscito && !bInFirma)
	{
		// La bozza non è stata salvata: alla prossima pagina si riprova.
		bInviataValida = false;
		MostraMessaggio(FText::FromString(Avviso), true);
	}
}

void SValdorsoRegistroColono::AvviaFinale()
{
	bInFirma = false;
	bFinale = true;
	InizioFinale = FPlatformTime::Seconds();
	Messaggio = FText::GetEmpty();
	RegisterActiveTimer(RegSchermoSecondiFinale, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoRegistroColono::FineFinale));
}

EActiveTimerReturnType SValdorsoRegistroColono::FineFinale(double Ora, float Delta)
{
	OnFirmato.ExecuteIfBound();
	return EActiveTimerReturnType::Stop;
}

void SValdorsoRegistroColono::MostraMessaggio(const FText& Scritta, bool bComeErrore)
{
	Messaggio = Scritta;
	bErrore = bComeErrore;
}

FReply SValdorsoRegistroColono::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Un clic sullo sfondo non deve togliere il fuoco al registro (Esc e controller smetterebbero di funzionare).
	const TSharedPtr<SWidget> Fuoco = FuocoIniziale();
	if (Fuoco.IsValid())
	{
		return FReply::Handled().SetUserFocus(Fuoco.ToSharedRef(), EFocusCause::Mouse);
	}
	return FReply::Handled();
}

FReply SValdorsoRegistroColono::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (bFinale || bInFirma || bInUscita)
	{
		return FReply::Handled();
	}
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		if (Pagina == EPagina::ChiSei)
		{
			Torna();
		}
		else
		{
			Indietro();
		}
		return FReply::Handled();
	}
	if (Tasto == EKeys::PageDown || Tasto == EKeys::Gamepad_RightShoulder)
	{
		Avanti();
		return FReply::Handled();
	}
	if (Tasto == EKeys::PageUp || Tasto == EKeys::Gamepad_LeftShoulder)
	{
		Indietro();
		return FReply::Handled();
	}
	if (Tasto == EKeys::Gamepad_FaceButton_Top)
	{
		TiraDadiPagina();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
