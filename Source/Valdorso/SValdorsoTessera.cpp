// Valdorso - La tessera del colono: quello che si vede (vedi il .h). Scritto da Claude il 09/11/2026.

#include "SValdorsoTessera.h"
#include "SValdorsoAccesso.h"
#include "SValdorsoFirma.h"
#include "ValdorsoModoFoto.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoTemaUI.h"
#include "ValdorsoZaino.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "GroomComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "TextureResource.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdorsoTessera"

namespace
{
	/** Quanto è grande la foto sulla tessera (e la pellicola, in pixel: il doppio, per lo schermo grande). */
	constexpr float TesseraLatoFoto = 190.f;
	constexpr float TesseraAltezzaFoto = 238.f;
	constexpr int32 TesseraPixelLargo = 256;
	constexpr int32 TesseraPixelAlto = 320;

	/** L'inchiostro del timbro: rosso, un poco sbiadito. */
	FLinearColor TesseraRossoTimbro()
	{
		FLinearColor Colore = ValdorsoTema::RossoSangue();
		Colore.A = 0.82f;
		return Colore;
	}

	FVector2f TesseraMisura(const FString& Testo, const FSlateFontInfo& Font)
	{
		if (!FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
		{
			return FVector2f::ZeroVector;
		}
		const TSharedRef<FSlateFontMeasure> Misuratore = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return FVector2f(Misuratore->Measure(Testo, Font));
	}

	/** Un testo centrato in Centro, girato di Angolo (radianti). */
	void TesseraScriviGirato(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria, const FString& Testo,
		const FSlateFontInfo& Font, const FVector2f& Centro, float Angolo, const FLinearColor& Colore)
	{
		const FVector2f Misura = TesseraMisura(Testo, Font);
		if (Misura.IsNearlyZero())
		{
			return;
		}
		const FGeometry Pezzo = Geometria.MakeChild(FVector2D(Misura), FSlateLayoutTransform(1.f, FVector2D(Centro - Misura * 0.5f)),
			TransformCast<FSlateRenderTransform>(FQuat2D(Angolo)), FVector2D(0.5f, 0.5f));
		FSlateDrawElement::MakeText(Elementi, Strato, Pezzo.ToPaintGeometry(), Testo, Font, ESlateDrawEffect::None, Colore);
	}

	/** Un cerchio (o un'ellisse) a linee. */
	void TesseraCerchio(FSlateWindowElementList& Elementi, int32 Strato, const FGeometry& Geometria, const FVector2f& Centro,
		const FVector2f& Raggi, const FLinearColor& Colore, float Spessore, float Da = 0.f, float A = 2.f * PI)
	{
		constexpr int32 Lati = 64;
		TArray<FVector2f> Punti;
		Punti.Reserve(Lati + 1);
		for (int32 i = 0; i <= Lati; ++i)
		{
			const float Angolo = Da + (A - Da) * static_cast<float>(i) / Lati;
			Punti.Add(Centro + FVector2f(FMath::Cos(Angolo) * Raggi.X, FMath::Sin(Angolo) * Raggi.Y));
		}
		FSlateDrawElement::MakeLines(Elementi, Strato, Geometria.ToPaintGeometry(), Punti, ESlateDrawEffect::None, Colore, true, Spessore);
	}

	/** Un numero da 0 a 1 sempre uguale per lo stesso punto (la grana della carta). */
	float TesseraGrana(int32 X, int32 Y)
	{
		uint32 H = static_cast<uint32>(X) * 73856093u ^ static_cast<uint32>(Y) * 19349663u;
		H ^= H >> 13;
		H *= 0x5bd1e995u;
		H ^= H >> 15;
		return static_cast<float>(H & 0xFFFF) / 65535.f;
	}
}

/** Al posto della foto, quando il colono non c'è: la sagoma a penna di una testa e due spalle. */
class SValdorsoTesseraSagoma : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SValdorsoTesseraSagoma) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) {}
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(TesseraLatoFoto, TesseraAltezzaFoto); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2f Lato = FVector2f(AllottedGeometry.GetLocalSize());
		FLinearColor Penna = ValdorsoTema::InchiostroTenue();
		Penna.A = 0.55f;
		TesseraCerchio(OutDrawElements, LayerId, AllottedGeometry, FVector2f(Lato.X * 0.5f, Lato.Y * 0.38f), FVector2f(Lato.X * 0.2f, Lato.Y * 0.2f), Penna, 2.f);
		TesseraCerchio(OutDrawElements, LayerId, AllottedGeometry, FVector2f(Lato.X * 0.5f, Lato.Y * 1.02f), FVector2f(Lato.X * 0.42f, Lato.Y * 0.4f), Penna, 2.f, PI, 2.f * PI);
		return LayerId + 1;
	}
};

