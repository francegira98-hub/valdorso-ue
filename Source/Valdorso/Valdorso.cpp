// Copyright Epic Games, Inc. All Rights Reserved.

#include "Valdorso.h"
#include "ValdorsoVersione.h"
#include "Misc/NetworkVersion.h"
#include "Modules/ModuleManager.h"

// (11/11, v0.1.7) Il modulo del gioco: all'avvio dice al motore il numero di rete di questa versione
// (ValdorsoVersione::DiRete), cosi' il server non fa entrare un gioco di un'altra versione.
class FValdorsoModulo : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		FNetworkVersion::GetLocalNetworkVersionOverride.BindStatic(&ValdorsoVersione::DiRete);
		// (se qualcuno l'ha gia' chiesto prima di noi, il motore l'ha in memoria: si ricalcola)
		FNetworkVersion::InvalidateNetworkChecksum();
	}

	virtual void ShutdownModule() override
	{
		FNetworkVersion::GetLocalNetworkVersionOverride.Unbind();
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FValdorsoModulo, Valdorso, "Valdorso" );

DEFINE_LOG_CATEGORY(LogValdorso)