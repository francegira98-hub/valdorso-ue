// Valdorso - Il contratto da firmare: quello che si vede (vedi il .h). Scritto da Claude il 18/11/2026.

#include "SValdorsoContratto.h"
#include "SValdorsoAccesso.h"
#include "SValdorsoFirma.h"
#include "ValdorsoModoFoto.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistro.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoTemaUI.h"
#include "ValdorsoZaino.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoContratto"

void SValdorsoContratto::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Vista = InArgs._Vista;
	OnChiudi = InArgs._OnChiudi;
	Stile = MakeShared<FValdorsoStileAccesso>();

	// la pergamena del Registro, "a nove pezzi" (i bordi bruciati restano uguali, il centro si allunga); se manca, un
	// foglio liscio dello stesso colore
	TexturePergamena.Reset(ValdorsoCaricaTextureUI(TEXT("/Game/UI/Registro/T_Pergamena.T_Pergamena")));
	if (TexturePergamena.IsValid())
	{
		Pergamena.SetResourceObject(TexturePergamena.Get());
		Pergamena.ImageSize = FVector2D(1024.f, 1280.f);
		Pergamena.DrawAs = ESlateBrushDrawType::Box;
		Pergamena.Margin = FMargin(0.085f, 0.07f);
	}
	else
	{
		Pergamena = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("D9C49A")), 4.f, ValdorsoTema::Hex(TEXT("5A3A1E")), 2.f);
	}
	Velo = FSlateColorBrush(FLinearColor(0.f, 0.f, 0.f, 0.66f));

	const FLinearColor Inchiostro = ValdorsoTema::Inchiostro();
	const FLinearColor Tenue = ValdorsoTema::InchiostroTenue();
	const FString Nome = Vista.Nome.IsEmpty() ? FString(TEXT("Colono senza nome")) : Vista.Nome;
	// (19/11) un foglio solo da leggere (il registro della ronda): spariscono il riquadro della firma e i suoi pulsanti
	const EVisibility PerFirma = Vista.bSoloLettura ? EVisibility::Collapsed : EVisibility::Visible;

	// le clausole
	TSharedRef<SVerticalBox> Clausole = SNew(SVerticalBox);
	for (const FString& Riga : Vista.Righe)
	{
		Clausole->AddSlot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 9.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Riga))
			.Font(Stile->Caratteri->Testo(18.f))
			.ColorAndOpacity(Inchiostro)
			.AutoWrapText(true)
		];
	}

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&Velo)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(760.f)
				[
					SNew(SBorder)
					.BorderImage(&Pergamena)
					.Padding(FMargin(74.f, 62.f, 74.f, 54.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Vista.Intestazione))
							.Font(Stile->Caratteri->Titolo(15.f))
							.ColorAndOpacity(Tenue)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 4.f, 0.f, 0.f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(Vista.Titolo))
							.Font(Stile->Caratteri->Titolo(22.f))
							.ColorAndOpacity(ValdorsoTema::Rubrica())
							.AutoWrapText(true)
							.Justification(ETextJustify::Center)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 10.f, 0.f, 16.f))
						[
							Stile->Separatore(480.f, Tenue)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							Clausole
						]
						// la firma
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 8.f, 0.f, 0.f))
						[
							SAssignNew(Firma, SValdorsoFirma)
							.Firma(FString())
							.Nome(Nome)
							.FontNome(Stile->Caratteri->Calligrafia(40.f))
							.bModificabile(true)
							.Visibility(PerFirma)
							.Grandezza(FVector2D(460.f, 110.f))
							.OnCambia(FValdorsoSuFirma::CreateLambda([this](const FString& Nuova) { FirmaDiAdesso = Nuova; }))
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SBox)
							.Visibility(PerFirma)
							[
								Stile->Separatore(460.f, Tenue)
							]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 3.f, 0.f, 0.f))
						[
							SNew(STextBlock)
							.Text(FText::Format(LOCTEXT("FirmaDi", "Firma di {0}"), FText::FromString(Nome)))
							.Visibility(PerFirma)
							.Font(Stile->Caratteri->Testo(14.f, TEXT("Italic")))
							.ColorAndOpacity(Tenue)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 12.f, 0.f, 0.f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(Vista.Controparte))
							.Font(Stile->Caratteri->Testo(15.f, TEXT("Italic")))
							.ColorAndOpacity(Inchiostro)
							.AutoWrapText(true)
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 16.f, 0.f, 0.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox)
					.Visibility(PerFirma)
					.Padding(FMargin(6.f, 0.f))
					[
						Stile->Pulsante(LOCTEXT("Consegna", "Firma e consegna"), FSimpleDelegate::CreateSP(this, &SValdorsoContratto::Consegna), 20.f,
							// (secondo controllo) si accende solo con una firma che il server accetta: non uno scarabocchio di due punti
							TAttribute<bool>::CreateLambda([this]() { return !FirmaDiAdesso.IsEmpty() && ValdorsoRegistro::ProblemaFirma(FirmaDiAdesso, true).IsEmpty(); }))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox)
					.Visibility(PerFirma)
					.Padding(FMargin(6.f, 0.f))
					[
						Stile->Pulsante(LOCTEXT("ComeRegistro", "Come sul Registro"), FSimpleDelegate::CreateSP(this, &SValdorsoContratto::ComeSulRegistro), 20.f)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox)
					.Visibility(PerFirma)
					.Padding(FMargin(6.f, 0.f))
					[
						Stile->Pulsante(LOCTEXT("Cancella", "Cancella"), FSimpleDelegate::CreateSP(this, &SValdorsoContratto::Cancella), 20.f)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(6.f, 0.f))
				[
					Stile->Pulsante(Vista.bSoloLettura ? LOCTEXT("ChiudiFoglio", "Chiudi") : LOCTEXT("NonFirmo", "Non firmo"), OnChiudi, 20.f)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 10.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(Vista.bSoloLettura
					? LOCTEXT("AiutoLettura", "Esc: chiudi")
					: LOCTEXT("Aiuto", "Firma nel riquadro tenendo premuto il mouse (o con lo stick, tenendo A)  ·  Esc: non firmo"))
				.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::TestoSecondario())
			]
		]
	];
}

