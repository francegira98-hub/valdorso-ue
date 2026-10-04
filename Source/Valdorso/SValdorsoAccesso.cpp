// Valdorso - Le schermate dell'accesso (vedi il .h).

#include "SValdorsoAccesso.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoAccesso"

namespace
{
	const TCHAR* const SezioneConfig = TEXT("Valdorso.Accesso");

	bool NomeAccettabile(const FString& Nome)
	{
		if (Nome.Len() < 3 || Nome.Len() > 20)
		{
			return false;
		}
		for (int32 i = 0; i < Nome.Len(); ++i)
		{
			const TCHAR C = Nome[i];
			const bool bLettera = (C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('A') && C <= TEXT('Z'));
			const bool bCifra = C >= TEXT('0') && C <= TEXT('9');
			const bool bSegno = C == TEXT('.') || C == TEXT('-') || C == TEXT('_');
			if ((i == 0 && !bLettera) || (!bLettera && !bCifra && !bSegno))
			{
				return false;
			}
		}
		return true;
	}

	/** Le stesse regole del server, per avvisare subito (il server ricontrolla comunque). */
	FText ProblemaPasswordSchermata(const FString& Password, const FString& Nome)
	{
		if (Password.Len() < 10)
		{
			return LOCTEXT("PwCorta", "La password deve avere almeno 10 caratteri.");
		}
		if (Password.Len() > 128)
		{
			return LOCTEXT("PwLunga", "La password può avere al massimo 128 caratteri.");
		}
		if (Nome.Len() >= 3 && Password.ToLower().Contains(Nome.ToLower()))
		{
			return LOCTEXT("PwNome", "La password non può contenere il nome dell'account.");
		}
		return FText::GetEmpty();
	}

	bool CodiceAccettabile(const FString& Codice)
	{
		int32 Caratteri = 0;
		for (const TCHAR C : Codice)
		{
			Caratteri += FChar::IsAlnum(C) ? 1 : 0;
		}
		return Caratteri == 12 || Caratteri == 16;
	}

	bool BlocMaiuscAcceso()
	{
		return FSlateApplication::IsInitialized() && FSlateApplication::Get().GetModifierKeys().AreCapsLocked();
	}
}

// ------------------------------------------------------------------------------------------------
// Lo stile comune
// ------------------------------------------------------------------------------------------------

FValdorsoStileAccesso::FValdorsoStileAccesso()
{
	Caratteri = MakeShared<FValdorsoCaratteri>();

	Pieno = FSlateColorBrush(FLinearColor::White);
	Velo = FSlateColorBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.55f));
	SfondoPannello = FSlateRoundedBoxBrush(ValdorsoTema::Pannelli().CopyWithNewOpacity(0.97f), 6.f,
		ValdorsoTema::Oro().CopyWithNewOpacity(0.55f), 1.f);
	SfondoCampo = FSlateRoundedBoxBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.9f), 3.f,
		ValdorsoTema::Oro().CopyWithNewOpacity(0.25f), 1.f);
	SfondoCampoAttivo = FSlateRoundedBoxBrush(ValdorsoTema::Pulsanti(), 3.f, ValdorsoTema::Oro().CopyWithNewOpacity(0.85f), 1.f);

	StilePulsante = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(ValdorsoTema::Pulsanti(), 3.f, ValdorsoTema::Oro().CopyWithNewOpacity(0.35f), 1.f))
		.SetHovered(FSlateRoundedBoxBrush(ValdorsoTema::PulsanteSopra(), 3.f, ValdorsoTema::Oro().CopyWithNewOpacity(0.8f), 1.f))
		.SetPressed(FSlateRoundedBoxBrush(ValdorsoTema::PulsantePremuto(), 3.f, ValdorsoTema::Oro(), 1.f))
		.SetDisabled(FSlateRoundedBoxBrush(ValdorsoTema::Pulsanti().CopyWithNewOpacity(0.5f), 3.f, ValdorsoTema::Oro().CopyWithNewOpacity(0.12f), 1.f))
		.SetNormalPadding(FMargin(22.f, 8.f))
		.SetPressedPadding(FMargin(22.f, 9.f, 22.f, 7.f));

	StileCampo = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
	StileCampo
		.SetBackgroundImageNormal(SfondoCampo)
		.SetBackgroundImageHovered(SfondoCampoAttivo)
		.SetBackgroundImageFocused(SfondoCampoAttivo)
		.SetBackgroundImageReadOnly(SfondoCampo)
		.SetPadding(FMargin(14.f, 10.f))
		.SetForegroundColor(FSlateColor(ValdorsoTema::Pergamena()));
}

