// Valdorso - L'interfaccia su misura e i sottotitoli dei suoni (vedi ValdorsoInterfaccia.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoInterfaccia.h"
#include "ValdorsoImpostazioni.h"
#include "ValdorsoTemaUI.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "ValdorsoInterfaccia"

namespace
{
	/** Quante righe di sottotitoli al massimo, e per quanti secondi resta ciascuna. */
	constexpr int32 InterfacciaRighe = 3;
	constexpr double InterfacciaSecondi = 4.0;

	/** Le righe dei sottotitoli in basso: disegnate sopra il gioco, sotto gli avvisi. */
	class SValdorsoSottotitoli : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SValdorsoSottotitoli) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Caratteri = MakeShared<FValdorsoCaratteri>();
			SetVisibility(EVisibility::HitTestInvisible);
		}

		void Aggiungi(const FString& Testo)
		{
			const double Ora = FPlatformTime::Seconds();
			for (FRiga& R : Righe)
			{
				if (R.Testo == Testo)
				{
					R.Fine = Ora + InterfacciaSecondi;
					return;
				}
			}
			FRiga& Nuova = Righe.AddDefaulted_GetRef();
			Nuova.Testo = Testo;
			Nuova.Fine = Ora + InterfacciaSecondi;
			while (Righe.Num() > InterfacciaRighe)
			{
				Righe.RemoveAt(0);
			}
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(8.f, 8.f);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const double Ora = FPlatformTime::Seconds();
			const_cast<SValdorsoSottotitoli*>(this)->Righe.RemoveAll([Ora](const FRiga& R) { return R.Fine <= Ora; });
			if (Righe.Num() == 0 || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
			{
				return LayerId;
			}
			const FSlateFontInfo Carattere = Caratteri->Testo(17.f, TEXT("Italic"));
			const TSharedRef<FSlateFontMeasure> Misure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const FVector2f Area = AllottedGeometry.GetLocalSize();
			const FSlateBrush* Pieno = FCoreStyle::Get().GetBrush("WhiteBrush");
			// dal basso in su, sopra le barre (che stanno in fondo al centro)
			float Y = Area.Y - 230.f;
			for (int32 i = Righe.Num() - 1; i >= 0; --i)
			{
				const FRiga& R = Righe[i];
				const float Sfuma = FMath::Clamp(static_cast<float>(R.Fine - Ora) / 0.6f, 0.f, 1.f);
				const FVector2D Misura = Misure->Measure(R.Testo, Carattere);
				const FVector2f Angolo(Area.X * 0.5f - static_cast<float>(Misura.X) * 0.5f, Y - static_cast<float>(Misura.Y));
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(
					FVector2f(static_cast<float>(Misura.X) + 20.f, static_cast<float>(Misura.Y) + 6.f), FSlateLayoutTransform(Angolo - FVector2f(10.f, 3.f))),
					Pieno, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.55f * Sfuma));
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(
					FVector2f(Misura), FSlateLayoutTransform(Angolo)), R.Testo, Carattere, ESlateDrawEffect::None,
					ValdorsoTema::Pergamena().CopyWithNewOpacity(Sfuma));
				Y -= static_cast<float>(Misura.Y) + 10.f;
			}
			return LayerId + 2;
		}

	private:
		struct FRiga
		{
			FString Testo;
			double Fine = 0.0;
		};
		TArray<FRiga> Righe;
		TSharedPtr<FValdorsoCaratteri> Caratteri;
	};

	/** (lo tiene vivo la finestra del gioco: qui solo un riferimento debole, cosi' non resta niente alla chiusura) */
	TWeakObjectPtr<UWorld> InterfacciaMondo;
	TWeakPtr<SValdorsoSottotitoli> InterfacciaSchermo;

	TSharedPtr<SValdorsoSottotitoli> InterfacciaSottotitoli(UWorld* Mondo)
	{
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		if (!Viewport)
		{
			return nullptr;
		}
		TSharedPtr<SValdorsoSottotitoli> Schermo = InterfacciaSchermo.Pin();
		if (!Schermo.IsValid() || InterfacciaMondo.Get() != Mondo)
		{
			if (Schermo.IsValid())
			{
				Viewport->RemoveViewportWidgetContent(Schermo.ToSharedRef());
			}
			Schermo = SNew(SValdorsoSottotitoli);
			InterfacciaSchermo = Schermo;
			InterfacciaMondo = Mondo;
			// sopra il gioco e le barre (34), sotto il suggerimento (40) e gli avvisi (45)
			Viewport->AddViewportWidgetContent(Schermo.ToSharedRef(), 39);
		}
		return Schermo;
	}
}

namespace ValdorsoInterfaccia
{
	TAttribute<TOptional<FSlateRenderTransform>> Sposta(EValdorsoPezzoHud Pezzo)
	{
		return TAttribute<TOptional<FSlateRenderTransform>>::CreateLambda([Pezzo]()
		{
			const FValdorsoImpostazioni& I = ValdorsoImpostazioni::Correnti();
			float X = 0.f;
			float Y = 0.f;
			switch (Pezzo)
			{
			case EValdorsoPezzoHud::Barre: X = static_cast<float>(I.BarreX); Y = static_cast<float>(I.BarreY); break;
			case EValdorsoPezzoHud::Bussola: X = static_cast<float>(I.BussolaX); Y = static_cast<float>(I.BussolaY); break;
			default: X = static_cast<float>(I.ChatX); Y = static_cast<float>(I.ChatY); break;
			}
			if (FMath::IsNearlyEqual(I.GrandezzaHud, 1.f) && X == 0.f && Y == 0.f)
			{
				return TOptional<FSlateRenderTransform>();
			}
			return TOptional<FSlateRenderTransform>(FSlateRenderTransform(I.GrandezzaHud, FVector2f(X, Y)));
		});
	}

