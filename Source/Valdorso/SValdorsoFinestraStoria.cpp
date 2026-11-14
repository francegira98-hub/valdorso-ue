// Valdorso - La finestra delle storie del Battito (vedi il .h). Scritto da Claude il 14/11/2026.

#include "SValdorsoFinestraStoria.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoQualita.h"
#include "ValdorsoSuoni.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoFinestraStoria"

namespace
{
	/** Le finestre aperte, una per PC (di solito c'e' un solo giocatore). */
	TMap<TWeakObjectPtr<const APlayerController>, TSharedPtr<SValdorsoFinestraStoria>>& FinestraAperte()
	{
		static TMap<TWeakObjectPtr<const APlayerController>, TSharedPtr<SValdorsoFinestraStoria>> Tutte;
		return Tutte;
	}

	FString FinestraStelle(int32 Quante)
	{
		FString Fuori;
		for (int32 i = 0; i < 5; ++i)
		{
			Fuori += i < Quante ? TEXT("★") : TEXT("☆");
		}
		return Fuori;
	}

	/** Una riga con l'etichetta in rubrica e il testo in inchiostro ("Primo passo: ..."); niente se il testo e' vuoto. */
	TSharedRef<SWidget> FinestraRiga(const FValdorsoStileAccesso& Stile, const FText& Etichetta, const FString& Testo)
	{
		return SNew(SVerticalBox)
			.Visibility(Testo.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(Etichetta)
				.Font(Stile.Caratteri->Titolo(14.f, TEXT("Bold")))
				.ColorAndOpacity(ValdorsoTema::Rubrica())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 2.f, 0.f, 10.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Testo))
				.Font(Stile.Caratteri->Testo(17.f))
				.ColorAndOpacity(ValdorsoTema::Inchiostro())
				.AutoWrapText(true)
			];
	}
}

void SValdorsoFinestraStoria::Construct(const FArguments& InArgs)
{
	Dati = InArgs._Finestra;
	OnRisposta = InArgs._OnRisposta;
	Stile = MakeShared<FValdorsoStileAccesso>();
	const FLinearColor ColoreRarita = ValdorsoBattito::ColoreRarita(Dati.Rarita, Dati.Colore);
	// la pergamena della tessera, con il bordo del colore della rarita'
	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 6.f, ColoreRarita, 3.f);
	Nastro = FSlateRoundedBoxBrush(ColoreRarita, 3.f);

	FSlateFontInfo FontTitolo = Stile->Caratteri->Titolo(28.f, TEXT("Bold"));
	FontTitolo.LetterSpacing = 15;
	FSlateFontInfo FontNastro = Stile->Caratteri->Titolo(15.f, TEXT("Bold"));
	FontNastro.LetterSpacing = 120;

	TSharedRef<SButton> Accetta = Stile->Pulsante(LOCTEXT("Accetta", "1  Accetta"), FSimpleDelegate::CreateLambda([this]() { Rispondi(0); }), 18.f);
	TSharedRef<SButton> CiPenso = Stile->Pulsante(LOCTEXT("CiPenso", "2  Ci penso"), FSimpleDelegate::CreateLambda([this]() { Rispondi(2); }), 18.f);
	TSharedRef<SButton> Rifiuta = Stile->Pulsante(LOCTEXT("Rifiuta", "3  Rifiuta"), FSimpleDelegate::CreateLambda([this]() { Rispondi(1); }), 18.f);
	PrimoPulsante = Accetta;

	const FString Scadenza = FString::Printf(TEXT("Hai %d giorni per decidere. \"Ci penso\" la tiene da parte fino ad allora."), FMath::Max(1, Dati.ScadenzaGiorni));

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
			.BorderImage(&Carta)
			.Padding(FMargin(44.f, 26.f))
			[
				SNew(SBox).WidthOverride(660.f).MaxDesiredHeight(860.f)
				[
					SNew(SVerticalBox)
					// il nastro della rarita' e le stelle
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SBorder)
						.BorderImage(&Nastro)
						.Padding(FMargin(26.f, 4.f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(ValdorsoBattito::NomeRarita(Dati.Rarita).ToUpper()))
							.Font(FontNastro)
							.ColorAndOpacity(ValdorsoTema::Pergamena())
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 6.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(FinestraStelle(Dati.Stelle)))
						// (le stelle con il carattere di Slate, che le ha: i caratteri del gioco forse no)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
						.ColorAndOpacity(ColoreRarita)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 8.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(Dati.Intestazione))
						.Font(Stile->Caratteri->Testo(15.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 4.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(Dati.Titolo))
						.Font(FontTitolo)
						.ColorAndOpacity(ValdorsoTema::Rubrica())
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Visibility(Dati.Serie.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
						.Text(FText::FromString(Dati.Serie))
						.Font(Stile->Caratteri->Testo(15.f, TEXT("Italic")))
						.ColorAndOpacity(ColoreRarita)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 10.f, 0.f, 14.f))
					[
						Stile->Separatore(280.f, ValdorsoTema::OroScuro())
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 14.f))
							[
								SNew(STextBlock)
								.Text(FText::FromString(Dati.Trama))
								.Font(Stile->Caratteri->Testo(19.f))
								.ColorAndOpacity(ValdorsoTema::Inchiostro())
								.AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								FinestraRiga(*Stile, LOCTEXT("PrimoPasso", "Primo passo"), Dati.PrimoPasso)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								FinestraRiga(*Stile, LOCTEXT("Premi", "Premi"), Dati.Premi)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								FinestraRiga(*Stile, LOCTEXT("SeRifiuti", "Se rifiuti"), Dati.SeRifiuti)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(Scadenza))
								.Font(Stile->Caratteri->Testo(14.f, TEXT("Italic")))
								.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
								.AutoWrapText(true)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 18.f, 0.f, 0.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(6.f, 0.f))[ SNew(SBox).WidthOverride(180.f)[ Accetta ] ]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(6.f, 0.f))[ SNew(SBox).WidthOverride(180.f)[ CiPenso ] ]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(6.f, 0.f))[ SNew(SBox).WidthOverride(180.f)[ Rifiuta ] ]
					]
				]
			]
		]
	];
}