TSharedRef<SButton> FValdorsoStileAccesso::Pulsante(TAttribute<FText> Testo, FSimpleDelegate Azione, float Dimensione, TAttribute<bool> Abilitato)
{
	TSharedRef<TWeakPtr<SButton>> Riferimento = MakeShared<TWeakPtr<SButton>>();
	auto Attivo = [Riferimento]()
	{
		const TSharedPtr<SButton> P = Riferimento->Pin();
		return P.IsValid() && P->IsEnabled() && (P->IsHovered() || P->HasKeyboardFocus());
	};

	TSharedRef<SButton> Nuovo = SNew(SButton)
		.ButtonStyle(&StilePulsante)
		.IsEnabled(Abilitato)
		.IsFocusable(true)
		.HAlign(HAlign_Center)
		.OnClicked_Lambda([Azione]() { Azione.ExecuteIfBound(); return FReply::Handled(); })
		[
			SNew(STextBlock)
			.Text(Testo)
			.Font(Caratteri->Titolo(Dimensione, TEXT("Bold")))
			.ColorAndOpacity_Lambda([Attivo, Riferimento]()
			{
				const TSharedPtr<SButton> P = Riferimento->Pin();
				if (P.IsValid() && !P->IsEnabled())
				{
					return FSlateColor(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.4f));
				}
				return FSlateColor(Attivo() ? ValdorsoTema::OroChiaro() : ValdorsoTema::Oro());
			})
		];
	*Riferimento = Nuovo;
	return Nuovo;
}

TSharedRef<SEditableTextBox> FValdorsoStileAccesso::Campo(const FText& Suggerimento, TAttribute<bool> Nascosto, FOnTextCommitted SuInvio)
{
	return SNew(SEditableTextBox)
		.Style(&StileCampo)
		.Font(Caratteri->Testo(22.f))
		.ForegroundColor(FSlateColor(ValdorsoTema::Pergamena()))
		.HintText(Suggerimento)
		.IsPassword(Nascosto)
		.SelectAllTextWhenFocused(true)
		.OnTextCommitted(SuInvio);
}

TSharedRef<SWidget> FValdorsoStileAccesso::Etichetta(const FText& Testo)
{
	FSlateFontInfo Font = Caratteri->Titolo(13.f, TEXT("Bold"));
	Font.LetterSpacing = 180;
	return SNew(STextBlock).Text(Testo).Font(Font).ColorAndOpacity(ValdorsoTema::Oro().CopyWithNewOpacity(0.85f));
}

TSharedRef<SWidget> FValdorsoStileAccesso::Diamante(float Lato, TAttribute<FSlateColor> Colore)
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

TSharedRef<SWidget> FValdorsoStileAccesso::Separatore(float Larghezza)
{
	const FLinearColor Linea = ValdorsoTema::Oro().CopyWithNewOpacity(0.5f);
	return SNew(SBox).WidthOverride(Larghezza).HeightOverride(14.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(1.f) [ SNew(SImage).Image(&Pieno).ColorAndOpacity(Linea) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(12.f, 0.f))
			[
				Diamante(8.f, FSlateColor(ValdorsoTema::Oro()))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(1.f) [ SNew(SImage).Image(&Pieno).ColorAndOpacity(Linea) ]
			]
		];
}

// ------------------------------------------------------------------------------------------------
// Prima di entrare
// ------------------------------------------------------------------------------------------------

