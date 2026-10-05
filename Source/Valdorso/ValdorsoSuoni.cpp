// Valdorso - I suoni delle schermate (vedi il .h).

#include "ValdorsoSuoni.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Valdorso.h"

namespace ValdorsoSuoni
{
	UWorld* Mondo()
	{
		if (GEngine && GEngine->GameViewport)
		{
			return GEngine->GameViewport->GetWorld();
		}
		return nullptr;
	}

	USoundBase* Carica(const TCHAR* Nome)
	{
		const FString Percorso = FString::Printf(TEXT("/Game/Audio/Registro/%s.%s"), Nome, Nome);
		USoundBase* Suono = LoadObject<USoundBase>(nullptr, *Percorso, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (Suono == nullptr)
		{
			// (Log e non Warning: nei test automatici un avviso può contare come errore.)
			UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Suoni: %s non trovato (lanciare importa_registro.py)"), Nome);
		}
		return Suono;
	}

	UAudioComponent* Suona(const TCHAR* Nome, float Volume, float Inizio)
	{
		UWorld* World = Mondo();
		if (World == nullptr || World->GetNetMode() == NM_DedicatedServer)
		{
			return nullptr;
		}
		USoundBase* Suono = Carica(Nome);
		if (Suono == nullptr)
		{
			return nullptr;
		}
		return UGameplayStatics::SpawnSound2D(World, Suono, Volume, 1.f, Inizio);
	}

	UAudioComponent* SuonaFede(const FString& Chiave, float Volume)
	{
		if (Chiave.IsEmpty())
		{
			return nullptr;
		}
		// "vecchidei" -> S_Fede_VecchiDei, "solara" -> S_Fede_Solara.
		FString Nome = Chiave == TEXT("vecchidei") ? FString(TEXT("VecchiDei")) : Chiave.Left(1).ToUpper() + Chiave.Mid(1);
		return Suona(*(TEXT("S_Fede_") + Nome), Volume);
	}

	void Sfuma(UAudioComponent* Componente, float Secondi)
	{
		if (IsValid(Componente) && Componente->IsPlaying())
		{
			Componente->FadeOut(Secondi, 0.f);
		}
	}
}
