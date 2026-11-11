// Valdorso - Il Vigore (vedi ValdorsoVigore.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoVigore.h"
#include "Valdorso.h"
#include "ValdorsoAbilitaDati.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoCrescita.h"
#include "ValdorsoImpostazioni.h"
#include "ValdorsoMovementComponent.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoScherma.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "ValdorsoVigore"

namespace
{
	/** Per i primi secondi dopo essere entrati nel mondo le cadute non contano (si arriva dall'alto). */
	constexpr double VigoreSecondiSenzaCadute = 10.0;
	/** Il Secondo fiato: quanta stamina in piu' torna al secondo quando non si corre. */
	constexpr float VigoreSecondoFiato = 3.3f;
	/** Quanto tempo resta libera la capriola dopo l'atterraggio (s). */
	constexpr double VigoreSecondiCapriola = 0.4;

	float VigorePuntiDi(const AActor* Corpo, FName Abilita)
	{
		const UValdorsoCrescitaComponent* Crescita = UValdorsoCrescitaComponent::DelCorpo(Corpo);
		return Crescita ? ValdorsoCrescere::Punti(Crescita->Contenuto(), Abilita) : 0.f;
	}

	/** Vero se il colono sta correndo davvero (a piedi, piu' svelto della camminata). */
	bool VigoreCorre(const ACharacter* Corpo)
	{
		const UValdorsoMovementComponent* Passi = Corpo ? Cast<UValdorsoMovementComponent>(Corpo->GetCharacterMovement()) : nullptr;
		return Passi && Passi->VuoleCorrere() && Passi->IsMovingOnGround()
			&& Corpo->GetVelocity().Size2D() > Passi->VelocitaCamminata * 1.15f;
	}

	/** Vero se il colono nuota veloce (Maiusc in acqua, e si muove). */
	bool VigoreNuotaVeloce(const ACharacter* Corpo)
	{
		const UValdorsoMovementComponent* Passi = Corpo ? Cast<UValdorsoMovementComponent>(Corpo->GetCharacterMovement()) : nullptr;
		return Passi && Passi->VuoleCorrere() && Passi->IsSwimming() && Corpo->GetVelocity().Size() > 60.f;
	}
}

namespace ValdorsoVigore
{
	float CalaCorsaPer(float PuntiAtletica, int32 EvoluzioniAtletica)
	{
		float Fattore = 1.f - 0.3f * FMath::Clamp(PuntiAtletica, 0.f, 100.f) / 100.f;
		if (EvoluzioniAtletica >= 2)
		{
			Fattore *= 0.75f;
		}
		if (EvoluzioniAtletica >= 4)
		{
			Fattore *= 0.5f;
		}
		return CalaCorsa * Fattore;
	}

	float CalaNuotoPer(int32 EvoluzioniNuoto)
	{
		return CalaNuoto * (EvoluzioniNuoto >= 1 ? 0.6f : 1.f);
	}

	float DannoCaduta(float Velocita, bool bCapriola)
	{
		if (Velocita <= CadutaInnocua)
		{
			return 0.f;
		}
		const float Danno = (Velocita - CadutaInnocua) / CadutaPerPunto;
		return bCapriola ? Danno / 3.f : Danno;
	}

	bool CapriolaPossibile(float Velocita, int32 EvoluzioniAtletica)
	{
		if (EvoluzioniAtletica < 1 || Velocita <= CadutaInnocua * 0.7f)
		{
			return false;
		}
		return Velocita <= (EvoluzioniAtletica >= 2 ? CapriolaFinoEvoluta : CapriolaFino);
	}

	float FattoreFiato(const AActor* Corpo)
	{
		const int32 Nuoto = ValdorsoScherma::Evoluzioni(Corpo, FName(TEXT("Nuoto")));
		if (Nuoto >= 4)
		{
			return 2.f;
		}
		return Nuoto >= 2 ? 1.5f : 1.f;
	}
}

UValdorsoVigoreComponent::UValdorsoVigoreComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;
	SetIsReplicatedByDefault(true);
}

UValdorsoVigoreComponent* UValdorsoVigoreComponent::Di(const AActor* Chi)
{
	return Chi ? Chi->FindComponentByClass<UValdorsoVigoreComponent>() : nullptr;
}

void UValdorsoVigoreComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UValdorsoVigoreComponent, bSenzaFiato, COND_OwnerOnly);
}

void UValdorsoVigoreComponent::BeginPlay()
{
	Super::BeginPlay();
	const UWorld* Mondo = GetWorld();
	NatoAlle = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	if (ACharacter* Corpo = Cast<ACharacter>(GetOwner()))
	{
		Corpo->LandedDelegate.AddDynamic(this, &UValdorsoVigoreComponent::QuandoAtterra);
		bLegato = true;
	}
}

void UValdorsoVigoreComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACharacter* Corpo = Cast<ACharacter>(GetOwner()); Corpo && bLegato)
	{
		Corpo->LandedDelegate.RemoveDynamic(this, &UValdorsoVigoreComponent::QuandoAtterra);
	}
	bLegato = false;
	Super::EndPlay(EndPlayReason);
}

void UValdorsoVigoreComponent::Avvisa(const FText& Testo, float Secondi) const
{
	const APawn* Corpo = Cast<APawn>(GetOwner());
	if (AValdorsoPlayerController* Giocatore = Corpo ? Cast<AValdorsoPlayerController>(Corpo->GetController()) : nullptr)
	{
		Giocatore->ClientAvviso(Testo, Secondi, 0.f);
	}
}

void UValdorsoVigoreComponent::SchivataIniziata()
{
	const UWorld* Mondo = GetWorld();
	InizioSchivata = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	CapriolaLiberaFino = 0.0;
}

bool UValdorsoVigoreComponent::SchivataPronta() const
{
	const UWorld* Mondo = GetWorld();
	const double Ora = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	if (Ora < CapriolaLiberaFino)
	{
		return true;
	}
	// il server e' un po' piu' largo (i due orologi non vanno uguali): mai rifiutare una schivata giusta
	const APawn* Corpo = Cast<APawn>(GetOwner());
	const bool bSoloServer = Corpo && Corpo->HasAuthority() && !Corpo->IsLocallyControlled();
	const double Attesa = bSoloServer ? ValdorsoVigore::SecondiTraSchivate * 0.8 : ValdorsoVigore::SecondiTraSchivate;
	return Ora - InizioSchivata >= Attesa;
}

bool UValdorsoVigoreComponent::Invulnerabile() const
{
	const UWorld* Mondo = GetWorld();
	const double Ora = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	return Ora - InizioSchivata < ValdorsoVigore::SecondiInvulnerabile;
}

void UValdorsoVigoreComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* Corpo = Cast<ACharacter>(GetOwner());
	if (!Corpo)
	{
		return;
	}
	const bool bServer = Corpo->HasAuthority();
	const bool bMio = Corpo->IsLocallyControlled();
	if (!bServer && !bMio)
	{
		// sugli altri PC non serve niente (il tick non si spegne: il Controller puo' arrivare qualche frame dopo)
		return;
	}
	// la caduta: la velocita' piu' forte verso il basso (per quando si tocca terra)
	if (const UCharacterMovementComponent* Passi = Corpo->GetCharacterMovement())
	{
		if (Passi->IsFalling())
		{
			CadutaPiuForte = FMath::Max(CadutaPiuForte, static_cast<float>(-Passi->Velocity.Z));
		}
		else if (!Passi->IsMovingOnGround())
		{
			CadutaPiuForte = 0.f;   // in acqua, sulla scala: la caduta e' finita senza danno
		}
	}
	if (bServer && Corpo->IsPlayerControlled())
	{
		Fatica(Corpo, DeltaTime);
	}
	Movimento(Corpo);
	// la corsa "premi una volta": fermandosi (o senza fiato) si spegne
	if (bMio && ValdorsoImpostazioni::Correnti().bCorsaAlterna)
	{
		if (UValdorsoMovementComponent* Passi = Cast<UValdorsoMovementComponent>(Corpo->GetCharacterMovement()); Passi && Passi->VuoleCorrere())
		{
			Fermo = Corpo->GetVelocity().Size2D() < 20.f ? Fermo + DeltaTime : 0.f;
			if (Fermo > 0.6f || bSenzaFiato)
			{
				Passi->ImpostaCorsa(false);
				Fermo = 0.f;
			}
		}
	}
}

