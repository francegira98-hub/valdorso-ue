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
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
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
#include "Valdorso.h"
#include "ValdorsoSuoni.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif

#define LOCTEXT_NAMESPACE "ValdorsoRegistroSchermo"

namespace
{
	/** Quanto dura il nero dopo la firma, prima che il personaggio entri. */
	constexpr float RegSchermoSecondiFinale = 5.5f;

	/** Il sigillo cade nei primi 0,7 secondi; poi la foto della pagina, poi il nero (passo 4.2b). */
	constexpr float RegSchermoSigillo = 0.7f;
	constexpr float RegSchermoInizioNero = 1.1f;

	/** La calligrafia del sacerdote: lettere al secondo. */
	constexpr float RegSchermoLettereAlSecondo = 70.f;

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

	/** Il colore dell'elemento di ogni fede (luce, fuoco, acqua, terra, aria; i Vecchi Dei verdi del bosco). */
	FLinearColor RegSchermoColoreFede(const FString& Chiave)
	{
		// (05/10) Più scuri dove serve, perché si vedano sulla pergamena.
		if (Chiave == TEXT("solara")) { return ValdorsoTema::Hex(TEXT("B8860B")); }
		if (Chiave == TEXT("ignar")) { return ValdorsoTema::Brace(); }
		if (Chiave == TEXT("nereia")) { return ValdorsoTema::BluArcano(); }
		if (Chiave == TEXT("torvald")) { return ValdorsoTema::Hex(TEXT("8B6B3E")); }
		if (Chiave == TEXT("zefira")) { return ValdorsoTema::Hex(TEXT("4F7F99")); }
		if (Chiave == TEXT("vecchidei")) { return ValdorsoTema::VerdeVita(); }
		return ValdorsoTema::InchiostroTenue().CopyWithNewOpacity(0.5f);
	}

	/**
	 * (05/10) Carica una texture dell'interfaccia e, nell'editor, aspetta che sia pronta: se Slate la chiede mentre
	 * l'editor la sta ancora preparando "in sottofondo", non la disegna (era il motivo della pergamena invisibile).
	 */
	UTexture2D* RegSchermoCaricaTexture(const TCHAR* Percorso)
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
#if WITH_EDITOR
		if (Texture)
		{
			UTexture* DaPreparare = Texture;
			FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&DaPreparare, 1));
		}
