// Valdorso - La finestra dello staff (vedi SValdorsoStaff.h e ValdorsoStaff.h). Scritto da Claude il 10/11/2026.

#include "SValdorsoStaff.h"
#include "ValdorsoGildaDati.h"
#include "ValdorsoLupi.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPermessi.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoStaff.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoStaffSchermo"

namespace
{
	FText StaffNomeScheda(SValdorsoStaff::EScheda Scheda)
	{
		switch (Scheda)
		{
		case SValdorsoStaff::EScheda::Coloni: return LOCTEXT("Coloni", "Coloni e Gilda");
		case SValdorsoStaff::EScheda::Prove: return LOCTEXT("Prove", "Prove");
		case SValdorsoStaff::EScheda::Bacheche: return LOCTEXT("Bacheche", "Bacheche");
		case SValdorsoStaff::EScheda::Mondo: return LOCTEXT("Mondo", "Mondo");
		case SValdorsoStaff::EScheda::Giocatori: return LOCTEXT("Giocatori", "Giocatori");
		case SValdorsoStaff::EScheda::Registro: return LOCTEXT("Registro", "Registro");
		default: return LOCTEXT("Permessi", "Permessi");
		}
	}

	const TCHAR* StaffNomeRuolo(uint8 Ruolo)
	{
		return Ruolo == 2 ? TEXT("Amministratore") : (Ruolo == 1 ? TEXT("Staff") : TEXT("Giocatore"));
	}
}

void SValdorsoStaff::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	OnChiudi = InArgs._OnChiudi;
	Staff = UValdorsoStaffComponent::Di(Controllore.Get());
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Fondo = FSlateRoundedBoxBrush(ValdorsoTema::Pannelli().CopyWithNewOpacity(0.97f), 4.f, ValdorsoTema::OroScuro(), 1.5f);
	Velo = FSlateColorBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.35f));
	Riquadro = FSlateRoundedBoxBrush(ValdorsoTema::Pulsanti(), 2.f);
	RiquadroScelto = FSlateRoundedBoxBrush(ValdorsoTema::PulsanteSopra(), 2.f, ValdorsoTema::Oro(), 1.f);
	if (const UValdorsoStaffComponent* S = Staff.Get())
	{
		TestoFiltro = S->Filtro;
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(&Velo)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Fondo)
			.Padding(FMargin(20.f, 14.f))
			[
				SNew(SBox)
				.WidthOverride(1120.f)
				.HeightOverride(700.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const UValdorsoStaffComponent* S = Staff.Get();
							return S ? FText::Format(LOCTEXT("Titolo", "Strumenti dello staff · {0} · permessi: {1}{2}"), FText::FromString(S->Vista().Io),
								FText::FromString(ValdorsoPermessi::Testo(S->Vista().Permessi)), S->Vista().bCapo ? LOCTEXT("ECapo", " · capo gilda") : FText::GetEmpty())
								: FText::GetEmpty();
						})
						.Font(Caratteri->Titolo(18.f, TEXT("Bold")))
						.ColorAndOpacity(ValdorsoTema::Oro())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 8.f))
					[
						SAssignNew(BoxLinguette, SBox)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					[
						SAssignNew(Contenuto, SBox)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 8.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const UValdorsoStaffComponent* S = Staff.Get();
							return S && !S->Esito().IsEmpty() ? FText::FromString(S->Esito())
								: LOCTEXT("Aiuto", "Clic su un nome per sceglierlo · ogni azione finisce nel registro eventi · F9 o Esc: chiudi.");
						})
						.Font(Caratteri->Testo(14.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::TestoSecondario())
						.AutoWrapText(true)
					]
				]
			]
		]
	];
	Rifai();
}

bool SValdorsoStaff::SchedaVisibile(EScheda Quale) const
{
	const UValdorsoStaffComponent* S = Staff.Get();
	if (!S)
	{
		return false;
	}
	const int32 P = S->Vista().Permessi;
	switch (Quale)
	{
	case EScheda::Coloni: return (P & ValdorsoPermessi::Gilda) || S->Vista().bCapo || P != 0;
	case EScheda::Prove: return (P & (ValdorsoPermessi::Prove | ValdorsoPermessi::Abilita)) != 0;   // (11/11) anche solo per le abilita'
	case EScheda::Bacheche: return (P & ValdorsoPermessi::Bacheche) != 0;
	case EScheda::Mondo: return (P & ValdorsoPermessi::Mondo) != 0;
	case EScheda::Giocatori: return (P & ValdorsoPermessi::Giocatori) != 0;
	case EScheda::Registro: return (P & ValdorsoPermessi::Registro) != 0;
	case EScheda::Permessi: return (P & ValdorsoPermessi::Permessi) != 0;
	}
	return false;
}

FString SValdorsoStaff::Scelto() const
{
	return ColonoScelto;
}

