// Valdorso - Il controllore del menu principale (vedi il .h).

#include "ValdorsoMenuController.h"
#include "SValdorsoMenuPrincipale.h"
#include "ValdorsoScenaMenu.h"
#include "ValdorsoGameInstance.h"
#include "SValdorsoAccesso.h"
#include "SValdorsoVolo.h"
#include "SValdorsoAvvio.h"
#include "SValdorsoImpostazioni.h"
#include "ValdorsoStatoMondo.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
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
#include "ValdorsoMenuValle.h"
#include "ValdorsoImpostazioni.h"
#include "Components/SplineComponent.h"
#include "Misc/PackageName.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"

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

	/** (07/11) La schermata d'avvio: 1 = una volta per avvio del gioco (normale), 0 = mai (per le prove veloci). */
	TAutoConsoleVariable<int32> CVarAvvio(
		TEXT("Valdorso.Avvio"),
		1,
		TEXT("1 = la schermata d'avvio (logo, avviso, premi un tasto) una volta per avvio del gioco; 0 = mai."));

	/** (07/11) Ogni quanti secondi il menu richiede lo stato del mondo al server. */
	constexpr float OgniQuantoLoStato = 30.f;

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

	// (08/11) Nella valle vera (aperta da Lvl_Menu con la modalità del menu): la telecamera del giro, poi come sempre.
	bNellaValle = GetWorld()->GetAuthGameMode<AValdorsoMenuValleGameMode>() != nullptr;
	if (bNellaValle)
	{
		if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
		{
			Istanza->bMenuValleRiuscito = true;
		}
		PreparaGiro();
		Comincia();
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

	// (08/11) Se c'è la valle vera, si va di là: l'avvio, il menu e i messaggi del server li mostra il menu nella valle.
	if (ValleDisponibile())
	{
		VaiNellaValle();
		return;
	}
	Comincia();
}

void AValdorsoMenuController::Comincia()
{
	// (07/11) Una volta per avvio del gioco, prima del menu, la schermata d'avvio. Tornando dal server (messaggio,
	// rientro) l'istanza del gioco è la stessa e l'avvio è già stato visto: si va dritti al menu.
	UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
	if (Istanza && !Istanza->bAvvioMostrato && CVarAvvio.GetValueOnGameThread() != 0)
	{
		Istanza->bAvvioMostrato = true;
		MostraAvvio();
	}
	else
	{
		CreaMenu();
		ControllaRitorno();
	}
}

// ---------------------------------------------------------------------------------------------
// (08/11) Il menu nella valle vera al tramonto
// ---------------------------------------------------------------------------------------------

bool AValdorsoMenuController::ValleDisponibile() const
{
	if (bNellaValle || LivelloMenuValle.IsNone() || !ValdorsoMenuValle::Attiva() || GetNetMode() != NM_Standalone)
	{
		return false;
	}
	const UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
	if (!Istanza)
	{
		return false;
	}
	// Già provata in questo avvio e mai aperta: qualcosa non va, resta la scena di sempre.
	if (Istanza->bMenuValleTentato && !Istanza->bMenuValleRiuscito)
	{
		return false;
	}
	// Collegamento interrotto con il biglietto: si rientra da qui, subito, senza caricare la valle.
	if (!Istanza->NomeRientro().IsEmpty())
	{
		return false;
	}
	if (!FPackageName::DoesPackageExist(LivelloMenuValle.ToString()))
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: la valle %s non c'è, resta la scena di sempre"), *LivelloMenuValle.ToString());
		return false;
	}
	return true;
}

