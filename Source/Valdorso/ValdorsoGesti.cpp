// Valdorso - La ruota dei gesti (vedi ValdorsoGesti.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoGesti.h"
#include "Valdorso.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoLetto.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoSeduta.h"
#include "ValdorsoTemaUI.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CoreDelegates.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	constexpr float GestiRaggio = 190.f;        // dove stanno i nomi (px, prima della scala dell'interfaccia)
	constexpr float GestiRaggioMorto = 45.f;    // lasciando B qui dentro non si fa niente
	constexpr double GestiSecondiTra = 1.5;
	constexpr float GestiPassiPerFermare = 35.f; // cm: chi si muove interrompe il gesto

	/** Sul PC: la ruota, sopra il gioco. */
	class SValdorsoRuotaGesti : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SValdorsoRuotaGesti) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Caratteri = MakeShared<FValdorsoCaratteri>();
			SetVisibility(EVisibility::HitTestInvisible);
		}

		int32 Scelto() const { return Ultimo; }

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			if (!FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
			{
				return LayerId;
			}
			const TArray<FValdorsoGesto>& Gesti = ValdorsoGesti::Tutti();
			const FVector2f Area = AllottedGeometry.GetLocalSize();
			const FVector2f Centro = Area * 0.5f;
			const FVector2D Mouse = FVector2D(AllottedGeometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos()));
			Ultimo = ValdorsoGesti::Spicchio(Mouse - FVector2D(Centro), Gesti.Num(), GestiRaggioMorto);
			const FSlateBrush* Bianco = FCoreStyle::Get().GetBrush("WhiteBrush");
			const TSharedRef<FSlateFontMeasure> Misure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const FSlateFontInfo Carattere = Caratteri->Titolo(17.f);
			const FSlateFontInfo Piccolo = Caratteri->Testo(13.f, TEXT("Italic"));
			// il centro: un tondo scuro (un quadrato, per ora) con la scritta
			const FString Mezzo = Ultimo < 0 ? FString(TEXT("lascia B qui: niente")) : FString(Gesti[Ultimo].Nome);
			const FVector2D MisuraMezzo = Misure->Measure(Mezzo, Piccolo);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(120.f, 40.f),
				FSlateLayoutTransform(Centro - FVector2f(60.f, 20.f))), Bianco, ESlateDrawEffect::None, ValdorsoTema::Inchiostro().CopyWithNewOpacity(0.7f));
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(MisuraMezzo),
				FSlateLayoutTransform(Centro - FVector2f(MisuraMezzo) * 0.5f)), Mezzo, Piccolo, ESlateDrawEffect::None,
				ValdorsoTema::Pergamena().CopyWithNewOpacity(0.9f));
			for (int32 i = 0; i < Gesti.Num(); ++i)
			{
				// dall'alto, in senso orario
				const float Angolo = -UE_HALF_PI + UE_TWO_PI * i / Gesti.Num();
				const FVector2f Punto = Centro + FVector2f(FMath::Cos(Angolo), FMath::Sin(Angolo)) * GestiRaggio;
				const FString Nome(Gesti[i].Nome);
				const FVector2D Misura = Misure->Measure(Nome, Carattere);
				const FVector2f Riquadro(static_cast<float>(Misura.X) + 28.f, static_cast<float>(Misura.Y) + 14.f);
				const bool bQui = i == Ultimo;
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(Riquadro,
					FSlateLayoutTransform(Punto - Riquadro * 0.5f)), Bianco, ESlateDrawEffect::None,
					(bQui ? ValdorsoTema::Oro() : ValdorsoTema::Inchiostro()).CopyWithNewOpacity(bQui ? 0.92f : 0.72f));
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(Misura),
					FSlateLayoutTransform(Punto - FVector2f(Misura) * 0.5f)), Nome, Carattere, ESlateDrawEffect::None,
					bQui ? ValdorsoTema::Inchiostro() : ValdorsoTema::Pergamena());
			}
			return LayerId + 2;
		}

	private:
		TSharedPtr<FValdorsoCaratteri> Caratteri;
		mutable int32 Ultimo = -1;
	};

	TWeakPtr<SValdorsoRuotaGesti> GestiRuota;
	TWeakObjectPtr<UGameViewportClient> GestiViewport;

	void GestiTogli()
	{
		const TSharedPtr<SValdorsoRuotaGesti> Ruota = GestiRuota.Pin();
		if (UGameViewportClient* Viewport = GestiViewport.Get(); Viewport && Ruota.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(Ruota.ToSharedRef());
		}
		GestiRuota.Reset();
		GestiViewport.Reset();
	}

	UAnimSequence* GestiAnimazione(int32 Indice)
	{
		const TArray<FValdorsoGesto>& Gesti = ValdorsoGesti::Tutti();
		if (!Gesti.IsValidIndex(Indice))
		{
			return nullptr;
		}
		const FString Nome(Gesti[Indice].Animazione);
		return LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Mixamo/Gesti/%s.%s"), *Nome, *Nome), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

namespace ValdorsoGesti
{
	const TArray<FValdorsoGesto>& Tutti()
	{
		static const TArray<FValdorsoGesto> Elenco = {
			{ TEXT("Salutare"), TEXT("Saluta"), false },
			{ TEXT("Inchino"), TEXT("Inchino"), false },
			{ TEXT("Applaudire"), TEXT("Applaudi"), false },
			{ TEXT("Esultare"), TEXT("Esulta"), false },
			{ TEXT("Seduto"), TEXT("Siediti per terra"), true },
			{ TEXT("Spallucce"), TEXT("Spallucce"), false },
			{ TEXT("Pregare"), TEXT("Prega"), false },
			{ TEXT("Indicare"), TEXT("Indica"), false },
		};
		return Elenco;
	}

	int32 Spicchio(const FVector2D& DalCentro, int32 Quanti, float RaggioMorto)
	{
		if (Quanti <= 0 || DalCentro.SizeSquared() < FMath::Square(RaggioMorto))
		{
			return -1;
		}
		// 0 in alto, in senso orario (y in giu'): l'angolo da "in alto", spostato di mezzo spicchio
		float Angolo = FMath::Atan2(static_cast<float>(DalCentro.X), static_cast<float>(-DalCentro.Y));
		if (Angolo < 0.f)
		{
			Angolo += UE_TWO_PI;
		}
		const float Largo = UE_TWO_PI / Quanti;
		return FMath::FloorToInt((Angolo + Largo * 0.5f) / Largo) % Quanti;
	}

	bool Aperta()
	{
		return GestiRuota.IsValid();
	}

	void Apri(APlayerController* Controllore)
	{
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		if (!Viewport || Aperta() || !Controllore->IsLocalController() || !Controllore->GetPawn())
		{
			return;
		}
		static bool bUscitaPronta = false;
		if (!bUscitaPronta)
		{
			bUscitaPronta = true;
			FCoreDelegates::OnPreExit.AddLambda([]() { GestiTogli(); });
		}
		const TSharedRef<SValdorsoRuotaGesti> Ruota = SNew(SValdorsoRuotaGesti);
		Viewport->AddViewportWidgetContent(Ruota, 900);
		GestiRuota = Ruota;
		GestiViewport = Viewport;
		// il mouse in mezzo, visibile, e la telecamera ferma finche' la ruota e' aperta
		FVector2D Misura;
		Viewport->GetViewportSize(Misura);
		Controllore->bShowMouseCursor = true;
		FInputModeGameAndUI Modo;
		Modo.SetHideCursorDuringCapture(false);
		Modo.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		Controllore->SetInputMode(Modo);
		Controllore->SetMouseLocation(FMath::RoundToInt(Misura.X * 0.5), FMath::RoundToInt(Misura.Y * 0.5));
		Controllore->SetIgnoreLookInput(true);
	}

	void Lascia(APlayerController* Controllore, bool bRimettiComandi)
	{
		const TSharedPtr<SValdorsoRuotaGesti> Ruota = GestiRuota.Pin();
		if (!Ruota.IsValid())
		{
			return;
		}
		const int32 Scelto = Ruota->Scelto();
		GestiTogli();
		if (!Controllore)
		{
			return;
		}
		Controllore->SetIgnoreLookInput(false);
		// (se nel frattempo si e' aperta un'altra schermata, il mouse e i comandi restano i suoi; e niente gesto)
		if (!bRimettiComandi)
		{
			return;
		}
		Controllore->bShowMouseCursor = false;
		Controllore->SetInputMode(FInputModeGameOnly());
		APawn* Corpo = Controllore->GetPawn();
		UValdorsoGestiComponent* Gesti = Corpo ? Corpo->FindComponentByClass<UValdorsoGestiComponent>() : nullptr;
		if (Gesti && Scelto >= 0)
		{
			Gesti->Chiedi(Scelto);
		}
	}
}

// ------------------------------------------------------------------------------------------------ sul corpo

UValdorsoGestiComponent::UValdorsoGestiComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	SetIsReplicatedByDefault(true);
}