void SValdorsoStaff::Rifai()
{
	const UValdorsoStaffComponent* S = Staff.Get();
	Visto = S ? S->Cambiamenti() : -1;
	if (!SchedaVisibile(Scheda))
	{
		Scheda = EScheda::Coloni;
	}
	if (BoxLinguette.IsValid())
	{
		BoxLinguette->SetContent(Linguette());
	}
	if (!Contenuto.IsValid())
	{
		return;
	}
	TSharedRef<SWidget> Dentro = SNullWidget::NullWidget;
	switch (Scheda)
	{
	case EScheda::Coloni: Dentro = Coloni(); break;
	case EScheda::Prove: Dentro = Prove(); break;
	case EScheda::Bacheche: Dentro = Bacheche(); break;
	case EScheda::Mondo: Dentro = Mondo(); break;
	case EScheda::Giocatori: Dentro = Giocatori(); break;
	case EScheda::Registro: Dentro = Registro(); break;
	case EScheda::Permessi: Dentro = Permessi(); break;
	}
	Contenuto->SetContent(Dentro);
}

TSharedRef<SWidget> SValdorsoStaff::Linguette()
{
	TSharedRef<SHorizontalBox> Fila = SNew(SHorizontalBox);
	TWeakPtr<SValdorsoStaff> Debole = SharedThis(this);
	for (uint8 i = 0; i <= static_cast<uint8>(EScheda::Permessi); ++i)
	{
		const EScheda Quale = static_cast<EScheda>(i);
		if (!SchedaVisibile(Quale))
		{
			continue;
		}
		Fila->AddSlot().AutoWidth().Padding(FMargin(0.f, 0.f, 6.f, 0.f))
		[
			SNew(SButton)
			.ButtonColorAndOpacity(Quale == Scheda ? ValdorsoTema::Oro() : FLinearColor::White)
			.OnClicked_Lambda([Debole, Quale]()
			{
				if (TSharedPtr<SValdorsoStaff> Lo = Debole.Pin())
				{
					Lo->Scheda = Quale;
					Lo->bDaRifare = true;
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(StaffNomeScheda(Quale))
				.Font(Caratteri->Titolo(14.f, TEXT("Bold")))
			]
		];
	}
	return Fila;
}

TSharedRef<SWidget> SValdorsoStaff::Pulsante(const FText& Testo, const FString& Azione, TFunction<FString()> Chi, TFunction<FString()> Valore, bool bAttivo)
{
	TWeakObjectPtr<UValdorsoStaffComponent> Debole = Staff;
	return SNew(SBox)
		.Padding(FMargin(0.f, 0.f, 6.f, 4.f))
		[
			SNew(SButton)
			.IsEnabled(bAttivo)
			.OnClicked_Lambda([Debole, Azione, Chi, Valore]()
			{
				if (UValdorsoStaffComponent* S = Debole.Get())
				{
					S->ServerAzione(Azione, Chi ? Chi() : FString(), Valore ? Valore() : FString());
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(Testo)
				.Font(Caratteri->Testo(13.f))
			]
		];
}

TSharedRef<SWidget> SValdorsoStaff::Casella(FString& Dove, const FText& Suggerimento, float Larga)
{
	FString* Puntatore = &Dove;
	return SNew(SBox)
		.WidthOverride(Larga)
		.Padding(FMargin(0.f, 0.f, 6.f, 4.f))
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Dove))
			.HintText(Suggerimento)
			.Font(Caratteri->Testo(13.f))
			// (le stringhe sono membri della finestra: vivono quanto lei)
			.OnTextChanged_Lambda([Puntatore](const FText& Nuovo) { *Puntatore = Nuovo.ToString(); })
		];
}

TSharedRef<SWidget> SValdorsoStaff::Titoletto(const FText& Testo)
{
	return SNew(SBox)
		.Padding(FMargin(0.f, 8.f, 0.f, 4.f))
		[
			SNew(STextBlock)
			.Text(Testo)
			.Font(Caratteri->Titolo(15.f, TEXT("Bold")))
			.ColorAndOpacity(ValdorsoTema::OroChiaro())
		];
}

TSharedRef<SWidget> SValdorsoStaff::Riga(const FString& Testo, const FLinearColor& Colore)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Testo))
		.Font(Caratteri->Testo(13.f))
		.ColorAndOpacity(Colore)
		.AutoWrapText(true);
}

