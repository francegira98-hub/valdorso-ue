// Valdorso - La schermata del baule del colono (v0.2.0, 12/11/2026). Scritto da Claude. Vedi SValdorsoBaule.h.

#include "SValdorsoBaule.h"
#include "SValdorsoZaino.h"
#include "ValdorsoBaule.h"
#include "ValdorsoGildaDati.h"
#include "ValdorsoOggetti.h"
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
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoBaule"

namespace
{
	/** Il disegno a penna di un oggetto, in un quadrato (come nella bottega). */
	class SBauleDisegno : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SBauleDisegno) {}
			SLATE_ARGUMENT(FName, Id)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Id = InArgs._Id;
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(38.f, 38.f); }

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const FVector2f Misura = FVector2f(AllottedGeometry.GetLocalSize());
			ValdorsoDisegnoOggetti::Disegna(OutDrawElements, LayerId, AllottedGeometry, Id, Misura * 0.5f,
				FMath::Min(Misura.X, Misura.Y) * 0.85f, ValdorsoTema::Inchiostro(), 1.8f);
			return LayerId;
		}

	private:
		FName Id;
	};
}

void SValdorsoBaule::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Zaino = InArgs._Zaino;
	OnChiudi = InArgs._OnChiudi;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 4.f, ValdorsoTema::OroScuro(), 2.f);
	Velo = FSlateColorBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.45f));
	Riquadro = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("D9C69C")), 3.f);
	if (const APlayerController* Chi = Controllore.Get(); Chi && Chi->GetPawn())
	{
		Partenza = Chi->GetPawn()->GetActorLocation();
	}
	ListaZaino = SNew(SScrollBox);
	ListaBaule = SNew(SScrollBox);
	TWeakObjectPtr<UValdorsoZainoComponent> Debole = Zaino;

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
				.WidthOverride(900.f)
				.HeightOverride(600.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Titolo", "Il tuo baule, da Bruno"))
						.Font(Caratteri->Titolo(26.f, TEXT("Bold")))
						.ColorAndOpacity(ValdorsoTema::Inchiostro())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 2.f, 0.f, 12.f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Bruno", "\"Il baule e' tuo, la chiave ce l'ho io, e io non apro niente.\" Quello che lasci qui non pesa."))
						.Font(Caratteri->Testo(15.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.Padding(FMargin(0.f, 0.f, 10.f, 0.f))
						[
							Colonna(LOCTEXT("Zaino", "Nello zaino"), TAttribute<FText>::CreateLambda([Debole]()
							{
								const UValdorsoZainoComponent* Il = Debole.Get();
								return FText::Format(LOCTEXT("Peso", "{0} / {1}"), FText::FromString(ValdorsoOggetti::TestoPeso(Il ? Il->Peso() : 0.f)),
									FText::FromString(ValdorsoOggetti::TestoPeso(ValdorsoOggetti::PortataKg)));
							}), ListaZaino.ToSharedRef())
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.Padding(FMargin(10.f, 0.f, 0.f, 0.f))
						[
							Colonna(LOCTEXT("Baule", "Nel baule"), TAttribute<FText>::CreateLambda([Debole]()
							{
								const UValdorsoZainoComponent* Il = Debole.Get();
								int32 Pieni = 0;
								if (Il)
								{
									for (const FValdorsoPostoZaino& Posto : Il->Contenuto().Baule)
									{
										Pieni += Posto.Vuoto() ? 0 : 1;
									}
								}
								return FText::Format(LOCTEXT("Posti", "{0} posti su {1}"), FText::AsNumber(Pieni), FText::AsNumber(ValdorsoZaino::PostiBaule));
							}), ListaBaule.ToSharedRef())
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 12.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Aiuto", "Metti: dallo zaino al baule. Prendi: dal baule allo zaino. Esc o E: chiudi."))
						.Font(Caratteri->Testo(15.f, TEXT("Italic")))
						.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
					]
				]
			]
		]
	];
	Rifai();
}

