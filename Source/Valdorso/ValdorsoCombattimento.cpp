// Valdorso - Il combattimento (vedi il .h). Scritto da Claude il 09/11/2026.

#include "ValdorsoCombattimento.h"
#include "Valdorso.h"
#include "ValdorsoArmi.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoEffetti.h"
#include "ValdorsoEtichette.h"
#include "ValdorsoLupi.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTemaUI.h"
#include "ValdorsoTerritorio.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Camera/CameraComponent.h"
#include "CombatDamageable.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Components/AudioComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoCombattimento"

namespace
{
	const TCHAR* CombattimentoCartellaSuoni = TEXT("/Game/Valle/Pericoli/Suoni/");

	USoundBase* CombattimentoSuono(const TCHAR* Nome)
	{
		const FString Percorso = FString(CombattimentoCartellaSuoni) + Nome + TEXT(".") + Nome;
		return LoadObject<USoundBase>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	/** Le animazioni di chi colpisce e di chi e' colpito si fermano un attimo (solo l'aspetto: il movimento no). */
	void CombattimentoArresto(AActor* Chi)
	{
		const ACharacter* Personaggio = Cast<ACharacter>(Chi);
		USkeletalMeshComponent* Corpo = Personaggio ? Personaggio->GetMesh() : (Chi ? Chi->FindComponentByClass<USkeletalMeshComponent>() : nullptr);
		UWorld* Mondo = Chi ? Chi->GetWorld() : nullptr;
		if (!Corpo || !Mondo || Corpo->GlobalAnimRateScale <= 0.f)
		{
			return;
		}
		Corpo->GlobalAnimRateScale = 0.f;
		TWeakObjectPtr<USkeletalMeshComponent> Debole(Corpo);
		FTimerHandle Maniglia;
		Mondo->GetTimerManager().SetTimer(Maniglia, FTimerDelegate::CreateLambda([Debole]()
		{
			if (USkeletalMeshComponent* Ancora = Debole.Get())
			{
				Ancora->GlobalAnimRateScale = 1.f;
			}
		}), ValdorsoCombattimento::SecondiArresto, false);
	}

	/** Il PlayerController (sul server) di chi guida questo corpo; nullptr per i PNG e gli animali. */
	AValdorsoPlayerController* CombattimentoGiocatore(const AActor* Chi)
	{
		const APawn* Corpo = Cast<APawn>(Chi);
		return Corpo ? Cast<AValdorsoPlayerController>(Corpo->GetController()) : nullptr;
	}

	/** Per non ripetere gli avvisi della pace a ogni pugno: una volta ogni 5 secondi per chi colpisce. */
	TMap<TWeakObjectPtr<AActor>, double> CombattimentoUltimoAvvisoPace;
}

namespace ValdorsoCombattimento
{
	bool InPace(const UWorld* Mondo, const FVector& Dove)
	{
		if (!Mondo)
		{
			return false;
		}
		for (TActorIterator<AValdorsoPietraVillaggio> It(Mondo); It; ++It)
		{
			if (It->Protegge(Dove))
			{
				return true;
			}
		}
		return false;
	}

	float DannoNellaPace(float Danno, float Salute, bool& bOutTrattenuto)
	{
		bOutTrattenuto = false;
		const float Massimo = FMath::Max(0.f, Salute - 1.f);
		if (Danno > Massimo)
		{
			bOutTrattenuto = true;
			return Massimo;
		}
		return FMath::Max(0.f, Danno);
	}

	float Spinta(EValdorsoTipoColpo Tipo)
	{
		switch (Tipo)
		{
		case EValdorsoTipoColpo::Lama: return 120.f;
		case EValdorsoTipoColpo::Botta: return 260.f;
		case EValdorsoTipoColpo::Morso: return 160.f;
		case EValdorsoTipoColpo::Scudo: return 320.f;
		default: return 180.f;
		}
	}

