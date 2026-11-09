// Valdorso - Lo zaino: quello che si vede (vedi il .h). Scritto da Claude il 08/11/2026.

#include "SValdorsoZaino.h"
#include "ValdorsoZaino.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoTemaUI.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoZaino"

namespace
{
	/** Quanto è grande un testo sullo schermo (zero se Slate non c'è). */
	FVector2f ZainoMisura(const FString& Testo, const FSlateFontInfo& Font)
	{
		if (!FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
		{
			return FVector2f::ZeroVector;
		}
		const TSharedRef<FSlateFontMeasure> Misuratore = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return FVector2f(Misuratore->Measure(Testo, Font));
	}

	/** Un testo con l'angolo in alto a sinistra in Inizio. */
	void ZainoScrivi(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria, const FString& Testo,
		const FSlateFontInfo& Font, const FVector2f& Inizio, const FLinearColor& Colore)
	{
		const FVector2f Misura = ZainoMisura(Testo, Font);
		if (Misura.IsNearlyZero())
		{
			return;
		}
		FSlateDrawElement::MakeText(Elementi, Strato,
			Geometria.ToPaintGeometry(FVector2D(Misura), FSlateLayoutTransform(1.f, FVector2D(Inizio))),
			Testo, Font, ESlateDrawEffect::None, Colore);
	}

	/** Le linee di un disegno (coordinate da -1 a 1, y in su) dentro un quadrato. */
	void ZainoLinee(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria,
		const TArray<TArray<FVector2D>>& Linee, const FVector2f& Centro, float Lato, const FLinearColor& Colore, float Spessore)
	{
		const float Mezzo = Lato * 0.5f;
		for (const TArray<FVector2D>& Linea : Linee)
		{
			if (Linea.Num() < 2)
			{
				continue;
			}
			TArray<FVector2f> Punti;
			Punti.Reserve(Linea.Num());
			for (const FVector2D& P : Linea)
			{
				Punti.Add(Centro + FVector2f(static_cast<float>(P.X) * Mezzo, static_cast<float>(-P.Y) * Mezzo));
			}
			FSlateDrawElement::MakeLines(Elementi, Strato, Geometria.ToPaintGeometry(), Punti, ESlateDrawEffect::None, Colore, true, Spessore);
		}
	}

	constexpr int32 ZainoPrimoTasto = 100;

	/** La larghezza della griglia e quella della fila dei tasti (la più larga decide). */
	constexpr float ZainoLarghezzaGriglia = ValdorsoZaino::Colonne * SValdorsoGrigliaZaino::LatoPosto + (ValdorsoZaino::Colonne - 1) * SValdorsoGrigliaZaino::Spazio;
	constexpr float ZainoLarghezzaTasti = ValdorsoZaino::NumeroAzioni * SValdorsoGrigliaZaino::LatoTasto + (ValdorsoZaino::NumeroAzioni - 1) * SValdorsoGrigliaZaino::Spazio;
	constexpr float ZainoLarghezza = ZainoLarghezzaGriglia > ZainoLarghezzaTasti ? ZainoLarghezzaGriglia : ZainoLarghezzaTasti;
	constexpr float ZainoAltezzaGriglia = ValdorsoZaino::Righe * SValdorsoGrigliaZaino::LatoPosto + (ValdorsoZaino::Righe - 1) * SValdorsoGrigliaZaino::Spazio;

	/** Che cosa fa un oggetto, in una riga ("Clic destro: mangi (+30 stamina, +10 salute)"). */
	FString ZainoRigaUso(const FValdorsoTipoOggetto& Tipo)
	{
		TArray<FString> Effetti;
		if (Tipo.Salute > 0.f)
		{
			Effetti.Add(FString::Printf(TEXT("+%d salute"), FMath::RoundToInt(Tipo.Salute)));
		}
		if (Tipo.Stamina > 0.f)
		{
			Effetti.Add(FString::Printf(TEXT("+%d stamina"), FMath::RoundToInt(Tipo.Stamina)));
		}
		if (Tipo.Equilibrio > 0.f)
		{
			Effetti.Add(FString::Printf(TEXT("+%d equilibrio"), FMath::RoundToInt(Tipo.Equilibrio)));
		}
		const FString Tra = Effetti.Num() ? FString::Printf(TEXT(" (%s)"), *FString::Join(Effetti, TEXT(", "))) : FString();
		switch (Tipo.Uso)
		{
		case EValdorsoUso::Mangia: return TEXT("Clic destro: lo mangi") + Tra;
		case EValdorsoUso::Bevi: return TEXT("Clic destro: bevi un sorso") + Tra;
		case EValdorsoUso::Cura: return TEXT("Clic destro: ti medichi") + Tra;
		case EValdorsoUso::Leggi: return TEXT("Clic destro: lo leggi");
		case EValdorsoUso::Documento: return TEXT("Clic destro: la apri (e la puoi mostrare a chi hai davanti)");
		case EValdorsoUso::Accendi: return FString::Printf(TEXT("Clic destro: accendi un falò davanti a te (servono %d fasci di rami secchi)"),
			ValdorsoOggetti::RamiPerFalo);
		case EValdorsoUso::Impugna:
		{
			// (09/11) le armi: il danno e la portata, senza troppi numeri; lo scudo dice quanto ferma
			if (Tipo.Posto == EValdorsoPosto::ManoSinistra)
			{
				return FString::Printf(TEXT("Clic destro: lo imbracci nella sinistra (dei colpi che arrivano davanti ne ferma il %d%%)"),
					FMath::RoundToInt(Tipo.Parata * 100.f));
			}
			return FString::Printf(TEXT("Clic destro: la impugni nella destra (danno %d, portata %s m). Di nuovo: la rimetti via"),
				FMath::RoundToInt(Tipo.DannoArma), *FString::SanitizeFloat(FMath::RoundToFloat(Tipo.PortataArma / 10.f) / 10.f));
		}
		default: return FString();
		}
	}
}

// ------------------------------------------------------------------------------------------------- i disegni

namespace ValdorsoDisegnoOggetti
{
	void Disegna(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria, FName Id,
		const FVector2f& Centro, float Lato, const FLinearColor& Colore, float Spessore)
	{
		ZainoLinee(Elementi, Strato, Geometria, ValdorsoOggetti::Disegno(Id), Centro, Lato, Colore, Spessore);
	}