void AValdorsoMenuController::VaiNellaValle()
{
	if (bVersoLaValle)
	{
		return;
	}
	bVersoLaValle = true;
	bValleChiesta = false;
	AttesaValle = 0.f;
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->bMenuValleTentato = true;
		// (08/11) Ogni volta di nuovo: lo rimette a vero solo la valle aperta. Se una volta è andata e la volta dopo no
		// (si torna in Lvl_Menu dalla valle che non si apre), niente giro infinito Lvl_Menu -> valle -> Lvl_Menu.
		Istanza->bMenuValleRiuscito = false;
	}
	// Subito il nero (niente scena di Lvl_Menu per un attimo) e la riga piccola: la valle è grande, il gioco si ferma
	// qualche secondo mentre la apre.
	if (PlayerCameraManager)
	{
		PlayerCameraManager->SetManualCameraFade(1.f, FLinearColor::Black, false);
	}
	TogliScritte();
	SAssignNew(Scritte, SValdorsoVolo);
	Scritte->ImpostaRiga(LOCTEXT("ValleSiSveglia", "La valle si sveglia..."), 1.f);
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Scritte.ToSharedRef(), 30);
	}
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Scritte);
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
	bShowMouseCursor = false;
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: vado nella valle vera (%s)"), *LivelloMenuValle.ToString());
}

void AValdorsoMenuController::AvanzaVersoLaValle(float DeltaSeconds)
{
	AttesaValle += DeltaSeconds;
	if (!bValleChiesta)
	{
		// qualche fotogramma di nero con la riga, poi si apre (mentre si apre lo schermo resta così)
		if (AttesaValle >= 0.25f)
		{
			bValleChiesta = true;
			AttesaValle = 0.f;
			ApriLaValle();
		}
		return;
	}
	// Se dopo l'apertura questo livello è ancora qui, la valle non si è aperta: la scena di sempre.
	if (AttesaValle >= 8.f)
	{
		RinunciaAllaValle();
	}
}

void AValdorsoMenuController::ApriLaValle()
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu: apro %s senza server"), *LivelloMenuValle.ToString());
	UGameplayStatics::OpenLevel(this, LivelloMenuValle, true, ValdorsoMenuValle::OpzioniLivello());
}

void AValdorsoMenuController::RinunciaAllaValle()
{
	UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Menu: la valle %s non si è aperta, resta la scena di sempre"), *LivelloMenuValle.ToString());
	bVersoLaValle = false;
	bValleChiesta = false;
	TogliScritte();
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.6f, FLinearColor::Black, false, false);
	}
	Comincia();
}

void AValdorsoMenuController::PreparaGiro()
{
	GiroValle.Prepara(ValdorsoMenuValle::Tappe());
	DistanzaGiro = 0.f;
	AlzataGiro = 0.f;
	bValleVisibile = false;
	bSuoloTrovato = false;
	AttesaVista = 0.f;
	SuoloDa = -1.f;

	FVector Posizione, Sguardo;
	GiroValle.Posa(0.f, Posizione, Sguardo);
	BersaglioGiro = Sguardo;

	// La telecamera nasce qui: è la vista del controllore, quindi World Partition carica la valle intorno a lei.
	FActorSpawnParameters Parametri;
	Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACameraActor* NuovaTelecamera = GetWorld()->SpawnActor<ACameraActor>(Posizione, (Sguardo - Posizione).Rotation(), Parametri);
	if (NuovaTelecamera)
	{
		NuovaTelecamera->SetActorEnableCollision(false);
		if (UCameraComponent* Componente = NuovaTelecamera->GetCameraComponent())
		{
			Componente->SetFieldOfView(SguardoGiro);
			Componente->bConstrainAspectRatio = false;
			Obiettivo = Componente;
			SguardoBase = SguardoGiro;
		}
		Telecamera = NuovaTelecamera;
		PosizioneBase = Posizione;
		RotazioneBase = NuovaTelecamera->GetActorRotation();
		SetViewTarget(NuovaTelecamera);
	}
	else
	{
		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Menu nella valle: non riesco a far nascere la telecamera"));
	}

	// Il nero finché la valle intorno alla telecamera non è arrivata (lo apre AvanzaGiro).
	if (PlayerCameraManager)
	{
		PlayerCameraManager->SetManualCameraFade(1.f, FLinearColor::Black, false);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu nella valle: giro di %.0f m in %.0f secondi"),
		GiroValle.Lunghezza() / 100.f, ValdorsoMenuValle::SecondiGiro);
}

