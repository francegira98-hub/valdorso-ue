// Valdorso - La scherma: le guardie, le tecniche alle soglie e i talenti (vedi ValdorsoScherma.h). Scritto da Claude
// l'11/11/2026.

#include "ValdorsoScherma.h"
#include "Valdorso.h"
#include "ValdorsoAbilitaDati.h"
#include "ValdorsoArmi.h"
#include "ValdorsoAttributeSet.h"
#include "ValdorsoCharacter.h"
#include "ValdorsoCrescita.h"
#include "ValdorsoEffetti.h"
#include "ValdorsoEtichette.h"
#include "ValdorsoLupi.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoScheletri.h"
#include "ValdorsoVigore.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoScherma"

namespace
{
	/** Ogni quanto il PC puo' chiedere di cambiare guardia (s). */
	constexpr double SchermaSecondiRichieste = 0.15;
	/** Il primo colpo di una tecnica arriva dopo questo (s): il tempo dello slancio e del braccio. */
	constexpr float SchermaPrimoColpo = 0.18f;
	/** Quanto dura la risposta della guardia bassa (s). */
	constexpr double SchermaSecondiRisposta = 1.5;

	FValdorsoTecnica SchermaColpo(const TCHAR* Id, const TCHAR* Abilita, int32 Livello, const TCHAR* Nome, const TCHAR* Sigla,
		const TCHAR* Descrizione, float Stamina, float Ricarica)
	{
		FValdorsoTecnica T;
		T.Id = FName(Id);
		T.Abilita = FName(Abilita);
		T.Livello = Livello;
		T.Tipo = EValdorsoTipoTecnica::Colpo;
		T.Nome = Nome;
		T.Sigla = Sigla;
		T.Descrizione = Descrizione;
		T.Stamina = Stamina;
		T.Ricarica = Ricarica;
		return T;
	}

	FValdorsoTecnica SchermaStato(const TCHAR* Id, const TCHAR* Abilita, int32 Livello, const TCHAR* Nome, const TCHAR* Sigla,
		const TCHAR* Descrizione, float Durata, float Stamina, float Ricarica)
	{
		FValdorsoTecnica T = SchermaColpo(Id, Abilita, Livello, Nome, Sigla, Descrizione, Stamina, Ricarica);
		T.Tipo = EValdorsoTipoTecnica::Stato;
		T.Durata = Durata;
		return T;
	}

	FValdorsoTecnica SchermaTalento(const TCHAR* Id, const TCHAR* Abilita, int32 Livello, const TCHAR* Nome, const TCHAR* Descrizione,
		bool bInArrivo = false)
	{
		FValdorsoTecnica T = SchermaColpo(Id, Abilita, Livello, Nome, TEXT(""), Descrizione, 0.f, 0.f);
		T.Tipo = bInArrivo ? EValdorsoTipoTecnica::InArrivo : EValdorsoTipoTecnica::Talento;
		return T;
	}

