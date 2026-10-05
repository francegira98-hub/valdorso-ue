// Valdorso - Il controllo automatico delle scritte tagliate (idea di Claude del 05/10/2026, approvata da Fra).
// Scritto da Claude il 05/10/2026.
//
// Ogni test costruisce una schermata vera (menu, Prima di entrare, anticamera, codici di recupero, Registro dei coloni,
// Registro di Val d'Orso, volo), la riempie con i casi peggiori (nomi di 20 lettere, messaggi lunghi, le risposte con i
// vantaggi più lunghi) e la misura a 7 grandezze dello schermo, con la stessa scala dell'interfaccia del gioco.
// Diventa rosso se una scritta:
//   - è più larga o più alta dello spazio che ha (come "COLON" del 05/10);
//   - esce dallo schermo (tranne dentro le zone che scorrono).
// Il messaggio dice la grandezza dello schermo, la schermata, la scritta e di quanto esce.
// Il rapporto completo va in Saved/Schermate/Rapporto_<schermata>.txt.
//
// Le foto: con Tools\Foto delle schermate.cmd (che avvia Unreal con la grafica e con -ValdorsoFoto) ogni schermata
// viene anche salvata come immagine in Saved/Schermate, a 1280x720, 1920x1080 e 2560x1080.
// Con Tools\Test Valdorso.cmd (senza grafica) si fanno solo le misure.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SValdorsoMenuPrincipale.h"
#include "SValdorsoAccesso.h"
#include "SValdorsoPersonaggi.h"
#include "SValdorsoRegistro.h"
#include "SValdorsoVolo.h"
#include "ValdorsoRegistro.h"
#include "ValdorsoRegole.h"
#include "ValdorsoTemaUI.h"
#include "Engine/FontFace.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/UserInterfaceSettings.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "Input/HittestGrid.h"
#include "Layout/ArrangedChildren.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/CoreStyle.h"
#include "Types/PaintArgs.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SVirtualWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoTestSchermate"

namespace ValdorsoTestSchermate
{
	// Solo nell'editor e nel gioco: sul server e nei commandlet non ci sono schermate da misurare.
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	/** Quanto può sforare una scritta senza che conti (mezzo pixel per arrotondamenti, più un margine). */
	constexpr float Tolleranza = 1.f;

	/** Oltre questo numero di problemi per test, si scrive solo il conto (il resto è nel rapporto). */
	constexpr int32 MassimoErroriMostrati = 20;

	struct FRisoluzione
	{
		int32 X;
		int32 Y;
		bool bFoto;
	};

	/** Le grandezze dello schermo provate: 16:9 dal portatile al 4K, 16:10 e l'ultrawide. */
	const FRisoluzione Risoluzioni[] =
	{
		{ 1280,  720, true  },
		{ 1366,  768, false },
		{ 1920, 1080, true  },
		{ 1920, 1200, false },
		{ 2560, 1080, true  },
		{ 2560, 1440, false },
		{ 3840, 2160, false },
	};

	/** I nomi più lunghi permessi (20 caratteri), con lettere larghe. */
	const TCHAR* NomeLungo = TEXT("Massimiliano Mormile");
	const TCHAR* NomeLungo2 = TEXT("Guglielmina Wolmardo");
	const TCHAR* NomeLungo3 = TEXT("Bartolomeo Malaspina");
	const TCHAR* AccountLungo = TEXT("Massimiliano_Mormile");

	FString Breve(const FString& Testo)
	{
		FString Riga = Testo.Replace(TEXT("\r"), TEXT(" ")).Replace(TEXT("\n"), TEXT(" "));
		if (Riga.Len() > 70)
		{
			Riga = Riga.Left(67) + TEXT("...");
		}
		return Riga;
	}

