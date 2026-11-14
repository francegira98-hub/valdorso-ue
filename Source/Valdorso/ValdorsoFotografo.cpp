// Valdorso - Il fotografo delle armi (v0.2.4, 14/11/2026). Scritto da Claude, promesso a Fra il 14/11: "vorrei che
// facessi tu queste cose" (vedere se l'arma e' ben salda in mano e come si muove nei colpi). Unreal non e' tra le app che
// Claude puo' comandare, quindi il gioco fa le foto da solo e Claude le guarda.
//
// Valdorso.Fotografo [Id ...]   (in Play, giocando da soli: solo dove il server e lo schermo sono lo stesso gioco)
//   Per ogni arma (tutte quelle con una famiglia, o solo quelle scritte): la mette in mano al colono, poi fotografa da tre
//   lati (davanti, di fianco, di tre quarti) il colono fermo con l'arma in mano e il colono nell'istante del colpo (la sua
//   animazione, ferma li'). Le foto vanno in Saved/Claude/Fotografo/<Id>_<presa|colpo>_<lato>.png; in
//   Saved/Claude/Fotografo/fotografo.txt i numeri: dove sta l'arma rispetto all'osso della mano, quanto e' lunga, e quale
//   animazione e istante del colpo. Alla fine lo zaino torna com'era. Dura circa due secondi per arma.
// Unity build: le cose del namespace anonimo cominciano con Fotografo.

#include "ValdorsoArmi.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoQualita.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Valdorso.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "ImageUtils.h"
#include "TextureResource.h"

namespace
{
	/** Il lavoro in corso (uno alla volta). */
	struct FFotografoLavoro
	{
		TWeakObjectPtr<APlayerController> Chi;
		TWeakObjectPtr<ASceneCapture2D> Macchina;
		TWeakObjectPtr<UTextureRenderTarget2D> Pellicola;
		TArray<FName> Armi;
		int32 Prossima = 0;
		FValdorsoZaino Prima;
		TArray<FString> Righe;
		FTimerHandle Timer;
	};

	TSharedPtr<FFotografoLavoro>& FotografoLavoro()
	{
		static TSharedPtr<FFotografoLavoro> Lavoro;
		return Lavoro;
	}

	FString FotografoCartella()
	{
		return FPaths::ProjectSavedDir() / TEXT("Claude") / TEXT("Fotografo");
	}

	/** Tre foto attorno al colono (davanti, di fianco, di tre quarti), a meta' altezza, verso il petto. */
	void FotografoScatta(FFotografoLavoro& L, ACharacter* Colono, const FString& Nome)
	{
		ASceneCapture2D* Macchina = L.Macchina.Get();
		UWorld* Mondo = Colono ? Colono->GetWorld() : nullptr;
		if (!Macchina || !Mondo || !L.Pellicola.IsValid())
		{
			return;
		}
		const FVector Centro = Colono->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		const FVector Avanti = Colono->GetActorForwardVector().GetSafeNormal2D();
		const FVector Destra = Colono->GetActorRightVector().GetSafeNormal2D();
		struct FLato { const TCHAR* Nome; FVector Verso; };
		const FLato Lati[] = {
			{ TEXT("davanti"), Avanti },
			{ TEXT("fianco"), Destra },
			{ TEXT("tre_quarti"), (Avanti + Destra).GetSafeNormal() },
		};
		for (const FLato& Lato : Lati)
		{
			const FVector Dove = Centro + Lato.Verso * 300.f + FVector(0.f, 0.f, 30.f);
			Macchina->SetActorLocationAndRotation(Dove, (Centro - Dove).Rotation());
			USceneCaptureComponent2D* Cattura = Macchina->GetCaptureComponent2D();
			// due volte: la prima "scalda" la luce e l'antialiasing, la seconda e' la foto
			Cattura->CaptureScene();
			Cattura->CaptureScene();
			// la foto in PNG, con l'alfa piena (la cattura la lascia a zero: sembrerebbe vuota)
			FTextureRenderTargetResource* Risorsa = L.Pellicola->GameThread_GetRenderTargetResource();
			TArray<FColor> Punti;
			if (!Risorsa || !Risorsa->ReadPixels(Punti) || Punti.Num() == 0)
			{
				continue;
			}
			for (FColor& Punto : Punti)
			{
				Punto.A = 255;
			}
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(L.Pellicola->SizeX, L.Pellicola->SizeY, TArrayView64<const FColor>(Punti.GetData(), Punti.Num()), Png);
			FFileHelper::SaveArrayToFile(Png, *(FotografoCartella() / FString::Printf(TEXT("%s_%s.png"), *Nome, Lato.Nome)));
		}
	}