	void DisegnaMoneta(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria,
		const FVector2f& Centro, float Lato, const FLinearColor& Colore, float Spessore)
	{
		ZainoLinee(Elementi, Strato, Geometria, ValdorsoOggetti::DisegnoMoneta(), Centro, Lato, Colore, Spessore);
	}
}

// ------------------------------------------------------------------------------------------------- la griglia

void SValdorsoGrigliaZaino::Construct(const FArguments& InArgs)
{
	Zaino = InArgs._Zaino;
	Fuoco = InArgs._Fuoco;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Casella = FSlateRoundedBoxBrush(FLinearColor::White, 6.f, ValdorsoTema::OroScuro(), 1.5f);
}

FVector2D SValdorsoGrigliaZaino::ComputeDesiredSize(float) const
{
	return FVector2D(ZainoLarghezza, ZainoAltezzaGriglia + SopraTasti + LatoTasto);
}

FVector2f SValdorsoGrigliaZaino::AngoloPosto(int32 Posto)
{
	const int32 Colonna = Posto % ValdorsoZaino::Colonne;
	const int32 Riga = Posto / ValdorsoZaino::Colonne;
	const float Margine = (ZainoLarghezza - ZainoLarghezzaGriglia) * 0.5f;
	return FVector2f(Margine + Colonna * (LatoPosto + Spazio), Riga * (LatoPosto + Spazio));
}

FVector2f SValdorsoGrigliaZaino::AngoloTasto(int32 Tasto)
{
	const float Margine = (ZainoLarghezza - ZainoLarghezzaTasti) * 0.5f;
	return FVector2f(Margine + Tasto * (LatoTasto + Spazio), ZainoAltezzaGriglia + SopraTasti);
}

int32 SValdorsoGrigliaZaino::Colpito(const FVector2f& Punto)
{
	for (int32 i = 0; i < ValdorsoZaino::NumeroPosti; ++i)
	{
		const FVector2f A = AngoloPosto(i);
		if (Punto.X >= A.X && Punto.Y >= A.Y && Punto.X < A.X + LatoPosto && Punto.Y < A.Y + LatoPosto)
		{
			return i;
		}
	}
	for (int32 k = 0; k < ValdorsoZaino::NumeroAzioni; ++k)
	{
		const FVector2f A = AngoloTasto(k);
		if (Punto.X >= A.X && Punto.Y >= A.Y && Punto.X < A.X + LatoTasto && Punto.Y < A.Y + LatoTasto)
		{
			return ZainoPrimoTasto + k;
		}
	}
	return INDEX_NONE;
}

int32 SValdorsoGrigliaZaino::PostoMostrato() const
{
	return InMano != INDEX_NONE ? InMano : Sotto;
}

FReply SValdorsoGrigliaZaino::Gestito() const
{
	const TSharedPtr<SWidget> Chi = Fuoco.Pin();
	return Chi.IsValid() ? FReply::Handled().SetUserFocus(Chi.ToSharedRef(), EFocusCause::Mouse) : FReply::Handled();
}

bool SValdorsoGrigliaZaino::Posa()
{
	if (InMano == INDEX_NONE)
	{
		return false;
	}
	InMano = INDEX_NONE;
	return true;
}

FReply SValdorsoGrigliaZaino::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Mouse = FVector2f(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
	bMouseDentro = true;
	const int32 Dove = Colpito(Mouse);
	Sotto = (Dove != INDEX_NONE && Dove < ZainoPrimoTasto) ? Dove : INDEX_NONE;
	TastoSotto = Dove >= ZainoPrimoTasto ? Dove - ZainoPrimoTasto : INDEX_NONE;
	return FReply::Unhandled();
}

void SValdorsoGrigliaZaino::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SLeafWidget::OnMouseLeave(MouseEvent);
	bMouseDentro = false;
	Sotto = INDEX_NONE;
	TastoSotto = INDEX_NONE;
}

