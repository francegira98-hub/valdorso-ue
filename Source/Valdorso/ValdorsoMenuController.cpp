// Valdorso - Il controllore del menu principale (vedi il .h).

#include "ValdorsoMenuController.h"
#include "SValdorsoMenuPrincipale.h"
#include "ValdorsoScenaMenu.h"
#include "ValdorsoGameInstance.h"
#include "SValdorsoAccesso.h"
#include "SValdorsoVolo.h"
#include "ValdorsoVoloIngresso.h"
#include "Camera/CameraComponent.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ValdorsoTemaUI.h"
#include "HAL/PlatformTime.h"
#include "Valdorso.h"

#define LOCTEXT_NAMESPACE "ValdorsoMenuController"

namespace
{
#if !UE_BUILD_SHIPPING
	/** Solo sviluppo: per provare il volo lungo senza creare ogni volta un account nuovo. */
	TAutoConsoleVariable<int32> CVarVoloLungo(
		TEXT("Valdorso.VoloLungo"),
		0,
		TEXT("0 = il volo lungo solo al primo ingresso (normale); 1 = sempre, anche con Entra e con \"prova senza server\"."));
#endif

	bool VoloLungoSempre()
	{
#if !UE_BUILD_SHIPPING
		return CVarVoloLungo.GetValueOnGameThread() != 0;
#else
		return false;
#endif
	}

	/** Parte piano e finisce piano. */
	float MorbidoVolo(float A)
	{
		A = FMath::Clamp(A, 0.f, 1.f);
		return A * A * (3.f - 2.f * A);
	}
}

AValdorsoMenuController::AValdorsoMenuController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
}

void AValdorsoMenuController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// La telecamera della scena: quella con l'etichetta MenuCamera, altrimenti la prima che c'è.
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MenuCamera")) || !Telecamera.IsValid())
		{
			Telecamera = *It;
			if (It->ActorHasTag(TEXT("MenuCamera")))
			{
				break;
			}
		}
	}
	if (Telecamera.IsValid())
	{
		PosizioneBase = Telecamera->GetActorLocation();
		RotazioneBase = Telecamera->GetActorRotation();
		SetViewTarget(Telecamera.Get());
		Obiettivo = Telecamera->FindComponentByClass<UCameraComponent>();
		if (Obiettivo.IsValid())
		{
			SguardoBase = Obiettivo->FieldOfView;
		}
	}
	else
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Menu: nessuna telecamera MenuCamera nel livello"));
	}

	// Il menu, sopra la scena.
	SAssignNew(Menu, SValdorsoMenuPrincipale)
		.OnEntra(FSimpleDelegate::CreateWeakLambda(this, [this]() { MostraAccesso(FText::GetEmpty()); }))
		.OnEsci(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::Esci));

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Menu.ToSharedRef(), 10);
	}

	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Menu->GetPrimoPulsante());
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
	bShowMouseCursor = true;

	// Tornando dal server (password sbagliata, espulsione, server spento) si riapre "Prima di entrare" con il motivo.
	// Se invece il collegamento si è solo interrotto e c'è il biglietto, si rientra da soli (fino a 3 volte).
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		const FString Messaggio = Istanza->PrendiMessaggio();
		FValdorsoRichiestaAccesso Rientro;
		if (Istanza->PrendiRientro(Rientro))
		{
			PreparaRientro(Rientro, Messaggio);
		}
		else if (!Messaggio.IsEmpty())
		{
			UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: %s"), *Messaggio);
			MostraAccesso(FText::FromString(Messaggio));
		}
	}
}

void AValdorsoMenuController::MostraAccesso(const FText& Messaggio)
{
	if (bInTransizione)
	{
		return;
	}
	ChiudiAccesso();

	const UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
	SAssignNew(Accesso, SValdorsoPrimaDiEntrare)
		.NomeIniziale(Istanza ? Istanza->UltimoNome : FString())
		.Messaggio(Messaggio)
		.OnRichiesta(FValdorsoSuRichiestaAccesso::CreateUObject(this, &AValdorsoMenuController::SuRichiestaAccesso))
		.OnIndietro(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::ChiudiAccesso))
		.OnProvaLocale(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::ProvaLocale));

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Accesso.ToSharedRef(), 20);
	}
	if (Menu.IsValid())
	{
		Menu->SetEnabled(false);
	}

	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Accesso->CampoIniziale());
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
}