	float Tremore(EValdorsoTipoColpo Tipo)
	{
		switch (Tipo)
		{
		case EValdorsoTipoColpo::Lama: return 0.55f;
		case EValdorsoTipoColpo::Botta: return 0.8f;
		case EValdorsoTipoColpo::Morso: return 0.7f;
		case EValdorsoTipoColpo::Scudo: return 0.9f;
		default: return 0.45f;
		}
	}

	const TCHAR* NomeSuono(EValdorsoTipoColpo Tipo)
	{
		switch (Tipo)
		{
		case EValdorsoTipoColpo::Lama: return TEXT("S_Colpo_Lama");
		case EValdorsoTipoColpo::Botta: return TEXT("S_Colpo_Botta");
		case EValdorsoTipoColpo::Morso: return TEXT("S_Colpo_Morso");
		case EValdorsoTipoColpo::Scudo: return TEXT("S_Colpo_Scudo");
		default: return TEXT("S_Colpo_Pugno");
		}
	}

	FVector PuntoBersaglio(const AActor* Bersaglio)
	{
		if (!Bersaglio)
		{
			return FVector::ZeroVector;
		}
		if (const ACharacter* Personaggio = Cast<ACharacter>(Bersaglio))
		{
			const UCapsuleComponent* Capsula = Personaggio->GetCapsuleComponent();
			return Personaggio->GetActorLocation() + FVector(0.f, 0.f, Capsula ? Capsula->GetScaledCapsuleHalfHeight() * 0.45f : 40.f);
		}
		FVector Centro;
		FVector Mezzo;
		Bersaglio->GetActorBounds(true, Centro, Mezzo);
		return Centro;
	}

	bool Salute(const AActor* Chi, float& OutSalute, float& OutMassima)
	{
		const UAbilitySystemComponent* Statistiche = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Chi);
		if (!Statistiche || !Statistiche->GetSet<UValdorsoAttributeSet>())
		{
			return false;
		}
		OutSalute = Statistiche->GetNumericAttribute(UValdorsoAttributeSet::GetSaluteAttribute());
		OutMassima = FMath::Max(1.f, Statistiche->GetNumericAttribute(UValdorsoAttributeSet::GetSaluteMaxAttribute()));
		return true;
	}

	USoundBase* SuonoPericoli(const TCHAR* Nome)
	{
		return CombattimentoSuono(Nome);
	}

	void Suona(UWorld* Mondo, USoundBase* Suono, const FVector& Dove, float Lontano, float Volume, float Tono)
	{
		AWorldSettings* Impostazioni = Mondo ? Mondo->GetWorldSettings() : nullptr;
		if (!Suono || !Impostazioni || Mondo->GetNetMode() == NM_DedicatedServer)
		{
			return;
		}
		UAudioComponent* Audio = NewObject<UAudioComponent>(Impostazioni);
		Audio->SetSound(Suono);
		Audio->bAutoDestroy = true;
		Audio->bAllowSpatialization = true;
		Audio->bOverrideAttenuation = true;
		FSoundAttenuationSettings& Smorzo = Audio->AttenuationOverrides;
		Smorzo.bAttenuate = true;
		Smorzo.bSpatialize = true;
		Smorzo.AttenuationShape = EAttenuationShape::Sphere;
		Smorzo.AttenuationShapeExtents = FVector(FMath::Min(300.f, Lontano * 0.1f), 0.0, 0.0);
		Smorzo.FalloffDistance = Lontano;
		Smorzo.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		Audio->SetVolumeMultiplier(Volume);
		Audio->SetPitchMultiplier(Tono);
		Audio->SetWorldLocation(Dove);
		Audio->RegisterComponentWithWorld(Mondo);
		Audio->Play();
	}

	bool DaDavanti(const FVector& AvantiDelColpito, const FVector& VersoChiColpisce)
	{
		const FVector Avanti = AvantiDelColpito.GetSafeNormal2D();
		const FVector Verso = VersoChiColpisce.GetSafeNormal2D();
		return !Avanti.IsNearlyZero() && !Verso.IsNearlyZero() && FVector::DotProduct(Avanti, Verso) > 0.34f;
	}

