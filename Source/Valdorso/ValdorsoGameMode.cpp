// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValdorsoGameMode.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoCielo.h"
#include "ValdorsoFasiVillaggio.h"
#include "ValdorsoTempo.h"
#include "ValdorsoAmbiente.h"
#include "ValdorsoValleViva.h"
#include "ValdorsoLupi.h"
#include "ValdorsoRovine.h"
#include "ValdorsoStatoMondo.h"
#include "ValdorsoStatoColono.h"
#include "ValdorsoGrottaNord.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/NetDriver.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AValdorsoGameMode::AValdorsoGameMode()
{
	// (11/11, v0.1.5) il PlayerState di Valdorso: porta a tutti la forza del colono (ValdorsoStatoColono.h)
	PlayerStateClass = AValdorsoPlayerState::StaticClass();
}

void AValdorsoGameMode::StartPlay()
{
	Super::StartPlay();
	// (13/11) Due server sullo stesso PC: il secondo trova la porta occupata e Unreal lo mette sulla successiva senza
	// dirlo (alla prova del 12/11 la password nuova di Fra era finita nel server sbagliato). Ora il server si ferma.
	if (GetNetMode() == NM_DedicatedServer)
	{
		int32 PortaChiesta = 0;
		UNetDriver* Rete = GetWorld() ? GetWorld()->GetNetDriver() : nullptr;
		if (Rete && FParse::Value(FCommandLine::Get(), TEXT("-port="), PortaChiesta) && PortaChiesta > 0)
		{
			const FString Indirizzo = Rete->LowLevelGetNetworkNumber();
			int32 Due = INDEX_NONE;
			const int32 PortaVera = Indirizzo.FindLastChar(TEXT(':'), Due) ? FCString::Atoi(*Indirizzo.Mid(Due + 1)) : 0;
			if (PortaVera > 0 && PortaVera != PortaChiesta)
			{
				UE_LOG(LogTemp, Error, TEXT("[Valdorso] La porta %d e' gia' occupata: c'e' gia' un server Valdorso acceso su questo PC. Chiudi quello vecchio e riavvia. Questo si ferma."), PortaChiesta);
				FPlatformMisc::RequestExit(false);
				return;
			}
		}
	}
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
	// (06/11) I suoni della valle intorno al colono (vento, uccelli, grilli, l'acqua più vicina): ogni PC li suona da sé.
	if (Mondo && !TActorIterator<AValdorsoAmbiente>(Mondo))
	{
		FActorSpawnParameters Parametri;
		Parametri.Name = TEXT("ValdorsoAmbiente");
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Mondo->SpawnActor<AValdorsoAmbiente>(AValdorsoAmbiente::StaticClass(), FTransform::Identity, Parametri);
	}
	// (06/11) La valle che reagisce: stormi che si alzano, pesci che saltano (ogni PC per il suo colono).
	if (Mondo && !TActorIterator<AValdorsoValleViva>(Mondo))
	{
		FActorSpawnParameters Parametri;
		Parametri.Name = TEXT("ValdorsoValleViva");
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Mondo->SpawnActor<AValdorsoValleViva>(AValdorsoValleViva::StaticClass(), FTransform::Identity, Parametri);
	}
	// (06/11) Le rune delle rovine che di notte si accendono (ogni PC le accende da sé, dall'ora del cielo).
	if (Mondo && !TActorIterator<AValdorsoRuneNotte>(Mondo))
	{
		FActorSpawnParameters Parametri;
		Parametri.Name = TEXT("ValdorsoRuneNotte");
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Mondo->SpawnActor<AValdorsoRuneNotte>(AValdorsoRuneNotte::StaticClass(), FTransform::Identity, Parametri);
	}
	// (09/11, v0.1.4) Le tane degli animali (dal 10/11 non solo i lupi) (solo il server: i lupi arrivano a tutti come personaggi).
	if (Mondo && GetNetMode() != NM_Client && !TActorIterator<AValdorsoTane>(Mondo))
	{
		FActorSpawnParameters Parametri;
		Parametri.Name = TEXT("ValdorsoTane");
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Mondo->SpawnActor<AValdorsoTane>(AValdorsoTane::StaticClass(), FTransform::Identity, Parametri);
	}
	// (07/11) Lo stato del mondo che il server racconta al menu e al launcher (ora, tempo, coloni, notizia): solo il server.
	if (Mondo && GetNetMode() != NM_Client && !TActorIterator<AValdorsoStatoMondo>(Mondo))
	{
		FActorSpawnParameters Parametri;
		Parametri.Name = TEXT("ValdorsoStatoMondo");
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Mondo->SpawnActor<AValdorsoStatoMondo>(AValdorsoStatoMondo::StaticClass(), FTransform::Identity, Parametri);
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