void AValdorsoMenuController::ChiudiAccesso()
{
	if (Accesso.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Accesso.ToSharedRef());
		}
		Accesso.Reset();
	}
	if (Menu.IsValid() && !bInTransizione)
	{
		Menu->SetEnabled(true);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Menu->GetPrimoPulsante());
		Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Modo);
	}
}

void AValdorsoMenuController::SuRichiestaAccesso(const FValdorsoRichiestaAccesso& Richiesta)
{
	// Prima si controlla che il viaggio possa partire: l'errore si vede subito, nel pannello, non dopo il nero.
	UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
	FString Errore;
	if (!Istanza || !Istanza->PuoiCollegarti(Errore))
	{
		if (Accesso.IsValid())
		{
			Accesso->MostraMessaggio(FText::FromString(Errore.IsEmpty() ? TEXT("Il gioco non è pronto: riprova.") : Errore), true);
		}
		return;
	}

	// Il collegamento parte alla fine del volo (Arriva): così il volo si vede tutto.
	RichiestaDaMandare = Richiesta;
	bVersoIlServer = true;
	bVoloLungo = Richiesta.Modo == EValdorsoModoAccesso::PrimoIngresso || VoloLungoSempre();
	// (05/10) Il pannello non si chiude di colpo: chiudendolo, per mezzo secondo si rivedeva il menu dietro
	// ("Entra nella valle", "Impostazioni"). Ora il menu sparisce subito e il pannello sfuma con il volo.
	if (Menu.IsValid())
	{
		Menu->SetVisibility(EVisibility::Collapsed);
	}
	if (Accesso.IsValid())
	{
		Accesso->SetVisibility(EVisibility::HitTestInvisible);
	}
	EntraNellaValle();
}

void AValdorsoMenuController::ProvaLocale()
{
	RichiestaDaMandare.Reset();
	bVersoIlServer = false;
	bVoloLungo = VoloLungoSempre();
	// Come per l'accesso vero: niente menu che ricompare per un attimo.
	if (Menu.IsValid())
	{
		Menu->SetVisibility(EVisibility::Collapsed);
	}
	if (Accesso.IsValid())
	{
		Accesso->SetVisibility(EVisibility::HitTestInvisible);
	}
	EntraNellaValle();
}

void AValdorsoMenuController::Arriva()
{
	if (bLivelloAperto)
	{
		return;
	}
	bLivelloAperto = true;
	// Sul nero non deve restare niente del menu o del pannello (anche se il volo è stato saltato subito).
	SfumaSopra(0.f);

	if (!bVersoIlServer)
	{
		UGameplayStatics::OpenLevel(this, LivelloValle);
		return;
	}

	// Ora, sul nero, parte il collegamento. La richiesta passa all'istanza del gioco e qui non resta niente.
	UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
	FString Errore;
	const bool bPartito = Istanza && RichiestaDaMandare.IsSet() && Istanza->Collegati(RichiestaDaMandare.GetValue(), Errore);
	RichiestaDaMandare.Reset();
	if (!bPartito)
	{
		TornaAlMenu(Errore.IsEmpty() ? TEXT("Il gioco non è pronto: riprova.") : Errore);
		return;
	}
	TempoCollegamento = 0.f;
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: volo finito, mi collego al server"));
}

void AValdorsoMenuController::PreparaRientro(const FValdorsoRichiestaAccesso& Rientro, const FString& Messaggio)
{
	RientroInAttesa = Rientro;
	MessaggioRientro = Messaggio;
	AttesaRientro = 2.5f;
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: collegamento interrotto (%s), rientro di %s tra poco"), *Messaggio, *Rientro.Nome);

	if (Menu.IsValid())
	{
		Menu->SetEnabled(false);
	}
	SAssignNew(Scritte, SValdorsoVolo)
		.OnSalta(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::AnnullaRientro));
	Scritte->ImpostaRiga(FText::Format(LOCTEXT("Rientro", "Il collegamento si è interrotto. Rientro nella valle come {0}..."),
		FText::FromString(Rientro.Nome)), 1.f);
	Scritte->ImpostaAiuto(LOCTEXT("Annulla", "ESC  ANNULLA"));
	Scritte->ImpostaSaltabile(true);
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Scritte.ToSharedRef(), 30);
	}
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Scritte);
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
}

