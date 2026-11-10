// Valdorso - La chat di prossimità (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoChat.h"
#include "ValdorsoGilda.h"
#include "SValdorsoChat.h"
#include "ValdorsoBarre.h"
#include "ValdorsoModoFoto.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoRegistroEventi.h"
#include "Valdorso.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/PlatformTime.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "ValdorsoChat"

namespace ValdorsoChat
{
	namespace
	{
		/** Spazi di ogni genere (diventano uno spazio solo). */
		bool ChatSpazio(uint32 Codice)
		{
			return Codice == ' ' || Codice == '\t' || Codice == '\n' || Codice == '\r' || Codice == 0x00A0 || Codice == 0x1680
				|| (Codice >= 0x2000 && Codice <= 0x200A) || Codice == 0x2028 || Codice == 0x2029 || Codice == 0x202F
				|| Codice == 0x205F || Codice == 0x3000;
		}

		/** Caratteri da togliere: di controllo, invisibili, quelli che girano il verso del testo. */
		bool ChatVietato(uint32 Codice)
		{
			return Codice < 0x20 || (Codice >= 0x7F && Codice <= 0x9F) || (Codice >= 0x200B && Codice <= 0x200F)
				|| (Codice >= 0x202A && Codice <= 0x202E) || (Codice >= 0x2060 && Codice <= 0x206F) || Codice == 0xFEFF
				|| (Codice >= 0xFFF9 && Codice <= 0xFFFB);
		}
	}

	FString Pulisci(const FString& Testo)
	{
		FString Fuori;
		Fuori.Reserve(FMath::Min(Testo.Len(), LunghezzaMassima + 1));
		bool bSpazio = false;
		for (const TCHAR Carattere : Testo)
		{
			const uint32 Codice = static_cast<uint32>(Carattere);
			if (ChatSpazio(Codice))
			{
				bSpazio = true;
				continue;
			}
			if (ChatVietato(Codice))
			{
				continue;
			}
			// uno spazio solo tra due parole (mai all'inizio)
			if (bSpazio && !Fuori.IsEmpty())
			{
				if (Fuori.Len() + 1 >= LunghezzaMassima)
				{
					break;
				}
				Fuori.AppendChar(TEXT(' '));
			}
			bSpazio = false;
			Fuori.AppendChar(Carattere);
			if (Fuori.Len() >= LunghezzaMassima)
			{
				break;
			}
		}
		// non lasciare mezzo carattere (una coppia UTF-16 tagliata a metà) alla fine
		if (Fuori.Len() > 0)
		{
			const uint32 Ultimo = static_cast<uint32>(Fuori[Fuori.Len() - 1]);
			if (Ultimo >= 0xD800 && Ultimo <= 0xDBFF)
			{
				Fuori = Fuori.LeftChop(1);
			}
		}
		return Fuori;
	}

	bool TogliGrido(FString& InOutTesto)
	{
		static const FString Comando(TEXT("/grida"));
		if (!InOutTesto.StartsWith(Comando, ESearchCase::IgnoreCase))
		{
			return false;
		}
		// "/grida" da solo o seguito da uno spazio (non "/gridare")
		if (InOutTesto.Len() > Comando.Len() && InOutTesto[Comando.Len()] != TEXT(' '))
		{
			return false;
		}
		InOutTesto = InOutTesto.Mid(Comando.Len()).TrimStartAndEnd();
		return true;
	}

	bool PuoParlare(double Ultima, double Adesso, double Intervallo)
	{
		return Adesso - Ultima >= Intervallo;
	}

	bool Sente(const FVector& Da, const FVector& A, bool bGrida)
	{
		const double Raggio = bGrida ? RaggioGrido : RaggioVoce;
		return FVector::DistSquared(Da, A) <= Raggio * Raggio;
	}

	float Presenza(double Eta)
	{
		if (Eta <= SecondiVisibile)
		{
			return 1.f;
		}
		return static_cast<float>(FMath::Clamp(1.0 - (Eta - SecondiVisibile) / SecondiSfumatura, 0.0, 1.0));
	}

	FString Riga(const FValdorsoFraseChat& Frase)
	{
		if (Frase.Nome.IsEmpty())
		{
			return Frase.Testo;
		}
		return Frase.bGrida ? FString::Printf(TEXT("%s grida: %s"), *Frase.Nome, *Frase.Testo)
			: FString::Printf(TEXT("%s: %s"), *Frase.Nome, *Frase.Testo);
	}

