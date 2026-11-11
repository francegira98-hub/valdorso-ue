// Valdorso - Lo zaino, con il peso e le monete (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoZaino.h"
#include "ValdorsoScherma.h"
#include "ValdorsoGilda.h"
#include "SValdorsoZaino.h"
#include "SValdorsoBottega.h"
#include "ValdorsoBotteghe.h"
#include "Valdorso.h"
#include "ValdorsoAcque.h"
#include "ValdorsoArmi.h"
#include "ValdorsoTempo.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoFalo.h"
#include "ValdorsoModoFoto.h"
#include "ValdorsoMovementComponent.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistro.h"
#include "ValdorsoTaccuino.h"
#include "SValdorsoTessera.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "ValdorsoZaino"

namespace
{
	/** Dopo quanti secondi da un cambiamento si salva il file (una volta sola, anche dopo dieci spostamenti). */
	constexpr double ZainoSecondiSalvataggio = 4.0;
	/** Ogni quanto (secondi) il server guarda l'acqua e il peso. */
	constexpr double ZainoSecondiControllo = 0.5;
	/** Un oggetto usato ogni quanti secondi. */
	constexpr double ZainoSecondiUso = 1.0;
	/** Quante richieste al secondo accetta il server da un giocatore. */
	constexpr int32 ZainoRichiesteAlSecondo = 15;
	/** Quanto deve essere sott'acqua il corpo perché la borraccia si riempia (0..1). */
	constexpr float ZainoAcquaPerBorraccia = 0.3f;

	/** Su ogni zaino con autorità (server, o da soli) del mondo, del giocatore con quel nome (vuoto = tutti). */
	int32 ZainoNelMondo(UWorld* Mondo, const FString& Nome, TFunctionRef<void(UValdorsoZainoComponent&)> Fai);

	int32 ZainoPerOgniGiocatore(UWorld* Mondo, const FString& Nome, TFunctionRef<void(UValdorsoZainoComponent&)> Fai)
	{
		const int32 Trovati = ZainoNelMondo(Mondo, Nome, Fai);
		if (Trovati > 0 || !GEngine)
		{
			return Trovati;
		}
		// (09/11) scritto nella console dell'editor durante il Play: il mondo giusto e' quello del gioco, non quello dell'editor
		int32 Altri = 0;
		for (const FWorldContext& Contesto : GEngine->GetWorldContexts())
		{
			UWorld* Gioco = Contesto.World();
			if (Gioco && Gioco != Mondo && (Contesto.WorldType == EWorldType::PIE || Contesto.WorldType == EWorldType::Game))
			{
				Altri += ZainoNelMondo(Gioco, Nome, Fai);
			}
		}
		return Altri;
	}