void AValdorsoMenuController::AvanzaGiro(float DeltaSeconds)
{
	if (!Telecamera.IsValid())
	{
		return;
	}
	// Si parte quando la valle si vede: prima la telecamera resta sulla prima tappa (la prima inquadratura è quella).
	if (bValleVisibile)
	{
		const bool bMeno = ValdorsoImpostazioni::Correnti().bMenoMovimenti;
		DistanzaGiro += ValdorsoMenuValle::Velocita(GiroValle.Lunghezza(), ValdorsoMenuValle::SecondiGiro, bMeno) * DeltaSeconds;
		if (GiroValle.Lunghezza() > 1.f)
		{
			DistanzaGiro = FMath::Fmod(DistanzaGiro, GiroValle.Lunghezza());
		}
	}
	FVector Posizione, Sguardo;
	GiroValle.Posa(DistanzaGiro, Posizione, Sguardo);

	// La sonda: se sotto la telecamera c'è qualcosa (un albero, un tetto, il monte) a meno di 20 m, si sale piano;
	// passato l'ostacolo si torna giù, ancora più piano.
	FHitResult Colpo;
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoMenuGiro), false, Telecamera.Get());
	const FVector Sopra = Posizione + FVector(0.f, 0.f, 5000.f);
	const FVector Sotto = Posizione - FVector(0.f, 0.f, 30000.f);
	float Voluta = 0.f;
	// (una sonda che parte già dentro qualcosa, per esempio una cupola del cielo con le collisioni, non conta)
	if (GetWorld()->LineTraceSingleByChannel(Colpo, Sopra, Sotto, ECC_Visibility, Parametri) && !Colpo.bStartPenetrating)
	{
		bSuoloTrovato = true;
		Voluta = FMath::Min(ValdorsoMenuValle::Alzata(static_cast<float>(Posizione.Z), static_cast<float>(Colpo.ImpactPoint.Z),
			ValdorsoMenuValle::MinimoSicuro), ValdorsoMenuValle::AlzataMassima);
	}
	AlzataGiro = bValleVisibile ? FMath::FInterpTo(AlzataGiro, Voluta, DeltaSeconds, Voluta > AlzataGiro ? 1.5f : 0.4f) : Voluta;
	Posizione.Z += AlzataGiro;
	Sguardo.Z += AlzataGiro * 0.5f;
	BersaglioGiro = Sguardo;
	Telecamera->SetActorLocationAndRotation(Posizione, (Sguardo - Posizione).Rotation());

	// Il nero si apre quando sotto la telecamera c'è la valle (e ancora un secondo e mezzo, perché arrivino i pezzi
	// intorno), al massimo dopo 25 secondi.
	if (!bValleVisibile)
	{
		AttesaVista += DeltaSeconds;
		if (bSuoloTrovato && SuoloDa < 0.f)
		{
			SuoloDa = AttesaVista;
		}
		const bool bPronta = (SuoloDa >= 0.f && AttesaVista - SuoloDa >= 1.5f && AttesaVista >= 2.f) || AttesaVista >= 25.f;
		if (bPronta)
		{
			bValleVisibile = true;
			if (PlayerCameraManager)
			{
				PlayerCameraManager->StartCameraFade(1.f, 0.f, 2.5f, FLinearColor::Black, false, false);
			}
			UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu nella valle: la valle si vede (dopo %.1f secondi%s)"),
				AttesaVista, bSuoloTrovato ? TEXT("") : TEXT(", senza trovare il suolo"));
		}
	}
}

void AValdorsoMenuController::MostraAvvio()
{
	SAssignNew(Avvio, SValdorsoAvvio)
		.OnFine(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::FineAvvio));
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Avvio.ToSharedRef(), 30);
	}
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Avvio);
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
	bShowMouseCursor = false;
	// lo stato del mondo si chiede già adesso: quando compare il menu, la riga è pronta
	ChiediStatoMondo();
}

void AValdorsoMenuController::FineAvvio()
{
	if (Avvio.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Avvio.ToSharedRef());
		}
		Avvio.Reset();
	}
	bShowMouseCursor = true;
	CreaMenu();
	ControllaRitorno();
}

