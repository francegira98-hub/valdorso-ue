// Valdorso - Le impostazioni del giocatore (vedi il .h). Scritto da Claude il 02/11/2026.

#include "ValdorsoImpostazioni.h"
#include "Valdorso.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Scalability.h"
#include "WorldPartition/WorldPartition.h"
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
		GConfig->GetInt(Sezione, TEXT("Distanza"), I.Distanza, GGameUserSettingsIni);
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
		I.Distanza = FMath::Clamp(I.Distanza, 0, 3);
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
		GConfig->SetInt(Sezione, TEXT("Distanza"), I.Distanza, GGameUserSettingsIni);
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

	void ImpostaVariabileF(const TCHAR* Nome, float Valore)
	{
		if (IConsoleVariable* Variabile = IConsoleManager::Get().FindConsoleVariable(Nome))
		{
			Variabile->Set(Valore, ECVF_SetByGameSetting);
		}
	}

	/** (10/11) La distanza visiva: quanto lontano si disegna e con quanto dettaglio (vale sopra la qualita' scelta). */
	void ApplicaVariabiliDistanza(const FValdorsoImpostazioni& I)
	{
		struct FLivello { float Vista; float Fogliame; float Modelli; float Terreno; };
		static const FLivello Livelli[4] = {
			{ 0.8f, 1.f, 1.25f, 1.f },     // vicino: per i PC deboli
			{ 1.f, 1.f, 1.f, 1.f },        // normale: come Unreal di serie
			{ 1.5f, 1.6f, 0.85f, 1.35f },  // lontano
			{ 2.2f, 2.5f, 0.7f, 1.8f },    // lontanissimo
		};
		const FLivello& L = Livelli[FMath::Clamp(I.Distanza, 0, 3)];
		// quanto lontano si disegnano le cose: la distanza della qualita' scelta (Unreal: 0,4 bassa, 0,6 media, 0,8 alta,
		// 1 epica) moltiplicata per quella voluta, cosi' un PC con la qualita' bassa resta leggero
		static const float VistaDellaQualita[5] = { 0.4f, 0.6f, 0.8f, 1.f, 1.f };
		const int32 Livello = FMath::Clamp(Scalability::GetQualityLevels().ViewDistanceQuality, 0, 4);
		ImpostaVariabileF(TEXT("r.ViewDistanceScale"), VistaDellaQualita[Livello] * L.Vista);
		ImpostaVariabileF(TEXT("foliage.LODDistanceScale"), L.Fogliame);
		// i modelli piu' dettagliati restano fino a piu' lontano (meno di 1 = piu' dettaglio)
		ImpostaVariabileF(TEXT("r.StaticMeshLODDistanceScale"), L.Modelli);
		// il terreno: i pezzi vicini dettagliati fino a piu' lontano
		ImpostaVariabileF(TEXT("r.LandscapeLOD0DistributionScale"), L.Terreno);
		ImpostaVariabileF(TEXT("r.LandscapeLODDistributionScale"), L.Terreno);
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

		// (10/11) La distanza visiva (dopo la qualita': vale sopra le sue scelte).
		ApplicaVariabiliDistanza(I);
		if (!Prima || Prima->Distanza != I.Distanza)
		{
			ValdorsoImpostazioni::ApplicaDistanza();
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

	void ApplicaDistanza()
	{
		if (!ConSchermo())
		{
			return;
		}
		const FValdorsoImpostazioni& I = Correnti();
		ApplicaVariabiliDistanza(I);
		// il caricamento dei pezzi della valle (World Partition): fino a dove si caricano intorno al colono. Il comando
		// c'e' con la griglia di serie; se il livello ne usa un'altra Unreal dice solo che non lo conosce.
		// (solo per "lontanissimo": per "lontano" basta quello del livello, sistema_lontananza.py lo porta a 512 m; tornando
		// indietro vale di nuovo quello del livello alla prossima apertura della valle)
		static const int32 Raggi[4] = { 0, 0, 0, 102400 };
		const int32 Raggio = Raggi[FMath::Clamp(I.Distanza, 0, 3)];
		if (Raggio <= 0)
		{
			return;  // vicino e normale: il caricamento resta quello del livello
		}
		for (const FWorldContext& Contesto : GEngine->GetWorldContexts())
		{
			UWorld* Mondo = Contesto.World();
			if (Mondo && (Contesto.WorldType == EWorldType::Game || Contesto.WorldType == EWorldType::PIE) && Mondo->GetWorldPartition())
			{
				// (il comando vale per tutti i mondi: basta una volta)
				GEngine->Exec(Mondo, *FString::Printf(TEXT("wp.Runtime.OverrideRuntimeSpatialHashLoadingRange -grid=0 -range=%d"), Raggio));
				break;
			}
		}
	}

	float VolumeSuono(bool bMusica)
	{
		const FValdorsoImpostazioni& I = Correnti();
		return bMusica ? I.VolumeMusica : I.VolumeEffetti;
	}
}