	bool Colpisci(AActor* Da, AActor* A, float Danno, float Equilibrio, const FVector& Punto, EValdorsoTipoColpo Tipo)
	{
		if (!A || !A->HasAuthority() || A == Da)
		{
			return false;
		}
		// (09/11) un lupo morto non si colpisce piu'
		if (const AValdorsoAnimale* Morto = Cast<AValdorsoAnimale>(A); Morto && !Morto->Vivo())
		{
			return false;
		}
		UWorld* Mondo = A->GetWorld();
		UAbilitySystemComponent* SuoContenitore = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(A);
		const FVector DaDove = Da ? Da->GetActorLocation() : Punto;
		const FVector Verso = (A->GetActorLocation() - DaDove).GetSafeNormal2D();
		// (09/11) lo scudo nella sinistra: dei colpi che arrivano davanti passa solo una parte, e l'equilibrio regge meglio
		bool bParato = false;
		if (const UValdorsoArmiComponent* SueArmi = UValdorsoArmiComponent::Di(A))
		{
			const FValdorsoTipoOggetto* Scudo = SueArmi->Scudo();
			if (Scudo && Scudo->Parata > 0.f && DaDavanti(A->GetActorForwardVector(), DaDove - A->GetActorLocation()))
			{
				Danno *= 1.f - FMath::Clamp(Scudo->Parata, 0.f, 1.f);
				Equilibrio *= 0.5f;
				Tipo = EValdorsoTipoColpo::Scudo;
				bParato = true;
			}
		}
		bool bColpito = false;

		if (SuoContenitore && SuoContenitore->GetSet<UValdorsoAttributeSet>())
		{
			float DannoVero = FMath::Max(0.f, Danno);
			// la pace del battito: tra giocatori, vicino a un frammento che batte, nessuno muore
			AValdorsoPlayerController* Aggressore = CombattimentoGiocatore(Da);
			AValdorsoPlayerController* Vittima = CombattimentoGiocatore(A);
			if (Aggressore && Vittima && InPace(Mondo, A->GetActorLocation()))
			{
				bool bTrattenuto = false;
				const float SaluteAdesso = SuoContenitore->GetNumericAttribute(UValdorsoAttributeSet::GetSaluteAttribute());
				DannoVero = DannoNellaPace(DannoVero, SaluteAdesso, bTrattenuto);
				if (bTrattenuto)
				{
					const double OraMondo = Mondo ? Mondo->GetTimeSeconds() : 0.0;
					// (chi non c'e' piu' non serve ricordarlo)
					if (CombattimentoUltimoAvvisoPace.Num() > 64)
					{
						for (auto It = CombattimentoUltimoAvvisoPace.CreateIterator(); It; ++It)
						{
							if (!It.Key().IsValid())
							{
								It.RemoveCurrent();
							}
						}
					}
					double& Ultimo = CombattimentoUltimoAvvisoPace.FindOrAdd(Da);
					if (OraMondo - Ultimo > 5.0 || OraMondo < Ultimo)
					{
						Ultimo = OraMondo;
						Aggressore->ClientAvviso(LOCTEXT("PaceFerma", "Il battito del tempio ti ferma la mano. E qualcuno ti ha visto."), 4.f, 0.f);
						Vittima->ClientAvviso(LOCTEXT("PaceProtegge", "Il battito ti protegge."), 3.f, 0.f);
						ValdorsoRegistroEventi::Annota(TEXT("Marchio"), Da, A, A->GetActorLocation(),
							TEXT("ha provato a uccidere nella pace del battito"));
					}
				}
			}
			UAbilitySystemComponent* MioContenitore = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Da);
			UAbilitySystemComponent* Fonte = MioContenitore ? MioContenitore : SuoContenitore;
			FGameplayEffectContextHandle Contesto = Fonte->MakeEffectContext();
			Contesto.AddInstigator(Da, Da);
			FGameplayEffectSpecHandle Effetto = Fonte->MakeOutgoingSpec(UValdorsoGE_Colpo::StaticClass(), 1.f, Contesto);
			if (Effetto.IsValid())
			{
				Effetto.Data->SetSetByCallerMagnitude(ValdorsoEtichette::Dato_Danno, -DannoVero);
				Effetto.Data->SetSetByCallerMagnitude(ValdorsoEtichette::Dato_Equilibrio, -FMath::Max(0.f, Equilibrio));
				Fonte->ApplyGameplayEffectSpecToTarget(*Effetto.Data.Get(), SuoContenitore);
				bColpito = true;
			}
		}
		else if (ICombatDamageable* Colpibile = Cast<ICombatDamageable>(A))
		{
			// i manichini e le casse di Variant_Combat (per le prove)
			Colpibile->ApplyDamage(Danno, Da, Punto, Verso * 250.f + FVector::UpVector * 100.f);
			bColpito = true;
		}
		if (!bColpito)
		{
			return false;
		}
		// (09/11) un lupo colpito si gira verso chi l'ha colpito (e sa a chi dare la pelle)
		if (AValdorsoAnimale* Lupo = Cast<AValdorsoAnimale>(A))
		{
			Lupo->Ferito(Da);
		}
		// la spinta: il colpito arretra un poco (lo vedono tutti: e' il movimento del personaggio)
		if (ACharacter* Colpito = Cast<ACharacter>(A))
		{
			// chi para con lo scudo arretra poco
			const float QuantaSpinta = bParato ? 90.f : Spinta(Tipo);
			Colpito->LaunchCharacter(Verso * QuantaSpinta + FVector(0.f, 0.f, 40.f), true, false);
		}
		ValdorsoRegistroEventi::Annota(TEXT("Colpo"), Da, A, Punto,
			FString::Printf(TEXT("%s, danno %.0f"), NomeSuono(Tipo) + 8, Danno));
		// la reazione su tutti i PC (dal colpito, o da chi colpisce se il colpito non ha reazioni)
		UValdorsoReazioniComponent* Reazioni = UValdorsoReazioniComponent::Di(A);
		if (!Reazioni)
		{
			Reazioni = UValdorsoReazioniComponent::Di(Da);
		}
		if (Reazioni)
		{
			Reazioni->MulticastColpito(Da, A, Punto, static_cast<uint8>(Tipo));
		}
		return true;
	}
}

