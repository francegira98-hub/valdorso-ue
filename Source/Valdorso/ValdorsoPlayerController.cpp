// Copyright Epic Games, Inc. All Rights Reserved.


#include "ValdorsoPlayerController.h"
#include "SValdorsoGilda.h"
#include "SValdorsoScheda.h"
#include "ValdorsoCrescita.h"
#include "ValdorsoGilda.h"
#include "ValdorsoStaff.h"
#include "GameFramework/Character.h"
#include "ValdorsoMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Valdorso.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "ValdorsoGameInstance.h"
#include "SValdorsoBottega.h"
#include "ValdorsoArmi.h"
#include "ValdorsoCombattimento.h"
#include "SValdorsoAccesso.h"
#include "SValdorsoPersonaggi.h"
#include "SValdorsoRegistro.h"
#include "SValdorsoPausa.h"
#include "SValdorsoImpostazioni.h"
#include "ValdorsoImpostazioni.h"
#include "ValdorsoPalcoRegistro.h"
#include "ValdorsoRegole.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoInterazione.h"
#include "SValdorsoBacheca.h"
#include "SValdorsoDialogo.h"
#include "ValdorsoPNG.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoZaino.h"
#include "SValdorsoTessera.h"
#include "ValdorsoBarre.h"
#include "ValdorsoChat.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoRune.h"
#include "SValdorsoMappa.h"
#include "ValdorsoModoFoto.h"
#include "ValdorsoTemaUI.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/SlateBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ValdorsoAspettoComponent.h"
#include "ValdorsoSeduta.h"
#include "ValdorsoLetto.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/InputComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

namespace
{
	/** Secondi per entrare dopo il collegamento. */
	constexpr float SecondiAnticamera = 60.f;

	/** Richieste massime nell'anticamera prima di essere scollegati. */
	constexpr int32 RichiesteMassime = 5;

	/** (02/11, passo 6) Quanto resta il colono nella valle dopo aver chiesto di uscire. */
	constexpr float SecondiUscita = 20.f;

	/** Oltre questa distanza (cm, in piano) dal punto in cui ha chiesto di uscire, l'uscita si annulla. */
	constexpr float SpostamentoMassimoUscita = 150.f;
}

AValdorsoPlayerController::AValdorsoPlayerController()
{
	// (07/11) il taccuino di viaggio e la bussola (ValdorsoTaccuino.h)
	Taccuino = CreateDefaultSubobject<UValdorsoTaccuinoComponent>(TEXT("Taccuino"));
	// (08/11, v0.1.4) lo zaino, con il peso e le monete (ValdorsoZaino.h)
	Zaino = CreateDefaultSubobject<UValdorsoZainoComponent>(TEXT("Zaino"));
	// (08/11, v0.1.4) le barre con la barra delle azioni (ValdorsoBarre.h) e la chat di prossimità (ValdorsoChat.h)
	Barre = CreateDefaultSubobject<UValdorsoBarreComponent>(TEXT("Barre"));
	Chat = CreateDefaultSubobject<UValdorsoChatComponent>(TEXT("Chat"));
	// (09/11, v0.1.4 passo 4.4) l'aggancio del bersaglio (ValdorsoCombattimento.h)
	Aggancio = CreateDefaultSubobject<UValdorsoAggancioComponent>(TEXT("Aggancio"));
	// (10/11, v0.1.4) la Gilda: punti e incarichi (ValdorsoGilda.h)
	Gilda = CreateDefaultSubobject<UValdorsoGildaComponent>(TEXT("Gilda"));
	// (10/11) la finestra dello staff, F9 (ValdorsoStaff.h)
	Staff = CreateDefaultSubobject<UValdorsoStaffComponent>(TEXT("Staff"));
	// (10/11, scheletro della v0.1.5) le abilita' e la scheda del personaggio, K (ValdorsoCrescita.h)
	Crescita = CreateDefaultSubobject<UValdorsoCrescitaComponent>(TEXT("Crescita"));
}

void AValdorsoPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogValdorso, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	// (10/11) la distanza visiva scelta nelle impostazioni (anche il caricamento della valle: serve il mondo)
	if (IsLocalPlayerController())
	{
		ValdorsoImpostazioni::ApplicaDistanza();
	}

	// Valdorso, anticamera: appena arrivato sul server, il client manda la sua richiesta (solo su connessione cifrata).
	if (IsLocalPlayerController() && GetNetMode() == NM_Client)
	{
		UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();
		FValdorsoRichiestaAccesso Richiesta;
		if (Istanza && Istanza->PrendiRichiesta(Richiesta))
		{
			if (!ConnessioneCifrata())
			{
				Istanza->RicordaMessaggio(TEXT("La connessione non è cifrata: la password non è partita."));
				ConsoleCommand(TEXT("disconnect"));
				return;
			}
			MostraAnticamera();
			if (Richiesta.Modo == EValdorsoModoAccesso::PrimoIngresso)
			{
				ServerPrimoIngresso(Richiesta.CodiceInvito, Richiesta.Nome, Richiesta.Password);
			}
			else if (Richiesta.Modo == EValdorsoModoAccesso::Rientro)
			{
				ServerRientra(Richiesta.Biglietto);
			}
			else if (Richiesta.Modo == EValdorsoModoAccesso::Recupero)
			{
				ServerRecupera(Richiesta.Nome, Richiesta.CodiceRecupero, Richiesta.Password);
			}
			else
			{
				ServerAccedi(Richiesta.Nome, Richiesta.Password);
			}
		}
	}
}

void AValdorsoPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// (02/11, passo 6) La pausa: Esc, F10 (nell'editor Esc ferma il Play) e Start sul controller.
	if (IsLocalPlayerController() && InputComponent)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AValdorsoPlayerController::TastoPausa);
		InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &AValdorsoPlayerController::TastoPausa);
		InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &AValdorsoPlayerController::TastoPausa);
		// (03/11, v0.1.3) Le interazioni: E, e X (tasto a sinistra) sul controller.
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AValdorsoPlayerController::TastoInteragisci);
		InputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AValdorsoPlayerController::TastoInteragisci);
		// (06/11) la mappa di carta
		InputComponent->BindKey(EKeys::M, IE_Pressed, this, &AValdorsoPlayerController::TastoMappa);
		InputComponent->BindKey(EKeys::Gamepad_Special_Left, IE_Pressed, this, &AValdorsoPlayerController::TastoMappa);
		// (07/11) la modalità foto (dentro la foto i tasti li prende lo schermo della foto: P o Esc per uscire)
		InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AValdorsoPlayerController::TastoFoto);
		InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AValdorsoPlayerController::TastoQuaderno);
		// (07/11) il taccuino di viaggio
		InputComponent->BindKey(EKeys::J, IE_Pressed, this, &AValdorsoPlayerController::TastoTaccuino);
		// (08/11, v0.1.4) lo zaino (dentro, I ed Esc lo chiudono da sé)
		InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AValdorsoPlayerController::TastoZaino);
		// (08/11, v0.1.4) 1-8 la barra delle azioni, Invio la chat. Dentro il dialogo (1-4, Invio), il taccuino (1-3) e la
		// modalità foto (1-5, Invio) i tasti li prendono quelle schermate (hanno la tastiera tutta per sé, qui non arrivano);
		// in più TastoAzione e TastoChat non fanno niente se c'è una schermata aperta.
		const FKey TastiAzioni[ValdorsoBarre::NumeroAzioni] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight };
		for (int32 Indice = 0; Indice < ValdorsoBarre::NumeroAzioni; ++Indice)
		{
			FInputKeyBinding Legame(FInputChord(TastiAzioni[Indice], false, false, false, false), IE_Pressed);
			Legame.KeyDelegate.GetDelegateForManualSet().BindUObject(this, &AValdorsoPlayerController::TastoAzione, Indice);
			InputComponent->KeyBindings.Add(Legame);
		}
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AValdorsoPlayerController::TastoChat);
		// (09/11, v0.1.4 passo 4.4) l'aggancio del bersaglio
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AValdorsoPlayerController::TastoAggancio);
		InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AValdorsoPlayerController::TastoAggancio);
		InputComponent->BindKey(EKeys::Gamepad_RightThumbstick, IE_Pressed, this, &AValdorsoPlayerController::TastoAggancio);
		// (09/11) Z: riponi o riprendi le armi
		InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AValdorsoPlayerController::TastoRiponi);
		// (10/11) F9: la finestra dello staff (e del capo gilda)
		InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AValdorsoPlayerController::TastoStaff);
		// (nell'editor F9 puo' essere la foto dello schermo: anche F7)
		InputComponent->BindKey(EKeys::F7, IE_Pressed, this, &AValdorsoPlayerController::TastoStaff);
		// (10/11) K: la scheda del personaggio
		InputComponent->BindKey(EKeys::K, IE_Pressed, this, &AValdorsoPlayerController::TastoScheda);
	}

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

// (02/11, passo 6) La pausa: Esc, F10 (nell'editor Esc ferma il Play) e Start sul controller.
void AValdorsoPlayerController::TastoPausa()
{
	if (Pausa.IsValid())
	{
		if (!Pausa->InUscita())
		{
			ChiudiPausa();
		}
		return;
	}
	ApriPausa();
}

void AValdorsoPlayerController::ApriPausa()
{
	// Solo nella valle, con il corpo, e quando non c'è già un'altra schermata (registro, scelta, anticamera, codici).
	if (Pausa.IsValid() || !GetPawn() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid() || Anticamera.IsValid()
		|| SchermataCodici.IsValid() || SchermataBacheca.IsValid() || SchermataDialogo.IsValid() || ValdorsoTaccuino::Aperto(this) || ValdorsoZainoSchermo::Aperto(this) || ValdorsoTesseraSchermo::Aperta(this) || ValdorsoFoto::Aperta(this))
	{
		return;
	}
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}
	// Chi gioca senza account (in locale o con l'accesso libero dell'editor) non ha personaggi a cui tornare.
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	SAssignNew(Pausa, SValdorsoPausa)
		.bConPersonaggi(bColonoDelRegistro)
		.OnRiprendi(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ChiudiPausa();
			}
		}))
		.OnImpostazioni(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ApriImpostazioni();
			}
		}))
		.OnEsci(FValdorsoSuUscita::CreateLambda([Debole](EValdorsoUscita Dove)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ServerChiediUscita(static_cast<uint8>(Dove));
			}
		}))
		.OnAnnulla(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ServerAnnullaUscita();
			}
		}));
	Viewport->AddViewportWidgetContent(Pausa.ToSharedRef(), 60);
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Pausa->FuocoIniziale());
	SetInputMode(Modo);
	bShowMouseCursor = true;
}