	void Aggiungi(TArray<FValdorsoFraseChat>& Elenco, const FValdorsoFraseChat& Frase)
	{
		Elenco.Add(Frase);
		if (Elenco.Num() > FrasiTenute)
		{
			Elenco.RemoveAt(0, Elenco.Num() - FrasiTenute);
		}
	}
}

// ------------------------------------------------------------------------------------------------- il componente

UValdorsoChatComponent::UValdorsoChatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	SetIsReplicatedByDefault(true);
}

UValdorsoChatComponent* UValdorsoChatComponent::Di(const APlayerController* Controllore)
{
	return Controllore ? Controllore->FindComponentByClass<UValdorsoChatComponent>() : nullptr;
}

void UValdorsoChatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// le frasi si leggono quando c'è lo schermo (sul server dedicato mai)
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (!Pannello.IsValid() && Controllore && Controllore->IsLocalController())
	{
		MettiPannello();
	}
}

void UValdorsoChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// il mondo se ne va: via la riga e le frasi dallo schermo (il modo dei tasti lo rimette chi resta)
	UWorld* Mondo = GetWorld();
	if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
	{
		if (RigaContenitore.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(RigaContenitore.ToSharedRef());
		}
	}
	Riga.Reset();
	RigaContenitore.Reset();
	TogliPannello();
	Super::EndPlay(EndPlayReason);
}

// --- Sul PC del giocatore ----------------------------------------------------------------------------

bool UValdorsoChatComponent::ApriRiga()
{
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	UWorld* Mondo = GetWorld();
	UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
	if (!Controllore || !Viewport || Riga.IsValid() || !Controllore->IsLocalController() || !ValdorsoBarre::SchermoLibero(Controllore))
	{
		return false;
	}
	TWeakObjectPtr<UValdorsoChatComponent> Debole(this);
	SAssignNew(Riga, SValdorsoRigaChat)
		.OnInvia(FValdorsoSuFraseChat::CreateLambda([Debole](const FString& Scritto)
		{
			if (UValdorsoChatComponent* Componente = Debole.Get())
			{
				Componente->Invia(Scritto);
				Componente->ChiudiRiga();
			}
		}))
		.OnAnnulla(FSimpleDelegate::CreateLambda([Debole]()
		{
			if (UValdorsoChatComponent* Componente = Debole.Get())
			{
				Componente->ChiudiRiga();
			}
		}));
	RigaContenitore = SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(24.f, 0.f, 0.f, 24.f))
		[
			Riga.ToSharedRef()
		];
	// sopra le frasi e l'avviso, sotto le schermate vere
	Viewport->AddViewportWidgetContent(RigaContenitore.ToSharedRef(), 46);

	// I tasti vanno solo alla riga: il colono si ferma (anche se si teneva premuto un tasto per camminare).
	Controllore->FlushPressedKeys();
	FInputModeUIOnly Modo;
	Modo.SetWidgetToFocus(Riga->Campo());
	Modo.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	Controllore->SetInputMode(Modo);
	++NumeroCambiamenti;
	return true;
}

void UValdorsoChatComponent::ChiudiRiga()
{
	if (!Riga.IsValid())
	{
		return;
	}
	UWorld* Mondo = GetWorld();
	if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
	{
		if (RigaContenitore.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(RigaContenitore.ToSharedRef());
		}
	}
	Riga.Reset();
	RigaContenitore.Reset();
	++NumeroCambiamenti;
	// Si torna al gioco, ma solo se nel frattempo non si è aperta un'altra schermata (che tiene lei i tasti).
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (Controllore && !Controllore->bShowMouseCursor && !ValdorsoFoto::Aperta(Controllore) && !ValdorsoTaccuino::Aperto(Controllore))
	{
		Controllore->SetInputMode(FInputModeGameOnly());
	}
}

void UValdorsoChatComponent::Invia(const FString& Testo)
{
	const FString Pulito = ValdorsoChat::Pulisci(Testo);
	if (Pulito.IsEmpty())
	{
		return;
	}
	const double Adesso = FPlatformTime::Seconds();
	if (!ValdorsoChat::PuoParlare(UltimaInviata, Adesso))
	{
		AggiungiFrase(FString(), LOCTEXT("Piano", "Una frase alla volta: aspetta un istante.").ToString(), false);
		return;
	}
	UltimaInviata = Adesso;
	ServerDici(Pulito);
}