void AValdorsoMenuController::CreaMenu()
{
	if (Menu.IsValid())
	{
		return;
	}
	// Il menu, sopra la scena.
	SAssignNew(Menu, SValdorsoMenuPrincipale)
		.OnEntra(FSimpleDelegate::CreateWeakLambda(this, [this]() { MostraAccesso(FText::GetEmpty()); }))
		.OnDaSolo(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::EsploraDaSolo))
		.OnEsci(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::Esci))
		.OnImpostazioni(FSimpleDelegate::CreateUObject(this, &AValdorsoMenuController::ApriImpostazioni));

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Menu.ToSharedRef(), 10);
	}
	// la risposta arrivata durante l'avvio è già pronta
	if (!UltimaRigaStato.IsEmpty())
	{
		Menu->ImpostaStatoMondo(UltimaRigaStato, UltimaNotiziaStato);
	}

	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Menu->GetPrimoPulsante());
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
	bShowMouseCursor = true;

	// lo stato del mondo: subito (se non è già in viaggio), poi ogni 30 secondi dal Tick
	AttesaStato = 0.f;
	ChiediStatoMondo();
}

void AValdorsoMenuController::ControllaRitorno()
{
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

void AValdorsoMenuController::ApriImpostazioni()
{
	if (Impostazioni.IsValid() || bInTransizione)
	{
		return;
	}
	TWeakObjectPtr<AValdorsoMenuController> Debole(this);
	SAssignNew(Impostazioni, SValdorsoImpostazioni)
		.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoMenuController* Controllore = Debole.Get())
			{
				Controllore->ChiudiImpostazioni();
			}
		}));
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(Impostazioni.ToSharedRef(), 25);
	}
	if (Menu.IsValid())
	{
		Menu->SetEnabled(false);
	}
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Impostazioni->FuocoIniziale());
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Modo);
}

void AValdorsoMenuController::ChiudiImpostazioni()
{
	if (Impostazioni.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Impostazioni.ToSharedRef());
		}
		Impostazioni.Reset();
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

