// Valdorso - Crescere nel mondo (vedi ValdorsoCrescita.h). Scritto da Claude il 10/11/2026.

#include "ValdorsoCrescita.h"
#include "SValdorsoScheda.h"
#include "Valdorso.h"
#include "ValdorsoCombattimento.h"
#include "ValdorsoLupi.h"
#include "ValdorsoPNG.h"
#include "ValdorsoMovementComponent.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistro.h"
#include "ValdorsoRegistroEventi.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "ValdorsoCrescita"

namespace
{
	TAutoConsoleVariable<int32> CVarCrescereCritico(
		TEXT("Valdorso.Crescere.Critico"),
		0,
		TEXT("Per le prove: 1 = ogni momento e' critico (le abilita' cariche si evolvono subito)."),
		ECVF_Cheat);

	/** Dopo quanto dall'ultimo cambiamento si salva (s). */
	constexpr double CrescitaSecondiSalvataggio = 60.0;
	/** Una richiesta dal PC ogni tanto (s). */
	constexpr double CrescitaSecondiRichieste = 0.2;
	/** Ogni quanti metri di corsa e di nuoto si conta un uso. */
	constexpr float CrescitaMetriCorsa = 10.f;
	constexpr float CrescitaMetriNuoto = 8.f;
	/** Oltre questa velocita' (cm/s) si corre. */
	constexpr float CrescitaVelocitaCorsa = 450.f;
	/** Per quanto conta lo stesso bersaglio di fila (s). */
	constexpr double CrescitaSecondiRipetizione = 60.0;

	/** Su ogni colono con autorita' del mondo (o di quello del Play), con quel nome (vuoto: tutti). */
	int32 CrescitaPerOgniGiocatore(UWorld* Mondo, const FString& Nome, TFunctionRef<void(UValdorsoCrescitaComponent&, const FString&)> Fai)
	{
		auto NelMondo = [&Nome, &Fai](UWorld* W)
		{
			int32 Trovati = 0;
			for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* Controllore = It->Get();
				UValdorsoCrescitaComponent* Crescita = UValdorsoCrescitaComponent::Di(Controllore);
				if (!Crescita || !Controllore->HasAuthority())
				{
					continue;
				}
				const APlayerState* Stato = Controllore->PlayerState;
				const FString SuoNome = Stato ? Stato->GetPlayerName() : FString();
				if (!Nome.IsEmpty() && !SuoNome.Equals(Nome, ESearchCase::IgnoreCase))
				{
					continue;
				}
				Fai(*Crescita, SuoNome);
				++Trovati;
			}
			return Trovati;
		};
		int32 Trovati = Mondo ? NelMondo(Mondo) : 0;
		if (Trovati > 0 || !GEngine)
		{
			return Trovati;
		}
		for (const FWorldContext& Contesto : GEngine->GetWorldContexts())
		{
			UWorld* Gioco = Contesto.World();
			if (Gioco && Gioco != Mondo && (Contesto.WorldType == EWorldType::PIE || Contesto.WorldType == EWorldType::Game))
			{
				Trovati += NelMondo(Gioco);
			}
		}
		return Trovati;
	}

	void CrescitaEsito(int32 Trovati, const FString& Cosa)
	{
		UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Crescere: %s"), Trovati > 0 ? *FString::Printf(TEXT("%s (%d giocatori)"), *Cosa, Trovati)
			: TEXT("nessun giocatore (il comando vale sul server, o giocando da soli)"));
	}

	FAutoConsoleCommandWithWorldAndArgs ComandoCrescereImposta(
		TEXT("Valdorso.Crescere.Imposta"),
		TEXT("Per le prove: Valdorso.Crescere.Imposta <Abilita> <punti 0..100> [nome] (per esempio Atletica 24)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			if (Argomenti.Num() < 2)
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Crescere: Valdorso.Crescere.Imposta <Abilita> <punti> [nome]"));
				return;
			}
			const FName Id(*Argomenti[0]);
			const float Nuovi = FCString::Atof(*Argomenti[1]);
			const FString Nome = Argomenti.Num() > 2 ? Argomenti[2] : FString();
			CrescitaEsito(CrescitaPerOgniGiocatore(Mondo, Nome, [Id, Nuovi](UValdorsoCrescitaComponent& C, const FString&)
			{
				C.ImpostaPunti(Id, Nuovi, TEXT("console"));
			}), FString::Printf(TEXT("%s a %.1f"), *Id.ToString(), Nuovi));
		}),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ComandoCrescereElenco(
		TEXT("Valdorso.Crescere.Elenco"),
		TEXT("Le abilita' dei coloni (o di quello col nome): Valdorso.Crescere.Elenco [nome]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			const FString Nome = Argomenti.Num() > 0 ? Argomenti[0] : FString();
			CrescitaEsito(CrescitaPerOgniGiocatore(Mondo, Nome, [](UValdorsoCrescitaComponent& C, const FString& SuoNome)
			{
				const FValdorsoAbilitaColono& Tutto = C.Contenuto();
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Crescere: %s, totale %.1f / %.0f"), *SuoNome, ValdorsoCrescere::Totale(Tutto), ValdorsoCrescere::Tetto);
				for (const FValdorsoAbilitaValore& V : Tutto.Valori)
				{
					UE_LOG(LogValdorso, Display, TEXT("    %s (%s): %.2f, evoluzioni %d, carica %.0f%%, verso %d"), *V.Id.ToString(),
						*ValdorsoCrescere::Nome(V.Id, V.Evoluzioni), V.Punti, V.Evoluzioni, V.Carica * 100.f, static_cast<int32>(V.Verso));
				}
			}), TEXT("elenco"));
		}));
}

