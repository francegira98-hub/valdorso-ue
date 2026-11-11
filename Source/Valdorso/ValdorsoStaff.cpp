// Valdorso - La finestra dello staff (vedi il .h). Scritto da Claude il 10/11/2026.

#include "ValdorsoStaff.h"
#include "SValdorsoStaff.h"
#include "Valdorso.h"
#include "ValdorsoArchivista.h"
#include "ValdorsoAbilitaDati.h"
#include "ValdorsoCrescita.h"
#include "ValdorsoGilda.h"
#include "ValdorsoLupi.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoPermessi.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ValdorsoAttributeSet.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#define LOCTEXT_NAMESPACE "ValdorsoStaff"

namespace
{
	/** Quante righe del registro arrivano alla finestra (un messaggio in rete non deve diventare enorme). */
	constexpr int32 StaffRigheRegistro = 80;
	/** Una richiesta ogni tanto (s). */
	constexpr double StaffSecondiRichieste = 0.2;

	/** Il punto a terra sopra/sotto Dove (o Dove stesso, se la zona non e' caricata). */
	FVector StaffATerra(UWorld* Mondo, const FVector& Dove, const AActor* Ignora)
	{
		FHitResult Colpo;
		FCollisionQueryParams Parametri(SCENE_QUERY_STAT(ValdorsoStaffTerra), false, Ignora);
		if (Mondo && Mondo->LineTraceSingleByChannel(Colpo, Dove + FVector(0.f, 0.f, 20000.f), Dove - FVector(0.f, 0.f, 20000.f), ECC_Visibility, Parametri))
		{
			return Colpo.ImpactPoint + FVector(0.f, 0.f, 150.f);
		}
		return Dove + FVector(0.f, 0.f, 250.f);
	}

	/** Il luogo con quel nome (anche solo l'inizio, senza maiuscole). */
	const FValdorsoLuogo* StaffLuogo(const FString& Nome)
	{
		const FString Cercato = Nome.TrimStartAndEnd();
		if (Cercato.IsEmpty())
		{
			return nullptr;
		}
		for (const FValdorsoLuogo& L : ValdorsoLuoghi::Tutti())
		{
			if (L.Nome.Equals(Cercato, ESearchCase::IgnoreCase))
			{
				return &L;
			}
		}
		for (const FValdorsoLuogo& L : ValdorsoLuoghi::Tutti())
		{
			if (L.Nome.StartsWith(Cercato, ESearchCase::IgnoreCase))
			{
				return &L;
			}
		}
		return nullptr;
	}

	/** Un comando della console sul mondo del server (ora, tempo, campane...): solo questi, scritti qui. */
	void StaffConsole(UWorld* Mondo, const FString& Comando)
	{
		if (GEngine && Mondo)
		{
			GEngine->Exec(Mondo, *Comando);
		}
	}
}

UValdorsoStaffComponent::UValdorsoStaffComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UValdorsoStaffComponent* UValdorsoStaffComponent::Di(const APlayerController* Controllore)
{
	return Controllore ? Controllore->FindComponentByClass<UValdorsoStaffComponent>() : nullptr;
}

int32 UValdorsoStaffComponent::PermessiDi(const APlayerController* Controllore)
{
	const AValdorsoPlayerController* Nostro = Cast<AValdorsoPlayerController>(Controllore);
	// solo chi e' entrato davvero (la password controllata): il nome dell'account arriva prima del controllo
	if (!Nostro || !Nostro->HasAuthority() || !Nostro->EAutenticato())
	{
		return 0;
	}
	// l'account vero, se c'e' (cosi' un ruolo o un permesso cambiato vale subito, senza rientrare)
	const UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(Nostro);
	const FString& Nome = Nostro->GetNomeAccount();
	if (Archivista && !Nome.IsEmpty())
	{
		const FString Id = Archivista->IdDi(Nome);
		if (!Id.IsEmpty())
		{
			return Id == Nostro->GetAccountId() ? Archivista->PermessiDi(Nome) : 0;
		}
	}
	// senza account (l'accesso libero dell'editor): dal ruolo che ha avuto entrando
	switch (Nostro->GetRuolo())
	{
	case EValdorsoRuolo::Amministratore: return ValdorsoPermessi::Tutti;
	case EValdorsoRuolo::Staff: return ValdorsoPermessi::DiSerieStaff;
	default: return 0;
	}
}

