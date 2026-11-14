// Valdorso - Bussola e taccuino di viaggio: quello che si vede (vedi il .h). Scritto da Claude il 07/11/2026.

#include "SValdorsoTaccuino.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoBagnato.h"
#include "ValdorsoCielo.h"
#include "ValdorsoClima.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "SValdorsoAccesso.h"
#include "SValdorsoBacheca.h"
#include "ValdorsoModoFoto.h"
#include "ValdorsoRune.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Camera/PlayerCameraManager.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoTaccuino"

namespace
{
	TAutoConsoleVariable<int32> CVarBussola(
		TEXT("Valdorso.Bussola"),
		1,
		TEXT("1: la bussola in alto sullo schermo (di serie). 0: niente bussola."),
		ECVF_Default);

	/** Le misure della bussola: la fascia scura con le lettere, sotto i segni dei luoghi e il nome. */
	constexpr float BussolaAltezzaFascia = 30.f;
	constexpr float BussolaAltezza = 64.f;
	/** (09/11) Ai lati della striscia: a sinistra l'ora, a destra la temperatura. */
	constexpr float BussolaLato = 185.f;
	/** Entro quanti gradi dal centro un luogo mostra il nome. */
	constexpr float BussolaGradiNome = 9.f;
	/** Più vicino di così (cm) il segno del luogo sparisce: ci si è dentro. */
	constexpr double BussolaVicinoCm = 1500.0;

	/** La rotta di chi guarda: la telecamera (se c'è), se no il controllore. */
	float BussolaRottaGuardata(const APlayerController* Controllore)
	{
		const float Imbardata = Controllore->PlayerCameraManager
			? static_cast<float>(Controllore->PlayerCameraManager->GetCameraRotation().Yaw)
			: static_cast<float>(Controllore->GetControlRotation().Yaw);
		return ValdorsoBussola::Rotta(Imbardata);
	}

	/** Quanto è grande un testo sullo schermo (zero se Slate non c'è). */
	FVector2f BussolaMisura(const FString& Testo, const FSlateFontInfo& Font)
	{
		if (!FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
		{
			return FVector2f::ZeroVector;
		}
		const TSharedRef<FSlateFontMeasure> Misuratore = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return FVector2f(Misuratore->Measure(Testo, Font));
	}

	/**
	 * Un testo con un'ombra scura sotto (si legge sopra il cielo e sopra la neve): lo stesso testo disegnato due volte,
	 * prima scuro e spostato di 1 pixel, poi del suo colore. (08/11) L'ombra è il Fondo del tema, più piena di prima.
	 */
	void BussolaScrivi(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria, const FString& Testo,
		const FSlateFontInfo& Font, const FVector2f& Centro, const FLinearColor& Colore)
	{
		const FVector2f Misura = BussolaMisura(Testo, Font);
		if (Misura.IsNearlyZero())
		{
			return;
		}
		const FVector2f Inizio = Centro - Misura * 0.5f;
		FSlateDrawElement::MakeText(Elementi, Strato,
			Geometria.ToPaintGeometry(FVector2D(Misura), FSlateLayoutTransform(1.f, FVector2D(Inizio + FVector2f(1.f, 1.f)))),
			Testo, Font, ESlateDrawEffect::None, ValdorsoTema::Fondo().CopyWithNewOpacity(0.85f * Colore.A));
		FSlateDrawElement::MakeText(Elementi, Strato + 1,
			Geometria.ToPaintGeometry(FVector2D(Misura), FSlateLayoutTransform(1.f, FVector2D(Inizio))),
			Testo, Font, ESlateDrawEffect::None, Colore);
	}

	/** "Scoperto il 07/11/2026, al tramonto" (la data sul fuso di questo PC). */
	FText TaccuinoQuando(const FValdorsoLuogoScoperto& Luogo)
	{
		if (Luogo.Quando <= 0)
		{
			return FText::GetEmpty();
		}
		const FDateTime Qui = FDateTime::FromUnixTimestamp(Luogo.Quando) + (FDateTime::Now() - FDateTime::UtcNow());
		const FText Giorno = FText::FromString(Qui.ToString(TEXT("%d/%m/%Y")));
		const FText Momento = ValdorsoTaccuino::Momento(Luogo.Ora);
		return Momento.IsEmpty() ? FText::Format(LOCTEXT("ScopertoIl", "Scoperto il {0}"), Giorno)
			: FText::Format(LOCTEXT("ScopertoIlMomento", "Scoperto il {0}, {1}"), Giorno, Momento);
	}
}

// ------------------------------------------------------------------------------------------------- la bussola

void SValdorsoBussola::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Taccuino = InArgs._Taccuino;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Pieno = FSlateColorBrush(FLinearColor::White);
	// (08/11) Sul cielo chiaro la fascia di prima (pannello al 62%, filo d'oro tenue) quasi spariva: adesso un fondo
	// scuro velato (il Fondo del tema al 55%), angoli tondi e un filo d'oro scuro. Lo stesso per il nome del luogo.
	Fascia = FSlateRoundedBoxBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.55f), 6.f,
		ValdorsoTema::OroScuro().CopyWithNewOpacity(0.9f), 1.f);
	SetVisibility(EVisibility::HitTestInvisible);
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoBussola::Anima));
}