	int32 ZainoNelMondo(UWorld* Mondo, const FString& Nome, TFunctionRef<void(UValdorsoZainoComponent&)> Fai)
	{
		int32 Trovati = 0;
		if (!Mondo)
		{
			return 0;
		}
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Controllore = It->Get();
			UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
			if (!Zaino || !Controllore->HasAuthority())
			{
				continue;
			}
			const APlayerState* Stato = Controllore->PlayerState;
			if (!Nome.IsEmpty() && (!Stato || !Stato->GetPlayerName().Equals(Nome, ESearchCase::IgnoreCase)))
			{
				continue;
			}
			Fai(*Zaino);
			++Trovati;
		}
		return Trovati;
	}

	void ZainoEsito(int32 Trovati, const FString& Cosa)
	{
		const FString Esito = Trovati > 0 ? FString::Printf(TEXT("%s (%d giocatori)"), *Cosa, Trovati)
			: FString(TEXT("nessun giocatore (il comando vale sul server, o giocando da soli)"));
		UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Zaino: %s"), *Esito);
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoZainoDai(
		TEXT("Valdorso.Zaino.Dai"),
		TEXT("Per le prove: dà oggetti, Valdorso.Zaino.Dai <Id> [quanti] [nome] (gli Id: Valdorso.Zaino.Elenco; sul server, o da soli)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			if (Argomenti.Num() < 1 || !ValdorsoOggetti::Trova(FName(*Argomenti[0])))
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Zaino: serve l'Id di un oggetto, per esempio Valdorso.Zaino.Dai Pane_Nero 3 (l'elenco: Valdorso.Zaino.Elenco)"));
				return;
			}
			const FName Id(*Argomenti[0]);
			const int32 Quanti = Argomenti.Num() > 1 ? FMath::Max(1, FCString::Atoi(*Argomenti[1])) : 1;
			const FString Nome = Argomenti.Num() > 2 ? Argomenti[2] : FString();
			ZainoEsito(ZainoPerOgniGiocatore(Mondo, Nome, [Id, Quanti](UValdorsoZainoComponent& Zaino) { Zaino.Dai(Id, Quanti); }),
				FString::Printf(TEXT("dati %d %s"), Quanti, *Id.ToString()));
		}));

	// (10/11) Per le prove: tutte le armi e gli scudi che si impugnano (quelli che entrano nello zaino)
	FAutoConsoleCommandWithWorldAndArgs ComandoZainoArmeria(
		TEXT("Valdorso.Zaino.Armeria"),
		TEXT("Per le prove: dà una di ogni arma e scudo che si impugna (quelli che entrano), Valdorso.Zaino.Armeria [nome]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const FString Nome = Argomenti.Num() > 0 ? Argomenti[0] : FString();
			ZainoEsito(ZainoPerOgniGiocatore(Mondo, Nome, [](UValdorsoZainoComponent& Zaino)
			{
				for (const FValdorsoTipoOggetto& Tipo : ValdorsoOggetti::Catalogo())
				{
					if (Tipo.Uso == EValdorsoUso::Impugna && ValdorsoZaino::Conta(Zaino.Contenuto(), Tipo.Id) == 0)
					{
						Zaino.Dai(Tipo.Id, 1, false);
					}
				}
			}), TEXT("l'armeria"));
		}));

	FAutoConsoleCommandWithWorldAndArgs ComandoZainoMonete(
		TEXT("Valdorso.Zaino.Monete"),
		TEXT("Per le prove: quanti Aureus ha il colono, Valdorso.Zaino.Monete <n> [nome] (sul server, o da soli)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			if (Argomenti.Num() < 1)
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Zaino: serve il numero, per esempio Valdorso.Zaino.Monete 50"));
				return;
			}
			const int32 Monete = FMath::Max(0, FCString::Atoi(*Argomenti[0]));
			const FString Nome = Argomenti.Num() > 1 ? Argomenti[1] : FString();
			ZainoEsito(ZainoPerOgniGiocatore(Mondo, Nome, [Monete](UValdorsoZainoComponent& Zaino) { Zaino.ImpostaMonete(Monete); }),
				FString::Printf(TEXT("%d Aureus"), Monete));
		}));

	FAutoConsoleCommandWithWorldAndArgs ComandoZainoSvuota(
		TEXT("Valdorso.Zaino.Svuota"),
		TEXT("Per le prove: svuota lo zaino (anche i ricordi) e la barra delle azioni, Valdorso.Zaino.Svuota [nome] (sul server, o da soli)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const FString Nome = Argomenti.Num() > 0 ? Argomenti[0] : FString();
			ZainoEsito(ZainoPerOgniGiocatore(Mondo, Nome, [](UValdorsoZainoComponent& Zaino) { Zaino.Svuota(); }), TEXT("svuotato"));
		}));

	FAutoConsoleCommandWithWorld ComandoZainoElenco(
		TEXT("Valdorso.Zaino.Elenco"),
		TEXT("Scrive gli Id degli oggetti della valle (per Valdorso.Zaino.Dai)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld*)
		{
			for (const FValdorsoTipoOggetto& Tipo : ValdorsoOggetti::Catalogo())
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Zaino: %-16s %s (%s)"), *Tipo.Id.ToString(), Tipo.Nome,
					*ValdorsoOggetti::TestoPeso(Tipo.Peso));
			}
		}));
}

// ------------------------------------------------------------------------------------------------- il componente

UValdorsoZainoComponent::UValdorsoZainoComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	SetIsReplicatedByDefault(true);
	ValdorsoZaino::Sistema(Zaino);
}

UValdorsoZainoComponent* UValdorsoZainoComponent::Di(const APlayerController* Controllore)
{
	return Controllore ? Controllore->FindComponentByClass<UValdorsoZainoComponent>() : nullptr;
}

void UValdorsoZainoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// solo al padrone: gli altri non sanno cosa porta nello zaino
	DOREPLIFETIME_CONDITION(UValdorsoZainoComponent, Zaino, COND_OwnerOnly);
}

float UValdorsoZainoComponent::Peso() const
{
	return ValdorsoZaino::Peso(Zaino);
}

void UValdorsoZainoComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	UWorld* Mondo = GetWorld();
	if (!Controllore || !Mondo)
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	if (Controllore->HasAuthority())
	{
		// l'accesso libero dell'editor: niente file, ma il colono ha comunque le sue cose
		if (!bPersonaggio && !Zaino.bPartenzaData && Controllore->GetPawn())
		{
			ValdorsoZaino::DaiPartenza(Zaino, nullptr);
			++NumeroCambiamenti;
		}
		if (Adesso >= ProssimoControllo)
		{
			ProssimoControllo = Adesso + ZainoSecondiControllo;
			ControllaAcqua();
			// (09/11) le armi in mano anche sul colono nuovo (dopo la rinascita, o appena entrato)
			ArmiInMano();
		}
		if (bDaSalvare && Adesso >= SalvaDopo)
		{
			bDaSalvare = false;
			if (bPersonaggio)
			{
				if (AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore))
				{
					Nostro->SalvaPersonaggio();
				}
			}
		}
	}
	// il peso sul movimento: sul server e sul PC di chi gioca (lo prevede lui), uguale
	if (Controllore->HasAuthority() || Controllore->IsLocalController())
	{
		ApplicaPeso();
	}
}

void UValdorsoZainoComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Schermata.IsValid())
	{
		UWorld* Mondo = GetWorld();
		if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Schermata.ToSharedRef());
		}
		Schermata.Reset();
	}
	if (SchermataBottega.IsValid())
	{
		UWorld* Mondo = GetWorld();
		if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(SchermataBottega.ToSharedRef());
		}
		SchermataBottega.Reset();
	}
	if (SchermataTessera.IsValid())
	{
		UWorld* Mondo = GetWorld();
		if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(SchermataTessera.ToSharedRef());
		}
		SchermataTessera.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

// --- Sul server --------------------------------------------------------------------------------------

void UValdorsoZainoComponent::Carica(const FValdorsoZaino& Salvato, const FValdorsoRegistro* Registro)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Zaino = Salvato;
	ValdorsoZaino::Sistema(Zaino);
	bPersonaggio = true;
	if (!Zaino.bPartenzaData)
	{
		// la prima volta nella valle: quello che il colono ha portato con sé (dal suo Registro)
		ValdorsoZaino::DaiPartenza(Zaino, Registro);
		Cambiato();
	}
	else if (Zaino.Aggiornamenti < ValdorsoZaino::UltimoAggiornamento)
	{
		// (08/11) chi era già arrivato riceve le cose aggiunte poi (la mappa della valle)
		if (ValdorsoZaino::Aggiorna(Zaino))
		{
			Cambiato();
		}
	}
	++NumeroCambiamenti;
}

int32 UValdorsoZainoComponent::Dai(FName Id, int32 Quantita, bool bAvvisa)
{
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Tipo || Quantita <= 0)
	{
		return 0;
	}
	const int32 Entrati = ValdorsoZaino::Aggiungi(Zaino, Id, Quantita);
	if (Entrati > 0)
	{
		Cambiato();
	}
	if (bAvvisa)
	{
		if (Entrati <= 0)
		{
			Avvisa(FText::Format(LOCTEXT("Pieno", "Lo zaino è pieno: {0} resta dov'è."), FText::FromString(Tipo->Nome)), 3.f);
		}
		else if (Entrati == 1)
		{
			Avvisa(FText::Format(LOCTEXT("Ricevi", "Ricevi: {0}"), FText::FromString(Tipo->Nome)), 3.f);
		}
		else
		{
			Avvisa(FText::Format(LOCTEXT("RiceviTanti", "Ricevi: {0} x{1}"), FText::FromString(Tipo->Nome), FText::AsNumber(Entrati)), 3.f);
		}
	}
	return Entrati;
}

void UValdorsoZainoComponent::ImpostaMonete(int32 Monete)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Zaino.Monete = FMath::Clamp(Monete, 0, ValdorsoZaino::MassimoMonete);
		Cambiato();
	}
}

void UValdorsoZainoComponent::Svuota()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		const bool bData = Zaino.bPartenzaData;
		Zaino = FValdorsoZaino();
		ValdorsoZaino::Sistema(Zaino);
		// svuotato apposta: la partenza non torna da sola
		Zaino.bPartenzaData = bData || !bPersonaggio;
		Cambiato();
	}
}

void UValdorsoZainoComponent::ImpostaTessera(const FValdorsoTesseraVista& Vista)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Tessera = Vista;
	}
}

void UValdorsoZainoComponent::ServerMostraTessera_Implementation()
{
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	APawn* Io = Controllore ? Controllore->GetPawn() : nullptr;
	UWorld* Mondo = GetWorld();
	if (!Io || !Mondo || TroppeRichieste())
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	if (Adesso < ProssimaTesseraMostrata)
	{
		return;
	}
	ProssimaTesseraMostrata = Adesso + 2.0;
	if (ValdorsoZaino::Conta(Zaino, TEXT("Tessera")) == 0)
	{
		Avvisa(LOCTEXT("SenzaTessera", "Non hai la tessera con te."), 3.f);
		return;
	}
	// chi si ha davanti: il colono più vicino entro 3,5 m, non alle spalle
	APlayerController* Scelto = nullptr;
	float Migliore = 350.f;
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Altro = It->Get();
		APawn* Corpo = Altro ? Altro->GetPawn() : nullptr;
		if (!Corpo || Altro == Controllore || !UValdorsoZainoComponent::Di(Altro))
		{
			continue;
		}
		const FVector Verso = Corpo->GetActorLocation() - Io->GetActorLocation();
		const float Distanza = Verso.Size();
		if (Distanza < Migliore && FVector::DotProduct(Io->GetActorForwardVector().GetSafeNormal2D(), Verso.GetSafeNormal2D()) > -0.2f)
		{
			Migliore = Distanza;
			Scelto = Altro;
		}
	}
	UValdorsoZainoComponent* SuoZaino = UValdorsoZainoComponent::Di(Scelto);
	if (!SuoZaino)
	{
		Avvisa(LOCTEXT("Nessuno", "Non c'è nessuno qui davanti a cui mostrarla."), 3.f);
		return;
	}
	TesseraDiProva();
	SuoZaino->ClientTessera(Tessera, Io, false);
	const APlayerState* Stato = Scelto->PlayerState;
	Avvisa(FText::Format(LOCTEXT("Mostrata", "Mostri la tessera a {0}."), FText::FromString(Stato ? Stato->GetPlayerName() : FString(TEXT("chi hai davanti")))), 3.f);
}

