// Valdorso - I sogni (v0.1.6, 11/11/2026). Scritto da Claude. Vedi ValdorsoSogni.h.

#include "ValdorsoSogni.h"
#include "Valdorso.h"
#include "ValdorsoCielo.h"
#include "ValdorsoGrottaNord.h"
#include "ValdorsoLetto.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoSuoni.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoTemaUI.h"
#include "ValdorsoVoci.h"
#include "Containers/Ticker.h"
#include "Misc/CoreDelegates.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	/** Ogni quanti secondi una riga nuova, e quanto resta il sogno dopo l'ultima. */
	constexpr double SogniSecondiRiga = 5.0;
	constexpr double SogniSecondiFine = 5.0;

	TArray<FValdorsoSogno> SogniTabella()
	{
		TArray<FValdorsoSogno> T;
		T.Add({ TEXT("primo"), TEXT("Il primo sogno"), {
			TEXT("Dormi, e la valle ti viene incontro da sola: il lago, il fiume, i tetti del villaggio, piccoli come su una mappa di carta."),
			TEXT("Poi il buio. E nel buio un suono che conosci senza averlo mai sentito: due colpi e una pausa. Due colpi e una pausa."),
			TEXT("Sulla collina a est c'e' un'ombra grande, ferma, che guarda il passo. Non ti fa paura. Sembra che ti aspettasse."),
			TEXT("L'ombra gira la testa verso di te. Senti una parola sola, piu' dentro che nelle orecchie: Ur."),
			TEXT("Ti svegli con il cuore che batte piano. Due colpi e una pausa.") } });
		T.Add({ TEXT("battito"), TEXT("Il battito che manca"), {
			TEXT("Sei nel tempio, ma il tempio e' vuoto: niente panche, niente candele. Solo l'altare, e sopra il frammento che brilla come brace."),
			TEXT("Due colpi e una pausa. Due colpi e una pausa. Due colpi... e la pausa non finisce."),
			TEXT("Il buio dura un respiro lungo come un inverno. Nel buio qualcuno, lontano, sta contando."),
			TEXT("Poi il colpo torna, piu' piano. Ti svegli con la mano sul petto, a sentire se il tuo c'e' ancora.") } });
		T.Add({ TEXT("martelli"), TEXT("I martelli"), {
			TEXT("Scendi una scala di pietra che non finisce. Ogni gradino e' piu' caldo di quello prima."),
			TEXT("In fondo c'e' una porta alta come tre case. Dall'altra parte, un colpo. Niente. Un altro colpo."),
			TEXT("Non sono martelli. Sono passi. Qualcuno, di la', cammina avanti e indietro da moltissimo tempo."),
			TEXT("Appoggi la mano alla porta, ed e' tiepida, come la pelle di chi dorme.") } });
		T.Add({ TEXT("luci"), TEXT("Le cinque luci"), {
			TEXT("Cammini su un prato, di notte, e ci sono cinque luci in cerchio: una fiamma, un'onda, un monte, una piuma, un sole."),
			TEXT("Ognuna ti chiama con una voce diversa, e tutte dicono il tuo nome come lo diceva tua madre."),
			TEXT("Al centro del cerchio c'e' una mano aperta. Ti avvicini per prenderla, e ti svegli.") } });
		T.Add({ TEXT("passo"), TEXT("La strada del passo"), {
			TEXT("Sei di nuovo sul carro della carovana, con la tessera di legno in tasca. Il velo di luce del passo si apre davanti a te come una tenda."),
			TEXT("Ti volti. Dietro, la strada da cui sei venuto non c'e' piu': solo nebbia, e nella nebbia qualcuno che ti saluta con la mano."),
			TEXT("Non ricordi chi sia. Ti svegli sicuro che, un tempo, lo sapevi.") } });
		T.Add({ TEXT("collina"), TEXT("La collina dell'Orso"), {
			TEXT("Sei sulla collina a est, all'alba. L'erba e' piegata, come se qualcuno di molto grande si fosse appena alzato."),
			TEXT("Nella terra c'e' un'impronta larga come una ruota di carro. Ci metti dentro la mano, ed e' calda."),
			TEXT("Il vento sa di neve, anche se e' estate. Da qualche parte, sotto i tuoi piedi, qualcosa respira.") } });
		return T;
	}

	/** Su questo PC: le righe del sogno nel buio, una alla volta, sopra il velo del sonno (1000). */
	class SValdorsoSogno : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SValdorsoSogno) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Caratteri = MakeShared<FValdorsoCaratteri>();
			Inizio = FPlatformTime::Seconds();
			SetVisibility(EVisibility::HitTestInvisible);
		}

		void Prepara(const FValdorsoSogno& Sogno)
		{
			Titolo = Sogno.Titolo;
			Righe.Reset();
			for (const TCHAR* Riga : Sogno.Righe)
			{
				Righe.Add(Riga);
			}
			Inizio = FPlatformTime::Seconds();
		}

		double Durata() const
		{
			return 2.0 + SogniSecondiRiga * Righe.Num() + SogniSecondiFine;
		}

		bool Finito() const
		{
			return FPlatformTime::Seconds() - Inizio > Durata();
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(8.f, 8.f);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			if (!FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
			{
				return LayerId;
			}
			const double Passato = FPlatformTime::Seconds() - Inizio;
			const double Totale = Durata();
			// il buio sale in due secondi e scende negli ultimi due
			const float Buio = static_cast<float>(FMath::Clamp(FMath::Min(Passato / 2.0, (Totale - Passato) / 2.0), 0.0, 1.0));
			const FVector2f Area = AllottedGeometry.GetLocalSize();
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), FCoreStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, FLinearColor(0.01f, 0.01f, 0.03f, 0.92f * Buio));
			const TSharedRef<FSlateFontMeasure> Misure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const FSlateFontInfo CarattereTitolo = Caratteri->Titolo(26.f);
			const FSlateFontInfo CarattereRiga = Caratteri->Testo(20.f, TEXT("Italic"));
			const FVector2D MisuraTitolo = Misure->Measure(Titolo, CarattereTitolo);
			const float Larghezza = FMath::Min(Area.X - 120.f, 900.f);
			float Y = Area.Y * 0.22f;
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(MisuraTitolo),
				FSlateLayoutTransform(FVector2f(Area.X * 0.5f - static_cast<float>(MisuraTitolo.X) * 0.5f, Y))), Titolo, CarattereTitolo,
				ESlateDrawEffect::None, ValdorsoTema::Oro().CopyWithNewOpacity(0.85f * Buio));
			Y += static_cast<float>(MisuraTitolo.Y) + 34.f;
			for (int32 i = 0; i < Righe.Num(); ++i)
			{
				const double Comparsa = 2.0 + SogniSecondiRiga * i;
				if (Passato < Comparsa)
				{
					break;
				}
				// ogni riga appare in un secondo e mezzo; le vecchie restano, piu' tenui
				const float Entrata = static_cast<float>(FMath::Clamp((Passato - Comparsa) / 1.5, 0.0, 1.0));
				const bool bUltima = i == Righe.Num() - 1 || Passato < Comparsa + SogniSecondiRiga;
				const float Alfa = Entrata * Buio * (bUltima ? 1.f : 0.45f);
				// a capo a mano: parole finche' ci stanno nella larghezza
				TArray<FString> Parole;
				Righe[i].ParseIntoArray(Parole, TEXT(" "));
				FString Pezzo;
				auto Scrivi = [&](const FString& Testo)
				{
					const FVector2D Misura = Misure->Measure(Testo, CarattereRiga);
					FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(Misura),
						FSlateLayoutTransform(FVector2f(Area.X * 0.5f - static_cast<float>(Misura.X) * 0.5f, Y))), Testo, CarattereRiga,
						ESlateDrawEffect::None, ValdorsoTema::Pergamena().CopyWithNewOpacity(Alfa));
					Y += static_cast<float>(Misura.Y) + 4.f;
				};
				for (const FString& Parola : Parole)
				{
					const FString Prova = Pezzo.IsEmpty() ? Parola : Pezzo + TEXT(" ") + Parola;
					if (!Pezzo.IsEmpty() && Misure->Measure(Prova, CarattereRiga).X > Larghezza)
					{
						Scrivi(Pezzo);
						Pezzo = Parola;
					}
					else
					{
						Pezzo = Prova;
					}
				}
				if (!Pezzo.IsEmpty())
				{
					Scrivi(Pezzo);
				}
				Y += 14.f;
			}
			return LayerId + 2;
		}

	private:
		TSharedPtr<FValdorsoCaratteri> Caratteri;
		FString Titolo;
		TArray<FString> Righe;
		double Inizio = 0.0;
	};

	TWeakObjectPtr<UGameViewportClient> SogniViewport;
	TWeakPtr<SValdorsoSogno> SogniSchermo;
	FTSTicker::FDelegateHandle SogniGiro;

	void SogniTogli()
	{
		if (SogniGiro.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(SogniGiro);
			SogniGiro.Reset();
		}
		const TSharedPtr<SValdorsoSogno> Schermo = SogniSchermo.Pin();
		if (UGameViewportClient* Viewport = SogniViewport.Get(); Viewport && Schermo.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(Schermo.ToSharedRef());
		}
		SogniSchermo.Reset();
		SogniViewport.Reset();
	}

	/** Solo sul server: l'ultimo sogno di ogni colono (secondi del mondo). */
	TMap<TWeakObjectPtr<APawn>, double>& SogniUltimi()
	{
		static TMap<TWeakObjectPtr<APawn>, double> Ultimi;
		return Ultimi;
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoSogniSogna(
		TEXT("Valdorso.Sogni.Sogna"),
		TEXT("Mostra un sogno su questo PC (prove): primo, battito, martelli, luci, passo, collina."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			ValdorsoSogni::Mostra(Mondo, FName(Argomenti.Num() > 0 ? *Argomenti[0] : TEXT("primo")));
		}));
}