bool SValdorsoBussola::DaMostrare() const
{
	const APlayerController* Chi = Controllore.Get();
	// una schermata aperta (pausa, mappa, bacheca, registro...) mostra il mouse: allora la bussola si toglie di mezzo
	return CVarBussola.GetValueOnGameThread() != 0 && Chi && Chi->GetPawn() && !Chi->bShowMouseCursor
		&& !ValdorsoFoto::Aperta(Chi) && !ValdorsoTaccuino::Aperto(Chi);
}

EActiveTimerReturnType SValdorsoBussola::Anima(double Tempo, float Delta)
{
	Presenza = FMath::FInterpConstantTo(Presenza, DaMostrare() ? 1.f : 0.f, Delta, 4.f);
	return EActiveTimerReturnType::Continue;
}

FVector2D SValdorsoBussola::ComputeDesiredSize(float) const
{
	// (09/11) più larga: ai lati l'ora (a sinistra) e la temperatura (a destra)
	return FVector2D(Larghezza + 2.f * BussolaLato, BussolaAltezza);
}

int32 SValdorsoBussola::OnPaint(const FPaintArgs& Args, const FGeometry& GeometriaIntera, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const APlayerController* Chi = Controllore.Get();
	if (Presenza <= 0.01f || !Chi)
	{
		return LayerId;
	}
	// la striscia sta nel mezzo; tutto quello che segue (fino alle due etichette dei lati) è disegnato lì dentro
	const FGeometry AllottedGeometry = GeometriaIntera.MakeChild(FVector2f(Larghezza, BussolaAltezza), FSlateLayoutTransform(FVector2f(BussolaLato, 0.f)));
	DisegnaLati(GeometriaIntera, OutDrawElements, LayerId, Presenza * InWidgetStyle.GetColorAndOpacityTint().A);
	const float Alfa = Presenza * InWidgetStyle.GetColorAndOpacityTint().A;
	const float Guardata = BussolaRottaGuardata(Chi);
	const float Mezzo = Larghezza * 0.5f;
	// verso i bordi tutto sfuma: la striscia sembra un pezzo di un cerchio che gira
	auto Sfuma = [](float ScartoGradi)
	{
		const float T = FMath::Clamp(FMath::Abs(ScartoGradi) / (Campo * 0.5f), 0.f, 1.f);
		return 1.f - T * T;
	};

	// la fascia
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2f(Larghezza, BussolaAltezzaFascia), FSlateLayoutTransform(FVector2f(0.f, 0.f))),
		&Fascia, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, Alfa));
	// (08/11) e dentro, appena sopra il bordo basso, un filo d'oro sottile che sfuma verso i lati (a pezzetti)
	{
		constexpr int32 Pezzi = 8;
		const float YFilo = BussolaAltezzaFascia - 3.f;
		const float Lungo = (Mezzo - 14.f) / Pezzi;
		for (int32 i = 0; i < Pezzi; ++i)
		{
			const float T = static_cast<float>(i) / Pezzi;
			const FLinearColor Filo = ValdorsoTema::Oro().CopyWithNewOpacity(0.4f * (1.f - T * T) * Alfa);
			for (const float Lato : { -1.f, 1.f })
			{
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2f>{ FVector2f(Mezzo + Lato * Lungo * i, YFilo), FVector2f(Mezzo + Lato * Lungo * (i + 1), YFilo) },
					ESlateDrawEffect::None, Filo, true, 1.f);
			}
		}
	}

	// le tacche ogni 15 gradi e le lettere ogni 45
	const FSlateFontInfo FontCardinale = Caratteri->Titolo(15.f, TEXT("Bold"));
	const FSlateFontInfo FontIntermedio = Caratteri->Titolo(11.f, TEXT("Regular"));
	for (int32 k = 0; k < 24; ++k)
	{
		const float Rotta = k * 15.f;
		const float Scarto = ValdorsoBussola::Scarto(Guardata, Rotta);
		float X = 0.f;
		if (!ValdorsoBussola::SullaStriscia(Scarto, Larghezza, Campo, X))
		{
			continue;
		}
		const float Forza = Sfuma(Scarto) * Alfa;
		if (k % 3 == 0)
		{
			const int32 Punto = k / 3;
			const bool bCardinale = Punto % 2 == 0;
			// (08/11) tutte d'oro chiaro (prima Oro e TestoSecondario, pallide sul cielo); le intermedie un poco più tenui
			const FLinearColor Colore = ValdorsoTema::OroChiaro().CopyWithNewOpacity(bCardinale ? 1.f : 0.8f);
			BussolaScrivi(OutDrawElements, LayerId + 1, AllottedGeometry, ValdorsoBussola::Sigla(Punto),
				bCardinale ? FontCardinale : FontIntermedio, FVector2f(X, BussolaAltezzaFascia * 0.5f + 1.f),
				Colore.CopyWithNewOpacity(Colore.A * Forza));
		}
		else
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2f>{ FVector2f(X, BussolaAltezzaFascia - 9.f), FVector2f(X, BussolaAltezzaFascia - 4.f) },
				ESlateDrawEffect::None, ValdorsoTema::OroChiaro().CopyWithNewOpacity(0.6f * Forza), true, 1.f);
		}
	}

	// il segno di dove si guarda: un piccolo triangolo d'oro che punta in giù, sul bordo alto
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ FVector2f(Mezzo - 5.f, 1.f), FVector2f(Mezzo + 5.f, 1.f), FVector2f(Mezzo, 7.f), FVector2f(Mezzo - 5.f, 1.f) },
		ESlateDrawEffect::None, ValdorsoTema::OroChiaro().CopyWithNewOpacity(Alfa), true, 1.5f);

	// i luoghi già scoperti: un rombo sotto la fascia; quello più davanti a sé ha il nome e la distanza
	const UValdorsoTaccuinoComponent* Diario = Taccuino.Get();
	const APawn* Colono = Chi->GetPawn();
	if (Diario && Colono)
	{
		const FVector Qui = Colono->GetActorLocation();
		const FValdorsoLuogo* Davanti = nullptr;
		float ScartoDavanti = BussolaGradiNome;
		const float YRombo = BussolaAltezzaFascia + 7.f;
		for (const FValdorsoLuogoScoperto& Scoperto : Diario->LuoghiScoperti())
		{
			const FValdorsoLuogo* Luogo = ValdorsoLuoghi::Trova(Scoperto.Nome);
			if (!Luogo)
			{
				continue;
			}
			const double Distanza = FVector2D::Distance(FVector2D(Qui.X, Qui.Y), FVector2D(Luogo->Posizione.X, Luogo->Posizione.Y));
			if (Distanza < BussolaVicinoCm)
			{
				continue;
			}
			const float Scarto = ValdorsoBussola::Scarto(Guardata, ValdorsoBussola::RottaVerso(Qui, Luogo->Posizione));
			float X = 0.f;
			if (!ValdorsoBussola::SullaStriscia(Scarto, Larghezza, Campo, X))
			{
				continue;
			}
			const FLinearColor Colore = ValdorsoTema::Pergamena().CopyWithNewOpacity(Sfuma(Scarto) * Alfa);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2f>{ FVector2f(X, YRombo - 4.f), FVector2f(X + 4.f, YRombo), FVector2f(X, YRombo + 4.f),
					FVector2f(X - 4.f, YRombo), FVector2f(X, YRombo - 4.f) },
				ESlateDrawEffect::None, Colore, true, 1.5f);
			if (FMath::Abs(Scarto) < FMath::Abs(ScartoDavanti))
			{
				ScartoDavanti = Scarto;
				Davanti = Luogo;
			}
		}
		if (Davanti)
		{
			const double Distanza = FVector2D::Distance(FVector2D(Qui.X, Qui.Y), FVector2D(Davanti->Posizione.X, Davanti->Posizione.Y));
			float X = Mezzo;
			ValdorsoBussola::SullaStriscia(ScartoDavanti, Larghezza, Campo, X);
			const FString Testo = FString::Printf(TEXT("%s  ·  %s"), *Davanti->Nome, *ValdorsoBussola::TestoDistanza(Distanza));
			const FSlateFontInfo FontNome = Caratteri->Testo(15.f, TEXT("Italic"));
			const FVector2f CentroNome(X, YRombo + 15.f);
			// (08/11) sotto il nome lo stesso fondo scuro velato della fascia, appena più largo del testo
			const FVector2f MisuraNome = BussolaMisura(Testo, FontNome);
			if (!MisuraNome.IsNearlyZero())
			{
				const FVector2f Velo(MisuraNome.X + 22.f, MisuraNome.Y + 2.f);
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
					AllottedGeometry.ToPaintGeometry(Velo, FSlateLayoutTransform(CentroNome - Velo * 0.5f)),
					&Fascia, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.85f * Alfa));
			}
			BussolaScrivi(OutDrawElements, LayerId + 2, AllottedGeometry, Testo, FontNome, CentroNome,
				ValdorsoTema::Pergamena().CopyWithNewOpacity(Alfa));
		}
	}
	return LayerId + 4;
}