UValdorsoCrescitaComponent::UValdorsoCrescitaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	SetIsReplicatedByDefault(true);
}

UValdorsoCrescitaComponent* UValdorsoCrescitaComponent::Di(const APlayerController* Controllore)
{
	return Controllore ? Controllore->FindComponentByClass<UValdorsoCrescitaComponent>() : nullptr;
}

UValdorsoCrescitaComponent* UValdorsoCrescitaComponent::DelCorpo(const AActor* Corpo)
{
	const APawn* Pedina = Cast<APawn>(Corpo);
	return Pedina ? Di(Cast<APlayerController>(Pedina->GetController())) : nullptr;
}

void UValdorsoCrescitaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UValdorsoCrescitaComponent, Abilita, COND_OwnerOnly);
}

void UValdorsoCrescitaComponent::OnRep_Abilita()
{
	++NumeroCambiamenti;
}

void UValdorsoCrescitaComponent::Carica(const FValdorsoAbilitaColono& Salvato, const FValdorsoRegistro* Registro)
{
	Abilita = Salvato;
	ValdorsoCrescere::Sistema(Abilita);
	// la prima volta: i 5 punti del vecchio mestiere (e del richiamo della magia), dal Registro firmato
	if (Registro && Registro->bFirmato && ValdorsoCrescere::DaiVantaggi(Abilita, *Registro))
	{
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Crescere: i vantaggi del Registro (mestiere %s)"), *Registro->Mestiere);
	}
	bPersonaggio = true;
	bPrima = false;
	++NumeroCambiamenti;
}

void UValdorsoCrescitaComponent::Avvisa(const FText& Testo, float Secondi) const
{
	if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(GetOwner()))
	{
		Controllore->ClientAvviso(Testo, Secondi, 0.f);
	}
}

void UValdorsoCrescitaComponent::Cambiato()
{
	++NumeroCambiamenti;
	if (bPersonaggio && !bDaSalvare)
	{
		bDaSalvare = true;
		const UWorld* Mondo = GetWorld();
		SalvaDopo = (Mondo ? Mondo->GetTimeSeconds() : 0.0) + CrescitaSecondiSalvataggio;
	}
}

bool UValdorsoCrescitaComponent::MomentoCritico() const
{
	if (CVarCrescereCritico.GetValueOnGameThread() != 0)
	{
		return true;
	}
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	float Salute = 0.f;
	float Massima = 0.f;
	return Controllore && ValdorsoCombattimento::Salute(Controllore->GetPawn(), Salute, Massima) && Massima > 0.f && Salute / Massima < 0.4f;
}