// --- Coloni e Gilda ----------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoStaff::Coloni()
{
	const UValdorsoStaffComponent* S = Staff.Get();
	if (!S)
	{
		return SNullWidget::NullWidget;
	}
	const FValdorsoStaffVista& V = S->Vista();
	const bool bStaffGilda = (V.Permessi & ValdorsoPermessi::Gilda) != 0;
	TSharedRef<SVerticalBox> Lista = SNew(SVerticalBox);
	TWeakPtr<SValdorsoStaff> Debole = SharedThis(this);
	for (const FValdorsoStaffColono& C : V.Coloni)
	{
		const FString Nome = C.Nome;
		const bool bScelto = Nome == ColonoScelto;
		const EValdorsoGrado Dopo = ValdorsoGilda::GradoDopo(C.Grado);
		// il capo gilda: solo il grado dopo, a chi e' pronto, fino all'Argento, non a se stesso (il server ricontrolla)
		const bool bPuoSalire = C.Grado != EValdorsoGrado::Oro
			&& (bStaffGilda || (V.bCapo && C.bPronto && Dopo <= ValdorsoGilda::MassimoDelCapo && Nome != V.Io));
		const FString Su = FString::FromInt(static_cast<int32>(Dopo));
		const FString Giu = FString::FromInt(FMath::Max(0, static_cast<int32>(C.Grado) - 1));
		const FString Testo = FString::Printf(TEXT("%s · %s%s · %d punti%s · %d incarichi · %d Aureus · salute %d%s%s"), *C.Nome,
			*ValdorsoGilda::NomeGrado(C.Grado).ToString(), C.bCapo ? TEXT(" (capo gilda)") : TEXT(""), C.Punti,
			C.bPronto ? *FString::Printf(TEXT(" · PRONTO per il %s"), *ValdorsoGilda::NomeGrado(Dopo).ToString()) : TEXT(""),
			C.Incarichi, C.Monete, C.Salute, C.Luogo.IsEmpty() ? TEXT("") : TEXT(" · "), *C.Luogo);
		Lista->AddSlot().AutoHeight().Padding(FMargin(0.f, 2.f))
		[
			SNew(SBorder)
			.BorderImage(bScelto ? &RiquadroScelto : &Riquadro)
			.Padding(FMargin(8.f, 4.f))
			.OnMouseButtonDown_Lambda([Debole, Nome](const FGeometry&, const FPointerEvent&)
			{
				if (TSharedPtr<SValdorsoStaff> Lo = Debole.Pin())
				{
					Lo->ColonoScelto = Nome;
					Lo->bDaRifare = true;
				}
				return FReply::Handled();
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					Riga(Testo, C.bPronto ? ValdorsoTema::OroChiaro() : ValdorsoTema::Pergamena())
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					Pulsante(LOCTEXT("GradoSu", "Grado +"), TEXT("grado"), [Nome]() { return Nome; }, [Su]() { return Su; }, bPuoSalire)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					Pulsante(LOCTEXT("GradoGiu", "Grado −"), TEXT("grado"), [Nome]() { return Nome; }, [Giu]() { return Giu; }, bStaffGilda && C.Grado != EValdorsoGrado::Comune)
				]
			]
		];
	}
	if (V.Coloni.IsEmpty())
	{
		Lista->AddSlot().AutoHeight()[Riga(TEXT("Nessun colono nella valle."), ValdorsoTema::TestoSecondario())];
	}
	TSharedRef<SVerticalBox> Tutto = SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Lista]];
	if (bStaffGilda)
	{
		const FString* Punti = &TestoPunti;
		Tutto->AddSlot().AutoHeight()[Titoletto(FText::Format(LOCTEXT("AlScelto", "Al colono scelto ({0})"),
			FText::FromString(ColonoScelto.IsEmpty() ? TEXT("clic su un nome") : ColonoScelto)))];
		Tutto->AddSlot().AutoHeight()
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot()[Casella(TestoPunti, LOCTEXT("PuntiHint", "punti della Gilda"), 160.f)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("ImpostaPunti", "Imposta i punti"), TEXT("punti"), [this]() { return Scelto(); }, [Punti]() { return *Punti; }, !ColonoScelto.IsEmpty())]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("FaiCapo", "Fai capo gilda"), TEXT("capo"), [this]() { return Scelto(); }, []() { return FString(TEXT("1")); }, !ColonoScelto.IsEmpty())]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("TogliCapo", "Togli capo gilda"), TEXT("capo"), [this]() { return Scelto(); }, []() { return FString(TEXT("0")); }, !ColonoScelto.IsEmpty())]
		];
		Tutto->AddSlot().AutoHeight()[Riga(TEXT("I gradi: lo staff li da' a nome di Mastro Ettore, il capo della filiale. Il capo gilda dei giocatori promuove chi e' PRONTO, un grado alla volta, fino all'Argento."), ValdorsoTema::TestoSecondario())];
	}
	else if (V.bCapo)
	{
		Tutto->AddSlot().AutoHeight()[Riga(TEXT("Sei il capo gilda: puoi dare il grado dopo a chi e' PRONTO, fino all'Argento. Lo staff vede ogni nomina nel registro."), ValdorsoTema::TestoSecondario())];
	}
	return Tutto;
}