void SValdorsoBussola::DisegnaLati(const FGeometry& Geometria, FSlateWindowElementList& Elementi, int32 Strato, float Alfa) const
{
	const APlayerController* Chi = Controllore.Get();
	UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
	if (!Mondo || Alfa <= 0.01f)
	{
		return;
	}
	if (!bCieloCercato)
	{
		bCieloCercato = true;
		for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
		{
			Cielo = *It;
			break;
		}
	}
	const FSlateFontInfo Font = Caratteri->Testo(15.f, TEXT("Italic"));
	const float Centro = BussolaAltezzaFascia * 0.5f;

	// a sinistra: l'ora canonica del tempio e l'ora ("Vespro · 18:40"), appoggiate alla striscia
	if (const AValdorsoCielo* IlCielo = Cielo.Get())
	{
		const float Ora = IlCielo->Ora();
		const FString Testo = FString::Printf(TEXT("%s  ·  %s"), *ValdorsoClima::OraCanonica(Ora).ToString(), *ValdorsoClima::OraEMinuti(Ora));
		const FVector2f Misura = BussolaMisura(Testo, Font);
		if (!Misura.IsNearlyZero())
		{
			const FVector2f Velo(Misura.X + 22.f, BussolaAltezzaFascia - 6.f);
			const FVector2f Angolo(FMath::Max(0.f, BussolaLato - 8.f - Velo.X), Centro - Velo.Y * 0.5f);
			FSlateDrawElement::MakeBox(Elementi, Strato, Geometria.ToPaintGeometry(Velo, FSlateLayoutTransform(Angolo)),
				&Fascia, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, Alfa));
			BussolaScrivi(Elementi, Strato + 1, Geometria, Testo, Font, Angolo + Velo * 0.5f, ValdorsoTema::Pergamena().CopyWithNewOpacity(Alfa));
		}
	}

	// a destra: il termometro, i gradi che sente il colono e, se non è mite, come si sente ("12°  ·  Freddo")
	const UValdorsoBagnatoComponent* Clima = UValdorsoBagnatoComponent::Di(Chi->GetPawn());
	if (!Clima)
	{
		return;
	}
	const float Gradi = Clima->Temperatura();
	const EValdorsoSensazione Come = ValdorsoClima::Sensazione(Gradi);
	FLinearColor Tinta = ValdorsoTema::Pergamena();
	switch (Come)
	{
	case EValdorsoSensazione::Gelo: Tinta = ValdorsoTema::Hex(TEXT("9CC8F0")); break;
	case EValdorsoSensazione::Freddo: Tinta = ValdorsoTema::Hex(TEXT("BBD6EE")); break;
	case EValdorsoSensazione::Caldo: Tinta = ValdorsoTema::Brace(); break;
	case EValdorsoSensazione::Afa: Tinta = ValdorsoTema::Hex(TEXT("E0603A")); break;
	default: break;
	}
	FString Testo = FString::Printf(TEXT("%d°"), FMath::RoundToInt(Gradi));
	if (Come != EValdorsoSensazione::Mite && Come != EValdorsoSensazione::Fresco)
	{
		Testo += TEXT("  ·  ") + ValdorsoClima::Nome(Come).ToString();
	}
	const FVector2f Misura = BussolaMisura(Testo, Font);
	if (Misura.IsNearlyZero())
	{
		return;
	}
	const float Icona = 14.f;
	const FVector2f Velo(Misura.X + Icona + 30.f, BussolaAltezzaFascia - 6.f);
	const FVector2f Angolo(BussolaLato + Larghezza + 8.f, Centro - Velo.Y * 0.5f);
	FSlateDrawElement::MakeBox(Elementi, Strato, Geometria.ToPaintGeometry(Velo, FSlateLayoutTransform(Angolo)),
		&Fascia, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, Alfa));
	// il termometro: il bulbo in basso, il tubo, e dentro il "mercurio" che sale con i gradi (da -10 a 35)
	const FVector2f Bulbo = Angolo + FVector2f(13.f, Velo.Y * 0.5f + 5.f);
	const float Mercurio = FMath::Clamp((Gradi + 10.f) / 45.f, 0.08f, 1.f);
	const FLinearColor Filo = ValdorsoTema::OroChiaro().CopyWithNewOpacity(0.9f * Alfa);
	TArray<FVector2f> Cerchio;
	for (int32 k = 0; k <= 12; ++k)
	{
		const float R = FMath::DegreesToRadians(-60.f + 300.f * k / 12.f);
		Cerchio.Add(Bulbo + FVector2f(3.5f * FMath::Sin(R), -3.5f * FMath::Cos(R)));
	}
	FSlateDrawElement::MakeLines(Elementi, Strato + 1, Geometria.ToPaintGeometry(), Cerchio, ESlateDrawEffect::None, Filo, true, 1.2f);
	FSlateDrawElement::MakeLines(Elementi, Strato + 1, Geometria.ToPaintGeometry(),
		TArray<FVector2f>{ Bulbo + FVector2f(-1.8f, -3.f), Bulbo + FVector2f(-1.8f, -14.f), Bulbo + FVector2f(1.8f, -14.f), Bulbo + FVector2f(1.8f, -3.f) },
		ESlateDrawEffect::None, Filo, true, 1.2f);
	FSlateDrawElement::MakeLines(Elementi, Strato + 2, Geometria.ToPaintGeometry(),
		TArray<FVector2f>{ Bulbo, Bulbo + FVector2f(0.f, -3.f - 10.f * Mercurio) }, ESlateDrawEffect::None, Tinta.CopyWithNewOpacity(Alfa), true, 2.f);
	BussolaScrivi(Elementi, Strato + 1, Geometria, Testo, Font, Angolo + FVector2f(Icona + 12.f + Misura.X * 0.5f, Velo.Y * 0.5f),
		Tinta.CopyWithNewOpacity(Alfa));
}