FReply SValdorsoGrigliaZaino::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UValdorsoZainoComponent* Il = Zaino.Get();
	if (!Il)
	{
		return FReply::Unhandled();
	}
	const FValdorsoZaino& Contenuto = Il->Contenuto();
	const int32 Dove = Colpito(FVector2f(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())));
	const bool bPosto = Dove != INDEX_NONE && Dove < ZainoPrimoTasto;
	const bool bTasto = Dove >= ZainoPrimoTasto;

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		InMano = INDEX_NONE;
		if (bPosto && Contenuto.Posti.IsValidIndex(Dove) && !Contenuto.Posti[Dove].Vuoto())
		{
			Il->ServerUsa(Dove);
		}
		else if (bTasto)
		{
			Il->ServerAssegna(Dove - ZainoPrimoTasto, INDEX_NONE);
		}
		return Gestito();
	}
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	if (InMano == INDEX_NONE)
	{
		if (bPosto && Contenuto.Posti.IsValidIndex(Dove) && !Contenuto.Posti[Dove].Vuoto())
		{
			InMano = Dove;
		}
		else if (bTasto)
		{
			// un clic su un tasto pieno lo usa, come premere il numero
			Il->ServerUsaAzione(Dove - ZainoPrimoTasto);
		}
		return Gestito();
	}
	if (bPosto && Dove != InMano)
	{
		Il->ServerSposta(InMano, Dove);
	}
	else if (bTasto)
	{
		Il->ServerAssegna(Dove - ZainoPrimoTasto, InMano);
	}
	InMano = INDEX_NONE;
	return Gestito();
}