// --- Prove ---------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoStaff::Prove()
{
	const FString* Oggetto = &TestoOggetto;
	const FString* Monete = &TestoMonete;
	const FString* AbilitaScritta = &TestoAbilita;
	auto ChiScelto = [this]() { return Scelto(); };
	TSharedRef<SWrapBox> Oggetti = SNew(SWrapBox).UseAllottedSize(true);
	TWeakPtr<SValdorsoStaff> Debole = SharedThis(this);
	for (const FValdorsoTipoOggetto& Tipo : ValdorsoOggetti::Catalogo())
	{
		const FString Id = Tipo.Id.ToString();
		Oggetti->AddSlot().Padding(FMargin(0.f, 0.f, 4.f, 4.f))
		[
			SNew(SButton)
			.ToolTipText(FText::FromString(Tipo.Nome))
			.OnClicked_Lambda([Debole, Id]()
			{
				if (TSharedPtr<SValdorsoStaff> Lo = Debole.Pin())
				{
					Lo->TestoOggetto = Id + TEXT(" 1");
					Lo->bDaRifare = true;
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(Id)).Font(Caratteri->Testo(11.f))
			]
		];
	}
	return SNew(SScrollBox)
		+ SScrollBox::Slot()[Titoletto(FText::Format(LOCTEXT("ProveA", "A chi: {0} (clic su un nome in Coloni e Gilda; vuoto = a te)"),
			FText::FromString(ColonoScelto.IsEmpty() ? TEXT("a te") : ColonoScelto)))]
		+ SScrollBox::Slot()
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot()[Casella(TestoOggetto, LOCTEXT("OggettoHint", "Id quanti (es. Pane_Nero 3)"), 260.f)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Dai", "Dai"), TEXT("dai"), ChiScelto, [Oggetto]() { return *Oggetto; })]
			+ SWrapBox::Slot()[Casella(TestoMonete, LOCTEXT("MoneteHint", "Aureus"), 120.f)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Monete", "Imposta gli Aureus"), TEXT("monete"), ChiScelto, [Monete]() { return *Monete; })]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Cura", "Cura"), TEXT("cura"), ChiScelto, nullptr)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Armeria", "Armeria"), TEXT("armeria"), ChiScelto, nullptr)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Fatto", "Incarichi fatti"), TEXT("fatto"), ChiScelto, nullptr)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Svuota", "Svuota lo zaino"), TEXT("svuota"), ChiScelto, nullptr)]
		]
		// (11/11) le abilita' (permesso "abilita"): Id e punti 0..100
		+ SScrollBox::Slot()
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot()[Casella(TestoAbilita, LOCTEXT("AbilitaHint", "Abilita' punti (es. Lame 40)"), 260.f)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Abilita", "Imposta l'abilita'"), TEXT("abilita"), ChiScelto, [AbilitaScritta]() { return *AbilitaScritta; })]
		]
		+ SScrollBox::Slot()[Titoletto(LOCTEXT("Oggetti", "Gli oggetti (clic: nella casella)"))]
		+ SScrollBox::Slot()[Oggetti];
}

