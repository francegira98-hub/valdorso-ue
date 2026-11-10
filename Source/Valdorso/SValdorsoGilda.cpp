// Valdorso - La schermata della Gilda (vedi SValdorsoGilda.h). Scritto da Claude il 10/11/2026.

#include "SValdorsoGilda.h"
#include "SValdorsoZaino.h"
#include "ValdorsoGilda.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoTemaUI.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoGildaSchermo"

namespace
{
	FLinearColor GildaColoreSigillo(EValdorsoGrado Grado)
	{
		switch (Grado)
		{
		case EValdorsoGrado::Bronzo: return ValdorsoTema::Hex(TEXT("A8642A"));
		case EValdorsoGrado::Argento: return ValdorsoTema::Hex(TEXT("9EA3AA"));
		case EValdorsoGrado::Oro: return ValdorsoTema::Hex(TEXT("D4AF37"));
		default: return ValdorsoTema::Hex(TEXT("8E2B25"));
		}
	}

	const TCHAR* GildaLetteraSigillo(EValdorsoGrado Grado)
	{
		switch (Grado)
		{
		case EValdorsoGrado::Bronzo: return TEXT("B");
		case EValdorsoGrado::Argento: return TEXT("A");
		case EValdorsoGrado::Oro: return TEXT("O");
		default: return TEXT("C");
		}
	}
}

void SValdorsoGilda::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	bBancone = InArgs._Bancone;
	OnChiudi = InArgs._OnChiudi;
	Gilda = UValdorsoGildaComponent::Di(Controllore.Get());
	Zaino = UValdorsoZainoComponent::Di(Controllore.Get());
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 4.f, ValdorsoTema::OroScuro(), 2.f);
	Velo = FSlateColorBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.45f));
	Riquadro = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("EFE3C4")), 2.f, ValdorsoTema::Hex(TEXT("B49A6A")), 1.f);
	RiquadroGiallo = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("E8CF7A")), 2.f, ValdorsoTema::Hex(TEXT("A07A2A")), 1.f);
	RiquadroScelto = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("F6EDD2")), 2.f, ValdorsoTema::OroScuro(), 3.f);
	Assi = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("6B4A2B")), 3.f, ValdorsoTema::Hex(TEXT("3E2914")), 2.f);
	for (int32 g = 0; g < 4; ++g)
	{
		const FLinearColor Colore = GildaColoreSigillo(static_cast<EValdorsoGrado>(g));
		Sigilli[g] = FSlateRoundedBoxBrush(Colore, 20.f, (Colore * 0.6f).CopyWithNewOpacity(1.f), 2.f);
	}
	if (const APlayerController* Chi = Controllore.Get(); Chi && Chi->GetPawn())
	{
		Partenza = Chi->GetPawn()->GetActorLocation();
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(&Velo)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Carta)
			.Padding(FMargin(28.f, 20.f))
			[
				SNew(SBox)
				.WidthOverride(bBancone ? 760.f : 980.f)
				.HeightOverride(620.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Bottom)
						[
							SNew(STextBlock)
							.Text(bBancone ? LOCTEXT("TitoloBancone", "Il bancone di Lucia") : LOCTEXT("TitoloBacheca", "La bacheca della Gilda"))
							.Font(Caratteri->Titolo(26.f, TEXT("Bold")))
							.ColorAndOpacity(ValdorsoTema::Inchiostro())
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Bottom)
						[
							SNew(STextBlock)
							.Text(this, &SValdorsoGilda::TestoGrado)
							.Font(Caratteri->Titolo(15.f, TEXT("Bold")))
							.ColorAndOpacity(ValdorsoTema::OroScuro())
						]
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					[
						SAssignNew(Contenuto, SBox)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 10.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(bBancone
							? LOCTEXT("AiutoBancone", "Il foglio si stacca dalla bacheca, Lucia lo assegna, a lavoro fatto lo timbra e paga. Esc o E: chiudi.")
							: LOCTEXT("AiutoBacheca", "Clic su un foglio per leggerlo. Staccato, va nello zaino: portalo a Lucia entro mezz'ora. Esc o E: chiudi."))
						.Font(Caratteri->Testo(14.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
						.AutoWrapText(true)
					]
				]
			]
		]
	];
	Rifai();
}