// ------------------------------------------------------------------------------------------------- il taccuino

void SValdorsoTaccuino::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Taccuino = InArgs._Taccuino;
	OnChiudi = InArgs._OnChiudi;
	Stile = MakeShared<FValdorsoStileAccesso>();
	// la carta: la stessa pergamena della bacheca
	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 4.f, ValdorsoTema::OroScuro(), 2.f);
	Trasparente = FSlateColorBrush(FLinearColor::Transparent);
	StileLinguetta = FButtonStyle()
		.SetNormal(Trasparente)
		.SetHovered(Trasparente)
		.SetPressed(Trasparente)
		.SetDisabled(Trasparente)
		.SetNormalPadding(FMargin(14.f, 4.f))
		.SetPressedPadding(FMargin(14.f, 5.f, 14.f, 3.f));

	FSlateFontInfo FontTitolo = Stile->Caratteri->Titolo(26.f, TEXT("Bold"));
	FontTitolo.LetterSpacing = 20;

	TSharedRef<SButton> Chiudi = Stile->Pulsante(LOCTEXT("Chiudi", "Chiudi"),
		FSimpleDelegate::CreateLambda([this]() { OnChiudi.ExecuteIfBound(); }), 18.f);

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
			.Padding(FMargin(44.f, 28.f))
			[
				SNew(SBox).WidthOverride(860.f).HeightOverride(700.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Titolo", "Taccuino di viaggio"))
						.Font(FontTitolo)
						.ColorAndOpacity(ValdorsoTema::Rubrica())
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 10.f, 0.f, 0.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth() [ Linguetta(0, LOCTEXT("SchedaLuoghi", "Luoghi")) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Stile->Diamante(6.f, FSlateColor(ValdorsoTema::OroScuro())) ]
						+ SHorizontalBox::Slot().AutoWidth() [ Linguetta(1, LOCTEXT("SchedaIscrizioni", "Iscrizioni")) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Stile->Diamante(6.f, FSlateColor(ValdorsoTema::OroScuro())) ]
						+ SHorizontalBox::Slot().AutoWidth() [ Linguetta(2, LOCTEXT("SchedaRune", "Rune")) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ Stile->Diamante(6.f, FSlateColor(ValdorsoTema::OroScuro())) ]
						+ SHorizontalBox::Slot().AutoWidth() [ Linguetta(3, LOCTEXT("SchedaStorie", "Storie")) ]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 6.f, 0.f, 14.f))
					[
						Stile->Separatore(320.f, ValdorsoTema::OroScuro())
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SAssignNew(Contenuto, SBox)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 16.f, 0.f, 0.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Aiuto", "J: chiudi  ·  1, 2, 3, 4: le schede"))
							.Font(Stile->Caratteri->Testo(15.f, TEXT("Italic")))
							.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox).WidthOverride(200.f) [ Chiudi ]
						]
						+ SHorizontalBox::Slot().FillWidth(1.f)
					]
				]
			]
		]
	];

	MostraScheda(0);
	RegisterActiveTimer(0.25f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoTaccuino::Controlla));
}