	TArray<FValdorsoTecnica> SchermaCreaTecniche()
	{
		TArray<FValdorsoTecnica> C;
		// --- Atletica, Nuoto, Tempra: i talenti del corpo (ValdorsoVigore.h, ValdorsoPericoli.h, ValdorsoBagnato.h)
		C.Add(SchermaTalento(TEXT("Tecnica.Capriola"), TEXT("Atletica"), 1, TEXT("La capriola d'atterraggio"),
			TEXT("Cadendo da un'altezza media rotoli invece di schiantarti: ti fai molto meno male e non perdi la corsa.")));
		C.Add(SchermaTalento(TEXT("Tecnica.ScattoLungo"), TEXT("Atletica"), 2, TEXT("Lo scatto lungo"),
			TEXT("Correndo il fiato cala un quarto piu' piano; e la capriola regge cadute piu' alte.")));
		C.Add(SchermaTalento(TEXT("Tecnica.SecondoFiato"), TEXT("Atletica"), 3, TEXT("Il secondo fiato"),
			TEXT("Quando non corri la stamina torna un terzo piu' in fretta.")));
		C.Add(SchermaTalento(TEXT("Tecnica.CorsaSenzaFine"), TEXT("Atletica"), 4, TEXT("La corsa senza fine"),
			TEXT("Correndo il fiato cala la meta'. Il passo dell'Orso non si stanca.")));
		C.Add(SchermaTalento(TEXT("Tecnica.Bracciata"), TEXT("Nuoto"), 1, TEXT("La bracciata leggera"),
			TEXT("Il nuoto veloce (Maiusc in acqua) costa meno fiato.")));
		C.Add(SchermaTalento(TEXT("Tecnica.FiatoLungo"), TEXT("Nuoto"), 2, TEXT("Il fiato lungo"),
			TEXT("Sott'acqua il fiato dura la meta' in piu'.")));
		C.Add(SchermaTalento(TEXT("Tecnica.NuotoVeloce"), TEXT("Nuoto"), 3, TEXT("Il nuoto veloce"),
			TEXT("Il nuoto veloce va piu' forte: come un pesce di fiume.")));
		C.Add(SchermaTalento(TEXT("Tecnica.Abisso"), TEXT("Nuoto"), 4, TEXT("Il respiro dell'abisso"),
			TEXT("Sott'acqua il fiato dura il doppio. L'acqua profonda non fa piu' paura.")));
		C.Add(SchermaTalento(TEXT("Tecnica.PiediPiantati"), TEXT("Tempra"), 1, TEXT("I piedi piantati"),
			TEXT("I colpi ti tolgono un quarto di equilibrio in meno.")));
		C.Add(SchermaTalento(TEXT("Tecnica.PelleDura"), TEXT("Tempra"), 2, TEXT("La pelle dura"),
			TEXT("Morsi e artigli fanno un quarto di male in meno.")));
		C.Add(SchermaTalento(TEXT("Tecnica.Rialzati"), TEXT("Tempra"), 3, TEXT("Ti rialzi prima"),
			TEXT("Quando perdi l'equilibrio barcolli la meta' del tempo.")));
		C.Add(SchermaTalento(TEXT("Tecnica.NonTiPiega"), TEXT("Tempra"), 4, TEXT("Il colpo che non ti piega"),
			TEXT("Una volta al minuto, il colpo che ti farebbe barcollare non ci riesce.")));

		// --- Lotta: a mani nude (e con il solo scudo)
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.Spallata"), TEXT("Lotta"), 1, TEXT("La spallata"), TEXT("Spa"),
				TEXT("Un balzo avanti con la spalla: poco male, ma butta indietro e toglie l'equilibrio."), 15.f, 6.f);
			T.Slancio = 1000.f;
			T.Danno = 0.8f;
			T.Equilibrio = 45.f;
			T.Spinta = 2.f;
			C.Add(T);
		}
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.Presa"), TEXT("Lotta"), 2, TEXT("La presa"), TEXT("Pre"),
				TEXT("Afferri chi hai davanti e lo tieni fermo un istante."), 15.f, 10.f);
			T.Danno = 0.5f;
			T.Stordisce = 1.5f;
			T.Portata = 0.9f;
			C.Add(T);
		}
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.Stordisce"), TEXT("Lotta"), 3, TEXT("Il colpo che stordisce"), TEXT("Sto"),
				TEXT("Un pugno alla tempia: chi lo prende resta fermo per due secondi e mezzo."), 20.f, 14.f);
			T.Danno = 1.2f;
			T.Stordisce = 2.5f;
			C.Add(T);
		}
		C.Add(SchermaTalento(TEXT("Tecnica.ManiFerro"), TEXT("Lotta"), 4, TEXT("Le mani di ferro"),
			TEXT("A mani nude colpisci come con una mazza: il doppio, e piu'.")));

		// --- Pugnali: coltelli, pugnali, falcetti
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.ColpoDoppio"), TEXT("Pugnali"), 1, TEXT("Il colpo doppio"), TEXT("Dop"),
				TEXT("Due colpi svelti, uno dietro l'altro."), 12.f, 5.f);
			T.Colpi = 2;
			T.Intervallo = 0.22f;
			T.Danno = 0.75f;
			C.Add(T);
		}
		C.Add(SchermaTalento(TEXT("Tecnica.AlleSpalle"), TEXT("Pugnali"), 2, TEXT("La lama alle spalle"),
			TEXT("Colpendo da dietro la lama corta fa piu' della meta' in piu'.")));
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.Sanguina"), TEXT("Pugnali"), 3, TEXT("Il taglio che sanguina"), TEXT("San"),
				TEXT("Un taglio lungo: chi lo prende perde sangue per tre secondi."), 14.f, 9.f);
			T.Danno = 0.8f;
			T.Sanguina = 18.f;
			C.Add(T);
		}
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.Serpente"), TEXT("Pugnali"), 4, TEXT("Il morso del serpente"), TEXT("Ser"),
				TEXT("Uno scatto e una punta sola, velocissima: due volte e mezza il colpo."), 25.f, 16.f);
			T.Danno = 2.5f;
			T.Slancio = 1100.f;
			T.Portata = 1.3f;
			C.Add(T);
		}

		// --- Spade: corte, lunghe, il gladio (le guardie della scherma italiana)
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.Affondo"), TEXT("Spade"), 1, TEXT("L'affondo"), TEXT("Aff"),
				TEXT("Un passo lungo e la punta avanti: arriva lontano e fa piu' male."), 18.f, 6.f);
			T.Danno = 1.6f;
			T.Portata = 1.6f;
			T.Slancio = 900.f;
			C.Add(T);
		}
		C.Add(SchermaStato(TEXT("Tecnica.ParataPerfetta"), TEXT("Spade"), 2, TEXT("La parata perfetta"), TEXT("Par"),
			TEXT("Per un attimo la lama aspetta il colpo: il primo che arriva da davanti non passa, e chi l'ha dato perde l'equilibrio."),
			0.7f, 12.f, 7.f));
		C.Add(SchermaTalento(TEXT("Tecnica.MaestroGuardie"), TEXT("Spade"), 3, TEXT("Il maestro di guardie"),
			TEXT("Con la spada le guardie contano una volta e mezzo, e si cambiano in un attimo.")));
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.LamaValle"), TEXT("Spade"), 4, TEXT("La lama della valle"), TEXT("Val"),
				TEXT("Tre colpi in fila, come nel libro: il fendente, il sottano e la punta."), 30.f, 20.f);
			T.Colpi = 3;
			T.Intervallo = 0.35f;
			T.Danno = 1.25f;
			T.Equilibrio = 15.f;
			C.Add(T);
		}

		// --- Asce
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.Spaccascudi"), TEXT("Asce"), 1, TEXT("Il colpo che spacca gli scudi"), TEXT("Spc"),
				TEXT("Dall'alto, con tutto il peso: lo scudo non lo ferma."), 20.f, 8.f);
			T.Danno = 1.4f;
			T.bSopraLoScudo = true;
			T.Equilibrio = 20.f;
			C.Add(T);
		}
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.GiroAscia"), TEXT("Asce"), 2, TEXT("Il giro"), TEXT("Gir"),
				TEXT("L'ascia gira intorno: colpisce tutti quelli che hai vicino."), 22.f, 9.f);
			T.Giro = 230.f;
			C.Add(T);
		}
		C.Add(SchermaStato(TEXT("Tecnica.Furia"), TEXT("Asce"), 3, TEXT("La furia"), TEXT("Fur"),
			TEXT("Per otto secondi colpisci un terzo piu' forte; ma anche i colpi che prendi fanno un po' piu' male."), 8.f, 15.f, 30.f));
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.ScurePasso"), TEXT("Asce"), 4, TEXT("La scure del passo"), TEXT("Scu"),
				TEXT("Il colpo dei boscaioli del passo: tre volte un colpo normale."), 35.f, 30.f);
			T.Danno = 3.f;
			T.Equilibrio = 50.f;
			C.Add(T);
		}

		// --- Mazze: martelli, mazze, clave
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.ButtaATerra"), TEXT("Mazze"), 1, TEXT("Il colpo che butta a terra"), TEXT("Ter"),
				TEXT("Basso, sulle gambe: chi lo prende quasi sempre perde l'equilibrio."), 18.f, 8.f);
			T.Danno = 1.1f;
			T.Equilibrio = 80.f;
			C.Add(T);
		}
		C.Add(SchermaTalento(TEXT("Tecnica.ControArmature"), TEXT("Mazze"), 2, TEXT("Contro le armature"),
			TEXT("Gli scudi di chi colpisci con la mazza fermano la meta'.")));
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.Stordimento"), TEXT("Mazze"), 3, TEXT("Lo stordimento"), TEXT("Std"),
				TEXT("Un colpo sull'elmo (o sulla testa): chi lo prende resta fermo per due secondi e mezzo."), 20.f, 14.f);
			T.Stordisce = 2.5f;
			C.Add(T);
		}
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.Tuono"), TEXT("Mazze"), 4, TEXT("Il tuono"), TEXT("Tuo"),
				TEXT("La mazza batte la terra: tutti intorno perdono l'equilibrio."), 35.f, 25.f);
			T.Giro = 320.f;
			T.Danno = 1.2f;
			T.Equilibrio = 60.f;
			C.Add(T);
		}

		// --- Armi in asta: lance, forconi, bastoni
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.ColpoLungo"), TEXT("Aste"), 1, TEXT("Il colpo lungo"), TEXT("Lun"),
				TEXT("Tutta l'asta avanti: arriva quasi al doppio della distanza."), 14.f, 5.f);
			T.Portata = 1.9f;
			T.Danno = 1.2f;
			C.Add(T);
		}
		C.Add(SchermaTalento(TEXT("Tecnica.FermaCarica"), TEXT("Aste"), 2, TEXT("Fermare la carica"),
			TEXT("Chi ti corre addosso (un cinghiale che carica, un colono che corre) prende la meta' in piu'.")));
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.GiroBastone"), TEXT("Aste"), 3, TEXT("Il giro del bastone"), TEXT("Gib"),
				TEXT("L'asta gira intorno e butta indietro tutti quelli vicini."), 20.f, 9.f);
			T.Giro = 280.f;
			T.Spinta = 2.f;
			T.Danno = 0.9f;
			C.Add(T);
		}
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.LungaOmbra"), TEXT("Aste"), 4, TEXT("La lunga ombra"), TEXT("Omb"),
				TEXT("Un passo e la punta lontanissima: due volte il colpo."), 30.f, 18.f);
			T.Portata = 2.4f;
			T.Slancio = 700.f;
			T.Danno = 2.f;
			C.Add(T);
		}

		// --- Scudo (con lo scudo nella sinistra)
		{
			FValdorsoTecnica T = SchermaColpo(TEXT("Tecnica.SpintaScudo"), TEXT("Scudo"), 1, TEXT("La spinta con lo scudo"), TEXT("Spi"),
				TEXT("Lo scudo avanti, con il peso del corpo: butta indietro e toglie l'equilibrio."), 15.f, 6.f);
			T.bScudo = true;
			T.Danno = 0.6f;
			T.Equilibrio = 50.f;
			T.Spinta = 2.5f;
			T.Slancio = 600.f;
			C.Add(T);
		}
		C.Add(SchermaTalento(TEXT("Tecnica.ScudoRisponde"), TEXT("Scudo"), 2, TEXT("La parata che toglie l'equilibrio"),
			TEXT("Chi colpisce il tuo scudo perde un po' di equilibrio.")));
		C.Add(SchermaTalento(TEXT("Tecnica.Muro"), TEXT("Scudo"), 3, TEXT("Il muro"),
			TEXT("Lo scudo para anche i colpi che arrivano di lato.")));
		{
			FValdorsoTecnica T = SchermaStato(TEXT("Tecnica.Rocca"), TEXT("Scudo"), 4, TEXT("La rocca"), TEXT("Roc"),
				TEXT("Per cinque secondi da davanti non passa quasi niente, e nessuno ti sposta."), 5.f, 25.f, 25.f);
			T.bScudo = true;
			C.Add(T);
		}

		// --- Caccia
		C.Add(SchermaTalento(TEXT("Tecnica.PelliMigliori"), TEXT("Caccia"), 1, TEXT("Le pelli migliori"),
			TEXT("Scuoiando prendi una pelle in piu'.")));
		C.Add(SchermaTalento(TEXT("Tecnica.Tracce"), TEXT("Caccia"), 2, TEXT("Le tracce degli animali"),
			TEXT("Leggere le orme sul terreno (arriva con la v0.2, con la caccia vera)."), true));
		C.Add(SchermaTalento(TEXT("Tecnica.PassoCacciatore"), TEXT("Caccia"), 3, TEXT("Il passo del cacciatore"),
			TEXT("Gli animali ti sentono da piu' vicino: puoi avvicinarti senza farli fuggire.")));
		C.Add(SchermaTalento(TEXT("Tecnica.SignoreBosco"), TEXT("Caccia"), 4, TEXT("Il signore del bosco"),
			TEXT("I lupi non ti attaccano per primi: ti riconoscono.")));
		return C;
	}

	/** Il tempo del server (anche sul PC). */
	double SchermaTempoServer(const UWorld* Mondo)
	{
		if (!Mondo)
		{
			return 0.0;
		}
		const AGameStateBase* Stato = Mondo->GetGameState();
		return Stato ? Stato->GetServerWorldTimeSeconds() : Mondo->GetTimeSeconds();
	}

	/** Il PlayerController di chi guida quel corpo (nullptr per animali, PNG, scheletri). */
	AValdorsoPlayerController* SchermaGiocatore(const AActor* Chi)
	{
		const APawn* Corpo = Cast<APawn>(Chi);
		return Corpo ? Cast<AValdorsoPlayerController>(Corpo->GetController()) : nullptr;
	}

	/** La guardia che vale adesso per quel corpo (falso se non ne ha: animali, o mentre la cambia). */
	bool SchermaGuardiaValida(const AActor* Chi, EValdorsoGuardia& OutGuardia)
	{
		const UValdorsoSchermaComponent* Scherma = UValdorsoSchermaComponent::Di(Chi);
		if (!Scherma || Scherma->InCambio())
		{
			return false;
		}
		OutGuardia = Scherma->GetGuardia();
		return true;
	}

	/** Quanto contano le guardie di chi colpisce: 1,5 al maestro di guardie con la spada in mano. */
	float SchermaForzaGuardie(const AActor* Chi)
	{
		const UValdorsoArmiComponent* Armi = UValdorsoArmiComponent::Di(Chi);
		const FValdorsoTipoOggetto* Arma = Armi ? Armi->Arma() : nullptr;
		if (Arma && ValdorsoCrescere::AbilitaDellArma(Arma->Id) == FName(TEXT("Spade"))
			&& ValdorsoScherma::Talento(Chi, FName(TEXT("Spade")), 3))
		{
			return 1.5f;
		}
		return 1.f;
	}

	/** Un danno piccolo e silenzioso (il sangue): niente reazioni, niente crescita. Rispetta la pace del battito. */
	void SchermaDannoSemplice(AActor* Da, AActor* A, float Danno)
	{
		UAbilitySystemComponent* Suo = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(A);
		if (!A || !Suo || !Suo->GetSet<UValdorsoAttributeSet>() || Danno <= 0.f)
		{
			return;
		}
		float Salute = 0.f;
		float Massima = 1.f;
		if (!ValdorsoCombattimento::Salute(A, Salute, Massima) || Salute <= 0.f)
		{
			return;
		}
		float DannoVero = Danno;
		if (SchermaGiocatore(Da) && SchermaGiocatore(A) && ValdorsoCombattimento::InPace(A->GetWorld(), A->GetActorLocation()))
		{
			bool bTrattenuto = false;
			DannoVero = ValdorsoCombattimento::DannoNellaPace(Danno, Salute, bTrattenuto);
		}
		UAbilitySystemComponent* Fonte = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Da);
		if (!Fonte)
		{
			Fonte = Suo;
		}
		FGameplayEffectContextHandle Contesto = Fonte->MakeEffectContext();
		Contesto.AddInstigator(Da, Da);
		FGameplayEffectSpecHandle Effetto = Fonte->MakeOutgoingSpec(UValdorsoGE_Colpo::StaticClass(), 1.f, Contesto);
		if (Effetto.IsValid())
		{
			Effetto.Data->SetSetByCallerMagnitude(ValdorsoEtichette::Dato_Danno, -DannoVero);
			Effetto.Data->SetSetByCallerMagnitude(ValdorsoEtichette::Dato_Equilibrio, 0.f);
			Fonte->ApplyGameplayEffectSpecToTarget(*Effetto.Data.Get(), Suo);
		}
	}

	/** Chi e' colpito resta fermo per qualche secondo: un colono (lo stordimento di ClientAvviso), un animale, uno scheletro. */
	void SchermaStordisci(AActor* A, float Secondi)
	{
		if (!A || Secondi <= 0.f)
		{
			return;
		}
		if (AValdorsoPlayerController* Giocatore = SchermaGiocatore(A))
		{
			Giocatore->ClientAvviso(LOCTEXT("Stordito", "Sei stordito!"), Secondi + 0.3f, Secondi);
		}
		else if (AValdorsoAnimale* Bestia = Cast<AValdorsoAnimale>(A))
		{
			Bestia->Stordisci(Secondi);
		}
		else if (AValdorsoScheletro* Ossa = Cast<AValdorsoScheletro>(A))
		{
			Ossa->Stordisci(Secondi);
		}
	}

	/** Tempra 100: l'ultima volta che il colpo che non ti piega ha salvato qualcuno (per colono). */
	TMap<TWeakObjectPtr<AActor>, double> SchermaUltimoNonTiPiega;

	/** Il comando Valdorso.Scherma.Guardia: il proprio colono (da soli) o tutti (sul server). */
	FAutoConsoleCommandWithWorldAndArgs ComandoSchermaGuardia(
		TEXT("Valdorso.Scherma.Guardia"),
		TEXT("Per le prove: Valdorso.Scherma.Guardia <0 alta | 1 lunga | 2 bassa>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Argomenti, UWorld* Mondo)
		{
			if (!Mondo || Argomenti.Num() < 1)
			{
				return;
			}
			const uint8 Quale = static_cast<uint8>(FMath::Clamp(FCString::Atoi(*Argomenti[0]), 0, 2));
			for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
			{
				const APlayerController* Controllore = It->Get();
				if (UValdorsoSchermaComponent* Scherma = UValdorsoSchermaComponent::Di(Controllore ? Controllore->GetPawn() : nullptr))
				{
					Scherma->ServerCambiaGuardia(Quale);
				}
			}
		}),
		ECVF_Cheat);

	FAutoConsoleCommand ComandoSchermaTecniche(
		TEXT("Valdorso.Scherma.Tecniche"),
		TEXT("L'elenco delle tecniche e dei talenti alle soglie."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			for (const FValdorsoTecnica& T : ValdorsoScherma::Tecniche())
			{
				UE_LOG(LogValdorso, Display, TEXT("[Valdorso] Scherma: %s (%s a %d): %s%s"), *T.Id.ToString(), *T.Abilita.ToString(),
					T.Livello * 25, T.Nome, T.Attiva() ? TEXT(" [barra]") : TEXT(""));
			}
		}));
}