int32 SValdorsoGrigliaZaino::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	if (!Il)
	{
		return LayerId;
	}
	const FValdorsoZaino& Contenuto = Il->Contenuto();
	const FLinearColor Inchiostro = ValdorsoTema::Inchiostro();
	const FLinearColor FondoPosto = ValdorsoTema::Hex(TEXT("D9C69C"));
	const FLinearColor FondoSopra = ValdorsoTema::Hex(TEXT("EBDDB8"));
	const FLinearColor FondoMano = ValdorsoTema::OroChiaro();
	const FSlateFontInfo FontQuanti = Caratteri->Titolo(13.f, TEXT("Bold"));
	const FSlateFontInfo FontTasto = Caratteri->Titolo(11.f, TEXT("Bold"));
	const FSlateFontInfo FontEtichetta = Caratteri->Testo(15.f, TEXT("Italic"));

	// (09/11) quello che si impugna: il primo posto con quell'oggetto ha il segno della mano
	int32 PostoDestra = INDEX_NONE;
	int32 PostoSinistra = INDEX_NONE;
	for (int32 i = 0; i < Contenuto.Posti.Num(); ++i)
	{
		if (PostoDestra == INDEX_NONE && !Contenuto.Destra.IsNone() && Contenuto.Posti[i].Id == Contenuto.Destra)
		{
			PostoDestra = i;
		}
		if (PostoSinistra == INDEX_NONE && !Contenuto.Sinistra.IsNone() && Contenuto.Posti[i].Id == Contenuto.Sinistra)
		{
			PostoSinistra = i;
		}
	}

	// --- la griglia
	for (int32 i = 0; i < ValdorsoZaino::NumeroPosti; ++i)
	{
		const FVector2f A = AngoloPosto(i);
		const FLinearColor Fondo = i == InMano ? FondoMano : (i == Sotto ? FondoSopra : FondoPosto);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(LatoPosto, LatoPosto), FSlateLayoutTransform(A)),
			&Casella, ESlateDrawEffect::None, Fondo);
		if (!Contenuto.Posti.IsValidIndex(i) || Contenuto.Posti[i].Vuoto())
		{
			continue;
		}
		const FValdorsoPostoZaino& Posto = Contenuto.Posti[i];
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Posto.Id);
		if (!Tipo)
		{
			continue;
		}
		// quello in mano resta un'ombra al suo posto
		const float Alfa = i == InMano ? 0.3f : 1.f;
		const FLinearColor Penna = (Tipo->bRicordo ? ValdorsoTema::Rubrica() : Inchiostro).CopyWithNewOpacity(Alfa);
		const FVector2f Centro = A + FVector2f(LatoPosto * 0.5f, LatoPosto * 0.46f);
		ValdorsoDisegnoOggetti::Disegna(OutDrawElements, LayerId + 1, AllottedGeometry, Posto.Id, Centro, LatoPosto * 0.62f, Penna, 2.f);
		if (Posto.Quantita > 1)
		{
			const FString Quanti = FString::FromInt(Posto.Quantita);
			const FVector2f Misura = ZainoMisura(Quanti, FontQuanti);
			ZainoScrivi(OutDrawElements, LayerId + 2, AllottedGeometry, Quanti, FontQuanti,
				A + FVector2f(LatoPosto - Misura.X - 6.f, LatoPosto - Misura.Y - 2.f), Inchiostro.CopyWithNewOpacity(Alfa));
		}
		if (Tipo->Cariche > 0)
		{
			// i sorsi: una tacca ciascuno, in fondo
			const float Larga = 6.f;
			const float Tra = 3.f;
			const float Tutte = Tipo->Cariche * Larga + (Tipo->Cariche - 1) * Tra;
			for (int32 c = 0; c < Tipo->Cariche; ++c)
			{
				const FVector2f Dove = A + FVector2f((LatoPosto - Tutte) * 0.5f + c * (Larga + Tra), LatoPosto - 9.f);
				const FLinearColor Tacca = c < Posto.Cariche ? ValdorsoTema::BluArcano() : ValdorsoTema::InchiostroTenue().CopyWithNewOpacity(0.3f);
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(FVector2f(Larga, 4.f), FSlateLayoutTransform(Dove)),
					&Casella, ESlateDrawEffect::None, Tacca.CopyWithNewOpacity(Tacca.A * Alfa));
			}
		}
		if (i == PostoDestra || i == PostoSinistra)
		{
			// in mano: una cornice d'oro e la lettera della mano, in alto a sinistra
			const FVector2f Q[5] = { A + FVector2f(2.f, 2.f), A + FVector2f(LatoPosto - 2.f, 2.f), A + FVector2f(LatoPosto - 2.f, LatoPosto - 2.f),
				A + FVector2f(2.f, LatoPosto - 2.f), A + FVector2f(2.f, 2.f) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), TArray<FVector2f>(Q, 5),
				ESlateDrawEffect::None, ValdorsoTema::OroScuro().CopyWithNewOpacity(Alfa), true, 2.f);
			ZainoScrivi(OutDrawElements, LayerId + 2, AllottedGeometry, i == PostoDestra ? TEXT("D") : TEXT("S"), FontTasto,
				A + FVector2f(6.f, 3.f), ValdorsoTema::OroScuro().CopyWithNewOpacity(Alfa));
		}
		if (Tipo->bRicordo)
		{
			// il segno dei ricordi: un piccolo rombo rosso in alto a destra
			const FVector2f C = A + FVector2f(LatoPosto - 9.f, 9.f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2f>{ C + FVector2f(0.f, -4.f), C + FVector2f(4.f, 0.f), C + FVector2f(0.f, 4.f), C + FVector2f(-4.f, 0.f), C + FVector2f(0.f, -4.f) },
				ESlateDrawEffect::None, ValdorsoTema::Rubrica().CopyWithNewOpacity(Alfa), true, 1.5f);
		}
	}

	// --- sopra i tasti: l'etichetta a sinistra, gli Aureus a destra
	const float RigaEtichetta = ZainoAltezzaGriglia + 8.f;
	ZainoScrivi(OutDrawElements, LayerId, AllottedGeometry, LOCTEXT("Barra", "La barra delle azioni (tasti 1-8)").ToString(), FontEtichetta,
		FVector2f(AngoloTasto(0).X, RigaEtichetta), ValdorsoTema::InchiostroTenue());
	{
		const FString Monete = FText::Format(LOCTEXT("Aureus", "{0} Aureus"), FText::AsNumber(Contenuto.Monete)).ToString();
		const FSlateFontInfo FontMonete = Caratteri->Titolo(14.f, TEXT("Bold"));
		const FVector2f Misura = ZainoMisura(Monete, FontMonete);
		const float Destra = AngoloTasto(ValdorsoZaino::NumeroAzioni - 1).X + LatoTasto;
		ZainoScrivi(OutDrawElements, LayerId, AllottedGeometry, Monete, FontMonete, FVector2f(Destra - Misura.X, RigaEtichetta), ValdorsoTema::OroScuro());
		ValdorsoDisegnoOggetti::DisegnaMoneta(OutDrawElements, LayerId, AllottedGeometry,
			FVector2f(Destra - Misura.X - 14.f, RigaEtichetta + Misura.Y * 0.5f), 20.f, ValdorsoTema::OroScuro(), 1.5f);
	}

	// --- i tasti della barra
	for (int32 k = 0; k < ValdorsoZaino::NumeroAzioni; ++k)
	{
		const FVector2f A = AngoloTasto(k);
		const bool bSopra = k == TastoSotto;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(LatoTasto, LatoTasto), FSlateLayoutTransform(A)),
			&Casella, ESlateDrawEffect::None, bSopra ? FondoSopra : ValdorsoTema::Hex(TEXT("CDB98C")));
		ZainoScrivi(OutDrawElements, LayerId + 2, AllottedGeometry, FString::FromInt(k + 1), FontTasto, A + FVector2f(5.f, 2.f), ValdorsoTema::InchiostroTenue());
		const FName Id = Contenuto.Azioni.IsValidIndex(k) ? Contenuto.Azioni[k] : NAME_None;
		if (Id.IsNone())
		{
			continue;
		}
		const int32 Quanti = ValdorsoZaino::Conta(Contenuto, Id);
		const float Alfa = Quanti > 0 ? 1.f : 0.35f;
		ValdorsoDisegnoOggetti::Disegna(OutDrawElements, LayerId + 1, AllottedGeometry, Id, A + FVector2f(LatoTasto * 0.5f, LatoTasto * 0.52f),
			LatoTasto * 0.6f, Inchiostro.CopyWithNewOpacity(Alfa), 1.8f);
		if (Quanti != 1)
		{
			const FString Testo = FString::FromInt(Quanti);
			const FVector2f Misura = ZainoMisura(Testo, FontTasto);
			ZainoScrivi(OutDrawElements, LayerId + 2, AllottedGeometry, Testo, FontTasto,
				A + FVector2f(LatoTasto - Misura.X - 4.f, LatoTasto - Misura.Y - 1.f), Inchiostro.CopyWithNewOpacity(Alfa));
		}
	}

	// --- quello in mano segue il mouse
	if (InMano != INDEX_NONE && bMouseDentro && Contenuto.Posti.IsValidIndex(InMano) && !Contenuto.Posti[InMano].Vuoto())
	{
		ValdorsoDisegnoOggetti::Disegna(OutDrawElements, LayerId + 4, AllottedGeometry, Contenuto.Posti[InMano].Id,
			Mouse + FVector2f(6.f, 6.f), LatoPosto * 0.62f, ValdorsoTema::Inchiostro().CopyWithNewOpacity(0.85f), 2.f);
	}
	return LayerId + 5;
}