void UValdorsoVigoreComponent::Fatica(ACharacter* Corpo, float DeltaTime)
{
	UAbilitySystemComponent* Statistiche = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Corpo);
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
	if (!Statistiche || !Statistiche->GetSet<UValdorsoAttributeSet>() || (Pericoli && Pericoli->Caduto()))
	{
		return;
	}
	const float Massima = FMath::Max(1.f, Statistiche->GetNumericAttribute(UValdorsoAttributeSet::GetStaminaMaxAttribute()));
	float Stamina = Statistiche->GetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute());
	const bool bCorre = !bSenzaFiato && VigoreCorre(Corpo);
	const bool bNuota = !bSenzaFiato && VigoreNuotaVeloce(Corpo);
	float Cambio = 0.f;
	if (bCorre)
	{
		const float Punti = VigorePuntiDi(Corpo, FName(TEXT("Atletica")));
		const int32 Evolute = ValdorsoScherma::Evoluzioni(Corpo, FName(TEXT("Atletica")));
		Cambio = -(ValdorsoVigore::RitornoDiSerie + ValdorsoVigore::CalaCorsaPer(Punti, Evolute)) * DeltaTime;
	}
	else if (bNuota)
	{
		Cambio = -(ValdorsoVigore::RitornoDiSerie + ValdorsoVigore::CalaNuotoPer(ValdorsoScherma::Evoluzioni(Corpo, FName(TEXT("Nuoto"))))) * DeltaTime;
	}
	else if (Stamina < Massima && ValdorsoScherma::Talento(Corpo, FName(TEXT("Atletica")), 3))
	{
		Cambio = VigoreSecondoFiato * DeltaTime;
	}
	if (Cambio != 0.f)
	{
		Stamina = FMath::Clamp(Stamina + Cambio, 0.f, Massima);
		Statistiche->SetNumericAttributeBase(UValdorsoAttributeSet::GetStaminaAttribute(), Stamina);
	}
	// senza fiato a zero; si riprende a un quarto
	if (!bSenzaFiato && (bCorre || bNuota) && Stamina <= 0.5f)
	{
		bSenzaFiato = true;
		Avvisa(LOCTEXT("SenzaFiato", "Sei senza fiato!"), 2.5f);
	}
	else if (bSenzaFiato && Stamina >= Massima * ValdorsoVigore::RipresaFiato)
	{
		bSenzaFiato = false;
	}
}

void UValdorsoVigoreComponent::Movimento(ACharacter* Corpo)
{
	UValdorsoMovementComponent* Passi = Cast<UValdorsoMovementComponent>(Corpo->GetCharacterMovement());
	if (!Passi)
	{
		return;
	}
	Passi->bFiatoPermetteCorsa = !bSenzaFiato;
	Passi->FattoreNuotoVeloce = ValdorsoScherma::Talento(Corpo, FName(TEXT("Nuoto")), 3) ? 1.6f : 1.35f;
}

void UValdorsoVigoreComponent::QuandoAtterra(const FHitResult& Urto)
{
	ACharacter* Corpo = Cast<ACharacter>(GetOwner());
	const UWorld* Mondo = GetWorld();
	if (!Corpo || !Mondo)
	{
		return;
	}
	const UCharacterMovementComponent* Passi = Corpo->GetCharacterMovement();
	const float Velocita = FMath::Max(CadutaPiuForte, Passi ? static_cast<float>(-Passi->Velocity.Z) : 0.f);
	CadutaPiuForte = 0.f;
	if (Mondo->GetTimeSeconds() - NatoAlle < VigoreSecondiSenzaCadute || !Corpo->IsPlayerControlled())
	{
		return;
	}
	const int32 Evolute = ValdorsoScherma::Evoluzioni(Corpo, FName(TEXT("Atletica")));
	const bool bCapriola = ValdorsoVigore::CapriolaPossibile(Velocita, Evolute);
	if (bCapriola)
	{
		// la capriola: la schivata parte subito, anche se ce n'e' stata una da poco (sul PC del colono la chiede lui)
		CapriolaLiberaFino = Mondo->GetTimeSeconds() + VigoreSecondiCapriola;
		if (Corpo->IsLocallyControlled())
		{
			if (AValdorsoCharacter* Colono = Cast<AValdorsoCharacter>(Corpo))
			{
				Colono->DoSchivata();
			}
		}
	}
	if (!Corpo->HasAuthority())
	{
		return;
	}
	const float Danno = ValdorsoVigore::DannoCaduta(Velocita, bCapriola);
	if (Danno <= 0.5f)
	{
		return;
	}
	if (UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo))
	{
		Pericoli->Ferisci(Danno);
	}
	ValdorsoRegistroEventi::Annota(TEXT("Caduta"), Corpo, nullptr, Corpo->GetActorLocation(),
		FString::Printf(TEXT("caduta a %.0f cm/s, danno %.0f%s"), Velocita, Danno, bCapriola ? TEXT(", con la capriola") : TEXT("")));
	Avvisa(bCapriola ? LOCTEXT("Capriola", "Rotoli e ti rialzi.") : LOCTEXT("Caduta", "Ti sei fatto male cadendo."), 2.f);
	// la caduta fa crescere l'Atletica (di piu' con la capriola: e' cosi' che si impara)
	UValdorsoCrescitaComponent::UsaDa(Corpo, FName(TEXT("Atletica")), bCapriola ? 1.2f : 0.8f, nullptr, Danno > 30.f);
}

#undef LOCTEXT_NAMESPACE