FString UValdorsoStaffComponent::NomeMio() const
{
	const APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	return Controllore && Controllore->PlayerState ? Controllore->PlayerState->GetPlayerName() : FString(TEXT("staff"));
}

APlayerController* UValdorsoStaffComponent::Colono(const FString& Nome) const
{
	UWorld* Mondo = GetWorld();
	if (!Mondo || Nome.IsEmpty())
	{
		return nullptr;
	}
	for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Altro = It->Get();
		if (Altro && Altro->PlayerState && Altro->PlayerState->GetPlayerName().Equals(Nome, ESearchCase::IgnoreCase))
		{
			return Altro;
		}
	}
	return nullptr;
}

FValdorsoStaffVista UValdorsoStaffComponent::Componi(const FString& FiltroRegistro) const
{
	FValdorsoStaffVista V;
	const APlayerController* Io = Cast<APlayerController>(GetOwner());
	V.Permessi = PermessiDi(Io);
	const UValdorsoGildaComponent* MiaGilda = UValdorsoGildaComponent::Di(Io);
	V.bCapo = MiaGilda && MiaGilda->Contenuto().bCapoGilda;
	V.Io = NomeMio();
	UWorld* Mondo = GetWorld();
	const UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
	if (Mondo)
	{
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			const AValdorsoPlayerController* Altro = Cast<AValdorsoPlayerController>(It->Get());
			if (!Altro || !Altro->PlayerState)
			{
				continue;
			}
			FValdorsoStaffColono C;
			C.Nome = Altro->PlayerState->GetPlayerName();
			C.Ruolo = static_cast<uint8>(Altro->GetRuolo());
			if (V.Permessi & ValdorsoPermessi::Permessi)
			{
				C.Account = Altro->GetNomeAccount();
				C.Permessi = PermessiDi(Altro);
				if (Archivista && !C.Account.IsEmpty() && !Archivista->IdDi(C.Account).IsEmpty())
				{
					C.Ruolo = static_cast<uint8>(Archivista->RuoloDi(C.Account));
				}
			}
			if (const UValdorsoGildaComponent* G = UValdorsoGildaComponent::Di(Altro))
			{
				C.Grado = G->Contenuto().Grado;
				C.Punti = G->Contenuto().Punti;
				C.bPronto = ValdorsoGilda::Pronto(G->Contenuto());
				C.bCapo = G->Contenuto().bCapoGilda;
				C.Incarichi = G->Contenuto().Presi.Num();
			}
			// gli Aureus, la salute e dove sta: solo a chi fa le prove o sposta i giocatori (non al capo gilda)
			const bool bDettagli = (V.Permessi & (ValdorsoPermessi::Prove | ValdorsoPermessi::Giocatori)) != 0;
			const UValdorsoZainoComponent* Z = bDettagli ? UValdorsoZainoComponent::Di(Altro) : nullptr;
			if (Z)
			{
				C.Monete = Z->Contenuto().Monete;
			}
			if (const APawn* Corpo = bDettagli ? Altro->GetPawn() : nullptr)
			{
				if (const UAbilitySystemComponent* Abilita = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Corpo))
				{
					C.Salute = FMath::RoundToInt(Abilita->GetNumericAttribute(UValdorsoAttributeSet::GetSaluteAttribute()));
				}
				const TArray<const FValdorsoLuogo*> Dove = ValdorsoLuoghi::Intorno(Corpo->GetActorLocation());
				C.Luogo = Dove.IsEmpty() ? FString() : Dove[0]->Nome;
			}
			V.Coloni.Add(C);
		}
	}
	if (V.Permessi & ValdorsoPermessi::Bacheche)
	{
		if (UValdorsoBacheche* Bacheche = UValdorsoBacheche::Di(this))
		{
			V.Incarichi = Bacheche->Dati().Incarichi;
			V.Proclami = Bacheche->Dati().Proclami;
			V.Cassa = Bacheche->Dati().Cassa;
		}
	}
	if (V.Permessi & ValdorsoPermessi::Registro)
	{
		for (const FString& Riga : ValdorsoRegistroEventi::Ultimi(StaffRigheRegistro, FiltroRegistro.Left(40)))
		{
			V.Registro.Add(Riga.Left(220));
		}
	}
	return V;
}