void AValdorsoPlayerController::ApriImpostazioni()
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (SchermataImpostazioni.IsValid() || !Viewport)
	{
		return;
	}
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	SAssignNew(SchermataImpostazioni, SValdorsoImpostazioni)
		.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ChiudiImpostazioni();
			}
		}));
	Viewport->AddViewportWidgetContent(SchermataImpostazioni.ToSharedRef(), 61);
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(SchermataImpostazioni->FuocoIniziale());
	SetInputMode(Modo);
	bShowMouseCursor = true;
}

void AValdorsoPlayerController::ChiudiImpostazioni()
{
	if (!SchermataImpostazioni.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(SchermataImpostazioni.ToSharedRef());
	}
	SchermataImpostazioni.Reset();
	// Si torna alla pausa, con il fuoco su di lei.
	if (Pausa.IsValid())
	{
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Pausa->FuocoIniziale());
		SetInputMode(Modo);
	}
}

void AValdorsoPlayerController::ChiudiPausa()
{
	ChiudiImpostazioni();
	if (!Pausa.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(Pausa.ToSharedRef());
	}
	Pausa.Reset();
	// Se nel frattempo si è aperta un'altra schermata (per esempio la scelta), il mouse resta a lei.
	if (!SchermataRegistro.IsValid() && !SceltaPersonaggio.IsValid() && !Anticamera.IsValid() && !SchermataCodici.IsValid())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

// --- Le interazioni (v0.1.3) -------------------------------------------------------------------------

namespace
{
	/** (03/11) Vero mentre il colono si sta sedendo o alzando: in quel momento E non fa niente. */
	bool SullaPancaInMovimento(const APawn* Corpo)
	{
		const AValdorsoCharacter* Colono = Cast<AValdorsoCharacter>(Corpo);
		const UValdorsoSedutaComponent* Seduta = Colono ? Colono->GetSeduta() : nullptr;
		const UValdorsoSonnoComponent* Sonno = Colono ? Colono->GetSonno() : nullptr;
		// (05/11) Anche mentre si corica o si alza dal letto.
		return (Seduta && Seduta->Occupato() && !Seduta->Seduto()) || (Sonno && Sonno->Occupato() && !Sonno->Dorme());
	}
}

void AValdorsoPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (IsLocalController())
	{
		AggiornaSuggerimento();
		Immersione();
		Telecamera(DeltaTime);
	}
}

void AValdorsoPlayerController::Telecamera(float DeltaTime)
{
	// (10/11) la telecamera che segue morbida (un poco in ritardo sul colono) e si allarga appena di corsa;
	// nella modalita' foto la telecamera e' sua
	AValdorsoCharacter* Corpo = Cast<AValdorsoCharacter>(GetPawn());
	UCameraComponent* Occhio = Corpo ? Corpo->GetFollowCamera() : nullptr;
	USpringArmComponent* Braccio = Corpo ? Corpo->GetCameraBoom() : nullptr;
	if (!Occhio || !Braccio || ValdorsoFoto::Aperta(this))
	{
		return;
	}
	// (ogni corpo nuovo: anche dopo la rinascita al tempio)
	if (CorpoTelecamera.Get() != Corpo)
	{
		CorpoTelecamera = Corpo;
		CampoBase = Occhio->FieldOfView;
		if (!Braccio->bEnableCameraLag)
		{
			Braccio->bEnableCameraLag = true;
			Braccio->CameraLagSpeed = 12.f;
			Braccio->CameraLagMaxDistance = 120.f;
		}
	}
	const float Veloce = Corpo->GetVelocity().Size2D();
	const float Voluto = CampoBase + (Veloce > 450.f ? 5.f : 0.f);
	Occhio->SetFieldOfView(FMath::FInterpTo(Occhio->FieldOfView, Voluto, DeltaTime, 3.f));
}

void AValdorsoPlayerController::TastoStaff()
{
	// aperta si chiude da se' (F9 dentro la finestra); qui si apre solo in gioco, senza altre schermate
	if (!GetPawn() || (SchermataAperta() && !ValdorsoStaffSchermo::Aperta(this)))
	{
		return;
	}
	ValdorsoStaffSchermo::Alterna(this);
}

void AValdorsoPlayerController::TastoScheda()
{
	// aperta si chiude da se' (K dentro la scheda); qui si apre solo in gioco, senza altre schermate
	if (!GetPawn() || (SchermataAperta() && !ValdorsoSchedaSchermo::Aperta(this)))
	{
		return;
	}
	ValdorsoSchedaSchermo::Alterna(this);
}

void AValdorsoPlayerController::Immersione()
{
	// (10/11, v0.1.4) Sott'acqua: Ctrl tenuto (o il dorsale sinistro) scende, Spazio tenuto (o A) risale. Va con il
	// movimento di ogni passo, che il server riceve e controlla come il resto (il nuoto si prevede come il camminare).
	ACharacter* Corpo = Cast<ACharacter>(GetPawn());
	const UValdorsoMovementComponent* Movimento = Corpo ? Cast<UValdorsoMovementComponent>(Corpo->GetCharacterMovement()) : nullptr;
	if (!Movimento || !Movimento->IsSwimming() || SchermataAperta())
	{
		return;
	}
	float Su = 0.f;
	// a galla Spazio non spinge piu' in su (si resterebbe a saltellare sul pelo dell'acqua)
	if ((IsInputKeyDown(EKeys::SpaceBar) || IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom)) && Movimento->AcquaSopraLaTesta() > -15.f)
	{
		Su += 1.f;
	}
	if (IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl) || IsInputKeyDown(EKeys::Gamepad_LeftShoulder))
	{
		Su -= 1.f;
	}
	if (Su != 0.f)
	{
		Corpo->AddMovementInput(FVector::UpVector, Su);
	}
}

void AValdorsoPlayerController::AggiornaSuggerimento()
{
	// Dieci volte al secondo basta (e costa poco).
	const double Adesso = FPlatformTime::Seconds();
	if (Adesso < ProssimaRicerca)
	{
		return;
	}
	ProssimaRicerca = Adesso + 0.1;

	// Niente suggerimento mentre c'è una schermata aperta o il colono sta uscendo.
	const bool bSchermata = Pausa.IsValid() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid() || Anticamera.IsValid()
		|| SchermataCodici.IsValid() || SchermataBacheca.IsValid() || SchermataDialogo.IsValid() || ValdorsoTaccuino::Aperto(this) || ValdorsoZainoSchermo::Aperto(this) || ValdorsoTesseraSchermo::Aperta(this) || SchermataMappa.IsValid() || SullaPancaInMovimento(GetPawn())
		|| ValdorsoFoto::Aperta(this);
	AActor* Trovata = (!bSchermata && GetPawn()) ? ValdorsoInterazione::PiuVicina(GetPawn()) : nullptr;
	CosaVicina = Trovata;
	const IValdorsoInterattivo* Cosa = Cast<IValdorsoInterattivo>(Trovata);
	TestoSuggerimento = Cosa ? Cosa->TestoInterazione(GetPawn()) : FText::GetEmpty();

	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}
	if (!TestoSuggerimento.IsEmpty() && !Suggerimento.IsValid())
	{
		static FValdorsoCaratteri* Caratteri = new FValdorsoCaratteri();
		static FSlateBrush Fondo = FSlateRoundedBoxBrush(ValdorsoTema::Pannelli().CopyWithNewOpacity(0.85f), 4.f,
			ValdorsoTema::Oro().CopyWithNewOpacity(0.5f), 1.f);
		TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
		Suggerimento = SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(0.f, 0.f, 0.f, 120.f))
			.Visibility(EVisibility::HitTestInvisible)
			[
				SNew(SBorder)
				.BorderImage(&Fondo)
				.Padding(FMargin(18.f, 8.f))
				[
					SNew(STextBlock)
					.Font(Caratteri->Testo(20.f))
					.ColorAndOpacity(ValdorsoTema::Pergamena())
					.Text_Lambda([Debole]()
					{
						const AValdorsoPlayerController* Controllore = Debole.Get();
						return Controllore ? FText::Format(NSLOCTEXT("ValdorsoInterazione", "Tasto", "E  ·  {0}"), Controllore->TestoSuggerimento)
							: FText::GetEmpty();
					})
				]
			];
		Viewport->AddViewportWidgetContent(Suggerimento.ToSharedRef(), 40);
	}
	else if (TestoSuggerimento.IsEmpty() && Suggerimento.IsValid())
	{
		Viewport->RemoveViewportWidgetContent(Suggerimento.ToSharedRef());
		Suggerimento.Reset();
	}
}

void AValdorsoPlayerController::ClientAvviso_Implementation(const FText& Testo, float Secondi, float SecondiFermo)
{
	// Lo stordimento: niente movimento per qualche istante (il contatore di Unreal si alza una volta sola).
	if (SecondiFermo > 0.f)
	{
		if (!bFermoPerAvviso)
		{
			SetIgnoreMoveInput(true);
			bFermoPerAvviso = true;
		}
		TWeakObjectPtr<AValdorsoPlayerController> DeboleFermo(this);
		GetWorldTimerManager().SetTimer(TimerFermo, FTimerDelegate::CreateLambda([DeboleFermo]()
		{
			if (AValdorsoPlayerController* Controllore = DeboleFermo.Get(); Controllore && Controllore->bFermoPerAvviso)
			{
				Controllore->SetIgnoreMoveInput(false);
				Controllore->bFermoPerAvviso = false;
			}
		}), SecondiFermo, false);
	}

	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport || Testo.IsEmpty())
	{
		return;
	}
	TestoAvviso = Testo;
	if (!Avviso.IsValid())
	{
		static FValdorsoCaratteri* Caratteri = new FValdorsoCaratteri();
		static FSlateBrush Fondo = FSlateRoundedBoxBrush(ValdorsoTema::Pannelli().CopyWithNewOpacity(0.88f), 4.f,
			ValdorsoTema::Oro().CopyWithNewOpacity(0.6f), 1.f);
		TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
		Avviso = SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			.Padding(FMargin(0.f, 140.f, 0.f, 0.f))
			.Visibility(EVisibility::HitTestInvisible)
			[
				SNew(SBorder)
				.BorderImage(&Fondo)
				.Padding(FMargin(22.f, 10.f))
				[
					SNew(STextBlock)
					.Font(Caratteri->Testo(22.f))
					.ColorAndOpacity(ValdorsoTema::Pergamena())
					.Text_Lambda([Debole]()
					{
						const AValdorsoPlayerController* Controllore = Debole.Get();
						return Controllore ? Controllore->TestoAvviso : FText::GetEmpty();
					})
				]
			];
		Viewport->AddViewportWidgetContent(Avviso.ToSharedRef(), 45);
	}
	GetWorldTimerManager().SetTimer(TimerAvviso, this, &AValdorsoPlayerController::ChiudiAvviso, FMath::Max(0.5f, Secondi), false);
}