// ------------------------------------------------------------------------------------------------- la pergamena

void SValdorsoZaino::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Zaino = InArgs._Zaino;
	OnChiudi = InArgs._OnChiudi;
	Caratteri = MakeShared<FValdorsoCaratteri>();
	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 4.f, ValdorsoTema::OroScuro(), 2.f);
	Velo = FSlateColorBrush(ValdorsoTema::Fondo().CopyWithNewOpacity(0.45f));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(&Velo)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&Carta)
			.Padding(FMargin(34.f, 24.f))
			[
				SNew(SVerticalBox)
				// il titolo e il peso
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 0.f, 0.f, 14.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Bottom)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Titolo", "Lo zaino"))
						.Font(Caratteri->Titolo(26.f, TEXT("Bold")))
						.ColorAndOpacity(ValdorsoTema::Inchiostro())
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Bottom)
					[
						SNew(STextBlock)
						.Text(this, &SValdorsoZaino::TestoPeso)
						.Font(Caratteri->Titolo(16.f, TEXT("Bold")))
						.ColorAndOpacity(this, &SValdorsoZaino::ColorePeso)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SAssignNew(Griglia, SValdorsoGrigliaZaino)
						.Zaino(Zaino)
						.Fuoco(SharedThis(this))
					]
					// l'oggetto mostrato
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(28.f, 0.f, 0.f, 0.f))
					[
						SNew(SBox)
						.WidthOverride(300.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(this, &SValdorsoZaino::NomeMostrato)
								.Font(Caratteri->Titolo(19.f, TEXT("Bold")))
								.ColorAndOpacity(ValdorsoTema::Inchiostro())
								.AutoWrapText(true)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(0.f, 8.f, 0.f, 0.f))
							[
								SNew(STextBlock)
								.Text(this, &SValdorsoZaino::DescrizioneMostrata)
								.Font(Caratteri->Testo(17.f, TEXT("Italic")))
								.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
								.AutoWrapText(true)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(0.f, 12.f, 0.f, 0.f))
							[
								SNew(STextBlock)
								.Text(this, &SValdorsoZaino::DettagliMostrati)
								.Font(Caratteri->Testo(16.f))
								.ColorAndOpacity(ValdorsoTema::Inchiostro())
								.AutoWrapText(true)
							]
						]
					]
				]
				// come si usa
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 16.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(this, &SValdorsoZaino::Suggerimento)
					.Font(Caratteri->Testo(15.f, TEXT("Italic")))
					.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
					.AutoWrapText(true)
				]
			]
		]
	];
}