void UValdorsoZainoComponent::TesseraDiProva()
{
	// chi entra con l'accesso libero dell'editor non ha un file: una tessera di prova
	if (!Tessera.Tessera.Numero.IsEmpty())
	{
		return;
	}
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APlayerState* Stato = Controllore ? Controllore->PlayerState.Get() : nullptr;
	const FString Nome = Stato ? Stato->GetPlayerName() : FString(TEXT("Colono"));
	Tessera = ValdorsoTessera::Vista(Nome, nullptr, ValdorsoTessera::Nuova(Nome));
}

void UValdorsoZainoComponent::ClientApriBottega_Implementation(uint8 Banco)
{
	ValdorsoBottegaSchermo::Apri(Cast<APlayerController>(GetOwner()),
		static_cast<EValdorsoTipoBanco>(FMath::Min<uint8>(Banco, static_cast<uint8>(EValdorsoTipoBanco::Carovana))));
}

void UValdorsoZainoComponent::ServerRicetta_Implementation(FName Ricetta)
{
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
	const FValdorsoRicetta* R = ValdorsoBotteghe::Trova(Ricetta);
	if (!Corpo || !R || TroppeRichieste())
	{
		return;
	}
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
	if (Pericoli && Pericoli->Caduto())
	{
		return;
	}
	// il server non si fida: il colono deve essere davvero al banco giusto (o vicino a un fuoco acceso)
	if (!ValdorsoBotteghe::BancoVicino(Corpo, R->Banco))
	{
		Avvisa(LOCTEXT("LontanoBanco", "Sei troppo lontano dal banco."), 2.f);
		return;
	}
	const FText Perche = ValdorsoBotteghe::Manca(Zaino, *R);
	if (!Perche.IsEmpty() || !ValdorsoBotteghe::Fai(Zaino, *R))
	{
		Avvisa(Perche.IsEmpty() ? LOCTEXT("NonRiesce", "Non si può fare.") : Perche, 2.5f);
		return;
	}
	Cambiato();
	Avvisa(FText::Format(LOCTEXT("Fatto", "{0}: {1}"), FText::FromString(R->Verbo), R->Guadagno > 0 && !R->Risultato
		? ValdorsoBotteghe::TestoServe(*R) : ValdorsoBotteghe::TestoDa(*R)), 2.f);
}

void UValdorsoZainoComponent::ClientTessera_Implementation(const FValdorsoTesseraVista& Vista, APawn* Colono, bool bPropria)
{
	ValdorsoTesseraSchermo::Apri(Cast<APlayerController>(GetOwner()), Vista, Colono, bPropria);
}

void UValdorsoZainoComponent::ArmiInMano()
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (!Controllore || !Controllore->HasAuthority())
	{
		return;
	}
	if (UValdorsoArmiComponent* Armi = UValdorsoArmiComponent::Di(Controllore->GetPawn()))
	{
		Armi->Imposta(Zaino.Destra, Zaino.Sinistra);
	}
}

void UValdorsoZainoComponent::Cambiato()
{
	// (09/11) quello che non c'e' piu' nello zaino non resta in mano (gettato, dato, consumato)
	ValdorsoZaino::SistemaMani(Zaino);
	ArmiInMano();
	++NumeroCambiamenti;
	if (!bDaSalvare)
	{
		bDaSalvare = true;
		const UWorld* Mondo = GetWorld();
		SalvaDopo = (Mondo ? Mondo->GetTimeSeconds() : 0.0) + ZainoSecondiSalvataggio;
	}
}

bool UValdorsoZainoComponent::TroppeRichieste()
{
	const UWorld* Mondo = GetWorld();
	const double Adesso = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	if (Adesso - InizioRichieste >= 1.0)
	{
		InizioRichieste = Adesso;
		RichiesteFatte = 0;
	}
	return ++RichiesteFatte > ZainoRichiesteAlSecondo;
}

bool UValdorsoZainoComponent::Cambia(TFunctionRef<bool(FValdorsoZaino&)> Cosa)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Cosa(Zaino))
	{
		return false;
	}
	ValdorsoZaino::Sistema(Zaino);
	Cambiato();
	return true;
}

void UValdorsoZainoComponent::Avvisa(const FText& Testo, float Secondi)
{
	if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(GetOwner()))
	{
		Controllore->ClientAvviso(Testo, Secondi, 0.f);
	}
}