void AValdorsoPlayerController::ChiudiAvviso()
{
	if (Avviso.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Avviso.ToSharedRef());
		}
		Avviso.Reset();
	}
}

void AValdorsoPlayerController::ClientMostraBacheca_Implementation(const FString& Titolo, const TArray<FString>& Avvisi)
{
	// (07/11) una schermata che arriva dal server chiude prima la modalità foto (che tiene tastiera e mouse)
	ValdorsoFoto::Chiudi(this);
	ValdorsoTaccuino::Chiudi(this);
	ValdorsoZainoSchermo::Chiudi(this);
	ValdorsoTesseraSchermo::Chiudi(this);
	ValdorsoGildaSchermo::Chiudi(this);
	ValdorsoSchedaSchermo::Chiudi(this);
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport || Pausa.IsValid() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid())
	{
		return;
	}
	ChiudiBacheca();
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	SAssignNew(SchermataBacheca, SValdorsoBacheca)
		.Titolo(Titolo)
		.Avvisi(Avvisi)
		.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ChiudiBacheca();
			}
		}));
	Viewport->AddViewportWidgetContent(SchermataBacheca.ToSharedRef(), 58);
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(SchermataBacheca->FuocoIniziale());
	SetInputMode(Modo);
	bShowMouseCursor = true;
}

void AValdorsoPlayerController::ChiudiBacheca()
{
	if (!SchermataBacheca.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(SchermataBacheca.ToSharedRef());
	}
	SchermataBacheca.Reset();
	if (!Pausa.IsValid() && !SchermataRegistro.IsValid() && !SceltaPersonaggio.IsValid() && !Anticamera.IsValid() && !SchermataCodici.IsValid())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

// (07/11) La modalità foto: solo in gioco, con il colono nella valle e nessun'altra schermata aperta.
void AValdorsoPlayerController::TastoFoto()
{
	if (!GetPawn() || Pausa.IsValid() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid() || Anticamera.IsValid()
		|| SchermataCodici.IsValid() || SchermataBacheca.IsValid() || SchermataDialogo.IsValid() || ValdorsoTaccuino::Aperto(this) || ValdorsoZainoSchermo::Aperto(this) || ValdorsoTesseraSchermo::Aperta(this) || SchermataMappa.IsValid() || SchermataImpostazioni.IsValid())
	{
		return;
	}
	if (ValdorsoFoto::Aperta(this))
	{
		ValdorsoFoto::Chiudi(this);
	}
	else
	{
		ValdorsoFoto::Apri(this);
	}
}

void AValdorsoPlayerController::TastoQuaderno()
{
	if (!GetPawn() || Pausa.IsValid() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid() || Anticamera.IsValid()
		|| SchermataCodici.IsValid() || SchermataMappa.IsValid() || SchermataImpostazioni.IsValid() || ValdorsoFoto::Aperta(this))
	{
		return;
	}
	if (SchermataBacheca.IsValid() || SchermataDialogo.IsValid() || ValdorsoTaccuino::Aperto(this) || ValdorsoZainoSchermo::Aperto(this) || ValdorsoTesseraSchermo::Aperta(this))
	{
		return;  // una bacheca (o il quaderno) è già aperta: si chiude con Esc, E o R dal suo tasto Chiudi
	}
	// Tutto qui sul PC: il quaderno è uguale per tutti (per ora: più avanti le rune che il colono conosce)
	ClientMostraBacheca_Implementation(NSLOCTEXT("ValdorsoQuaderno", "Titolo", "Il quaderno delle rune").ToString(),
		ValdorsoRune::RigheQuaderno());
}

// (07/11) J: il taccuino di viaggio, solo in gioco e senza altre schermate aperte (dentro, J lo chiude da sé).
void AValdorsoPlayerController::TastoTaccuino()
{
	if (!GetPawn() || Pausa.IsValid() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid() || Anticamera.IsValid()
		|| SchermataCodici.IsValid() || SchermataBacheca.IsValid() || SchermataDialogo.IsValid() || SchermataMappa.IsValid()
		|| SchermataImpostazioni.IsValid() || ValdorsoFoto::Aperta(this) || ValdorsoZainoSchermo::Aperto(this) || ValdorsoTesseraSchermo::Aperta(this))
	{
		return;
	}
	ValdorsoTaccuino::Alterna(this);
}

// (08/11, v0.1.4) I: lo zaino, solo in gioco e senza altre schermate aperte (dentro, I lo chiude da sé).
void AValdorsoPlayerController::TastoZaino()
{
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(GetPawn());
	if (!GetPawn() || Pausa.IsValid() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid() || Anticamera.IsValid()
		|| SchermataCodici.IsValid() || SchermataBacheca.IsValid() || SchermataDialogo.IsValid() || SchermataMappa.IsValid()
		|| SchermataImpostazioni.IsValid() || ValdorsoFoto::Aperta(this) || ValdorsoTaccuino::Aperto(this) || ValdorsoTesseraSchermo::Aperta(this)
		|| (Chat && Chat->RigaAperta()) || (Pericoli && Pericoli->Caduto()))
	{
		return;
	}
	ValdorsoZainoSchermo::Alterna(this);
}

// --- (08/11, v0.1.4) La barra delle azioni e la chat --------------------------------------------------

bool AValdorsoPlayerController::SchermataAperta() const
{
	return Pausa.IsValid() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid() || Anticamera.IsValid()
		|| SchermataCodici.IsValid() || SchermataBacheca.IsValid() || SchermataDialogo.IsValid() || SchermataMappa.IsValid()
		|| SchermataImpostazioni.IsValid() || ValdorsoTaccuino::Aperto(this) || ValdorsoZainoSchermo::Aperto(this) || ValdorsoTesseraSchermo::Aperta(this) || ValdorsoFoto::Aperta(this)
		|| ValdorsoBottegaSchermo::Aperta(this) || ValdorsoGildaSchermo::Aperta(this) || ValdorsoStaffSchermo::Aperta(this) || ValdorsoSchedaSchermo::Aperta(this) || (Chat && Chat->RigaAperta());
}

void AValdorsoPlayerController::TastoAggancio()
{
	if (!Aggancio || !GetPawn() || SchermataAperta())
	{
		return;
	}
	Aggancio->Alterna();
}

void AValdorsoPlayerController::TastoRiponi()
{
	if (!GetPawn() || SchermataAperta())
	{
		return;
	}
	if (UValdorsoArmiComponent* Armi = UValdorsoArmiComponent::Di(GetPawn()))
	{
		Armi->ServerAlterna();
	}
}

void AValdorsoPlayerController::TastoAzione(int32 Indice)
{
	// solo in gioco, con il colono in piedi e niente di aperto (per ora i posti sono vuoti: si illuminano e basta)
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(GetPawn());
	if (!GetPawn() || !Barre || SchermataAperta() || (Pericoli && Pericoli->Caduto()))
	{
		return;
	}
	Barre->PremiAzione(Indice);
}

void AValdorsoPlayerController::TastoChat()
{
	// Invio apre la riga della chat (dentro dialoghi e modalità foto Invio lo prendono loro, e qui non arriva)
	if (!GetPawn() || !Chat || SchermataAperta())
	{
		return;
	}
	Chat->ApriRiga();
}

// --- (07/11) Il dialogo con i PNG ------------------------------------------------------------------

void AValdorsoPlayerController::ClientMostraDialogo_Implementation(AActor* Chi, const FValdorsoBattuta& Battuta)
{
	// Il server ha chiuso la conversazione (il colono si è allontanato, o ha scelto di andarsene).
	if (Battuta.bChiudi)
	{
		ChiudiDialogo(false);
		return;
	}
	ValdorsoFoto::Chiudi(this);
	ValdorsoTaccuino::Chiudi(this);
	ValdorsoZainoSchermo::Chiudi(this);
	ValdorsoTesseraSchermo::Chiudi(this);
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport || Pausa.IsValid() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid() || Anticamera.IsValid()
		|| SchermataCodici.IsValid() || SchermataBacheca.IsValid() || SchermataMappa.IsValid())
	{
		// Adesso non si può mostrare: il server chiude la conversazione.
		ServerScegliDialogo(Chi, Battuta.Passo, INDEX_NONE);
		return;
	}
	DialogoCon = Chi;
	DialogoPasso = Battuta.Passo;
	if (SchermataDialogo.IsValid())
	{
		SchermataDialogo->Mostra(Battuta);
	}
	else
	{
		TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
		SAssignNew(SchermataDialogo, SValdorsoDialogo)
			.Battuta(Battuta)
			.OnScelta(FValdorsoSuSceltaDialogo::CreateLambda([Debole](int32 Scelta)
			{
				AValdorsoPlayerController* Controllore = Debole.Get();
				if (!Controllore)
				{
					return;
				}
				if (Scelta == INDEX_NONE)
				{
					Controllore->ChiudiDialogo(true);
				}
				else
				{
					Controllore->ServerScegliDialogo(Controllore->DialogoCon.Get(), Controllore->DialogoPasso, Scelta);
				}
			}));
		Viewport->AddViewportWidgetContent(SchermataDialogo.ToSharedRef(), 58);
	}
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(SchermataDialogo->FuocoIniziale());
	SetInputMode(Modo);
	bShowMouseCursor = true;
}

void AValdorsoPlayerController::ServerScegliDialogo_Implementation(AActor* Chi, int32 Passo, int32 Scelta)
{
	// Il PNG controlla tutto (conversazione aperta, numero della battuta, distanza).
	if (AValdorsoPNG* Png = Cast<AValdorsoPNG>(Chi))
	{
		Png->Scegli(this, Passo, Scelta);
	}
}