namespace ValdorsoScherma
{
	const TArray<FValdorsoTecnica>& Tecniche()
	{
		static const TArray<FValdorsoTecnica> Tutte = SchermaCreaTecniche();
		return Tutte;
	}

	const FValdorsoTecnica* Tecnica(FName Id)
	{
		if (Id.IsNone())
		{
			return nullptr;
		}
		return Tecniche().FindByPredicate([Id](const FValdorsoTecnica& T) { return T.Id == Id; });
	}

	const FValdorsoTecnica* TecnicaDi(FName Abilita, int32 Livello)
	{
		return Tecniche().FindByPredicate([Abilita, Livello](const FValdorsoTecnica& T) { return T.Abilita == Abilita && T.Livello == Livello; });
	}

	bool DaBarra(FName Id)
	{
		const FValdorsoTecnica* T = Tecnica(Id);
		return T && T->Attiva();
	}

	FText NomeGuardia(FName Famiglia, EValdorsoGuardia Guardia)
	{
		// i nomi del Flos Duellatorum (Fiore dei Liberi, 1409): per ogni arma le sue poste
		struct FPoste
		{
			const TCHAR* Famiglia;
			const TCHAR* Alta;
			const TCHAR* Lunga;
			const TCHAR* Bassa;
		};
		static const FPoste Poste[] = {
			{ TEXT("Spade"), TEXT("Posta di Donna"), TEXT("Posta Longa"), TEXT("Tutta Porta di Ferro") },
			{ TEXT("Pugnali"), TEXT("Posta di Vera Croce"), TEXT("Mezza Porta di Ferro"), TEXT("Dente di Zenghiaro") },
			{ TEXT("Asce"), TEXT("Posta di Vera Finestra"), TEXT("Posta Breve la Serpentina"), TEXT("Dente di Zenghiaro") },
			{ TEXT("Mazze"), TEXT("Posta di Donna"), TEXT("Posta Frontale"), TEXT("Mezza Porta di Ferro") },
			{ TEXT("Aste"), TEXT("Posta di Finestra"), TEXT("Posta di Vera Croce"), TEXT("Tutta Porta di Ferro") },
			{ TEXT("Lotta"), TEXT("Posta Frontale"), TEXT("Posta Longa"), TEXT("Porta di Ferro") },
		};
		const FPoste* Scelte = &Poste[UE_ARRAY_COUNT(Poste) - 1];
		for (const FPoste& P : Poste)
		{
			if (Famiglia == FName(P.Famiglia))
			{
				Scelte = &P;
				break;
			}
		}
		switch (Guardia)
		{
		case EValdorsoGuardia::Alta: return FText::FromString(Scelte->Alta);
		case EValdorsoGuardia::Bassa: return FText::FromString(Scelte->Bassa);
		default: return FText::FromString(Scelte->Lunga);
		}
	}