void UValdorsoStaffComponent::ServerApri_Implementation(const FString& FiltroRegistro)
{
	UWorld* Mondo = GetWorld();
	const double Ora = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	if (Ora < UltimaRichiesta + StaffSecondiRichieste)
	{
		return;
	}
	UltimaRichiesta = Ora;
	APlayerController* Io = Cast<APlayerController>(GetOwner());
	const UValdorsoGildaComponent* MiaGilda = UValdorsoGildaComponent::Di(Io);
	// chi non ha permessi (e non e' capo gilda) non riceve niente: la finestra non si apre
	if (PermessiDi(Io) == 0 && !(MiaGilda && MiaGilda->Contenuto().bCapoGilda))
	{
		ClientEsito(TEXT("La finestra dello staff e' per lo staff (e per il capo gilda)."));
		return;
	}
	ClientVista(Componi(FiltroRegistro));
}

void UValdorsoStaffComponent::ServerAzione_Implementation(const FString& Azione, const FString& Chi, const FString& Valore)
{
	UWorld* Mondo = GetWorld();
	const double Ora = Mondo ? Mondo->GetTimeSeconds() : 0.0;
	if (Ora < UltimaRichiesta + StaffSecondiRichieste)
	{
		return;
	}
	UltimaRichiesta = Ora;
	// chi non ha permessi (e non e' capo gilda) non fa niente e non vede niente
	APlayerController* Io = Cast<APlayerController>(GetOwner());
	const UValdorsoGildaComponent* MiaGilda = UValdorsoGildaComponent::Di(Io);
	if (PermessiDi(Io) == 0 && !(MiaGilda && MiaGilda->Contenuto().bCapoGilda))
	{
		return;
	}
	const FString Pulita = Valore.Left(200);
	const FString Risposta = Fai(Azione.Left(30), Chi.Left(60), Pulita);
	if (!Risposta.IsEmpty())
	{
		ClientEsito(Risposta);
	}
	ClientVista(Componi(Filtro));
}