void AValdorsoPlayerController::ChiudiDialogo(bool bAvvisaServer)
{
	if (bAvvisaServer && DialogoCon.IsValid())
	{
		ServerScegliDialogo(DialogoCon.Get(), DialogoPasso, INDEX_NONE);
	}
	DialogoCon.Reset();
	if (!SchermataDialogo.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(SchermataDialogo.ToSharedRef());
	}
	SchermataDialogo.Reset();
	if (!Pausa.IsValid() && !SchermataRegistro.IsValid() && !SceltaPersonaggio.IsValid() && !Anticamera.IsValid() && !SchermataCodici.IsValid()
		&& !SchermataBacheca.IsValid() && !SchermataMappa.IsValid())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void AValdorsoPlayerController::TastoMappa()
{
	// La mappa si apre solo in gioco, con il colono nella valle e nessun'altra schermata aperta.
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport || !GetPawn() || Pausa.IsValid() || SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid()
		|| Anticamera.IsValid() || SchermataCodici.IsValid() || SchermataBacheca.IsValid() || SchermataDialogo.IsValid() || ValdorsoTaccuino::Aperto(this) || ValdorsoZainoSchermo::Aperto(this) || ValdorsoTesseraSchermo::Aperta(this) || ValdorsoFoto::Aperta(this))
	{
		return;
	}
	if (SchermataMappa.IsValid())
	{
		ChiudiMappa();
		return;
	}
	// (08/11) la mappa si apre solo a chi ce l'ha nello zaino (ce l'hanno tutti dall'arrivo; chi la getta, la perde)
	if (const UValdorsoZainoComponent* Il = UValdorsoZainoComponent::Di(this);
		Il && Il->Contenuto().bPartenzaData && ValdorsoZaino::Conta(Il->Contenuto(), TEXT("Mappa_Valle")) == 0)
	{
		ClientAvviso_Implementation(NSLOCTEXT("ValdorsoMappa", "SenzaMappa", "Non hai una mappa della valle."), 2.5f, 0.f);
		return;
	}
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	SAssignNew(SchermataMappa, SValdorsoMappa)
		.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ChiudiMappa();
			}
		}));
	Viewport->AddViewportWidgetContent(SchermataMappa.ToSharedRef(), 57);
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(SchermataMappa);
	SetInputMode(Modo);
	bShowMouseCursor = true;
}

void AValdorsoPlayerController::ChiudiMappa()
{
	if (!SchermataMappa.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(SchermataMappa.ToSharedRef());
	}
	SchermataMappa.Reset();
	if (!Pausa.IsValid() && !SchermataRegistro.IsValid() && !SceltaPersonaggio.IsValid() && !Anticamera.IsValid() && !SchermataCodici.IsValid()
		&& !SchermataBacheca.IsValid())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void AValdorsoPlayerController::TastoInteragisci()
{
	if (Pausa.IsValid() || SchermataBacheca.IsValid() || SchermataDialogo.IsValid() || ValdorsoTaccuino::Aperto(this) || ValdorsoZainoSchermo::Aperto(this) || ValdorsoTesseraSchermo::Aperta(this) || ValdorsoBottegaSchermo::Aperta(this) || ValdorsoGildaSchermo::Aperta(this) || ValdorsoStaffSchermo::Aperta(this) || ValdorsoSchedaSchermo::Aperta(this) || !GetPawn())
	{
		return;
	}
	if (AActor* Cosa = CosaVicina.Get())
	{
		ServerInteragisci(Cosa);
	}
}

void AValdorsoPlayerController::ServerInteragisci_Implementation(AActor* Cosa)
{
	APawn* Corpo = GetPawn();
	IValdorsoInterattivo* Interattivo = Cast<IValdorsoInterattivo>(Cosa);
	if (!Corpo || !Interattivo || bInUscita || SullaPancaInMovimento(Corpo) || !ValdorsoInterazione::PuoUsare(Corpo, Cosa))
	{
		return;
	}
	// Un limite alle richieste: al massimo 10 al secondo (chi ne manda di più sta barando).
	const double Adesso = FPlatformTime::Seconds();
	if (Adesso - InizioConteggioInterazioni > 1.0)
	{
		InizioConteggioInterazioni = Adesso;
		RichiesteInterazione = 0;
	}
	if (++RichiesteInterazione > 10)
	{
		return;
	}
	// Il server non si fida: il colono deve essere davvero vicino.
	if (FVector::Dist(Corpo->GetActorLocation(), Interattivo->PuntoInterazionePer(Corpo)) > ValdorsoInterazione::RaggioServer
		|| !ValdorsoInterazione::SiVede(Corpo, Cosa))
	{
		return;
	}
	Interattivo->Interagisci(Corpo);
}

// --- Server: l'uscita con attesa -------------------------------------------------------------------

float AValdorsoPlayerController::SaluteAttuale() const
{
	const IAbilitySystemInterface* ConAbilita = Cast<IAbilitySystemInterface>(GetPawn());
	const UAbilitySystemComponent* Abilita = ConAbilita ? ConAbilita->GetAbilitySystemComponent() : nullptr;
	return Abilita ? Abilita->GetNumericAttribute(UValdorsoAttributeSet::GetSaluteAttribute()) : 0.f;
}

void AValdorsoPlayerController::ServerChiediUscita_Implementation(uint8 Dove)
{
	if (Dove > static_cast<uint8>(EValdorsoUscita::Gioco) || bInUscita || !bAutenticato || !GetPawn())
	{
		return;
	}
	if (++RichiesteUscita > 60)
	{
		Espelli(TEXT("Troppe richieste: ricollegati."));
		return;
	}
	// Ai personaggi si torna solo con un account (chi gioca senza account torna al menu).
	if (Dove == static_cast<uint8>(EValdorsoUscita::Personaggi) && !bPersonaggioScelto)
	{
		Dove = static_cast<uint8>(EValdorsoUscita::Menu);
	}
	bInUscita = true;
	UscitaDove = Dove;
	UscitaFine = GetWorld()->GetTimeSeconds() + SecondiUscita;
	UscitaPartenza = GetPawn()->GetActorLocation();
	UscitaSalute = SaluteAttuale();
	GetWorldTimerManager().SetTimer(TimerUscita, this, &AValdorsoPlayerController::ControllaUscita, 0.25f, true);
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] %s chiede di uscire (%d): %.0f secondi"), *NomeAccount, Dove, SecondiUscita);
	ClientContoUscita(Dove, FMath::CeilToInt(SecondiUscita));
}

void AValdorsoPlayerController::ServerAnnullaUscita_Implementation()
{
	if (bInUscita)
	{
		AnnullaUscita(FString());
	}
}

void AValdorsoPlayerController::ControllaUscita()
{
	APawn* Corpo = GetPawn();
	if (!bInUscita || !Corpo)
	{
		GetWorldTimerManager().ClearTimer(TimerUscita);
		bInUscita = false;
		return;
	}
	// Colpito: la salute è scesa (mezzo punto di tolleranza per gli arrotondamenti).
	if (SaluteAttuale() < UscitaSalute - 0.5f)
	{
		AnnullaUscita(TEXT("Sei stato colpito: resti nella valle."));
		return;
	}
	// Spostato (spinto, trascinato, caduto): la distanza in piano dal punto di partenza.
	if (FVector::Dist2D(Corpo->GetActorLocation(), UscitaPartenza) > SpostamentoMassimoUscita)
	{
		AnnullaUscita(TEXT("Ti sei spostato: resti nella valle."));
		return;
	}
	if (GetWorld()->GetTimeSeconds() >= UscitaFine)
	{
		CompletaUscita();
	}
}

void AValdorsoPlayerController::AnnullaUscita(const FString& Motivo)
{
	bInUscita = false;
	GetWorldTimerManager().ClearTimer(TimerUscita);
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Uscita annullata per %s: %s"), *NomeAccount, Motivo.IsEmpty() ? TEXT("resta") : *Motivo);
	ClientUscitaAnnullata(Motivo);
}

void AValdorsoPlayerController::CompletaUscita()
{
	bInUscita = false;
	GetWorldTimerManager().ClearTimer(TimerUscita);
	// Prima di tutto si salva dov'era e com'era.
	SalvaPersonaggio();
	const uint8 Dove = UscitaDove;
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] %s lascia la valle (%d)"), *NomeAccount, Dove);
	if (APawn* Corpo = GetPawn())
	{
		ValdorsoRegistroEventi::Annota(TEXT("Esce"), Corpo, nullptr, Corpo->GetActorLocation(),
			Dove == static_cast<uint8>(EValdorsoUscita::Personaggi) ? TEXT("ai personaggi") : TEXT("dal gioco"));
	}

	if (Dove == static_cast<uint8>(EValdorsoUscita::Personaggi) && bPersonaggioScelto)
	{
		// Il corpo lascia la valle e si torna alla scelta, restando collegati.
		GetWorldTimerManager().ClearTimer(TimerSalvataggio);
		bPersonaggioScelto = false;
		PersonaggioId.Empty();
		if (APawn* Corpo = GetPawn())
		{
			UnPossess();
			Corpo->Destroy();
		}
		ClientUscitaFatta(Dove);
		InviaSceltaPersonaggio();
		return;
	}
	// Al menu o fuori dal gioco: il client si scollega da solo (il corpo sparisce quando la connessione si chiude).
	ClientUscitaFatta(Dove);
}

// --- Client: le risposte del server ----------------------------------------------------------------

void AValdorsoPlayerController::ClientContoUscita_Implementation(uint8 Dove, int32 Secondi)
{
	if (Pausa.IsValid())
	{
		Pausa->MostraConto(static_cast<EValdorsoUscita>(FMath::Min<uint8>(Dove, 2)), Secondi);
	}
}

void AValdorsoPlayerController::ClientUscitaAnnullata_Implementation(const FString& Motivo)
{
	if (Motivo.IsEmpty())
	{
		// L'ha annullata il giocatore ("Resta nella valle" o Esc): si torna al gioco.
		ChiudiPausa();
		return;
	}
	if (!Pausa.IsValid())
	{
		ApriPausa();
	}
	if (Pausa.IsValid())
	{
		Pausa->Annullata(FText::FromString(Motivo));
	}
}

void AValdorsoPlayerController::ClientUscitaFatta_Implementation(uint8 Dove)
{
	ChiudiPausa();
	bColonoDelRegistro = false;
	if (Dove == static_cast<uint8>(EValdorsoUscita::Personaggi))
	{
		// Il server manda subito l'elenco dei personaggi (ClientSceltaPersonaggio).
		return;
	}
	// Uscita voluta: niente rientro automatico.
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->DimenticaBiglietto();
	}
	if (Dove == static_cast<uint8>(EValdorsoUscita::Gioco))
	{
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		return;
	}
	ConsoleCommand(TEXT("disconnect"));
}

