// Valdorso - La musica della valle (vedi ValdorsoMusica.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoMusica.h"
#include "Valdorso.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoCielo.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoImpostazioni.h"
#include "ValdorsoPNG.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	constexpr float MusicaDissolvenza = 4.f;
	constexpr double MusicaSecondiCombattimento = 12.0;
	constexpr double MusicaPausaMinima = 40.0;
	constexpr double MusicaPausaMassima = 100.0;
	/** Il tempio: entro quanto da Padre Anselmo (cm, in piano) e quanto in alto. */
	constexpr float MusicaRaggioTempio = 1300.f;
	constexpr float MusicaAltezzaTempio = 450.f;
}

namespace ValdorsoMusica
{
	EValdorsoMomentoMusica Momento(bool bCombattimento, bool bTempio, float Ora)
	{
		if (bCombattimento)
		{
			return EValdorsoMomentoMusica::Combattimento;
		}
		if (bTempio)
		{
			return EValdorsoMomentoMusica::Tempio;
		}
		return (Ora >= 20.f || Ora < 6.f) ? EValdorsoMomentoMusica::Notte : EValdorsoMomentoMusica::Giorno;
	}

	const TCHAR* Nome(EValdorsoMomentoMusica Quale)
	{
		switch (Quale)
		{
		case EValdorsoMomentoMusica::Giorno: return TEXT("giorno");
		case EValdorsoMomentoMusica::Notte: return TEXT("notte");
		case EValdorsoMomentoMusica::Tempio: return TEXT("tempio");
		case EValdorsoMomentoMusica::Combattimento: return TEXT("combattimento");
		default: return TEXT("silenzio");
		}
	}
}

UValdorsoMusicaComponent::UValdorsoMusicaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;
	SetIsReplicatedByDefault(false);
}

void UValdorsoMusicaComponent::BeginPlay()
{
	Super::BeginPlay();
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (!Controllore || !Controllore->IsLocalController() || GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
		return;
	}
	for (TObjectPtr<UAudioComponent>* Quale : { &Uno, &Due })
	{
		UAudioComponent* Nuovo = NewObject<UAudioComponent>(GetOwner());
		Nuovo->bAutoActivate = false;
		Nuovo->bIsUISound = true;          // non si ferma con la pausa, e non sta in un punto
		Nuovo->bAllowSpatialization = false;
		Nuovo->RegisterComponent();
		*Quale = Nuovo;
	}
}

void UValdorsoMusicaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UAudioComponent* Quale : { Uno.Get(), Due.Get() })
	{
		if (Quale)
		{
			Quale->Stop();
			Quale->DestroyComponent();
		}
	}
	Uno = nullptr;
	Due = nullptr;
	Super::EndPlay(EndPlayReason);
}

USoundBase* UValdorsoMusicaComponent::Brano(EValdorsoMomentoMusica Quale)
{
	if (Quale == EValdorsoMomentoMusica::Silenzio || Mancanti.Contains(Quale))
	{
		return nullptr;
	}
	if (const TObjectPtr<USoundBase>* Gia = Brani.Find(Quale))
	{
		return Gia->Get();
	}
	static const TCHAR* Nomi[] = { TEXT(""), TEXT("M_Giorno"), TEXT("M_Notte"), TEXT("M_Tempio"), TEXT("M_Combattimento") };
	const TCHAR* Nome = Nomi[static_cast<int32>(Quale)];
	USoundBase* Suono = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Valle/Musica/%s.%s"), Nome, Nome), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Suono)
	{
		Mancanti.Add(Quale);
		return nullptr;
	}
	Brani.Add(Quale, Suono);
	return Suono;
}

bool UValdorsoMusicaComponent::InCombattimento(double Ora)
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
	if (!Corpo)
	{
		SaluteVista = -1.f;
		return false;
	}
	if (const UValdorsoAggancioComponent* Aggancio = Controllore->FindComponentByClass<UValdorsoAggancioComponent>())
	{
		if (Aggancio->Agganciato() || Aggancio->BarraDi())
		{
			CombattimentoFino = Ora + MusicaSecondiCombattimento;
		}
	}
	// ferito: la salute (che arriva dal server) e' scesa
	const IAbilitySystemInterface* ConAbilita = Cast<IAbilitySystemInterface>(Corpo);
	if (const UAbilitySystemComponent* Abilita = ConAbilita ? ConAbilita->GetAbilitySystemComponent() : nullptr)
	{
		const float Salute = Abilita->GetNumericAttribute(UValdorsoAttributeSet::GetSaluteAttribute());
		if (SaluteVista >= 0.f && Salute < SaluteVista - 0.5f)
		{
			CombattimentoFino = Ora + MusicaSecondiCombattimento;
		}
		SaluteVista = Salute;
	}
	return Ora < CombattimentoFino;
}

bool UValdorsoMusicaComponent::NelTempio() const
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
	const AActor* Sacerdote = Tempio.Get();
	if (!Corpo || !Sacerdote)
	{
		return false;
	}
	const FVector Da = Corpo->GetActorLocation();
	const FVector A = Sacerdote->GetActorLocation();
	return FVector::DistSquared2D(Da, A) < FMath::Square(MusicaRaggioTempio) && FMath::Abs(Da.Z - A.Z) < MusicaAltezzaTempio;
}

void UValdorsoMusicaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UWorld* Mondo = GetWorld();
	if (!Mondo || !Uno || !Due)
	{
		return;
	}
	const double Ora = Mondo->GetRealTimeSeconds();
	// il volume delle impostazioni (anche mentre suona)
	const float Volume = ValdorsoImpostazioni::VolumeSuono(true);
	if (!FMath::IsNearlyEqual(Volume, VolumeUsato))
	{
		VolumeUsato = Volume;
		Uno->SetVolumeMultiplier(FMath::Max(0.001f, Volume));
		Due->SetVolumeMultiplier(FMath::Max(0.001f, Volume));
	}
	// il tempio: Padre Anselmo (lo si cerca ogni tanto: puo' arrivare dopo)
	if (!Tempio.IsValid() && Ora - CercatoTempio > 10.0)
	{
		CercatoTempio = Ora;
		for (TActorIterator<AValdorsoPNG> It(Mondo); It; ++It)
		{
			if (It->Dialogo == FName(TEXT("PadreAnselmo")))
			{
				Tempio = *It;
				break;
			}
		}
	}
	float OraValle = 12.f;
	for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
	{
		OraValle = It->Ora();
		break;
	}
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const bool bInGioco = Controllore && Controllore->GetPawn();
	const EValdorsoMomentoMusica Voluto = bInGioco ? ValdorsoMusica::Momento(InCombattimento(Ora), NelTempio(), OraValle) : EValdorsoMomentoMusica::Silenzio;
	UAudioComponent* Attivo = bSuUno ? Uno.Get() : Due.Get();
	if (Voluto != Adesso)
	{
		Suona(Voluto, Ora);
		return;
	}
	// finito il brano: un poco di silenzio, poi di nuovo (il combattimento subito)
	if (Adesso != EValdorsoMomentoMusica::Silenzio && !Attivo->IsPlaying())
	{
		if (ProssimoGiro <= 0.0)
		{
			ProssimoGiro = Ora + (Adesso == EValdorsoMomentoMusica::Combattimento ? 0.0 : FMath::FRandRange(MusicaPausaMinima, MusicaPausaMassima));
		}
		if (Ora >= ProssimoGiro)
		{
			ProssimoGiro = 0.0;
			if (USoundBase* Suono = Brano(Adesso))
			{
				Attivo->SetSound(Suono);
				Attivo->FadeIn(MusicaDissolvenza, 1.f);
			}
		}
	}
}

void UValdorsoMusicaComponent::Suona(EValdorsoMomentoMusica Nuovo, double Ora)
{
	UAudioComponent* Vecchio = bSuUno ? Uno.Get() : Due.Get();
	UAudioComponent* Prossimo = bSuUno ? Due.Get() : Uno.Get();
	if (Vecchio->IsPlaying())
	{
		Vecchio->FadeOut(MusicaDissolvenza, 0.f);
	}
	Adesso = Nuovo;
	bSuUno = !bSuUno;
	ProssimoGiro = 0.0;
	if (USoundBase* Suono = Brano(Nuovo))
	{
		Prossimo->Stop();
		Prossimo->SetSound(Suono);
		Prossimo->FadeIn(MusicaDissolvenza, 1.f);
	}
	UE_LOG(LogValdorso, Verbose, TEXT("[Valdorso] Musica: %s"), ValdorsoMusica::Nome(Nuovo));
}

FString UValdorsoMusicaComponent::Racconta() const
{
	const UAudioComponent* Attivo = bSuUno ? Uno.Get() : Due.Get();
	TArray<FString> Senza;
	for (const EValdorsoMomentoMusica M : Mancanti)
	{
		Senza.Add(ValdorsoMusica::Nome(M));
	}
	return FString::Printf(TEXT("Musica: %s (%s); combattimento ancora %.0f s; tempio %s; volume %.2f; brani che mancano: %s"),
		ValdorsoMusica::Nome(Adesso), Attivo && Attivo->IsPlaying() ? TEXT("suona") : TEXT("tace"),
		FMath::Max(0.0, CombattimentoFino - (GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0)),
		NelTempio() ? TEXT("si'") : TEXT("no"), VolumeUsato, Senza.IsEmpty() ? TEXT("nessuno") : *FString::Join(Senza, TEXT(", ")));
}

namespace
{
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ComandoMusicaStato(
		TEXT("Valdorso.Musica.Stato"),
		TEXT("Valdorso: quale brano suona adesso, e perche'."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
		{
			const APlayerController* Chi = Mondo ? Mondo->GetFirstPlayerController() : nullptr;
			const UValdorsoMusicaComponent* Musica = Chi ? Chi->FindComponentByClass<UValdorsoMusicaComponent>() : nullptr;
			Uscita.Log(Musica ? Musica->Racconta() : FString(TEXT("Musica: nessun colono su questo PC.")));
		}));
}