#endif
		return Texture;
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
	OnCambia = InArgs._OnCambia;
	Nome = InArgs._NomePersonaggio;

	// Il ritratto del palco (passo 4.2b): la schermata lo tiene vivo finché si vede.
	if (InArgs._Ritratto)
	{
		RitrattoVivo.Reset(InArgs._Ritratto);
		PennelloRitratto.SetResourceObject(InArgs._Ritratto);
		PennelloRitratto.ImageSize = FVector2D(InArgs._Ritratto->SizeX, InArgs._Ritratto->SizeY);
		PennelloRitratto.DrawAs = ESlateBrushDrawType::Image;
	}
	// Il sigillo: un disco rosso sangue con il bordo più scuro (disegnati come rettangoli tutti arrotondati).
	PennelloSigillo = FSlateRoundedBoxBrush(ValdorsoTema::RossoSangue(), 46.f, ValdorsoTema::Hex(TEXT("5C1A16")), 4.f);
	PennelloSigilloBordo = FSlateRoundedBoxBrush(FLinearColor::Transparent, 34.f, ValdorsoTema::Oro().CopyWithNewOpacity(0.55f), 1.5f);
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

	// (05/10) Le risposte sulla pergamena: un filo d'inchiostro e un velo appena scuro; quella scelta in rosso di rubrica.
	const FLinearColor Inchiostro = ValdorsoTema::Inchiostro();
	const FLinearColor Rubrica = ValdorsoTema::Rubrica();
	StileVoce = Stile->StilePulsante;
	StileVoce
		.SetNormal(FSlateRoundedBoxBrush(Inchiostro.CopyWithNewOpacity(0.04f), 3.f, ValdorsoTema::InchiostroTenue().CopyWithNewOpacity(0.45f), 1.f))
		.SetHovered(FSlateRoundedBoxBrush(Inchiostro.CopyWithNewOpacity(0.10f), 3.f, Inchiostro.CopyWithNewOpacity(0.75f), 1.f))
		.SetPressed(FSlateRoundedBoxBrush(Inchiostro.CopyWithNewOpacity(0.16f), 3.f, Inchiostro, 1.f))
		.SetDisabled(FSlateRoundedBoxBrush(Inchiostro.CopyWithNewOpacity(0.02f), 3.f, ValdorsoTema::InchiostroTenue().CopyWithNewOpacity(0.2f), 1.f))
		.SetNormalPadding(FMargin(16.f, 7.f)).SetPressedPadding(FMargin(16.f, 8.f, 16.f, 6.f));
	StileVoceScelta = StileVoce;
	StileVoceScelta
		.SetNormal(FSlateRoundedBoxBrush(Rubrica.CopyWithNewOpacity(0.10f), 3.f, Rubrica, 1.5f))
		.SetHovered(FSlateRoundedBoxBrush(Rubrica.CopyWithNewOpacity(0.16f), 3.f, Rubrica, 1.5f));

	// La pergamena: disegnata "a nove pezzi", così i bordi bruciati restano uguali e il centro si allunga con la pagina.
	// Se la texture non è ancora importata (Content/Python/importa_registro.py), un foglio liscio dello stesso colore.
	TexturePergamena.Reset(RegSchermoCaricaTexture(TEXT("/Game/UI/Registro/T_Pergamena.T_Pergamena")));
	if (TexturePergamena.IsValid())
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Registro: pergamena %dx%d, pronta: %s"),
			TexturePergamena->GetSizeX(), TexturePergamena->GetSizeY(), TexturePergamena->GetResource() ? TEXT("sì") : TEXT("no"));
		PennelloPergamena.SetResourceObject(TexturePergamena.Get());
		PennelloPergamena.ImageSize = FVector2D(1024.f, 1280.f);
		PennelloPergamena.DrawAs = ESlateBrushDrawType::Box;
		PennelloPergamena.Margin = FMargin(0.085f, 0.07f);
	}
	else
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Registro: T_Pergamena non trovata in Content/UI/Registro (lanciare importa_registro.py)"));
		PennelloPergamena = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("D9C49A")), 4.f, ValdorsoTema::Hex(TEXT("5A3A1E")), 2.f);
	}

	// Il riquadro del capolettera: oro scuro su un velo di rosso.
	PennelloCapolettera = FSlateRoundedBoxBrush(ValdorsoTema::Rubrica().CopyWithNewOpacity(0.07f), 3.f, ValdorsoTema::OroScuro(), 1.5f);

	// I campi di testo (racconto e storia): niente fondo scuro, un filo d'inchiostro che si scurisce quando si scrive.
	StileCampoPergamena = Stile->StileCampo;
	StileCampoPergamena
		.SetBackgroundImageNormal(FSlateRoundedBoxBrush(Inchiostro.CopyWithNewOpacity(0.03f), 3.f, ValdorsoTema::InchiostroTenue().CopyWithNewOpacity(0.4f), 1.f))
		.SetBackgroundImageHovered(FSlateRoundedBoxBrush(Inchiostro.CopyWithNewOpacity(0.05f), 3.f, Inchiostro.CopyWithNewOpacity(0.6f), 1.f))
		.SetBackgroundImageFocused(FSlateRoundedBoxBrush(Inchiostro.CopyWithNewOpacity(0.05f), 3.f, Rubrica.CopyWithNewOpacity(0.8f), 1.f))
		.SetBackgroundImageReadOnly(FSlateRoundedBoxBrush(Inchiostro.CopyWithNewOpacity(0.03f), 3.f, ValdorsoTema::InchiostroTenue().CopyWithNewOpacity(0.4f), 1.f))
		.SetForegroundColor(FSlateColor(Inchiostro))
		.SetFocusedForegroundColor(FSlateColor(Inchiostro))
		.SetReadOnlyForegroundColor(FSlateColor(Inchiostro));

	// (05/10) L'inchiostro che si asciuga: mentre il sacerdote scrive è fresco, quasi nero con un riflesso blu;
	// finita la scrittura passa a "mezzo asciutto" e dopo un attimo al bruno dell'inchiostro secco.
	auto ConInchiostro = [this](const FLinearColor& Colore)
	{
		FEditableTextBoxStyle Nuovo = StileCampoPergamena;
		Nuovo.SetForegroundColor(FSlateColor(Colore)).SetFocusedForegroundColor(FSlateColor(Colore)).SetReadOnlyForegroundColor(FSlateColor(Colore));
		return Nuovo;
	};
	StileCampoFresco = ConInchiostro(ValdorsoTema::Hex(TEXT("15111C")));
	StileCampoMezzo = ConInchiostro(ValdorsoTema::Hex(TEXT("221610")));

	// (05/10) I pulsanti del Registro diventano linguette di cuoio cucite (solo qui: questo Stile è del Registro).
	TextureCuoio.Reset(RegSchermoCaricaTexture(TEXT("/Game/UI/Registro/T_Cuoio.T_Cuoio")));
	if (TextureCuoio.IsValid())
	{
		auto Cuoio = [this](const FLinearColor& Tinta)
		{
			FSlateBrush Pennello;
			Pennello.SetResourceObject(TextureCuoio.Get());
			Pennello.ImageSize = FVector2D(256.f, 80.f);
			Pennello.DrawAs = ESlateBrushDrawType::Box;
			Pennello.Margin = FMargin(0.1f, 0.22f);
			Pennello.TintColor = FSlateColor(Tinta);
			return Pennello;
		};
		Stile->StilePulsante
			.SetNormal(Cuoio(FLinearColor(0.88f, 0.88f, 0.88f, 1.f)))
			.SetHovered(Cuoio(FLinearColor::White))
			.SetPressed(Cuoio(FLinearColor(0.68f, 0.68f, 0.68f, 1.f)))
			.SetDisabled(Cuoio(FLinearColor(0.75f, 0.75f, 0.75f, 0.45f)))
			.SetNormalPadding(FMargin(24.f, 10.f))
			.SetPressedPadding(FMargin(24.f, 11.f, 24.f, 9.f));
	}

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
					// La pagina di adesso batte con il Cuore, come il frammento.
					const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
					return FSlateColor(FMath::Lerp(ValdorsoTema::Rubrica(), ValdorsoTema::Brace(), 0.2f + 0.6f * Colpo));
				}
				return FSlateColor(PaginaCompleta(Questa) ? ValdorsoTema::OroScuro() : ValdorsoTema::InchiostroTenue().CopyWithNewOpacity(0.3f));
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
	BottoneProssimo = Prossimo;
	// (05/10) "Firma il registro" in rosso ceralacca, come il sigillo che cadrà: è il gesto più importante della pagina.
	StileFirma = Stile->StilePulsante;
	StileFirma
		.SetNormal(FSlateRoundedBoxBrush(ValdorsoTema::RossoSangue(), 3.f, ValdorsoTema::Oro().CopyWithNewOpacity(0.6f), 1.f))
		.SetHovered(FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("A8362E")), 3.f, ValdorsoTema::OroChiaro(), 1.f))
		.SetPressed(FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("6E201B")), 3.f, ValdorsoTema::Oro(), 1.f))
		.SetDisabled(FSlateRoundedBoxBrush(ValdorsoTema::RossoSangue().CopyWithNewOpacity(0.45f), 3.f, ValdorsoTema::Oro().CopyWithNewOpacity(0.15f), 1.f));

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
				.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
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
				SNew(SOverlay)
				// (05/10) L'ombra della pergamena: la stessa forma, nera e spostata, così il foglio "si stacca" dal fondo.
				// (Nel SBox a misura zero: l'ombra non cambia la grandezza della pagina.)
				+ SOverlay::Slot()
				[
					SNew(SBox).WidthOverride(0.f).HeightOverride(0.f)
					[
						SNew(SImage)
						.Image(&PennelloPergamena)
						.ColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f))
						.Visibility(EVisibility::HitTestInvisible)
						.RenderTransform(FSlateRenderTransform(FVector2f(9.f, 12.f)))
					]
				]
				+ SOverlay::Slot()
				[
				SNew(SBorder)
				.BorderImage(&PennelloPergamena)
				// (05/10) Più margine: i bordi bruciati della pergamena non devono toccare le scritte.
				.Padding(FMargin(76.f, 64.f))
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
								Stile->Etichetta(LOCTEXT("Intestazione", "IL REGISTRO DI VAL D'ORSO"), ValdorsoTema::Rubrica())
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
								.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
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
								return FSlateColor(FMath::Lerp(ValdorsoTema::Rubrica(), ValdorsoTema::Hex(TEXT("B0402F")), Colpo * 0.5f));
							})
						]

						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 6.f, 0.f, 10.f))
						[
							Stile->Separatore(300.f, ValdorsoTema::InchiostroTenue())
						]

						// La frase della storia che apre la pagina.
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 14.f))
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return FrasePagina(Pagina); })
							.Font(Stile->Caratteri->Testo(19.f, TEXT("Italic")))
							.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
							.Justification(ETextJustify::Center)
							.AutoWrapText(true)
						]

						// La domanda.
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 12.f))
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return TitoloPagina(Pagina); })
							.Font(Stile->Caratteri->Testo(24.f, TEXT("SemiBold")))
							.ColorAndOpacity(ValdorsoTema::Inchiostro())
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
									return FSlateColor(FMath::Lerp(ValdorsoTema::Rubrica(), ValdorsoTema::Brace(), Colpo));
								}
								return FSlateColor(bErrore ? ValdorsoTema::Rubrica() : ValdorsoTema::Inchiostro());
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
							// (Dentro un SBox: quando i dadi spariscono lo spazio resta, e Firma resta a destra.)
							+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(SBox)
								[
									DadiPagina
								]
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

				// Il sigillo di ceralacca: alla firma cade sulla pergamena (passo 4.2b).
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0.f, 0.f, 36.f, 92.f))
				[
					Sigillo()
				]
			]

			// A destra il ritratto del colono, disegnato dalla telecamera del palco (passo 4.2b);
			// senza palco, il frammento che batte.
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				Destra()
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
				const float Passati = static_cast<float>(FPlatformTime::Seconds() - InizioFinale) - RegSchermoInizioNero;
				return FSlateColor(FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(Passati / 1.2f, 0.f, 1.f)));
			})
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(80.f, 150.f, 80.f, 0.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FinaleFoto", "La pagina firmata è salvata tra gli screenshot del gioco."))
			.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
			.Visibility_Lambda([this]() { return bFinale ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this]()
			{
				const float Passati = static_cast<float>(FPlatformTime::Seconds() - InizioFinale) - RegSchermoInizioNero;
				return FSlateColor(ValdorsoTema::TestoSecondario().CopyWithNewOpacity(FMath::Clamp((Passati - 2.0f) / 1.0f, 0.f, 0.8f)));
			})
		]
		// (05/10) Fill e non Center (il testo è già centrato): con Center la frase finale andava a capo a ogni parola.
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).Padding(FMargin(80.f, 0.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Finale", "Il frammento batte più forte, per un istante. La valle ti ha sentito."))
			.Font(Stile->Caratteri->Testo(30.f, TEXT("Italic")))
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
			.Visibility_Lambda([this]() { return bFinale ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.ColorAndOpacity_Lambda([this]()
			{
				const float Passati = static_cast<float>(FPlatformTime::Seconds() - InizioFinale) - RegSchermoInizioNero;
				const float Luce = FMath::Clamp((Passati - 1.0f) / 1.0f, 0.f, 1.f);
				const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
				return FSlateColor(FMath::Lerp(ValdorsoTema::Pergamena(), ValdorsoTema::OroChiaro(), Colpo * 0.6f).CopyWithNewOpacity(Luce));
			})
		]
	];

	// (05/10) I suoni del palco: il fuoco dei bracieri e il battito del Cuore, a tempo con il rombo che pulsa
	// (il battito di ValdorsoTema dura 1,6 secondi: si parte dal punto giusto del giro).
	SuonoFuoco.Reset(ValdorsoSuoni::Suona(TEXT("S_Fuoco"), 0.35f));
	SuonoBattito.Reset(ValdorsoSuoni::Suona(TEXT("S_Battito"), 0.55f, static_cast<float>(FMath::Fmod(FPlatformTime::Seconds(), 1.6))));

	// Si riparte dalla prima domanda senza risposta (o dal racconto, se ci sono tutte).
	VaiA(RegSchermoPaginaDellaDomanda(ValdorsoRegistro::PrimaMancante(Bozza)));
}

SValdorsoRegistroColono::~SValdorsoRegistroColono()
{
	ValdorsoSuoni::Sfuma(SuonoFuoco.Get(), 0.8f);
	ValdorsoSuoni::Sfuma(SuonoBattito.Get(), 0.8f);
	ValdorsoSuoni::Sfuma(SuonoPennino.Get(), 0.2f);
	ValdorsoSuoni::Sfuma(SuonoMotivo.Get(), 0.8f);
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
	FinisciScrittura();
	PrimoFuoco.Reset();
	CampoTesto.Reset();
	if (BottoneProssimo.IsValid())
	{
		BottoneProssimo->SetButtonStyle(Pagina == EPagina::Firma ? &StileFirma : &Stile->StilePulsante);
	}
	Corpo->SetContent(ContenutoPagina());
	OnCambia.ExecuteIfBound(Bozza);
	if (Pagina == EPagina::Racconto && !bRaccontoScritto && !bRaccontoToccato)
	{
		IniziaScrittura();
	}

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

TSharedRef<SButton> SValdorsoRegistroColono::PulsanteVoce(const FText& Scritta, TFunction<bool()> Scelta, TFunction<void()> Azione, float Dimensione,
	const FText& Aiuto, FLinearColor Simbolo)
{
	const bool bConSimbolo = Simbolo.A > 0.f;
	// La risposta scelta ha il bordo d'oro pieno: a ogni scelta la pagina si ridisegna (e il fuoco torna sulla scelta).
	return SNew(SButton)
		.ButtonStyle(Scelta() ? &StileVoceScelta : &StileVoce)
		.IsFocusable(true)
		// (05/10, segnalato da Fra) Fill e non Left: con Left il testo che va a capo si stringeva da solo fino alla
		// parola più lunga ("La / solitudine" su due righe). Il testo resta allineato a sinistra.
		.HAlign(HAlign_Fill)
		.ToolTipText(Aiuto)
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
				Stile->Diamante(bConSimbolo ? 11.f : 8.f, TAttribute<FSlateColor>::CreateLambda([Scelta, bConSimbolo, Simbolo]()
				{
					if (bConSimbolo)
					{
						return FSlateColor(Scelta() ? Simbolo : Simbolo.CopyWithNewOpacity(Simbolo.A * 0.45f));
					}
					return FSlateColor(Scelta() ? ValdorsoTema::Rubrica() : ValdorsoTema::InchiostroTenue().CopyWithNewOpacity(0.35f));
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
					return FSlateColor(Scelta() ? ValdorsoTema::Rubrica() : ValdorsoTema::Inchiostro());
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
		const FString PortaVoce = ValdorsoRegistro::Vantaggio(Quale, Chiave);
		TSharedRef<SButton> Voce = PulsanteVoce(Scritta,
			[this, Quale, Chiave]() { return ValdorsoRegistro::Risposta(Bozza, Quale) == Chiave; },
			[this, Quale, Chiave]()
			{
				ValdorsoRegistro::Risposta(Bozza, Quale) = Chiave;
				// (05/10) Scegliendo una fede si sente il suo motivo.
				if (Quale == ValdorsoRegistro::EDomanda::Fede)
				{
					ValdorsoSuoni::Sfuma(SuonoMotivo.Get(), 0.3f);
					SuonoMotivo.Reset(ValdorsoSuoni::SuonaFede(Chiave, 0.7f));
				}
			},
			bCompatte ? 20.f : 21.f,
			FText::FromString(PortaVoce),
			Quale == ValdorsoRegistro::EDomanda::Fede ? RegSchermoColoreFede(Chiave) : FLinearColor::Transparent);

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
	// Sotto le risposte: cosa porterà nella valle quella scelta (passando sopra le altre, lo dice il suggerimento).
	TSharedRef<SWidget> Porta = SNew(STextBlock)
		.Text_Lambda([this, Quale]()
		{
			const FString Riga = ValdorsoRegistro::Vantaggio(Quale, ValdorsoRegistro::Risposta(Bozza, Quale));
			return Riga.IsEmpty() ? FText::GetEmpty() : FText::Format(LOCTEXT("NellaValle", "Nella valle: {0}"), FText::FromString(Riga));
		})
		.Visibility_Lambda([this, Quale]()
		{
			return FCString::Strlen(ValdorsoRegistro::Vantaggio(Quale, ValdorsoRegistro::Risposta(Bozza, Quale))) > 0
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
		.ColorAndOpacity(ValdorsoTema::OroScuro())
		.AutoWrapText(true);

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			bDueColonne ? StaticCastSharedRef<SWidget>(Griglia) : StaticCastSharedRef<SWidget>(Colonna)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(4.f, 6.f, 4.f, 0.f))
		[
			Porta
		];
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
			.ColorAndOpacity(ValdorsoTema::Inchiostro())
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
				.ColorAndOpacity(ValdorsoTema::Rubrica())
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
			.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
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
			.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
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

	// (05/10) Il racconto nella mano del sacerdote (Tangerine); la tua storia con le tue lettere (EB Garamond).
	TSharedRef<SMultiLineEditableTextBox> Campo = SNew(SMultiLineEditableTextBox)
		.Style(&StileCampoPergamena)
		.Font(bRacconto ? Stile->Caratteri->Calligrafia(34.f) : Stile->Caratteri->Testo(20.f))
		.AutoWrapText(true)
		.Text(FText::FromString(bRacconto ? Bozza.Racconto : Bozza.Storia))
		.HintText(bRacconto ? FText::GetEmpty() : LOCTEXT("SugStoria", "Mio padre forgiava spade a Torre Grigia; io ho imparato a tacere..."))
		.IsEnabled_Lambda([this]() { return !bInFirma && !bFinale && !bInUscita; })
		.IsReadOnly_Lambda([this]() { return bScrivendo; })
		.OnTextChanged_Lambda([this, bRacconto](const FText& Nuovo)
		{
			if (bScrivendo)
			{
				// Lo sta scrivendo il sacerdote, non il giocatore.
				return;
			}
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
	// (05/10) Fill e non Right (il testo è già allineato a destra): con Right "2.974 / 3.000" andava a capo da solo.
	Sotto->AddSlot().FillWidth(1.f).HAlign(HAlign_Fill).VAlign(VAlign_Center)
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
			return FSlateColor(PaginaCompleta(bRacconto ? EPagina::Racconto : EPagina::Storia) ? ValdorsoTema::InchiostroTenue() : ValdorsoTema::Rubrica());
		})
	];

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 10.f))
		[
			SNew(STextBlock)
			.Text(Spiegazione)
			.Font(Stile->Caratteri->Testo(18.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
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
						.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Text(bManca ? LOCTEXT("Manca", "manca: tocca qui per rispondere") : FText::FromString(Valore))
					.Font(Stile->Caratteri->Testo(18.f))
					.AutoWrapText(true)
					.ColorAndOpacity(bManca ? ValdorsoTema::Rubrica() : ValdorsoTema::Inchiostro())
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
		FString Tratto = Scelta(Coppia);
		if (!Tratto.IsEmpty())
		{
			// Maiuscola solo la prima: "Onesto, coraggioso, scettico, gentile".
			if (Tratti.Num() > 0)
			{
				Tratto[0] = FChar::ToLower(Tratto[0]);
			}
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
		// Il racconto del sacerdote, da rileggere prima di firmare (un clic riporta alla sua pagina).
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 12.f, 0.f, 8.f))
		[
			Stile->Separatore(200.f, ValdorsoTema::InchiostroTenue())
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
			.ToolTipText(LOCTEXT("RitoccaRacconto", "Torna alla pagina del racconto per ritoccarlo."))
			.IsEnabled_Lambda([this]() { return !bInFirma && !bFinale && !bInUscita; })
			.OnClicked_Lambda([this]() { VaiA(EPagina::Racconto); return FReply::Handled(); })
			[
				Capolettera(PerLaFirma().Racconto)
			]
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
				return FSlateColor(ValdorsoRegistro::Problema(PerLaFirma(), true).IsEmpty() ? ValdorsoTema::Inchiostro() : ValdorsoTema::Rubrica());
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
	FinisciScrittura();
	RegisterActiveTimer(RegSchermoSecondiFinale, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoRegistroColono::FineFinale));
	// (05/10) La campana del tempio; il sigillo tocca la carta con la ceralacca; il fuoco si quieta e il Cuore
	// batte più forte ("Il frammento batte più forte, per un istante").
	SuonoCampana.Reset(ValdorsoSuoni::Suona(TEXT("S_Campana"), 0.9f));
	RegisterActiveTimer(RegSchermoSigillo * 0.85f, FWidgetActiveTimerDelegate::CreateLambda([this](double, float)
	{
		SuonoCeralacca.Reset(ValdorsoSuoni::Suona(TEXT("S_Ceralacca"), 0.9f));
		return EActiveTimerReturnType::Stop;
	}));
	if (SuonoBattito.IsValid())
	{
		// (AdjustVolume moltiplica il volume di partenza, 0,55: 1/0,55 lo porta a 1.)
		SuonoBattito->AdjustVolume(1.5f, 1.f / 0.55f);
	}
	if (SuonoFuoco.IsValid())
	{
		SuonoFuoco->AdjustVolume(2.f, 0.12f / 0.35f);
	}
	// Quando il sigillo è caduto, la foto della pagina firmata.
	RegisterActiveTimer(RegSchermoSigillo + 0.1f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoRegistroColono::FotoPagina));
}

EActiveTimerReturnType SValdorsoRegistroColono::FineFinale(double Ora, float Delta)
{
	OnFirmato.ExecuteIfBound();
	return EActiveTimerReturnType::Stop;
}

void SValdorsoRegistroColono::MostraMessaggio(const FText& Scritta, bool bComeErrore)
{
	bMessaggioScrittura = false;
	Messaggio = Scritta;
	bErrore = bComeErrore;
}

// ------------------------------------------------------------------------------------------------
// Passo 4.2b: ritratto, sigillo, calligrafia, foto della pagina
// ------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SValdorsoRegistroColono::Capolettera(const FString& Racconto)
{
	// (05/10) Il capolettera, come nei manoscritti: la prima lettera grande, rossa, in un riquadro d'oro;
	// il resto del racconto accanto, nella mano del sacerdote.
	const FString Pulito = Racconto.TrimStart();
	if (Pulito.IsEmpty())
	{
		return SNullWidget::NullWidget;
	}
	FSlateFontInfo FontLettera = Stile->Caratteri->Titolo(44.f, TEXT("Black"));
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(FMargin(0.f, 6.f, 12.f, 0.f))
		[
			SNew(SBorder)
			.BorderImage(&PennelloCapolettera)
			.Padding(FMargin(0.f))
			[
				// Almeno 72x72, ma cresce con la lettera: Cinzel Black a 44 punti è alta circa 80 (controllo automatico).
				SNew(SBox).MinDesiredWidth(72.f).MinDesiredHeight(72.f).Padding(FMargin(10.f, 0.f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Pulito.Left(1).ToUpper()))
					.Font(FontLettera)
					.ColorAndOpacity(ValdorsoTema::Rubrica())
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Pulito.Mid(1)))
			.Font(Stile->Caratteri->Calligrafia(30.f))
			.ColorAndOpacity(ValdorsoTema::Inchiostro())
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SValdorsoRegistroColono::Destra()
{
	if (!RitrattoVivo.IsValid())
	{
		return SNew(SVerticalBox)
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
			];
	}

	// Il ritratto in una cornice sottile d'oro, con l'iscrizione sotto.
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Stile->SfondoPannello)
			.Padding(FMargin(6.f))
			[
				SNew(SBox).WidthOverride(560.f).HeightOverride(700.f)
				[
					SNew(SImage).Image(&PennelloRitratto)
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 18.f, 0.f, 0.f))
		[
			Stile->Etichetta(LOCTEXT("CuoreRitratto", "IL CUORE BATTE ANCORA"))
		];
}

TSharedRef<SWidget> SValdorsoRegistroColono::Sigillo()
{
	FSlateFontInfo FontV = Stile->Caratteri->Titolo(40.f, TEXT("Black"));

	// Cade dall'alto: grande e trasparente, poi giusto e pieno, con un piccolo rimbalzo.
	auto Caduta = [this]()
	{
		const float Passati = static_cast<float>(FPlatformTime::Seconds() - InizioFinale);
		return FMath::Clamp(Passati / RegSchermoSigillo, 0.f, 1.f);
	};

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.Padding(FMargin(0.f))
		.Visibility_Lambda([this]() { return bFinale ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		.RenderTransformPivot(FVector2D(0.5f, 0.5f))
		.RenderTransform_Lambda([Caduta]()
		{
			const float A = Caduta();
			const float Rimbalzo = A < 0.75f ? FMath::Lerp(2.2f, 0.92f, A / 0.75f) : FMath::Lerp(0.92f, 1.f, (A - 0.75f) / 0.25f);
			const FSlateRenderTransform Scala{ TScale2<float>(Rimbalzo) };
			const FSlateRenderTransform Giro{ FQuat2D(FMath::DegreesToRadians(-12.f)) };
			return TOptional<FSlateRenderTransform>(Scala.Concatenate(Giro));
		})
		.ColorAndOpacity_Lambda([Caduta]()
		{
			return FLinearColor(1.f, 1.f, 1.f, FMath::Clamp(Caduta() * 1.6f, 0.f, 1.f));
		})
		[
			SNew(SBox)
			.WidthOverride(92.f)
			.HeightOverride(92.f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage).Image(&PennelloSigillo)
				]
				+ SOverlay::Slot().Padding(FMargin(12.f))
				[
					SNew(SImage).Image(&PennelloSigilloBordo)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SigilloV", "V"))
					.Font(FontV)
					.ColorAndOpacity(ValdorsoTema::Oro())
				]
			]
		];
}

void SValdorsoRegistroColono::IniziaScrittura()
{
	if (!CampoTesto.IsValid() || Bozza.Racconto.IsEmpty())
	{
		return;
	}
	bScrivendo = true;
	Scritte = 0.f;
	CampoTesto->SetStyle(&StileCampoFresco);
	// (05/10) Il pennino che gratta finché il sacerdote scrive.
	ValdorsoSuoni::Sfuma(SuonoPennino.Get(), 0.1f);
	SuonoPennino.Reset(ValdorsoSuoni::Suona(TEXT("S_Pennino"), 0.5f));
	CampoTesto->SetText(FText::GetEmpty());
	MostraMessaggio(LOCTEXT("Scrivendo", "Il sacerdote scrive... (un tasto o un clic per leggere subito)"), false);
	bMessaggioScrittura = true;
	TimerScrittura = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoRegistroColono::Scrivi));
}

EActiveTimerReturnType SValdorsoRegistroColono::Scrivi(double Ora, float Delta)
{
	if (!bScrivendo || !CampoTesto.IsValid() || Pagina != EPagina::Racconto)
	{
		TimerScrittura.Reset();
		FinisciScrittura();
		return EActiveTimerReturnType::Stop;
	}
	Scritte += Delta * RegSchermoLettereAlSecondo;
	const int32 Quante = FMath::Min(FMath::FloorToInt32(Scritte), Bozza.Racconto.Len());
	CampoTesto->SetText(FText::FromString(Bozza.Racconto.Left(Quante)));
	// Le lettere nuove restano in vista anche quando il racconto è più lungo della casella.
	CampoTesto->GoTo(ETextLocation::EndOfDocument);
	if (Quante >= Bozza.Racconto.Len())
	{
		TimerScrittura.Reset();
		FinisciScrittura();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

void SValdorsoRegistroColono::FinisciScrittura()
{
	if (TimerScrittura.IsValid())
	{
		UnRegisterActiveTimer(TimerScrittura.ToSharedRef());
		TimerScrittura.Reset();
	}
	if (SuonoPennino.IsValid())
	{
		ValdorsoSuoni::Sfuma(SuonoPennino.Get(), 0.15f);
		SuonoPennino.Reset();
	}
	if (!bScrivendo)
	{
		return;
	}
	bScrivendo = false;
	bRaccontoScritto = true;
	if (CampoTesto.IsValid())
	{
		CampoTesto->SetText(FText::FromString(Bozza.Racconto));
		// L'inchiostro si asciuga: mezzo asciutto subito, secco dopo poco meno di un secondo.
		CampoTesto->SetStyle(&StileCampoMezzo);
		TWeakPtr<SMultiLineEditableTextBox> Debole = CampoTesto;
		RegisterActiveTimer(0.9f, FWidgetActiveTimerDelegate::CreateLambda([this, Debole](double, float)
		{
			if (const TSharedPtr<SMultiLineEditableTextBox> Campo = Debole.Pin())
			{
				Campo->SetStyle(&StileCampoPergamena);
			}
			return EActiveTimerReturnType::Stop;
		}));
	}
	// SetText può far credere che il giocatore abbia ritoccato il racconto: non è così.
	bRaccontoToccato = false;
	if (bMessaggioScrittura)
	{
		Messaggio = FText::GetEmpty();
		bMessaggioScrittura = false;
	}
}

EActiveTimerReturnType SValdorsoRegistroColono::FotoPagina(double Ora, float Delta)
{
	// Saved/Screenshots/...: Registro_<nome>_<data>.png, con la pagina, il sigillo e il ritratto.
	const FString NomeFile = FString::Printf(TEXT("Registro_%s_%s.png"),
		*FPaths::MakeValidFileName(Nome.Replace(TEXT(" "), TEXT("_")).Replace(TEXT("'"), TEXT(""))),
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir() / NomeFile, true, false);
	return EActiveTimerReturnType::Stop;
}

FReply SValdorsoRegistroColono::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (bScrivendo)
	{
		FinisciScrittura();
		return FReply::Handled();
	}
	return SCompoundWidget::OnPreviewKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoRegistroColono::OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bScrivendo)
	{
		FinisciScrittura();
		return FReply::Handled();
	}
	return SCompoundWidget::OnPreviewMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SValdorsoRegistroColono::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bScrivendo)
	{
		FinisciScrittura();
	}
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
	if (bScrivendo)
	{
		// Un tasto qualsiasi: il sacerdote finisce subito di scrivere.
		FinisciScrittura();
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