TSharedRef<SWidget> SValdorsoTaccuino::Linguetta(int32 Indice, const FText& Testo)
{
	TSharedRef<TWeakPtr<SButton>> Riferimento = MakeShared<TWeakPtr<SButton>>();
	TSharedRef<SButton> Pulsante = SNew(SButton)
		.ButtonStyle(&StileLinguetta)
		.IsFocusable(false)
		.OnClicked_Lambda([this, Indice]() { MostraScheda(Indice); return FReply::Handled(); })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(Testo)
				.Font(Stile->Caratteri->Titolo(17.f, TEXT("Bold")))
				.ColorAndOpacity_Lambda([this, Indice, Riferimento]()
				{
					const TSharedPtr<SButton> P = Riferimento->Pin();
					if (Scheda == Indice)
					{
						return FSlateColor(ValdorsoTema::Rubrica());
					}
					return FSlateColor(P.IsValid() && P->IsHovered() ? ValdorsoTema::Inchiostro() : ValdorsoTema::InchiostroTenue());
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))
			[
				SNew(SBox).HeightOverride(2.f)
				[
					SNew(SImage)
					.Image(&Stile->Pieno)
					.ColorAndOpacity(ValdorsoTema::Rubrica().CopyWithNewOpacity(0.8f))
					.Visibility_Lambda([this, Indice]() { return Scheda == Indice ? EVisibility::Visible : EVisibility::Hidden; })
				]
			]
		];
	*Riferimento = Pulsante;
	return Pulsante;
}