void AValdorsoMenuController::AvanzaRientro(float DeltaSeconds)
{
	AttesaRientro -= DeltaSeconds;
	if (AttesaRientro > 0.f)
	{
		return;
	}
	const FValdorsoRichiestaAccesso Rientro = RientroInAttesa.GetValue();
	RientroInAttesa.Reset();
	TogliScritte();

	UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
	FString Errore;
	if (!Istanza || !Istanza->PuoiCollegarti(Errore))
	{
		MostraAccesso(FText::FromString(Errore.IsEmpty() ? MessaggioRientro : Errore));
		return;
	}
	// Come "Entra": il volo corto, il nero, poi il collegamento con il biglietto.
	RichiestaDaMandare = Rientro;
	bVersoIlServer = true;
	bVoloLungo = false;
	EntraNellaValle();
}

void AValdorsoMenuController::AnnullaRientro()
{
	if (!RientroInAttesa.IsSet())
	{
		return;
	}
	RientroInAttesa.Reset();
	TogliScritte();
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->DimenticaBiglietto();
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: rientro annullato"));
	MostraAccesso(FText::FromString(MessaggioRientro));
}

void AValdorsoMenuController::TogliScritte()
{
	if (Scritte.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Scritte.ToSharedRef());
		}
		Scritte.Reset();
	}
}

void AValdorsoMenuController::TornaAlMenu(const FString& Motivo)
{
	// Il modo più pulito: si riapre il livello del menu, che all'avvio mostra "Prima di entrare" con il motivo.
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->RicordaMessaggio(Motivo);
	}
	UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Menu: il collegamento non parte (%s)"), *Motivo);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void AValdorsoMenuController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ChiudiAccesso();
	RichiestaDaMandare.Reset();
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (Menu.IsValid())
	{
		if (Viewport)
		{
			Viewport->RemoveViewportWidgetContent(Menu.ToSharedRef());
		}
		Menu.Reset();
	}
	if (Scritte.IsValid())
	{
		if (Viewport)
		{
			Viewport->RemoveViewportWidgetContent(Scritte.ToSharedRef());
		}
		Scritte.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void AValdorsoMenuController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RientroInAttesa.IsSet())
	{
		AvanzaRientro(DeltaSeconds);
		return;
	}

	if (bInTransizione)
	{
		if (bLivelloAperto)
		{
			AvanzaCollegamento(DeltaSeconds);
		}
		else if (bVoloLungo)
		{
			AvanzaVoloLungo(DeltaSeconds);
		}
		else
		{
			AvanzaTransizione(DeltaSeconds);
		}
		return;
	}

	// La telecamera respira: si sposta e gira di pochissimo, lentamente.
	if (Telecamera.IsValid())
	{
		Tempo += DeltaSeconds;
		const FVector Spostamento(0.f, FMath::Sin(Tempo * 0.13f) * Ondeggio, FMath::Sin(Tempo * 0.09f) * Ondeggio * 0.4f);
		const FRotator Giro(FMath::Sin(Tempo * 0.11f) * 0.4f, FMath::Sin(Tempo * 0.07f) * 1.2f, 0.f);
		Telecamera->SetActorLocationAndRotation(PosizioneBase + Spostamento, RotazioneBase + Giro);
	}
}