bool AValdorsoPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

// ------------------------------------------------------------------------------------------------
// Valdorso: l'anticamera
// ------------------------------------------------------------------------------------------------

bool AValdorsoPlayerController::ConnessioneCifrata() const
{
	if (GetNetMode() == NM_Client)
	{
		const UNetDriver* Driver = GetWorld() ? GetWorld()->GetNetDriver() : nullptr;
		return Driver && Driver->ServerConnection && Driver->ServerConnection->IsEncryptionEnabled();
	}
	const UNetConnection* Connessione = GetNetConnection();
	return Connessione && Connessione->IsEncryptionEnabled();
}

FString AValdorsoPlayerController::Indirizzo() const
{
	UNetConnection* Connessione = GetNetConnection();
	return Connessione ? Connessione->LowLevelGetRemoteAddress(false) : FString(TEXT("locale"));
}

void AValdorsoPlayerController::Accogli()
{
	// (09/11, v0.1.4 passo 4.2) "Esplora da solo": l'archivio del gioco da solo su questo PC, il suo account locale e
	// la scelta del personaggio come con il server (Registro, zaino, taccuino: tutto si salva qui, in Saved/Solo).
	const UValdorsoGameInstance* IstanzaSolo = GetGameInstance<UValdorsoGameInstance>();
	UValdorsoArchivista* ArchivistaSolo = UValdorsoArchivista::Di(this);
	if (IsLocalController() && IstanzaSolo && IstanzaSolo->bModoSolo && GetNetMode() == NM_Standalone && ArchivistaSolo)
	{
		FString Nome;
		AccountId = ArchivistaSolo->ApriArchivioSolo(Nome);
		ArchivistaSolo->SegnaCollegato(AccountId);
		bAutenticato = true;
		bSenzaPersonaggio = false;
		NomeAccount = Nome;
		Ruolo = EValdorsoRuolo::Giocatore;
		if (PlayerState)
		{
			PlayerState->SetPlayerName(Nome);
		}
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Gioco da solo: account locale %s"), *Nome);
		// al giro dopo (la schermata ha bisogno del mondo pronto): la scelta del personaggio
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			InviaSceltaPersonaggio();
		}));
		return;
	}

	// Chi gioca sul PC del server (gioco autonomo o server in ascolto) non passa dall'anticamera.
	if (IsLocalController())
	{
		bAutenticato = true;
		bSenzaPersonaggio = true;
		NomeAccount = TEXT("Locale");
		return;
	}

	bAutenticato = false;
	const UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>();

	if (!ConnessioneCifrata())
	{
		// Nell'editor (Gioca come client) si entra liberi, per provare il gioco senza account.
		if (GetWorld() && GetWorld()->IsPlayInEditor() && Istanza && Istanza->bAccessoLiberoNellEditor)
		{
			static int32 Contatore = 0;
			bAutenticato = true;
			bSenzaPersonaggio = true;
			NomeAccount = FString::Printf(TEXT("Prova%d"), ++Contatore);
			AccountId = TEXT("prova-") + NomeAccount;
			Ruolo = EValdorsoRuolo::Amministratore;
			if (PlayerState)
			{
				PlayerState->SetPlayerName(NomeAccount);
			}
			UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Anticamera: accesso libero dell'editor come %s"), *NomeAccount);
			return;
		}

		UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Anticamera: connessione non cifrata da %s, scollego."), *Indirizzo());
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			Espelli(TEXT("Connessione non cifrata: aggiorna il gioco."));
		}));
		return;
	}

	GetWorldTimerManager().SetTimer(TimerAnticamera, this, &AValdorsoPlayerController::TempoScaduto, SecondiAnticamera, false);
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Anticamera: arrivato %s (connessione cifrata)"), *Indirizzo());
}

bool AValdorsoPlayerController::CanRestartPlayer()
{
	return bAutenticato && !bAttendeCodici && (bPersonaggioScelto || bSenzaPersonaggio) && Super::CanRestartPlayer();
}

void AValdorsoPlayerController::ServerCodiciScritti_Implementation()
{
	if (!bAttendeCodici || !bAutenticato)
	{
		return;
	}
	bAttendeCodici = false;
	if (UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this))
	{
		Archivista->ConfermaCodiciRecupero(AccountId);
	}
	InviaSceltaPersonaggio();
}

bool AValdorsoPlayerController::PuoChiedere(const FString& Nome, const FString& Segreto)
{
	if (bAutenticato || bRichiestaInCorso)
	{
		return false;
	}
	if (!ConnessioneCifrata())
	{
		Espelli(TEXT("Connessione non cifrata."));
		return false;
	}
	if (++RichiesteFatte > RichiesteMassime)
	{
		Espelli(TEXT("Troppe richieste: ricollegati."));
		return false;
	}
	if (Nome.Len() > 64 || Segreto.Len() > 256)
	{
		Espelli(TEXT("Richiesta non valida."));
		return false;
	}
	if (!UValdorsoArchivista::Di(this))
	{
		Espelli(TEXT("Il server non ha l'archivio degli account."));
		return false;
	}
	return true;
}

void AValdorsoPlayerController::ServerAccedi_Implementation(const FString& Nome, const FString& Password)
{
	if (!PuoChiedere(Nome, Password))
	{
		return;
	}
	bRichiestaInCorso = true;
	NomeAccount = Nome;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->Accedi(Nome, Password, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
	{
		if (AValdorsoPlayerController* Controllore = Debole.Get())
		{
			Controllore->RispostaArchivista(Esito);
		}
	});
}

void AValdorsoPlayerController::ServerPrimoIngresso_Implementation(const FString& CodiceInvito, const FString& Nome, const FString& Password)
{
	if (CodiceInvito.Len() > 64 || !PuoChiedere(Nome, Password))
	{
		return;
	}
	bRichiestaInCorso = true;
	NomeAccount = Nome;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->CreaAccount(CodiceInvito, Nome, Password, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
	{
		if (AValdorsoPlayerController* Controllore = Debole.Get())
		{
			Controllore->RispostaArchivista(Esito);
		}
	});
}

void AValdorsoPlayerController::ServerRecupera_Implementation(const FString& Nome, const FString& Codice, const FString& Nuova)
{
	if (Codice.Len() > 64)
	{
		Espelli(TEXT("Richiesta non valida."));
		return;
	}
	if (!PuoChiedere(Nome, Nuova))
	{
		return;
	}
	bRichiestaInCorso = true;
	NomeAccount = Nome;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->RecuperaConCodice(Nome, Codice, Nuova, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
	{
		if (AValdorsoPlayerController* Controllore = Debole.Get())
		{
			Controllore->RispostaArchivista(Esito);
		}
	});
}

void AValdorsoPlayerController::ServerRientra_Implementation(const FString& Biglietto)
{
	if (Biglietto.Len() > 128 || !PuoChiedere(FString(), Biglietto))
	{
		return;
	}
	bRichiestaInCorso = true;
	bRientro = true;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->RientraConBiglietto(Biglietto, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
	{
		if (AValdorsoPlayerController* Controllore = Debole.Get())
		{
			Controllore->RispostaArchivista(Esito);
		}
	});
}

void AValdorsoPlayerController::ServerCambiaPassword_Implementation(const FString& Attuale, const FString& Nuova)
{
	// Si cambia qui solo la password temporanea, prima di entrare (il cambio normale arriverà nelle Impostazioni).
	if (!bDeveCambiarePassword || Nuova.Len() > 256 || !PuoChiedere(NomeAccount, Attuale))
	{
		return;
	}
	bRichiestaInCorso = true;
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	UValdorsoArchivista::Di(this)->CambiaPassword(NomeAccount, Attuale, Nuova, Indirizzo(), [Debole](const FValdorsoEsitoAccount& Esito)
	{
		if (AValdorsoPlayerController* Controllore = Debole.Get())
		{
			Controllore->RispostaArchivista(Esito);
		}
	});
}

void AValdorsoPlayerController::RispostaArchivista(const FValdorsoEsitoAccount& Esito)
{
	bRichiestaInCorso = false;
	if (!Esito.Riuscito())
	{
		bRientro = false;
	}
	if (!IsValid(this) || IsActorBeingDestroyed())
	{
		return;
	}

	switch (Esito.Esito)
	{
	case EValdorsoEsitoAccount::Ok:
		FaiEntrare(Esito);
		break;

	case EValdorsoEsitoAccount::OkDeveCambiarePassword:
		// Resta nell'anticamera finché non sceglie una password nuova.
		bDeveCambiarePassword = true;
		NomeAccount = Esito.Nome;
		ClientEsitoAccesso(Esito);
		break;

	case EValdorsoEsitoAccount::PasswordDebole:
	case EValdorsoEsitoAccount::CredenzialiSbagliate:
		if (bDeveCambiarePassword)
		{
			// Durante il cambio della password temporanea si può riprovare (entro i tentativi).
			ClientEsitoAccesso(Esito);
			break;
		}
		Espelli(Esito.Messaggio);
		break;

	default:
		Espelli(Esito.Messaggio);
		break;
	}
}

void AValdorsoPlayerController::FaiEntrare(const FValdorsoEsitoAccount& Esito)
{
	UWorld* Mondo = GetWorld();
	UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);

	// Un account, un collegamento: se era già dentro da un altro posto, quel collegamento si chiude.
	if (Archivista && !Archivista->SegnaCollegato(Esito.AccountId))
	{
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			AValdorsoPlayerController* Altro = Cast<AValdorsoPlayerController>(It->Get());
			if (Altro && Altro != this && Altro->AccountId == Esito.AccountId)
			{
				// Prima si salva dov'era il personaggio del collegamento vecchio (al rientro è il caso normale).
				Altro->SalvaPersonaggio();
				Altro->bPersonaggioScelto = false;
				Altro->bAutenticato = false;
				Altro->AccountId.Empty();
				Altro->Espelli(TEXT("Il tuo account è entrato da un altro posto."));
			}
		}
		Archivista->SegnaScollegato(Esito.AccountId);
		Archivista->SegnaCollegato(Esito.AccountId);
	}

	bAutenticato = true;
	bDeveCambiarePassword = false;
	// Codici di recupero nuovi: il personaggio nasce dopo che il giocatore li ha scritti.
	bAttendeCodici = Esito.CodiciRecupero.Num() > 0;
	AccountId = Esito.AccountId;
	NomeAccount = Esito.Nome;
	Ruolo = Esito.Ruolo;
	GetWorldTimerManager().ClearTimer(TimerAnticamera);
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Anticamera: %s è entrato nella valle"), *Esito.Nome);

	ClientEsitoAccesso(Esito);

	// Il biglietto per rientrare senza password se il collegamento si interrompe.
	if (Archivista)
	{
		const FString Biglietto = Archivista->CreaBiglietto(AccountId);
		if (!Biglietto.IsEmpty())
		{
			ClientBiglietto(Biglietto, NomeAccount);
		}
	}

	// Poi la scelta del personaggio (dopo i codici di recupero, se ce ne sono di nuovi da scrivere).
	if (!bAttendeCodici)
	{
		InviaSceltaPersonaggio();
	}
}