	FText ScrittaDelSuono(const FString& NomeSuono)
	{
		struct FScritta
		{
			const TCHAR* Inizio;
			const TCHAR* Testo;
		};
		static const FScritta Scritte[] = {
			{ TEXT("S_Lupo_Ululato"), TEXT("Un ululato") },
			{ TEXT("S_Lupo_Ringhio"), TEXT("Un lupo ringhia") },
			{ TEXT("S_Lupo_Guaito"), TEXT("Un lupo guaisce") },
			{ TEXT("S_Orso_Ruggito"), TEXT("Il ruggito di un orso") },
			{ TEXT("S_Cinghiale_Grugnito"), TEXT("Un cinghiale grugnisce") },
			{ TEXT("S_Cinghiale_Strillo"), TEXT("Lo strillo di un cinghiale") },
			{ TEXT("S_Cervo_Bramito"), TEXT("Un cervo bramisce") },
			{ TEXT("S_Volpe_Verso"), TEXT("Il verso di una volpe") },
			{ TEXT("S_Colpo_Lama"), TEXT("Colpi di lama") },
			{ TEXT("S_Colpo_Botta"), TEXT("Una botta") },
			{ TEXT("S_Colpo_Morso"), TEXT("Un morso") },
			{ TEXT("S_Colpo_Pugno"), TEXT("Un pugno") },
			{ TEXT("S_Colpo_Scudo"), TEXT("Un colpo sullo scudo") },
			{ TEXT("S_Campana"), TEXT("La campana del tempio") },
			{ TEXT("S_Ali"), TEXT("Grandi ali, in alto") },
			{ TEXT("S_Martelli"), TEXT("Martelli, sotto la montagna") },
			{ TEXT("S_Scheletro"), TEXT("Ossa che si muovono") },
			{ TEXT("S_Frana"), TEXT("La terra trema") },
			{ TEXT("S_Tuffo"), TEXT("Un tuffo") },
			{ TEXT("S_Gatto_Fusa"), TEXT("Un gatto fa le fusa") },
			{ TEXT("S_Gatto_Miao"), TEXT("Un gatto miagola") },
		};
		for (const FScritta& S : Scritte)
		{
			if (NomeSuono.StartsWith(S.Inizio))
			{
				return FText::FromString(S.Testo);
			}
		}
		return FText::GetEmpty();
	}

	FText DaDove(const FVector& Occhio, const FVector& Avanti, const FVector& Suono)
	{
		const FVector Verso = (Suono - Occhio).GetSafeNormal2D();
		const FVector Davanti = Avanti.GetSafeNormal2D();
		if (Verso.IsNearlyZero() || Davanti.IsNearlyZero())
		{
			return LOCTEXT("Vicino", "qui vicino");
		}
		const FVector Destra = FVector::CrossProduct(FVector::UpVector, Davanti);
		const float Avanzo = FVector::DotProduct(Verso, Davanti);
		const float Lato = FVector::DotProduct(Verso, Destra);
		if (Avanzo > 0.7f)
		{
			return LOCTEXT("Davanti", "davanti a te");
		}
		if (Avanzo < -0.7f)
		{
			return LOCTEXT("Dietro", "dietro di te");
		}
		return Lato > 0.f ? LOCTEXT("ADestra", "a destra") : LOCTEXT("ASinistra", "a sinistra");
	}

	void Sottotitolo(UWorld* Mondo, const FText& Scritta, const FVector& Dove, bool bVicino)
	{
		if (!Mondo || Scritta.IsEmpty() || Mondo->GetNetMode() == NM_DedicatedServer || !ValdorsoImpostazioni::Correnti().bSottotitoli)
		{
			return;
		}
		const APlayerController* Giocatore = Mondo->GetFirstPlayerController();
		if (!Giocatore || !Giocatore->IsLocalController())
		{
			return;
		}
		FVector Occhio;
		FRotator Sguardo;
		Giocatore->GetPlayerViewPoint(Occhio, Sguardo);
		const TSharedPtr<SValdorsoSottotitoli> Schermo = InterfacciaSottotitoli(Mondo);
		if (!Schermo.IsValid())
		{
			return;
		}
		const FString Testo = bVicino ? FString::Printf(TEXT("[%s]"), *Scritta.ToString())
			: FString::Printf(TEXT("[%s, %s]"), *Scritta.ToString(), *DaDove(Occhio, Sguardo.Vector(), Dove).ToString());
		Schermo->Aggiungi(Testo);
	}

	void SuonoSentito(UWorld* Mondo, const USoundBase* Suono, const FVector& Dove, float Lontano)
	{
		if (!Mondo || !Suono || Mondo->GetNetMode() == NM_DedicatedServer || !ValdorsoImpostazioni::Correnti().bSottotitoli)
		{
			return;
		}
		const FText Scritta = ScrittaDelSuono(Suono->GetName());
		if (Scritta.IsEmpty())
		{
			return;
		}
		const APlayerController* Giocatore = Mondo->GetFirstPlayerController();
		const APawn* Io = Giocatore ? Giocatore->GetPawn() : nullptr;
		if (!Io)
		{
			return;
		}
		// solo quello che si sente davvero (non al limite dell'udito)
		const float Distanza = static_cast<float>(FVector::Dist(Io->GetActorLocation(), Dove));
		if (Distanza > Lontano * 0.8f)
		{
			return;
		}
		Sottotitolo(Mondo, Scritta, Dove, Distanza < 250.f);
	}
}

#undef LOCTEXT_NAMESPACE