FText SValdorsoZaino::TestoPeso() const
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	const float Kg = Il ? Il->Peso() : 0.f;
	FText Testo = FText::Format(LOCTEXT("Peso", "{0} / {1}"), FText::FromString(ValdorsoOggetti::TestoPeso(Kg)),
		FText::FromString(ValdorsoOggetti::TestoPeso(ValdorsoOggetti::PortataKg)));
	if (Kg > ValdorsoOggetti::SovraccaricoKg)
	{
		return FText::Format(LOCTEXT("PesoTroppo", "{0} · troppo carico: cammini piano"), Testo);
	}
	if (!ValdorsoOggetti::PuoCorrere(Kg))
	{
		return FText::Format(LOCTEXT("PesoPesante", "{0} · pesante: non corri"), Testo);
	}
	return Testo;
}

FSlateColor SValdorsoZaino::ColorePeso() const
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	const float Kg = Il ? Il->Peso() : 0.f;
	return ValdorsoOggetti::PuoCorrere(Kg) ? FSlateColor(ValdorsoTema::InchiostroTenue()) : FSlateColor(ValdorsoTema::Rubrica());
}

FText SValdorsoZaino::NomeMostrato() const
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	const int32 Posto = Griglia.IsValid() ? Griglia->PostoMostrato() : INDEX_NONE;
	if (!Il || !Il->Contenuto().Posti.IsValidIndex(Posto) || Il->Contenuto().Posti[Posto].Vuoto())
	{
		return LOCTEXT("Niente", "Quello che porti");
	}
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Il->Contenuto().Posti[Posto].Id);
	return Tipo ? FText::FromString(Tipo->Nome) : FText::GetEmpty();
}