	/** Un testo lungo fatto di parole vere, fino a circa Caratteri lettere. */
	FString TestoLungo(int32 Caratteri)
	{
		const FString Frase = TEXT("Sono arrivato nella valle con poche cose e molti ricordi, e la notte sento ancora il battito del Cuore sotto la terra. ");
		FString Out;
		while (Out.Len() + Frase.Len() <= Caratteri)
		{
			Out += Frase;
		}
		return Out.TrimEnd();
	}

	bool CaratteriPresenti()
	{
		return LoadObject<UFontFace>(nullptr, TEXT("/Game/UI/Font/Cinzel-Bold.Cinzel-Bold"), nullptr, LOAD_NoWarn | LOAD_Quiet) != nullptr
			|| LoadObject<UFontFace>(nullptr, TEXT("/Game/UI/Font/Cinzel_Bold.Cinzel_Bold"), nullptr, LOAD_NoWarn | LOAD_Quiet) != nullptr;
	}

	bool FotoAccese()
	{
		return FParse::Param(FCommandLine::Get(), TEXT("ValdorsoFoto")) && !FParse::Param(FCommandLine::Get(), TEXT("nullrhi"));
	}

	FString CartellaSchermate()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Schermate"));
	}

	/** Il ritratto finto del palco (stessa grandezza di quello vero), perché le schermate si dispongano come nel gioco. */
	UTextureRenderTarget2D* RitrattoFinto()
	{
		UTextureRenderTarget2D* Tela = NewObject<UTextureRenderTarget2D>();
		Tela->ClearColor = FLinearColor(0.08f, 0.06f, 0.05f, 1.f);
		Tela->InitAutoFormat(512, 640);
		return Tela;
	}
}

/**
 * Le misure. È "friend" delle schermate (vedi i loro .h): può aprire pannelli, schede e pagine come farebbe il giocatore.
 */
struct FValdorsoProvaSchermate
{
	FAutomationTestBase& Test;
	FString NomeTest;
	TArray<FString> Rapporto;
	int32 Errori = 0;
	int32 Misure = 0;

	FValdorsoProvaSchermate(FAutomationTestBase& InTest, const FString& InNome) : Test(InTest), NomeTest(InNome) {}

	/** Prima di cominciare: senza Slate o senza i caratteri le misure non valgono. */
	bool Pronto()
	{
		if (!FSlateApplication::IsInitialized())
		{
			Test.AddError(TEXT("Slate non è avviato: le schermate non si possono misurare."));
			return false;
		}
		if (!ValdorsoTestSchermate::CaratteriPresenti())
		{
			Test.AddError(TEXT("I caratteri di Content/UI/Font (Cinzel, EB Garamond) non si caricano: le misure non sarebbero quelle vere."));
			return false;
		}
		return true;
	}