FString UValdorsoStaffComponent::Fai(const FString& Azione, const FString& Chi, const FString& Valore)
{
	APlayerController* Io = Cast<APlayerController>(GetOwner());
	UWorld* Mondo = GetWorld();
	if (!Io || !Mondo)
	{
		return FString();
	}
	const int32 Miei = PermessiDi(Io);
	UValdorsoGildaComponent* MiaGilda = UValdorsoGildaComponent::Di(Io);
	const bool bCapo = MiaGilda && MiaGilda->Contenuto().bCapoGilda;
	const FString Me = NomeMio();
	auto Puo = [Miei](int32 Permesso) { return (Miei & Permesso) != 0; };
	auto Annota = [Io, &Me, &Azione, &Chi, &Valore](const FString& Come)
	{
		ValdorsoRegistroEventi::Annota(TEXT("Staff"), Io, nullptr, Io->GetPawn() ? Io->GetPawn()->GetActorLocation() : FVector::ZeroVector,
			FString::Printf(TEXT("%s: %s %s %s: %s"), *Me, *Azione, *Chi, *Valore.Left(80), *Come));
	};
	const FString NonPuoi = TEXT("Non hai il permesso per questo.");
	if (Azione == TEXT("aggiorna"))
	{
		Filtro = Valore.Left(40);
		return FString();
	}

	// --- Gilda: gradi, punti, capo gilda ----------------------------------------------------------------
	if (Azione == TEXT("grado") || Azione == TEXT("punti") || Azione == TEXT("capo"))
	{
		APlayerController* Suo = Colono(Chi);
		UValdorsoGildaComponent* SuaGilda = UValdorsoGildaComponent::Di(Suo);
		if (!SuaGilda)
		{
			return FString::Printf(TEXT("%s non e' nella valle."), *Chi);
		}
		const bool bStaff = Puo(ValdorsoPermessi::Gilda);
		if (Azione == TEXT("grado"))
		{
			const int32 Numero = Valore.IsNumeric() ? FCString::Atoi(*Valore) : -1;
			if (Numero < 0 || Numero > 3)
			{
				return TEXT("Il grado: 0 Comune, 1 Bronzo, 2 Argento, 3 Oro.");
			}
			if (!bStaff && !bCapo)
			{
				return NonPuoi;
			}
			const EValdorsoGrado Nuovo = static_cast<EValdorsoGrado>(Numero);
			const FText Perche = ValdorsoGilda::PercheNoNomina(SuaGilda->Contenuto(), Nuovo, bStaff, MiaGilda ? &MiaGilda->Contenuto() : nullptr, Suo == Io);
			if (!Perche.IsEmpty())
			{
				return Perche.ToString();
			}
			SuaGilda->Nomina(Nuovo, Me, bStaff);
			Annota(TEXT("fatto"));
			return FString::Printf(TEXT("%s ora e' %s della Gilda."), *Chi, *ValdorsoGilda::NomeGrado(Nuovo).ToString());
		}
		if (!bStaff)
		{
			return NonPuoi;
		}
		if (Azione == TEXT("punti"))
		{
			if (!Valore.IsNumeric())
			{
				return TEXT("Scrivi i punti (un numero).");
			}
			SuaGilda->ImpostaPunti(FMath::Clamp(FCString::Atoi(*Valore), 0, 100000));
			Annota(TEXT("fatto"));
			return FString::Printf(TEXT("%s ha %d punti della Gilda."), *Chi, SuaGilda->Contenuto().Punti);
		}
		SuaGilda->ImpostaCapo(Valore == TEXT("1"), Me);
		Annota(TEXT("fatto"));
		return FString::Printf(TEXT("%s %s capo gilda."), *Chi, Valore == TEXT("1") ? TEXT("ora e'") : TEXT("non e' piu'"));
	}

	// --- Prove sui coloni ----------------------------------------------------------------------------
	if (Azione == TEXT("dai") || Azione == TEXT("monete") || Azione == TEXT("cura") || Azione == TEXT("armeria") || Azione == TEXT("svuota") || Azione == TEXT("fatto"))
	{
		if (!Puo(ValdorsoPermessi::Prove))
		{
			return NonPuoi;
		}
		APlayerController* Suo = Colono(Chi.IsEmpty() ? Me : Chi);
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Suo);
		if (!Zaino)
		{
			return FString::Printf(TEXT("%s non e' nella valle."), *Chi);
		}
		const FString Nome = Suo->PlayerState ? Suo->PlayerState->GetPlayerName() : Chi;
		if (Azione == TEXT("dai"))
		{
			TArray<FString> Parti;
			Valore.ParseIntoArrayWS(Parti);
			const FName Id = Parti.Num() > 0 ? FName(*Parti[0], FNAME_Find) : NAME_None;
			if (!ValdorsoOggetti::Trova(Id))
			{
				return FString::Printf(TEXT("Non c'e' l'oggetto \"%s\" (gli Id: Valdorso.Zaino.Elenco)."), Parti.Num() > 0 ? *Parti[0] : TEXT(""));
			}
			const int32 Quanti = Parti.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Parti[1]), 1, 99) : 1;
			const int32 Entrati = Zaino->Dai(Id, Quanti);
			Annota(FString::Printf(TEXT("entrati %d"), Entrati));
			return FString::Printf(TEXT("Dati a %s: %d × %s."), *Nome, Entrati, *Id.ToString());
		}
		if (Azione == TEXT("monete"))
		{
			if (!Valore.IsNumeric())
			{
				return TEXT("Scrivi gli Aureus (un numero).");
			}
			Zaino->ImpostaMonete(FMath::Clamp(FCString::Atoi(*Valore), 0, ValdorsoZaino::MassimoMonete));
			Annota(TEXT("fatto"));
			return FString::Printf(TEXT("%s ha %d Aureus."), *Nome, Zaino->Contenuto().Monete);
		}
		if (Azione == TEXT("cura"))
		{
			UValdorsoPericoliComponent* Pericoli = UValdorsoPericoliComponent::Di(Suo->GetPawn());
			if (!Pericoli)
			{
				return FString::Printf(TEXT("%s non ha un corpo da curare."), *Nome);
			}
			if (Pericoli->Caduto())
			{
				return FString::Printf(TEXT("%s e' caduto: rinasce da solo al Tempio del Cuore (non si cura da terra)."), *Nome);
			}
			Pericoli->Ferisci(-10000.f);
			Annota(TEXT("fatto"));
			return FString::Printf(TEXT("%s curato."), *Nome);
		}
		if (Azione == TEXT("armeria"))
		{
			for (const FValdorsoTipoOggetto& Tipo : ValdorsoOggetti::Catalogo())
			{
				if (Tipo.Uso == EValdorsoUso::Impugna && ValdorsoZaino::Conta(Zaino->Contenuto(), Tipo.Id) == 0)
				{
					Zaino->Dai(Tipo.Id, 1, false);
				}
			}
			Annota(TEXT("fatto"));
			return FString::Printf(TEXT("A %s le armi e gli scudi (quelli che entrano)."), *Nome);
		}
		if (Azione == TEXT("svuota"))
		{
			Zaino->Svuota();
			// i fogli della Gilda tornano (gli incarichi sono ancora suoi)
			if (UValdorsoGildaComponent* SuaGilda = UValdorsoGildaComponent::Di(Suo))
			{
				SuaGilda->RimettiFogli();
			}
			Annota(TEXT("fatto"));
			return FString::Printf(TEXT("Zaino di %s svuotato (anche i ricordi; i fogli della Gilda tornano)."), *Nome);
		}
		// fatto: gli incarichi assegnati diventano fatti
		if (UValdorsoGildaComponent* SuaGilda = UValdorsoGildaComponent::Di(Suo))
		{
			SuaGilda->FaiTutto();
		}
		Annota(TEXT("fatto"));
		return FString::Printf(TEXT("Gli incarichi assegnati di %s sono fatti (le consegne no: servono le cose)."), *Nome);
	}

	// --- (11/11) Le abilita' dei coloni (permesso "abilita"): "Lame 40" ---------------------------------------
	if (Azione == TEXT("abilita"))
	{
		if (!Puo(ValdorsoPermessi::Abilita))
		{
			return NonPuoi;
		}
		APlayerController* Suo = Colono(Chi.IsEmpty() ? Me : Chi);
		UValdorsoCrescitaComponent* SuaCrescita = UValdorsoCrescitaComponent::Di(Suo);
		if (!SuaCrescita)
		{
			return FString::Printf(TEXT("%s non e' nella valle."), *Chi);
		}
		TArray<FString> Parti;
		Valore.ParseIntoArrayWS(Parti);
		const FName Id = Parti.Num() > 0 ? FName(*Parti[0], FNAME_Find) : NAME_None;
		if (Parti.Num() < 2 || !Parti[1].IsNumeric() || !ValdorsoCrescere::Trova(Id))
		{
			FString Elenco;
			for (const FValdorsoAbilitaTipo& Tipo : ValdorsoCrescere::Tutte())
			{
				Elenco += (Elenco.IsEmpty() ? TEXT("") : TEXT(", ")) + Tipo.Id.ToString();
			}
			return FString::Printf(TEXT("Scrivi l'abilita' e i punti (es. Lame 40). Le abilita': %s."), *Elenco.Left(400));
		}
		const float Punti = FMath::Clamp(FCString::Atof(*Parti[1]), 0.f, ValdorsoCrescere::Massimo);
		const FString Nome = Suo->PlayerState ? Suo->PlayerState->GetPlayerName() : Chi;
		if (!SuaCrescita->ImpostaPunti(Id, Punti, FString::Printf(TEXT("staff (%s)"), *Me)))
		{
			return FString::Printf(TEXT("Non riesco a impostare %s a %s."), *Id.ToString(), *Nome);
		}
		Annota(TEXT("fatto"));
		return FString::Printf(TEXT("%s: %s a %.0f punti."), *Nome, *Id.ToString(), Punti);
	}

	// --- Bacheche --------------------------------------------------------------------------------------
	if (Azione == TEXT("proclama") || Azione == TEXT("legge") || Azione == TEXT("incarico") || Azione == TEXT("togliincarico")
		|| Azione == TEXT("togliproclama") || Azione == TEXT("genera") || Azione == TEXT("cassa"))
	{
		if (!Puo(ValdorsoPermessi::Bacheche))
		{
			return NonPuoi;
		}
		UValdorsoBacheche* Bacheche = UValdorsoBacheche::Di(this);
		if (!Bacheche)
		{
			return TEXT("Le bacheche ci sono solo nella valle.");
		}
		if (Azione == TEXT("genera"))
		{
			FRandomStream Caso(static_cast<int32>(FPlatformTime::Cycles()));
			const bool bMesso = ValdorsoGilda::Genera(Bacheche->Dati(), UValdorsoBacheche::Adesso(), Caso);
			Bacheche->Segna();
			Annota(bMesso ? TEXT("messo") : TEXT("niente"));
			return bMesso ? TEXT("Gli abitanti hanno appeso un incarico nuovo.") : TEXT("Niente di nuovo (sono gia' sei, o la cassa non basta).");
		}
		if (Azione == TEXT("cassa"))
		{
			if (!Valore.IsNumeric())
			{
				return TEXT("Scrivi gli Aureus della cassa (un numero).");
			}
			Bacheche->Dati().Cassa = FMath::Clamp(FCString::Atoi(*Valore), 0, ValdorsoGilda::CassaMassima);
			Bacheche->Segna();
			Annota(TEXT("fatto"));
			return FString::Printf(TEXT("La cassa del villaggio ha %d Aureus."), Bacheche->Dati().Cassa);
		}
		// gli altri sono gli stessi comandi della chat (ComandoStaff controlla ancora i permessi)
		FString Risposta;
		Bacheche->ComandoStaff(Io, FString::Printf(TEXT("/%s %s"), *Azione, *Valore), Risposta);
		Annota(Risposta.Left(80));
		return Risposta;
	}

	// --- Mondo ----------------------------------------------------------------------------------------
	if (Azione == TEXT("ora") || Azione == TEXT("tempo") || Azione == TEXT("giorno") || Azione == TEXT("animali") || Azione == TEXT("animalivia")
		|| Azione == TEXT("ricresci") || Azione == TEXT("campane") || Azione == TEXT("fase") || Azione == TEXT("sigillo")
		|| Azione == TEXT("frana") || Azione == TEXT("atto"))
	{
		if (!Puo(ValdorsoPermessi::Mondo))
		{
			return NonPuoi;
		}
		// i valori si controllano qui: alla console arrivano solo comandi scritti da noi
		const bool bNumero = Valore.IsNumeric();
		const int32 Numero = bNumero ? FCString::Atoi(*Valore) : 0;
		FString Comando;
		if (Azione == TEXT("ora") && bNumero && Numero >= 0 && Numero <= 24) { Comando = FString::Printf(TEXT("Valdorso.Ora %d"), Numero); }
		else if (Azione == TEXT("giorno") && bNumero && Numero >= 1 && Numero <= 600) { Comando = FString::Printf(TEXT("Valdorso.Giorno.Minuti %d"), Numero); }
		else if (Azione == TEXT("campane")) { Comando = Valore == TEXT("1") ? TEXT("Valdorso.Campane.Allarme 1") : TEXT("Valdorso.Campane.Allarme 0"); }
		else if (Azione == TEXT("fase") && bNumero && Numero >= 1 && Numero <= 3) { Comando = FString::Printf(TEXT("Valdorso.Villaggio.Fase %d"), Numero); }
		else if (Azione == TEXT("sigillo")) { Comando = Valore == TEXT("apri") ? TEXT("Valdorso.Sigillo.Apri") : TEXT("Valdorso.Sigillo.Chiudi"); }
		// (11/11, v0.1.7) la frana della grotta del nord (ValdorsoGrottaNord.h): trema, si apre, si richiude
		else if (Azione == TEXT("frana"))
		{
			if (Valore == TEXT("trema")) { Comando = TEXT("Valdorso.Grotta.Trema"); }
			else if (Valore == TEXT("apri")) { Comando = TEXT("Valdorso.Grotta.Apri"); }
			else if (Valore == TEXT("chiudi")) { Comando = TEXT("Valdorso.Grotta.Chiudi"); }
		}
		// (11/11, v0.1.6) l'Atto I "Il battito cambiato" (ValdorsoVoci.h): il frammento del tempio salta un colpo
		else if (Azione == TEXT("atto")) { Comando = Valore == TEXT("0") ? TEXT("Valdorso.Atto.Battito 0") : TEXT("Valdorso.Atto.Battito 1"); }
		else if (Azione == TEXT("ricresci")) { Comando = TEXT("Valdorso.Raccolta.Ricresci"); }
		else if (Azione == TEXT("animalivia")) { Comando = TEXT("Valdorso.Animali.Via"); }
		else if (Azione == TEXT("tempo"))
		{
			static const TCHAR* Tempi[] = { TEXT("Sereno"), TEXT("Nuvoloso"), TEXT("Nebbia"), TEXT("Pioggia"), TEXT("Temporale") };
			for (const TCHAR* T : Tempi)
			{
				if (Valore.Equals(T, ESearchCase::IgnoreCase))
				{
					Comando = FString::Printf(TEXT("Valdorso.Tempo %s"), T);
				}
			}
		}
		else if (Azione == TEXT("animali"))
		{
			// davanti a chi chiama (non al primo colono, come la console)
			const FName QualeSpecie(*Valore, FNAME_Find);
			const FValdorsoSpecie* Specie = ValdorsoAnimali::Specie(QualeSpecie);
			AValdorsoTane* Tane = AValdorsoTane::Di(Mondo);
			const APawn* Corpo = Io->GetPawn();
			if (!Specie || !Tane || !Corpo)
			{
				return TEXT("Serve una specie (Lupo, Cinghiale, Orso, Cervo, Lepre, Volpe) e un colono in gioco.");
			}
			const int32 Venuti = Tane->Chiama(QualeSpecie, Corpo->GetActorLocation() + Corpo->GetActorForwardVector().GetSafeNormal2D() * 2500.f);
			Annota(FString::Printf(TEXT("%d animali"), Venuti));
			return FString::Printf(TEXT("%d × %s a 25 m davanti a te."), Venuti, *Valore);
		}
		if (Comando.IsEmpty())
		{
			return TEXT("Valore non valido.");
		}
		StaffConsole(Mondo, Comando);
		Annota(Comando);
		return FString::Printf(TEXT("Fatto: %s"), *Comando);
	}

	// --- Giocatori: dove stanno ------------------------------------------------------------------------
	if (Azione == TEXT("vaida") || Azione == TEXT("chiama") || Azione == TEXT("luogo"))
	{
		if (!Puo(ValdorsoPermessi::Giocatori))
		{
			return NonPuoi;
		}
		APawn* MioCorpo = Io->GetPawn();
		APlayerController* Suo = Colono(Chi.IsEmpty() ? Me : Chi);
		APawn* SuoCorpo = Suo ? Suo->GetPawn() : nullptr;
		if (!SuoCorpo || !MioCorpo)
		{
			return FString::Printf(TEXT("%s non e' nella valle (o non ha un corpo)."), Chi.IsEmpty() ? *Me : *Chi);
		}
		if (Azione == TEXT("luogo"))
		{
			const FValdorsoLuogo* Luogo = StaffLuogo(Valore);
			if (!Luogo)
			{
				return FString::Printf(TEXT("Non c'e' il luogo \"%s\"."), *Valore);
			}
			SuoCorpo->TeleportTo(StaffATerra(Mondo, Luogo->Posizione, SuoCorpo), SuoCorpo->GetActorRotation(), false, true);
			Annota(Luogo->Nome);
			return FString::Printf(TEXT("%s portato a %s."), Chi.IsEmpty() ? *Me : *Chi, *Luogo->Nome);
		}
		APawn* ChiVa = Azione == TEXT("vaida") ? MioCorpo : SuoCorpo;
		const APawn* DaChi = Azione == TEXT("vaida") ? SuoCorpo : MioCorpo;
		// due metri davanti a chi resta fermo
		const FVector Dove = DaChi->GetActorLocation() + DaChi->GetActorForwardVector().GetSafeNormal2D() * 200.f;
		ChiVa->TeleportTo(StaffATerra(Mondo, Dove, ChiVa), (DaChi->GetActorLocation() - Dove).Rotation(), false, true);
		Annota(TEXT("fatto"));
		return Azione == TEXT("vaida") ? FString::Printf(TEXT("Sei da %s."), *Chi) : FString::Printf(TEXT("%s e' qui."), *Chi);
	}

	// --- Permessi e ruoli (solo l'Amministratore) -----------------------------------------------------
	if (Azione == TEXT("permessi") || Azione == TEXT("ruolo"))
	{
		if (!Puo(ValdorsoPermessi::Permessi))
		{
			return NonPuoi;
		}
		const AValdorsoPlayerController* Suo = Cast<AValdorsoPlayerController>(Colono(Chi));
		UValdorsoArchivista* Archivista = UValdorsoArchivista::Di(this);
		if (!Suo || !Archivista || Suo->GetNomeAccount().IsEmpty())
		{
			return FString::Printf(TEXT("%s non e' nella valle (o e' entrato senza account)."), *Chi);
		}
		if (Archivista->RuoloDi(Suo->GetNomeAccount()) == EValdorsoRuolo::Amministratore)
		{
			return TEXT("Un Amministratore si cambia solo dalla console del server.");
		}
		const FString Risposta = Azione == TEXT("permessi")
			? Archivista->ImpostaPermessi(Suo->GetNomeAccount(), Valore, Me)
			: (Valore == TEXT("staff") || Valore == TEXT("giocatore") ? Archivista->ImpostaRuolo(Suo->GetNomeAccount(), Valore, Me) : FString(TEXT("Il ruolo: giocatore o staff (l'Amministratore si nomina dalla console del server).")));
		Annota(Risposta.Left(80));
		return Risposta;
	}
	return FString::Printf(TEXT("Non conosco l'azione \"%s\"."), *Azione);
}

void UValdorsoStaffComponent::ClientVista_Implementation(const FValdorsoStaffVista& Nuova)
{
	VistaArrivata = Nuova;
	++NumeroCambiamenti;
	APlayerController* Controllore = Cast<APlayerController>(GetOwner());
	if (bApertura && !Schermata.IsValid())
	{
		bApertura = false;
		ValdorsoStaffSchermo::Apri(Controllore);
	}
}

void UValdorsoStaffComponent::ClientEsito_Implementation(const FString& Testo)
{
	UltimoEsito = Testo;
	++NumeroCambiamenti;
	if (!Schermata.IsValid())
	{
		bApertura = false;
		if (AValdorsoPlayerController* Controllore = Cast<AValdorsoPlayerController>(GetOwner()))
		{
			Controllore->ClientAvviso(FText::FromString(Testo), 3.f, 0.f);
		}
	}
}

void UValdorsoStaffComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Schermata.IsValid())
	{
		ValdorsoStaffSchermo::Chiudi(Cast<APlayerController>(GetOwner()));
	}
	Super::EndPlay(EndPlayReason);
}

#undef LOCTEXT_NAMESPACE