// ------------------------------------------------------------------------------------------------- le reazioni

UValdorsoReazioniComponent::UValdorsoReazioniComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

UValdorsoReazioniComponent* UValdorsoReazioniComponent::Di(const AActor* Chi)
{
	return Chi ? Chi->FindComponentByClass<UValdorsoReazioniComponent>() : nullptr;
}

void UValdorsoReazioniComponent::MulticastColpito_Implementation(AActor* Da, AActor* Colpito, FVector_NetQuantize Punto, uint8 Tipo)
{
	UWorld* Mondo = GetWorld();
	if (!Mondo || Mondo->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const EValdorsoTipoColpo Quale = static_cast<EValdorsoTipoColpo>(FMath::Min<uint8>(Tipo, static_cast<uint8>(EValdorsoTipoColpo::Scudo)));
	if (USoundBase* Suono = CombattimentoSuono(ValdorsoCombattimento::NomeSuono(Quale)))
	{
		ValdorsoCombattimento::Suona(Mondo, Suono, Punto, 3000.f, FMath::FRandRange(0.85f, 1.f), FMath::FRandRange(0.92f, 1.08f));
	}
	CombattimentoArresto(Colpito);
	CombattimentoArresto(Da);
	// chi gioca su questo PC: se e' dentro il colpo, la sua telecamera trema; se ha colpito, vede la barra del colpito
	APlayerController* Mio = UGameplayStatics::GetPlayerController(this, 0);
	APawn* MioCorpo = Mio ? Mio->GetPawn() : nullptr;
	if (MioCorpo && MioCorpo == Colpito)
	{
		if (UValdorsoReazioniComponent* MieReazioni = Di(MioCorpo))
		{
			MieReazioni->Scossa(ValdorsoCombattimento::Tremore(Quale));
		}
	}
	else if (MioCorpo && MioCorpo == Da)
	{
		if (UValdorsoReazioniComponent* MieReazioni = Di(MioCorpo))
		{
			MieReazioni->Scossa(ValdorsoCombattimento::Tremore(Quale) * 0.35f);
		}
		if (UValdorsoAggancioComponent* Aggancio = UValdorsoAggancioComponent::Di(Mio))
		{
			Aggancio->HaiColpito(Colpito);
		}
	}
}

void UValdorsoReazioniComponent::Scossa(float Forza)
{
	ScossaForza = FMath::Max(ScossaForza * (ScossaRimasta > 0.f ? 1.f : 0.f), FMath::Clamp(Forza, 0.f, 1.f));
	ScossaRimasta = 0.18f + 0.12f * ScossaForza;
	SetComponentTickEnabled(true);
}

void UValdorsoReazioniComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UCameraComponent* Telecamera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr;
	if (!Telecamera)
	{
		Ferma();
		return;
	}
	if (!bScossaBase)
	{
		ScossaBase = Telecamera->GetRelativeLocation();
		bScossaBase = true;
	}
	ScossaRimasta -= DeltaTime;
	if (ScossaRimasta <= 0.f)
	{
		Ferma();
		return;
	}
	// un tremito che si spegne: pochi centimetri, veloce
	const float Quanto = ScossaForza * FMath::Clamp(ScossaRimasta / 0.3f, 0.f, 1.f) * 6.f;
	const FVector Scarto(0.f, FMath::FRandRange(-1.f, 1.f) * Quanto, FMath::FRandRange(-1.f, 1.f) * Quanto * 0.7f);
	Telecamera->SetRelativeLocation(ScossaBase + Scarto);
}

