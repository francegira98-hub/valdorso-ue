// Valdorso - La modalità di gioco del menu principale (vedi il .h).

#include "ValdorsoMenuGameMode.h"
#include "ValdorsoMenuController.h"

AValdorsoMenuGameMode::AValdorsoMenuGameMode()
{
	PlayerControllerClass = AValdorsoMenuController::StaticClass();
	DefaultPawnClass = nullptr;
}