void AValdorsoMenuController::EntraNellaValle()
{
	if (bInTransizione)
	{
		return;
	}
	bInTransizione = true;
	TempoTransizione = 0.f;

	// Il menu non risponde più e sfuma; sopra arrivano le scritte del volo, che ascoltano tastiera e controller
	// (Esc salta il volo lungo; tutto il resto si ferma lì).
	bShowMouseCursor = false;
	if (Menu.IsValid() && Menu->GetVisibility() != EVisibility::Collapsed)
	{
		Menu->SetVisibility(EVisibility::HitTestInvisible);
	}
	SAssignNew(Scritte, SValdorsoVolo)
		.OnSalta(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::SaltaVolo));
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Scritte.ToSharedRef(), 30);
	}
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Scritte);
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: entra nella valle (%s, volo %s)"),
		bVersoIlServer ? TEXT("server") : *LivelloValle.ToString(), bVoloLungo ? TEXT("lungo") : TEXT("corto"));

	// Il frammento si risveglia e la telecamera gli vola incontro.
	FVector Cuore(0.f, 0.f, 165.f);
	for (TActorIterator<AValdorsoFrammentoCuore> It(GetWorld()); It; ++It)
	{
		Cuore = It->GetActorLocation() + FVector(0.f, 0.f, 75.f);
		It->Risveglia();
		break;
	}

	if (!Telecamera.IsValid())
	{
		// Senza telecamera non c'è niente da far volare: solo il nero, poi si parte.
		bVoloLungo = false;
		if (PlayerCameraManager)
		{
			PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.4f, FLinearColor::Black, false, true);
		}
		Arriva();
		return;
	}
	Partenza = Telecamera->GetActorLocation();
	GiroPartenza = Telecamera->GetActorQuat();
	const FVector Direzione = (Partenza - Cuore).GetSafeNormal2D();
	Arrivo = Cuore + Direzione * 230.f + FVector(0.f, 0.f, 15.f);
	GiroArrivo = (Cuore - Arrivo).Rotation().Quaternion();

	// Al primo ingresso, il volo lungo (se la curva manca del tutto, resta il volo corto qui sopra).
	if (bVoloLungo)
	{
		PreparaVoloLungo();
	}
}

void AValdorsoMenuController::AvanzaTransizione(float DeltaSeconds)
{
	TempoTransizione += DeltaSeconds;
	const float Durata = FMath::Max(DurataTransizione, 0.5f);
	const float A = FMath::Clamp(TempoTransizione / Durata, 0.f, 1.f);

	// Parte piano e accelera, come chi si avvicina a qualcosa che lo chiama.
	const float Corsa = A * A * A;
	const float Sguardo = FMath::InterpEaseInOut(0.f, 1.f, FMath::Min(A * 1.6f, 1.f), 2.f);
	if (Telecamera.IsValid())
	{
		Telecamera->SetActorLocationAndRotation(FMath::Lerp(Partenza, Arrivo, Corsa), FQuat::Slerp(GiroPartenza, GiroArrivo, Sguardo));
	}

	// Il menu sfuma nel primo mezzo secondo.
	SfumaSopra(1.f - FMath::Clamp(TempoTransizione / 0.5f, 0.f, 1.f));

	// Dopo un po' tutto diventa nero.
	if (!bBuio && A >= 0.45f)
	{
		bBuio = true;
		if (PlayerCameraManager)
		{
			PlayerCameraManager->StartCameraFade(0.f, 1.f, Durata * 0.5f, FLinearColor::Black, false, true);
		}
	}

	if (!bLivelloAperto && A >= 1.f)
	{
		Arriva();
	}
}

void AValdorsoMenuController::SfumaSopra(float Opacita)
{
	if (Menu.IsValid())
	{
		Menu->SetRenderOpacity(Opacita);
	}
	if (Accesso.IsValid())
	{
		Accesso->SetRenderOpacity(Opacita);
		// (05/10) I bordi arrotondati non seguono del tutto la trasparenza: a fine sfumatura il pannello si toglie.
		// (Durante il volo ChiudiAccesso non riaccende il menu.)
		if (Opacita <= 0.01f)
		{
			ChiudiAccesso();
		}
	}
	if (Menu.IsValid() && Opacita <= 0.01f)
	{
		Menu->SetVisibility(EVisibility::Collapsed);
	}
}

void AValdorsoMenuController::PreparaVoloLungo()
{
	// La curva messa in Lvl_Menu; se non c'è, se ne crea una con i punti di serie.
	for (TActorIterator<AValdorsoVoloIngresso> It(GetWorld()); It; ++It)
	{
		Volo = *It;
		break;
	}
	if (!Volo.IsValid())
	{
		FActorSpawnParameters Parametri;
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Volo = GetWorld()->SpawnActor<AValdorsoVoloIngresso>(FVector::ZeroVector, FRotator::ZeroRotator, Parametri);
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: nel livello non c'è VoloIngresso, uso i punti di serie"));
	}
	if (!Volo.IsValid())
	{
		bVoloLungo = false;   // non dovrebbe succedere: si ripiega sul volo corto
		return;
	}
	Volo->PartiDa(Partenza);

	// Il Cuore batte più forte: anche le rune si accendono.
	for (TActorIterator<AValdorsoCerchioRune> It(GetWorld()); It; ++It)
	{
		It->Risveglia();
	}
	if (Scritte.IsValid())
	{
		Scritte->ImpostaSaltabile(true);
	}
}