FText SValdorsoGilda::TestoGrado() const
{
	const UValdorsoGildaComponent* G = Gilda.Get();
	const int32 Punti = G ? G->Contenuto().Punti : 0;
	// (10/11) il grado vero (lo da' lo staff o il capo gilda); i punti dicono solo quando si e' pronti
	const EValdorsoGrado Grado = G ? G->Contenuto().Grado : EValdorsoGrado::Comune;
	if (Grado == EValdorsoGrado::Oro)
	{
		return FText::Format(LOCTEXT("GradoOro", "Grado Oro · {0} punti"), FText::AsNumber(Punti));
	}
	const EValdorsoGrado Dopo = ValdorsoGilda::GradoDopo(Grado);
	if (G && ValdorsoGilda::Pronto(G->Contenuto()))
	{
		return FText::Format(LOCTEXT("GradoPronto", "Grado {0} · {1} punti · pronto per il {2}"), ValdorsoGilda::NomeGrado(Grado), FText::AsNumber(Punti),
			ValdorsoGilda::NomeGrado(Dopo));
	}
	return FText::Format(LOCTEXT("Grado", "Grado {0} · {1} punti ({2} da {3})"), ValdorsoGilda::NomeGrado(Grado), FText::AsNumber(Punti),
		ValdorsoGilda::NomeGrado(Dopo), FText::AsNumber(ValdorsoGilda::PuntiPer(Dopo)));
}

void SValdorsoGilda::Rifai()
{
	if (!Contenuto.IsValid())
	{
		return;
	}
	const UValdorsoGildaComponent* G = Gilda.Get();
	const UValdorsoZainoComponent* Z = Zaino.Get();
	VistoGilda = G ? G->Cambiamenti() : -1;
	VistoZaino = Z ? Z->Cambiamenti() : -1;
	Contenuto->SetContent(bBancone ? Bancone() : Bacheca());
}

TSharedRef<SWidget> SValdorsoGilda::Sigillo(EValdorsoGrado Grado, float Lato)
{
	return SNew(SBox)
		.WidthOverride(Lato)
		.HeightOverride(Lato)
		[
			SNew(SBorder)
			.BorderImage(&Sigilli[FMath::Clamp(static_cast<int32>(Grado), 0, 3)])
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(GildaLetteraSigillo(Grado)))
				.Font(Caratteri->Titolo(Lato * 0.5f, TEXT("Bold")))
				.ColorAndOpacity(ValdorsoTema::Pergamena())
			]
		];
}

// --- La bacheca ------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoGilda::Bacheca()
{
	const UValdorsoGildaComponent* G = Gilda.Get();
	if (!G)
	{
		return SNullWidget::NullWidget;
	}
	const int64 Adesso = G->OraServer();
	const TArray<FValdorsoIncarico>& Fogli = G->FogliVisti();
	if (!Fogli.ContainsByPredicate([this](const FValdorsoIncarico& I) { return I.Numero == Scelto; }))
	{
		Scelto = Fogli.IsEmpty() ? 0 : Fogli[0].Numero;
	}
	TSharedRef<SWrapBox> Parete = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10.f, 10.f));
	for (const FValdorsoIncarico& I : Fogli)
	{
		Parete->AddSlot()[Foglio(I, Adesso)];
	}
	TSharedRef<SWidget> Sinistra = Fogli.IsEmpty()
		? StaticCastSharedRef<SWidget>(SNew(STextBlock)
			.Text(LOCTEXT("Vuota", "La bacheca e' vuota: oggi nessuno cerca braccia. Torna piu' tardi."))
			.Font(Caratteri->Testo(16.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::Pergamena())
			.AutoWrapText(true))
		: StaticCastSharedRef<SWidget>(SNew(SScrollBox) + SScrollBox::Slot()[Parete]);

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			SNew(SBorder)
			.BorderImage(&Assi)
			.Padding(FMargin(12.f))
			[
				Sinistra
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(14.f, 0.f, 0.f, 0.f))
		[
			SNew(SBox)
			.WidthOverride(330.f)
			[
				Dettaglio(Adesso)
			]
		];
}

