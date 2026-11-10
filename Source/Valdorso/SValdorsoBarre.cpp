// Valdorso - Le barre, la barra delle azioni e il velo della caduta: quello che si vede (vedi il .h). Scritto da Claude
// il 08/11/2026.

#include "SValdorsoBarre.h"
#include "ValdorsoBarre.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoTemaUI.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoBagnato.h"
#include "ValdorsoZaino.h"
#include "SValdorsoZaino.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"

#define LOCTEXT_NAMESPACE "ValdorsoBarre"

namespace
{
	TAutoConsoleVariable<int32> CVarBarreNumeri(
		TEXT("Valdorso.Barre.Numeri"),
		0,
		TEXT("1: accanto alle barre di salute, stamina, mana ed equilibrio si leggono i numeri. 0: solo le barre (di serie)."),
		ECVF_Default);

	/** Quanto è grande un testo sullo schermo (zero se Slate non c'è). */
	FVector2f BarreMisura(const FString& Testo, const FSlateFontInfo& Font)
	{
		if (!FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
		{
			return FVector2f::ZeroVector;
		}
		const TSharedRef<FSlateFontMeasure> Misuratore = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return FVector2f(Misuratore->Measure(Testo, Font));
	}

	/** Un testo con un'ombra scura sotto (si legge sopra il cielo e sopra la neve), centrato su Centro. */
	void BarreScrivi(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria, const FString& Testo,
		const FSlateFontInfo& Font, const FVector2f& Centro, const FLinearColor& Colore)
	{
		const FVector2f Misura = BarreMisura(Testo, Font);
		if (Misura.IsNearlyZero() || Colore.A <= 0.01f)
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

	/** Un rombo di linee (centro, mezza diagonale). */
	void BarreRombo(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria, const FVector2f& Centro,
		float Mezzo, const FLinearColor& Colore, float Spessore)
	{
		FSlateDrawElement::MakeLines(Elementi, Strato, Geometria.ToPaintGeometry(),
			TArray<FVector2f>{ Centro + FVector2f(0.f, -Mezzo), Centro + FVector2f(Mezzo, 0.f), Centro + FVector2f(0.f, Mezzo),
				Centro + FVector2f(-Mezzo, 0.f), Centro + FVector2f(0.f, -Mezzo) },
			ESlateDrawEffect::None, Colore, true, Spessore);
	}

	/** I colori delle quattro barre: salute (brace), stamina (oro), mana (blu arcano), equilibrio (pergamena). */
	FLinearColor BarreColore(int32 Indice)
	{
		switch (Indice)
		{
		case 0: return FMath::Lerp(ValdorsoTema::RossoSangue(), ValdorsoTema::Brace(), 0.35f);
		case 1: return ValdorsoTema::Oro();
		case 2: return ValdorsoTema::BluArcano();
		default: return ValdorsoTema::Pergamena();
		}
	}
}

// ------------------------------------------------------------------------------------------------- le barre

void SValdorsoBarre::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	// la cornice: un fondo scuro velato con un filo d'oro scuro (come la bussola), il riempimento tinto per ogni barra
	Cornice = FSlateRoundedBoxBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.6f), 3.f, ValdorsoTema::OroScuro().CopyWithNewOpacity(0.9f), 1.f);
	Riempimento = FSlateRoundedBoxBrush(FLinearColor::White, 2.f);
	SetVisibility(EVisibility::HitTestInvisible);
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoBarre::Anima));
}