TSharedRef<SWidget> SValdorsoBaule::Colonna(const FText& Titolo, TAttribute<FText> Sotto, TSharedRef<SScrollBox> Lista)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Bottom)
			[
				SNew(STextBlock)
				.Text(Titolo)
				.Font(Caratteri->Titolo(18.f, TEXT("Bold")))
				.ColorAndOpacity(ValdorsoTema::Inchiostro())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			[
				SNew(STextBlock)
				.Text(Sotto)
				.Font(Caratteri->Testo(14.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::OroScuro())
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			Lista
		];
}

void SValdorsoBaule::Rifai()
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	if (!Il || !ListaZaino.IsValid() || !ListaBaule.IsValid())
	{
		return;
	}
	RifattoA = Il->Cambiamenti();
	ListaZaino->ClearChildren();
	ListaBaule->ClearChildren();
	const FValdorsoZaino& Dentro = Il->Contenuto();
	int32 QuantiZaino = 0;
	for (int32 i = 0; i < Dentro.Posti.Num(); ++i)
	{
		if (!Dentro.Posti[i].Vuoto())
		{
			ListaZaino->AddSlot().Padding(FMargin(0.f, 2.f))[Riga(Dentro.Posti[i], i, false)];
			++QuantiZaino;
		}
	}
	int32 QuantiBaule = 0;
	for (int32 i = 0; i < Dentro.Baule.Num(); ++i)
	{
		if (!Dentro.Baule[i].Vuoto())
		{
			ListaBaule->AddSlot().Padding(FMargin(0.f, 2.f))[Riga(Dentro.Baule[i], i, true)];
			++QuantiBaule;
		}
	}
	const FText VuotoZaino = LOCTEXT("ZainoVuoto", "Lo zaino e' vuoto.");
	const FText VuotoBaule = LOCTEXT("BauleVuoto", "Il baule e' vuoto. Quello che ci metti resta qui, anche quando esci dal gioco.");
	for (int32 k = 0; k < 2; ++k)
	{
		if ((k == 0 ? QuantiZaino : QuantiBaule) == 0)
		{
			(k == 0 ? ListaZaino : ListaBaule)->AddSlot().Padding(FMargin(4.f, 8.f))
			[
				SNew(STextBlock)
				.Text(k == 0 ? VuotoZaino : VuotoBaule)
				.Font(Caratteri->Testo(15.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
				.AutoWrapText(true)
			];
		}
	}
}

TSharedRef<SWidget> SValdorsoBaule::Riga(const FValdorsoPostoZaino& Posto, int32 Indice, bool bNelBaule)
{
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Posto.Id);
	FString Nome = Tipo ? FString(Tipo->Nome) : Posto.Id.ToString();
	if (Posto.Quantita > 1)
	{
		Nome += FString::Printf(TEXT("  x%d"), Posto.Quantita);
	}
	if (Tipo && Tipo->Cariche > 0)
	{
		Nome += FString::Printf(TEXT("  (%d/%d)"), Posto.Cariche, Tipo->Cariche);
	}
	const bool bSiPuo = bNelBaule || (Posto.Id != FName(TEXT("Tessera")) && Posto.Id != ValdorsoGilda::IdFoglio && Posto.Numero == 0);
	TWeakObjectPtr<UValdorsoZainoComponent> Debole = Zaino;
	return SNew(SBorder)
		.BorderImage(&Riquadro)
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 10.f, 0.f))
			[
				SNew(SBauleDisegno).Id(Posto.Id)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Nome))
				.Font(Caratteri->Titolo(14.f, TEXT("Bold")))
				.ColorAndOpacity(ValdorsoTema::Inchiostro())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.MinDesiredWidth(96.f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.IsEnabled(bSiPuo)
					.ToolTipText(bSiPuo ? FText::GetEmpty() : LOCTEXT("TesseraConTe", "Questo va sempre con te."))
					.OnClicked_Lambda([Debole, Indice, bNelBaule]()
					{
						if (UValdorsoZainoComponent* Il = Debole.Get())
						{
							if (bNelBaule)
							{
								Il->ServerRitira(Indice);
							}
							else
							{
								Il->ServerDeposita(Indice);
							}
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(bNelBaule ? LOCTEXT("Prendi", "Prendi") : LOCTEXT("Metti", "Metti"))
						.Font(Caratteri->Titolo(14.f, TEXT("Bold")))
					]
				]
			]
		];
}

FReply SValdorsoBaule::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
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

void SValdorsoBaule::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const APlayerController* Chi = Controllore.Get();
	const APawn* Corpo = Chi ? Chi->GetPawn() : nullptr;
	if (!Corpo || FVector::Dist(Corpo->GetActorLocation(), Partenza) > AValdorsoBaule::Raggio)
	{
		OnChiudi.ExecuteIfBound();
		return;
	}
	const UValdorsoZainoComponent* Il = Zaino.Get();
	if (Il && Il->Cambiamenti() != RifattoA)
	{
		Rifai();
	}
}

namespace ValdorsoBauleSchermo
{
	bool Aperto(const APlayerController* Controllore)
	{
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		return Zaino && Zaino->SchermataBaule.IsValid();
	}

	bool Apri(APlayerController* Controllore)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		const AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore);
		if (!Zaino || !Viewport || !Controllore->IsLocalController() || !Controllore->GetPawn() || Zaino->SchermataBaule.IsValid()
			|| (Nostro && Nostro->SchermataAperta()))
		{
			return false;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		SAssignNew(Zaino->SchermataBaule, SValdorsoBaule)
			.Controllore(Controllore)
			.Zaino(Zaino)
			.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		Viewport->AddViewportWidgetContent(Zaino->SchermataBaule.ToSharedRef(), 57);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Zaino->SchermataBaule);
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
		return true;
	}

	void Chiudi(APlayerController* Controllore)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		if (!Zaino || !Zaino->SchermataBaule.IsValid())
		{
			return;
		}
		const TSharedRef<SValdorsoBaule> Schermo = Zaino->SchermataBaule.ToSharedRef();
		Zaino->SchermataBaule.Reset();
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