// ------------------------------------------------------------------------------------------------- il timbro

void SValdorsoTimbro::Construct(const FArguments& InArgs)
{
	Testo = InArgs._Testo;
	Caratteri = MakeShared<FValdorsoCaratteri>();
}

int32 SValdorsoTimbro::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Lato = FVector2f(AllottedGeometry.GetLocalSize());
	const FVector2f Centro = Lato * 0.5f;
	const float Raggio = FMath::Min(Lato.X, Lato.Y) * 0.5f - 2.f;
	const FLinearColor Rosso = TesseraRossoTimbro();

	TesseraCerchio(OutDrawElements, LayerId, AllottedGeometry, Centro, FVector2f(Raggio, Raggio), Rosso, 2.6f);
	TesseraCerchio(OutDrawElements, LayerId, AllottedGeometry, Centro, FVector2f(Raggio * 0.74f, Raggio * 0.74f), Rosso, 1.6f);

	// la scritta tutto intorno, fra i due cerchi, a partire dall'alto
	const FString Giro = TEXT("CAPITANERIA DEL PASSO · VALLE DELL'ORSO · ");
	const FSlateFontInfo FontGiro = Caratteri->Titolo(9.f);
	const float RaggioScritta = Raggio * 0.87f;
	for (int32 i = 0; i < Giro.Len(); ++i)
	{
		const float Angolo = -0.5f * PI + 2.f * PI * static_cast<float>(i) / Giro.Len();
		const FVector2f Dove = Centro + FVector2f(FMath::Cos(Angolo), FMath::Sin(Angolo)) * RaggioScritta;
		TesseraScriviGirato(OutDrawElements, LayerId, AllottedGeometry, FString(1, &Giro[i]), FontGiro, Dove, Angolo + 0.5f * PI, Rosso);
	}

	// la stella in alto e il testo nel mezzo (una riga per "\n")
	TArray<FVector2f> Stella;
	const FVector2f CentroStella = Centro - FVector2f(0.f, Raggio * 0.5f);
	for (int32 i = 0; i <= 10; ++i)
	{
		const float Angolo = -0.5f * PI + PI * static_cast<float>(i) / 5.f;
		const float R = (i % 2 == 0) ? 8.f : 3.4f;
		Stella.Add(CentroStella + FVector2f(FMath::Cos(Angolo), FMath::Sin(Angolo)) * R);
	}
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Stella, ESlateDrawEffect::None, Rosso, true, 1.6f);

	TArray<FString> Righe;
	Testo.ParseIntoArray(Righe, TEXT("\n"));
	const float Passo = 15.f;
	float Y = Centro.Y + 4.f - (Righe.Num() - 1) * Passo * 0.5f;
	for (int32 i = 0; i < Righe.Num(); ++i)
	{
		TesseraScriviGirato(OutDrawElements, LayerId, AllottedGeometry, Righe[i], Caratteri->Titolo(i == 0 ? 13.f : 10.f),
			FVector2f(Centro.X, Y), 0.f, Rosso);
		Y += Passo;
	}
	return LayerId + 1;
}