void SValdorsoFinestraStoria::Rispondi(uint8 Cosa)
{
	if (bRisposto)
	{
		return;
	}
	bRisposto = true;
	OnRisposta.ExecuteIfBound(Dati.Id, Cosa);
}

FReply SValdorsoFinestraStoria::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::One || Tasto == EKeys::NumPadOne)
	{
		Rispondi(0);
	}
	else if (Tasto == EKeys::Two || Tasto == EKeys::NumPadTwo || Tasto == EKeys::Escape || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		Rispondi(2);
	}
	else if (Tasto == EKeys::Three || Tasto == EKeys::NumPadThree)
	{
		Rispondi(1);
	}
	return FReply::Handled();
}

namespace ValdorsoFinestraStoria
{
	bool Aperta(const APlayerController* Controllore)
	{
		const TSharedPtr<SValdorsoFinestraStoria>* Trovata = FinestraAperte().Find(TWeakObjectPtr<const APlayerController>(Controllore));
		return Trovata && Trovata->IsValid();
	}

	bool Apri(APlayerController* Controllore, const FValdorsoFinestraStoria& Finestra, TFunction<void(const FString&, uint8)> Risposta)
	{
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		if (!Viewport || Aperta(Controllore))
		{
			return false;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		TSharedPtr<SValdorsoFinestraStoria> Nuova;
		SAssignNew(Nuova, SValdorsoFinestraStoria)
			.Finestra(Finestra)
			.OnRisposta(FValdorsoSuRispostaStoria::CreateLambda([Debole, Risposta](const FString& Id, uint8 Cosa)
			{
				if (Risposta)
				{
					Risposta(Id, Cosa);
				}
				Chiudi(Debole.Get());
			}));
		Viewport->AddViewportWidgetContent(Nuova.ToSharedRef(), 59);
		FinestraAperte().Add(TWeakObjectPtr<const APlayerController>(Controllore), Nuova);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Nuova->FuocoIniziale());
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
		// il suono della rarita': la campana, piu' forte e due volte per le storie piu' rare
		const uint8 Grado = ValdorsoBattito::QualitaDelPremio(Finestra.Rarita);
		ValdorsoSuoni::Suona(TEXT("S_Campana"), 0.3f + 0.15f * Grado);
		if (Grado >= ValdorsoQualita::Epica)
		{
			FTimerHandle Seconda;
			Mondo->GetTimerManager().SetTimer(Seconda, FTimerDelegate::CreateLambda([Grado]()
			{
				ValdorsoSuoni::Suona(TEXT("S_Campana"), 0.4f + 0.15f * Grado);
			}), 0.9f, false);
		}
		return true;
	}

	void Chiudi(APlayerController* Controllore)
	{
		TSharedPtr<SValdorsoFinestraStoria> Finestra;
		FinestraAperte().RemoveAndCopyValue(TWeakObjectPtr<const APlayerController>(Controllore), Finestra);
		// via anche quelle dei PC che non ci sono piu'
		for (auto It = FinestraAperte().CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent();
			}
		}
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		if (Finestra.IsValid() && Viewport)
		{
			Viewport->RemoveViewportWidgetContent(Finestra.ToSharedRef());
		}
		const AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Controllore);
		if (Controllore && (!Giocatore || !Giocatore->SchermataAperta()))
		{
			Controllore->SetInputMode(FInputModeGameOnly());
			Controllore->bShowMouseCursor = false;
		}
	}
}

#undef LOCTEXT_NAMESPACE