void UValdorsoChatComponent::AggiungiFrase(const FString& Nome, const FString& Testo, bool bGrida)
{
	FValdorsoFraseChat Frase;
	Frase.Nome = Nome;
	Frase.Testo = Testo;
	Frase.bGrida = bGrida;
	Frase.Quando = FPlatformTime::Seconds();
	ValdorsoChat::Aggiungi(Elenco, Frase);
	++NumeroCambiamenti;
}

void UValdorsoChatComponent::MettiPannello()
{
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	UWorld* Mondo = GetWorld();
	UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
	if (!Controllore || !Viewport)
	{
		return;
	}
	Pannello = SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(24.f, 0.f, 0.f, 78.f))
		.Visibility(EVisibility::HitTestInvisible)
		[
			SNew(SValdorsoPannelloChat)
			.Controllore(Controllore)
			.Chat(this)
		];
	// come la bussola e le barre: sotto il suggerimento (40), l'avviso (45) e tutte le schermate
	Viewport->AddViewportWidgetContent(Pannello.ToSharedRef(), 36);
}

void UValdorsoChatComponent::TogliPannello()
{
	if (!Pannello.IsValid())
	{
		return;
	}
	UWorld* Mondo = GetWorld();
	if (UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(Pannello.ToSharedRef());
	}
	Pannello.Reset();
}

void UValdorsoChatComponent::ClientSenti_Implementation(const FString& Nome, const FString& Testo, bool bGrida)
{
	// anche qui si pulisce (non si sa mai chi ha scritto il server)
	AggiungiFrase(ValdorsoChat::Pulisci(Nome), ValdorsoChat::Pulisci(Testo), bGrida);
}

// --- Sul server ----------------------------------------------------------------------------------------

void UValdorsoChatComponent::ServerDici_Implementation(const FString& Testo)
{
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
	UWorld* Mondo = GetWorld();
	// solo chi è nella valle con il suo colono
	if (!Corpo || !Mondo)
	{
		return;
	}
	// al massimo una frase al secondo (chi ne manda di più le perde)
	const double Adesso = FPlatformTime::Seconds();
	if (!ValdorsoChat::PuoParlare(UltimaAccettata, Adesso, ValdorsoChat::SecondiTraFrasiServer))
	{
		return;
	}
	FString Pulito = ValdorsoChat::Pulisci(Testo);
	// (10/11) le righe dello staff per le bacheche (/proclama, /incarico...): le legge solo il server (ValdorsoGilda.h)
	if (Pulito.StartsWith(TEXT("/")))
	{
		FString Risposta;
		UValdorsoBacheche* Bacheche = UValdorsoBacheche::Di(this);
		if (Bacheche && Bacheche->ComandoStaff(Controllore, Pulito, Risposta))
		{
			UltimaAccettata = Adesso;
			ClientSenti(FString(), Risposta, false);
			return;
		}
	}
	const bool bGrida = ValdorsoChat::TogliGrido(Pulito);
	if (Pulito.IsEmpty())
	{
		return;
	}
	UltimaAccettata = Adesso;

	FString Nome = Controllore->PlayerState ? ValdorsoChat::Pulisci(Controllore->PlayerState->GetPlayerName()) : FString();
	if (Nome.IsEmpty())
	{
		Nome = LOCTEXT("Colono", "Un colono").ToString();
	}
	const FVector Dove = Corpo->GetActorLocation();
	int32 Sentito = 0;
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Altro = It->Get();
		const APawn* SuoCorpo = Altro ? Altro->GetPawn() : nullptr;
		UValdorsoChatComponent* SuaChat = Di(Altro);
		if (!SuoCorpo || !SuaChat || !ValdorsoChat::Sente(Dove, SuoCorpo->GetActorLocation(), bGrida))
		{
			continue;
		}
		SuaChat->ClientSenti(Nome, Pulito, bGrida);
		++Sentito;
	}
	ValdorsoRegistroEventi::Annota(TEXT("Chat"), Corpo, nullptr, Dove,
		FString::Printf(TEXT("%s%s (sentito da %d)"), bGrida ? TEXT("grida: ") : TEXT(""), *Pulito, Sentito));
}

#undef LOCTEXT_NAMESPACE