void UValdorsoGestiComponent::Chiedi(int32 Indice)
{
	if (ValdorsoGesti::Tutti().IsValidIndex(Indice))
	{
		ServerGesto(static_cast<uint8>(Indice));
	}
}

void UValdorsoGestiComponent::ServerGesto_Implementation(uint8 Indice)
{
	AActor* Corpo = GetOwner();
	UWorld* Mondo = GetWorld();
	if (!Corpo || !Mondo || !ValdorsoGesti::Tutti().IsValidIndex(Indice))
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	if (Adesso - UltimoGesto < GestiSecondiTra)
	{
		return;
	}
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
	const UValdorsoSedutaComponent* Seduta = Corpo->FindComponentByClass<UValdorsoSedutaComponent>();
	const UValdorsoSonnoComponent* Sonno = Corpo->FindComponentByClass<UValdorsoSonnoComponent>();
	if ((Pericoli && Pericoli->Caduto()) || (Seduta && Seduta->Occupato()) || (Sonno && Sonno->Occupato()))
	{
		return;
	}
	UltimoGesto = Adesso;
	MulticastGesto(Indice);
}

void UValdorsoGestiComponent::MulticastGesto_Implementation(uint8 Indice)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		Fai(Indice);
	}
}

void UValdorsoGestiComponent::Fai(int32 Indice)
{
	const ACharacter* Colono = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anima = Colono && Colono->GetMesh() ? Colono->GetMesh()->GetAnimInstance() : nullptr;
	UAnimSequence* Sequenza = GestiAnimazione(Indice);
	if (!Anima || !Sequenza)
	{
		return;
	}
	bResta = ValdorsoGesti::Tutti()[Indice].bResta;
	Anima->PlaySlotAnimationAsDynamicMontage(Sequenza, TEXT("DefaultSlot"), 0.25f, 0.3f, 1.f, bResta ? 100000 : 1);
	bInCorso = true;
	Partenza = Colono->GetActorLocation();
	Inizio = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	Durata = Sequenza->GetPlayLength();
}

void UValdorsoGestiComponent::Ferma()
{
	const ACharacter* Colono = Cast<ACharacter>(GetOwner());
	if (UAnimInstance* Anima = Colono && Colono->GetMesh() ? Colono->GetMesh()->GetAnimInstance() : nullptr)
	{
		Anima->StopSlotAnimation(0.25f, TEXT("DefaultSlot"));
	}
	bInCorso = false;
}

void UValdorsoGestiComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bInCorso || !GetOwner() || !GetWorld())
	{
		return;
	}
	if (FVector::DistSquared2D(GetOwner()->GetActorLocation(), Partenza) > FMath::Square(GestiPassiPerFermare))
	{
		Ferma();
		return;
	}
	if (!bResta && GetWorld()->GetTimeSeconds() - Inizio > Durata)
	{
		bInCorso = false;
	}
}