// ------------------------------------------------------------------------------------------------
// La scelta del personaggio (v0.1.2, passo 3)
// ------------------------------------------------------------------------------------------------

void AValdorsoPlayerController::InviaSceltaPersonaggio(const FString& Messaggio, bool bErrore)
{
	UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	if (!Archivista || !bAutenticato || bPersonaggioScelto)
	{
		return;
	}
	// Al rientro senza password si torna subito con l'ultimo personaggio.
	if (bRientro)
	{
		bRientro = false;
		const FString Ultimo = Archivista->UltimoPersonaggio(AccountId);
		if (!Ultimo.IsEmpty())
		{
			FaiNascere(Ultimo);
			return;
		}
	}
	ClientSceltaPersonaggio(Archivista->ElencoPersonaggi(AccountId), Messaggio, bErrore);
}

bool AValdorsoPlayerController::PuoChiederePersonaggi()
{
	if (!bAutenticato || bPersonaggioScelto || bAttendeCodici || !UValdorsoArchivista::Di(this))
	{
		return false;
	}
	if (++RichiestePersonaggi > 40)
	{
		Espelli(TEXT("Troppe richieste: ricollegati."));
		return false;
	}
	return true;
}

void AValdorsoPlayerController::ServerScegliPersonaggio_Implementation(const FString& Id)
{
	if (Id.Len() > 64 || !PuoChiederePersonaggi())
	{
		return;
	}
	FaiNascere(Id);
}

void AValdorsoPlayerController::ServerCreaPersonaggio_Implementation(const FString& Nome)
{
	if (Nome.Len() > 64 || !PuoChiederePersonaggi())
	{
		return;
	}
	FString NuovoId;
	const FString Problema = UValdorsoArchivista::Di(this)->CreaPersonaggio(AccountId, Nome, NuovoId);
	if (!Problema.IsEmpty())
	{
		InviaSceltaPersonaggio(Problema, true);
		return;
	}
	InviaSceltaPersonaggio(FString::Printf(TEXT("%s è scritto nel registro."), *ValdorsoRegole::NormalizzaNomePersonaggio(Nome)), false);

	// Passo 4.2a: subito dopo il nome, il sacerdote apre il Registro di Val d'Orso.
	if (const FValdorsoPersonaggio* Nuovo = UValdorsoArchivista::Di(this)->TrovaPersonaggio(AccountId, NuovoId))
	{
		ClientRegistro(Nuovo->Id, Nuovo->Nome, Nuovo->Registro);
	}
}

void AValdorsoPlayerController::ServerTornaAllaScelta_Implementation()
{
	if (!PuoChiederePersonaggi())
	{
		return;
	}
	InviaSceltaPersonaggio();
}

void AValdorsoPlayerController::ServerCancellaPersonaggio_Implementation(const FString& Id, const FString& Conferma)
{
	if (Id.Len() > 64 || Conferma.Len() > 64 || !PuoChiederePersonaggi())
	{
		return;
	}
	const FString Problema = UValdorsoArchivista::Di(this)->CancellaPersonaggio(AccountId, Id, Conferma, NomeAccount);
	InviaSceltaPersonaggio(Problema.IsEmpty() ? FString(TEXT("Il personaggio è stato cancellato.")) : Problema, !Problema.IsEmpty());
}

void AValdorsoPlayerController::ServerChiediRegistro_Implementation(const FString& IdRegistro)
{
	if (IdRegistro.Len() > 64 || !PuoChiederePersonaggi())
	{
		return;
	}
	const FValdorsoPersonaggio* Dati = UValdorsoArchivista::Di(this)->TrovaPersonaggio(AccountId, IdRegistro);
	if (!Dati)
	{
		InviaSceltaPersonaggio(TEXT("Questo personaggio non c'è più."), true);
		return;
	}
	ClientRegistro(Dati->Id, Dati->Nome, Dati->Registro);
}

void AValdorsoPlayerController::ServerSalvaRegistro_Implementation(const FString& IdRegistro, const FValdorsoRegistro& Proposto, bool bFirma)
{
	if (IdRegistro.Len() > 64 || !bAutenticato || bPersonaggioScelto || bAttendeCodici || !UValdorsoArchivista::Di(this))
	{
		return;
	}
	if (++RichiesteRegistro > 300)
	{
		Espelli(TEXT("Troppe richieste: ricollegati."));
		return;
	}
	FValdorsoRegistro Salvato;
	const FString Errore = UValdorsoArchivista::Di(this)->SalvaRegistro(AccountId, IdRegistro, Proposto, bFirma, Salvato);
	ClientEsitoRegistro(IdRegistro, Errore, Errore.IsEmpty(), Salvato);
}

void AValdorsoPlayerController::ClientRegistro_Implementation(const FString& IdRegistro, const FString& NomePersonaggio, const FValdorsoRegistro& Registro)
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Registro di %s: %s"), *NomePersonaggio, Registro.bFirmato ? TEXT("firmato") : TEXT("da compilare"));
	MostraRegistro(IdRegistro, NomePersonaggio, Registro);
}

void AValdorsoPlayerController::ClientEsitoRegistro_Implementation(const FString& IdRegistro, const FString& Messaggio, bool bRiuscito, const FValdorsoRegistro& Salvato)
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Registro: %s"), bRiuscito ? (Salvato.bFirmato ? TEXT("firmato") : TEXT("bozza salvata")) : *Messaggio);
	if (SchermataRegistro.IsValid() && IdRegistro == RegistroAperto)
	{
		SchermataRegistro->Esito(Messaggio, bRiuscito, Salvato);
		if (Salvato.bFirmato && !Salvato.bStoriaDaCorreggere && Palco.IsValid())
		{
			Palco->Firmato();
		}
	}
}

void AValdorsoPlayerController::FaiNascere(const FString& Id)
{
	UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	FValdorsoPersonaggio* Dati = Archivista ? Archivista->TrovaPersonaggio(AccountId, Id) : nullptr;
	if (!Dati)
	{
		InviaSceltaPersonaggio(TEXT("Questo personaggio non c'è più."), true);
		return;
	}

	// Passo 4.2a: si entra nella valle solo con il registro firmato (i personaggi di prima lo aprono ora).
	// (06/10, passo 4.5) E se lo staff ha chiesto di correggere la storia, prima la si corregge.
	if (!Dati->Registro.bFirmato || Dati->Registro.bStoriaDaCorreggere)
	{
		ClientRegistro(Dati->Id, Dati->Nome, Dati->Registro);
		return;
	}

	PersonaggioId = Dati->Id;
	bPersonaggioScelto = true;
	// (07/11) il taccuino di viaggio del personaggio (luoghi scoperti, iscrizioni lette)
	if (Taccuino)
	{
		Taccuino->Carica(Dati->Taccuino);
	}
	// (08/11) lo zaino del personaggio (la prima volta: quello che ha portato con sé, dal suo Registro)
	if (Zaino)
	{
		Zaino->Carica(Dati->Zaino, &Dati->Registro);
		Dati->Zaino = Zaino->Dati();
		// (09/11) la tessera del colono: rilasciata al primo ingresso, poi sempre quella (ValdorsoTessera.h)
		ValdorsoTessera::Sistema(Dati->Tessera, Dati->Id);
		Zaino->ImpostaTessera(ValdorsoTessera::Vista(Dati->Nome, &Dati->Registro, Dati->Tessera));
	}
	// (10/11) la Gilda (dopo lo zaino: i fogli degli incarichi stanno nello zaino)
	if (Gilda)
	{
		Gilda->Carica(Dati->Gilda);
		Dati->Zaino = Zaino ? Zaino->Dati() : Dati->Zaino;
	}
	// (10/11, scheletro della v0.1.5) le abilita' (la prima volta con i vantaggi del Registro)
	if (Crescita)
	{
		Crescita->Carica(Dati->Abilita, &Dati->Registro);
		Dati->Abilita = Crescita->Dati();
	}
	UltimoSalvataggio = FPlatformTime::Seconds();
	Dati->UltimoGioco = FDateTime::UtcNow().ToUnixTimestamp();
	Archivista->SalvaPersonaggio(*Dati);
	if (PlayerState)
	{
		// Gli altri vedono il nome del personaggio, mai quello dell'account.
		PlayerState->SetPlayerName(Dati->Nome);
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] %s entra come %s"), *NomeAccount, *Dati->Nome);

	// Dove era rimasto (solo se è la stessa mappa e il punto è sensato), altrimenti al punto di partenza.
	UWorld* Mondo = GetWorld();
	AGameModeBase* Modalita = Mondo ? Mondo->GetAuthGameMode() : nullptr;
	const FString MappaQui = UGameplayStatics::GetCurrentLevelName(this, true);
	const bool bDovEra = Dati->bHaPosizione && Dati->Mappa == MappaQui && Dati->Posizione.Z > -50000.f && !Dati->Posizione.ContainsNaN();
	if (Modalita && Modalita->PlayerCanRestart(this))
	{
		if (bDovEra)
		{
			Modalita->RestartPlayerAtTransform(this, FTransform(FRotator(0.f, Dati->Direzione, 0.f), Dati->Posizione));
		}
		if (!GetPawn())
		{
			Modalita->RestartPlayer(this);
		}
	}
	if (!GetPawn())
	{
		// Non è nato: si torna alla scelta invece di restare senza corpo.
		bPersonaggioScelto = false;
		PersonaggioId.Empty();
		InviaSceltaPersonaggio(TEXT("Il personaggio non è riuscito a entrare nella valle: riprova."), true);
		return;
	}

	// (28/10) L'aspetto dal Registro: arriva a tutti e si vede sul volto MetaHuman.
	if (AValdorsoCharacter* Colono = Cast<AValdorsoCharacter>(GetPawn()))
	{
		if (UValdorsoAspettoComponent* AspettoColono = Colono->GetAspetto())
		{
			// Finché il giocatore non sceglie l'aspetto (pagina Aspetto, passo 4.4), ogni colono ha comunque un volto
			// suo: quello "dei dadi", sempre uguale per lo stesso personaggio (non si salva).
			AspettoColono->ImpostaAspetto(ValdorsoAspetto::Visibile(Dati->Registro.Aspetto, Dati->Registro.Sesso, Dati->Id),
				Dati->Registro.Sesso);
		}
	}

	// Le statistiche com'erano (le prime volte restano quelle di partenza).
	IAbilitySystemInterface* ConAbilita = Cast<IAbilitySystemInterface>(GetPawn());
	UAbilitySystemComponent* Abilita = ConAbilita ? ConAbilita->GetAbilitySystemComponent() : nullptr;
	if (Abilita)
	{
		if (Dati->Salute > 0.f)
		{
			Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute(), Dati->Salute);
		}
		if (Dati->Stamina >= 0.f)
		{
			Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute(), Dati->Stamina);
		}
		if (Dati->Mana >= 0.f)
		{
			Abilita->SetNumericAttributeBase(UValdorsoAttributeSet::GetManaAttribute(), Dati->Mana);
		}
	}

	ClientPersonaggioScelto(Dati->Nome);
	// (03/11, v0.1.3) Nel registro eventi: chi entra nella valle, dove.
	ValdorsoRegistroEventi::Annota(TEXT("Entra"), GetPawn(), nullptr, GetPawn()->GetActorLocation());
	GetWorldTimerManager().SetTimer(TimerSalvataggio, this, &AValdorsoPlayerController::SalvaPersonaggio, 60.f, true);
}

