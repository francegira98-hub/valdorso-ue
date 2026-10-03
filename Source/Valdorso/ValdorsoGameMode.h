// Copyright Epic Games, Inc. All Rights Reserved.
// Modificato per Valdorso (03/10/2026, v0.1.2 passo 2.3): chi arriva passa dall'anticamera
// (AValdorsoPlayerController::Accogli) e chi esce libera il suo account.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ValdorsoGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AValdorsoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AValdorsoGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
};