void SValdorsoPrimaDiEntrare::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	OnRichiesta = InArgs._OnRichiesta;
	OnIndietro = InArgs._OnIndietro;
	OnProvaLocale = InArgs._OnProvaLocale;
	bPrimoIngresso = InArgs._bPrimoIngresso;
	Messaggio = InArgs._Messaggio;
	bErrore = !Messaggio.IsEmpty();

	// Il nome ricordato dall'ultima volta (salvato sul PC del giocatore, mai la password).
	FString NomeRicordato = InArgs._NomeIniziale;
	if (GConfig)
	{
		GConfig->GetBool(SezioneConfig, TEXT("RicordaNome"), bRicordaNome, GGameUserSettingsIni);
		if (NomeRicordato.IsEmpty() && bRicordaNome)
		{
			GConfig->GetString(SezioneConfig, TEXT("UltimoNome"), NomeRicordato, GGameUserSettingsIni);
		}
	}

	const FOnTextCommitted Invio = FOnTextCommitted::CreateSP(this, &SValdorsoPrimaDiEntrare::SuInvio);
	const TAttribute<bool> Nascosta = TAttribute<bool>::CreateLambda([this]() { return !bMostraPassword; });
	const TAttribute<bool> Libero = TAttribute<bool>::CreateLambda([this]() { return !bInAttesa; });

	NomeEntra = Stile->Campo(LOCTEXT("SugNome", "Il nome del tuo account"), false, Invio);
	PasswordEntra = Stile->Campo(LOCTEXT("SugPassword", "La tua password"), Nascosta, Invio);
	Codice = Stile->Campo(LOCTEXT("SugCodice", "VALD-XXXX-XXXX-XXXX"), false, Invio);
	NomeNuovo = Stile->Campo(LOCTEXT("SugNomeNuovo", "Da 3 a 20 caratteri, comincia con una lettera"), false, Invio);
	PasswordNuova = Stile->Campo(LOCTEXT("SugPwNuova", "Almeno 10 caratteri"), Nascosta, Invio);
	PasswordRipetuta = Stile->Campo(LOCTEXT("SugPwRipeti", "La stessa password, di nuovo"), Nascosta, Invio);
	NomeRecupero = Stile->Campo(LOCTEXT("SugNomeRec", "Il nome del tuo account"), false, Invio);
	CodiceRecupero = Stile->Campo(LOCTEXT("SugCodiceRec", "XXXX-XXXX-XXXX-XXXX"), false, Invio);
	PasswordRecupero = Stile->Campo(LOCTEXT("SugPwRec", "Almeno 10 caratteri"), Nascosta, Invio);
	PasswordRecuperoRipetuta = Stile->Campo(LOCTEXT("SugPwRecRipeti", "La stessa password, di nuovo"), Nascosta, Invio);
	NomeEntra->SetText(FText::FromString(NomeRicordato));
	NomeRecupero->SetText(FText::FromString(NomeRicordato));
	NomeNuovo->SetText(FText::FromString(NomeRicordato));

	const TAttribute<EVisibility> VisibileEntra = TAttribute<EVisibility>::CreateLambda([this]()
	{
		return bPrimoIngresso || bRecupero ? EVisibility::Collapsed : EVisibility::Visible;
	});
	const TAttribute<EVisibility> VisibileRecupero = TAttribute<EVisibility>::CreateLambda([this]()
	{
		return !bPrimoIngresso && bRecupero ? EVisibility::Visible : EVisibility::Collapsed;
	});
	const TAttribute<EVisibility> VisibilePrimo = TAttribute<EVisibility>::CreateLambda([this]()
	{
		return bPrimoIngresso ? EVisibility::Visible : EVisibility::Collapsed;
	});

	FSlateFontInfo FontTitolo = Stile->Caratteri->Titolo(40.f, TEXT("Bold"));
	FontTitolo.LetterSpacing = 60;

	ChildSlot
	[
		SNew(SOverlay)

		// Un velo leggero: la scena resta visibile dietro.
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&Stile->Velo)
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Stile->SfondoPannello)
			.Padding(FMargin(54.f, 40.f))
			[
				SNew(SBox).WidthOverride(560.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Titolo", "Prima di entrare"))
						.Font(FontTitolo)
						.ColorAndOpacity_Lambda([]()
						{
							const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
							return FSlateColor(FMath::Lerp(ValdorsoTema::Oro(), ValdorsoTema::OroChiaro(), Colpo * 0.6f));
						})
					]

					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 12.f, 0.f, 18.f))
					[
						Stile->Separatore(300.f)
					]

					// Le due schede.
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 22.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 26.f, 0.f))
						[
							Scheda(LOCTEXT("SchedaEntra", "Entra"), false)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							Scheda(LOCTEXT("SchedaPrimo", "Primo ingresso"), true)
						]
					]

					// Entra.
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SVerticalBox)
						.Visibility(VisibileEntra)
						.IsEnabled(Libero)
						+ SVerticalBox::Slot().AutoHeight() [ Riga(LOCTEXT("EtNome", "NOME"), NomeEntra.ToSharedRef()) ]
						+ SVerticalBox::Slot().AutoHeight() [ Riga(LOCTEXT("EtPassword", "PASSWORD"), PasswordEntra.ToSharedRef()) ]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(FMargin(0.f, -6.f, 0.f, 6.f))
						[
							SNew(SButton)
							.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
							.OnClicked_Lambda([this]() { ScegliRecupero(true); return FReply::Handled(); })
							[
								SNew(STextBlock)
								.Text(LOCTEXT("Persa", "Ho perso la password"))
								.Font(Stile->Caratteri->Testo(18.f, TEXT("Italic")))
								.ColorAndOpacity(ValdorsoTema::Oro().CopyWithNewOpacity(0.8f))
							]
						]
					]

					// Password dimenticata.
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SVerticalBox)
						.Visibility(VisibileRecupero)
						.IsEnabled(Libero)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SpiegaRecupero", "Scrivi il nome e uno dei codici di recupero che hai ricevuto al primo ingresso: vale una volta sola. Poi scegli una password nuova."))
							.Font(Stile->Caratteri->Testo(19.f, TEXT("Italic")))
							.ColorAndOpacity(ValdorsoTema::TestoSecondario())
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 10.f, 0.f, 0.f)) [ Riga(LOCTEXT("EtNomeRec", "NOME"), NomeRecupero.ToSharedRef()) ]
						+ SVerticalBox::Slot().AutoHeight() [ Riga(LOCTEXT("EtCodiceRec", "CODICE DI RECUPERO"), CodiceRecupero.ToSharedRef()) ]
						+ SVerticalBox::Slot().AutoHeight() [ Riga(LOCTEXT("EtPwRec", "PASSWORD NUOVA"), PasswordRecupero.ToSharedRef()) ]
						+ SVerticalBox::Slot().AutoHeight() [ Riga(LOCTEXT("EtPwRecRipeti", "RIPETI LA PASSWORD NUOVA"), PasswordRecuperoRipetuta.ToSharedRef()) ]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(FMargin(0.f, -6.f, 0.f, 6.f))
						[
							SNew(SButton)
							.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
							.OnClicked_Lambda([this]() { ScegliRecupero(false); return FReply::Handled(); })
							[
								SNew(STextBlock)
								.Text(LOCTEXT("Ricordata", "Me la ricordo: torna a Entra"))
								.Font(Stile->Caratteri->Testo(18.f, TEXT("Italic")))
								.ColorAndOpacity(ValdorsoTema::Oro().CopyWithNewOpacity(0.8f))
							]
						]
					]

					// Primo ingresso.
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SVerticalBox)
						.Visibility(VisibilePrimo)
						.IsEnabled(Libero)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SpiegaInvito", "Per entrare la prima volta serve un codice d'invito: te lo dà l'Amministratrice. Vale una volta sola."))
							.Font(Stile->Caratteri->Testo(19.f, TEXT("Italic")))
							.ColorAndOpacity(ValdorsoTema::TestoSecondario())
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 10.f, 0.f, 0.f)) [ Riga(LOCTEXT("EtCodice", "CODICE D'INVITO"), Codice.ToSharedRef()) ]
						+ SVerticalBox::Slot().AutoHeight() [ Riga(LOCTEXT("EtNomeNuovo", "NOME DELL'ACCOUNT"), NomeNuovo.ToSharedRef()) ]
						+ SVerticalBox::Slot().AutoHeight() [ Riga(LOCTEXT("EtPwNuova", "PASSWORD"), PasswordNuova.ToSharedRef()) ]
						+ SVerticalBox::Slot().AutoHeight() [ Riga(LOCTEXT("EtPwRipeti", "RIPETI LA PASSWORD"), PasswordRipetuta.ToSharedRef()) ]
					]

					// Mostra la password, ricorda il nome.
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 4.f, 0.f, 0.f))
					[
						SNew(SHorizontalBox)
						.IsEnabled(Libero)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
							.OnClicked_Lambda([this]() { bMostraPassword = !bMostraPassword; return FReply::Handled(); })
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(2.f, 0.f, 10.f, 0.f))
								[
									Stile->Diamante(9.f, TAttribute<FSlateColor>::CreateLambda([this]()
									{
										return FSlateColor(bMostraPassword ? ValdorsoTema::Brace() : ValdorsoTema::Oro().CopyWithNewOpacity(0.25f));
									}))
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text(LOCTEXT("Mostra", "Mostra la password"))
									.Font(Stile->Caratteri->Testo(19.f)).ColorAndOpacity(ValdorsoTema::TestoSecondario())
								]
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(28.f, 0.f, 0.f, 0.f))
						[
							SNew(SButton)
							.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
							.OnClicked_Lambda([this]() { bRicordaNome = !bRicordaNome; return FReply::Handled(); })
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(2.f, 0.f, 10.f, 0.f))
								[
									Stile->Diamante(9.f, TAttribute<FSlateColor>::CreateLambda([this]()
									{
										return FSlateColor(bRicordaNome ? ValdorsoTema::Brace() : ValdorsoTema::Oro().CopyWithNewOpacity(0.25f));
									}))
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text(LOCTEXT("Ricorda", "Ricorda il nome"))
									.Font(Stile->Caratteri->Testo(19.f)).ColorAndOpacity(ValdorsoTema::TestoSecondario())
								]
							]
						]
					]

					// Bloc Maiusc.
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Maiusc", "Attenzione: il Bloc Maiusc è acceso."))
						.Font(Stile->Caratteri->Testo(18.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::Brace())
						.Visibility_Lambda([]() { return BlocMaiuscAcceso() ? EVisibility::Visible : EVisibility::Collapsed; })
					]

					// Il messaggio.
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 12.f, 0.f, 0.f))
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

					// I pulsanti.
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 26.f, 0.f, 0.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Left)
						[
							Stile->Pulsante(LOCTEXT("Indietro", "Indietro"),
								FSimpleDelegate::CreateLambda([this]() { if (!bInAttesa) { OnIndietro.ExecuteIfBound(); } }), 20.f, Libero)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							Stile->Pulsante(
								TAttribute<FText>::CreateLambda([this]()
								{
									if (bPrimoIngresso)
									{
										return LOCTEXT("Crea", "Crea l'account");
									}
									return bRecupero ? LOCTEXT("CambiaEntra", "Cambia la password ed entra") : LOCTEXT("EntraOra", "Entra nella valle");
								}),
								FSimpleDelegate::CreateSP(this, &SValdorsoPrimaDiEntrare::Invia), 22.f, Libero)
						]
					]