EActiveTimerReturnType SValdorsoBarre::Anima(double Tempo, float Delta)
{
	Orologio = Tempo;
	const APlayerController* Chi = Controllore.Get();
	const APawn* Colono = Chi ? Chi->GetPawn() : nullptr;
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Colono);
	// con il colono caduto le barre si tolgono (c'è il velo)
	const bool bVoluto = ValdorsoBarre::SchermoLibero(Chi) && !(Pericoli && Pericoli->Caduto());
	PresenzaSchermo = FMath::FInterpConstantTo(PresenzaSchermo, bVoluto ? 1.f : 0.f, Delta, 4.f);
	// (08/11) la goccia: si vede finché si è un poco bagnati
	const UValdorsoBagnatoComponent* Acqua = UValdorsoBagnatoComponent::Di(Colono);
	Bagnato = Acqua ? Acqua->Bagnato() : 0.f;
	PresenzaGoccia = FMath::FInterpConstantTo(PresenzaGoccia, Bagnato > 0.02f ? 1.f : 0.f, Delta, 2.f);
	// (10/11) le bolle del fiato: si vedono mentre lo si trattiene (o lo si riprende)
	Fiato = Acqua ? Acqua->Fiato() : 1.f;
	PresenzaFiato = FMath::FInterpConstantTo(PresenzaFiato, Fiato < 0.995f ? 1.f : 0.f, Delta, 3.f);

	const UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Colono);
	if (!Abilita)
	{
		return EActiveTimerReturnType::Continue;
	}
	const FGameplayAttribute Valore[Quante] = {
		UValdorsoAttributeSet::GetSaluteAttribute(), UValdorsoAttributeSet::GetStaminaAttribute(),
		UValdorsoAttributeSet::GetManaAttribute(), UValdorsoAttributeSet::GetEquilibrioAttribute() };
	const FGameplayAttribute Massimo[Quante] = {
		UValdorsoAttributeSet::GetSaluteMaxAttribute(), UValdorsoAttributeSet::GetStaminaMaxAttribute(),
		UValdorsoAttributeSet::GetManaMaxAttribute(), UValdorsoAttributeSet::GetEquilibrioMaxAttribute() };
	for (int32 i = 0; i < Quante; ++i)
	{
		Valori[i] = Abilita->GetNumericAttribute(Valore[i]);
		Massimi[i] = Abilita->GetNumericAttribute(Massimo[i]);
		ValdorsoBarre::Aggiorna(Stati[i], Valori[i], Massimi[i], Tempo, Delta);
	}
	return EActiveTimerReturnType::Continue;
}

FVector2D SValdorsoBarre::ComputeDesiredSize(float) const
{
	return FVector2D(Larghezza + 2.f * Margine, Quante * Passo);
}