void SValdorsoTaccuino::MostraScheda(int32 Nuova)
{
	Scheda = FMath::Clamp(Nuova, 0, 3);
	Rifai();
}

void SValdorsoTaccuino::Rifai()
{
	const UValdorsoTaccuinoComponent* Diario = Taccuino.Get();
	CambiamentiVisti = Diario ? Diario->Cambiamenti() : -1;
	bAttendeTesto = false;
	if (!Contenuto.IsValid())
	{
		return;
	}
	if (Scheda == 1)
	{
		Contenuto->SetContent(PaginaIscrizioni());
	}
	else if (Scheda == 2)
	{
		Contenuto->SetContent(PaginaRune());
	}
	else if (Scheda == 3)
	{
		Contenuto->SetContent(PaginaStorie());
	}
	else
	{
		Contenuto->SetContent(PaginaLuoghi());
	}
}

EActiveTimerReturnType SValdorsoTaccuino::Controlla(double Tempo, float Delta)
{
	UValdorsoTaccuinoComponent* Diario = Taccuino.Get();
	if (!Diario)
	{
		return EActiveTimerReturnType::Continue;
	}
	if (Diario->Cambiamenti() != CambiamentiVisti)
	{
		Rifai();
	}
	else if (bAttendeTesto && Scheda == 1)
	{
		// ancora niente: se la richiesta si è persa, la rifà (la pagina si rifà quando il testo arriva)
		Diario->RigheIscrizione(IscrizioneScelta);
	}
	return EActiveTimerReturnType::Continue;
}