#if !UE_BUILD_SHIPPING
					// Solo nelle versioni di sviluppo: la mappa di prova in locale, senza server.
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 22.f, 0.f, 0.f))
					[
						SNew(SButton)
						.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
						.IsEnabled(Libero)
						.OnClicked_Lambda([this]() { OnProvaLocale.ExecuteIfBound(); return FReply::Handled(); })
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ProvaLocale", "Sviluppo: prova senza server"))
							.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
							.ColorAndOpacity(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.6f))
						]
					]
#endif
				]
			]
		]
	];
}

TSharedRef<SWidget> SValdorsoPrimaDiEntrare::Scheda(const FText& Testo, bool bPrimo)
{
	FSlateFontInfo Font = Stile->Caratteri->Titolo(21.f, TEXT("Bold"));
	Font.LetterSpacing = 80;
	return SNew(SButton)
		.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
		.IsEnabled_Lambda([this]() { return !bInAttesa; })
		.OnClicked_Lambda([this, bPrimo]() { ScegliScheda(bPrimo); return FReply::Handled(); })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(Testo)
				.Font(Font)
				.ColorAndOpacity_Lambda([this, bPrimo]()
				{
					return FSlateColor(bPrimoIngresso == bPrimo ? ValdorsoTema::Oro() : ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.7f));
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 6.f, 0.f, 0.f))
			[
				SNew(SBox).HeightOverride(2.f)
				[
					SNew(SImage)
					.Image(&Stile->Pieno)
					.ColorAndOpacity_Lambda([this, bPrimo]()
					{
						return FSlateColor(ValdorsoTema::Brace().CopyWithNewOpacity(bPrimoIngresso == bPrimo ? 1.f : 0.f));
					})
				]
			]
		];
}