	/** Dietro la schermata il fondo del tema (nel gioco c'è la scena 3D): serve solo alle foto. */
	static TSharedRef<SWidget> ConFondo(const TSharedRef<SWidget>& Schermata)
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage).Image(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).ColorAndOpacity(ValdorsoTema::Fondo())
			]
			+ SOverlay::Slot()
			[
				Schermata
			];
	}

	/** Un "disegno" senza schermo: serve perché le scritte che vanno a capo sappiano quanto spazio hanno. */
	static void Disegna(const TSharedRef<SVirtualWindow>& Finestra, const FGeometry& Radice, const FVector2D& Pixel)
	{
		FHittestGrid Griglia;
		Griglia.SetHittestArea(FVector2D::ZeroVector, Pixel);
		FSlateWindowElementList Elementi(StaticCastSharedRef<SWindow>(Finestra));
		FPaintArgs Argomenti(nullptr, Griglia, FVector2D::ZeroVector, FPlatformTime::Seconds(), 1.f / 60.f);
		Finestra->Paint(Argomenti, Radice, FSlateRect(0.f, 0.f, static_cast<float>(Pixel.X), static_cast<float>(Pixel.Y)), Elementi, 0, FWidgetStyle(), true);
	}

	/**
	 * Scende in tutti i widget visibili e annota le scritte che non ci stanno.
	 * Visibile: la parte dello schermo che si vede davvero (le zone che scorrono la restringono): le scritte fuori non
	 * vengono disegnate, quindi non sanno ancora dove andare a capo e non si misurano.
	 * Contenitore: lo spazio orizzontale di tutti i genitori insieme: una scritta che esce dal suo pannello conta
	 * anche se il suo pezzetto di spazio è abbastanza largo.
	 */
	static void Controlla(const TSharedRef<SWidget>& Widget, const FGeometry& Geometria, const FVector2D& Pixel, float Scala,
		bool bInZonaCheScorre, const FSlateRect& Visibile, const FSlateRect& Contenitore, TArray<FString>& OutProblemi)
	{
		using namespace ValdorsoTestSchermate;
		static const FName TipoTesto(TEXT("STextBlock"));
		static const FName TipoScorrimento(TEXT("SScrollBox"));

		const FName Tipo = Widget->GetType();
		const FSlateRect Rettangolo = Geometria.GetLayoutBoundingRect();
		// Margine in unità dell'interfaccia: la misura dei caratteri arrotonda ai pixel interi (a 720p un pixel vale 1,5).
		const float Margine = Tolleranza + 1.f / FMath::Max(Scala, 0.1f);
		// Lo stesso margine in pixel dello schermo.
		const float MarginePixel = Margine * Scala;

		if (Tipo == TipoTesto)
		{
			const FString Scritta = StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString();
			const bool bDisegnata = FSlateRect::DoRectanglesIntersect(Rettangolo, Visibile);
			if (!Scritta.TrimStartAndEnd().IsEmpty() && bDisegnata)
			{
				const FVector2D Voluto = Widget->GetDesiredSize();
				const FVector2D Spazio = Geometria.GetLocalSize();
				// Le scritte che vanno a capo misurano anche lo spazio in fondo alla riga, che non si vede:
				// per loro si accetta fino a 0,4 em in più (al primo giro davano 3-5 pixel su 21 punti).
				const float Em = StaticCastSharedRef<STextBlock>(Widget)->GetFont().Size * 96.f / 72.f;
				const bool bPiuRighe = Voluto.Y > Em * 1.9f;
				const float MargineX = Margine + (bPiuRighe ? Em * 0.4f : 0.f);
				// (05/10) Il difetto di "La / solitudine": una scritta che va a capo in uno spazio stretto quanto la sua
				// parola più lunga. Succede quando il genitore le dà solo la larghezza che chiede (HAlign Left, Right o
				// Center): lei chiede poco perché va a capo, e va a capo perché ha poco. Si sistema con HAlign_Fill.
				if (bPiuRighe)
				{
					const TSharedRef<FSlateFontMeasure> Misuratore = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
					const FSlateFontInfo Carattere = StaticCastSharedRef<STextBlock>(Widget)->GetFont();
					TArray<FString> Parole;
					Scritta.ParseIntoArrayWS(Parole);
					float ParolaPiuLarga = 0.f;
					for (const FString& Parola : Parole)
					{
						ParolaPiuLarga = FMath::Max(ParolaPiuLarga, static_cast<float>(Misuratore->Measure(Parola, Carattere).X));
					}
					if (Parole.Num() >= 2 && ParolaPiuLarga > 0.f && Spazio.X < ParolaPiuLarga * 1.6f)
					{
						OutProblemi.Add(FString::Printf(TEXT("\"%s\" va a capo quasi a ogni parola: ha solo %.0f di spazio (serve HAlign_Fill)"),
							*Breve(Scritta), Spazio.X));
					}
				}
				if (Voluto.X > Spazio.X + MargineX)
				{
					OutProblemi.Add(FString::Printf(TEXT("\"%s\" è larga %.0f ma ha %.0f: esce di %.0f"),
						*Breve(Scritta), Voluto.X, Spazio.X, Voluto.X - Spazio.X));
				}
				if (Voluto.Y > Spazio.Y + Margine)
				{
					OutProblemi.Add(FString::Printf(TEXT("\"%s\" è alta %.0f ma ha %.0f: è tagliata sotto di %.0f"),
						*Breve(Scritta), Voluto.Y, Spazio.Y, Voluto.Y - Spazio.Y));
				}
				if (Rettangolo.Left < Contenitore.Left - MarginePixel || Rettangolo.Right > Contenitore.Right + MarginePixel)
				{
					OutProblemi.Add(FString::Printf(TEXT("\"%s\" esce dal suo pannello (%.0f pixel a sinistra, %.0f a destra)"),
						*Breve(Scritta), FMath::Max(0.f, Contenitore.Left - Rettangolo.Left), FMath::Max(0.f, Rettangolo.Right - Contenitore.Right)));
				}
			}
			if (!Scritta.TrimStartAndEnd().IsEmpty() && !bInZonaCheScorre)
			{
				if (Rettangolo.Left < -MarginePixel || Rettangolo.Top < -MarginePixel
					|| Rettangolo.Right > Pixel.X + MarginePixel || Rettangolo.Bottom > Pixel.Y + MarginePixel)
				{
					OutProblemi.Add(FString::Printf(TEXT("\"%s\" esce dallo schermo (da %.0f,%.0f a %.0f,%.0f pixel)"),
						*Breve(Scritta), Rettangolo.Left, Rettangolo.Top, Rettangolo.Right, Rettangolo.Bottom));
				}
			}
		}

		const bool bScorre = bInZonaCheScorre || Tipo == TipoScorrimento;
		const FSlateRect VisibileFigli = (Tipo == TipoScorrimento) ? Visibile.IntersectionWith(Rettangolo) : Visibile;
		// Solo in orizzontale: in verticale alcuni pannelli usano margini negativi apposta.
		const FSlateRect ContenitoreFigli(FMath::Max(Contenitore.Left, Rettangolo.Left), Contenitore.Top,
			FMath::Min(Contenitore.Right, Rettangolo.Right), Contenitore.Bottom);

		FArrangedChildren Figli(EVisibility::Visible);
		Widget->ArrangeChildren(Geometria, Figli);
		for (int32 Indice = 0; Indice < Figli.Num(); ++Indice)
		{
			const FArrangedWidget& Figlio = Figli[Indice];
			Controlla(Figlio.Widget, Figlio.Geometry, Pixel, Scala, bScorre, VisibileFigli, ContenitoreFigli, OutProblemi);
		}
	}

	/** La foto di una schermata (solo con la grafica accesa, vedi FotoAccese). */
	static void Foto(const TSharedRef<SWidget>& Contenuto, const FVector2D& Pixel, float Scala, const FString& File)
	{
		TUniquePtr<FArchive> Scrittore(IFileManager::Get().CreateFileWriter(*File));
		if (!Scrittore.IsValid())
		{
			return;
		}
		// (05/10) Senza la correzione della luminosità: con "true" le foto uscivano più chiare del gioco.
		FWidgetRenderer Pittore(false, true);
		TStrongObjectPtr<UTextureRenderTarget2D> Tela(FWidgetRenderer::CreateTargetFor(Pixel, TF_Bilinear, false));
		if (!Tela.IsValid())
		{
			return;
		}
		// Tre disegni: le scritte che vanno a capo si sistemano dal secondo.
		for (int32 Giro = 0; Giro < 3; ++Giro)
		{
			Pittore.DrawWidget(Tela.Get(), Contenuto, Scala, Pixel, 1.f / 60.f);
		}
		FImageUtils::ExportRenderTarget2DAsPNG(Tela.Get(), *Scrittore);
	}

	/** Misura una schermata (in uno stato) a tutte le grandezze dello schermo. Stato: "tre personaggi", "pagina Fede"... */
	void Misura(const TSharedRef<SWidget>& Schermata, const FString& Stato)
	{
		using namespace ValdorsoTestSchermate;
		const TSharedRef<SWidget> Contenuto = ConFondo(Schermata);
		const bool bFoto = FotoAccese();

		for (const FRisoluzione& Risoluzione : Risoluzioni)
		{
			const FVector2D Pixel(Risoluzione.X, Risoluzione.Y);
			// La stessa scala che il gioco dà all'interfaccia a questa grandezza (Impostazioni progetto, Interfaccia utente).
			const float Scala = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(Risoluzione.X, Risoluzione.Y));

			TSharedRef<SVirtualWindow> Finestra = SNew(SVirtualWindow).Size(Pixel);
			Finestra->SetContent(Contenuto);
			const FGeometry Radice = FGeometry::MakeRoot(Pixel / Scala, FSlateLayoutTransform(Scala));

			// Misura, disegno, e di nuovo: le scritte che vanno a capo si sistemano in due o tre giri.
			for (int32 Giro = 0; Giro < 3; ++Giro)
			{
				Finestra->SlatePrepass(Scala);
				Disegna(Finestra, Radice, Pixel);
			}
			Finestra->SlatePrepass(Scala);

			TArray<FString> Problemi;
			const FSlateRect Schermo(0.f, 0.f, static_cast<float>(Pixel.X), static_cast<float>(Pixel.Y));
			Controlla(Finestra, Radice, Pixel, Scala, false, Schermo, Schermo, Problemi);
			++Misure;

			// La schermata torna libera (per la foto e per lo stato dopo).
			Finestra->SetContent(SNullWidget::NullWidget);

			for (const FString& Problema : Problemi)
			{
				const FString Riga = FString::Printf(TEXT("[%dx%d] %s, %s: %s"), Risoluzione.X, Risoluzione.Y, *NomeTest, *Stato, *Problema);
				Rapporto.Add(Riga);
				if (Errori < MassimoErroriMostrati)
				{
					Test.AddError(Riga);
				}
				++Errori;
			}

			if (bFoto && Risoluzione.bFoto)
			{
				FString NomeFile = FString::Printf(TEXT("%s - %s - %dx%d.png"), *NomeTest, *Stato, Risoluzione.X, Risoluzione.Y);
				NomeFile = FPaths::MakeValidFileName(NomeFile, TCHAR('_'));
				Foto(Contenuto, Pixel, Scala, FPaths::Combine(CartellaSchermate(), NomeFile));
			}
		}
	}

	/** Alla fine: il rapporto su file e il riassunto nel test. */
	bool Chiudi()
	{
		using namespace ValdorsoTestSchermate;
		if (Errori > MassimoErroriMostrati)
		{
			Test.AddError(FString::Printf(TEXT("... e altri %d problemi: tutti in Saved/Schermate/Rapporto_%s.txt"), Errori - MassimoErroriMostrati, *NomeTest));
		}
		TArray<FString> Righe;
		Righe.Add(FString::Printf(TEXT("Valdorso - controllo delle scritte: %s (%s)"), *NomeTest, *FDateTime::Now().ToString(TEXT("%d/%m/%Y %H:%M"))));
		Righe.Add(FString::Printf(TEXT("Misure fatte: %d. Problemi: %d."), Misure, Errori));
		Righe.Append(Rapporto);
		FFileHelper::SaveStringArrayToFile(Righe, *FPaths::Combine(CartellaSchermate(), FString::Printf(TEXT("Rapporto_%s.txt"), *NomeTest)),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		Test.AddInfo(FString::Printf(TEXT("%s: %d misure, %d problemi."), *NomeTest, Misure, Errori));
		return Errori == 0;
	}

	// --------------------------------------------------------------------------------------------
	// Le schermate, ognuna nei suoi stati
	// --------------------------------------------------------------------------------------------

	void Menu()
	{
		TSharedRef<SValdorsoMenuPrincipale> Schermata = SNew(SValdorsoMenuPrincipale);
		// La comparsa lenta è già finita.
		Schermata->Inizio = FPlatformTime::Seconds() - 60.0;
		// (05/10) Il menu parte trasparente e appare con un timer: nelle foto restava invisibile.
		Schermata->SetRenderOpacity(1.f);
		Misura(Schermata, TEXT("menu"));
		Schermata->MostraImpostazioni();
		Misura(Schermata, TEXT("Impostazioni"));
		Schermata->MostraRiconoscimenti();
		Misura(Schermata, TEXT("Riconoscimenti"));
	}

	void PrimaDiEntrare()
	{
		using namespace ValdorsoTestSchermate;
		const FText Errore = LOCTEXT("ErroreLungo", "Il nome o la password non vanno. Dopo 5 tentativi sbagliati l'account si chiude per 15 minuti: se hai perso la password, usa un codice di recupero.");

		TSharedRef<SValdorsoPrimaDiEntrare> Schermata = SNew(SValdorsoPrimaDiEntrare)
			.NomeIniziale(AccountLungo)
			.Messaggio(Errore);
		Misura(Schermata, TEXT("Entra"));

		Schermata->ScegliScheda(true);
		Schermata->MostraMessaggio(LOCTEXT("ErroreCodice", "Questo codice d'invito non esiste o è già stato usato. Chiedine uno nuovo allo staff sul Discord di Valdorso."), true);
		Misura(Schermata, TEXT("Primo ingresso"));

		Schermata->ScegliScheda(false);
		Schermata->ScegliRecupero(true);
		Schermata->MostraMessaggio(LOCTEXT("ErroreRecupero", "Il codice di recupero non è giusto o è già stato usato: ogni codice vale una volta sola. Controlla di averlo scritto bene."), true);
		Misura(Schermata, TEXT("Ho perso la password"));
	}

	void Anticamera()
	{
		TSharedRef<SValdorsoAnticamera> Schermata = SNew(SValdorsoAnticamera);
		Schermata->MostraAttesa(LOCTEXT("Attesa", "Il Cuore ti sta riconoscendo..."));
		Misura(Schermata, TEXT("attesa"));
		Schermata->MostraCambioPassword(LOCTEXT("ErrorePassword", "La password nuova deve avere almeno 12 caratteri, non può contenere il tuo nome e non può essere uguale a quella di prima."), true);
		Misura(Schermata, TEXT("password nuova"));
	}

	void CodiciRecupero()
	{
		TArray<FString> Codici;
		for (int32 Indice = 0; Indice < ValdorsoRegole::NumeroCodiciRecupero; ++Indice)
		{
			Codici.Add(TEXT("WMWM-MWMW-WMWM-MWMW"));
		}
		TSharedRef<SValdorsoCodiciRecupero> Schermata = SNew(SValdorsoCodiciRecupero).Codici(Codici);
		Misura(Schermata, TEXT("otto codici"));
	}

	void Personaggi()
	{
		using namespace ValdorsoTestSchermate;
		const int64 Ora = FDateTime::UtcNow().ToUnixTimestamp();

		TArray<FValdorsoPersonaggioBreve> Tre;
		{
			FValdorsoPersonaggioBreve& A = Tre.AddDefaulted_GetRef();
			A.Id = TEXT("prova-1"); A.Nome = NomeLungo; A.UltimoGioco = Ora; A.TempoDiGioco = (123 * 60 + 58) * 60;
			A.bRegistroFirmato = true; A.Sesso = TEXT("uomo"); A.Fede = TEXT("solara");
			FValdorsoPersonaggioBreve& B = Tre.AddDefaulted_GetRef();
			B.Id = TEXT("prova-2"); B.Nome = NomeLungo2; B.UltimoGioco = Ora; B.TempoDiGioco = 20;
			B.bRegistroFirmato = true; B.Sesso = TEXT("donna"); B.Fede = TEXT("nereia");
			FValdorsoPersonaggioBreve& C = Tre.AddDefaulted_GetRef();
			C.Id = TEXT("prova-3"); C.Nome = NomeLungo3; C.UltimoGioco = 0; C.TempoDiGioco = 0;
		}
		TArray<FValdorsoPersonaggioBreve> Uno;
		Uno.Add(Tre[0]);

		TStrongObjectPtr<UTextureRenderTarget2D> Ritratto(RitrattoFinto());
		TSharedRef<SValdorsoSceltaPersonaggio> Schermata = SNew(SValdorsoSceltaPersonaggio).Ritratto(Ritratto.Get());

		Schermata->Aggiorna(TArray<FValdorsoPersonaggioBreve>(), LOCTEXT("Benvenuto", "Benvenuto nella valle. Scrivi il nome del tuo primo colono: sarà unico, e tutti lo conosceranno così."), false);
		Misura(Schermata, TEXT("nessun personaggio"));

		Schermata->Aggiorna(Uno, LOCTEXT("NomePreso", "Il nome \"Guglielmina Wolmardo\" è già di un altro colono, o è riservato per 30 giorni dopo una cancellazione. Scegline un altro."), true);
		Misura(Schermata, TEXT("un personaggio e un errore"));

		Schermata->Aggiorna(Tre, FText::GetEmpty(), false);
		Misura(Schermata, TEXT("tre personaggi"));

		Schermata->IdDaCancellare = Tre[0].Id;
		Schermata->Ridisegna();
		Misura(Schermata, TEXT("cancellazione"));
	}

	void Registro()
	{
		using namespace ValdorsoTestSchermate;
		using ValdorsoRegistro::EDomanda;
		TStrongObjectPtr<UTextureRenderTarget2D> Ritratto(RitrattoFinto());

		for (const FValdorsoVoceRegistro& VoceSesso : ValdorsoRegistro::Voci(EDomanda::Sesso))
		{
			// Le risposte con il testo e il vantaggio più lunghi: il caso peggiore per ogni pagina.
			FValdorsoRegistro Dati;
			Dati.Sesso = VoceSesso.Chiave;
			Dati.Eta = ValdorsoRegistro::EtaMassima;
			for (int32 Indice = static_cast<int32>(EDomanda::Sesso) + 1; Indice < static_cast<int32>(EDomanda::Numero); ++Indice)
			{
				const EDomanda Domanda = static_cast<EDomanda>(Indice);
				int32 Lunghezza = -1;
				for (const FValdorsoVoceRegistro& Voce : ValdorsoRegistro::Voci(Domanda))
				{
					const TCHAR* Vantaggio = ValdorsoRegistro::Vantaggio(Domanda, Voce.Chiave);
					const int32 Questa = ValdorsoRegistro::Testo(Domanda, Voce.Chiave, Dati.Sesso).Len() + (Vantaggio ? FCString::Strlen(Vantaggio) : 0);
					if (Questa > Lunghezza)
					{
						Lunghezza = Questa;
						ValdorsoRegistro::Risposta(Dati, Domanda) = Voce.Chiave;
					}
				}
			}
			Dati.Storia = TestoLungo(ValdorsoRegistro::MassimoStoria);
			// (05/10) Una firma disegnata (un'onda in tre tratti), perché le foto la mostrino.
			{
				TArray<FString> Tratti;
				for (int32 Tratto = 0; Tratto < 3; ++Tratto)
				{
					TArray<FString> Punti;
					for (int32 i = 0; i < 30; ++i)
					{
						const float X = 0.08f + Tratto * 0.28f + i * 0.008f;
						const float Y = 0.55f + 0.18f * FMath::Sin(i * 0.45f + Tratto);
						Punti.Add(FString::Printf(TEXT("%d,%d"), FMath::RoundToInt(X * 999.f), FMath::RoundToInt(Y * 999.f)));
					}
					Tratti.Add(FString::Join(Punti, TEXT(" ")));
				}
				Dati.Firma = FString::Join(Tratti, TEXT(";"));
			}

			const FString Sesso = VoceSesso.Chiave;
			TSharedRef<SValdorsoRegistroColono> Schermata = SNew(SValdorsoRegistroColono)
				.NomePersonaggio(Sesso == TEXT("donna") ? NomeLungo2 : NomeLungo)
				.Registro(Dati)
				.Ritratto(Ritratto.Get());

			using EPagina = SValdorsoRegistroColono::EPagina;
			for (int32 Indice = 0; Indice < static_cast<int32>(EPagina::Numero); ++Indice)
			{
				const EPagina Pagina = static_cast<EPagina>(Indice);
				Schermata->VaiA(Pagina);
				// Il racconto si vede intero (non lettera per lettera).
				Schermata->FinisciScrittura();
				Misura(Schermata, FString::Printf(TEXT("%s, pagina %d (%s)"), *Sesso, Indice + 1, *Schermata->TitoloPagina(Pagina).ToString()));
			}

			// Un messaggio d'errore lungo sulla pagina più piena.
			Schermata->VaiA(EPagina::Firma);
			Schermata->FinisciScrittura();
			Schermata->MostraMessaggio(LOCTEXT("ErroreFirma", "Il registro non è partito: il server non risponde. Le risposte sono salvate come bozza e restano quando torni nella valle."), true);
			Misura(Schermata, FString::Printf(TEXT("%s, firma con un errore"), *Sesso));
		}
	}

	void Volo()
	{
		TSharedRef<SValdorsoVolo> Schermata = SNew(SValdorsoVolo);
		Schermata->ImpostaTitolo(1.f);
		Schermata->ImpostaRiga(LOCTEXT("RigaVolo", "In cammino verso la valle... il Cuore ti sta aspettando oltre il passo."), 1.f);
		Schermata->ImpostaSaltabile(true);
		Misura(Schermata, TEXT("primo ingresso"));
	}
};