// --- Bacheche ------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoStaff::Bacheche()
{
	const UValdorsoStaffComponent* S = Staff.Get();
	if (!S)
	{
		return SNullWidget::NullWidget;
	}
	const FValdorsoStaffVista& V = S->Vista();
	const int64 Adesso = FDateTime::UtcNow().ToUnixTimestamp();
	TSharedRef<SVerticalBox> Sinistra = SNew(SVerticalBox);
	Sinistra->AddSlot().AutoHeight()[Titoletto(FText::Format(LOCTEXT("Incarichi", "Incarichi ({0}) · cassa del villaggio {1} Aureus"),
		FText::AsNumber(V.Incarichi.Num()), FText::AsNumber(V.Cassa)))];
	for (const FValdorsoIncarico& I : V.Incarichi)
	{
		const FString Numero = FString::FromInt(I.Numero);
		Sinistra->AddSlot().AutoHeight().Padding(FMargin(0.f, 1.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				Riga(FString::Printf(TEXT("n. %d %s · %s · %s · %d Aureus · presi %d su %d, compiuti %d · %s%s"), I.Numero, *I.Titolo,
					*ValdorsoGilda::TestoLavoro(I).ToString(), *ValdorsoGilda::NomeGrado(I.Grado).ToString(), I.Monete, I.Presi, I.Posti, I.Compiuti,
					*ValdorsoGilda::TestoScadenza(I.Scadenza, Adesso).ToString(), I.bAbitanti ? TEXT(" (abitanti)") : *FString::Printf(TEXT(" (di %s)"), *I.Autore)),
					ValdorsoTema::Pergamena())
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Pulsante(LOCTEXT("Togli", "Togli"), TEXT("togliincarico"), nullptr, [Numero]() { return Numero; })
			]
		];
	}
	Sinistra->AddSlot().AutoHeight()[Titoletto(FText::Format(LOCTEXT("Proclami", "Proclami e leggi ({0})"), FText::AsNumber(V.Proclami.Num())))];
	for (int32 k = V.Proclami.Num() - 1; k >= 0; --k)
	{
		const FValdorsoProclama& P = V.Proclami[k];
		const FString Numero = FString::FromInt(P.Numero);
		Sinistra->AddSlot().AutoHeight().Padding(FMargin(0.f, 1.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				Riga(FString::Printf(TEXT("n. %d%s %s (di %s)"), P.Numero, P.bLegge ? TEXT(" LEGGE") : TEXT(""), *P.Testo, *P.Autore), ValdorsoTema::Pergamena())
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Pulsante(LOCTEXT("TogliP", "Togli"), TEXT("togliproclama"), nullptr, [Numero]() { return Numero; })
			]
		];
	}
	const FString* Proclama = &TestoProclama;
	const FString* Annuncio = &TestoAnnuncio;
	const FString* Incarico = &TestoIncarico;
	const FString* Cassa = &TestoCassa;
	TSharedRef<SVerticalBox> Destra = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Titoletto(LOCTEXT("NuovoProclama", "Un proclama del Balivo (190 lettere)"))]
		+ SVerticalBox::Slot().AutoHeight()[Casella(TestoProclama, LOCTEXT("ProclamaHint", "Il mercato si tiene ogni giorno..."), 380.f)]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("Appendi", "Appendi il proclama"), TEXT("proclama"), nullptr, [Proclama]() { return *Proclama; })]
			+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("Legge", "Appendi come legge (da domani)"), TEXT("legge"), nullptr, [Proclama]() { return *Proclama; })]
		]
		// (13/11) gli annunci dello staff: li proclama il Banditore della Corona, a tutti i coloni nella valle, sulla bacheca
		// e (con Pubblica, scelta 2) sul sito e nel launcher. La Gazzetta del passo si compone fuori: Tools\Gazzetta.
		+ SVerticalBox::Slot().AutoHeight()[Titoletto(LOCTEXT("NuovoAnnuncio", "Un annuncio del Banditore (a tutti, e sul sito)"))]
		+ SVerticalBox::Slot().AutoHeight()[Casella(TestoAnnuncio, LOCTEXT("AnnuncioHint", "Stasera alle 21 la grande prova..."), 380.f)]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Pulsante(LOCTEXT("Annuncia", "Fai proclamare l'annuncio"), TEXT("annuncio"), nullptr, [Annuncio]() { return *Annuncio; })
		]
		+ SVerticalBox::Slot().AutoHeight()[Titoletto(LOCTEXT("NuovoIncarico", "Un incarico dello staff"))]
		+ SVerticalBox::Slot().AutoHeight()[Riga(TEXT("tipo obiettivo quanti grado monete posti giorni titolo | testo\nes. caccia Lupo 3 bronzo 40 2 5 I lupi del boschetto | Tre lupi, prima della luna nuova."), ValdorsoTema::TestoSecondario())]
		+ SVerticalBox::Slot().AutoHeight()[Casella(TestoIncarico, LOCTEXT("IncaricoHint", "caccia Lupo 3 bronzo 40 2 5 Titolo | testo"), 380.f)]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("AppendiIncarico", "Appendi l'incarico"), TEXT("incarico"), nullptr, [Incarico]() { return *Incarico; })]
			+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("Genera", "Uno degli abitanti"), TEXT("genera"), nullptr, nullptr)]
		]
		+ SVerticalBox::Slot().AutoHeight()[Titoletto(LOCTEXT("Cassa", "La cassa del villaggio"))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Casella(TestoCassa, LOCTEXT("CassaHint", "Aureus"), 120.f)]
			+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("ImpostaCassa", "Imposta la cassa"), TEXT("cassa"), nullptr, [Cassa]() { return *Cassa; })]
		];
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Sinistra]]
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(12.f, 0.f, 0.f, 0.f))[SNew(SBox).WidthOverride(400.f)[Destra]];
}