TSharedRef<SWidget> SValdorsoPrimaDiEntrare::Riga(const FText& Titolo, const TSharedRef<SWidget>& Contenuto)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(2.f, 0.f, 0.f, 5.f)) [ Stile->Etichetta(Titolo) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 14.f)) [ Contenuto ];
}

void SValdorsoPrimaDiEntrare::ScegliRecupero(bool bSi)
{
	bRecupero = bSi;
	Messaggio = FText::GetEmpty();
	if (bSi && NomeRecupero->GetText().IsEmpty())
	{
		NomeRecupero->SetText(NomeEntra->GetText());
	}
	if (TSharedPtr<SWidget> Campo = CampoIniziale())
	{
		FSlateApplication::Get().SetAllUserFocus(Campo, EFocusCause::SetDirectly);
	}
}

void SValdorsoPrimaDiEntrare::ScegliScheda(bool bPrimo)
{
	bPrimoIngresso = bPrimo;
	bRecupero = false;
	Messaggio = FText::GetEmpty();
	if (TSharedPtr<SWidget> Campo = CampoIniziale())
	{
		FSlateApplication::Get().SetAllUserFocus(Campo, EFocusCause::SetDirectly);
	}
}

TSharedPtr<SWidget> SValdorsoPrimaDiEntrare::CampoIniziale() const
{
	if (bPrimoIngresso)
	{
		return Codice;
	}
	if (bRecupero)
	{
		return NomeRecupero->GetText().IsEmpty() ? NomeRecupero : CodiceRecupero;
	}
	return NomeEntra->GetText().IsEmpty() ? NomeEntra : PasswordEntra;
}

void SValdorsoPrimaDiEntrare::MostraMessaggio(const FText& Testo, bool bComeErrore)
{
	bInAttesa = false;
	bErrore = bComeErrore;
	Messaggio = Testo;
}

void SValdorsoPrimaDiEntrare::Attendi(const FText& Testo)
{
	bInAttesa = true;
	Messaggio = Testo;
}