// ------------------------------------------------------------------------------------------------- la tessera

void SValdorsoTessera::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	Vista = InArgs._Vista;
	Di = InArgs._Di;
	bPropria = InArgs._bPropria;
	OnChiudi = InArgs._OnChiudi;
	Stile = MakeShared<FValdorsoStileAccesso>();

	Carta = FSlateRoundedBoxBrush(ValdorsoTema::Pergamena(), 6.f, ValdorsoTema::OroScuro(), 2.f);
	// (13/11, chiesto da Fra) la carta vera: pesante, con le fibre, piegata in tre, gli angoli consumati; e il sigillo di
	// ceralacca della capitana al posto del timbro (Tools/Generatori/icone/tessera.py, importate da importa_icone.py).
	// Se mancano, il foglio liscio e il timbro d'inchiostro di prima.
	TextureCarta.Reset(ValdorsoCaricaTextureUI(TEXT("/Game/UI/Icone/T_Tessera_Carta.T_Tessera_Carta")));
	if (TextureCarta.IsValid())
	{
		Carta = FSlateBrush();
		Carta.SetResourceObject(TextureCarta.Get());
		Carta.ImageSize = FVector2D(1024.f, 720.f);
		Carta.DrawAs = ESlateBrushDrawType::Image;
	}
	TextureCeralacca.Reset(ValdorsoCaricaTextureUI(TEXT("/Game/UI/Icone/T_Ceralacca.T_Ceralacca")));
	if (TextureCeralacca.IsValid())
	{
		Ceralacca.SetResourceObject(TextureCeralacca.Get());
		Ceralacca.ImageSize = FVector2D(256.f, 320.f);
		Ceralacca.DrawAs = ESlateBrushDrawType::Image;
	}
	Velo = FSlateColorBrush(FLinearColor(0.f, 0.f, 0.f, 0.62f));
	Cornice = FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.f), 2.f, ValdorsoTema::Inchiostro(), 2.f);

	const FLinearColor Inchiostro = ValdorsoTema::Inchiostro();
	const FLinearColor Tenue = ValdorsoTema::InchiostroTenue();
	const bool bDonna = Vista.Sesso == TEXT("donna");
	const FString Nome = Vista.Nome.IsEmpty() ? FString(TEXT("Colono senza nome")) : Vista.Nome;

	// le righe a destra della foto
	TSharedRef<SVerticalBox> Dati = SNew(SVerticalBox);
	auto Aggiungi = [&Dati](TSharedRef<SWidget> Contenuto)
	{
		Dati->AddSlot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 7.f))[Contenuto];
	};
	Aggiungi(Riga(LOCTEXT("Nome", "Nome"), Nome, true));
	Aggiungi(Riga(LOCTEXT("Eta", "Età"), Vista.Eta > 0 ? FString::Printf(TEXT("%d anni"), Vista.Eta) : FString()));
	Aggiungi(Riga(bDonna ? LOCTEXT("Venuta", "Venuta da") : LOCTEXT("Venuto", "Venuto da"), Vista.Origine));
	Aggiungi(Riga(LOCTEXT("Mestiere", "Già"), Vista.Mestiere));
	Aggiungi(Riga(LOCTEXT("Fede", "Fede"), Vista.Fede));
	Aggiungi(Riga(LOCTEXT("Qualifica", "Qualifica"), Vista.Tessera.Qualifica, true));
	Aggiungi(Riga(LOCTEXT("Rilascio", "Rilasciata"), Vista.Tessera.Rilascio));
	if (Vista.Tessera.Timbri.Num() > 0)
	{
		Aggiungi(Riga(LOCTEXT("Timbri", "Timbri"), FString::Join(Vista.Tessera.Timbri, TEXT("  ·  "))));
	}
	if (Vista.Tessera.Note.Num() > 0)
	{
		Aggiungi(Riga(LOCTEXT("Note", "Note"), FString::Join(Vista.Tessera.Note, TEXT("  ·  "))));
	}

	// in fondo: la mostri (la tua) o chi te la mostra (quella di un altro)
	TSharedRef<SWidget> Fondo = bPropria
		? StaticCastSharedRef<SWidget>(Stile->Pulsante(LOCTEXT("Mostra", "Mostrala a chi hai davanti"),
			FSimpleDelegate::CreateSP(this, &SValdorsoTessera::Mostra), 20.f))
		: StaticCastSharedRef<SWidget>(SNullWidget::NullWidget);

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(&Velo)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 12.f))
			[
				SNew(STextBlock)
				.Visibility(bPropria ? EVisibility::Collapsed : EVisibility::HitTestInvisible)
				.Text(FText::Format(LOCTEXT("TiMostra", "{0} ti mostra la sua tessera"), FText::FromString(Nome)))
				.Font(Stile->Caratteri->Testo(22.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::Pergamena())
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(800.f)
				// la tessera è un foglio appoggiato un poco storto, come sul tavolo
				.RenderTransform(TransformCast<FSlateRenderTransform>(FQuat2D(FMath::DegreesToRadians(-0.8f))))
				.RenderTransformPivot(FVector2D(0.5f, 0.5f))
				[
					SNew(SBorder)
					.BorderImage(&Carta)
					.Padding(FMargin(34.f, 24.f, 34.f, 22.f))
					[
						SNew(SVerticalBox)
						// l'intestazione
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Corona", "CORONA DI AURELIA"))
							.Font(Stile->Caratteri->Titolo(15.f))
							.ColorAndOpacity(Tenue)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 2.f, 0.f, 0.f))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Titolo", "Tessera del colono"))
							.Font(Stile->Caratteri->Titolo(24.f))
							.ColorAndOpacity(ValdorsoTema::Rubrica())
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 8.f, 0.f, 16.f))
						[
							Stile->Separatore(560.f, Tenue)
						]
						// la foto con il numero, e i dati
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(FMargin(0.f, 0.f, 28.f, 0.f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SBox)
									.WidthOverride(TesseraLatoFoto)
									.HeightOverride(TesseraAltezzaFoto)
									[
										SNew(SOverlay)
										+ SOverlay::Slot()
										[
											SNew(SImage)
											.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
											.ColorAndOpacity(FLinearColor(0.83f, 0.74f, 0.56f, 1.f))
										]
										+ SOverlay::Slot()
										[
											SNew(SValdorsoTesseraSagoma)
											.Visibility_Lambda([this]() { return bFotoPronta ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
										]
										+ SOverlay::Slot()
										[
											SNew(SImage)
											.Image(&Foto)
											.Visibility_Lambda([this]() { return bFotoPronta ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
										]
										+ SOverlay::Slot()
										[
											SNew(SImage).Image(&Cornice)
										]
									]
								]
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 8.f, 0.f, 0.f))
								[
									SNew(STextBlock)
									.Text(FText::Format(LOCTEXT("Numero", "N. {0}"), FText::FromString(Vista.Tessera.Numero)))
									.Font(Stile->Caratteri->Titolo(15.f))
									.ColorAndOpacity(Inchiostro)
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Top)
							[
								Dati
							]
						]
						// la firma e il timbro
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 10.f, 0.f, 0.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Bottom)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SValdorsoFirma)
									.Firma(Vista.Firma.IsEmpty() ? FString(TEXT("@")) : Vista.Firma)
									.Nome(Nome)
									.FontNome(Stile->Caratteri->Calligrafia(40.f))
									.bModificabile(false)
									.Grandezza(FVector2D(420.f, 92.f))
									.Visibility(EVisibility::HitTestInvisible)
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									Stile->Separatore(420.f, Tenue)
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))
								[
									SNew(STextBlock)
									.Text(bDonna ? LOCTEXT("FirmaLei", "Firma della titolare") : LOCTEXT("FirmaLui", "Firma del titolare"))
									.Font(Stile->Caratteri->Testo(14.f, TEXT("Italic")))
									.ColorAndOpacity(Tenue)
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(FMargin(16.f, 0.f, 6.f, 0.f))
							[
								SNew(SBox)
								.RenderTransform(TransformCast<FSlateRenderTransform>(FQuat2D(FMath::DegreesToRadians(-14.f))))
								.RenderTransformPivot(FVector2D(0.5f, 0.5f))
								[
									// (13/11, chiesto da Fra) il timbro della capitana in ceralacca, con il suo segno (la serra e la stella):
									// così si vede che la tessera è vera. Senza l'immagine, il timbro d'inchiostro di prima.
									TextureCeralacca.IsValid()
									? StaticCastSharedRef<SWidget>(SNew(SBox)
										.WidthOverride(124.f)
										.HeightOverride(155.f)
										.RenderTransform(FSlateRenderTransform(FVector2f(0.f, 46.f)))
										[
											SNew(SImage).Image(&Ceralacca)
										])
									: StaticCastSharedRef<SWidget>(SNew(SValdorsoTimbro).Testo(TEXT("VISTO\nL. SERRA\ncapitana")))
								]
							]
						]
					]
				]
			]
			// (13/11) un po' più di spazio sotto la tessera quando c'è il sigillo di cera, che pende fuori dal foglio
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, TextureCeralacca.IsValid() ? 40.f : 18.f, 0.f, 0.f))
			[
				Fondo
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 10.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(bPropria ? LOCTEXT("AiutoMia", "Esc, E o Invio: rimetti via  ·  X sul controller: mostrala")
					: LOCTEXT("AiutoSua", "Esc, E o Invio: restituiscila"))
				.Font(Stile->Caratteri->Testo(17.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::TestoSecondario())
			]
		]
	];

	Fotografa();
}