	FText SpiegaGuardia(EValdorsoGuardia Guardia)
	{
		switch (Guardia)
		{
		case EValdorsoGuardia::Alta:
			return LOCTEXT("SpiegaAlta", "guardia alta: colpi forti, si para poco. Rompe la guardia bassa.");
		case EValdorsoGuardia::Bassa:
			return LOCTEXT("SpiegaBassa", "guardia bassa: si aspetta, si para da davanti e si risponde. Passa sotto la lunga.");
		default:
			return LOCTEXT("SpiegaLunga", "guardia lunga: la punta avanti tiene lontano. Ferma la guardia alta.");
		}
	}

	int32 Vantaggio(EValdorsoGuardia ChiColpisce, EValdorsoGuardia Colpito)
	{
		if (ChiColpisce == Colpito)
		{
			return 0;
		}
		// Alta batte Bassa, Bassa batte Lunga, Lunga batte Alta
		const bool bVince = (ChiColpisce == EValdorsoGuardia::Alta && Colpito == EValdorsoGuardia::Bassa)
			|| (ChiColpisce == EValdorsoGuardia::Bassa && Colpito == EValdorsoGuardia::Lunga)
			|| (ChiColpisce == EValdorsoGuardia::Lunga && Colpito == EValdorsoGuardia::Alta);
		return bVince ? 1 : -1;
	}

	float DannoGuardia(EValdorsoGuardia Guardia)
	{
		switch (Guardia)
		{
		case EValdorsoGuardia::Alta: return 1.25f;
		case EValdorsoGuardia::Bassa: return 0.85f;
		default: return 0.95f;
		}
	}

	float EquilibrioGuardia(EValdorsoGuardia Guardia)
	{
		return Guardia == EValdorsoGuardia::Alta ? 1.3f : 1.f;
	}

	float PortataGuardia(EValdorsoGuardia Guardia)
	{
		switch (Guardia)
		{
		case EValdorsoGuardia::Lunga: return 1.3f;
		case EValdorsoGuardia::Bassa: return 0.95f;
		default: return 1.f;
		}
	}

	float StaminaGuardia(EValdorsoGuardia Guardia)
	{
		switch (Guardia)
		{
		case EValdorsoGuardia::Alta: return 1.2f;
		case EValdorsoGuardia::Bassa: return 0.9f;
		default: return 1.f;
		}
	}

	float DifesaGuardia(EValdorsoGuardia Guardia)
	{
		return Guardia == EValdorsoGuardia::Bassa ? 0.25f : 0.f;
	}

	float SecondiCambio(int32 Evoluzioni)
	{
		static const float Secondi[5] = { 0.6f, 0.45f, 0.3f, 0.15f, 0.1f };
		return Secondi[FMath::Clamp(Evoluzioni, 0, 4)];
	}