bool SValdorsoPrimaDiEntrare::Controlla(FValdorsoRichiestaAccesso& Out, FText& OutErrore) const
{
	if (!bPrimoIngresso && bRecupero)
	{
		Out.Modo = EValdorsoModoAccesso::Recupero;
		Out.Nome = NomeRecupero->GetText().ToString().TrimStartAndEnd();
		Out.CodiceRecupero = CodiceRecupero->GetText().ToString().TrimStartAndEnd();
		Out.Password = PasswordRecupero->GetText().ToString();
		int32 Caratteri = 0;
		for (const TCHAR C : Out.CodiceRecupero)
		{
			Caratteri += FChar::IsAlnum(C) ? 1 : 0;
		}
		if (Out.Nome.IsEmpty())
		{
			OutErrore = LOCTEXT("RecNome", "Scrivi il nome dell'account.");
			return false;
		}
		if (Caratteri != 16)
		{
			OutErrore = LOCTEXT("RecForma", "Il codice di recupero ha questa forma: XXXX-XXXX-XXXX-XXXX.");
			return false;
		}
		const FText Problema = ProblemaPasswordSchermata(Out.Password, Out.Nome);
		if (!Problema.IsEmpty())
		{
			OutErrore = Problema;
			return false;
		}
		if (Out.Password != PasswordRecuperoRipetuta->GetText().ToString())
		{
			OutErrore = LOCTEXT("RecDiverse", "Le due password nuove non sono uguali.");
			return false;
		}
		return true;
	}

	if (!bPrimoIngresso)
	{
		Out.Modo = EValdorsoModoAccesso::Entra;
		Out.Nome = NomeEntra->GetText().ToString().TrimStartAndEnd();
		Out.Password = PasswordEntra->GetText().ToString();
		if (Out.Nome.IsEmpty() || Out.Password.IsEmpty())
		{
			OutErrore = LOCTEXT("MancaQualcosa", "Scrivi il nome e la password.");
			return false;
		}
		return true;
	}

	Out.Modo = EValdorsoModoAccesso::PrimoIngresso;
	Out.CodiceInvito = Codice->GetText().ToString().TrimStartAndEnd();
	Out.Nome = NomeNuovo->GetText().ToString().TrimStartAndEnd();
	Out.Password = PasswordNuova->GetText().ToString();
	if (!CodiceAccettabile(Out.CodiceInvito))
	{
		OutErrore = LOCTEXT("CodiceForma", "Il codice d'invito ha questa forma: VALD-XXXX-XXXX-XXXX.");
		return false;
	}
	if (!NomeAccettabile(Out.Nome))
	{
		OutErrore = LOCTEXT("NomeForma", "Il nome deve avere da 3 a 20 caratteri (lettere senza accenti, numeri, punto, trattino o trattino basso) e cominciare con una lettera.");
		return false;
	}
	const FText Problema = ProblemaPasswordSchermata(Out.Password, Out.Nome);
	if (!Problema.IsEmpty())
	{
		OutErrore = Problema;
		return false;
	}
	if (Out.Password != PasswordRipetuta->GetText().ToString())
	{
		OutErrore = LOCTEXT("PwDiverse", "Le due password non sono uguali.");
		return false;
	}
	return true;
}