SValdorsoTessera::~SValdorsoTessera()
{
	// (durante la pulizia della memoria non si toccano gli oggetti: se ne va tutto comunque)
	if (!IsGarbageCollecting())
	{
		FineScatti();
	}
	Obiettivo.Reset();
}

void SValdorsoTessera::FineScatti()
{
	if (USceneCaptureComponent2D* Telecamera = Obiettivo.Get())
	{
		Telecamera->DestroyComponent();
	}
	Obiettivo.Reset();
	// i capelli tornano come prima (durante gli scatti erano a ciocche: quelli a fili la foto non li vede)
	for (const TWeakObjectPtr<UGroomComponent>& Pelo : Capelli)
	{
		if (UGroomComponent* Ciocche = Pelo.Get())
		{
			Ciocche->SetForcedLOD(-1);
		}
	}
	Capelli.Reset();
}

TSharedRef<SWidget> SValdorsoTessera::Riga(const FText& Etichetta, const FString& Valore, bool bForte)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
		[
			SNew(SBox)
			.WidthOverride(112.f)
			.Padding(FMargin(0.f, bForte ? 4.f : 2.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(Etichetta)
				.Font(Stile->Caratteri->Testo(15.f, TEXT("Italic")))
				.ColorAndOpacity(ValdorsoTema::InchiostroTenue())
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Top)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Valore.IsEmpty() ? FString(TEXT("—")) : Valore))
			.Font(bForte ? Stile->Caratteri->Titolo(19.f) : Stile->Caratteri->Testo(19.f))
			.ColorAndOpacity(ValdorsoTema::Inchiostro())
			.AutoWrapText(true)
		];
}