void UValdorsoReazioniComponent::Ferma()
{
	if (bScossaBase && GetOwner())
	{
		if (UCameraComponent* Telecamera = GetOwner()->FindComponentByClass<UCameraComponent>())
		{
			Telecamera->SetRelativeLocation(ScossaBase);
		}
	}
	bScossaBase = false;
	ScossaRimasta = 0.f;
	ScossaForza = 0.f;
	SetComponentTickEnabled(false);
}

void UValdorsoReazioniComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Ferma();
	Super::EndPlay(EndPlayReason);
}

// ------------------------------------------------------------------------------------------------- il disegno del bersaglio

void SValdorsoBersaglio::Construct(const FArguments& InArgs)
{
	Controllore = InArgs._Controllore;
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SValdorsoBersaglio::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	APlayerController* Chi = Controllore.Get();
	const UValdorsoAggancioComponent* Aggancio = UValdorsoAggancioComponent::Di(Chi);
	if (!Chi || !Aggancio)
	{
		return LayerId;
	}
	const float Scala = FMath::Max(0.01f, AllottedGeometry.Scale);
	auto SulloSchermo = [Chi, Scala](const AActor* Bersaglio, float Sopra, FVector2f& OutPunto) -> bool
	{
		FVector2D Pixel;
		const FVector PuntoTesta = ValdorsoCombattimento::PuntoBersaglio(Bersaglio) + FVector(0.f, 0.f, Sopra);
		if (!Chi->ProjectWorldLocationToScreen(PuntoTesta, Pixel, true))
		{
			return false;
		}
		OutPunto = FVector2f(Pixel) / Scala;
		return true;
	};
	// il rombo d'oro sopra chi e' agganciato
	if (const AActor* Agganciato = Aggancio->Agganciato())
	{
		FVector2f P;
		if (SulloSchermo(Agganciato, 95.f, P))
		{
			const float R = 9.f;
			TArray<FVector2f> Rombo = { P + FVector2f(0.f, -R), P + FVector2f(R * 0.7f, 0.f), P + FVector2f(0.f, R), P + FVector2f(-R * 0.7f, 0.f), P + FVector2f(0.f, -R) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Rombo, ESlateDrawEffect::None,
				ValdorsoTema::Oro(), true, 2.5f);
		}
	}
	// la barra di chi hai colpito: senza numeri, solo quanto gli resta
	if (const AActor* Colpito = Aggancio->BarraDi())
	{
		float Salute = 0.f;
		float Massima = 1.f;
		FVector2f P;
		if (ValdorsoCombattimento::Salute(Colpito, Salute, Massima) && SulloSchermo(Colpito, 120.f, P))
		{
			const float Largo = 90.f;
			const float Alto = 6.f;
			const FVector2f Angolo = P - FVector2f(Largo * 0.5f, Alto * 0.5f);
			const FSlateBrush* Pieno = FCoreStyle::Get().GetBrush("WhiteBrush");
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(Largo + 4.f, Alto + 4.f),
				FSlateLayoutTransform(1.f, FVector2D(Angolo - FVector2f(2.f, 2.f)))), Pieno, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.6f));
			const float Quota = FMath::Clamp(Salute / Massima, 0.f, 1.f);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2D(Largo * Quota, Alto),
				FSlateLayoutTransform(1.f, FVector2D(Angolo))), Pieno, ESlateDrawEffect::None, ValdorsoTema::RossoSangue());
		}
	}
	return LayerId + 2;
}