void SValdorsoPrimaDiEntrare::Invia()
{
	if (bInAttesa)
	{
		return;
	}
	FValdorsoRichiestaAccesso Richiesta;
	FText Errore;
	if (!Controlla(Richiesta, Errore))
	{
		MostraMessaggio(Errore, true);
		return;
	}

	if (GConfig)
	{
		GConfig->SetBool(SezioneConfig, TEXT("RicordaNome"), bRicordaNome, GGameUserSettingsIni);
		GConfig->SetString(SezioneConfig, TEXT("UltimoNome"), bRicordaNome ? *Richiesta.Nome : TEXT(""), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

	// Le password non restano nei campi.
	PasswordEntra->SetText(FText::GetEmpty());
	PasswordNuova->SetText(FText::GetEmpty());
	PasswordRipetuta->SetText(FText::GetEmpty());
	CodiceRecupero->SetText(FText::GetEmpty());
	PasswordRecupero->SetText(FText::GetEmpty());
	PasswordRecuperoRipetuta->SetText(FText::GetEmpty());

	Attendi(LOCTEXT("InCammino", "In cammino verso la valle..."));
	OnRichiesta.ExecuteIfBound(Richiesta);
}

void SValdorsoPrimaDiEntrare::SuInvio(const FText& Testo, ETextCommit::Type Tipo)
{
	if (Tipo == ETextCommit::OnEnter)
	{
		Invia();
	}
}

FReply SValdorsoPrimaDiEntrare::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	if ((Tasto == EKeys::Escape || Tasto == EKeys::Gamepad_FaceButton_Right) && !bInAttesa)
	{
		if (bRecupero && !bPrimoIngresso)
		{
			ScegliRecupero(false);
			return FReply::Handled();
		}
		OnIndietro.ExecuteIfBound();
		return FReply::Handled();
	}
	if (Tasto == EKeys::Gamepad_LeftShoulder || Tasto == EKeys::Gamepad_RightShoulder)
	{
		ScegliScheda(Tasto == EKeys::Gamepad_RightShoulder);
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

// ------------------------------------------------------------------------------------------------
// L'anticamera
// ------------------------------------------------------------------------------------------------

void SValdorsoAnticamera::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	OnCambia = InArgs._OnCambia;
	Messaggio = LOCTEXT("Riconoscendo", "Il Cuore ti sta riconoscendo...");

	const FOnTextCommitted Invio = FOnTextCommitted::CreateSP(this, &SValdorsoAnticamera::SuInvio);
	Attuale = Stile->Campo(LOCTEXT("SugTemp", "La password temporanea"), true, Invio);
	Nuova = Stile->Campo(LOCTEXT("SugNuova", "Almeno 10 caratteri"), true, Invio);
	Ripetuta = Stile->Campo(LOCTEXT("SugRipeti", "La stessa password, di nuovo"), true, Invio);

	const TAttribute<EVisibility> VisibileCambio = TAttribute<EVisibility>::CreateLambda([this]()
	{
		return bCambio ? EVisibility::Visible : EVisibility::Collapsed;
	});

	ChildSlot
	[
		SNew(SOverlay)

		// Fondo pieno: dietro c'è il mondo, ma non ancora per noi.
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&Stile->Pieno).ColorAndOpacity(ValdorsoTema::Fondo())
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(560.f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 18.f))
				[
					Stile->Diamante(16.f, TAttribute<FSlateColor>::CreateLambda([]()
					{
						const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
						return FSlateColor(FMath::Lerp(ValdorsoTema::Brace().CopyWithNewOpacity(0.5f), ValdorsoTema::OroChiaro(), Colpo));
					}))
				]

				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return Messaggio; })
					.Font(Stile->Caratteri->Testo(26.f, TEXT("Italic")))
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
					.ColorAndOpacity_Lambda([this]()
					{
						return FSlateColor(bErrore ? ValdorsoTema::Brace() : ValdorsoTema::Pergamena());
					})
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 26.f, 0.f, 0.f))
				[
					SNew(SVerticalBox)
					.Visibility(VisibileCambio)
					.IsEnabled_Lambda([this]() { return !bInAttesa; })
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(2.f, 0.f, 0.f, 5.f)) [ Stile->Etichetta(LOCTEXT("EtTemp", "PASSWORD TEMPORANEA")) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 14.f)) [ Attuale.ToSharedRef() ]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(2.f, 0.f, 0.f, 5.f)) [ Stile->Etichetta(LOCTEXT("EtNuova", "PASSWORD NUOVA")) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 14.f)) [ Nuova.ToSharedRef() ]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(2.f, 0.f, 0.f, 5.f)) [ Stile->Etichetta(LOCTEXT("EtRipeti", "RIPETI LA PASSWORD NUOVA")) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 20.f)) [ Ripetuta.ToSharedRef() ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						Stile->Pulsante(LOCTEXT("Conferma", "Conferma ed entra"), FSimpleDelegate::CreateSP(this, &SValdorsoAnticamera::Invia), 22.f)
					]
				]
			]
		]
	];
}

void SValdorsoAnticamera::MostraAttesa(const FText& Testo)
{
	bInAttesa = true;
	bErrore = false;
	Messaggio = Testo;
}

void SValdorsoAnticamera::MostraCambioPassword(const FText& Testo, bool bComeErrore)
{
	bCambio = true;
	bInAttesa = false;
	bErrore = bComeErrore;
	Messaggio = Testo;
}

TSharedPtr<SWidget> SValdorsoAnticamera::CampoIniziale() const
{
	return Attuale;
}

void SValdorsoAnticamera::Invia()
{
	if (bInAttesa)
	{
		return;
	}
	const FString PwAttuale = Attuale->GetText().ToString();
	const FString PwNuova = Nuova->GetText().ToString();
	if (PwAttuale.IsEmpty())
	{
		MostraCambioPassword(LOCTEXT("ScriviTemp", "Scrivi la password temporanea che ti ha dato l'Amministratrice."), true);
		return;
	}
	const FText Problema = ProblemaPasswordSchermata(PwNuova, FString());
	if (!Problema.IsEmpty())
	{
		MostraCambioPassword(Problema, true);
		return;
	}
	if (PwNuova != Ripetuta->GetText().ToString())
	{
		MostraCambioPassword(LOCTEXT("NuoveDiverse", "Le due password nuove non sono uguali."), true);
		return;
	}
	Attuale->SetText(FText::GetEmpty());
	Nuova->SetText(FText::GetEmpty());
	Ripetuta->SetText(FText::GetEmpty());
	MostraAttesa(LOCTEXT("Salvo", "Il Cuore impara la tua nuova parola..."));
	OnCambia.ExecuteIfBound(PwAttuale, PwNuova);
}

void SValdorsoAnticamera::SuInvio(const FText& Testo, ETextCommit::Type Tipo)
{
	if (Tipo == ETextCommit::OnEnter)
	{
		Invia();
	}
}