	float Rafforza(float Valore, float Forza)
	{
		return 1.f + (Valore - 1.f) * Forza;
	}

	FName FamigliaGuardie(FName IdArma)
	{
		return ValdorsoCrescere::AbilitaDellArma(IdArma);
	}

	int32 Evoluzioni(const AActor* Corpo, FName Abilita)
	{
		const UValdorsoCrescitaComponent* Crescita = UValdorsoCrescitaComponent::DelCorpo(Corpo);
		if (!Crescita)
		{
			return 0;
		}
		const FValdorsoAbilitaValore* V = ValdorsoCrescere::Valore(Crescita->Contenuto(), Abilita);
		return V ? V->Evoluzioni : 0;
	}

	bool Talento(const AActor* Corpo, FName Abilita, int32 Livello)
	{
		return Livello >= 1 && Evoluzioni(Corpo, Abilita) >= Livello;
	}

	FString Sigla(FName Id)
	{
		const FValdorsoTecnica* T = Tecnica(Id);
		return T ? FString(T->Sigla) : FString();
	}

	TArray<const FValdorsoTecnica*> Conosciute(const AActor* Corpo)
	{
		TArray<const FValdorsoTecnica*> Sa;
		for (const FValdorsoTecnica& T : Tecniche())
		{
			if (T.Tipo != EValdorsoTipoTecnica::InArrivo && Talento(Corpo, T.Abilita, T.Livello))
			{
				Sa.Add(&T);
			}
		}
		return Sa;
	}