void UValdorsoZainoComponent::ServerUsa_Implementation(int32 Posto)
{
	if (!TroppeRichieste())
	{
		Usa(Posto);
	}
}

void UValdorsoZainoComponent::ServerUsaAzione_Implementation(int32 Tasto)
{
	if (TroppeRichieste() || !Zaino.Azioni.IsValidIndex(Tasto) || Zaino.Azioni[Tasto].IsNone())
	{
		return;
	}
	const FName Id = Zaino.Azioni[Tasto];
	// (11/11, v0.1.5) una tecnica (ValdorsoScherma.h): la fa il colono, se puo'
	if (ValdorsoScherma::DaBarra(Id))
	{
		const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
		if (UValdorsoSchermaComponent* Scherma = UValdorsoSchermaComponent::Di(Controllore ? Controllore->GetPawn() : nullptr))
		{
			Scherma->UsaTecnica(Id);
		}
		return;
	}
	const int32 Posto = ValdorsoZaino::PostoDi(Zaino, Id);
	if (Posto == INDEX_NONE)
	{
		const UWorld* Mondo = GetWorld();
		const double Adesso = Mondo ? Mondo->GetTimeSeconds() : 0.0;
		if (Adesso < ProssimoUso)
		{
			return;
		}
		ProssimoUso = Adesso + ZainoSecondiUso;
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
		Avvisa(FText::Format(LOCTEXT("Finito", "Non hai più: {0}"), FText::FromString(Tipo ? Tipo->Nome : TEXT("?"))));
		return;
	}
	Usa(Posto);
}

void UValdorsoZainoComponent::ServerSposta_Implementation(int32 Da, int32 A)
{
	if (!TroppeRichieste() && ValdorsoZaino::Sposta(Zaino, Da, A))
	{
		Cambiato();
	}
}

void UValdorsoZainoComponent::ServerGetta_Implementation(int32 Posto)
{
	if (TroppeRichieste() || !Zaino.Posti.IsValidIndex(Posto) || Zaino.Posti[Posto].Vuoto())
	{
		return;
	}
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Zaino.Posti[Posto].Id);
	if (!ValdorsoZaino::SiPuoGettare(Zaino, Posto))
	{
		Avvisa(FText::Format(LOCTEXT("NonSiGetta", "{0}: questo non lo getti."), FText::FromString(Tipo ? Tipo->Nome : TEXT("?"))), 3.f);
		return;
	}
	// per ora gettare vuol dire lasciarlo lì: non resta per terra (gli oggetti nel mondo arriveranno più avanti)
	Zaino.Posti[Posto] = FValdorsoPostoZaino();
	Cambiato();
	if (Tipo)
	{
		Avvisa(FText::Format(LOCTEXT("Gettato", "Lasci: {0}"), FText::FromString(Tipo->Nome)));
	}
}

void UValdorsoZainoComponent::ServerAssegna_Implementation(int32 Tasto, int32 Posto)
{
	if (TroppeRichieste())
	{
		return;
	}
	FName Id = NAME_None;
	if (Posto != INDEX_NONE)
	{
		if (!Zaino.Posti.IsValidIndex(Posto) || Zaino.Posti[Posto].Vuoto())
		{
			return;
		}
		Id = Zaino.Posti[Posto].Id;
		if (!ValdorsoZaino::DaBarra(Id))
		{
			const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
			Avvisa(FText::Format(LOCTEXT("NonDaBarra", "{0} non si usa: nella barra non serve."), FText::FromString(Tipo ? Tipo->Nome : TEXT("?"))), 3.f);
			return;
		}
	}
	if (ValdorsoZaino::Assegna(Zaino, Tasto, Id))
	{
		Cambiato();
	}
}