// ------------------------------------------------------------------------------------------------- l'aggancio

UValdorsoAggancioComponent::UValdorsoAggancioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

UValdorsoAggancioComponent* UValdorsoAggancioComponent::Di(const APlayerController* Controllore)
{
	return Controllore ? Controllore->FindComponentByClass<UValdorsoAggancioComponent>() : nullptr;
}

void UValdorsoAggancioComponent::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	UWorld* Mondo = GetWorld();
	UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
	if (!Controllore || !Controllore->IsLocalController() || !Viewport)
	{
		SetComponentTickEnabled(false);
		return;
	}
	SAssignNew(Schermo, SValdorsoBersaglio).Controllore(TWeakObjectPtr<APlayerController>(Controllore));
	// sopra il gioco, sotto le schermate (lo zaino e il taccuino stanno a 57)
	Viewport->AddViewportWidgetContent(Schermo.ToSharedRef(), 5);
}

void UValdorsoAggancioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Schermo.IsValid())
	{
		UWorld* Mondo = GetWorld();
		if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Schermo.ToSharedRef());
		}
		Schermo.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

AActor* UValdorsoAggancioComponent::Cerca() const
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APawn* Io = Controllore ? Controllore->GetPawn() : nullptr;
	UWorld* Mondo = GetWorld();
	if (!Io || !Mondo)
	{
		return nullptr;
	}
	FVector Occhio;
	FRotator Sguardo;
	Controllore->GetPlayerViewPoint(Occhio, Sguardo);
	const FVector Avanti = Sguardo.Vector().GetSafeNormal2D();
	AActor* Migliore = nullptr;
	float Punteggio = TNumericLimits<float>::Max();
	for (TActorIterator<APawn> It(Mondo); It; ++It)
	{
		APawn* Altro = *It;
		if (!Altro || Altro == Io || Altro->IsHidden())
		{
			continue;
		}
		float Salute = 0.f;
		float Massima = 1.f;
		if (!ValdorsoCombattimento::Salute(Altro, Salute, Massima) || Salute <= 0.f)
		{
			continue;
		}
		const FVector Verso = Altro->GetActorLocation() - Io->GetActorLocation();
		const float Distanza = Verso.Size();
		if (Distanza > ValdorsoCombattimento::PortataAggancio)
		{
			continue;
		}
		const float Coseno = FVector::DotProduct(Avanti, Verso.GetSafeNormal2D());
		if (Coseno < 0.5f)
		{
			continue;   // fuori dal campo visivo (piu' di 60 gradi di lato)
		}
		// si preferisce chi sta piu' al centro dello sguardo, poi il piu' vicino
		const float Quanto = (1.f - Coseno) * 3000.f + Distanza;
		if (Quanto < Punteggio)
		{
			Punteggio = Quanto;
			Migliore = Altro;
		}
	}
	return Migliore;
}