FText SValdorsoZaino::DescrizioneMostrata() const
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	const int32 Posto = Griglia.IsValid() ? Griglia->PostoMostrato() : INDEX_NONE;
	if (!Il || !Il->Contenuto().Posti.IsValidIndex(Posto) || Il->Contenuto().Posti[Posto].Vuoto())
	{
		return LOCTEXT("NienteDesc", "Passa con il mouse su un oggetto per guardarlo da vicino.");
	}
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Il->Contenuto().Posti[Posto].Id);
	return Tipo ? FText::FromString(Tipo->Descrizione) : FText::GetEmpty();
}

FText SValdorsoZaino::DettagliMostrati() const
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	const int32 Posto = Griglia.IsValid() ? Griglia->PostoMostrato() : INDEX_NONE;
	if (!Il || !Il->Contenuto().Posti.IsValidIndex(Posto) || Il->Contenuto().Posti[Posto].Vuoto())
	{
		return FText::GetEmpty();
	}
	const FValdorsoPostoZaino& Dove = Il->Contenuto().Posti[Posto];
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Dove.Id);
	if (!Tipo)
	{
		return FText::GetEmpty();
	}
	TArray<FString> Righe;
	if (Dove.Quantita > 1)
	{
		Righe.Add(FString::Printf(TEXT("Peso: %s ciascuno, %s in tutto"), *ValdorsoOggetti::TestoPeso(Tipo->Peso),
			*ValdorsoOggetti::TestoPeso(Tipo->Peso * Dove.Quantita)));
	}
	else
	{
		Righe.Add(FString::Printf(TEXT("Peso: %s"), *ValdorsoOggetti::TestoPeso(Tipo->Peso)));
	}
	if (Tipo->Cariche > 0)
	{
		Righe.Add(FString::Printf(TEXT("Sorsi: %d su %d"), Dove.Cariche, Tipo->Cariche));
	}
	const FString Uso = ZainoRigaUso(*Tipo);
	if (!Uso.IsEmpty())
	{
		Righe.Add(Uso);
	}
	if (Tipo->bRicordo)
	{
		Righe.Add(TEXT("Un ricordo: non si getta e non si perde morendo."));
	}
	return FText::FromString(FString::Join(Righe, TEXT("\n")));
}