void UValdorsoZainoComponent::Usa(int32 Posto)
{
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
	UWorld* Mondo = GetWorld();
	if (!Corpo || !Mondo || !Zaino.Posti.IsValidIndex(Posto) || Zaino.Posti[Posto].Vuoto())
	{
		return;
	}
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
	if (Pericoli && Pericoli->Caduto())
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	if (Adesso < ProssimoUso)
	{
		return;
	}
	FValdorsoPostoZaino& Dove = Zaino.Posti[Posto];
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Dove.Id);
	if (!Tipo)
	{
		return;
	}
	const FText Nome = FText::FromString(Tipo->Nome);
	// anche gli avvisi aspettano il secondo: un PC che preme di continuo non riempie lo schermo
	ProssimoUso = Adesso + ZainoSecondiUso;
	UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Corpo);
	// quanto ridarebbe davvero (salute, stamina, equilibrio già pieni: niente)
	auto Manca = [Abilita](const FGameplayAttribute& Attributo, const FGameplayAttribute& Massimo, float Quanto)
	{
		if (!Abilita || Quanto <= 0.f || !Abilita->HasAttributeSetForAttribute(Attributo))
		{
			return 0.f;
		}
		return FMath::Min(Quanto, FMath::Max(0.f, Abilita->GetNumericAttribute(Massimo) - Abilita->GetNumericAttributeBase(Attributo)));
	};
	const float Guadagno = Manca(UValdorsoAttributeSet::GetSaluteAttribute(), UValdorsoAttributeSet::GetSaluteMaxAttribute(), Tipo->Salute)
		+ Manca(UValdorsoAttributeSet::GetStaminaAttribute(), UValdorsoAttributeSet::GetStaminaMaxAttribute(), Tipo->Stamina)
		+ Manca(UValdorsoAttributeSet::GetEquilibrioAttribute(), UValdorsoAttributeSet::GetEquilibrioMaxAttribute(), Tipo->Equilibrio);
	const bool bSiConsuma = Tipo->Uso == EValdorsoUso::Mangia || Tipo->Uso == EValdorsoUso::Bevi || Tipo->Uso == EValdorsoUso::Cura;
	if (bSiConsuma && Guadagno < 0.5f && !(Tipo->Uso == EValdorsoUso::Bevi && Dove.Cariche <= 0))
	{
		Avvisa(Tipo->Uso == EValdorsoUso::Cura ? LOCTEXT("NonFerito", "Non hai ferite da medicare.")
			: LOCTEXT("NonServe", "Adesso non ne hai bisogno: tienilo per dopo."), 2.f);
		return;
	}
	// (10/11) il foglio di un incarico: si legge quello che c'e' scritto (lo sa la Gilda del colono)
	if (Dove.Id == ValdorsoGilda::IdFoglio)
	{
		const UValdorsoGildaComponent* Gilda = UValdorsoGildaComponent::Di(Controllore);
		Avvisa(Gilda ? Gilda->LetturaFoglio(Dove.Numero) : FText::FromString(Tipo->Lettura), 8.f);
		return;
	}
	switch (Tipo->Uso)
	{
	case EValdorsoUso::Niente:
		Avvisa(FText::Format(LOCTEXT("NonSiUsa", "{0}: per adesso non serve a niente. Tienilo da conto."), Nome), 3.f);
		return;
	case EValdorsoUso::Leggi:
		Avvisa(FText::FromString(Tipo->Lettura), 7.f);
		return;
	case EValdorsoUso::Documento:
	{
		// (09/11) la tessera: si apre come un foglio vero sul PC di chi gioca (con la sua faccia)
		TesseraDiProva();
		ClientTessera(Tessera, Corpo, true);
		return;
	}
	case EValdorsoUso::Accendi:
	{
		// (08/11) il falò: servono i rami secchi; dove e se si può lo decide AValdorsoFalo::Accendi
		const FValdorsoTipoOggetto* Rami = ValdorsoOggetti::Trova(TEXT("Rami_Secchi"));
		if (ValdorsoZaino::Conta(Zaino, TEXT("Rami_Secchi")) < ValdorsoOggetti::RamiPerFalo)
		{
			Avvisa(FText::Format(LOCTEXT("SenzaRami", "Per un falò servono {0} fasci di {1}: nei boschi se ne trovano."),
				FText::AsNumber(ValdorsoOggetti::RamiPerFalo), FText::FromString(Rami ? FString(Rami->Nome).ToLower() : FString(TEXT("rami secchi")))), 3.5f);
			return;
		}
		const FText Perche = AValdorsoFalo::Accendi(Corpo);
		if (!Perche.IsEmpty())
		{
			Avvisa(Perche, 3.5f);
			return;
		}
		ValdorsoZaino::TogliOggetto(Zaino, TEXT("Rami_Secchi"), ValdorsoOggetti::RamiPerFalo);
		Cambiato();
		Avvisa(LOCTEXT("Acceso", "Accendi il falò. Al fuoco ti asciughi e ti scaldi."), 3.f);
		return;
	}
	case EValdorsoUso::Impugna:
	{
		// (09/11) le armi: in mano o via; il modello lo mette il tick (ArmiInMano), qui solo lo zaino
		const bool bInMano = ValdorsoZaino::Impugna(Zaino, Dove.Id);
		Cambiato();
		const bool bSinistra = Tipo->Posto == EValdorsoPosto::ManoSinistra;
		Avvisa(bInMano ? FText::Format(bSinistra ? LOCTEXT("Imbracci", "Imbracci: {0}") : LOCTEXT("Impugni", "Impugni: {0}"), Nome)
			: FText::Format(LOCTEXT("RimettiVia", "Rimetti via: {0}"), Nome), 2.f);
		return;
	}
	case EValdorsoUso::Bevi:
		if (Dove.Cariche <= 0)
		{
			Avvisa(FText::Format(LOCTEXT("Vuota", "{0} è vuota: riempila entrando nel lago o nel fiume."), Nome), 3.f);
			return;
		}
		--Dove.Cariche;
		// (11/11, v0.2) il secchio finito torna vuoto
		if (Dove.Cariche <= 0 && FCString::Strlen(Tipo->Vuoto) > 0 && ValdorsoOggetti::Trova(FName(Tipo->Vuoto)))
		{
			const FName Prima = Dove.Id;
			Dove.Id = FName(Tipo->Vuoto);
			Dove.Cariche = 0;
			// (anche nella barra delle azioni)
			for (FName& Azione : Zaino.Azioni)
			{
				Azione = Azione == Prima ? Dove.Id : Azione;
			}
		}
		break;
	case EValdorsoUso::Raccogli:
	{
		// (11/11, v0.2 "L'acqua della valle") il secchio: dentro l'acqua almeno fino alle ginocchia; quale acqua, lo dice
		// il nome dello specchio o del corso (ValdorsoAcqua::Qualita)
		const ACharacter* Colono = Cast<ACharacter>(Corpo);
		const UValdorsoMovementComponent* Movimento = Colono ? Cast<UValdorsoMovementComponent>(Colono->GetCharacterMovement()) : nullptr;
		const bool bDentro = Movimento && (Movimento->IsSwimming() || Movimento->ImmersioneNellaValle() >= ZainoAcquaPerBorraccia);
		// (11/11) fuori dall'acqua, sotto la pioggia vera e senza un tetto sopra: l'acqua piovana (pura, ma poca)
		if (!bDentro && Corpo && GetWorld())
		{
			const AValdorsoTempo* Tempo = AValdorsoTempo::Di(GetWorld());
			const FVector Testa = Corpo->GetActorLocation() + FVector(0.f, 0.f, 80.f);
			FHitResult Tetto;
			const FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoSecchioPioggia), false, Corpo);
			if (Tempo && Tempo->AriaAdesso().Pioggia >= 0.35f
				&& !GetWorld()->LineTraceSingleByChannel(Tetto, Testa, Testa + FVector(0.f, 0.f, 3000.f), ECC_Visibility, Parametri))
			{
				for (FName& Azione : Zaino.Azioni)
				{
					Azione = Azione == Dove.Id ? FName(TEXT("Secchio_Acqua_Pura")) : Azione;
				}
				Dove.Id = TEXT("Secchio_Acqua_Pura");
				Dove.Cariche = 3;
				Cambiato();
				Avvisa(LOCTEXT("SecchioPioggia", "Lasci il secchio sotto la pioggia: un fondo d'acqua piovana, pura (3 sorsi)."), 3.f);
				return;
			}
		}
		if (!bDentro)
		{
			Avvisa(LOCTEXT("SecchioFuori", "Per riempire il secchio entra nell'acqua, almeno fino alle ginocchia (la pozza della cascata, il lago, il fiume), tiralo su dal pozzo del villaggio, o lascialo sotto la pioggia."), 3.5f);
			return;
		}
		const AValdorsoAcque* Acque = AValdorsoAcque::Di(GetWorld());
		const FString Dove2D = Acque ? Acque->NomeQui(FVector2D(Corpo->GetActorLocation())) : FString();
		static const TCHAR* Pieni[] = { TEXT("Secchio_Acqua_Torbida"), TEXT("Secchio_Acqua_Chiara"), TEXT("Secchio_Acqua_Pura") };
		const FName Pieno(Pieni[FMath::Clamp(ValdorsoAcqua::Qualita(Dove2D), 0, 2)]);
		const FValdorsoTipoOggetto* TipoPieno = ValdorsoOggetti::Trova(Pieno);
		if (!TipoPieno)
		{
			return;
		}
		for (FName& Azione : Zaino.Azioni)
		{
			Azione = Azione == Dove.Id ? Pieno : Azione;
		}
		Dove.Id = Pieno;
		Dove.Cariche = TipoPieno->Cariche;
		Cambiato();
		Avvisa(FText::Format(LOCTEXT("SecchioPieno", "Riempi il secchio: {0}."), FText::FromString(FString(TipoPieno->Nome).ToLower())), 3.f);
		return;
	}
	case EValdorsoUso::Mangia:
	case EValdorsoUso::Cura:
		ValdorsoZaino::Togli(Zaino, Posto, 1);
		break;
	}

	// le statistiche: come Valdorso.Ferisci (UValdorsoPericoliComponent::Ferisci), dentro i massimi
	if (Abilita)
	{
		auto AggiungiA = [Abilita](const FGameplayAttribute& Attributo, const FGameplayAttribute& Massimo, float Quanto)
		{
			if (Quanto != 0.f && Abilita->HasAttributeSetForAttribute(Attributo))
			{
				const float Prima = Abilita->GetNumericAttributeBase(Attributo);
				const float Tetto = Abilita->GetNumericAttribute(Massimo);
				Abilita->SetNumericAttributeBase(Attributo, FMath::Clamp(Prima + Quanto, 0.f, Tetto));
			}
		};
		AggiungiA(UValdorsoAttributeSet::GetSaluteAttribute(), UValdorsoAttributeSet::GetSaluteMaxAttribute(), Tipo->Salute);
		AggiungiA(UValdorsoAttributeSet::GetStaminaAttribute(), UValdorsoAttributeSet::GetStaminaMaxAttribute(), Tipo->Stamina);
		AggiungiA(UValdorsoAttributeSet::GetEquilibrioAttribute(), UValdorsoAttributeSet::GetEquilibrioMaxAttribute(), Tipo->Equilibrio);
	}
	Cambiato();
	switch (Tipo->Uso)
	{
	case EValdorsoUso::Mangia: Avvisa(FText::Format(LOCTEXT("Mangi", "Mangi: {0}"), Nome), 2.f); break;
	case EValdorsoUso::Bevi: Avvisa(FText::Format(LOCTEXT("Bevi", "Bevi un sorso ({0} rimasti)."), FText::AsNumber(Zaino.Posti[Posto].Cariche)), 2.f); break;
	case EValdorsoUso::Cura: Avvisa(FText::Format(LOCTEXT("Curi", "Ti medichi: {0}"), Nome), 2.f); break;
	default: break;
	}
}