	/** I numeri dell'arma in mano: i modelli SM_Arma_ attaccati al colono, rispetto al loro osso. */
	void FotografoNumeri(FFotografoLavoro& L, ACharacter* Colono, FName Id)
	{
		TArray<UStaticMeshComponent*> Modelli;
		Colono->GetComponents<UStaticMeshComponent>(Modelli);
		bool bTrovato = false;
		for (const UStaticMeshComponent* Modello : Modelli)
		{
			const UStaticMesh* Forma = Modello ? Modello->GetStaticMesh() : nullptr;
			if (!Forma || !Forma->GetName().Contains(TEXT("SM_Arma_")) || !Modello->IsVisible())
			{
				continue;
			}
			bTrovato = true;
			const FBoxSphereBounds Misure = Forma->GetBounds();
			const FTransform Rel = Modello->GetRelativeTransform();
			L.Righe.Add(FString::Printf(TEXT("  %s su %s (osso %s): posizione %s, rotazione %s, scala %s, lunga %.0f cm (scatola %s)"),
				*Forma->GetName(), Modello->GetAttachParent() ? *Modello->GetAttachParent()->GetName() : TEXT("-"),
				*Modello->GetAttachSocketName().ToString(), *Rel.GetLocation().ToCompactString(), *Rel.Rotator().ToCompactString(),
				*Rel.GetScale3D().ToCompactString(), Misure.BoxExtent.GetMax() * 2.f * Rel.GetScale3D().GetMax(), *Misure.BoxExtent.ToCompactString()));
		}
		if (!bTrovato)
		{
			L.Righe.Add(FString::Printf(TEXT("  %s: nessun modello SM_Arma_ visibile addosso al colono (manca il modello o non si e' messa in mano)"), *Id.ToString()));
		}
	}

