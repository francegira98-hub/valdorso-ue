// Valdorso - La chat di prossimità: quello che si vede (vedi il .h). Scritto da Claude il 08/11/2026.

#include "SValdorsoChat.h"
#include "ValdorsoChat.h"
#include "ValdorsoBarre.h"
#include "SValdorsoAccesso.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoChat"

// ------------------------------------------------------------------------------------------------- le frasi

void SValdorsoPannelloChat::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Chat = InArgs._Chat;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	// lo stesso velo scuro della bussola, con un filo d'oro scuro appena visibile
	Velo = FSlateRoundedBoxBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.45f), 5.f, ValdorsoTema::OroScuro().CopyWithNewOpacity(0.55f), 1.f);
	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(Larghezza)
		// (08/11, dopo la prima prova di Fra) senza frasi da mostrare il pannello sparisce del tutto: prima ne restava il
		// filo d'oro del bordo, una barra vuota in basso a sinistra
		.Visibility_Lambda([this]()
		{
			return PresenzaMassima > 0.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		})
		[
			SNew(SBorder)
			.BorderImage(&Velo)
			.Padding(FMargin(12.f, 8.f))
			.BorderBackgroundColor_Lambda([this]()
			{
				return FSlateColor(FLinearColor(1.f, 1.f, 1.f, PresenzaMassima));
			})
			[
				SAssignNew(Lista, SVerticalBox)
			]
		]
	];
	SetVisibility(EVisibility::HitTestInvisible);
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoPannelloChat::Controlla));
}

float SValdorsoPannelloChat::PresenzaFrase(int32 Indice) const
{
	const UValdorsoChatComponent* Componente = Chat.Get();
	if (!Componente || !Componente->Frasi().IsValidIndex(Indice))
	{
		return 0.f;
	}
	// mentre si scrive si rivedono tutte
	const float Propria = Componente->RigaAperta() ? 1.f
		: ValdorsoChat::Presenza(FPlatformTime::Seconds() - Componente->Frasi()[Indice].Quando);
	return Propria * PresenzaSchermo;
}

EActiveTimerReturnType SValdorsoPannelloChat::Controlla(double Tempo, float Delta)
{
	const APlayerController* Chi = Controllore.Get();
	const UValdorsoChatComponent* Componente = Chat.Get();
	const bool bVoluto = ValdorsoBarre::SchermoLibero(Chi) || (Componente && Componente->RigaAperta());
	PresenzaSchermo = FMath::FInterpConstantTo(PresenzaSchermo, bVoluto ? 1.f : 0.f, Delta, 4.f);
	if (Componente && Componente->Cambiamenti() != CambiamentiVisti)
	{
		CambiamentiVisti = Componente->Cambiamenti();
		Rifai();
	}
	float Massima = 0.f;
	if (Componente)
	{
		for (int32 i = 0; i < Componente->Frasi().Num(); ++i)
		{
			Massima = FMath::Max(Massima, PresenzaFrase(i));
		}
	}
	PresenzaMassima = Massima;
	return EActiveTimerReturnType::Continue;
}

void SValdorsoPannelloChat::Rifai()
{
	if (!Lista.IsValid())
	{
		return;
	}
	Lista->ClearChildren();
	const UValdorsoChatComponent* Componente = Chat.Get();
	if (!Componente)
	{
		return;
	}
	const TArray<FValdorsoFraseChat>& Elenco = Componente->Frasi();
	for (int32 i = 0; i < Elenco.Num(); ++i)
	{
		const FValdorsoFraseChat& Frase = Elenco[i];
		// gli avvisi di questo PC in corsivo tenue, chi grida d'oro chiaro, gli altri color pergamena
		const bool bAvviso = Frase.Nome.IsEmpty();
		const FLinearColor Colore = bAvviso ? ValdorsoTema::TestoSecondario()
			: (Frase.bGrida ? ValdorsoTema::OroChiaro() : ValdorsoTema::Pergamena());
		Lista->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.f, 1.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(ValdorsoChat::Riga(Frase)))
				.Font(bAvviso ? Caratteri->Testo(17.f, TEXT("Italic")) : Caratteri->Testo(18.f))
				.AutoWrapText(true)
				.ShadowOffset(FVector2D(1.f, 1.f))
				.Visibility_Lambda([this, i]()
				{
					return PresenzaFrase(i) > 0.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
				})
				.ColorAndOpacity_Lambda([this, i, Colore]()
				{
					return FSlateColor(Colore.CopyWithNewOpacity(PresenzaFrase(i)));
				})
				.ShadowColorAndOpacity_Lambda([this, i]()
				{
					return ValdorsoTema::Fondo().CopyWithNewOpacity(0.85f * PresenzaFrase(i));
				})
			];
	}
}

