// Valdorso - La scheda del personaggio (vedi SValdorsoScheda.h). Scritto da Claude il 10/11/2026.

#include "SValdorsoScheda.h"
#include "ValdorsoCrescita.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoScheda"

namespace
{
	/** Quanto e' larga la barra (px). */
	constexpr float SchedaLarghezzaBarra = 260.f;

	FText SchedaTestoVerso(EValdorsoVerso Verso)
	{
		switch (Verso)
		{
		case EValdorsoVerso::Ferma: return LOCTEXT("Ferma", "Ferma");
		case EValdorsoVerso::Scende: return LOCTEXT("Cala", "Cala");
		default: return LOCTEXT("Sale", "Sale");
		}
	}

	FLinearColor SchedaColoreVerso(EValdorsoVerso Verso)
	{
		switch (Verso)
		{
		case EValdorsoVerso::Ferma: return ValdorsoTema::InchiostroTenue();
		case EValdorsoVerso::Scende: return ValdorsoTema::Rubrica();
		default: return ValdorsoTema::Hex(TEXT("3F6B2A"));
		}
	}
}

void SValdorsoScheda::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	OnChiudi = InArgs._OnChiudi;
	Crescita = UValdorsoCrescitaComponent::Di(Controllore.Get());
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 4.f, ValdorsoTema::OroScuro(), 2.f);
	Velo = FSlateColorBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.45f));
	Sfondo = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("D8C69C")), 2.f, ValdorsoTema::Hex(TEXT("B49A6A")), 1.f);
	Pieno = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("9C7A2E")), 2.f);
	PienoGrigio = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("A89F8C")), 2.f);
	Caricato = FSlateRoundedBoxBrush(ValdorsoTema::Hex(TEXT("D4AF37")), 2.f);
	Tacca = FSlateColorBrush(ValdorsoTema::Inchiostro().CopyWithNewOpacity(0.35f));

	const APlayerController* Chi = Controllore.Get();
	const FString Nome = Chi && Chi->PlayerState ? Chi->PlayerState->GetPlayerName() : FString();

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
				.WidthOverride(860.f)
				.HeightOverride(640.f)
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
							.Text(Nome.IsEmpty() ? LOCTEXT("Titolo", "La scheda del colono")
								: FText::Format(LOCTEXT("TitoloNome", "La scheda di {0}"), FText::FromString(Nome)))
							.Font(Caratteri->Titolo(26.f, TEXT("Bold")))
							.ColorAndOpacity(ValdorsoTema::Inchiostro())
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Bottom)
						[
							SNew(STextBlock)
							.Text(this, &SValdorsoScheda::TestoTotale)
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
						.Text(LOCTEXT("Aiuto", "Le abilita' salgono usandole. A 25, 50, 75 e 100 si fermano e si caricano: crescono in un momento difficile, e "
							"il Cuore ne dice il nuovo nome. In tutto si arriva a 700: metti a calare quelle che non ti servono. K o Esc: chiudi."))
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

FText SValdorsoScheda::TestoTotale() const
{
	const UValdorsoCrescitaComponent* C = Crescita.Get();
	const float Somma = C ? ValdorsoCrescere::Totale(C->Contenuto()) : 0.f;
	return FText::Format(LOCTEXT("Totale", "{0} / {1}"), FText::AsNumber(FMath::FloorToInt32(Somma)), FText::AsNumber(FMath::RoundToInt32(ValdorsoCrescere::Tetto)));
}

TSharedRef<SWidget> SValdorsoScheda::Barra(float Punti, float Carica, bool bInArrivo)
{
	const float Fatto = FMath::Clamp(Punti / ValdorsoCrescere::Massimo, 0.f, 1.f);
	TSharedRef<SOverlay> Sopra = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(&Sfondo)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.Padding(FMargin(1.f))
		[
			SNew(SBox)
			.WidthOverride(FMath::Max(0.f, (SchedaLarghezzaBarra - 2.f) * Fatto))
			[
				SNew(SBorder).BorderImage(bInArrivo ? &PienoGrigio : (Carica >= 1.f ? &Caricato : &Pieno))
			]
		];
	// le soglie: 25, 50, 75
	for (int32 s = 1; s < 4; ++s)
	{
		Sopra->AddSlot()
		.HAlign(HAlign_Left)
		.Padding(FMargin(SchedaLarghezzaBarra * 0.25f * static_cast<float>(s) - 1.f, 0.f, 0.f, 0.f))
		[
			SNew(SBox)
			.WidthOverride(2.f)
			[
				SNew(SBorder).BorderImage(&Tacca)
			]
		];
	}
	return SNew(SBox)
		.WidthOverride(SchedaLarghezzaBarra)
		.HeightOverride(14.f)
		[
			Sopra
		];
}

TSharedRef<SWidget> SValdorsoScheda::Riga(const FValdorsoAbilitaTipo& Tipo, const FValdorsoAbilitaValore* Valore)
{
	const float Punti = Valore ? Valore->Punti : 0.f;
	const int32 Evoluzioni = Valore ? Valore->Evoluzioni : 0;
	const float Carica = Valore ? Valore->Carica : 0.f;
	const EValdorsoVerso Verso = Valore ? Valore->Verso : EValdorsoVerso::Sale;
	const bool bAllaSoglia = Evoluzioni < 4 && Punti >= ValdorsoCrescere::Soglia(Evoluzioni) - KINDA_SMALL_NUMBER;
	const FString NomeAdesso = ValdorsoCrescere::Nome(Tipo.Id, Evoluzioni);

	// cosa dice passandoci sopra: cosa fa, e cosa viene dopo
	FString Spiega = Tipo.Descrizione;
	const FString Dopo = ValdorsoCrescere::ProssimoSblocco(Tipo.Id, Evoluzioni);
	if (!Dopo.IsEmpty())
	{
		Spiega += FString::Printf(TEXT("\nA %d: %s."), FMath::RoundToInt32(ValdorsoCrescere::Soglia(Evoluzioni)), *Dopo);
	}
	if (Evoluzioni > 0)
	{
		Spiega += FString::Printf(TEXT("\nEra: %s."), Tipo.Nomi[0]);
	}

	FText Stato;
	if (Tipo.bInArrivo)
	{
		Stato = LOCTEXT("InArrivo", "in arrivo");
	}
	else if (Evoluzioni >= 4)
	{
		Stato = LOCTEXT("Leggenda", "al massimo");
	}
	else if (bAllaSoglia)
	{
		Stato = Carica >= 1.f ? LOCTEXT("Pronta", "pronta a crescere")
			: FText::Format(LOCTEXT("Carica", "carica {0}%"), FText::AsNumber(FMath::FloorToInt32(Carica * 100.f)));
	}

	TWeakObjectPtr<UValdorsoCrescitaComponent> DeboleCrescita = Crescita;
	const FName Id = Tipo.Id;
	const uint8 ProssimoVerso = static_cast<uint8>((static_cast<uint8>(Verso) + 1) % 3);
	const FLinearColor ColoreNome = Tipo.bInArrivo ? ValdorsoTema::InchiostroTenue() : ValdorsoTema::Inchiostro();

	return SNew(SBox)
		.Padding(FMargin(0.f, 3.f))
		.ToolTipText(FText::FromString(Spiega))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(NomeAdesso))
				.Font(Caratteri->Testo(17.f, Evoluzioni > 0 ? TEXT("SemiBold") : TEXT("Regular")))
				.ColorAndOpacity(ColoreNome)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(10.f, 0.f))
			[
				Barra(Punti, bAllaSoglia ? Carica : 0.f, Tipo.bInArrivo)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(56.f)
				[
					SNew(STextBlock)
					.Text(FText::AsNumber(FMath::FloorToFloat(Punti * 10.f) / 10.f, &FNumberFormattingOptions::DefaultNoGrouping()))
					.Font(Caratteri->Titolo(14.f, TEXT("Bold")))
					.ColorAndOpacity(ColoreNome)
					.Justification(ETextJustify::Right)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(12.f, 0.f, 0.f, 0.f))
			[
				SNew(SBox)
				.WidthOverride(84.f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.IsFocusable(false)
					.IsEnabled(!Tipo.bInArrivo || Punti > 0.f)
					.ToolTipText(LOCTEXT("VersoAiuto", "Un clic: sale, ferma (non sale e non cala), cala (fa spazio alle altre quando sei al tetto)."))
					.OnClicked_Lambda([DeboleCrescita, Id, ProssimoVerso]()
					{
						if (UValdorsoCrescitaComponent* Il = DeboleCrescita.Get())
						{
							Il->ServerVerso(Id, ProssimoVerso);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(SchedaTestoVerso(Verso))
						.Font(Caratteri->Titolo(12.f, TEXT("Bold")))
						.ColorAndOpacity(SchedaColoreVerso(Verso))
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(10.f, 0.f, 0.f, 0.f))
			[
				SNew(SBox)
				.WidthOverride(120.f)
				[
					SNew(STextBlock)
					.Text(Stato)
					.Font(Caratteri->Testo(14.f, TEXT("Italic")))
					.ColorAndOpacity(Carica >= 1.f && bAllaSoglia ? ValdorsoTema::OroScuro() : ValdorsoTema::InchiostroTenue())
				]
			]
		];
}

void SValdorsoScheda::Rifai()
{
	if (!Contenuto.IsValid())
	{
		return;
	}
	const UValdorsoCrescitaComponent* C = Crescita.Get();
	Visti = C ? C->Cambiamenti() : -1;
	static const FValdorsoAbilitaColono Vuoto;
	const FValdorsoAbilitaColono& Tutto = C ? C->Contenuto() : Vuoto;
	TSharedRef<SScrollBox> Elenco = SNew(SScrollBox);
	const EValdorsoFamigliaAbilita Famiglie[] = { EValdorsoFamigliaAbilita::Corpo, EValdorsoFamigliaAbilita::Armi, EValdorsoFamigliaAbilita::Mondo,
		EValdorsoFamigliaAbilita::Mestieri, EValdorsoFamigliaAbilita::Spirito };
	for (const EValdorsoFamigliaAbilita Famiglia : Famiglie)
	{
		Elenco->AddSlot()
		.Padding(FMargin(0.f, 10.f, 0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(ValdorsoCrescere::NomeFamiglia(Famiglia))
			.Font(Caratteri->Titolo(16.f, TEXT("Bold")))
			.ColorAndOpacity(ValdorsoTema::Rubrica())
		];
		for (const FValdorsoAbilitaTipo& Tipo : ValdorsoCrescere::Tutte())
		{
			if (Tipo.Famiglia == Famiglia)
			{
				Elenco->AddSlot()
				[
					Riga(Tipo, ValdorsoCrescere::Valore(Tutto, Tipo.Id))
				];
			}
		}
	}
	Contenuto->SetContent(Elenco);
}

FReply SValdorsoScheda::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::K || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SValdorsoScheda::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const APlayerController* Chi = Controllore.Get();
	if (!Chi || !Chi->GetPawn())
	{
		OnChiudi.ExecuteIfBound();
		return;
	}
	const UValdorsoCrescitaComponent* C = Crescita.Get();
	if (C && C->Cambiamenti() != Visti)
	{
		Rifai();
	}
}

namespace ValdorsoSchedaSchermo
{
	bool Aperta(const APlayerController* Controllore)
	{
		const UValdorsoCrescitaComponent* Crescita = UValdorsoCrescitaComponent::Di(Controllore);
		return Crescita && Crescita->Schermata.IsValid();
	}

	void Alterna(APlayerController* Controllore)
	{
		UValdorsoCrescitaComponent* Crescita = UValdorsoCrescitaComponent::Di(Controllore);
		if (!Crescita)
		{
			return;
		}
		if (Crescita->Schermata.IsValid())
		{
			Chiudi(Controllore);
			return;
		}
		UWorld* Mondo = Controllore->GetWorld();
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		const AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore);
		if (!Viewport || !Controllore->IsLocalController() || !Controllore->GetPawn() || (Nostro && Nostro->SchermataAperta()))
		{
			return;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		SAssignNew(Crescita->Schermata, SValdorsoScheda)
			.Controllore(Controllore)
			.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		Viewport->AddViewportWidgetContent(Crescita->Schermata.ToSharedRef(), 57);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Crescita->Schermata);
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
	}

	void Chiudi(APlayerController* Controllore)
	{
		UValdorsoCrescitaComponent* Crescita = UValdorsoCrescitaComponent::Di(Controllore);
		if (!Crescita || !Crescita->Schermata.IsValid())
		{
			return;
		}
		const TSharedRef<SValdorsoScheda> Schermo = Crescita->Schermata.ToSharedRef();
		Crescita->Schermata.Reset();
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