	void FotografoFine(bool bTutto)
	{
		TSharedPtr<FFotografoLavoro> L = FotografoLavoro();
		FotografoLavoro().Reset();
		if (!L.IsValid())
		{
			return;
		}
		APlayerController* Chi = L->Chi.Get();
		if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi))
		{
			const FValdorsoZaino Prima = L->Prima;
			Zaino->Cambia([Prima](FValdorsoZaino& Z) { Z = Prima; return true; });
		}
		if (ACharacter* Colono = Chi ? Cast<ACharacter>(Chi->GetPawn()) : nullptr)
		{
			Colono->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		}
		if (ASceneCapture2D* Macchina = L->Macchina.Get())
		{
			Macchina->Destroy();
		}
		L->Righe.Add(bTutto ? TEXT("FINITO: lo zaino e' tornato com'era.") : TEXT("INTERROTTO (il colono non c'e' piu'): lo zaino e' tornato com'era."));
		const FString File = FotografoCartella() / TEXT("fotografo.txt");
		FFileHelper::SaveStringToFile(FString::Join(L->Righe, TEXT("\n")) + TEXT("\n"), *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Fotografo: fatto, le foto e i numeri sono in %s"), *FotografoCartella());
	}

	/** Un passo del lavoro: 0 le mani vuote, 1 l'arma in mano, 2 la foto della presa e l'animazione del colpo, 3 la foto
	 *  del colpo. */
	void FotografoPasso(int32 Fase)
	{
		TSharedPtr<FFotografoLavoro> L = FotografoLavoro();
		APlayerController* Chi = L.IsValid() ? L->Chi.Get() : nullptr;
		ACharacter* Colono = Chi ? Cast<ACharacter>(Chi->GetPawn()) : nullptr;
		UWorld* Mondo = Colono ? Colono->GetWorld() : nullptr;
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		if (!L.IsValid() || !Colono || !Mondo || !Zaino)
		{
			FotografoFine(false);
			return;
		}
		if (!L->Armi.IsValidIndex(L->Prossima))
		{
			FotografoFine(true);
			return;
		}
		const FName Id = L->Armi[L->Prossima];
		float Attesa = 0.6f;
		int32 Dopo = Fase + 1;
		if (Fase == 0)
		{
			// prima le mani vuote (cosi' l'arma dopo si prende davvero in mano, anche se era riposta)
			Colono->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			Zaino->Cambia([](FValdorsoZaino& Z)
			{
				const bool bPiene = !Z.Destra.IsNone() || !Z.Sinistra.IsNone();
				Z.Destra = NAME_None;
				Z.Sinistra = NAME_None;
				return bPiene;
			});
			Attesa = 0.3f;
		}
		else if (Fase == 1)
		{
			// poi solo lei
			Zaino->Cambia([Id](FValdorsoZaino& Z)
			{
				if (ValdorsoZaino::Conta(Z, Id) == 0)
				{
					ValdorsoZaino::Aggiungi(Z, Id, 1);
				}
				ValdorsoZaino::Impugna(Z, Id);
				return true;
			});
			L->Righe.Add(FString::Printf(TEXT("%s (famiglia %s):"), *Id.ToString(),
				*ValdorsoQualita::NomeFamiglia(ValdorsoQualita::Famiglia(Id)).ToString()));
			Attesa = 0.8f;
		}
		else if (Fase == 2)
		{
			FotografoScatta(*L, Colono, FString::Printf(TEXT("%s_presa"), *Id.ToString()));
			FotografoNumeri(*L, Colono, Id);
			float Istante = 0.f;
			float Velocita = 1.f;
			UAnimSequenceBase* Colpo = ValdorsoArmi::AnimazioneColpo(Id, 0, Istante, Velocita);
			if (Colpo)
			{
				USkeletalMeshComponent* Corpo = Colono->GetMesh();
				Corpo->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				Corpo->PlayAnimation(Colpo, false);
				Corpo->SetPosition(Istante * Colpo->GetPlayLength(), false);
				Corpo->SetPlayRate(0.f);
				L->Righe.Add(FString::Printf(TEXT("  colpo: %s, istante %.2f (%.2f s di %.2f), velocita' %.2f"), *Colpo->GetName(), Istante,
					Istante * Colpo->GetPlayLength(), Colpo->GetPlayLength(), Velocita));
				Attesa = 0.4f;
			}
			else
			{
				L->Righe.Add(TEXT("  colpo: nessuna animazione per questa famiglia (usa il pugno): niente foto del colpo"));
				Dopo = 4;
				Attesa = 0.1f;
			}
		}
		else if (Fase == 3)
		{
			FotografoScatta(*L, Colono, FString::Printf(TEXT("%s_colpo"), *Id.ToString()));
			Colono->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			Attesa = 0.2f;
		}
		if (Dopo >= 4)
		{
			++L->Prossima;
			Dopo = 0;
		}
		Mondo->GetTimerManager().SetTimer(L->Timer, FTimerDelegate::CreateLambda([Dopo]() { FotografoPasso(Dopo); }), Attesa, false);
	}

	void FotografoComando(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		// un lavoro rimasto a meta' (il Play fermato mentre fotografava): si chiude e lo zaino torna com'era
		if (FotografoLavoro().IsValid() && (!FotografoLavoro()->Chi.IsValid() || !FotografoLavoro()->Macchina.IsValid()))
		{
			FotografoFine(false);
		}
		if (FotografoLavoro().IsValid())
		{
			Uscita.Log(TEXT("Il fotografo sta gia' lavorando."));
			return;
		}
		APlayerController* Chi = nullptr;
		if (Mondo)
		{
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				if (It->Get() && It->Get()->IsLocalController() && It->Get()->HasAuthority())
				{
					Chi = It->Get();
					break;
				}
			}
		}
		ACharacter* Colono = Chi ? Cast<ACharacter>(Chi->GetPawn()) : nullptr;
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		if (!Colono || !Zaino)
		{
			Uscita.Log(TEXT("Valdorso.Fotografo: in Play, giocando da soli, con il colono nella valle."));
			return;
		}
		TSharedPtr<FFotografoLavoro> L = MakeShared<FFotografoLavoro>();
		L->Chi = Chi;
		L->Prima = Zaino->Dati();
		for (const FString& Scritto : Argomenti)
		{
			L->Armi.Add(FName(*Scritto));
		}
		if (L->Armi.Num() == 0)
		{
			for (const FValdorsoTipoOggetto& Tipo : ValdorsoOggetti::Catalogo())
			{
				if (ValdorsoQualita::Famiglia(Tipo.Id) != EValdorsoFamigliaArma::Nessuna)
				{
					L->Armi.Add(Tipo.Id);
				}
			}
		}
		// la macchina fotografica: 1024 x 1024, la luce del gioco
		FActorSpawnParameters Come;
		Come.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASceneCapture2D* Macchina = Mondo->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), Colono->GetActorLocation(), FRotator::ZeroRotator, Come);
		if (!Macchina)
		{
			Uscita.Log(TEXT("Valdorso.Fotografo: non riesco a creare la macchina fotografica."));
			return;
		}
		// la pellicola e' della macchina (la tiene viva lei, e se ne va con lei)
		UTextureRenderTarget2D* Pellicola = NewObject<UTextureRenderTarget2D>(Macchina);
		Pellicola->RenderTargetFormat = RTF_RGBA8;
		Pellicola->InitAutoFormat(1024, 1024);
		Pellicola->UpdateResourceImmediate(true);
		L->Pellicola = Pellicola;
		USceneCaptureComponent2D* Cattura = Macchina->GetCaptureComponent2D();
		Cattura->TextureTarget = Pellicola;
		Cattura->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Cattura->FOVAngle = 60.f;
		Cattura->bCaptureEveryFrame = false;
		Cattura->bCaptureOnMovement = false;
		L->Macchina = Macchina;
		IFileManager::Get().MakeDirectory(*FotografoCartella(), true);
		L->Righe.Add(FString::Printf(TEXT("Il fotografo delle armi, %s: %d armi. Le foto: <Id>_presa_<lato>.png e <Id>_colpo_<lato>.png."),
			*FDateTime::Now().ToString(TEXT("%d/%m/%Y %H:%M")), L->Armi.Num()));
		FotografoLavoro() = L;
		Uscita.Logf(TEXT("Il fotografo comincia: %d armi, circa %.0f secondi. Non muovere il colono. Le foto in %s"),
			L->Armi.Num(), L->Armi.Num() * 2.f, *FotografoCartella());
		FotografoPasso(0);
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice FotografoComandoArmi(TEXT("Valdorso.Fotografo"),
		TEXT("Fotografa il colono con ogni arma in mano (ferma e nel colpo, da tre lati) in Saved/Claude/Fotografo. Valdorso.Fotografo [Id ...]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&FotografoComando));
}