// --- Mondo ---------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoStaff::Mondo()
{
	const FString* Ora = &TestoOra;
	const FString* Giorno = &TestoGiorno;
	auto Fisso = [](const TCHAR* Valore) { const FString V(Valore); return [V]() { return V; }; };
	TSharedRef<SWrapBox> Tempi = SNew(SWrapBox).UseAllottedSize(true);
	for (const TCHAR* T : { TEXT("Sereno"), TEXT("Nuvoloso"), TEXT("Nebbia"), TEXT("Pioggia"), TEXT("Temporale") })
	{
		Tempi->AddSlot()[Pulsante(FText::FromString(T), TEXT("tempo"), nullptr, Fisso(T))];
	}
	TSharedRef<SWrapBox> Animali = SNew(SWrapBox).UseAllottedSize(true);
	for (const FValdorsoSpecie& Specie : ValdorsoAnimali::Tutte())
	{
		Animali->AddSlot()[Pulsante(FText::FromString(Specie.Id), TEXT("animali"), nullptr, Fisso(Specie.Id))];
	}
	Animali->AddSlot()[Pulsante(LOCTEXT("AllaTana", "Tutti alla tana"), TEXT("animalivia"), nullptr, nullptr)];
	return SNew(SScrollBox)
		+ SScrollBox::Slot()[Titoletto(LOCTEXT("Ora", "L'ora e il giorno"))]
		+ SScrollBox::Slot()
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot()[Casella(TestoOra, LOCTEXT("OraHint", "ora 0-24"), 100.f)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("ImpostaOra", "Imposta l'ora"), TEXT("ora"), nullptr, [Ora]() { return *Ora; })]
			+ SWrapBox::Slot()[Casella(TestoGiorno, LOCTEXT("GiornoHint", "minuti di un giorno"), 160.f)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("ImpostaGiorno", "Durata del giorno"), TEXT("giorno"), nullptr, [Giorno]() { return *Giorno; })]
		]
		+ SScrollBox::Slot()[Titoletto(LOCTEXT("Tempo", "Il tempo"))]
		+ SScrollBox::Slot()[Tempi]
		+ SScrollBox::Slot()[Titoletto(LOCTEXT("Animali", "Animali (25 m davanti a te)"))]
		+ SScrollBox::Slot()[Animali]
		+ SScrollBox::Slot()[Titoletto(LOCTEXT("Villaggio", "Il villaggio e la valle"))]
		+ SScrollBox::Slot()
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Ricresci", "La raccolta ricresce"), TEXT("ricresci"), nullptr, nullptr)]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Allarme", "Campane: allarme"), TEXT("campane"), nullptr, Fisso(TEXT("1")))]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Basta", "Campane: basta"), TEXT("campane"), nullptr, Fisso(TEXT("0")))]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Fase1", "Villaggio: fase 1"), TEXT("fase"), nullptr, Fisso(TEXT("1")))]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Fase2", "fase 2"), TEXT("fase"), nullptr, Fisso(TEXT("2")))]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("Fase3", "fase 3"), TEXT("fase"), nullptr, Fisso(TEXT("3")))]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("SigilloApri", "Sigillo del passo: apri"), TEXT("sigillo"), nullptr, Fisso(TEXT("apri")))]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("SigilloChiudi", "chiudi"), TEXT("sigillo"), nullptr, Fisso(TEXT("chiudi")))]
			// (11/11, v0.1.7) la frana della grotta del nord: la notte in cui trema, e (per le prove) aperta o chiusa
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("FranaTrema", "Frana del nord: trema"), TEXT("frana"), nullptr, Fisso(TEXT("trema")))]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("FranaApri", "apri"), TEXT("frana"), nullptr, Fisso(TEXT("apri")))]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("FranaChiudi", "richiudi"), TEXT("frana"), nullptr, Fisso(TEXT("chiudi")))]
			// (11/11, v0.1.6) l'Atto I: il battito del frammento salta (e resta debole, anche dopo un riavvio)
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("AttoUno", "Atto I: il battito salta"), TEXT("atto"), nullptr, Fisso(TEXT("1")))]
			+ SWrapBox::Slot()[Pulsante(LOCTEXT("AttoZero", "come prima (prove)"), TEXT("atto"), nullptr, Fisso(TEXT("0")))]
		]
		// (13/11) il collaudo della valle: il server riconosce tutte le cose che si usano con la E? (ValdorsoStaff.h)
		+ SScrollBox::Slot()[Titoletto(LOCTEXT("Collaudo", "Il collaudo (prima di ogni grande prova)"))]
		+ SScrollBox::Slot()
		[
			SNew(SBox)
			.Padding(FMargin(0.f, 0.f, 6.f, 4.f))
			.HAlign(HAlign_Left)
			[
				SNew(SButton)
				.OnClicked_Lambda([DeboleStaff = Staff]()
				{
					if (UValdorsoStaffComponent* S = DeboleStaff.Get())
					{
						S->Collauda();
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CollaudoVia", "La E su tutto: il server riconosce porte, PNG, pozzo...? (Saved/Claude/collaudo.txt)"))
					.Font(Caratteri->Testo(13.f))
				]
			]
		];
}

// --- Giocatori ------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoStaff::Giocatori()
{
	const UValdorsoStaffComponent* S = Staff.Get();
	if (!S)
	{
		return SNullWidget::NullWidget;
	}
	TSharedRef<SVerticalBox> Lista = SNew(SVerticalBox);
	for (const FValdorsoStaffColono& C : S->Vista().Coloni)
	{
		const FString Nome = C.Nome;
		Lista->AddSlot().AutoHeight().Padding(FMargin(0.f, 1.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				Riga(FString::Printf(TEXT("%s%s%s"), *C.Nome, C.Luogo.IsEmpty() ? TEXT("") : TEXT(" · "), *C.Luogo), ValdorsoTema::Pergamena())
			]
			+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("VaiDa", "Vai da"), TEXT("vaida"), [Nome]() { return Nome; }, nullptr, Nome != S->Vista().Io)]
			+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("Chiama", "Chiamalo qui"), TEXT("chiama"), [Nome]() { return Nome; }, nullptr, Nome != S->Vista().Io)]
		];
	}
	TSharedRef<SWrapBox> Luoghi = SNew(SWrapBox).UseAllottedSize(true);
	auto ChiScelto = [this]() { return Scelto(); };
	for (const FValdorsoLuogo& L : ValdorsoLuoghi::Tutti())
	{
		const FString Nome = L.Nome;
		Luoghi->AddSlot()[Pulsante(FText::FromString(L.Nome), TEXT("luogo"), ChiScelto, [Nome]() { return Nome; })];
	}
	return SNew(SScrollBox)
		+ SScrollBox::Slot()[Titoletto(LOCTEXT("ColoniValle", "I coloni nella valle"))]
		+ SScrollBox::Slot()[Lista]
		+ SScrollBox::Slot()[Titoletto(FText::Format(LOCTEXT("Porta", "Porta {0} in un luogo"),
			FText::FromString(ColonoScelto.IsEmpty() ? TEXT("te (nessuno scelto)") : ColonoScelto)))]
		+ SScrollBox::Slot()[Luoghi];
}

