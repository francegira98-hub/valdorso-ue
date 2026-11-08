// Valdorso - Le barre e la barra delle azioni (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoBarre.h"
#include "SValdorsoBarre.h"
#include "ValdorsoModoFoto.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoZaino.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

namespace ValdorsoBarre
{
	float Frazione(float Valore, float Massimo)
	{
		if (Massimo <= KINDA_SMALL_NUMBER)
		{
			return 1.f;
		}
		return FMath::Clamp(Valore / Massimo, 0.f, 1.f);
	}

	bool Piena(float InFrazione)
	{
		return InFrazione >= 0.999f;
	}

	bool DaMostrare(const FValdorsoStatoBarra& Stato, double Adesso)
	{
		if (Stato.Frazione < 0.f)
		{
			return false;
		}
		return !Piena(Stato.Frazione) || (Adesso - Stato.UltimoCambio) < SecondiDopoPiena;
	}

	void Aggiorna(FValdorsoStatoBarra& Stato, float Valore, float Massimo, double Adesso, float Delta)
	{
		const float Nuova = Frazione(Valore, Massimo);
		if (Stato.Frazione < 0.f)
		{
			// la prima lettura: la barra si fa vedere un momento (anche piena), poi sfuma
			Stato.Scia = Nuova;
			Stato.UltimoCambio = Adesso;
		}
		else if (!FMath::IsNearlyEqual(Nuova, Stato.Frazione, 0.0005f) || !Piena(Nuova))
		{
			Stato.UltimoCambio = Adesso;
		}
		Stato.Frazione = Nuova;
		// la scia: sale subito, scende piano
		Stato.Scia = Nuova >= Stato.Scia ? Nuova : FMath::FInterpConstantTo(Stato.Scia, Nuova, Delta, VelocitaScia);
		const float Voluta = DaMostrare(Stato, Adesso) ? 1.f : 0.f;
		Stato.Presenza = FMath::FInterpConstantTo(Stato.Presenza, Voluta, Delta, 1.f / SecondiSfumatura);
	}

	FString TestoNumeri(float Valore, float Massimo)
	{
		return FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(FMath::Max(0.f, Valore)), FMath::RoundToInt(FMath::Max(0.f, Massimo)));
	}

	bool SchermoLibero(const APlayerController* Controllore)
	{
		// una schermata aperta (pausa, mappa, bacheca, dialogo, registro...) mostra il mouse: allora via
		return Controllore && Controllore->GetPawn() && !Controllore->bShowMouseCursor
			&& !ValdorsoFoto::Aperta(Controllore) && !ValdorsoTaccuino::Aperto(Controllore) && !ValdorsoZainoSchermo::Aperto(Controllore);
	}
}

// ------------------------------------------------------------------------------------------------- il componente

UValdorsoBarreComponent::UValdorsoBarreComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	Azioni.Init(NAME_None, ValdorsoBarre::NumeroAzioni);
}

UValdorsoBarreComponent* UValdorsoBarreComponent::Di(const APlayerController* Controllore)
{
	return Controllore ? Controllore->FindComponentByClass<UValdorsoBarreComponent>() : nullptr;
}

void UValdorsoBarreComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// le barre si mettono quando c'è lo schermo (sul server dedicato mai)
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (!Hud.IsValid() && Controllore && Controllore->IsLocalController())
	{
		MettiSchermo();
	}
}

void UValdorsoBarreComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TogliSchermo();
	Super::EndPlay(EndPlayReason);
}

void UValdorsoBarreComponent::PremiAzione(int32 Indice)
{
	if (Indice < 0 || Indice >= ValdorsoBarre::NumeroAzioni)
	{
		return;
	}
	const FName Scelta = Azione(Indice);
	if (BarraAzioni.IsValid())
	{
		BarraAzioni->Premi(Indice, Scelta.IsNone());
	}
	// (08/11) un oggetto dello zaino: lo usa il server (che controlla se c'è ancora, se si può...)
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
	if (Zaino && !Scelta.IsNone() && Zaino->Contenuto().Azioni.IsValidIndex(Indice) && Zaino->Contenuto().Azioni[Indice] == Scelta)
	{
		Zaino->ServerUsaAzione(Indice);
	}
}

FName UValdorsoBarreComponent::Azione(int32 Indice) const
{
	const UValdorsoZainoComponent* Zaino = ZainoDelGiocatore();
	if (Zaino && Zaino->Contenuto().Azioni.IsValidIndex(Indice) && !Zaino->Contenuto().Azioni[Indice].IsNone())
	{
		return Zaino->Contenuto().Azioni[Indice];
	}
	return Azioni.IsValidIndex(Indice) ? Azioni[Indice] : NAME_None;
}

const UValdorsoZainoComponent* UValdorsoBarreComponent::ZainoDelGiocatore() const
{
	return UValdorsoZainoComponent::Di(Cast<APlayerController>(GetOwner()));
}

void UValdorsoBarreComponent::MettiSchermo()
{
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	UWorld* Mondo = GetWorld();
	UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
	if (!Controllore || !Viewport)
	{
		return;
	}
	Hud = SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 12.f))
		.Visibility(EVisibility::HitTestInvisible)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
			[
				SAssignNew(BarraAzioni, SValdorsoBarraAzioni)
				.Controllore(Controllore)
				.Barre(this)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SValdorsoBarre)
				.Controllore(Controllore)
			]
		];
	// come la bussola (35): sotto il suggerimento (40), l'avviso (45) e tutte le schermate
	Viewport->AddViewportWidgetContent(Hud.ToSharedRef(), 34);
	// il velo della caduta: sopra le scritte del gioco, sotto la pausa (60)
	Velo = SNew(SValdorsoVeloCaduta).Controllore(Controllore);
	Viewport->AddViewportWidgetContent(Velo.ToSharedRef(), 50);
}

void UValdorsoBarreComponent::TogliSchermo()
{
	UWorld* Mondo = GetWorld();
	UGameViewportClient* Viewport = Mondo ? Mondo->GetGameViewport() : nullptr;
	if (Viewport && Hud.IsValid())
	{
		Viewport->RemoveViewportWidgetContent(Hud.ToSharedRef());
	}
	if (Viewport && Velo.IsValid())
	{
		Viewport->RemoveViewportWidgetContent(Velo.ToSharedRef());
	}
	Hud.Reset();
	Velo.Reset();
	BarraAzioni.Reset();
}