namespace ValdorsoSogni
{
	const TArray<FValdorsoSogno>& Tutti()
	{
		static const TArray<FValdorsoSogno> Tabella = SogniTabella();
		return Tabella;
	}

	const FValdorsoSogno* Trova(FName Id)
	{
		for (const FValdorsoSogno& S : Tutti())
		{
			if (Id == FName(S.Id))
			{
				return &S;
			}
		}
		return nullptr;
	}

	FName Scegli(bool bPrimo, bool bNotte, float Caso, int32 Atto, int32 Frana, const TSet<FName>& GiaFatti, int32 Giro)
	{
		if (bPrimo)
		{
			return FName(TEXT("primo"));
		}
		if (!bNotte || Caso >= ProbabilitaSogno)
		{
			return NAME_None;
		}
		// prima i sogni del mondo che cambia, una volta sola
		if (Atto >= 1 && !GiaFatti.Contains(FName(TEXT("battito"))))
		{
			return FName(TEXT("battito"));
		}
		if (Frana >= 2 && !GiaFatti.Contains(FName(TEXT("martelli"))))
		{
			return FName(TEXT("martelli"));
		}
		static const TCHAR* Valle[] = { TEXT("luci"), TEXT("passo"), TEXT("collina") };
		return FName(Valle[FMath::Abs(Giro) % UE_ARRAY_COUNT(Valle)]);
	}