// --- Registro -------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoStaff::Registro()
{
	const UValdorsoStaffComponent* S = Staff.Get();
	if (!S)
	{
		return SNullWidget::NullWidget;
	}
	const FString* Filtro = &TestoFiltro;
	TSharedRef<SVerticalBox> Righe = SNew(SVerticalBox);
	for (const FString& R : S->Vista().Registro)
	{
		Righe->AddSlot().AutoHeight()[Riga(R, R.Contains(TEXT(" · Staff · ")) || R.Contains(TEXT(" · Gilda · ")) ? ValdorsoTema::OroChiaro() : ValdorsoTema::Pergamena())];
	}
	if (S->Vista().Registro.IsEmpty())
	{
		Righe->AddSlot().AutoHeight()[Riga(TEXT("Niente (da quando il server e' acceso). I giorni passati: Saved/Server/Eventi sul server."), ValdorsoTema::TestoSecondario())];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Casella(TestoFiltro, LOCTEXT("FiltroHint", "filtro: Gilda, Staff, un nome..."), 300.f)]
			+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("Aggiorna", "Aggiorna"), TEXT("aggiorna"), nullptr, [this, Filtro]()
			{
				// il filtro resta anche sul PC (per la prossima volta che si apre)
				if (UValdorsoStaffComponent* Il = Staff.Get())
				{
					Il->Filtro = *Filtro;
				}
				return *Filtro;
			})]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Righe]];
}