void UValdorsoAggancioComponent::Orienta(APawn* Corpo, bool bAgganciato)
{
	ACharacter* Personaggio = Cast<ACharacter>(Corpo);
	UCharacterMovementComponent* Movimento = Personaggio ? Personaggio->GetCharacterMovement() : nullptr;
	if (!Movimento)
	{
		return;
	}
	// agganciato: il colono guarda dove guarda la telecamera (cioe' il bersaglio); poi si rimette come prima
	if (bAgganciato)
	{
		if (!bGirataSalvata)
		{
			bGirataSalvata = true;
			bPrimaVersoMovimento = Movimento->bOrientRotationToMovement;
			bPrimaVersoControllo = Movimento->bUseControllerDesiredRotation;
		}
		Movimento->bOrientRotationToMovement = false;
		Movimento->bUseControllerDesiredRotation = true;
	}
	else if (bGirataSalvata)
	{
		bGirataSalvata = false;
		Movimento->bOrientRotationToMovement = bPrimaVersoMovimento;
		Movimento->bUseControllerDesiredRotation = bPrimaVersoControllo;
	}
}

void UValdorsoAggancioComponent::ServerGuarda_Implementation(bool bAgganciato)
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	Orienta(Controllore ? Controllore->GetPawn() : nullptr, bAgganciato);
}

void UValdorsoAggancioComponent::Alterna()
{
	if (Bersaglio.IsValid())
	{
		Sgancia();
		return;
	}
	AActor* Trovato = Cerca();
	if (!Trovato)
	{
		return;
	}
	Bersaglio = Trovato;
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	Orienta(Controllore ? Controllore->GetPawn() : nullptr, true);
	ServerGuarda(true);
}

void UValdorsoAggancioComponent::Sgancia()
{
	const bool bEra = Bersaglio.IsValid();
	Bersaglio.Reset();
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	Orienta(Controllore ? Controllore->GetPawn() : nullptr, false);
	if (bEra || (Controllore && Controllore->IsLocalController()))
	{
		ServerGuarda(false);
	}
}

void UValdorsoAggancioComponent::HaiColpito(AActor* Chi)
{
	UltimoColpito = Chi;
	const UWorld* Mondo = GetWorld();
	UltimoColpoAlle = Mondo ? Mondo->GetTimeSeconds() : 0.0;
}

AActor* UValdorsoAggancioComponent::BarraDi() const
{
	const UWorld* Mondo = GetWorld();
	if (!Mondo || !UltimoColpito.IsValid() || Mondo->GetTimeSeconds() - UltimoColpoAlle > ValdorsoCombattimento::SecondiBarra)
	{
		return nullptr;
	}
	return UltimoColpito.Get();
}

void UValdorsoAggancioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Agganciato = Bersaglio.Get();
	if (!Agganciato)
	{
		if (Bersaglio.IsStale())
		{
			Sgancia();
		}
		return;
	}
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	APawn* Io = Controllore ? Controllore->GetPawn() : nullptr;
	float Salute = 0.f;
	float Massima = 1.f;
	if (!Io || !ValdorsoCombattimento::Salute(Agganciato, Salute, Massima) || Salute <= 0.f
		|| FVector::Dist(Io->GetActorLocation(), Agganciato->GetActorLocation()) > ValdorsoCombattimento::PortataSgancio)
	{
		Sgancia();
		return;
	}
	// la telecamera segue il bersaglio, morbida (solo di lato; l'alto e il basso restano al giocatore, un poco guidati)
	const FVector Verso = ValdorsoCombattimento::PuntoBersaglio(Agganciato) - Io->GetActorLocation();
	const FRotator Voluto = Verso.Rotation();
	FRotator Rotazione = Controllore->GetControlRotation();
	Rotazione.Yaw = FMath::RInterpTo(FRotator(0.f, Rotazione.Yaw, 0.f), FRotator(0.f, Voluto.Yaw, 0.f), DeltaTime, 8.f).Yaw;
	const float PitchVoluto = FMath::Clamp(FRotator::NormalizeAxis(Voluto.Pitch) - 10.f, -35.f, 20.f);
	Rotazione.Pitch = FMath::FInterpTo(FRotator::NormalizeAxis(Rotazione.Pitch), PitchVoluto, DeltaTime, 2.f);
	Controllore->SetControlRotation(Rotazione);
}

#undef LOCTEXT_NAMESPACE