	void Addormentato(APawn* Chi)
	{
		UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
		APlayerController* Controllore = Chi ? Cast<APlayerController>(Chi->GetController()) : nullptr;
		UValdorsoSonnoComponent* Sonno = Chi ? Chi->FindComponentByClass<UValdorsoSonnoComponent>() : nullptr;
		if (!Mondo || !Controllore || !Sonno || !Chi->HasAuthority())
		{
			return;
		}
		UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Controllore);
		const FName Memoria(MemoriaSogni);
		const bool bPrimo = !Taccuino || !Taccuino->HaSegno(Memoria, FName(TEXT("primo")));
		const double Ora = Mondo->GetTimeSeconds();
		if (!bPrimo)
		{
			if (const double* Ultimo = SogniUltimi().Find(TWeakObjectPtr<APawn>(Chi)); Ultimo && Ora - *Ultimo < SecondiTraSogni)
			{
				return;
			}
		}
		float OraValle = 12.f;
		for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
		{
			OraValle = It->Ora();
			break;
		}
		TSet<FName> GiaFatti;
		for (const FValdorsoSogno& S : Tutti())
		{
			if (Taccuino && Taccuino->HaSegno(Memoria, FName(S.Id)))
			{
				GiaFatti.Add(FName(S.Id));
			}
		}
		const AValdorsoFrana* Frana = AValdorsoFrana::Di(Mondo);
		const FName Quale = Scegli(bPrimo, OraValle >= 20.f || OraValle < 6.f, FMath::FRand(), ValdorsoVoci::Atto(Mondo),
			Frana ? static_cast<int32>(Frana->GetStato()) : 0, GiaFatti, FMath::Rand());
		const FValdorsoSogno* Sogno = Trova(Quale);
		if (!Sogno)
		{
			return;
		}
		for (auto It = SogniUltimi().CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent();
			}
		}
		SogniUltimi().Add(TWeakObjectPtr<APawn>(Chi), Ora);
		if (Taccuino)
		{
			Taccuino->MettiSegno(Memoria, Quale);
			TArray<FString> Righe;
			for (const TCHAR* Riga : Sogno->Righe)
			{
				Righe.Add(Riga);
			}
			Taccuino->AnnotaIscrizione(FString::Printf(TEXT("Sogno: %s"), Sogno->Titolo), Righe);
		}
		Sonno->ClientSogno(Quale);
		ValdorsoRegistroEventi::Annota(TEXT("Sogno"), Chi, nullptr, Chi->GetActorLocation(), Sogno->Titolo);
	}

	void Mostra(UWorld* Mondo, FName Id)
	{
		SogniTogli();
		const FValdorsoSogno* Sogno = Trova(Id);
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		if (!Sogno || !Viewport || Mondo->GetNetMode() == NM_DedicatedServer)
		{
			return;
		}
		// all'uscita dal gioco con un sogno aperto: via il ticker prima che il modulo si spenga
		static bool bUscitaPronta = false;
		if (!bUscitaPronta)
		{
			bUscitaPronta = true;
			FCoreDelegates::OnPreExit.AddLambda([]() { SogniTogli(); });
		}
		const TSharedRef<SValdorsoSogno> Schermo = SNew(SValdorsoSogno);
		Schermo->Prepara(*Sogno);
		Viewport->AddViewportWidgetContent(Schermo, 1001);
		SogniSchermo = Schermo;
		SogniViewport = Viewport;
		ValdorsoSuoni::Suona(TEXT("S_Campana"), 0.35f);
		// via da solo alla fine (controllato due volte al secondo)
		SogniGiro = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			const TSharedPtr<SValdorsoSogno> Aperto = SogniSchermo.Pin();
			if (!Aperto.IsValid() || !SogniViewport.IsValid() || Aperto->Finito())
			{
				SogniGiro.Reset();
				SogniTogli();
				return false;
			}
			return true;
		}), 0.5f);
	}

	void Chiudi()
	{
		SogniTogli();
	}
}