void UValdorsoZainoComponent::ControllaAcqua()
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const ACharacter* Corpo = Controllore ? Cast<ACharacter>(Controllore->GetPawn()) : nullptr;
	const UValdorsoMovementComponent* Movimento = Corpo ? Cast<UValdorsoMovementComponent>(Corpo->GetCharacterMovement()) : nullptr;
	if (!Movimento)
	{
		return;
	}
	if ((Movimento->IsSwimming() || Movimento->ImmersioneNellaValle() >= ZainoAcquaPerBorraccia) && ValdorsoZaino::RiempiBorracce(Zaino))
	{
		Cambiato();
		Avvisa(LOCTEXT("Riempita", "Riempi la borraccia."), 2.5f);
	}
}

void UValdorsoZainoComponent::ApplicaPeso()
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const ACharacter* Corpo = Controllore ? Cast<ACharacter>(Controllore->GetPawn()) : nullptr;
	UValdorsoMovementComponent* Movimento = Corpo ? Cast<UValdorsoMovementComponent>(Corpo->GetCharacterMovement()) : nullptr;
	if (!Movimento)
	{
		return;
	}
	const float Kg = Peso();
	Movimento->FattoreCarico = ValdorsoOggetti::FattoreVelocita(Kg);
	Movimento->bCaricoPermetteCorsa = ValdorsoOggetti::PuoCorrere(Kg);
}