void SValdorsoTessera::Fotografa()
{
	APawn* Colono = Di.Get();
	UWorld* Mondo = Colono ? Colono->GetWorld() : nullptr;
	if (!Colono || !Mondo || Mondo->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	Pellicola.Reset(NewObject<UTextureRenderTarget2D>(GetTransientPackage()));
	Pellicola->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
	Pellicola->ClearColor = FLinearColor::Black;
	Pellicola->InitAutoFormat(TesseraPixelLargo, TesseraPixelAlto);
	Pellicola->UpdateResourceImmediate(true);

	// la testa: l'osso "head" del manichino (se c'è), se no all'altezza degli occhi
	FVector Testa = Colono->GetActorLocation() + FVector(0.f, 0.f, Colono->BaseEyeHeight);
	if (const ACharacter* Personaggio = Cast<ACharacter>(Colono))
	{
		const USkeletalMeshComponent* Manichino = Personaggio->GetMesh();
		if (Manichino && Manichino->DoesSocketExist(TEXT("head")))
		{
			Testa = Manichino->GetSocketLocation(TEXT("head")) + FVector(0.f, 0.f, 4.f);
		}
	}
	const FVector Avanti = Colono->GetActorForwardVector().GetSafeNormal2D();
	const FVector Dove = Testa + Avanti * 115.f + FVector(0.f, 0.f, 2.f);
	const FVector Guarda = Testa - FVector(0.f, 0.f, 7.f);

	USceneCaptureComponent2D* Telecamera = NewObject<USceneCaptureComponent2D>(Colono, NAME_None, RF_Transient);
	Telecamera->bCaptureEveryFrame = false;
	Telecamera->bCaptureOnMovement = false;
	Telecamera->FOVAngle = 24.f;
	// il colore puro dei materiali (niente luci): di giorno e di notte, al sole e nella nebbia, la foto è la stessa
	Telecamera->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
	Telecamera->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Telecamera->TextureTarget = Pellicola.Get();
	Telecamera->ShowFlags.SetAtmosphere(false);
	Telecamera->ShowFlags.SetFog(false);
	Telecamera->ShowFlags.SetVolumetricFog(false);
	Telecamera->ShowFlags.SetCloud(false);
	// solo il colono e quello che gli sta attaccato (il MetaHuman, i capelli, i vestiti)
	TArray<AActor*> DaVedere = { Colono };
	for (int32 i = 0; i < DaVedere.Num() && i < 64; ++i)
	{
		TArray<AActor*> Attaccati;
		DaVedere[i]->GetAttachedActors(Attaccati, false);
		for (AActor* Pezzo : Attaccati)
		{
			if (Pezzo)
			{
				DaVedere.AddUnique(Pezzo);
			}
		}
	}
	for (AActor* Pezzo : DaVedere)
	{
		Telecamera->ShowOnlyActors.Add(Pezzo);
		// i capelli a fili non finiscono nel colore dei materiali: per gli scatti, quelli a ciocche (come il palco del Registro)
		TInlineComponentArray<UGroomComponent*> Peli(Pezzo);
		for (UGroomComponent* Pelo : Peli)
		{
			Pelo->SetForcedLOD(1);
			Capelli.Add(Pelo);
		}
	}
	Telecamera->SetWorldLocationAndRotation(Dove, (Guarda - Dove).Rotation());
	Telecamera->RegisterComponentWithWorld(Mondo);
	// attaccata al colono: se si muove un poco, la telecamera lo segue
	if (USceneComponent* Radice = Colono->GetRootComponent())
	{
		Telecamera->AttachToComponent(Radice, FAttachmentTransformRules::KeepWorldTransform);
	}
	Obiettivo = Telecamera;

	// tre scatti (dopo mezzo secondo, poi ogni mezzo secondo): l'ultimo riuscito resta, con il volto e i capelli caricati
	RegisterActiveTimer(0.5f, FWidgetActiveTimerDelegate::CreateSP(this, &SValdorsoTessera::Scatta));
}

EActiveTimerReturnType SValdorsoTessera::Scatta(double Tempo, float Delta)
{
	USceneCaptureComponent2D* Telecamera = Obiettivo.Get();
	if (!Telecamera || !Pellicola.IsValid() || !Di.IsValid())
	{
		FineScatti();
		return EActiveTimerReturnType::Stop;
	}
	Telecamera->CaptureScene();
	if (Sviluppa())
	{
		bFotoPronta = true;
	}
	++ScattiFatti;
	if (ScattiFatti >= 3)
	{
		FineScatti();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

bool SValdorsoTessera::Sviluppa()
{
	FTextureRenderTargetResource* Risorsa = Pellicola.IsValid() ? Pellicola->GameThread_GetRenderTargetResource() : nullptr;
	if (!Risorsa)
	{
		return false;
	}
	TArray<FColor> Pixel;
	if (!Risorsa->ReadPixels(Pixel) || Pixel.Num() != TesseraPixelLargo * TesseraPixelAlto)
	{
		return false;
	}

	// il colono e lo sfondo (dove la telecamera non ha visto niente, il nero puro); quanto è chiaro il colono
	TArray<float> Luce;
	Luce.SetNumUninitialized(Pixel.Num());
	float Minima = 1.f;
	float Massima = 0.f;
	int32 Pieni = 0;
	for (int32 i = 0; i < Pixel.Num(); ++i)
	{
		const FColor& P = Pixel[i];
		if (static_cast<int32>(P.R) + P.G + P.B <= 2)
		{
			Luce[i] = -1.f;
			continue;
		}
		const float L = (0.3f * P.R + 0.59f * P.G + 0.11f * P.B) / 255.f;
		Luce[i] = L;
		Minima = FMath::Min(Minima, L);
		Massima = FMath::Max(Massima, L);
		++Pieni;
	}
	// troppo poco colono (non è ancora comparso): si aspetta il prossimo scatto
	if (Pieni < Pixel.Num() / 20)
	{
		return false;
	}
	const float Ampiezza = FMath::Max(0.08f, Massima - Minima);

	// la stampa in seppia: dall'inchiostro scuro alla carta, con la grana e i bordi un poco bruciati
	const FLinearColor Scuro = FLinearColor(FColor(46, 27, 14));
	const FLinearColor Chiaro = FLinearColor(FColor(236, 220, 186));
	const FLinearColor Sfondo = FLinearColor(FColor(214, 194, 152));
	TArray<FColor> Stampati;
	Stampati.SetNumUninitialized(Pixel.Num());
	for (int32 Y = 0; Y < TesseraPixelAlto; ++Y)
	{
		for (int32 X = 0; X < TesseraPixelLargo; ++X)
		{
			const int32 i = Y * TesseraPixelLargo + X;
			const float Grana = (TesseraGrana(X, Y) - 0.5f) * 0.06f;
			const float Dx = (X + 0.5f) / TesseraPixelLargo - 0.5f;
			const float Dy = (Y + 0.5f) / TesseraPixelAlto - 0.5f;
			const float Bordo = 1.f - 0.55f * FMath::Clamp((Dx * Dx + Dy * Dy) * 2.2f - 0.12f, 0.f, 1.f);
			FLinearColor Colore;
			if (Luce[i] < 0.f)
			{
				Colore = Sfondo * (0.97f + Grana);
			}
			else
			{
				const float T = FMath::Pow(FMath::Clamp((Luce[i] - Minima) / Ampiezza, 0.f, 1.f), 0.8f);
				Colore = FMath::Lerp(Scuro, Chiaro, FMath::Clamp(0.12f + 0.86f * T + Grana, 0.f, 1.f));
			}
			Colore *= Bordo;
			Colore.A = 1.f;
			Stampati[i] = Colore.ToFColor(true);
		}
	}

	if (!Stampa.IsValid())
	{
		Stampa.Reset(UTexture2D::CreateTransient(TesseraPixelLargo, TesseraPixelAlto, PF_B8G8R8A8));
		if (!Stampa.IsValid())
		{
			return false;
		}
		Stampa->SRGB = true;
	}
	FTexturePlatformData* Dati = Stampa->GetPlatformData();
	if (!Dati || Dati->Mips.Num() == 0)
	{
		return false;
	}
	void* Scrivi = Dati->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Scrivi, Stampati.GetData(), Stampati.Num() * sizeof(FColor));
	Dati->Mips[0].BulkData.Unlock();
	Stampa->UpdateResource();

	Foto.SetResourceObject(Stampa.Get());
	Foto.ImageSize = FVector2D(TesseraLatoFoto, TesseraAltezzaFoto);
	Foto.DrawAs = ESlateBrushDrawType::Image;
	return true;
}

void SValdorsoTessera::Mostra()
{
	if (!bPropria)
	{
		return;
	}
	if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore.Get()))
	{
		Zaino->ServerMostraTessera();
	}
	OnChiudi.ExecuteIfBound();
}