int32 SValdorsoBarre::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (PresenzaSchermo <= 0.01f)
	{
		return LayerId;
	}
	const float Tinta = InWidgetStyle.GetColorAndOpacityTint().A;
	const bool bNumeri = CVarBarreNumeri.GetValueOnGameThread() != 0;
	// (08/11) la goccia dei bagnati, a sinistra: il contorno blu, e dentro l'acqua che sale con quanto si è bagnati
	const float AlfaGoccia = PresenzaGoccia * PresenzaSchermo * Tinta;
	if (AlfaGoccia > 0.01f)
	{
		const FVector2f Fondo(16.f, Quante * Passo * 0.5f + 7.f);
		const float Raggio = 6.5f;
		TArray<FVector2f> Goccia;
		Goccia.Add(Fondo + FVector2f(0.f, -Raggio - 11.f));
		for (int32 k = 0; k <= 14; ++k)
		{
			// da destra a sinistra, passando sotto: l'arco della pancia della goccia
			const float Gradi = FMath::Lerp(-30.f, 210.f, k / 14.f);
			const float R = FMath::DegreesToRadians(Gradi);
			Goccia.Add(Fondo + FVector2f(Raggio * FMath::Cos(R), Raggio * FMath::Sin(R)));
		}
		Goccia.Add(Fondo + FVector2f(0.f, -Raggio - 11.f));
		const FLinearColor Blu = FMath::Lerp(ValdorsoTema::BluArcano(), ValdorsoTema::Pergamena(), 0.25f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Goccia,
			ESlateDrawEffect::None, Blu.CopyWithNewOpacity(0.9f * AlfaGoccia), true, 1.5f);
		const float Livello = FMath::Clamp(Bagnato, 0.f, 1.f);
		if (Livello > 0.02f)
		{
			const float Alto = (2.f * Raggio) * Livello;
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(Raggio * 1.3f, Alto), FSlateLayoutTransform(Fondo + FVector2f(-Raggio * 0.65f, Raggio - Alto))),
				&Riempimento, ESlateDrawEffect::None, ValdorsoTema::BluArcano().CopyWithNewOpacity((ValdorsoBagnato::Infreddolito(Bagnato) ? 0.85f : 0.5f) * AlfaGoccia));
		}
	}
	// (10/11) il fiato: dieci bolle sotto le barre, piene finche' c'e' fiato (rosse quando sta per finire)
	const float AlfaFiato = PresenzaFiato * PresenzaSchermo * Tinta;
	if (AlfaFiato > 0.01f)
	{
		const int32 Bolle = 10;
		const int32 Piene = FMath::CeilToInt(FMath::Clamp(Fiato, 0.f, 1.f) * Bolle);
		const FLinearColor Colore = Fiato < 0.3f ? FMath::Lerp(ValdorsoTema::RossoSangue(), ValdorsoTema::Pergamena(), 0.2f)
			: FMath::Lerp(ValdorsoTema::BluArcano(), ValdorsoTema::Pergamena(), 0.35f);
		for (int32 b = 0; b < Bolle; ++b)
		{
			const FVector2f Centro(Margine + 6.f + b * 14.f, Quante * Passo + 5.f);
			const float Raggio = 5.f;
			TArray<FVector2f> Giro;
			for (int32 k = 0; k <= 12; ++k)
			{
				const float R = 2.f * PI * k / 12.f;
				Giro.Add(Centro + FVector2f(Raggio * FMath::Cos(R), Raggio * FMath::Sin(R)));
			}
			const bool bPiena = b < Piene;
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Giro,
				ESlateDrawEffect::None, Colore.CopyWithNewOpacity((bPiena ? 0.95f : 0.35f) * AlfaFiato), true, 1.5f);
			if (bPiena)
			{
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
					AllottedGeometry.ToPaintGeometry(FVector2f(Raggio * 1.2f, Raggio * 1.2f), FSlateLayoutTransform(Centro - FVector2f(Raggio * 0.6f, Raggio * 0.6f))),
					&Riempimento, ESlateDrawEffect::None, Colore.CopyWithNewOpacity(0.45f * AlfaFiato));
			}
		}
	}
	const FSlateFontInfo FontNumeri = Caratteri->Titolo(9.f, TEXT("Regular"));
	for (int32 i = 0; i < Quante; ++i)
	{
		const FValdorsoStatoBarra& Stato = Stati[i];
		const float Alfa = Stato.Presenza * PresenzaSchermo * Tinta;
		if (Alfa <= 0.01f || Stato.Frazione < 0.f)
		{
			continue;
		}
		const float Y = i * Passo + (Passo - AltezzaBarra) * 0.5f;
		const FVector2f Inizio(Margine, Y);
		const FLinearColor Colore = BarreColore(i);

		// la cornice, un pixel più grande intorno alla barra
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(Larghezza + 2.f, AltezzaBarra + 2.f), FSlateLayoutTransform(Inizio - FVector2f(1.f, 1.f))),
			&Cornice, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, Alfa));
		// la scia del danno (color brace), tra il valore di adesso e quello di poco fa
		if (Stato.Scia > Stato.Frazione + 0.002f)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f((Stato.Scia - Stato.Frazione) * Larghezza, AltezzaBarra),
					FSlateLayoutTransform(Inizio + FVector2f(Stato.Frazione * Larghezza, 0.f))),
				&Riempimento, ESlateDrawEffect::None, ValdorsoTema::Brace().CopyWithNewOpacity(0.55f * Alfa));
		}
		// il riempimento, con un filo di luce in alto
		if (Stato.Frazione > 0.001f)
		{
			const float Lungo = Stato.Frazione * Larghezza;
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
				AllottedGeometry.ToPaintGeometry(FVector2f(Lungo, AltezzaBarra), FSlateLayoutTransform(Inizio)),
				&Riempimento, ESlateDrawEffect::None, Colore.CopyWithNewOpacity(0.92f * Alfa));
			if (Lungo > 4.f)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2f>{ Inizio + FVector2f(2.f, 1.f), Inizio + FVector2f(Lungo - 2.f, 1.f) },
					ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.22f * Alfa), true, 1.f);
			}
		}
		// un piccolo rombo del colore della barra, a sinistra
		BarreRombo(OutDrawElements, LayerId + 2, AllottedGeometry, FVector2f(Margine - 9.f, Y + AltezzaBarra * 0.5f), 3.f,
			Colore.CopyWithNewOpacity(0.85f * Alfa), 1.f);
		// i numeri (Valdorso.Barre.Numeri 1), a destra
		if (bNumeri)
		{
			const FString Testo = ValdorsoBarre::TestoNumeri(Valori[i], Massimi[i]);
			const FVector2f Misura = BarreMisura(Testo, FontNumeri);
			BarreScrivi(OutDrawElements, LayerId + 3, AllottedGeometry, Testo, FontNumeri,
				FVector2f(Margine + Larghezza + 6.f + Misura.X * 0.5f, Y + AltezzaBarra * 0.5f),
				ValdorsoTema::Pergamena().CopyWithNewOpacity(Alfa));
		}
	}
	return LayerId + 5;
}