TSharedRef<SWidget> SValdorsoTaccuino::PaginaLuoghi()
{
	const UValdorsoTaccuinoComponent* Diario = Taccuino.Get();
	TSharedRef<SVerticalBox> Pagina = SNew(SVerticalBox);
	const int32 Scoperti = Diario ? Diario->LuoghiScoperti().Num() : 0;
	if (Scoperti == 0)
	{
		Pagina->AddSlot().AutoHeight().Padding(FMargin(0.f, 30.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NessunLuogo", "Il taccuino è ancora bianco. Cammina per la valle: i luoghi che trovi si annotano qui da soli, e da quel giorno la bussola li ricorda."))
			.Font(Stile->Caratteri->Testo(20.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
		];
		return Pagina;
	}
	const TWeakObjectPtr<APlayerController> DeboleControllore = Controllore;
	for (const FValdorsoLuogoScoperto& Scoperto : Diario->LuoghiScoperti())
	{
		const FValdorsoLuogo* Luogo = ValdorsoLuoghi::Trova(Scoperto.Nome);
		const bool bHaPosto = Luogo != nullptr;
		const FVector Posto = bHaPosto ? Luogo->Posizione : FVector::ZeroVector;
		Pagina->AddSlot().AutoHeight().Padding(FMargin(0.f, 6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(4.f, 0.f, 16.f, 0.f))
			[
				Stile->Diamante(8.f, FSlateColor(ValdorsoTema::OroScuro()))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Scoperto.Nome))
					.Font(Stile->Caratteri->Titolo(18.f, TEXT("Bold")))
					.ColorAndOpacity(ValdorsoTema::Inchiostro())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(TaccuinoQuando(Scoperto))
					.Font(Stile->Caratteri->Testo(16.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(16.f, 0.f, 4.f, 0.f))
			[
				// quanto è lontano adesso e verso dove (si aggiorna camminando)
				SNew(STextBlock)
				.Font(Stile->Caratteri->Testo(18.f))
				.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
				.Text_Lambda([DeboleControllore, bHaPosto, Posto]()
				{
					const APlayerController* Chi = DeboleControllore.Get();
					const APawn* Colono = Chi ? Chi->GetPawn() : nullptr;
					if (!Colono || !bHaPosto)
					{
						return FText::GetEmpty();
					}
					const FVector Qui = Colono->GetActorLocation();
					const double Distanza = FVector2D::Distance(FVector2D(Qui.X, Qui.Y), FVector2D(Posto.X, Posto.Y));
					if (Distanza < BussolaVicinoCm)
					{
						return LOCTEXT("SeiQui", "sei qui");
					}
					const FString Verso = ValdorsoBussola::Sigla(ValdorsoBussola::PuntoCardinale(ValdorsoBussola::RottaVerso(Qui, Posto)));
					return FText::FromString(FString::Printf(TEXT("%s  ·  %s"), *ValdorsoBussola::TestoDistanza(Distanza), *Verso));
				})
			]
		];
	}
	// quanti ne mancano (contando solo i luoghi che la valle ha davvero)
	int32 Conosciuti = 0;
	for (const FValdorsoLuogoScoperto& Scoperto : Diario->LuoghiScoperti())
	{
		Conosciuti += ValdorsoLuoghi::Trova(Scoperto.Nome) ? 1 : 0;
	}
	const int32 Mancano = FMath::Max(0, ValdorsoLuoghi::Tutti().Num() - Conosciuti);
	Pagina->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 14.f, 0.f, 6.f))
	[
		Stile->Separatore(160.f, ValdorsoTema::OroScuro())
	];
	Pagina->AddSlot().AutoHeight().HAlign(HAlign_Center)
	[
		SNew(STextBlock)
		.Text(Mancano > 1
			? FText::Format(LOCTEXT("Mancano", "Nella valle restano {0} luoghi da trovare."), FText::AsNumber(Mancano))
			: (Mancano == 1 ? LOCTEXT("MancaUno", "Nella valle resta un ultimo luogo da trovare.")
				: LOCTEXT("TuttiTrovati", "Hai trovato tutti i luoghi della valle.")))
		.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
		.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
	];
	return SNew(SScrollBox) + SScrollBox::Slot() [ Pagina ];
}

TSharedRef<SWidget> SValdorsoTaccuino::PaginaIscrizioni()
{
	UValdorsoTaccuinoComponent* Diario = Taccuino.Get();
	const TArray<FString> Titoli = Diario ? Diario->TitoliIscrizioni() : TArray<FString>();
	if (Titoli.Num() == 0)
	{
		return SNew(SBox).Padding(FMargin(0.f, 30.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NessunaIscrizione", "Nessuna iscrizione ricopiata. Le bacheche del villaggio e le pietre incise degli Antichi, una volta lette, restano qui da rileggere."))
				.Font(Stile->Caratteri->Testo(20.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
			];
	}
	if (!Titoli.IsValidIndex(IscrizioneScelta))
	{
		IscrizioneScelta = Titoli.Num() - 1;  // di serie l'ultima letta
	}

	// a sinistra l'elenco, dalla più nuova alla più vecchia
	TSharedRef<SVerticalBox> Elenco = SNew(SVerticalBox);
	for (int32 i = Titoli.Num() - 1; i >= 0; --i)
	{
		Elenco->AddSlot().AutoHeight()
		[
			SNew(SButton)
			.ButtonStyle(&StileLinguetta)
			.IsFocusable(false)
			.OnClicked_Lambda([this, i]() { IscrizioneScelta = i; Rifai(); return FReply::Handled(); })
			[
				SNew(STextBlock)
				.Text(FText::FromString(Titoli[i]))
				.Font(Stile->Caratteri->Testo(18.f, i == IscrizioneScelta ? TEXT("SemiBold") : TEXT("Regular")))
				.ColorAndOpacity(i == IscrizioneScelta ? ValdorsoTema::Rubrica() : ValdorsoTema::Inchiostro())
				.AutoWrapText(true)
			]
		];
	}

	// a destra il testo, come sulla pergamena della bacheca
	TSharedRef<SVerticalBox> Testo = SNew(SVerticalBox);
	Testo->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 12.f))
	[
		SNew(STextBlock)
		.Text(FText::FromString(Titoli[IscrizioneScelta]))
		.Font(Stile->Caratteri->Titolo(20.f, TEXT("Bold")))
		.ColorAndOpacity(ValdorsoTema::Rubrica())
		.Justification(ETextJustify::Center)
		.AutoWrapText(true)
	];
	if (const TArray<FString>* Righe = Diario->RigheIscrizione(IscrizioneScelta))
	{
		Testo->AddSlot().AutoHeight()
		[
			SValdorsoBacheca::ScriviRighe(*Righe, *Stile)
		];
	}
	else
	{
		bAttendeTesto = true;
		Testo->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Ricopio", "Sfogli le pagine..."))
			.Font(Stile->Caratteri->Testo(18.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
		];
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(250.f)
			[
				SNew(SScrollBox) + SScrollBox::Slot() [ Elenco ]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(18.f, 0.f))
		[
			SNew(SBox).WidthOverride(1.f)
			[
				SNew(SImage).Image(&Stile->Pieno).ColorAndOpacity(ValdorsoTema::OroScuro().CopyWithNewOpacity(0.5f))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(SScrollBox) + SScrollBox::Slot() [ Testo ]
		];
}

TSharedRef<SWidget> SValdorsoTaccuino::PaginaRune()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SValdorsoBacheca::ScriviRighe(ValdorsoRune::RigheQuaderno(), *Stile)
		];
}

TSharedRef<SWidget> SValdorsoTaccuino::PaginaStorie()
{
	const UValdorsoTaccuinoComponent* Diario = Taccuino.Get();
	const TArray<FValdorsoStoriaNelTaccuino> Storie = Diario ? Diario->PagineDelleStorie() : TArray<FValdorsoStoriaNelTaccuino>();
	if (Storie.Num() == 0)
	{
		return SNew(SBox).Padding(FMargin(0.f, 30.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NessunaStoria", "Nessuna storia, per ora. Il Battito intreccia storie per chi vive la valle: quando ne nasce una per te, si apre al centro dello schermo, e da quel giorno la ritrovi qui."))
			.Font(Stile->Caratteri->Testo(20.f, TEXT("Italic")))
			.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
		];
	}
	TSharedRef<SVerticalBox> Pagina = SNew(SVerticalBox);
	int32 GruppoPrima = -1;
	for (const FValdorsoStoriaNelTaccuino& Storia : Storie)
	{
		const EValdorsoStatoStoria Stato = static_cast<EValdorsoStatoStoria>(FMath::Min<uint8>(Storia.Stato, 5));
		const int32 Gruppo = Stato == EValdorsoStatoStoria::Accettata ? 0 : (Stato == EValdorsoStatoStoria::Offerta ? 1 : (Stato == EValdorsoStatoStoria::Finita ? 2 : 3));
		if (Gruppo != GruppoPrima)
		{
			GruppoPrima = Gruppo;
			const FText Titoli[] = { LOCTEXT("InCorso", "In corso"), LOCTEXT("DaDecidere", "Da decidere"), LOCTEXT("Finite", "Finite"), LOCTEXT("Lasciate", "Lasciate andare") };
			Pagina->AddSlot().AutoHeight().Padding(FMargin(0.f, Pagina->NumSlots() > 0 ? 14.f : 0.f, 0.f, 4.f))
			[
				SNew(STextBlock)
				.Text(Titoli[Gruppo])
				.Font(Stile->Caratteri->Titolo(16.f, TEXT("Bold")))
				.ColorAndOpacity(ValdorsoTema::Rubrica())
			];
		}
		const FLinearColor Colore = ValdorsoBattito::ColoreRarita(Storia.Rarita, Storia.Colore);
		const bool bSpenta = Gruppo == 3;
		const FString Sotto = Gruppo == 0 && Storia.Passi > 0
			? FString::Printf(TEXT("%s  ·  passo %d di %d"), *ValdorsoBattito::NomeRarita(Storia.Rarita), Storia.Passo, Storia.Passi)
			: ValdorsoBattito::NomeRarita(Storia.Rarita);
		Pagina->AddSlot().AutoHeight().Padding(FMargin(0.f, 5.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(FMargin(4.f, 6.f, 16.f, 0.f))
			[
				Stile->Diamante(9.f, FSlateColor(bSpenta ? Colore.CopyWithNewOpacity(0.4f) : Colore))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Storia.Titolo))
					.Font(Stile->Caratteri->Titolo(18.f, TEXT("Bold")))
					.ColorAndOpacity(bSpenta ? ValdorsoTema::InchiostroTenue() : ValdorsoTema::Inchiostro())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Sotto))
					.Font(Stile->Caratteri->Testo(14.f, TEXT("Italic")))
					.ColorAndOpacity(bSpenta ? Colore.CopyWithNewOpacity(0.5f) : Colore)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 2.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(Storia.Testo))
					.Font(Stile->Caratteri->Testo(17.f, Gruppo == 2 ? TEXT("Italic") : TEXT("Regular")))
					.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
					.AutoWrapText(true)
				]
			]
		];
	}
	return SNew(SScrollBox) + SScrollBox::Slot() [ Pagina ];
}