// ------------------------------------------------------------------------------------------------- la riga per scrivere

void SValdorsoRigaChat::Construct(const FArguments& InArgs)
{
	SuInvio = InArgs._OnInvia;
	SuAnnulla = InArgs._OnAnnulla;
	Stile = MakeShared<FValdorsoStileAccesso>();
	// il campo dell'accesso, un poco più basso (è una riga sola sopra il gioco)
	Stile->StileCampo.SetPadding(FMargin(10.f, 6.f));
	FSlateFontInfo FontDici = Stile->Caratteri->Titolo(12.f, TEXT("Bold"));
	FontDici.LetterSpacing = 120;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(SValdorsoPannelloChat::Larghezza)
		[
			SNew(SBorder)
			.BorderImage(&Stile->SfondoPannello)
			.Padding(FMargin(10.f, 6.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(4.f, 0.f, 10.f, 0.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Dici", "DICI"))
					.Font(FontDici)
					.ColorAndOpacity(FSlateColor(ValdorsoTema::Oro()))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SAssignNew(Casella, SEditableTextBox)
					.Style(&Stile->StileCampo)
					.Font(Stile->Caratteri->Testo(18.f))
					.ForegroundColor(FSlateColor(ValdorsoTema::Pergamena()))
					.HintText(LOCTEXT("Suggerimento", "Di' qualcosa…  (/grida: ti sentono più lontano)"))
					.ClearKeyboardFocusOnCommit(false)
					.OnTextChanged_Lambda([this](const FText& Nuovo)
					{
						// al massimo 200 caratteri (il server comunque taglia)
						const FString Scritto = Nuovo.ToString();
						if (Scritto.Len() > ValdorsoChat::LunghezzaMassima && Casella.IsValid())
						{
							Casella->SetText(FText::FromString(Scritto.Left(ValdorsoChat::LunghezzaMassima)));
						}
					})
				]
			]
		]
	];
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoRigaChat::Controlla));
}

TSharedPtr<SWidget> SValdorsoRigaChat::Campo() const
{
	return Casella;
}

FReply SValdorsoRigaChat::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// Invio ed Esc li prende la riga, prima del campo (e prima del gioco: mentre si scrive non arriva niente al colono)
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Enter)
	{
		if (!bFinita)
		{
			bFinita = true;
			SuInvio.ExecuteIfBound(Casella.IsValid() ? Casella->GetText().ToString() : FString());
		}
		return FReply::Handled();
	}
	if (Tasto == EKeys::Escape)
	{
		Annulla();
		return FReply::Handled();
	}
	return SCompoundWidget::OnPreviewKeyDown(MyGeometry, InKeyEvent);
}

void SValdorsoRigaChat::Annulla()
{
	if (bFinita)
	{
		return;
	}
	bFinita = true;
	SuAnnulla.ExecuteIfBound();
}

EActiveTimerReturnType SValdorsoRigaChat::Controlla(double Tempo, float Delta)
{
	if (ApertaDa < 0.0)
	{
		ApertaDa = Tempo;
		return EActiveTimerReturnType::Continue;
	}
	// ha perso la tastiera (una schermata dal server, un clic fuori): la riga si chiude da sola
	if (Tempo - ApertaDa > 0.3 && Casella.IsValid() && !Casella->HasKeyboardFocus() && !Casella->HasFocusedDescendants())
	{
		Annulla();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

#undef LOCTEXT_NAMESPACE