	void DopoEvoluzione(APlayerController* Controllore, FName Abilita, int32 Evoluzione)
	{
		const FValdorsoTecnica* T = TecnicaDi(Abilita, Evoluzione);
		AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Controllore);
		if (!T || !Giocatore || T->Tipo == EValdorsoTipoTecnica::InArrivo)
		{
			return;
		}
		if (!T->Attiva())
		{
			Giocatore->ClientAvviso(FText::Format(LOCTEXT("NuovoTalento", "Ora sai: {0}. {1}"), FText::FromString(T->Nome),
				FText::FromString(T->Descrizione)), 7.f, 0.f);
			return;
		}
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Controllore);
		int32 Messo = INDEX_NONE;
		if (Zaino)
		{
			const FName Id = T->Id;
			Zaino->Cambia([Id, &Messo](FValdorsoZaino& Z)
			{
				Z.Azioni.SetNum(ValdorsoZaino::NumeroAzioni);
				if (Z.Azioni.Contains(Id))
				{
					return false;
				}
				for (int32 k = 0; k < Z.Azioni.Num(); ++k)
				{
					if (Z.Azioni[k].IsNone())
					{
						Messo = k;
						return ValdorsoZaino::Assegna(Z, k, Id);
					}
				}
				return false;
			});
		}
		const FText Dove = Messo != INDEX_NONE ? FText::Format(LOCTEXT("NelTasto", "e' nel tasto {0}"), FText::AsNumber(Messo + 1))
			: LOCTEXT("NellaScheda", "mettila in un tasto dalla scheda (K)");
		Giocatore->ClientAvviso(FText::Format(LOCTEXT("NuovaTecnica", "Nuova tecnica: {0} ({1}). {2}"), FText::FromString(T->Nome), Dove,
			FText::FromString(T->Descrizione)), 8.f, 0.f);
	}

	FValdorsoEsitoScherma PrimaDelColpo(AActor* Da, AActor* A, EValdorsoTipoColpo Tipo, bool bDaDavanti, float& Danno, float& Equilibrio)
	{
		FValdorsoEsitoScherma Esito;
		if (!A)
		{
			return Esito;
		}
		const UWorld* Mondo = A->GetWorld();
		const double Ora = Mondo ? Mondo->GetTimeSeconds() : 0.0;
		UValdorsoSchermaComponent* MiaScherma = UValdorsoSchermaComponent::Di(Da);
		UValdorsoSchermaComponent* SuaScherma = UValdorsoSchermaComponent::Di(A);
		const FValdorsoTecnica* InCorso = MiaScherma ? MiaScherma->TecnicaInCorso() : nullptr;

		// --- 1. Il colpo non arriva: la schivata appena partita (mezzo secondo), la parata perfetta da davanti
		if (const UValdorsoVigoreComponent* Vigore = UValdorsoVigoreComponent::Di(A); Vigore && Vigore->Invulnerabile())
		{
			Esito.bAnnullato = true;
			Esito.AvvisoColpito = LOCTEXT("Schivato", "Schivato!");
			Esito.CresceIlColpito = FName(TEXT("Atletica"));
			return Esito;
		}
		if (SuaScherma && SuaScherma->InParataPerfetta() && bDaDavanti)
		{
			Esito.bAnnullato = true;
			Esito.EquilibrioAChiColpisce = 60.f;
			Esito.AvvisoColpito = LOCTEXT("ParataPerfetta", "Parata perfetta!");
			Esito.AvvisoChiColpisce = LOCTEXT("ParataSubita", "La sua lama ti aspettava: perdi l'equilibrio.");
			Esito.CresceIlColpito = FName(TEXT("Spade"));
			return Esito;
		}

		// --- 2. Le guardie: quella di chi colpisce, quella del colpito, e chi vince
		EValdorsoGuardia MiaGuardia = EValdorsoGuardia::Lunga;
		EValdorsoGuardia SuaGuardia = EValdorsoGuardia::Lunga;
		const bool bMia = SchermaGuardiaValida(Da, MiaGuardia);
		const bool bSua = SchermaGuardiaValida(A, SuaGuardia);
		const float Forza = SchermaForzaGuardie(Da);
		if (bMia)
		{
			Danno *= Rafforza(DannoGuardia(MiaGuardia), Forza);
			Equilibrio *= Rafforza(EquilibrioGuardia(MiaGuardia), Forza);
			if (MiaGuardia == EValdorsoGuardia::Lunga)
			{
				Esito.Spinta *= 1.4f;
			}
			// la risposta: dopo una parata della guardia bassa il colpo vale di piu'
			if (MiaScherma && MiaScherma->InRisposta())
			{
				Danno *= 1.4f;
				MiaScherma->ChiudiRisposta();
			}
		}
		bool bGuardiaRotta = false;
		if (bMia && bSua)
		{
			const int32 Chi = Vantaggio(MiaGuardia, SuaGuardia);
			if (Chi > 0)
			{
				Danno *= Rafforza(1.2f, Forza);
				Equilibrio *= Rafforza(1.4f, Forza);
				bGuardiaRotta = true;
			}
			else if (Chi < 0)
			{
				Danno *= 0.85f;
			}
		}
		// la guardia bassa del colpito ferma una parte di quello che arriva da davanti (se non e' rotta)
		if (bSua && bDaDavanti && !bGuardiaRotta)
		{
			const float Difesa = DifesaGuardia(SuaGuardia) * SchermaForzaGuardie(A);
			Danno *= 1.f - FMath::Clamp(Difesa, 0.f, 0.6f);
			if (SuaGuardia == EValdorsoGuardia::Bassa)
			{
				Equilibrio *= 0.6f;
				if (SuaScherma)
				{
					SuaScherma->ApriRisposta();
				}
			}
		}

		// --- 3. I talenti e le tecniche di chi colpisce
		const UValdorsoArmiComponent* MieArmi = UValdorsoArmiComponent::Di(Da);
		const FValdorsoTipoOggetto* MiaArma = MieArmi ? MieArmi->Arma() : nullptr;
		const FName MiaFamiglia = ValdorsoCrescere::AbilitaDellArma(MiaArma ? MiaArma->Id : NAME_None);
		if (!MiaArma && Tipo == EValdorsoTipoColpo::Pugno && Talento(Da, FName(TEXT("Lotta")), 4))
		{
			Danno *= 2.2f;
		}
		if (!bDaDavanti && MiaFamiglia == FName(TEXT("Pugnali")) && Talento(Da, FName(TEXT("Pugnali")), 2))
		{
			Danno *= 1.6f;
		}
		if (MiaFamiglia == FName(TEXT("Aste")) && Talento(Da, FName(TEXT("Aste")), 2) && Da)
		{
			// chi viene addosso correndo
			const FVector Verso = (Da->GetActorLocation() - A->GetActorLocation()).GetSafeNormal2D();
			if (FVector::DotProduct(A->GetVelocity(), Verso) > 300.f)
			{
				Danno *= 1.5f;
			}
		}
		if (MiaFamiglia == FName(TEXT("Mazze")) && Talento(Da, FName(TEXT("Mazze")), 2))
		{
			Esito.ScudoConta *= 0.5f;
		}
		if (InCorso)
		{
			if (InCorso->bSopraLoScudo)
			{
				Esito.ScudoConta = 0.f;
			}
			Esito.Spinta *= InCorso->Spinta;
		}
		if (MiaScherma && MiaScherma->InFuria())
		{
			Danno *= 1.3f;
		}

		// --- 4. I talenti e gli stati del colpito
		if (SuaScherma && SuaScherma->InFuria())
		{
			Danno *= 1.15f;
		}
		if (SuaScherma && SuaScherma->InRocca() && bDaDavanti)
		{
			Danno *= 0.1f;
			Equilibrio *= 0.1f;
			Esito.Spinta = 0.f;
		}
		if (Talento(A, FName(TEXT("Scudo")), 3))
		{
			Esito.AngoloScudo = -0.2f;
		}
		if (Talento(A, FName(TEXT("Scudo")), 2))
		{
			// lo scudo risponde: chi lo colpisce perde un po' di equilibrio (lo applica Colpisci se lo scudo ha parato)
			Esito.EquilibrioSeParato = 25.f;
		}
		if (Talento(A, FName(TEXT("Tempra")), 1))
		{
			Equilibrio *= 0.75f;
		}
		if ((Tipo == EValdorsoTipoColpo::Morso) && Talento(A, FName(TEXT("Tempra")), 2))
		{
			Danno *= 0.75f;
		}
		if (Talento(A, FName(TEXT("Tempra")), 4))
		{
			const UAbilitySystemComponent* SueStatistiche = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(A);
			const float SuoEquilibrio = SueStatistiche ? SueStatistiche->GetNumericAttribute(UValdorsoAttributeSet::GetEquilibrioAttribute()) : 100.f;
			if (Equilibrio >= SuoEquilibrio - 1.f)
			{
				const double* Ultimo = SchermaUltimoNonTiPiega.Find(TWeakObjectPtr<AActor>(A));
				if (!Ultimo || Ora - *Ultimo > 60.0 || Ora < *Ultimo)
				{
					SchermaUltimoNonTiPiega.Add(TWeakObjectPtr<AActor>(A), Ora);
					Equilibrio = FMath::Max(0.f, SuoEquilibrio - 1.f);
					Esito.AvvisoColpito = LOCTEXT("NonTiPiega", "Il colpo non ti piega.");
				}
			}
		}
		// (11/11) la grotta del nord: gli scheletri temono le botte e poco le lame (ValdorsoScheletri.h)
		if (const AValdorsoScheletro* Ossa = Cast<AValdorsoScheletro>(A))
		{
			Danno *= Ossa->Debolezza(Tipo);
			// lo scheletro con lo scudo para da davanti (meno contro chi gli passa sopra lo scudo)
			const FValdorsoTipoScheletro* SuoTipo = Ossa->GetTipo();
			if (SuoTipo && SuoTipo->Parata > 0.f && bDaDavanti)
			{
				Danno *= 1.f - FMath::Clamp(SuoTipo->Parata * Esito.ScudoConta, 0.f, 1.f);
			}
		}
		return Esito;
	}

	void DopoIlColpo(AActor* Da, AActor* A, bool bDaDavanti, float DannoFatto)
	{
		const UValdorsoSchermaComponent* MiaScherma = UValdorsoSchermaComponent::Di(Da);
		const FValdorsoTecnica* InCorso = MiaScherma ? MiaScherma->TecnicaInCorso() : nullptr;
		if (!InCorso || !A)
		{
			return;
		}
		if (InCorso->Stordisce > 0.f)
		{
			SchermaStordisci(A, InCorso->Stordisce);
		}
		if (InCorso->Sanguina > 0.f && A->GetWorld())
		{
			// tre gocce, una al secondo
			const float Goccia = InCorso->Sanguina / 3.f;
			TWeakObjectPtr<AActor> DeboleDa(Da);
			TWeakObjectPtr<AActor> DeboleA(A);
			for (int32 k = 1; k <= 3; ++k)
			{
				FTimerHandle Maniglia;
				A->GetWorld()->GetTimerManager().SetTimer(Maniglia, FTimerDelegate::CreateLambda([DeboleDa, DeboleA, Goccia]()
				{
					if (AActor* Ferito = DeboleA.Get())
					{
						SchermaDannoSemplice(DeboleDa.Get(), Ferito, Goccia);
					}
				}), static_cast<float>(k), false);
			}
			if (AValdorsoPlayerController* Giocatore = SchermaGiocatore(A))
			{
				Giocatore->ClientAvviso(LOCTEXT("Sanguini", "Sanguini!"), 2.f, 0.f);
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------- il componente

UValdorsoSchermaComponent::UValdorsoSchermaComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UValdorsoSchermaComponent* UValdorsoSchermaComponent::Di(const AActor* Chi)
{
	return Chi ? Chi->FindComponentByClass<UValdorsoSchermaComponent>() : nullptr;
}

void UValdorsoSchermaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UValdorsoSchermaComponent, Guardia);
	DOREPLIFETIME_CONDITION(UValdorsoSchermaComponent, Ricariche, COND_OwnerOnly);
}

void UValdorsoSchermaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* Mondo = GetWorld())
	{
		for (FTimerHandle& Maniglia : Timer)
		{
			Mondo->GetTimerManager().ClearTimer(Maniglia);
		}
	}
	Timer.Reset();
	InCorso = nullptr;
	Super::EndPlay(EndPlayReason);
}

double UValdorsoSchermaComponent::Adesso() const
{
	const UWorld* Mondo = GetWorld();
	return Mondo ? Mondo->GetTimeSeconds() : 0.0;
}

bool UValdorsoSchermaComponent::InCambio() const
{
	return Adesso() < FineCambio;
}

bool UValdorsoSchermaComponent::InParataPerfetta() const
{
	return Adesso() < FineParata;
}