void UValdorsoZainoComponent::OnRep_Zaino()
{
	++NumeroCambiamenti;
	ApplicaPeso();
}

// ------------------------------------------------------------------------------------------------- lo schermo

namespace ValdorsoZainoSchermo
{
	bool Aperto(const APlayerController* Controllore)
	{
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		return Zaino && Zaino->Schermata.IsValid();
	}

	bool Apri(APlayerController* Controllore)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		UWorld* Mondo = Controllore ? Controllore->GetWorld() : nullptr;
		UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
		if (!Zaino || !Viewport || !Controllore->IsLocalController() || !Controllore->GetPawn() || Zaino->Schermata.IsValid()
			|| ValdorsoFoto::Aperta(Controllore) || ValdorsoTaccuino::Aperto(Controllore))
		{
			return false;
		}
		TWeakObjectPtr<APlayerController> Debole(Controllore);
		SAssignNew(Zaino->Schermata, SValdorsoZaino)
			.Controllore(Controllore)
			.Zaino(Zaino)
			.OnChiudi(FSimpleDelegate::CreateLambda([Debole]()
			{
				if (APlayerController* Chi = Debole.Get())
				{
					Chiudi(Chi);
				}
			}));
		// come il taccuino (57): sopra le scritte del gioco, sotto la pausa
		Viewport->AddViewportWidgetContent(Zaino->Schermata.ToSharedRef(), 57);
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Zaino->Schermata);
		Controllore->SetInputMode(Modo);
		Controllore->bShowMouseCursor = true;
		return true;
	}

	void Chiudi(APlayerController* Controllore)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		if (!Zaino || !Zaino->Schermata.IsValid())
		{
			return;
		}
		const TSharedRef<SValdorsoZaino> Schermo = Zaino->Schermata.ToSharedRef();
		Zaino->Schermata.Reset();
		UWorld* Mondo = Controllore->GetWorld();
		if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Schermo);
		}
		// lo zaino si apre solo quando non c'è nient'altro: chiudendolo si torna al gioco
		Controllore->SetInputMode(FInputModeGameOnly());
		Controllore->bShowMouseCursor = false;
	}

	void Alterna(APlayerController* Controllore)
	{
		if (Aperto(Controllore))
		{
			Chiudi(Controllore);
		}
		else
		{
			Apri(Controllore);
		}
	}
}

#undef LOCTEXT_NAMESPACE
