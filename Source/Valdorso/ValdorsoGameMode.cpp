// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValdorsoGameMode.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoPlayerController.h"

AValdorsoGameMode::AValdorsoGameMode()
{
	// stub
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
	const AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(Exiting);
	if (Controllore && Controllore->EAutenticato() && !Controllore->GetAccountId().IsEmpty())
	{
		if (UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this))
		{
			Archivista->SegnaScollegato(Controllore->GetAccountId());
		}
	}
	Super::Logout(Exiting);
}