bool UValdorsoSchermaComponent::InFuria() const
{
	return Adesso() < FineFuria;
}

bool UValdorsoSchermaComponent::InRocca() const
{
	return Adesso() < FineRocca;
}

bool UValdorsoSchermaComponent::InRisposta() const
{
	return Adesso() < FineRisposta;
}

void UValdorsoSchermaComponent::ApriRisposta()
{
	FineRisposta = Adesso() + SchermaSecondiRisposta;
}

void UValdorsoSchermaComponent::ChiudiRisposta()
{
	FineRisposta = 0.0;
}

void UValdorsoSchermaComponent::Avvisa(const FText& Testo, float Secondi) const
{
	if (AValdorsoPlayerController* Giocatore = SchermaGiocatore(GetOwner()))
	{
		Giocatore->ClientAvviso(Testo, Secondi, 0.f);
	}
}

void UValdorsoSchermaComponent::GuardiaDopo()
{
	const uint8 Dopo = static_cast<uint8>((static_cast<uint8>(Guardia) + 1) % 3);
	ServerCambiaGuardia(Dopo);
}

void UValdorsoSchermaComponent::ServerCambiaGuardia_Implementation(uint8 Nuova)
{
	const double Ora = Adesso();
	if (Nuova > static_cast<uint8>(EValdorsoGuardia::Bassa) || Ora - UltimaRichiesta < SchermaSecondiRichieste)
	{
		return;
	}
	UltimaRichiesta = Ora;
	const EValdorsoGuardia Scelta = static_cast<EValdorsoGuardia>(Nuova);
	const AActor* Corpo = GetOwner();
	const UValdorsoArmiComponent* Armi = UValdorsoArmiComponent::Di(Corpo);
	const FValdorsoTipoOggetto* Arma = Armi ? Armi->Arma() : nullptr;
	const FName Famiglia = ValdorsoScherma::FamigliaGuardie(Arma ? Arma->Id : NAME_None);
	const int32 Evoluta = ValdorsoScherma::Evoluzioni(Corpo, Famiglia);
	float Secondi = ValdorsoScherma::SecondiCambio(Evoluta);
	if (Famiglia == FName(TEXT("Spade")) && ValdorsoScherma::Talento(Corpo, Famiglia, 3))
	{
		Secondi = ValdorsoScherma::SecondiCambio(4);
	}
	Guardia = Scelta;
	FineCambio = Ora + Secondi;
	Avvisa(FText::Format(LOCTEXT("Guardia", "{0} - {1}"), ValdorsoScherma::NomeGuardia(Famiglia, Scelta), ValdorsoScherma::SpiegaGuardia(Scelta)), 2.5f);
}

void UValdorsoSchermaComponent::OnRep_Guardia()
{
	// (l'avviso lo manda il server; qui niente: il disegno della guardia lo leggono la barra e l'aggancio)
}

void UValdorsoSchermaComponent::ServerMettiInBarra_Implementation(FName Id, int32 Tasto)
{
	const double Ora = Adesso();
	const APawn* Corpo = Cast<APawn>(GetOwner());
	const FValdorsoTecnica* T = ValdorsoScherma::Tecnica(Id);
	if (!Corpo || !T || !T->Attiva() || Tasto < -1 || Tasto >= ValdorsoZaino::NumeroAzioni || Ora - UltimaRichiesta < SchermaSecondiRichieste
		|| !ValdorsoScherma::Talento(Corpo, T->Abilita, T->Livello))
	{
		return;
	}
	UltimaRichiesta = Ora;
	UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Cast<APlayerController>(Corpo->GetController()));
	if (!Zaino)
	{
		return;
	}
	Zaino->Cambia([Id, Tasto](FValdorsoZaino& Z)
	{
		Z.Azioni.SetNum(ValdorsoZaino::NumeroAzioni);
		int32 Dove = Tasto;
		if (Dove < 0)
		{
			Dove = Z.Azioni.IndexOfByKey(NAME_None);
			if (Dove == INDEX_NONE)
			{
				return false;
			}
		}
		return ValdorsoZaino::Assegna(Z, Dove, Id);
	});
}

float UValdorsoSchermaComponent::Ricarica(FName Id) const
{
	const double Ora = SchermaTempoServer(GetWorld());
	for (const FValdorsoRicaricaTecnica& R : Ricariche)
	{
		if (R.Id == Id && R.Durata > 0.f)
		{
			return FMath::Clamp(static_cast<float>((R.Fine - Ora) / R.Durata), 0.f, 1.f);
		}
	}
	return 0.f;
}