// ------------------------------------------------------------------------------------------------- la barra delle azioni

void SValdorsoBarraAzioni::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Barre = InArgs._Barre;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Casella = FSlateRoundedBoxBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.55f), 4.f, ValdorsoTema::OroScuro().CopyWithNewOpacity(0.9f), 1.f);
	Pieno = FSlateRoundedBoxBrush(FLinearColor::White, 4.f);
	SetVisibility(EVisibility::HitTestInvisible);
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoBarraAzioni::Anima));
}

void SValdorsoBarraAzioni::Premi(int32 Indice, bool bVuoto)
{
	if (Indice < 0 || Indice >= ValdorsoBarre::NumeroAzioni)
	{
		return;
	}
	Lampi[Indice] = 1.f;
	if (bVuoto)
	{
		PostoScritta = Indice;
		// resta piena un poco più di mezzo secondo, poi sfuma in un secondo
		Scritta = 1.6f;
	}
}

EActiveTimerReturnType SValdorsoBarraAzioni::Anima(double Tempo, float Delta)
{
	const APlayerController* Chi = Controllore.Get();
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Chi ? Chi->GetPawn() : nullptr);
	const bool bVoluto = ValdorsoBarre::SchermoLibero(Chi) && !(Pericoli && Pericoli->Caduto());
	PresenzaSchermo = FMath::FInterpConstantTo(PresenzaSchermo, bVoluto ? 1.f : 0.f, Delta, 4.f);
	for (float& Lampo : Lampi)
	{
		Lampo = FMath::Max(0.f, Lampo - Delta / 0.45f);
	}
	Scritta = FMath::Max(0.f, Scritta - Delta);
	return EActiveTimerReturnType::Continue;
}

FVector2D SValdorsoBarraAzioni::ComputeDesiredSize(float) const
{
	return FVector2D(ValdorsoBarre::NumeroAzioni * Lato + (ValdorsoBarre::NumeroAzioni - 1) * Spazio, AltezzaScritta + Lato);
}