void AValdorsoMenuController::ChiediStatoMondo()
{
	const UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
	const FString Indirizzo = Istanza ? Istanza->IndirizzoDelloStato() : FString();
	if (bStatoInViaggio || Indirizzo.IsEmpty() || !FHttpModule::Get().IsHttpEnabled())
	{
		return;
	}
	bStatoInViaggio = true;
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Richiesta = FHttpModule::Get().CreateRequest();
	Richiesta->SetURL(Indirizzo);
	Richiesta->SetVerb(TEXT("GET"));
	Richiesta->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Richiesta->SetTimeout(5.f);
	Richiesta->OnProcessRequestComplete().BindWeakLambda(this, [this](FHttpRequestPtr, FHttpResponsePtr Risposta, bool bRiuscita)
	{
		bStatoInViaggio = false;
		FValdorsoStatoMondo Stato;
		const bool bLetto = bRiuscita && Risposta.IsValid() && Risposta->GetResponseCode() == 200
			&& ValdorsoStatoMondo::DaJson(Risposta->GetContentAsString(), Stato);
		UltimaRigaStato = bLetto ? ValdorsoStatoMondo::Riga(Stato) : LOCTEXT("ValleDorme", "La valle dorme: il server non risponde.");
		UltimaNotiziaStato = bLetto ? FText::FromString(Stato.Notizia) : FText::GetEmpty();
		if (Menu.IsValid())
		{
			Menu->ImpostaStatoMondo(UltimaRigaStato, UltimaNotiziaStato);
		}
	});
	Richiesta->ProcessRequest();
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

void AValdorsoMenuController::EsploraDaSolo()
{
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->bModoSolo = true;
	}
	RichiestaDaMandare.Reset();
	bVersoIlServer = false;
	bVoloLungo = VoloLungoSempre();
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
	// la prova dello sviluppo resta quella di sempre: un colono qualunque, niente salvato
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->bModoSolo = false;
	}
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
	if (Istanza)
	{
		Istanza->bModoSolo = false;   // con il server si gioca con l'account vero
	}
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
	// (08/11) Nella valle vera si riapre la valle con la modalità del menu (aperta da sola avrebbe quella del gioco).
	if (bNellaValle)
	{
		ApriLaValle();
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void AValdorsoMenuController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ChiudiAccesso();
	RichiestaDaMandare.Reset();
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	// (07/11) l'avvio e le impostazioni, se erano ancora aperti
	if (Viewport && Avvio.IsValid())
	{
		Viewport->RemoveViewportWidgetContent(Avvio.ToSharedRef());
	}
	Avvio.Reset();
	if (Viewport && Impostazioni.IsValid())
	{
		Viewport->RemoveViewportWidgetContent(Impostazioni.ToSharedRef());
	}
	Impostazioni.Reset();
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

	// (08/11) In Lvl_Menu, verso la valle vera: il nero, poi si apre.
	if (bVersoLaValle)
	{
		AvanzaVersoLaValle(DeltaSeconds);
		return;
	}

	// (08/11) Nella valle: il giro della telecamera (durante il volo verso la valle la muove il volo).
	if (bNellaValle && !bInTransizione)
	{
		AvanzaGiro(DeltaSeconds);
	}

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

	// (07/11) Lo stato del mondo, ogni 30 secondi mentre si sta nel menu.
	if (Menu.IsValid())
	{
		AttesaStato += DeltaSeconds;
		if (AttesaStato >= OgniQuantoLoStato)
		{
			AttesaStato = 0.f;
			ChiediStatoMondo();
		}
	}

	// La telecamera respira: si sposta e gira di pochissimo, lentamente (in Lvl_Menu; (08/11) ferma con "Meno movimento").
	if (Telecamera.IsValid() && !bNellaValle)
	{
		Tempo += DeltaSeconds;
		const float Respiro = ValdorsoImpostazioni::Correnti().bMenoMovimenti ? 0.f : 1.f;
		const FVector Spostamento(0.f, FMath::Sin(Tempo * 0.13f) * Ondeggio * Respiro, FMath::Sin(Tempo * 0.09f) * Ondeggio * 0.4f * Respiro);
		const FRotator Giro(FMath::Sin(Tempo * 0.11f) * 0.4f * Respiro, FMath::Sin(Tempo * 0.07f) * 1.2f * Respiro, 0.f);
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
	if (bNellaValle)
	{
		// (08/11) Nella valle non c'è il frammento: si vola verso dove guarda il giro (il villaggio).
		Cuore = BersaglioGiro;
	}
	else
	{
		for (TActorIterator<AValdorsoFrammentoCuore> It(GetWorld()); It; ++It)
		{
			Cuore = It->GetActorLocation() + FVector(0.f, 0.f, 75.f);
			It->Risveglia();
			break;
		}
	}

	if (!Telecamera.IsValid())
	{
		// Senza telecamera non c'è niente da far volare: solo il nero, poi si parte.
		bVoloLungo = false;
		if (PlayerCameraManager)
		{
			PlayerCameraManager->StartCameraFade(PlayerCameraManager->FadeAmount, 1.f, 0.4f, FLinearColor::Black, false, true);
		}
		Arriva();
		return;
	}
	Partenza = Telecamera->GetActorLocation();
	GiroPartenza = Telecamera->GetActorQuat();
	if (bNellaValle)
	{
		// (08/11) Un terzo della strada verso il villaggio (al massimo 40 m), scendendo appena: resta sopra i tetti.
		const FVector Verso = Cuore - Partenza;
		Arrivo = Partenza + Verso.GetSafeNormal() * FMath::Min(static_cast<float>(Verso.Size()) * 0.3f, 4000.f);
	}
	else
	{
		const FVector Direzione = (Partenza - Cuore).GetSafeNormal2D();
		Arrivo = Cuore + Direzione * 230.f + FVector(0.f, 0.f, 15.f);
	}
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
			PlayerCameraManager->StartCameraFade(PlayerCameraManager->FadeAmount, 1.f, Durata * 0.5f, FLinearColor::Black, false, true);
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
	if (bNellaValle && Volo->Percorso)
	{
		// (08/11) Nella valle i punti di Lvl_Menu non valgono: sopra la piazza, poi su verso i monti (ValdorsoMenuValle).
		TArray<FVector> Punti;
		ValdorsoMenuValle::VoloLungo(Partenza, Punti);
		Volo->Percorso->SetSplinePoints(Punti, ESplineCoordinateSpace::World, true);
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
			PlayerCameraManager->StartCameraFade(PlayerCameraManager->FadeAmount, 1.f, Durata * (1.f - Buio), FLinearColor::Black, false, true);
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