// --- Permessi -------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoStaff::Permessi()
{
	const UValdorsoStaffComponent* S = Staff.Get();
	if (!S)
	{
		return SNullWidget::NullWidget;
	}
	TSharedRef<SVerticalBox> Lista = SNew(SVerticalBox);
	TWeakPtr<SValdorsoStaff> Debole = SharedThis(this);
	for (const FValdorsoStaffColono& C : S->Vista().Coloni)
	{
		const FString Nome = C.Nome;
		// la bozza riparte da quello che dice il server quando cambia la' (un ruolo nuovo, un salvataggio)
		const int32* Gia = BozzeDi.Find(Nome);
		if (!Bozze.Contains(Nome) || !Gia || *Gia != C.Permessi)
		{
			Bozze.Add(Nome, C.Permessi);
			BozzeDi.Add(Nome, C.Permessi);
		}
		TSharedRef<SHorizontalBox> Spunte = SNew(SHorizontalBox);
		// i permessi sui permessi sono dell'Amministratore: non si danno
		for (int32 i = 0; i < ValdorsoPermessi::Quanti; ++i)
		{
			const int32 Bit = 1 << i;
			if (Bit == ValdorsoPermessi::Permessi)
			{
				continue;
			}
			Spunte->AddSlot().AutoWidth().Padding(FMargin(0.f, 0.f, 8.f, 0.f)).VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([Debole, Nome, Bit]()
				{
					const TSharedPtr<SValdorsoStaff> Lo = Debole.Pin();
					const int32* M = Lo ? Lo->Bozze.Find(Nome) : nullptr;
					return M && (*M & Bit) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([Debole, Nome, Bit](ECheckBoxState Stato)
				{
					if (TSharedPtr<SValdorsoStaff> Lo = Debole.Pin())
					{
						int32& M = Lo->Bozze.FindOrAdd(Nome);
						M = Stato == ECheckBoxState::Checked ? (M | Bit) : (M & ~Bit);
					}
				})
				[
					SNew(STextBlock).Text(FText::FromString(ValdorsoPermessi::Nome(i))).Font(Caratteri->Testo(12.f)).ColorAndOpacity(ValdorsoTema::Pergamena())
				]
			];
		}
		auto Lista_ = [Debole, Nome]()
		{
			const TSharedPtr<SValdorsoStaff> Lo = Debole.Pin();
			const int32* M = Lo ? Lo->Bozze.Find(Nome) : nullptr;
			return ValdorsoPermessi::Testo(M ? *M : 0);
		};
		Lista->AddSlot().AutoHeight().Padding(FMargin(0.f, 3.f))
		[
			SNew(SBorder)
			.BorderImage(&Riquadro)
			.Padding(FMargin(8.f, 4.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					Riga(FString::Printf(TEXT("%s · account %s · %s · permessi: %s"), *C.Nome, C.Account.IsEmpty() ? TEXT("(nessuno)") : *C.Account,
						StaffNomeRuolo(C.Ruolo), *ValdorsoPermessi::Testo(C.Permessi)), ValdorsoTema::Pergamena())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[Spunte]
					+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("SalvaPermessi", "Salva i permessi"), TEXT("permessi"), [Nome]() { return Nome; }, Lista_, C.Ruolo == 1)]
					+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("FaiStaff", "Fai Staff"), TEXT("ruolo"), [Nome]() { return Nome; }, []() { return FString(TEXT("staff")); }, C.Ruolo == 0)]
					+ SHorizontalBox::Slot().AutoWidth()[Pulsante(LOCTEXT("FaiGiocatore", "Torna Giocatore"), TEXT("ruolo"), [Nome]() { return Nome; }, []() { return FString(TEXT("giocatore")); }, C.Ruolo == 1)]
				]
			]
		];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Riga(TEXT("Solo per l'Amministratore. I permessi valgono per lo Staff (l'Amministratore li ha tutti). Valgono subito, anche per chi e' gia' nella valle. Dalla console del server: Valdorso.Account.Permessi e Valdorso.Account.Ruolo."), ValdorsoTema::TestoSecondario())]
		+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Lista]];
}

FReply SValdorsoStaff::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::F9 || Tasto == EKeys::F7)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SValdorsoStaff::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const UValdorsoStaffComponent* S = Staff.Get();
	if (!S || !Controllore.IsValid())
	{
		OnChiudi.ExecuteIfBound();
		return;
	}
	if (bDaRifare || S->Cambiamenti() != Visto)
	{
		bDaRifare = false;
		Rifai();
	}
}

namespace ValdorsoStaffSchermo
{
	bool Aperta(const APlayerController* Controllore)
	{
		const UValdorsoStaffComponent* Staff = UValdorsoStaffComponent::Di(Controllore);
		return Staff && Staff->Schermata.IsValid();
	}

	bool Apri(APlayerController* Controllore)
	{
		UValdorsoStaffComponent* Staff = UValdorsoStaffComponent::Di(Controllore);
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		const AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore);
		if (!Staff || !Viewport || !Controllore->IsLocalController() || Staff->Schermata.IsValid() || (Nostro && Nostro->SchermataAperta()))
		{
			return false;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		SAssignNew(Staff->Schermata, SValdorsoStaff)
			.Controllore(Controllore)
			.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		Viewport->AddViewportWidgetContent(Staff->Schermata.ToSharedRef(), 57);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Staff->Schermata);
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
		return true;
	}

	void Alterna(APlayerController* Controllore)
	{
		UValdorsoStaffComponent* Staff = UValdorsoStaffComponent::Di(Controllore);
		if (!Staff)
		{
			return;
		}
		if (Staff->Schermata.IsValid())
		{
			Chiudi(Controllore);
			return;
		}
		// la vista la manda il server (solo a chi ha i permessi): la finestra si apre quando arriva
		Staff->bApertura = true;
		Staff->ServerApri(Staff->Filtro);
	}

	void Chiudi(APlayerController* Controllore)
	{
		UValdorsoStaffComponent* Staff = UValdorsoStaffComponent::Di(Controllore);
		if (!Staff || !Staff->Schermata.IsValid())
		{
			return;
		}
		const TSharedRef<SValdorsoStaff> Schermo = Staff->Schermata.ToSharedRef();
		Staff->Schermata.Reset();
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