void AValdorsoPlayerController::SalvaPersonaggio()
{
	if (!HasAuthority() || !bPersonaggioScelto)
	{
		return;
	}
	UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	FValdorsoPersonaggio* Dati = Archivista ? Archivista->TrovaPersonaggio(AccountId, PersonaggioId) : nullptr;
	if (!Dati)
	{
		return;
	}
	const double Ora = FPlatformTime::Seconds();
	Dati->TempoDiGioco += static_cast<int64>(FMath::Max(0.0, Ora - UltimoSalvataggio));
	UltimoSalvataggio = Ora;
	Dati->UltimoGioco = FDateTime::UtcNow().ToUnixTimestamp();

	if (APawn* Corpo = GetPawn())
	{
		Dati->Mappa = UGameplayStatics::GetCurrentLevelName(this, true);
		Dati->Posizione = Corpo->GetActorLocation();
		Dati->Direzione = Corpo->GetActorRotation().Yaw;
		Dati->bHaPosizione = true;
		IAbilitySystemInterface* ConAbilita = Cast<IAbilitySystemInterface>(Corpo);
		if (UAbilitySystemComponent* Abilita = ConAbilita ? ConAbilita->GetAbilitySystemComponent() : nullptr)
		{
			Dati->Salute = Abilita->GetNumericAttributeBase(UValdorsoAttributeSet::GetSaluteAttribute());
			Dati->Stamina = Abilita->GetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute());
			Dati->Mana = Abilita->GetNumericAttributeBase(UValdorsoAttributeSet::GetManaAttribute());
		}
	}
	// (07/11) il taccuino di viaggio va nel file con il resto
	if (Taccuino)
	{
		Dati->Taccuino = Taccuino->Dati();
	}
	// (08/11) e lo zaino
	if (Zaino)
	{
		Dati->Zaino = Zaino->Dati();
	}
	// (10/11) e la Gilda
	if (Gilda)
	{
		Dati->Gilda = Gilda->Dati();
	}
	// (10/11) e le abilita'
	if (Crescita)
	{
		Dati->Abilita = Crescita->Dati();
	}
	Archivista->SalvaPersonaggio(*Dati);
}

void AValdorsoPlayerController::PawnLeavingGame()
{
	// (10/11) i fogli della Gilda solo prenotati tornano sulla bacheca
	if (Gilda)
	{
		Gilda->Esce();
	}
	SalvaPersonaggio();
	Super::PawnLeavingGame();
}

void AValdorsoPlayerController::TempoScaduto()
{
	if (bAutenticato)
	{
		return;
	}
	if (bRichiestaInCorso)
	{
		// Il server sta ancora controllando: qualche secondo in più.
		GetWorldTimerManager().SetTimer(TimerAnticamera, this, &AValdorsoPlayerController::TempoScaduto, 10.f, false);
		return;
	}
	Espelli(TEXT("Tempo scaduto: ricollegati e riprova."));
}

void AValdorsoPlayerController::Espelli(const FString& Motivo)
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Anticamera: scollego %s (%s) - %s"), *NomeAccount, *Indirizzo(), *Motivo);
	AGameModeBase* Modalita = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
	if (Modalita && Modalita->GameSession)
	{
		Modalita->GameSession->KickPlayer(this, FText::FromString(Motivo));
	}
}

void AValdorsoPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Anticamera.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Anticamera.ToSharedRef());
		}
		Anticamera.Reset();
	}
	ChiudiCodiciRecupero();
	ChiudiRegistro();
	ChiudiSceltaPersonaggio();
	ChiudiPausa();
	ChiudiBacheca();
	ChiudiDialogo(false);
	ChiudiMappa();
	ValdorsoFoto::Chiudi(this);
	if (Suggerimento.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Suggerimento.ToSharedRef());
		}
		Suggerimento.Reset();
	}
	ChiudiAvviso();
	// Server che si spegne o cambia mappa: l'ultimo minuto non va perso.
	SalvaPersonaggio();
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerAnticamera);
		GetWorldTimerManager().ClearTimer(TimerSalvataggio);
		GetWorldTimerManager().ClearTimer(TimerUscita);
	}
	Super::EndPlay(EndPlayReason);
}

void AValdorsoPlayerController::ClientEsitoAccesso_Implementation(const FValdorsoEsitoAccount& Esito)
{
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Accesso: %s"), *Esito.Messaggio);
	switch (Esito.Esito)
	{
	case EValdorsoEsitoAccount::Ok:
		// Il personaggio sta nascendo: via l'anticamera, mouse e tastiera al gioco.
		if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
		{
			Istanza->Entrato();
		}
		ChiudiAnticamera();
		MostraSulloSchermo(Esito.Messaggio, FColor(232, 196, 120));
		if (Esito.CodiciRecupero.Num() > 0)
		{
			MostraCodiciRecupero(Esito.CodiciRecupero);
		}
		break;

	case EValdorsoEsitoAccount::OkDeveCambiarePassword:
	case EValdorsoEsitoAccount::PasswordDebole:
	case EValdorsoEsitoAccount::CredenzialiSbagliate:
		MostraAnticamera();
		if (Anticamera.IsValid())
		{
			Anticamera->MostraCambioPassword(FText::FromString(Esito.Messaggio), Esito.Esito != EValdorsoEsitoAccount::OkDeveCambiarePassword);
			FInputModeUIOnly Modo;
			Modo.SetWidgetToFocus(Anticamera->CampoIniziale());
			SetInputMode(Modo);
			bShowMouseCursor = true;
		}
		break;

	default:
		if (Anticamera.IsValid())
		{
			Anticamera->MostraAttesa(FText::FromString(Esito.Messaggio));
		}
		break;
	}
}

void AValdorsoPlayerController::MostraAnticamera()
{
	if (Anticamera.IsValid())
	{
		return;
	}
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	SAssignNew(Anticamera, SValdorsoAnticamera)
		.OnCambia(FValdorsoSuCambioPassword::CreateLambda([Debole](const FString& Attuale, const FString& Nuova)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ServerCambiaPassword(Attuale, Nuova);
			}
		}));
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->AddViewportWidgetContent(Anticamera.ToSharedRef(), 50);
	}
	SetInputMode(FInputModeUIOnly());
}

void AValdorsoPlayerController::ChiudiAnticamera()
{
	if (!Anticamera.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(Anticamera.ToSharedRef());
	}
	Anticamera.Reset();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AValdorsoPlayerController::MostraCodiciRecupero(const TArray<FString>& Codici)
{
	ChiudiCodiciRecupero();
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	TSharedRef<SValdorsoCodiciRecupero> Schermata = SNew(SValdorsoCodiciRecupero)
		.Codici(Codici)
		.OnFatto(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->ChiudiCodiciRecupero();
				Controllore->ServerCodiciScritti();
			}
		}));
	SchermataCodici = Schermata;
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->AddViewportWidgetContent(Schermata, 60);
	}
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Schermata->PulsanteIniziale());
	SetInputMode(Modo);
	bShowMouseCursor = true;
}