TSharedRef<SWidget> SValdorsoGilda::Foglio(const FValdorsoIncarico& I, int64 Adesso)
{
	const bool bGiallo = I.Scadenza - Adesso < ValdorsoGilda::SecondiGiallo;
	const bool bScelto = I.Numero == Scelto;
	const int32 Numero = I.Numero;
	TWeakPtr<SValdorsoGilda> Debole = SharedThis(this);
	return SNew(SBox)
		.WidthOverride(190.f)
		.HeightOverride(150.f)
		[
			SNew(SBorder)
			.BorderImage(bScelto ? &RiquadroScelto : (bGiallo ? &RiquadroGiallo : &Riquadro))
			.Padding(FMargin(9.f, 7.f))
			.OnMouseButtonDown_Lambda([Debole, Numero](const FGeometry&, const FPointerEvent&)
			{
				if (TSharedPtr<SValdorsoGilda> Lo = Debole.Pin())
				{
					Lo->Scelto = Numero;
					Lo->RinunciaChiesta = 0;
					Lo->bDaRifare = true;
				}
				return FReply::Handled();
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(I.Titolo))
						.Font(Caratteri->Titolo(14.f, TEXT("Bold")))
						.ColorAndOpacity(ValdorsoTema::Inchiostro())
						.AutoWrapText(true)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(6.f, 0.f, 0.f, 0.f))
					[
						Sigillo(I.Grado, 28.f)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 3.f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(I.Committente))
					.Font(Caratteri->Testo(12.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(ValdorsoGilda::TestoLavoro(I))
					.Font(Caratteri->Testo(13.f))
					.ColorAndOpacity(ValdorsoTema::Inchiostro())
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				.VAlign(VAlign_Bottom)
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("PagaBreve", "{0} Aureus · {1} su {2} · {3}"), FText::AsNumber(I.Monete), FText::AsNumber(I.Presi),
						FText::AsNumber(I.Posti), ValdorsoGilda::TestoScadenza(I.Scadenza, Adesso)))
					.Font(Caratteri->Testo(12.f, TEXT("Bold")))
					.ColorAndOpacity(bGiallo ? ValdorsoTema::Rubrica() : ValdorsoTema::OroScuro())
				]
			]
		];
}