bool UValdorsoCrescitaComponent::ForzaDi(const AActor* Chi, float& OutForza)
{
	if (const AValdorsoAnimale* Bestia = Cast<AValdorsoAnimale>(Chi))
	{
		const FValdorsoSpecie* S = Bestia->GetSpecie();
		OutForza = ValdorsoCrescere::ForzaSpecie(S ? FName(S->Id) : NAME_None);
		return true;
	}
	if (const AValdorsoPNG* Abitante = Cast<AValdorsoPNG>(Chi))
	{
		OutForza = ValdorsoCrescere::ForzaAbitante(Abitante->Dialogo);
		return true;
	}
	if (const UValdorsoCrescitaComponent* Suo = DelCorpo(Chi); Suo && Suo->bPersonaggio)
	{
		OutForza = ValdorsoCrescere::Forza(Suo->Abilita);
		return true;
	}
	// sul PC: le abilita' arrivano solo al loro colono (bPersonaggio vale solo sul server)
	if (const UValdorsoCrescitaComponent* Mio = DelCorpo(Chi); Mio && Mio->NumeroCambiamenti > 0 && Mio->GetOwner() && !Mio->GetOwner()->HasAuthority())
	{
		OutForza = ValdorsoCrescere::Forza(Mio->Abilita);
		return true;
	}
	return false;
}

void UValdorsoCrescitaComponent::Usa(FName Id, float Difficolta, const UObject* Bersaglio, bool bPericolo)
{
	const AActor* Proprietario = GetOwner();
	const UWorld* Mondo = GetWorld();
	if (!bPersonaggio || !Proprietario || !Proprietario->HasAuthority() || !Mondo)
	{
		return;
	}
	const double Adesso = Mondo->GetTimeSeconds();
	// lo stesso bersaglio di fila rende sempre meno (per un minuto)
	int32 Volte = 0;
	if (Bersaglio)
	{
		TMap<TWeakObjectPtr<const UObject>, FRipetizione>& DiQuesta = Ripetizioni.FindOrAdd(Id);
		// via i bersagli vecchi (o spariti)
		if (DiQuesta.Num() > 16)
		{
			for (auto It = DiQuesta.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid() || Adesso >= It.Value().Fino)
				{
					It.RemoveCurrent();
				}
			}
		}
		FRipetizione& R = DiQuesta.FindOrAdd(TWeakObjectPtr<const UObject>(Bersaglio));
		R.Volte = Adesso < R.Fino ? R.Volte + 1 : 0;
		R.Fino = Adesso + CrescitaSecondiRipetizione;
		Volte = R.Volte;
	}
	const FValdorsoEsitoUso Risultato = ValdorsoCrescere::Usa(Abilita, Id, Difficolta, Volte, bPericolo || MomentoCritico());
	if (Risultato.Guadagno > 0.f || Risultato.bCaricata || Risultato.bPronta || Risultato.bEvoluta || !Risultato.Calata.IsNone())
	{
		Cambiato();
	}
	const FValdorsoAbilitaValore* V = ValdorsoCrescere::Valore(Abilita, Id);
	if (Risultato.bPronta && !Risultato.bEvoluta && V)
	{
		Avvisa(FText::Format(LOCTEXT("Pronta", "{0} e' carica: crescera' in un momento difficile, quando conta davvero."),
			FText::FromString(ValdorsoCrescere::Nome(Id, V->Evoluzioni))), 5.f);
	}
	if (Risultato.bEvoluta)
	{
		Avvisa(ValdorsoCrescere::Annuncio(Id, Risultato.Evoluzione), 7.f);
		const APlayerController* Controllore = Cast<APlayerController>(Proprietario);
		const APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
		ValdorsoRegistroEventi::Annota(TEXT("Crescere"), Corpo, nullptr, Corpo ? Corpo->GetActorLocation() : FVector::ZeroVector,
			FString::Printf(TEXT("%s si evolve: %s"), *Id.ToString(), *ValdorsoCrescere::Nome(Id, Risultato.Evoluzione)));
	}
	if (Risultato.bTettoPieno && Adesso - UltimoAvvisoTetto > 300.0)
	{
		UltimoAvvisoTetto = Adesso;
		Avvisa(LOCTEXT("Tetto", "Sai fare gia' molte cose (700 punti in tutto). Nella scheda (K) metti a calare un'abilita' per far crescere le altre."), 6.f);
	}
}