// ------------------------------------------------------------------------------------------------
// I test: uno per schermata (filtro "Valdorso.Schermate")
// ------------------------------------------------------------------------------------------------

#define VALDORSO_TEST_SCHERMATA(Classe, Nome, Funzione) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(Classe, "Valdorso.Schermate." Nome, ValdorsoTestSchermate::Bandiere) \
	bool Classe::RunTest(const FString& Parameters) \
	{ \
		FValdorsoProvaSchermate Prova(*this, TEXT(Nome)); \
		if (!Prova.Pronto()) \
		{ \
			return false; \
		} \
		Prova.Funzione(); \
		return Prova.Chiudi(); \
	}

VALDORSO_TEST_SCHERMATA(FValdorsoTestSchermataMenu, "Menu", Menu)
VALDORSO_TEST_SCHERMATA(FValdorsoTestSchermataPrimaDiEntrare, "PrimaDiEntrare", PrimaDiEntrare)
VALDORSO_TEST_SCHERMATA(FValdorsoTestSchermataAnticamera, "Anticamera", Anticamera)
VALDORSO_TEST_SCHERMATA(FValdorsoTestSchermataCodici, "CodiciRecupero", CodiciRecupero)
VALDORSO_TEST_SCHERMATA(FValdorsoTestSchermataPersonaggi, "RegistroDeiColoni", Personaggi)
VALDORSO_TEST_SCHERMATA(FValdorsoTestSchermataRegistro, "RegistroDiValdOrso", Registro)
VALDORSO_TEST_SCHERMATA(FValdorsoTestSchermataVolo, "Volo", Volo)

#undef VALDORSO_TEST_SCHERMATA
#undef LOCTEXT_NAMESPACE

#endif // WITH_DEV_AUTOMATION_TESTS
