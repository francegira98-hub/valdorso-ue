// Valdorso - La schermata di una bottega (v0.1.4, 10/11/2026). Scritto da Claude. Vedi SValdorsoBottega.h.

#include "SValdorsoBottega.h"
#include "SValdorsoZaino.h"
#include "ValdorsoBotteghe.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoTemaUI.h"
#include "ValdorsoZaino.h"
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
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoBottega"

namespace
{
	/** Il disegno a penna di un oggetto (o dell'Aureus, se Id e' vuoto), in un quadrato. */
	class SBottegaDisegno : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SBottegaDisegno) {}
			SLATE_ARGUMENT(FName, Id)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Id = InArgs._Id;
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(46.f, 46.f); }

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const FVector2f Misura = FVector2f(AllottedGeometry.GetLocalSize());
			const FVector2f Centro = Misura * 0.5f;
			const float Lato = FMath::Min(Misura.X, Misura.Y) * 0.7f;   // (13/11) 0,7: l'icona ad acquerello (x1,39) sta nel riquadro
			if (Id.IsNone())
			{
				ValdorsoDisegnoOggetti::DisegnaMoneta(OutDrawElements, LayerId, AllottedGeometry, Centro, Lato, ValdorsoTema::OroScuro(), 2.f);
			}
			else
			{
				ValdorsoDisegnoOggetti::Disegna(OutDrawElements, LayerId, AllottedGeometry, Id, Centro, Lato, ValdorsoTema::Inchiostro(), 2.f);
			}
			return LayerId;
		}

	private:
		FName Id;
	};
}

