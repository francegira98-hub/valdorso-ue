// Valdorso - Le impostazioni del giocatore (vedi il .h). Scritto da Claude il 02/11/2026.

#include "ValdorsoImpostazioni.h"
#include "Valdorso.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/UserInterfaceSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Rendering/RenderingCommon.h"
#include "Rendering/SlateRenderer.h"

namespace
{
	const TCHAR* Sezione = TEXT("Valdorso.Impostazioni");

	bool bLette = false;
	FValdorsoImpostazioni Attuali;

	/** Sul server dedicato e nei comandi senza finestra non c'è niente da applicare. */
	bool ConSchermo()
	{
		return !IsRunningDedicatedServer() && !IsRunningCommandlet() && GEngine != nullptr;
	}

	void LeggiImpostazioni()
	{
		bLette = true;
		if (!GConfig)
		{
			return;
		}
		FValdorsoImpostazioni& I = Attuali;
		GConfig->GetInt(Sezione, TEXT("Qualita"), I.Qualita, GGameUserSettingsIni);
		GConfig->GetInt(Sezione, TEXT("Finestra"), I.Finestra, GGameUserSettingsIni);
		GConfig->GetFloat(Sezione, TEXT("VolumeGenerale"), I.VolumeGenerale, GGameUserSettingsIni);
		GConfig->GetFloat(Sezione, TEXT("VolumeMusica"), I.VolumeMusica, GGameUserSettingsIni);
		GConfig->GetFloat(Sezione, TEXT("VolumeEffetti"), I.VolumeEffetti, GGameUserSettingsIni);
		GConfig->GetFloat(Sezione, TEXT("Sensibilita"), I.Sensibilita, GGameUserSettingsIni);
		GConfig->GetBool(Sezione, TEXT("InvertiY"), I.bInvertiY, GGameUserSettingsIni);
		GConfig->GetFloat(Sezione, TEXT("GrandezzaTesto"), I.GrandezzaTesto, GGameUserSettingsIni);
		GConfig->GetBool(Sezione, TEXT("MenoMovimenti"), I.bMenoMovimenti, GGameUserSettingsIni);
		GConfig->GetInt(Sezione, TEXT("Daltonismo"), I.Daltonismo, GGameUserSettingsIni);

		// Un file scritto a mano non deve rompere niente.
		I.Qualita = FMath::Clamp(I.Qualita, -1, 3);
		I.Finestra = FMath::Clamp(I.Finestra, 0, 2);
		I.VolumeGenerale = FMath::Clamp(I.VolumeGenerale, 0.f, 1.f);
		I.VolumeMusica = FMath::Clamp(I.VolumeMusica, 0.f, 1.f);
		I.VolumeEffetti = FMath::Clamp(I.VolumeEffetti, 0.f, 1.f);
		I.Sensibilita = FMath::Clamp(I.Sensibilita, 0.25f, 3.f);
		I.GrandezzaTesto = FMath::Clamp(I.GrandezzaTesto, 1.f, 1.3f);
		I.Daltonismo = FMath::Clamp(I.Daltonismo, 0, 3);
	}