void AValdorsoPlayerController::ChiudiCodiciRecupero()
{
	if (!SchermataCodici.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(SchermataCodici.ToSharedRef());
	}
	SchermataCodici.Reset();
	if (IsLocalController() && !IsActorBeingDestroyed())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void AValdorsoPlayerController::ClientSceltaPersonaggio_Implementation(const TArray<FValdorsoPersonaggioBreve>& Elenco, const FString& Messaggio, bool bErrore)
{
	ChiudiAnticamera();
	ChiudiRegistro();
	MostraSceltaPersonaggio();
	if (SceltaPersonaggio.IsValid())
	{
		SceltaPersonaggio->Aggiorna(Elenco, FText::FromString(Messaggio), bErrore);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(SceltaPersonaggio->FuocoIniziale());
		SetInputMode(Modo);
		bShowMouseCursor = true;
	}
}

void AValdorsoPlayerController::ClientPersonaggioScelto_Implementation(const FString& Nome)
{
	ChiudiRegistro();
	ChiudiSceltaPersonaggio();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	bColonoDelRegistro = true;
	MostraSulloSchermo(FString::Printf(TEXT("%s entra nella valle."), *Nome), FColor(232, 196, 120));
}

void AValdorsoPlayerController::MostraSceltaPersonaggio()
{
	// (07/11) una schermata che arriva dal server chiude prima la modalità foto (che tiene tastiera e mouse)
	ValdorsoFoto::Chiudi(this);
	ValdorsoTaccuino::Chiudi(this);
	ValdorsoZainoSchermo::Chiudi(this);
	ValdorsoTesseraSchermo::Chiudi(this);
	if (SceltaPersonaggio.IsValid())
	{
		return;
	}
	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	AValdorsoPalcoRegistro* Scena = AssicuraPalco();
	SAssignNew(SceltaPersonaggio, SValdorsoSceltaPersonaggio)
		.Ritratto(Scena ? Scena->GetRitratto() : nullptr)
		.OnMostra(FValdorsoSuMostraPersonaggio::CreateLambda([Debole](const FValdorsoPersonaggioBreve& Scelto)
		{
			// Chi è selezionato sale sul palco: uomo o donna, con la luce della sua fede.
			AValdorsoPlayerController* Controllore = Debole.Get();
			if (Controllore && Controllore->Palco.IsValid())
			{
				FValdorsoRegistro Aspetto;
				Aspetto.Sesso = Scelto.Sesso;
				Aspetto.Fede = Scelto.Fede;
				// (28/10) Con il suo volto, lo stesso che avrà nella valle.
				Aspetto.Aspetto = ValdorsoAspetto::Visibile(Scelto.Aspetto, Scelto.Sesso, Scelto.Id);
				Controllore->Palco->Presenta(Aspetto);
			}
		}))
		.OnScegli(FValdorsoSuPersonaggio::CreateLambda([Debole](const FString& Id)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->SceltaPersonaggio->Attendi(NSLOCTEXT("ValdorsoPersonaggi", "Entrando", "Il Cuore ti riconosce..."));
				Controllore->ServerScegliPersonaggio(Id);
			}
		}))
		.OnCrea(FValdorsoSuPersonaggio::CreateLambda([Debole](const FString& Nome)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->SceltaPersonaggio->Attendi(NSLOCTEXT("ValdorsoPersonaggi", "Scrivendo", "Il sacerdote scrive nel registro..."));
				Controllore->ServerCreaPersonaggio(Nome);
			}
		}))
		.OnCancella(FValdorsoSuCancellaPersonaggio::CreateLambda([Debole](const FString& Id, const FString& Conferma)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				Controllore->SceltaPersonaggio->Attendi(NSLOCTEXT("ValdorsoPersonaggi", "Cancellando", "Il sacerdote cancella il nome..."));
				Controllore->ServerCancellaPersonaggio(Id, Conferma);
			}
		}))
		.OnEsci(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				// Uscita voluta: niente rientro automatico.
				if (UValdorsoGameInstance* Istanza = Controllore->GetGameInstance<UValdorsoGameInstance>())
				{
					Istanza->DimenticaBiglietto();
				}
				Controllore->ConsoleCommand(TEXT("disconnect"));
			}
		}));
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->AddViewportWidgetContent(SceltaPersonaggio.ToSharedRef(), 55);
	}
}

void AValdorsoPlayerController::ChiudiSceltaPersonaggio()
{
	if (!SceltaPersonaggio.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(SceltaPersonaggio.ToSharedRef());
	}
	SceltaPersonaggio.Reset();
	ForseTogliPalco();
}

void AValdorsoPlayerController::ChiudiProvaRegistro()
{
	ChiudiRegistro();
	RegistroAperto.Empty();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AValdorsoPlayerController::ProvaRegistro()
{
#if WITH_EDITOR
	if (IsLocalController() && GetWorld() && GetWorld()->IsPlayInEditor())
	{
		MostraRegistro(TEXT("prova-registro"), TEXT("Colono di prova"), FValdorsoRegistro());
	}
#endif
}

void AValdorsoPlayerController::MostraRegistro(const FString& IdRegistro, const FString& NomePersonaggio, const FValdorsoRegistro& RisposteServer)
{
	// (07/11) una schermata che arriva dal server chiude prima la modalità foto (che tiene tastiera e mouse)
	ValdorsoFoto::Chiudi(this);
	ValdorsoTaccuino::Chiudi(this);
	ValdorsoZainoSchermo::Chiudi(this);
	ValdorsoTesseraSchermo::Chiudi(this);
	// (28/10, passo 4.4) Un registro ancora da firmare parte con il volto "dei dadi" del personaggio (lo stesso che
	// avrebbe nella valle): il giocatore lo vede sul palco e lo cambia nella pagina del volto.
	FValdorsoRegistro Risposte = RisposteServer;
	if (!Risposte.bFirmato && ValdorsoAspetto::NonScelto(Risposte.Aspetto))
	{
		Risposte.Aspetto = ValdorsoAspetto::Visibile(Risposte.Aspetto, Risposte.Sesso, IdRegistro);
		ValdorsoAspetto::Adatta(Risposte.Aspetto, Risposte.Sesso);
	}

	// Un registro già firmato non si riapre per scriverci (una vista in sola lettura arriverà con l'esame del personaggio).
	// (06/10, passo 4.5) Tranne quando lo staff chiede di correggere la storia: allora si apre su quella pagina.
	if (Risposte.bFirmato && !Risposte.bStoriaDaCorreggere)
	{
		return;
	}
	ChiudiRegistro();
	ChiudiAnticamera();
	RegistroAperto = IdRegistro;

	// Il palco del ritratto (passo 4.2b): lo stesso della scelta del personaggio, se c'è già.
	if (AValdorsoPalcoRegistro* Scena = AssicuraPalco())
	{
		Scena->Presenta(Risposte);
	}

	TWeakObjectPtr<AValdorsoPlayerController> Debole(this);
	SAssignNew(SchermataRegistro, SValdorsoRegistroColono)
		.NomePersonaggio(NomePersonaggio)
		.Registro(Risposte)
		.Ritratto(Palco.IsValid() ? Palco->GetRitratto() : nullptr)
		.OnCambia(FValdorsoSuCambioRisposte::CreateLambda([Debole](const FValdorsoRegistro& Cambiate)
		{
			AValdorsoPlayerController* Controllore = Debole.Get();
			if (Controllore && Controllore->Palco.IsValid())
			{
				Controllore->Palco->Aggiorna(Cambiate);
			}
		}))
		.OnPrimoPiano(FValdorsoSuPrimoPiano::CreateLambda([Debole](bool bSulVolto)
		{
			AValdorsoPlayerController* Controllore = Debole.Get();
			if (Controllore && Controllore->Palco.IsValid())
			{
				Controllore->Palco->PrimoPiano(bSulVolto);
			}
		}))
		.OnGira(FValdorsoSuGira::CreateLambda([Debole](float Gradi, bool bDiFronte)
		{
			AValdorsoPlayerController* Controllore = Debole.Get();
			if (Controllore && Controllore->Palco.IsValid())
			{
				Controllore->Palco->Gira(Gradi, bDiFronte);
			}
		}))
		.OnSalva(FValdorsoSuSalvaRegistro::CreateLambda([Debole](const FValdorsoRegistro& Proposto, bool bFirma)
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				// (28/10) Il Registro di prova (ProvaRegistro) non va al server: va sempre bene.
				if (Controllore->RegistroAperto == TEXT("prova-registro"))
				{
					// La risposta arriva al giro dopo, come farebbe quella del server. (02/11) Come il server, alla firma
					// segna il registro come firmato: senza, la prova restava ferma su "Il sacerdote firma il registro...".
					FValdorsoRegistro Risposta = Proposto;
					Risposta.bFirmato = bFirma;
					Controllore->GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(Controllore, [Controllore, Risposta]()
					{
						if (Controllore->SchermataRegistro.IsValid())
						{
							Controllore->SchermataRegistro->Esito(FString(), true, Risposta);
						}
					}));
					return;
				}
				Controllore->ServerSalvaRegistro(Controllore->RegistroAperto, Proposto, bFirma);
			}
		}))
		.OnFirmato(FSimpleDelegate::CreateLambda([Debole]()
		{
			// Il registro resta nero finché il personaggio non nasce (ClientPersonaggioScelto lo chiude).
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				if (Controllore->RegistroAperto == TEXT("prova-registro"))
				{
					Controllore->ChiudiProvaRegistro();
					return;
				}
				Controllore->ServerScegliPersonaggio(Controllore->RegistroAperto);
			}
		}))
		.OnTorna(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (AValdorsoPlayerController* Controllore = Debole.Get())
			{
				if (Controllore->RegistroAperto == TEXT("prova-registro"))
				{
					Controllore->ChiudiProvaRegistro();
					return;
				}
				Controllore->ServerTornaAllaScelta();
			}
		}));
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->AddViewportWidgetContent(SchermataRegistro.ToSharedRef(), 57);
	}
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(SchermataRegistro->FuocoIniziale());
	SetInputMode(Modo);
	bShowMouseCursor = true;
}

AValdorsoPalcoRegistro* AValdorsoPlayerController::AssicuraPalco()
{
	if (!Palco.IsValid())
	{
		// Molto in alto sopra la mappa, visto solo dalla sua telecamera.
		if (UWorld* Mondo = GetWorld())
		{
			FActorSpawnParameters Parametri;
			Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Palco = Mondo->SpawnActor<AValdorsoPalcoRegistro>(AValdorsoPalcoRegistro::StaticClass(),
				FTransform(FVector(0.f, 0.f, 30000.f)), Parametri);
		}
	}
	return Palco.Get();
}

void AValdorsoPlayerController::ForseTogliPalco()
{
	// Prima si tolgono le schermate (che tengono vivo il ritratto), poi il palco.
	if (SchermataRegistro.IsValid() || SceltaPersonaggio.IsValid())
	{
		return;
	}
	if (Palco.IsValid())
	{
		Palco->Destroy();
	}
	Palco.Reset();
}

void AValdorsoPlayerController::ChiudiRegistro()
{
	if (!SchermataRegistro.IsValid())
	{
		ForseTogliPalco();
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(SchermataRegistro.ToSharedRef());
	}
	SchermataRegistro.Reset();
	RegistroAperto.Empty();
	ForseTogliPalco();
}

void AValdorsoPlayerController::ClientBiglietto_Implementation(const FString& Biglietto, const FString& Nome)
{
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->RicordaBiglietto(Biglietto, Nome);
	}
}

void AValdorsoPlayerController::ClientWasKicked_Implementation(const FText& KickReason)
{
	if (UValdorsoGameInstance* Istanza = GetGameInstance<UValdorsoGameInstance>())
	{
		Istanza->RicordaMessaggio(KickReason.ToString());
		// Espulsi dal server: niente rientro automatico (si torna dalla schermata, con la password).
		Istanza->DimenticaBiglietto();
	}
	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Scollegato dal server: %s"), *KickReason.ToString());
	Super::ClientWasKicked_Implementation(KickReason);
}

void AValdorsoPlayerController::MostraSulloSchermo(const FString& Testo, const FColor& Colore) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, Colore, Testo);
	}
}