void UValdorsoCrescitaComponent::UsaDa(const AActor* Corpo, FName Id, float Difficolta, const UObject* Bersaglio, bool bPericolo)
{
	if (UValdorsoCrescitaComponent* Crescita = DelCorpo(Corpo))
	{
		Crescita->Usa(Id, Difficolta, Bersaglio, bPericolo);
	}
}

bool UValdorsoCrescitaComponent::ImpostaPunti(FName Id, float Punti, const FString& Da)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ValdorsoCrescere::ImpostaPunti(Abilita, Id, Punti))
	{
		return false;
	}
	Cambiato();
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const APawn* Corpo = Controllore ? Controllore->GetPawn() : nullptr;
	ValdorsoRegistroEventi::Annota(TEXT("Staff"), Corpo, nullptr, Corpo ? Corpo->GetActorLocation() : FVector::ZeroVector,
		FString::Printf(TEXT("%s a %.1f (da %s)"), *Id.ToString(), Punti, *Da));
	return true;
}

void UValdorsoCrescitaComponent::ServerVerso_Implementation(FName Id, uint8 Verso)
{
	const UWorld* Mondo = GetWorld();
	const double Adesso = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	if (!bPersonaggio || Verso > static_cast<uint8>(EValdorsoVerso::Scende) || Adesso - UltimaRichiesta < CrescitaSecondiRichieste)
	{
		return;
	}
	UltimaRichiesta = Adesso;
	if (ValdorsoCrescere::ImpostaVerso(Abilita, Id, static_cast<EValdorsoVerso>(Verso)))
	{
		Cambiato();
	}
}

void UValdorsoCrescitaComponent::Movimento()
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	const ACharacter* Corpo = Controllore ? Cast<ACharacter>(Controllore->GetPawn()) : nullptr;
	const UCharacterMovementComponent* Passi = Corpo ? Corpo->GetCharacterMovement() : nullptr;
	if (!Passi)
	{
		bPrima = false;
		return;
	}
	const FVector Qui = Corpo->GetActorLocation();
	if (!bPrima)
	{
		Prima = Qui;
		bPrima = true;
		return;
	}
	const float Tratto = static_cast<float>(FVector::Dist2D(Qui, Prima)) / 100.f;
	Prima = Qui;
	// un salto grande (la rinascita, un teletrasporto dello staff) non conta
	if (Tratto > 20.f)
	{
		return;
	}
	if (Passi->IsSwimming())
	{
		Nuotati += Tratto;
		if (Nuotati >= CrescitaMetriNuoto)
		{
			Nuotati -= CrescitaMetriNuoto;
			// sott'acqua (la testa sotto) vale di piu'
			const UValdorsoMovementComponent* Nostro = Cast<UValdorsoMovementComponent>(Passi);
			Usa(FName(TEXT("Nuoto")), Nostro && Nostro->AcquaSopraLaTesta() > 0.f ? 1.2f : 0.7f);
		}
	}
	else if (Passi->IsMovingOnGround() && Corpo->GetVelocity().Size2D() > CrescitaVelocitaCorsa)
	{
		Corsi += Tratto;
		if (Corsi >= CrescitaMetriCorsa)
		{
			Corsi -= CrescitaMetriCorsa;
			Usa(FName(TEXT("Atletica")), 0.6f);
		}
	}
}

void UValdorsoCrescitaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UWorld* Mondo = GetWorld();
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (!Mondo || !Controllore || !Controllore->HasAuthority() || !bPersonaggio)
	{
		return;
	}
	Movimento();
	if (bDaSalvare && Mondo->GetTimeSeconds() >= SalvaDopo)
	{
		bDaSalvare = false;
		if (AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore))
		{
			Nostro->SalvaPersonaggio();
		}
	}
}

#undef LOCTEXT_NAMESPACE