bool UValdorsoSchermaComponent::UsaTecnica(FName Id)
{
	ACharacter* Corpo = Cast<ACharacter>(GetOwner());
	const FValdorsoTecnica* T = ValdorsoScherma::Tecnica(Id);
	UWorld* Mondo = GetWorld();
	if (!Corpo || !Corpo->HasAuthority() || !T || !T->Attiva() || !Mondo)
	{
		return false;
	}
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
	if (Pericoli && Pericoli->Caduto())
	{
		return false;
	}
	if (const AValdorsoCharacter* Colono = Cast<AValdorsoCharacter>(Corpo); Colono && Colono->SuUnaPanca())
	{
		return false;
	}
	// l'abilita' deve essersi evoluta fin li'
	if (!ValdorsoScherma::Talento(Corpo, T->Abilita, T->Livello))
	{
		Avvisa(FText::Format(LOCTEXT("NonAncora", "{0}: non la sai ancora fare."), FText::FromString(T->Nome)));
		return false;
	}
	// l'arma giusta in mano (o lo scudo)
	const UValdorsoArmiComponent* Armi = UValdorsoArmiComponent::Di(Corpo);
	if (T->bScudo)
	{
		if (!Armi || !Armi->Scudo())
		{
			Avvisa(FText::Format(LOCTEXT("SenzaScudo", "{0}: ti serve lo scudo nella sinistra."), FText::FromString(T->Nome)));
			return false;
		}
	}
	else
	{
		const FValdorsoTipoOggetto* Arma = Armi ? Armi->Arma() : nullptr;
		if (ValdorsoCrescere::AbilitaDellArma(Arma ? Arma->Id : NAME_None) != T->Abilita)
		{
			Avvisa(FText::Format(LOCTEXT("ArmaSbagliata", "{0}: non con quello che hai in mano."), FText::FromString(T->Nome)));
			return false;
		}
	}
	// la ricarica
	const double OraServer = SchermaTempoServer(Mondo);
	Ricariche.RemoveAll([OraServer](const FValdorsoRicaricaTecnica& R) { return R.Fine <= OraServer; });
	if (Ricariche.ContainsByPredicate([Id](const FValdorsoRicaricaTecnica& R) { return R.Id == Id; }))
	{
		return false;
	}
	// la stamina
	UAbilitySystemComponent* Statistiche = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Corpo);
	if (!Statistiche || !Statistiche->GetSet<UValdorsoAttributeSet>())
	{
		return false;
	}
	const float Stamina = Statistiche->GetNumericAttribute(UValdorsoAttributeSet::GetStaminaAttribute());
	if (Stamina < T->Stamina)
	{
		Avvisa(LOCTEXT("SenzaFiato", "Non hai abbastanza fiato."));
		return false;
	}
	FGameplayEffectSpecHandle Fatica = Statistiche->MakeOutgoingSpec(UValdorsoGE_Fatica::StaticClass(), 1.f, Statistiche->MakeEffectContext());
	if (Fatica.IsValid())
	{
		Fatica.Data->SetSetByCallerMagnitude(ValdorsoEtichette::Dato_Stamina, -T->Stamina);
		Statistiche->ApplyGameplayEffectSpecToSelf(*Fatica.Data.Get());
	}
	FValdorsoRicaricaTecnica& Nuova = Ricariche.AddDefaulted_GetRef();
	Nuova.Id = Id;
	Nuova.Durata = T->Ricarica;
	Nuova.Fine = static_cast<float>(OraServer + T->Ricarica);

	ValdorsoRegistroEventi::Annota(TEXT("Tecnica"), Corpo, nullptr, Corpo->GetActorLocation(), T->Id.ToString());
	if (T->Tipo == EValdorsoTipoTecnica::Stato)
	{
		const double Ora = Adesso();
		if (T->Id == FName(TEXT("Tecnica.ParataPerfetta")))
		{
			FineParata = Ora + T->Durata;
			MulticastAnimazione(1.3f);
		}
		else if (T->Id == FName(TEXT("Tecnica.Furia")))
		{
			FineFuria = Ora + T->Durata;
			Avvisa(LOCTEXT("Furia", "La furia!"), 2.f);
		}
		else if (T->Id == FName(TEXT("Tecnica.Rocca")))
		{
			FineRocca = Ora + T->Durata;
			Avvisa(LOCTEXT("Rocca", "La rocca: da davanti non passa niente."), 2.f);
		}
		return true;
	}
	// un colpo: lo slancio subito, il braccio, poi i colpi
	MulticastAnimazione(T->Colpi > 1 ? 1.4f : 1.15f);
	if (T->Slancio > 0.f && Corpo->GetCharacterMovement() && Corpo->GetCharacterMovement()->IsMovingOnGround())
	{
		Corpo->LaunchCharacter(Corpo->GetActorForwardVector() * T->Slancio + FVector(0.f, 0.f, 60.f), true, false);
	}
	TWeakObjectPtr<UValdorsoSchermaComponent> Debole(this);
	for (int32 k = 0; k < FMath::Max(1, T->Colpi); ++k)
	{
		FTimerHandle& Maniglia = Timer.AddDefaulted_GetRef();
		const float Quando = SchermaPrimoColpo + T->Intervallo * static_cast<float>(k);
		Mondo->GetTimerManager().SetTimer(Maniglia, FTimerDelegate::CreateLambda([Debole, T, k]()
		{
			if (UValdorsoSchermaComponent* Ancora = Debole.Get())
			{
				Ancora->Colpo(T, k);
			}
		}), Quando, false);
	}
	// (i timer finiti non servono piu')
	if (Timer.Num() > 16)
	{
		Timer.RemoveAll([Mondo](const FTimerHandle& Maniglia) { return !Mondo->GetTimerManager().IsTimerActive(Maniglia); });
	}
	return true;
}

void UValdorsoSchermaComponent::Colpo(const FValdorsoTecnica* Quale, int32 Numero)
{
	ACharacter* Corpo = Cast<ACharacter>(GetOwner());
	UWorld* Mondo = GetWorld();
	if (!Corpo || !Quale || !Mondo)
	{
		return;
	}
	const UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Corpo);
	if (Pericoli && Pericoli->Caduto())
	{
		return;
	}
	if (Numero > 0)
	{
		MulticastAnimazione(1.4f);
	}
	const UValdorsoArmiComponent* Armi = UValdorsoArmiComponent::Di(Corpo);
	const FValdorsoTipoOggetto* ConCosa = Quale->bScudo ? (Armi ? Armi->Scudo() : nullptr) : (Armi ? Armi->Arma() : nullptr);
	const float DannoBase = ConCosa ? ConCosa->DannoArma : 10.f;
	const float PortataBase = ConCosa ? ConCosa->PortataArma : 90.f;
	const float EquilibrioBase = ConCosa ? ConCosa->EquilibrioArma : ValdorsoPericoli::EquilibrioPerPugno;
	const EValdorsoTipoColpo TipoColpo = ConCosa ? static_cast<EValdorsoTipoColpo>(ConCosa->TipoColpo) : EValdorsoTipoColpo::Pugno;
	EValdorsoGuardia Mia = EValdorsoGuardia::Lunga;
	const float PerGuardia = SchermaGuardiaValida(Corpo, Mia) ? ValdorsoScherma::PortataGuardia(Mia) : 1.f;
	const float DannoColpo = DannoBase * Quale->Danno;
	const float EquilibrioColpo = EquilibrioBase + Quale->Equilibrio;
	const float Portata = PortataBase * Quale->Portata * PerGuardia;

	// chi prende il colpo: tutti intorno (il giro), o chi sta davanti fino alla portata
	TArray<FHitResult> Toccati;
	FCollisionObjectQueryParams Oggetti;
	Oggetti.AddObjectTypesToQuery(ECC_Pawn);
	Oggetti.AddObjectTypesToQuery(ECC_WorldDynamic);
	Oggetti.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoTecnica), false, Corpo);
	const FVector Centro = Corpo->GetActorLocation();
	if (Quale->Giro > 0.f)
	{
		Mondo->SweepMultiByObjectType(Toccati, Centro, Centro + FVector(0.f, 0.f, 1.f), FQuat::Identity, Oggetti,
			FCollisionShape::MakeSphere(Quale->Giro), Parametri);
	}
	else
	{
		const FVector Avanti = Corpo->GetActorForwardVector();
		const float Raggio = 35.f;
		Mondo->SweepMultiByObjectType(Toccati, Centro + Avanti * 30.f, Centro + Avanti * FMath::Max(60.f, Portata), FQuat::Identity, Oggetti,
			FCollisionShape::MakeSphere(Raggio), Parametri);
	}
	InCorso = Quale;
	TSet<AActor*> Gia;
	for (const FHitResult& Toccato : Toccati)
	{
		AActor* Bersaglio = Toccato.GetActor();
		if (!Bersaglio || Bersaglio == Corpo || Gia.Contains(Bersaglio))
		{
			continue;
		}
		Gia.Add(Bersaglio);
		const FVector Punto = Toccato.bStartPenetrating ? ValdorsoCombattimento::PuntoBersaglio(Bersaglio) : FVector(Toccato.ImpactPoint);
		ValdorsoCombattimento::Colpisci(Corpo, Bersaglio, DannoColpo, EquilibrioColpo, Punto, TipoColpo);
	}
	InCorso = nullptr;
}

void UValdorsoSchermaComponent::MulticastAnimazione_Implementation(float Velocita)
{
	const AValdorsoCharacter* Colono = Cast<AValdorsoCharacter>(GetOwner());
	USkeletalMeshComponent* Corpo = Colono ? Colono->GetMesh() : nullptr;
	UAnimInstance* Animazioni = Corpo ? Corpo->GetAnimInstance() : nullptr;
	if (!Colono || !Animazioni || !Colono->MontaggioPugno || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	Animazioni->Montage_Play(Colono->MontaggioPugno, Velocita);
}

#undef LOCTEXT_NAMESPACE