void SValdorsoTaccuino::ScorriIscrizioni(int32 Verso)
{
	const UValdorsoTaccuinoComponent* Diario = Taccuino.Get();
	const int32 Numero = Diario ? Diario->TitoliIscrizioni().Num() : 0;
	if (Scheda != 1 || Numero == 0)
	{
		return;
	}
	// l'elenco va dalla più nuova (in alto) alla più vecchia: giù = più vecchia
	const int32 Adesso = Diario->TitoliIscrizioni().IsValidIndex(IscrizioneScelta) ? IscrizioneScelta : Numero - 1;
	IscrizioneScelta = FMath::Clamp(Adesso - Verso, 0, Numero - 1);
	Rifai();
}

FReply SValdorsoTaccuino::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::J || Tasto == EKeys::Escape || Tasto == EKeys::F10 || Tasto == EKeys::E
		|| Tasto == EKeys::Gamepad_FaceButton_Right || Tasto == EKeys::Gamepad_Special_Right)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	if (Tasto == EKeys::One || Tasto == EKeys::Two || Tasto == EKeys::Three || Tasto == EKeys::Four)
	{
		MostraScheda(Tasto == EKeys::One ? 0 : (Tasto == EKeys::Two ? 1 : (Tasto == EKeys::Three ? 2 : 3)));
		return FReply::Handled();
	}
	if (Tasto == EKeys::Gamepad_LeftShoulder || Tasto == EKeys::Gamepad_RightShoulder)
	{
		MostraScheda((Scheda + (Tasto == EKeys::Gamepad_RightShoulder ? 1 : 3)) % 4);
		return FReply::Handled();
	}
	if (Tasto == EKeys::Down || Tasto == EKeys::Gamepad_DPad_Down || Tasto == EKeys::Up || Tasto == EKeys::Gamepad_DPad_Up)
	{
		ScorriIscrizioni(Tasto == EKeys::Down || Tasto == EKeys::Gamepad_DPad_Down ? 1 : -1);
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoTaccuino::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

#undef LOCTEXT_NAMESPACE