TSharedPtr<SWidget> SValdorsoContratto::RiquadroFirma() const
{
	return Firma;
}

void SValdorsoContratto::Consegna()
{
	if (Vista.bSoloLettura)
	{
		// un foglio da leggere: Invio lo chiude
		OnChiudi.ExecuteIfBound();
		return;
	}
	if (FirmaDiAdesso.IsEmpty() || !ValdorsoRegistro::ProblemaFirma(FirmaDiAdesso, true).IsEmpty())
	{
		return;
	}
	if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore.Get()))
	{
		Zaino->ServerFirmaContratto(Vista.Tipo, FirmaDiAdesso);
	}
	OnChiudi.ExecuteIfBound();
}

void SValdorsoContratto::ComeSulRegistro()
{
	// la firma del Registro; chi non ne ha una disegnata firma con il nome ("@", in calligrafia)
	FirmaDiAdesso = Vista.FirmaRegistro.IsEmpty() ? FString(TEXT("@")) : Vista.FirmaRegistro;
	if (Firma.IsValid())
	{
		Firma->ImpostaFirma(FirmaDiAdesso);
	}
}

void SValdorsoContratto::Cancella()
{
	FirmaDiAdesso.Reset();
	if (Firma.IsValid())
	{
		Firma->ImpostaFirma(FString());
	}
}

FReply SValdorsoContratto::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::Gamepad_FaceButton_Right || Tasto == EKeys::Gamepad_Special_Left)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	if (Tasto == EKeys::Enter || Tasto == EKeys::Gamepad_Special_Right)
	{
		Consegna();
		return FReply::Handled();
	}
	// (secondo controllo) gli altri tasti come nella tessera: la croce del controller e Tab spostano il fuoco sui pulsanti
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoContratto::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// i clic fuori dal foglio non passano al gioco sotto; il fuoco torna al contratto (per Esc)
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

// ------------------------------------------------------------------------------------------------- aprire e chiudere

namespace ValdorsoContrattoSchermo
{
	bool Aperto(const APlayerController* Controllore)
	{
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		return Zaino && Zaino->SchermataContratto.IsValid();
	}

	bool Apri(APlayerController* Controllore, const FValdorsoContrattoVista& Vista)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		if (!Zaino || !Viewport || !Controllore->IsLocalController())
		{
			return true;  // qui non si puo' proprio aprire: inutile riprovare
		}
		if (Zaino->SchermataContratto.IsValid())
		{
			Chiudi(Controllore);
		}
		const AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore);
		if ((Nostro && Nostro->SchermataAperta()) || ValdorsoFoto::Aperta(Controllore) || ValdorsoTaccuino::Aperto(Controllore))
		{
			return false;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		SAssignNew(Zaino->SchermataContratto, SValdorsoContratto)
			.Controllore(Debole)
			.Vista(Vista)
			.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		// come la tessera (57): sopra le scritte del gioco, sotto la pausa
		Viewport->AddViewportWidgetContent(Zaino->SchermataContratto.ToSharedRef(), 57);
		FInputModeUIOnly Modo;
		// (19/11) il foglio solo da leggere non ha il riquadro della firma: il fuoco va al foglio (per Esc)
		if (Vista.bSoloLettura)
		{
			Modo.SetWidgetToFocus(Zaino->SchermataContratto);
		}
		else
		{
			Modo.SetWidgetToFocus(Zaino->SchermataContratto->RiquadroFirma());
		}
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
		return true;
	}

	void Chiudi(APlayerController* Controllore)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		if (!Zaino || !Zaino->SchermataContratto.IsValid())
		{
			return;
		}
		const TSharedRef<SValdorsoContratto> Schermo = Zaino->SchermataContratto.ToSharedRef();
		Zaino->SchermataContratto.Reset();
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
