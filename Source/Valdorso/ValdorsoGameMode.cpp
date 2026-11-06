// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValdorsoGameMode.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoCielo.h"
#include "ValdorsoFasiVillaggio.h"
#include "ValdorsoTempo.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AValdorsoGameMode::AValdorsoGameMode()
{
	// stub
}

void AValdorsoGameMode::StartPlay()
{
	Super::StartPlay();
	// Il giorno e la notte: uno solo per mondo, deciso dal server e mandato a tutti.
	UWorld* Mondo = GetWorld();
	if (Mondo && !TActorIterator<AValdorsoCielo>(Mondo))
	{
		FActorSpawnParameters Parametri;
		Parametri.Name = TEXT("ValdorsoCielo");
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Mondo->SpawnActor<AValdorsoCielo>(AValdorsoCielo::StaticClass(), FTransform::Identity, Parametri);
	}
	// (06/11) Le fasi del villaggio: una sola per mondo, decisa dal server (se il livello non ha cose a fasi non fa niente).
	if (Mondo && !TActorIterator<AValdorsoFasiVillaggio>(Mondo))
	{
		FActorSpawnParameters Parametri;
		Parametri.Name = TEXT("ValdorsoFasiVillaggio");
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Mondo->SpawnActor<AValdorsoFasiVillaggio>(AValdorsoFasiVillaggio::StaticClass(), FTransform::Identity, Parametri);
	}
	// (06/11) Il tempo della valle (sereno, nuvoloso, nebbia, pioggia, temporale), deciso dal server.
	if (Mondo && !TActorIterator<AValdorsoTempo>(Mondo))
	{
		FActorSpawnParameters Parametri;
		Parametri.Name = TEXT("ValdorsoTempo");
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Mondo->SpawnActor<AValdorsoTempo>(AValdorsoTempo::StaticClass(), FTransform::Identity, Parametri);
	}
}

void AValdorsoGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Prima l'anticamera: decide se il personaggio può nascere subito (gioco locale, prova nell'editor) o dopo l'accesso.
	if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(NewPlayer))
	{
		Controllore->Accogli();
	}
	Super::PostLogin(NewPlayer);
}

void AValdorsoGameMode::Logout(AController* Exiting)
{
	AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(Exiting);
	if (Controllore)
	{
		// Il personaggio si salva dov'era (se il corpo c'è ancora; il luogo è già stato salvato all'uscita del corpo).
		Controllore->SalvaPersonaggio();
	}
	if (Controllore && Controllore->EAutenticato() && !Controllore->GetAccountId().IsEmpty())
	{
		if (UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this))
		{
			Archivista->SegnaScollegato(Controllore->GetAccountId());
		}
	}
	Super::Logout(Exiting);
}