int32 SValdorsoBarraAzioni::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const float Alfa = PresenzaSchermo * InWidgetStyle.GetColorAndOpacityTint().A;
	if (Alfa <= 0.01f)
	{
		return LayerId;
	}
	const UValdorsoBarreComponent* Componente = Barre.Get();
	const FSlateFontInfo FontTasto = Caratteri->Titolo(12.f, TEXT("Bold"));
	const FSlateFontInfo FontAzione = Caratteri->Titolo(15.f, TEXT("Bold"));
	for (int32 i = 0; i < ValdorsoBarre::NumeroAzioni; ++i)
	{
		const FName Id = Componente ? Componente->Azione(i) : NAME_None;
		const bool bVuoto = Id.IsNone();
		const float Lampo = Lampi[i];
		// i posti vuoti sono tenui; quello appena premuto si accende
		const float Forza = FMath::Min(1.f, (bVuoto ? 0.5f : 1.f) + 0.5f * Lampo) * Alfa;
		const FVector2f Angolo(i * (Lato + Spazio), AltezzaScritta);
		const FVector2f Centro = Angolo + FVector2f(Lato * 0.5f, Lato * 0.5f);

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(Lato, Lato), FSlateLayoutTransform(Angolo)),
			&Casella, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, Forza));
		// il lampo: una luce di brace dentro e un filo d'oro chiaro intorno
		if (Lampo > 0.01f)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(Lato - 4.f, Lato - 4.f), FSlateLayoutTransform(Angolo + FVector2f(2.f, 2.f))),
				&Pieno, ESlateDrawEffect::None, ValdorsoTema::Brace().CopyWithNewOpacity(0.32f * Lampo * Alfa));
			const float L = Lato - 1.f;
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2f>{ Angolo + FVector2f(1.f, 1.f), Angolo + FVector2f(L, 1.f), Angolo + FVector2f(L, L),
					Angolo + FVector2f(1.f, L), Angolo + FVector2f(1.f, 1.f) },
				ESlateDrawEffect::None, ValdorsoTema::OroChiaro().CopyWithNewOpacity(Lampo * Alfa), true, 1.f);
		}
		// la runa vuota: un rombo d'oro scuro e, dentro, un rombo più piccolo e più tenue
		if (bVuoto)
		{
			BarreRombo(OutDrawElements, LayerId + 1, AllottedGeometry, Centro, Lato * 0.3f, ValdorsoTema::Oro().CopyWithNewOpacity(0.35f * Forza), 1.f);
			BarreRombo(OutDrawElements, LayerId + 1, AllottedGeometry, Centro, Lato * 0.14f, ValdorsoTema::Oro().CopyWithNewOpacity(0.2f * Forza), 1.f);
		}
		// il numero del tasto, in alto a sinistra
		BarreScrivi(OutDrawElements, LayerId + 3, AllottedGeometry, FString::FromInt(i + 1), FontTasto,
			Angolo + FVector2f(8.f, 9.f), ValdorsoTema::OroChiaro().CopyWithNewOpacity(0.85f * Forza));
		// (08/11) un oggetto dello zaino: il suo disegno a penna e, in basso a destra, quanti ne restano (tenue se finiti)
		const UValdorsoZainoComponent* Zaino = Componente ? Componente->ZainoDelGiocatore() : nullptr;
		if (!bVuoto && Zaino && ValdorsoOggetti::Trova(Id))
		{
			const int32 Quanti = ValdorsoZaino::Conta(Zaino->Contenuto(), Id);
			const float Tenue = Quanti > 0 ? 1.f : 0.35f;
			ValdorsoDisegnoOggetti::Disegna(OutDrawElements, LayerId + 2, AllottedGeometry, Id, Centro + FVector2f(0.f, 2.f), Lato * 0.6f,
				ValdorsoTema::Pergamena().CopyWithNewOpacity(Forza * Tenue), 1.5f);
			if (Quanti != 1)
			{
				BarreScrivi(OutDrawElements, LayerId + 3, AllottedGeometry, FString::FromInt(Quanti), FontTasto,
					Angolo + FVector2f(Lato - 8.f, Lato - 8.f), ValdorsoTema::OroChiaro().CopyWithNewOpacity(0.85f * Forza * Tenue));
			}
		}
		else if (!bVuoto)
		{
			// un'azione che non è un oggetto (le abilità, più avanti): le prime lettere del suo nome al centro
			BarreScrivi(OutDrawElements, LayerId + 3, AllottedGeometry, Id.ToString().Left(2), FontAzione,
				Centro, ValdorsoTema::Pergamena().CopyWithNewOpacity(Forza));
		}
	}
	// "Nessuna azione" sopra il posto premuto (senza uscire dai lati della barra)
	if (PostoScritta != INDEX_NONE && Scritta > 0.01f)
	{
		const FString Testo = LOCTEXT("NessunaAzione", "Nessuna azione").ToString();
		const FSlateFontInfo FontScritta = Caratteri->Testo(15.f, TEXT("Italic"));
		const FVector2f Misura = BarreMisura(Testo, FontScritta);
		const float Totale = ValdorsoBarre::NumeroAzioni * Lato + (ValdorsoBarre::NumeroAzioni - 1) * Spazio;
		const float X = FMath::Clamp(PostoScritta * (Lato + Spazio) + Lato * 0.5f, Misura.X * 0.5f, FMath::Max(Misura.X * 0.5f, Totale - Misura.X * 0.5f));
		BarreScrivi(OutDrawElements, LayerId + 3, AllottedGeometry, Testo, FontScritta, FVector2f(X, AltezzaScritta * 0.5f),
			ValdorsoTema::Pergamena().CopyWithNewOpacity(FMath::Clamp(Scritta, 0.f, 1.f) * Alfa));
	}
	return LayerId + 5;
}