void SValdorsoBottega::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Zaino = InArgs._Zaino;
	Banco = InArgs._Banco;
	OnChiudi = InArgs._OnChiudi;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 4.f, ValdorsoTema::OroScuro(), 2.f);
	Velo = FSlateColorBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.45f));
	Riquadro = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("D9C69C")), 3.f);
	if (const APlayerController* Chi = Controllore.Get(); Chi && Chi->GetPawn())
	{
		Partenza = Chi->GetPawn()->GetActorLocation();
	}

	TSharedRef<SScrollBox> Lista = SNew(SScrollBox);
	for (const FValdorsoRicetta* Ricetta : ValdorsoBotteghe::Del(Banco))
	{
		Lista->AddSlot().Padding(FMargin(0.f, 3.f))[Riga(Ricetta)];
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
			.Padding(FMargin(30.f, 22.f))
			[
				SNew(SBox)
				.WidthOverride(720.f)
				.MaxDesiredHeight(640.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 12.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Bottom)
						[
							SNew(STextBlock)
							.Text(ValdorsoBotteghe::Titolo(Banco))
							.Font(Caratteri->Titolo(26.f, TEXT("Bold")))
							.ColorAndOpacity(ValdorsoTema::Inchiostro())
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Bottom)
						[
							SNew(STextBlock)
							.Text(this, &SValdorsoBottega::TestoMonete)
							.Font(Caratteri->Titolo(16.f, TEXT("Bold")))
							.ColorAndOpacity(ValdorsoTema::OroScuro())
						]
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					[
						Lista
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 12.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Aiuto", "Clic sul pulsante: fatto. In rosso quello che ti manca. Esc o E: chiudi."))
						.Font(Caratteri->Testo(15.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SValdorsoBottega::Riga(const FValdorsoRicetta* Ricetta)
{
	const FName IdRicetta(Ricetta->Id);
	const FName Disegno = Ricetta->Risultato ? FName(Ricetta->Risultato) : NAME_None;
	TWeakObjectPtr<UValdorsoZainoComponent> Debole = Zaino;
	// (la tabella delle ricette e' fissa: il puntatore resta buono)
	auto Manca = [this, Ricetta]() -> FText { return MancaDi(Ricetta); };
	const FText Bottone = Ricetta->Costo > 0 && FCString::Strcmp(Ricetta->Verbo, TEXT("Compra")) == 0
		? FText::Format(LOCTEXT("CompraPrezzo", "Compra · {0}"), FText::AsNumber(Ricetta->Costo))
		: (Ricetta->Guadagno > 0 ? FText::Format(LOCTEXT("VendiPrezzo", "Vendi · {0}"), FText::AsNumber(Ricetta->Guadagno))
			: FText::FromString(Ricetta->Verbo));
	const bool bVendita = Ricetta->Guadagno > 0;

	return SNew(SBorder)
		.BorderImage(&Riquadro)
		.Padding(FMargin(10.f, 6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 12.f, 0.f))
			[
				SNew(SBottegaDisegno).Id(bVendita && Ricetta->Serve[0].Id ? FName(Ricetta->Serve[0].Id) : Disegno)
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
					.Text(bVendita ? ValdorsoBotteghe::TestoServe(*Ricetta) : ValdorsoBotteghe::TestoDa(*Ricetta))
					.Font(Caratteri->Titolo(15.f, TEXT("Bold")))
					.ColorAndOpacity(ValdorsoTema::Inchiostro())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([Manca, Ricetta, bVendita]()
					{
						const FText Perche = Manca();
						if (!Perche.IsEmpty())
						{
							return Perche;
						}
						return bVendita ? FText::Format(LOCTEXT("Ricavi", "Ne ricavi {0}"), ValdorsoBotteghe::TestoDa(*Ricetta))
							: (ValdorsoBotteghe::TestoServe(*Ricetta).IsEmpty() ? FText::GetEmpty()
								: FText::Format(LOCTEXT("Serve", "Serve: {0}"), ValdorsoBotteghe::TestoServe(*Ricetta)));
					})
					.Font(Caratteri->Testo(15.f, TEXT("Italic")))
					.ColorAndOpacity_Lambda([Manca]()
					{
						return FSlateColor(Manca().IsEmpty() ? ValdorsoTema::InchiostroTenue() : ValdorsoTema::Rubrica());
					})
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.MinDesiredWidth(140.f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.IsEnabled_Lambda([Manca]() { return Manca().IsEmpty(); })
					.OnClicked_Lambda([Debole, IdRicetta]()
					{
						if (UValdorsoZainoComponent* Il = Debole.Get())
						{
							Il->ServerRicetta(IdRicetta);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(Bottone)
						.Font(Caratteri->Titolo(14.f, TEXT("Bold")))
					]
				]
			]
		];
}

FText SValdorsoBottega::MancaDi(const FValdorsoRicetta* Ricetta) const
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	if (!Il || !Ricetta)
	{
		return LOCTEXT("Niente", "-");
	}
	if (MancanzeDi != Il->Cambiamenti())
	{
		MancanzeDi = Il->Cambiamenti();
		Mancanze.Reset();
	}
	if (const FText* Gia = Mancanze.Find(Ricetta))
	{
		return *Gia;
	}
	return Mancanze.Add(Ricetta, ValdorsoBotteghe::Manca(Il->Contenuto(), *Ricetta));
}

FText SValdorsoBottega::TestoMonete() const
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	return FText::Format(LOCTEXT("Monete", "{0} Aureus"), FText::AsNumber(Il ? Il->Contenuto().Monete : 0));
}

FReply SValdorsoBottega::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// un tasto tenuto premuto (la E con cui si e' aperta) non la richiude
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::E || Tasto == EKeys::I || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SValdorsoBottega::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// allontanandosi (o cadendo, o con il colono sparito) la bottega si chiude
	const APlayerController* Chi = Controllore.Get();
	const APawn* Corpo = Chi ? Chi->GetPawn() : nullptr;
	if (!Corpo || FVector::Dist(Corpo->GetActorLocation(), Partenza) > ValdorsoBotteghe::Raggio)
	{
		OnChiudi.ExecuteIfBound();
	}
}

namespace ValdorsoBottegaSchermo
{
	bool Aperta(const APlayerController* Controllore)
	{
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		return Zaino && Zaino->SchermataBottega.IsValid();
	}

	bool Apri(APlayerController* Controllore, EValdorsoTipoBanco Banco)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		const AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore);
		if (!Zaino || !Viewport || !Controllore->IsLocalController() || !Controllore->GetPawn() || Zaino->SchermataBottega.IsValid()
			|| ValdorsoZainoSchermo::Aperto(Controllore) || (Nostro && Nostro->SchermataAperta()))
		{
			return false;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		SAssignNew(Zaino->SchermataBottega, SValdorsoBottega)
			.Controllore(Controllore)
			.Zaino(Zaino)
			.Banco(Banco)
			.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		Viewport->AddViewportWidgetContent(Zaino->SchermataBottega.ToSharedRef(), 57);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Zaino->SchermataBottega);
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
		return true;
	}

	void Chiudi(APlayerController* Controllore)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		if (!Zaino || !Zaino->SchermataBottega.IsValid())
		{
			return;
		}
		const TSharedRef<SValdorsoBottega> Schermo = Zaino->SchermataBottega.ToSharedRef();
		Zaino->SchermataBottega.Reset();
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