TSharedRef<SWidget> SValdorsoGilda::Dettaglio(int64 Adesso)
{
	const UValdorsoGildaComponent* G = Gilda.Get();
	const FValdorsoIncarico* I = G ? G->FogliVisti().FindByPredicate([this](const FValdorsoIncarico& F) { return F.Numero == Scelto; }) : nullptr;
	if (!I)
	{
		return SNullWidget::NullWidget;
	}
	FText Perche = ValdorsoGilda::PercheNoStacca(*I, G->Contenuto(), Adesso);
	const UValdorsoZainoComponent* Z = Zaino.Get();
	if (Perche.IsEmpty() && Z && !Z->Contenuto().Posti.ContainsByPredicate([](const FValdorsoPostoZaino& P) { return P.Vuoto(); }))
	{
		Perche = LOCTEXT("ZainoPienoQui", "Lo zaino e' pieno: fai posto per il foglio.");
	}
	const int32 Cauzione = ValdorsoGilda::Cauzione(*I);
	TArray<FString> Righe;
	Righe.Add(FString::Printf(TEXT("Paga: %d Aureus e %d punti della Gilda."), I->Monete, I->Punti));
	Righe.Add(FString::Printf(TEXT("Posti: %d presi su %d. Scade %s."), I->Presi, I->Posti, *ValdorsoGilda::TestoScadenza(I->Scadenza, Adesso).ToString()));
	Righe.Add(FString::Printf(TEXT("Grado: %s."), *ValdorsoGilda::NomeGrado(I->Grado).ToString()));
	if (Cauzione > 0)
	{
		Righe.Add(FString::Printf(TEXT("Lucia vuole una cauzione di %d Aureus (torna alla fine)."), Cauzione));
	}
	const int32 Numero = I->Numero;
	TWeakObjectPtr<UValdorsoGildaComponent> DeboleGilda = Gilda;
	return SNew(SBorder)
		.BorderImage(&RiquadroScelto)
		.Padding(FMargin(14.f, 12.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(I->Titolo))
					.Font(Caratteri->Titolo(19.f, TEXT("Bold")))
					.ColorAndOpacity(ValdorsoTema::Inchiostro())
					.AutoWrapText(true)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(8.f, 0.f, 0.f, 0.f))
				[
					Sigillo(I->Grado, 40.f)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 4.f))
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("Chiede", "Lo chiede: {0}"), FText::FromString(I->Committente)))
				.Font(Caratteri->Testo(14.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 6.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(I->Testo))
				.Font(Caratteri->Testo(15.f))
				.ColorAndOpacity(ValdorsoTema::Inchiostro())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 4.f))
			[
				SNew(STextBlock)
				.Text(ValdorsoGilda::TestoLavoro(*I))
				.Font(Caratteri->Titolo(15.f, TEXT("Bold")))
				.ColorAndOpacity(ValdorsoTema::Inchiostro())
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Join(Righe, TEXT("\n"))))
				.Font(Caratteri->Testo(14.f))
				.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.VAlign(VAlign_Bottom)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
				[
					SNew(STextBlock)
					.Text(Perche)
					.Font(Caratteri->Testo(14.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::Rubrica())
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.IsEnabled(Perche.IsEmpty())
					.OnClicked_Lambda([DeboleGilda, Numero]()
					{
						if (UValdorsoGildaComponent* Il = DeboleGilda.Get())
						{
							Il->ServerStacca(Numero);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Stacca", "Stacca il foglio"))
						.Font(Caratteri->Titolo(15.f, TEXT("Bold")))
					]
				]
			]
		];
}

// --- Il bancone ------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoGilda::Bancone()
{
	const UValdorsoGildaComponent* G = Gilda.Get();
	if (!G)
	{
		return SNullWidget::NullWidget;
	}
	const int64 Adesso = G->OraServer();
	const TArray<FValdorsoIncaricoPreso>& Presi = G->Contenuto().Presi;
	if (Presi.IsEmpty())
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("NienteFogli", "(Lucia alza gli occhi dal registro.) Non hai fogli della Gilda. Gli incarichi sono appesi alla bacheca: scegline uno, staccalo e portamelo."))
			.Font(Caratteri->Testo(17.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::Inchiostro())
			.AutoWrapText(true);
	}
	TSharedRef<SVerticalBox> Lista = SNew(SVerticalBox);
	for (const FValdorsoIncaricoPreso& P : Presi)
	{
		Lista->AddSlot().AutoHeight().Padding(FMargin(0.f, 4.f))[RigaPreso(P, Adesso)];
	}
	return SNew(SScrollBox) + SScrollBox::Slot()[Lista];
}