void AValdorsoMenuController::AvanzaVoloLungo(float DeltaSeconds)
{
	if (!Volo.IsValid() || !Telecamera.IsValid())
	{
		Arriva();
		return;
	}

	TempoTransizione += DeltaSeconds;
	const float Durata = FMath::Max(Volo->Durata, 1.f);
	const float A = FMath::Clamp(TempoTransizione / Durata, 0.f, 1.f);

	// Lungo la curva; nel primo secondo lo sguardo passa piano da quello del menu a quello del volo.
	FVector Posizione, Bersaglio;
	Volo->Posa(A, Posizione, Bersaglio);
	const FQuat GiroVolo = (Bersaglio - Posizione).Rotation().Quaternion();
	const FQuat Giro = FQuat::Slerp(GiroPartenza, GiroVolo, MorbidoVolo(TempoTransizione / 1.2f));
	Telecamera->SetActorLocationAndRotation(Posizione, Giro);

	// Il Cuore batte più forte: a ogni colpo lo sguardo si stringe un poco; correndo si allarga.
	if (Obiettivo.IsValid())
	{
		const float Colpo = ValdorsoTema::Battito(FPlatformTime::Seconds());
		Obiettivo->SetFieldOfView(SguardoBase + 12.f * MorbidoVolo((A - 0.45f) / 0.55f) - ColpoSulloSguardo * Colpo);
	}

	// Il menu sfuma nel primo mezzo secondo; "La valle ti accoglie" compare dopo un secondo e mezzo.
	SfumaSopra(1.f - FMath::Clamp(TempoTransizione / 0.5f, 0.f, 1.f));
	if (Scritte.IsValid())
	{
		Scritte->ImpostaTitolo(MorbidoVolo((TempoTransizione - 1.5f) / 1.2f));
	}

	// Verso la fine tutto diventa nero.
	const float Buio = FMath::Clamp(Volo->InizioBuio, 0.3f, 0.98f);
	if (!bBuio && A >= Buio)
	{
		bBuio = true;
		if (PlayerCameraManager)
		{
			PlayerCameraManager->StartCameraFade(0.f, 1.f, Durata * (1.f - Buio), FLinearColor::Black, false, true);
		}
	}

	if (bSaltato)
	{
		AttesaSalto -= DeltaSeconds;
		if (AttesaSalto <= 0.f)
		{
			Arriva();
		}
		return;
	}
	if (A >= 1.f)
	{
		Arriva();
	}
}

void AValdorsoMenuController::SaltaVolo()
{
	if (!bInTransizione || !bVoloLungo || bLivelloAperto || bSaltato)
	{
		return;
	}
	// Mezzo secondo di nero e si parte.
	bSaltato = true;
	AttesaSalto = 0.45f;
	bBuio = true;
	if (PlayerCameraManager)
	{
		// Dal nero che c'è già (se il volo stava finendo) al nero pieno, in 0,4 secondi.
		PlayerCameraManager->StartCameraFade(PlayerCameraManager->FadeAmount, 1.f, 0.4f, FLinearColor::Black, false, true);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: volo saltato"));
}

void AValdorsoMenuController::AvanzaCollegamento(float DeltaSeconds)
{
	TempoCollegamento += DeltaSeconds;
	if (!Scritte.IsValid())
	{
		return;
	}
	Scritte->ImpostaSaltabile(false);
	// Dopo un attimo di nero, la riga piccola: si capisce che il gioco sta lavorando.
	if (bVersoIlServer)
	{
		Scritte->ImpostaRiga(LOCTEXT("InCammino", "In cammino verso la valle..."), MorbidoVolo((TempoCollegamento - 0.6f) / 0.8f));
	}
}

void AValdorsoMenuController::Esci()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