FText SValdorsoZaino::Suggerimento() const
{
	const UValdorsoZainoComponent* Il = Zaino.Get();
	if (Il && GettaPosto != INDEX_NONE && FPlatformTime::Seconds() < GettaFino && Il->Contenuto().Posti.IsValidIndex(GettaPosto))
	{
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Il->Contenuto().Posti[GettaPosto].Id);
		return FText::Format(LOCTEXT("GettaConferma", "Premi ancora Canc per lasciare qui: {0}. (Per ora non resta per terra.)"),
			FText::FromString(Tipo ? Tipo->Nome : TEXT("?")));
	}
	if (Griglia.IsValid() && Griglia->PostoInMano() != INDEX_NONE)
	{
		return LOCTEXT("InMano", "Clic su un posto: lo metti lì · clic su un tasto 1-8: va nella barra · Esc: lo rimetti giù.");
	}
	return LOCTEXT("Aiuto", "Clic: prendi e sposta · clic destro: usa · col mouse sopra, 1-8: nella barra · Canc due volte: lascia · I o Esc: chiudi.");
}

FReply SValdorsoZaino::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// un clic fuori dalla griglia rimette giù quello che si ha in mano
	if (Griglia.IsValid())
	{
		Griglia->Posa();
	}
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SValdorsoZaino::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// un tasto tenuto premuto non vale due volte (Canc due volte deve volerlo davvero)
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::Gamepad_FaceButton_Right)
	{
		if (Griglia.IsValid() && Griglia->Posa())
		{
			return FReply::Handled();
		}
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	if (Tasto == EKeys::I || Tasto == EKeys::Tab)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	UValdorsoZainoComponent* Il = Zaino.Get();
	const int32 Sotto = Griglia.IsValid() ? Griglia->PostoSotto() : INDEX_NONE;
	const FKey Numeri[ValdorsoZaino::NumeroAzioni] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six,
		EKeys::Seven, EKeys::Eight };
	for (int32 k = 0; k < ValdorsoZaino::NumeroAzioni; ++k)
	{
		if (Tasto == Numeri[k])
		{
			if (Il && Sotto != INDEX_NONE && Il->Contenuto().Posti.IsValidIndex(Sotto) && !Il->Contenuto().Posti[Sotto].Vuoto())
			{
				Il->ServerAssegna(k, Sotto);
			}
			return FReply::Handled();
		}
	}
	if (Tasto == EKeys::Delete || Tasto == EKeys::BackSpace)
	{
		if (Il && Sotto != INDEX_NONE && Il->Contenuto().Posti.IsValidIndex(Sotto) && !Il->Contenuto().Posti[Sotto].Vuoto())
		{
			const double Adesso = FPlatformTime::Seconds();
			if (GettaPosto == Sotto && Adesso < GettaFino)
			{
				Il->ServerGetta(Sotto);
				GettaPosto = INDEX_NONE;
			}
			else
			{
				GettaPosto = Sotto;
				GettaFino = Adesso + 3.0;
			}
		}
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