FReply SValdorsoTessera::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// un tasto tenuto premuto (E o Invio di quando si è aperta) non la richiude subito
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}
	const FKey Tasto = InKeyEvent.GetKey();
	if (Tasto == EKeys::Escape || Tasto == EKeys::E || Tasto == EKeys::Enter || Tasto == EKeys::I
		|| Tasto == EKeys::Gamepad_FaceButton_Right || Tasto == EKeys::Gamepad_Special_Left)
	{
		OnChiudi.ExecuteIfBound();
		return FReply::Handled();
	}
	if (Tasto == EKeys::Gamepad_FaceButton_Left && bPropria)
	{
		Mostra();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SValdorsoTessera::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// i clic non passano al gioco sotto; il fuoco torna alla tessera (per Esc)
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

// ------------------------------------------------------------------------------------------------- aprire e chiudere

namespace ValdorsoTesseraSchermo
{
	bool Aperta(const APlayerController* Controllore)
	{
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		return Zaino && Zaino->SchermataTessera.IsValid();
	}

	void Apri(APlayerController* Controllore, const FValdorsoTesseraVista& Vista, APawn* Di, bool bPropria)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		if (!Zaino || !Viewport || !Controllore->IsLocalController())
		{
			return;
		}
		// una tessera già aperta lascia il posto alla nuova; lo zaino si chiude (la tessera si prende da lì)
		if (Zaino->SchermataTessera.IsValid())
		{
			Chiudi(Controllore);
		}
		ValdorsoZainoSchermo::Chiudi(Controllore);
		AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore);
		if ((Nostro && Nostro->SchermataAperta()) || ValdorsoFoto::Aperta(Controllore) || ValdorsoTaccuino::Aperto(Controllore))
		{
			// c'è altro aperto (un dialogo, la pausa, la mappa...): non si copre
			if (Nostro)
			{
				const FText Testo = bPropria
					? LOCTEXT("Occupato", "Chiudi prima quello che hai aperto.")
					: FText::Format(LOCTEXT("OccupatoAltro", "{0} vuole mostrarti la sua tessera: chiudi quello che hai aperto e chiedigliela di nuovo."),
						FText::FromString(Vista.Nome));
				Nostro->ClientAvviso(Testo, 4.f, 0.f);  // sul PC di chi gioca: subito, qui
			}
			return;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		SAssignNew(Zaino->SchermataTessera, SValdorsoTessera)
			.Controllore(Debole)
			.Vista(Vista)
			.Di(TWeakObjectPtr<APawn>(Di))
			.bPropria(bPropria)
			.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		// come lo zaino (57): sopra le scritte del gioco, sotto la pausa
		Viewport->AddViewportWidgetContent(Zaino->SchermataTessera.ToSharedRef(), 57);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Zaino->SchermataTessera);
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
	}

	void Chiudi(APlayerController* Controllore)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		if (!Zaino || !Zaino->SchermataTessera.IsValid())
		{
			return;
		}
		const TSharedRef<SValdorsoTessera> Schermo = Zaino->SchermataTessera.ToSharedRef();
		Zaino->SchermataTessera.Reset();
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