	void ScriviImpostazioni()
	{
		if (!GConfig)
		{
			return;
		}
		const FValdorsoImpostazioni& I = Attuali;
		GConfig->SetInt(Sezione, TEXT("Qualita"), I.Qualita, GGameUserSettingsIni);
		GConfig->SetInt(Sezione, TEXT("Finestra"), I.Finestra, GGameUserSettingsIni);
		GConfig->SetFloat(Sezione, TEXT("VolumeGenerale"), I.VolumeGenerale, GGameUserSettingsIni);
		GConfig->SetFloat(Sezione, TEXT("VolumeMusica"), I.VolumeMusica, GGameUserSettingsIni);
		GConfig->SetFloat(Sezione, TEXT("VolumeEffetti"), I.VolumeEffetti, GGameUserSettingsIni);
		GConfig->SetFloat(Sezione, TEXT("Sensibilita"), I.Sensibilita, GGameUserSettingsIni);
		GConfig->SetBool(Sezione, TEXT("InvertiY"), I.bInvertiY, GGameUserSettingsIni);
		GConfig->SetFloat(Sezione, TEXT("GrandezzaTesto"), I.GrandezzaTesto, GGameUserSettingsIni);
		GConfig->SetBool(Sezione, TEXT("MenoMovimenti"), I.bMenoMovimenti, GGameUserSettingsIni);
		GConfig->SetInt(Sezione, TEXT("Daltonismo"), I.Daltonismo, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

	void ImpostaVariabile(const TCHAR* Nome, int32 Valore)
	{
		if (IConsoleVariable* Variabile = IConsoleManager::Get().FindConsoleVariable(Nome))
		{
			Variabile->Set(Valore, ECVF_SetByGameSetting);
		}
	}

	void ApplicaGrafica(const FValdorsoImpostazioni& I, const FValdorsoImpostazioni* Prima)
	{
		UGameUserSettings* Motore = GEngine->GetGameUserSettings();
		if (!Motore)
		{
			return;
		}
		const bool bQualitaCambiata = !Prima || Prima->Qualita != I.Qualita;
		const bool bFinestraCambiata = !Prima || Prima->Finestra != I.Finestra;
		if (!bQualitaCambiata && !bFinestraCambiata)
		{
			return;
		}
		if (bQualitaCambiata)
		{
			if (I.Qualita < 0)
			{
				// Automatica: Unreal misura processore e scheda video e sceglie (ci vuole un attimo).
				Motore->RunHardwareBenchmark();
				Motore->ApplyHardwareBenchmarkResults();
			}
			else
			{
				Motore->SetOverallScalabilityLevel(I.Qualita);
			}
		}
		// Nell'editor la finestra è quella dell'editor: non si tocca.
		if (bFinestraCambiata && !GIsEditor)
		{
			const EWindowMode::Type Modo = I.Finestra == 0 ? EWindowMode::Fullscreen
				: (I.Finestra == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
			Motore->SetFullscreenMode(Modo);
		}
		Motore->ApplySettings(false);
	}

	void ApplicaTutto(const FValdorsoImpostazioni& I, const FValdorsoImpostazioni* Prima)
	{
		if (!ConSchermo())
		{
			return;
		}
		// All'avvio la grafica la rimette Unreal dal suo file: qui si cambia solo quando il giocatore la cambia
		// (così la misura automatica del PC non si rifà a ogni avvio).
		if (Prima)
		{
			ApplicaGrafica(I, Prima);
		}

		// Il volume generale tocca tutti i suoni del gioco.
		if (FAudioDeviceHandle Audio = GEngine->GetMainAudioDevice())
		{
			Audio->SetTransientPrimaryVolume(I.VolumeGenerale);
		}

		// La grandezza delle scritte: la scala di tutta l'interfaccia (schermate Slate e UMG).
		if (UUserInterfaceSettings* Interfaccia = GetMutableDefault<UUserInterfaceSettings>())
		{
			Interfaccia->ApplicationScale = I.GrandezzaTesto;
		}

		// Meno movimenti: niente sfocatura di movimento.
		ImpostaVariabile(TEXT("r.MotionBlurQuality"), I.bMenoMovimenti ? 0 : 4);

		// I colori per il daltonismo: Unreal corregge i colori di tutto lo schermo.
		if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
		{
			static const EColorVisionDeficiency Tipi[] = { EColorVisionDeficiency::NormalVision, EColorVisionDeficiency::Deuteranope,
				EColorVisionDeficiency::Protanope, EColorVisionDeficiency::Tritanope };
			const bool bCorreggi = I.Daltonismo > 0;
			FSlateApplication::Get().GetRenderer()->SetColorVisionDeficiencyType(Tipi[FMath::Clamp(I.Daltonismo, 0, 3)],
				bCorreggi ? 10 : 0, bCorreggi, false);
		}
	}
}

namespace ValdorsoImpostazioni
{
	const FValdorsoImpostazioni& Correnti()
	{
		if (!bLette)
		{
			LeggiImpostazioni();
		}
		return Attuali;
	}

	void Cambia(const FValdorsoImpostazioni& Nuove)
	{
		const FValdorsoImpostazioni Prima = Correnti();
		Attuali = Nuove;
		ApplicaTutto(Attuali, &Prima);
		ScriviImpostazioni();
	}

	void Applica()
	{
		ApplicaTutto(Correnti(), nullptr);
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Impostazioni applicate (qualità %d, testo %.2f)."), Attuali.Qualita, Attuali.GrandezzaTesto);
	}

	float VolumeSuono(bool bMusica)
	{
		const FValdorsoImpostazioni& I = Correnti();
		return bMusica ? I.VolumeMusica : I.VolumeEffetti;
	}
}