// ------------------------------------------------------------------------------------------------
// I codici di recupero
// ------------------------------------------------------------------------------------------------

void SValdorsoCodiciRecupero::Construct(const FArguments& InArgs)
{
	Stile = MakeShared<FValdorsoStileAccesso>();
	Codici = InArgs._Codici;
	OnFatto = InArgs._OnFatto;

	// (05/10) Titolo e codici erano troppo larghi per il pannello: caratteri un poco più piccoli e pannello più largo.
	FSlateFontInfo FontTitolo = Stile->Caratteri->Titolo(30.f, TEXT("Bold"));
	FontTitolo.LetterSpacing = 30;
	FSlateFontInfo FontCodice = Stile->Caratteri->Titolo(21.f, TEXT("Bold"));
	FontCodice.LetterSpacing = 60;

	// Due colonne di codici.
	TSharedRef<SHorizontalBox> Colonne = SNew(SHorizontalBox);
	for (int32 Colonna = 0; Colonna < 2; ++Colonna)
	{
		TSharedRef<SVerticalBox> Elenco = SNew(SVerticalBox);
		for (int32 i = Colonna; i < Codici.Num(); i += 2)
		{
			Elenco->AddSlot().AutoHeight().Padding(FMargin(0.f, 5.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(0.f, 0.f, 12.f, 0.f))
				[
					SNew(STextBlock)
					.Text(FText::AsNumber(i + 1))
					.Font(Stile->Caratteri->Testo(18.f))
					.ColorAndOpacity(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(0.7f))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Codici[i])).Font(FontCodice).ColorAndOpacity(ValdorsoTema::Pergamena())
				]
			];
		}
		Colonne->AddSlot().AutoWidth().Padding(FMargin(Colonna == 0 ? 0.f : 40.f, 0.f, 0.f, 0.f)) [ Elenco ];
	}

	TSharedRef<SButton> PulsanteFatto = Stile->Pulsante(LOCTEXT("CodiciFatto", "Entra nella valle"),
		FSimpleDelegate::CreateLambda([this]() { if (bScritti) { OnFatto.ExecuteIfBound(); } }), 22.f,
		TAttribute<bool>::CreateLambda([this]() { return bScritti; }));

	TSharedRef<SButton> Spunta = SNew(SButton)
		.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
		.IsFocusable(true)
		.OnClicked_Lambda([this]() { bScritti = !bScritti; return FReply::Handled(); })
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(2.f, 0.f, 10.f, 0.f))
			[
				Stile->Diamante(10.f, TAttribute<FSlateColor>::CreateLambda([this]()
				{
					return FSlateColor(bScritti ? ValdorsoTema::Brace() : ValdorsoTema::Oro().CopyWithNewOpacity(0.3f));
				}))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("CodiciScritti", "Li ho scritti e li tengo in un posto sicuro"))
				.Font(Stile->Caratteri->Testo(20.f)).ColorAndOpacity(ValdorsoTema::Pergamena())
			]
		];
	Conferma = Spunta;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot() [ SNew(SImage).Image(&Stile->Pieno).ColorAndOpacity(ValdorsoTema::Fondo().CopyWithNewOpacity(0.92f)) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Stile->SfondoPannello)
			.Padding(FMargin(54.f, 40.f))
			[
				SNew(SBox).WidthOverride(820.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("CodiciTitolo", "I tuoi codici di recupero")).Font(FontTitolo).ColorAndOpacity(ValdorsoTema::Oro())
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 12.f, 0.f, 16.f))
					[
						Stile->Separatore(300.f)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CodiciSpiega", "Se un giorno dimentichi la password, con il tuo nome e uno di questi codici ne scegli una nuova. Ognuno vale una volta sola. Scrivili su carta e tienili al sicuro: dopo questa schermata non li vedrai più, e nessuno, nemmeno l'Amministratrice, li può rileggere."))
						.Font(Stile->Caratteri->Testo(20.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::TestoSecondario())
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 22.f))
					[
						Colonne
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 18.f))
					[
						Stile->Pulsante(
							TAttribute<FText>::CreateLambda([this]()
							{
								return bCopiati ? LOCTEXT("CodiciCopiati", "Copiati: incollali in un posto sicuro, poi copia altro") : LOCTEXT("CodiciCopia", "Copia i codici");
							}),
							FSimpleDelegate::CreateLambda([this]()
							{
								FPlatformApplicationMisc::ClipboardCopy(*FString::Join(Codici, LINE_TERMINATOR));
								bCopiati = true;
							}), 18.f)
					]
					+ SVerticalBox::Slot().AutoHeight() [ Spunta ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(FMargin(0.f, 22.f, 0.f, 0.f)) [ PulsanteFatto ]
				]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