// ------------------------------------------------------------------------------------------------- il velo della caduta

void SValdorsoVeloCaduta::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Pieno = FSlateColorBrush(FLinearColor::White);
	SetVisibility(EVisibility::HitTestInvisible);
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoVeloCaduta::Anima));
}

EActiveTimerReturnType SValdorsoVeloCaduta::Anima(double Tempo, float Delta)
{
	const APlayerController* Chi = Controllore.Get();
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Chi ? Chi->GetPawn() : nullptr);
	const bool bCaduto = Pericoli && Pericoli->Caduto();
	// scende in poco più di un secondo; si alza un poco più svelto (ma copre il primo istante al tempio)
	Buio = FMath::FInterpConstantTo(Buio, bCaduto ? 1.f : 0.f, Delta, bCaduto ? 1.f / 1.2f : 1.f / 0.9f);
	return EActiveTimerReturnType::Continue;
}

FVector2D SValdorsoVeloCaduta::ComputeDesiredSize(float) const
{
	return FVector2D(1.f, 1.f);
}

int32 SValdorsoVeloCaduta::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (Buio <= 0.01f)
	{
		return LayerId;
	}
	const FVector2f Misure = FVector2f(AllottedGeometry.GetLocalSize());
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		&Pieno, ESlateDrawEffect::None, ValdorsoTema::Fondo().CopyWithNewOpacity(0.86f * Buio));

	// la scritta arriva quando lo schermo è già scuro
	const float Scritta = FMath::Clamp((Buio - 0.4f) / 0.6f, 0.f, 1.f);
	if (Scritta <= 0.01f)
	{
		return LayerId + 1;
	}
	const FVector2f Centro(Misure.X * 0.5f, Misure.Y * 0.46f);
	BarreScrivi(OutDrawElements, LayerId + 1, AllottedGeometry,
		LOCTEXT("Caduto", "Sei caduto. Il Cuore ti riporta al tempio…").ToString(),
		Caratteri->Testo(30.f, TEXT("Italic")), Centro, ValdorsoTema::Pergamena().CopyWithNewOpacity(Scritta));
	// sotto, un filo d'oro con un rombo nel mezzo
	const float Y = Centro.Y + 30.f;
	const FLinearColor Filo = ValdorsoTema::Oro().CopyWithNewOpacity(0.5f * Scritta);
	for (const float Lato : { -1.f, 1.f })
	{
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>{ FVector2f(Centro.X + Lato * 10.f, Y), FVector2f(Centro.X + Lato * 150.f, Y) },
			ESlateDrawEffect::None, Filo, true, 1.f);
	}
	BarreRombo(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2f(Centro.X, Y), 4.f, ValdorsoTema::OroChiaro().CopyWithNewOpacity(0.8f * Scritta), 1.f);
	return LayerId + 3;
}

#undef LOCTEXT_NAMESPACE