TSharedRef<SWidget> SValdorsoGilda::RigaPreso(const FValdorsoIncaricoPreso& P, int64 Adesso)
{
	const UValdorsoZainoComponent* Z = Zaino.Get();
	const int32 Nello = Z ? ValdorsoZaino::Conta(Z->Contenuto(), P.Foglio.Obiettivo) : 0;
	const int32 Monete = Z ? Z->Contenuto().Monete : 0;
	const UValdorsoGildaComponent* G = Gilda.Get();
	const bool bPrenotato = P.Stato == EValdorsoStatoIncarico::Prenotato;
	FText Perche;
	FText Pulsante;
	uint8 Azione = 0;
	if (bPrenotato)
	{
		Perche = G ? ValdorsoGilda::PercheNoAssegna(G->Contenuto(), P.Foglio.Numero, Monete, Adesso) : FText::GetEmpty();
		const int32 Cauzione = ValdorsoGilda::Cauzione(P.Foglio);
		Pulsante = Cauzione > 0 ? FText::Format(LOCTEXT("AssegnaCauzione", "Fatti assegnare · {0}"), FText::AsNumber(Cauzione))
			: LOCTEXT("Assegna", "Fatti assegnare");
		Azione = 0;
	}
	else
	{
		const bool bFatto = ValdorsoGilda::Fatto(P, Nello);
		Perche = bFatto ? FText::GetEmpty() : ValdorsoGilda::TestoProgresso(P, Nello);
		Pulsante = P.Foglio.Tipo == EValdorsoTipoIncarico::Consegna ? LOCTEXT("Consegna", "Consegna") : LOCTEXT("Compiuto", "Compiuto");
		Azione = 1;
	}
	const FText Stato = bPrenotato
		? FText::Format(LOCTEXT("StatoPrenotato", "Prenotato: da assegnare, {0}"), ValdorsoGilda::TestoScadenza(P.ScadePrenotazione, Adesso))
		: FText::Format(LOCTEXT("StatoAssegnato", "Assegnato · {0} · scade {1}"), ValdorsoGilda::TestoProgresso(P, Nello), ValdorsoGilda::TestoScadenza(P.Foglio.Scadenza, Adesso));
	const int32 Numero = P.Foglio.Numero;
	const bool bConferma = RinunciaChiesta == Numero && FPlatformTime::Seconds() < RinunciaFino;
	TWeakObjectPtr<UValdorsoGildaComponent> DeboleGilda = Gilda;
	TWeakPtr<SValdorsoGilda> Debole = SharedThis(this);
	const bool bPuo = Perche.IsEmpty();

	return SNew(SBorder)
		.BorderImage(&Riquadro)
		.Padding(FMargin(10.f, 8.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 12.f, 0.f))
			[
				Sigillo(P.Foglio.Grado, 36.f)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("TitoloRiga", "{0} — {1}"), FText::FromString(P.Foglio.Titolo), ValdorsoGilda::TestoLavoro(P.Foglio)))
					.Font(Caratteri->Titolo(15.f, TEXT("Bold")))
					.ColorAndOpacity(ValdorsoTema::Inchiostro())
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("PagaRiga", "{0} · {1} Aureus e {2} punti"), FText::FromString(P.Foglio.Committente),
						FText::AsNumber(P.Foglio.Monete), FText::AsNumber(P.Foglio.Punti)))
					.Font(Caratteri->Testo(13.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(bPuo ? Stato : Perche)
					.Font(Caratteri->Testo(14.f))
					.ColorAndOpacity(bPuo ? ValdorsoTema::OroScuro() : ValdorsoTema::Rubrica())
					.AutoWrapText(true)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(10.f, 0.f, 0.f, 0.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
				[
					SNew(SBox)
					.MinDesiredWidth(170.f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.IsEnabled(bPuo)
						.OnClicked_Lambda([DeboleGilda, Numero, Azione]()
						{
							if (UValdorsoGildaComponent* Il = DeboleGilda.Get())
							{
								Il->ServerBancone(Numero, Azione);
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Text(Pulsante)
							.Font(Caratteri->Titolo(14.f, TEXT("Bold")))
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.MinDesiredWidth(170.f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.OnClicked_Lambda([DeboleGilda, Debole, Numero, bConferma, bPrenotato]()
						{
							TSharedPtr<SValdorsoGilda> Lo = Debole.Pin();
							if (!bConferma && !bPrenotato)
							{
								// da assegnato si chiede due volte: rinunciare costa punti
								if (Lo)
								{
									Lo->RinunciaChiesta = Numero;
									Lo->RinunciaFino = FPlatformTime::Seconds() + 4.0;
									Lo->bDaRifare = true;
								}
								return FReply::Handled();
							}
							if (UValdorsoGildaComponent* Il = DeboleGilda.Get())
							{
								Il->ServerBancone(Numero, 2);
							}
							if (Lo)
							{
								Lo->RinunciaChiesta = 0;
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Text(bConferma ? FText::Format(LOCTEXT("RinunciaSicuro", "Sicuro? -{0} punti"), FText::AsNumber(ValdorsoGilda::PuntiRinuncia))
								: (bPrenotato ? LOCTEXT("Rimetti", "Rimettilo in bacheca") : LOCTEXT("RinunciaPulsante", "Rinuncia")))
							.Font(Caratteri->Testo(13.f))
						]
					]
				]
			]
		];
}

FReply SValdorsoGilda::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::E || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SValdorsoGilda::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const APlayerController* Chi = Controllore.Get();
	const APawn* Corpo = Chi ? Chi->GetPawn() : nullptr;
	if (!Corpo || FVector::Dist(Corpo->GetActorLocation(), Partenza) > ValdorsoGilda::Raggio)
	{
		OnChiudi.ExecuteIfBound();
		return;
	}
	// la Gilda o lo zaino sono cambiati (un foglio staccato, assegnato, pagato): si rifa'
	const UValdorsoGildaComponent* G = Gilda.Get();
	const UValdorsoZainoComponent* Z = Zaino.Get();
	if (bDaRifare || (G && G->Cambiamenti() != VistoGilda) || (Z && Z->Cambiamenti() != VistoZaino)
		|| (RinunciaChiesta != 0 && FPlatformTime::Seconds() >= RinunciaFino))
	{
		bDaRifare = false;
		if (RinunciaChiesta != 0 && FPlatformTime::Seconds() >= RinunciaFino)
		{
			RinunciaChiesta = 0;
		}
		Rifai();
	}
}

namespace ValdorsoGildaSchermo
{
	bool Aperta(const APlayerController* Controllore)
	{
		const UValdorsoGildaComponent* Gilda = UValdorsoGildaComponent::Di(Controllore);
		return Gilda && Gilda->Schermata.IsValid();
	}

	bool Apri(APlayerController* Controllore, bool bBancone)
	{
		UValdorsoGildaComponent* Gilda = UValdorsoGildaComponent::Di(Controllore);
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		const AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore);
		if (!Gilda || !Viewport || !Controllore->IsLocalController() || !Controllore->GetPawn() || Gilda->Schermata.IsValid()
			|| (Nostro && Nostro->SchermataAperta()))
		{
			return false;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		SAssignNew(Gilda->Schermata, SValdorsoGilda)
			.Controllore(Controllore)
			.Bancone(bBancone)
			.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		Viewport->AddViewportWidgetContent(Gilda->Schermata.ToSharedRef(), 57);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Gilda->Schermata);
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
		return true;
	}

	void Rifai(APlayerController* Controllore)
	{
		if (UValdorsoGildaComponent* Gilda = UValdorsoGildaComponent::Di(Controllore); Gilda && Gilda->Schermata.IsValid())
		{
			Gilda->Schermata->Rifai();
		}
	}

	void Chiudi(APlayerController* Controllore)
	{
		UValdorsoGildaComponent* Gilda = UValdorsoGildaComponent::Di(Controllore);
		if (!Gilda || !Gilda->Schermata.IsValid())
		{
			return;
		}
		const TSharedRef<SValdorsoGilda> Schermo = Gilda->Schermata.ToSharedRef();
		Gilda->Schermata.Reset();
		UWorld* Mondo = Controllore->GetWorld();
		if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Schermo);
		}
		Controllore->SetInputMode(FInputModeGameOnly());
		Controllore->bShowMouseCursor = false;
	}
}

#undef LOCTEXT_NAMESPACE
