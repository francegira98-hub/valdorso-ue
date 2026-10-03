// Valdorso - La modalità di gioco del menu principale: nessun personaggio, solo il controllore del menu.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ValdorsoMenuGameMode.generated.h"

UCLASS()
class VALDORSO_API AValdorsoMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AValdorsoMenuGameMode();
};
