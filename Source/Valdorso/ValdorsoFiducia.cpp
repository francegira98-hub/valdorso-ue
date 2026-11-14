// Valdorso - La fiducia dei PNG, la valle che ricorda e la colpa (vedi il .h). Scritto da Claude il 14/11/2026.
// Unity build: le cose del namespace anonimo cominciano con Fiducia.

#include "ValdorsoFiducia.h"
#include "ValdorsoPNG.h"
#include "ValdorsoStatoColono.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "ValdorsoChat.h"
#include "Valdorso.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "ValdorsoFiducia"

namespace
{
	/** Il nome di un dialogo come sta nel file (come ValdorsoTaccuinoDati.cpp: senza spazi ai lati, non troppo lungo). */
	FString FiduciaNome(FName IdDialogo)
	{
		return IdDialogo.IsNone() ? FString() : IdDialogo.ToString().TrimStartAndEnd().Left(ValdorsoTaccuino::MassimoNomeDialogo);
	}

	const FValdorsoRicordoDialogo* FiduciaTrova(const FValdorsoTaccuino& Taccuino, FName IdDialogo)
	{
		const FString Nome = FiduciaNome(IdDialogo);
		if (Nome.IsEmpty())
		{
			return nullptr;
		}
		return Taccuino.Dialoghi.FindByPredicate([&Nome](const FValdorsoRicordoDialogo& R) { return R.Dialogo.Equals(Nome, ESearchCase::IgnoreCase); });
	}

	/** Il ricordo da cambiare (lo aggiunge se non c'e'; nullptr se il nome e' vuoto o la memoria e' piena). */
	FValdorsoRicordoDialogo* FiduciaDaCambiare(FValdorsoTaccuino& Taccuino, FName IdDialogo)
	{
		const FString Nome = FiduciaNome(IdDialogo);
		if (Nome.IsEmpty())
		{
			return nullptr;
		}
		FValdorsoRicordoDialogo* Trovato = Taccuino.Dialoghi.FindByPredicate([&Nome](const FValdorsoRicordoDialogo& R) { return R.Dialogo.Equals(Nome, ESearchCase::IgnoreCase); });
		if (Trovato || Taccuino.Dialoghi.Num() >= ValdorsoTaccuino::MassimoDialoghi)
		{
			return Trovato;
		}
		FValdorsoRicordoDialogo& Nuovo = Taccuino.Dialoghi.AddDefaulted_GetRef();
		Nuovo.Dialogo = Nome;
		return &Nuovo;
	}

	/** Le righe dei fatti: {Quando} diventa "Ieri" o "L'altro giorno". Due per tipo, senza maschile ne' femminile. */
	struct FFiduciaRigaFatto
	{
		const TCHAR* Tipo;
		const TCHAR* Testo;
	};

	const FFiduciaRigaFatto FiduciaRighe[] = {
		{ TEXT("Animale"), TEXT("{Quando} la caccia ti e' andata bene, si dice. L'odore di bosco ce l'hai ancora addosso.") },
		{ TEXT("Animale"), TEXT("{Quando} ti hanno visto tornare dal bosco con una preda. Gaspare dira' che hai avuto fortuna.") },
		{ TEXT("Scheletro"), TEXT("{Quando} hai rotto le ossa a un morto che camminava, mi dicono. Spero che fosse morto davvero.") },
		{ TEXT("Scheletro"), TEXT("{Quando} sei sceso tra le ossa, eh? Si vede: hai ancora la polvere nei capelli.") },
		{ TEXT("Storia"), TEXT("{Quando} hai chiuso una storia. In valle se ne parla: non tutti ci riescono.") },
		{ TEXT("Storia"), TEXT("{Quando} hai fatto una cosa che la valle si ricordera'. Io me la ricordo gia'.") },
		{ TEXT("Cerimonia"), TEXT("{Quando} il tempio ti ha fatto dei nostri. Cittadino di Valdorso: suona bene, no?") },
		{ TEXT("Cerimonia"), TEXT("{Quando} Padre Anselmo ha detto il tuo nome davanti alla pietra. Adesso sei di qui.") },
		{ TEXT("Sogno"), TEXT("{Quando} hai sognato, vero? Si vede dagli occhi. Il Battito parla, a chi dorme bene.") },
		{ TEXT("Sogno"), TEXT("{Quando}, di notte, qualcosa ti ha parlato nel sonno. Non chiedo cosa. Ma lo vedo.") },
		{ TEXT("Morte"), TEXT("{Quando} il Cuore ti ha riportato al tempio. Non farci l'abitudine.") },
		{ TEXT("Morte"), TEXT("{Quando} sei caduto, ho sentito. Il Cuore ti ha voluto indietro: ringrazialo.") },
		{ TEXT("Forziere"), TEXT("{Quando} hai trovato un forziere, eh? Le monete fanno rumore, in tasca.") },
		{ TEXT("Forziere"), TEXT("{Quando} sei tornato con le tasche piu' pesanti. La grotta ti ha voluto bene.") },
		{ TEXT("PortaSegreta"), TEXT("{Quando} hai trovato una porta che non voleva essere trovata. Io non te l'ho chiesto.") },
		{ TEXT("PortaSegreta"), TEXT("{Quando} hai aperto qualcosa che era chiuso da tanto. Certe cose si chiudono per un motivo.") },
		{ TEXT("Trappola"), TEXT("{Quando} ti e' scattata una trappola sotto i piedi, mi dicono. Le grotte non perdonano la fretta.") },
		{ TEXT("Trappola"), TEXT("{Quando} una trappola degli antichi ha provato a prenderti. Guarda dove metti i piedi.") },
		{ TEXT("Crescere"), TEXT("{Quando} qualcosa in te e' cresciuto. Lo si vede da come tieni le mani.") },
		{ TEXT("Crescere"), TEXT("{Quando} hai imparato qualcosa di nuovo, eh? Si vede. Il mestiere entra nelle ossa.") },
		{ TEXT("Passaggio"), TEXT("{Quando} sei sceso nelle gallerie del nord. Pochi ne escono con quella faccia.") },
		{ TEXT("Passaggio"), TEXT("{Quando} hai trovato la strada sotto la montagna. Le gallerie si ricordano di chi passa.") },
		{ TEXT("Gatto"), TEXT("{Quando} un gatto ti ha scelto. I gatti non scelgono a caso.") },
		{ TEXT("Gatto"), TEXT("{Quando} ti hanno visto con un gatto alle calcagna. Trattalo bene: lui ti guarda.") },
		{ TEXT("Magazzino"), TEXT("{Quando} hai portato roba al magazzino. La valle se ne ricorda, quando ha fame.") },
		{ TEXT("Magazzino"), TEXT("{Quando} il magazzino si e' riempito un po' grazie a te. Grazie, colono.") },
		{ TEXT("Colpa"), TEXT("{Quando} hai alzato le mani su uno di noi. Me lo ricordo, io.") },
		{ TEXT("Colpa"), TEXT("{Quando} hai picchiato qualcuno della valle. Qui le voci corrono piu' delle gambe.") },
		{ TEXT("Multa"), TEXT("{Quando} hai pagato la multa del Balivo. Monete spese male, ma spese.") },
		{ TEXT("Multa"), TEXT("{Quando} le guardie ti hanno fatto pagare. Il Balivo ringrazia, io meno.") },
		{ TEXT("Cella"), TEXT("{Quando} hai dormito in cella, eh? Il pane del Balivo e' duro.") },
		{ TEXT("Cella"), TEXT("{Quando} hai passato la notte dietro le sbarre. Spero che tu ci abbia pensato su.") },
	};

	TArray<const TCHAR*> FiduciaRigheDi(const FString& Tipo)
	{
		TArray<const TCHAR*> Fuori;
		for (const FFiduciaRigaFatto& R : FiduciaRighe)
		{
			if (Tipo.Equals(R.Tipo, ESearchCase::IgnoreCase))
			{
				Fuori.Add(R.Testo);
			}
		}
		return Fuori;
	}

	const TCHAR* FiduciaUno(const TArray<const TCHAR*>& Righe, int32 Seme)
	{
		return Righe.Num() > 0 ? Righe[FMath::Abs(Seme) % Righe.Num()] : TEXT("");
	}

	/** Il taccuino di quel PlayerController, sul server. */
	UValdorsoTaccuinoComponent* FiduciaTaccuino(const APlayerController* Controllore)
	{
		return (Controllore && Controllore->HasAuthority()) ? UValdorsoTaccuinoComponent::Di(Controllore) : nullptr;
	}

	const APlayerController* FiduciaControllore(const AActor* Chi)
	{
		const APawn* Corpo = Cast<APawn>(Chi);
		return Corpo ? Cast<APlayerController>(Corpo->GetController()) : nullptr;
	}
}

namespace ValdorsoFiducia
{
	bool Cade(float DannoAccumulato, float DannoColpo)
	{
		return DannoColpo >= 25.f || DannoAccumulato >= 30.f;
	}

	EValdorsoFiducia Livello(int32 Fiducia)
	{
		if (Fiducia <= -25)
		{
			return EValdorsoFiducia::Diffidente;
		}
		if (Fiducia < 15)
		{
			return EValdorsoFiducia::Neutrale;
		}
		if (Fiducia < 40)
		{
			return EValdorsoFiducia::Cordiale;
		}
		if (Fiducia < 75)
		{
			return EValdorsoFiducia::Amico;
		}
		return EValdorsoFiducia::DiFamiglia;
	}

	const TCHAR* Parola(EValdorsoFiducia Quale)
	{
		switch (Quale)
		{
		case EValdorsoFiducia::Diffidente: return TEXT("Diffidente");
		case EValdorsoFiducia::Cordiale: return TEXT("Cordiale");
		case EValdorsoFiducia::Amico: return TEXT("Amico");
		case EValdorsoFiducia::DiFamiglia: return TEXT("Di famiglia");
		default: return TEXT("Neutrale");
		}
	}

	FLinearColor Colore(EValdorsoFiducia Quale)
	{
		// i colori dei gradi (ValdorsoQualita.h), e un rosso brace per chi non si fida
		switch (Quale)
		{
		case EValdorsoFiducia::Diffidente: return FLinearColor(FColor(0xa0, 0x50, 0x3c));
		case EValdorsoFiducia::Cordiale: return FLinearColor(FColor(0x5f, 0x8a, 0x5a));
		case EValdorsoFiducia::Amico: return FLinearColor(FColor(0x4a, 0x6f, 0xa5));
		case EValdorsoFiducia::DiFamiglia: return FLinearColor(FColor(0xc8, 0x96, 0x2e));
		default: return FLinearColor(FColor(0x7a, 0x6f, 0x5a));
		}
	}

	int32 Applica(int32 Fiducia, int32 Delta)
	{
		return FMath::Clamp(Fiducia + Delta, Minima, Massima);
	}

	int32 DopoChiacchiera(int32 Fiducia)
	{
		if (Fiducia >= TettoChiacchiere)
		{
			return Fiducia;
		}
		return FMath::Min(Fiducia + PerChiacchiera, TettoChiacchiere);
	}

	int32 Di(const FValdorsoTaccuino& Taccuino, FName IdDialogo)
	{
		const FValdorsoRicordoDialogo* Ricordo = FiduciaTrova(Taccuino, IdDialogo);
		return Ricordo ? FMath::Clamp(Ricordo->Fiducia, Minima, Massima) : 0;
	}

	bool Cambia(FValdorsoTaccuino& Taccuino, FName IdDialogo, int32 Delta)
	{
		if (Delta == 0)
		{
			return false;
		}
		FValdorsoRicordoDialogo* Ricordo = FiduciaDaCambiare(Taccuino, IdDialogo);
		if (!Ricordo)
		{
			return false;
		}
		const int32 Nuova = Applica(Ricordo->Fiducia, Delta);
		if (Nuova == Ricordo->Fiducia)
		{
			return false;
		}
		Ricordo->Fiducia = Nuova;
		return true;
	}

	bool Chiacchiera(FValdorsoTaccuino& Taccuino, FName IdDialogo, int64 Oggi)
	{
		FValdorsoRicordoDialogo* Ricordo = FiduciaDaCambiare(Taccuino, IdDialogo);
		if (!Ricordo || Ricordo->UltimoGiorno >= Oggi)
		{
			return false;
		}
		Ricordo->UltimoGiorno = Oggi;
		Ricordo->Fiducia = DopoChiacchiera(FMath::Clamp(Ricordo->Fiducia, Minima, Massima));
		return true;
	}

	bool Memorabile(const FString& Tipo)
	{
		return !Tipo.IsEmpty() && FiduciaRigheDi(Tipo).Num() > 0;
	}

	bool AnnotaFatto(FValdorsoTaccuino& Taccuino, const FString& Tipo, int64 Quando)
	{
		if (!Memorabile(Tipo))
		{
			return false;
		}
		// lo stesso tipo nello stesso giorno: uno solo (l'ultima ora)
		for (FValdorsoFatto& Fatto : Taccuino.Fatti)
		{
			if (Fatto.Tipo.Equals(Tipo, ESearchCase::IgnoreCase) && Giorno(Fatto.Quando) == Giorno(Quando))
			{
				if (Quando <= Fatto.Quando)
				{
					return false;
				}
				Fatto.Quando = Quando;
				return true;
			}
		}
		FValdorsoFatto& Nuovo = Taccuino.Fatti.AddDefaulted_GetRef();
		Nuovo.Tipo = Tipo;
		Nuovo.Quando = Quando;
		while (Taccuino.Fatti.Num() > MassimoFatti)
		{
			Taccuino.Fatti.RemoveAt(0);
		}
		return true;
	}

	bool FattoDaRicordare(const FValdorsoTaccuino& Taccuino, FName IdDialogo, int64 Adesso, FValdorsoFatto& OutFatto)
	{
		const FValdorsoRicordoDialogo* Ricordo = FiduciaTrova(Taccuino, IdDialogo);
		const int64 GiaDetto = Ricordo ? Ricordo->UltimoRicordo : 0;
		const int64 Oggi = Giorno(Adesso);
		const FValdorsoFatto* Migliore = nullptr;
		for (const FValdorsoFatto& Fatto : Taccuino.Fatti)
		{
			const int64 Fa = Oggi - Giorno(Fatto.Quando);
			if (Fa < 1 || Fa > GiorniRicordo || Fatto.Quando <= GiaDetto || !Memorabile(Fatto.Tipo))
			{
				continue;
			}
			if (!Migliore || Fatto.Quando > Migliore->Quando)
			{
				Migliore = &Fatto;
			}
		}
		if (!Migliore)
		{
			return false;
		}
		OutFatto = *Migliore;
		return true;
	}

	bool SegnaRicordato(FValdorsoTaccuino& Taccuino, FName IdDialogo, int64 Quando)
	{
		FValdorsoRicordoDialogo* Ricordo = FiduciaDaCambiare(Taccuino, IdDialogo);
		if (!Ricordo || Quando <= Ricordo->UltimoRicordo)
		{
			return false;
		}
		Ricordo->UltimoRicordo = Quando;
		return true;
	}

	FString RigaDelFatto(const FValdorsoFatto& Fatto, int64 Adesso, int32 Seme)
	{
		const TArray<const TCHAR*> Righe = FiduciaRigheDi(Fatto.Tipo);
		if (Righe.Num() == 0)
		{
			return FString();
		}
		const int64 Fa = Giorno(Adesso) - Giorno(Fatto.Quando);
		FString Riga = FiduciaUno(Righe, Seme);
		Riga.ReplaceInline(TEXT("{Quando}"), Fa <= 1 ? TEXT("Ieri") : TEXT("L'altro giorno"));
		return Riga;
	}

	void SistemaFatti(TArray<FValdorsoFatto>& Fatti)
	{
		TArray<FValdorsoFatto> Letti = MoveTemp(Fatti);
		Fatti.Reset();
		Letti.StableSort([](const FValdorsoFatto& A, const FValdorsoFatto& B) { return A.Quando < B.Quando; });
		FValdorsoTaccuino Lavoro;
		for (const FValdorsoFatto& Fatto : Letti)
		{
			if (Fatto.Quando > 0)
			{
				AnnotaFatto(Lavoro, Fatto.Tipo.TrimStartAndEnd().Left(32), Fatto.Quando);
			}
		}
		Fatti = MoveTemp(Lavoro.Fatti);
	}

	FString SalutoDellaFiducia(EValdorsoFiducia Quale, const FString& Nome, int32 Seme)
	{
		static const TCHAR* Freddi[] = {
			TEXT("Mh. Tu."),
			TEXT("(Ti guarda, e non saluta.)"),
			TEXT("Ti tengo d'occhio, colono."),
			TEXT("Passa pure. Lontano da me, se puoi."),
		};
		static const TCHAR* Amici[] = {
			TEXT("{nome}! Che piacere vederti."),
			TEXT("Ehi, {nome}. Tutto bene, laggiu'?"),
			TEXT("{nome}, passa quando vuoi: per te c'e' sempre tempo."),
		};
		static const TCHAR* Famiglia[] = {
			TEXT("{nome}! Vieni qui, che ti si vede poco."),
			TEXT("Ecco {nome}, uno di casa. Siediti, se hai un momento."),
			TEXT("{nome}... la valle e' piu' tranquilla quando ci sei tu."),
		};
		const int32 Quale2 = FMath::Abs(Seme);
		FString Fuori;
		switch (Quale)
		{
		case EValdorsoFiducia::Diffidente:
			Fuori = Freddi[Quale2 % UE_ARRAY_COUNT(Freddi)];
			break;
		case EValdorsoFiducia::Amico:
			Fuori = Amici[Quale2 % UE_ARRAY_COUNT(Amici)];
			break;
		case EValdorsoFiducia::DiFamiglia:
			Fuori = Famiglia[Quale2 % UE_ARRAY_COUNT(Famiglia)];
			break;
		default:
			return FString();
		}
		Fuori.ReplaceInline(TEXT("{nome}"), Nome.IsEmpty() ? TEXT("colono") : *Nome);
		return Fuori;
	}

	FString SalutoAlColpevole(int32 Seme)
	{
		static const TCHAR* Righe[] = {
			TEXT("Stammi lontano. Le guardie ti stanno cercando."),
			TEXT("(Si scosta, e non ti guarda negli occhi.)"),
			TEXT("Non voglio guai. Vai per la tua strada."),
		};
		return Righe[FMath::Abs(Seme) % UE_ARRAY_COUNT(Righe)];
	}

	FString Grido(bool bVittima, bool bGuardia, int32 Seme)
	{
		static const TCHAR* Vittime[] = {
			TEXT("Ahi! Ma sei matto?"),
			TEXT("Aiuto! Aiuto, guardie!"),
			TEXT("Fermo! Che ti ho fatto?"),
		};
		static const TCHAR* Guardie[] = {
			TEXT("Fermo dove sei, nel nome della Corona!"),
			TEXT("Giu' le mani! Il Balivo sapra' tutto."),
			TEXT("Alt! Non ti muovere!"),
		};
		static const TCHAR* Testimoni[] = {
			TEXT("Guardie! Guardie!"),
			TEXT("Ha colpito qualcuno! Chiamate le guardie!"),
			TEXT("Via, via di qui!"),
			TEXT("Madonna del Cuore... scappa!"),
		};
		const int32 N = FMath::Abs(Seme);
		if (bGuardia)
		{
			return Guardie[N % UE_ARRAY_COUNT(Guardie)];
		}
		return bVittima ? Vittime[N % UE_ARRAY_COUNT(Vittime)] : Testimoni[N % UE_ARRAY_COUNT(Testimoni)];
	}

	// --- sul server ---------------------------------------------------------------------------------------------------

	void DopoAnnota(const FString& Tipo, const AActor* Chi)
	{
		if (!Memorabile(Tipo))
		{
			return;
		}
		if (UValdorsoTaccuinoComponent* Taccuino = FiduciaTaccuino(FiduciaControllore(Chi)))
		{
			Taccuino->AnnotaFatto(Tipo, FDateTime::UtcNow().ToUnixTimestamp());
		}
	}

	void CambiaPer(const APawn* Colono, FName IdDialogo, int32 Delta)
	{
		if (UValdorsoTaccuinoComponent* Taccuino = FiduciaTaccuino(FiduciaControllore(Colono)))
		{
			Taccuino->CambiaFiducia(IdDialogo, Delta);
		}
	}

	int32 DiColono(const APlayerController* Controllore, FName IdDialogo)
	{
		const UValdorsoTaccuinoComponent* Taccuino = FiduciaTaccuino(Controllore);
		return Taccuino ? Taccuino->Fiducia(IdDialogo) : 0;
	}

	void Crimine(APawn* Colono, AValdorsoPNG* Vittima)
	{
		if (!Colono || !Vittima || !Colono->HasAuthority())
		{
			return;
		}
		AValdorsoPlayerState* Stato = Colono->GetPlayerState<AValdorsoPlayerState>();
		// ogni colpo e' una colpa, ma la fiducia e il registro contano una volta ogni SecondiTraColpe
		const bool bConta = Stato ? Stato->Incolpa(Vittima->Guardia()) : true;
		if (!bConta)
		{
			return;
		}
		UWorld* Mondo = Colono->GetWorld();
		const FString NomeVittima = Vittima->NomeBreve();
		ValdorsoRegistroEventi::Annota(TEXT("Colpa"), Colono, Vittima, Vittima->GetActorLocation(),
			FString::Printf(TEXT("ha colpito %s"), *NomeVittima));
		CambiaPer(Colono, Vittima->Dialogo, PerColpito);
		Vittima->Spaventa(Colono, true);
		// chi ha visto
		if (Mondo)
		{
			for (TActorIterator<AValdorsoPNG> It(Mondo); It; ++It)
			{
				AValdorsoPNG* Testimone = *It;
				if (!Testimone || Testimone == Vittima || Testimone->Via()
					|| FVector::Dist(Testimone->GetActorLocation(), Vittima->GetActorLocation()) > RaggioTestimoni)
				{
					continue;
				}
				CambiaPer(Colono, Testimone->Dialogo, PerTestimone);
				Testimone->Spaventa(Colono, false);
			}
		}
	}

	bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Png, FName& InOutProssimo)
	{
		const bool bMulta = Evento == FName(TEXT("Multa"));
		const bool bCella = Evento == FName(TEXT("Cella"));
		if ((!bMulta && !bCella) || !Chi)
		{
			return false;
		}
		APawn* Colono = Chi->GetPawn();
		AValdorsoPlayerState* Stato = Chi->GetPlayerState<AValdorsoPlayerState>();
		if (bMulta)
		{
			UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
			const bool bPagato = Zaino && Zaino->Cambia([](FValdorsoZaino& Z)
			{
				if (Z.Monete < Multa)
				{
					return false;
				}
				Z.Monete -= Multa;
				return true;
			});
			if (!bPagato)
			{
				InOutProssimo = FName(TEXT("non_basta"));
				return true;
			}
			if (Stato)
			{
				Stato->Assolvi();
			}
			if (Colono)
			{
				ValdorsoRegistroEventi::Annota(TEXT("Multa"), Colono, Png, Colono->GetActorLocation(),
					FString::Printf(TEXT("%d Aureus al Balivo"), Multa));
			}
			return true;
		}
		if (Stato)
		{
			Stato->InCella();
		}
		if (Colono)
		{
			ValdorsoRegistroEventi::Annota(TEXT("Cella"), Colono, Png, Colono->GetActorLocation(), TEXT("una notte in cella"));
		}
		return true;
	}
}

// --- I comandi di console (sul server, o giocando da soli) -------------------------------------------------------------
namespace
{
	APlayerController* FiduciaGiocatore(UWorld* Mondo, const TArray<FString>& Argomenti, int32 Da)
	{
		if (!Mondo)
		{
			return nullptr;
		}
		const FString Nome = Argomenti.IsValidIndex(Da) ? Argomenti[Da] : FString();
		for (FConstPlayerControllerIterator It = Mondo->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Giocatore = It->Get();
			const APlayerState* Stato = Giocatore ? Giocatore->PlayerState.Get() : nullptr;
			if (Giocatore && (Nome.IsEmpty() || (Stato && Stato->GetPlayerName().Equals(Nome, ESearchCase::IgnoreCase))))
			{
				return Giocatore;
			}
		}
		return nullptr;
	}

	void FiduciaComandoImposta(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		APlayerController* Giocatore = FiduciaGiocatore(Mondo, Argomenti, 2);
		UValdorsoTaccuinoComponent* Taccuino = Giocatore ? UValdorsoTaccuinoComponent::Di(Giocatore) : nullptr;
		if (Argomenti.Num() < 2 || !Taccuino || !Giocatore->HasAuthority())
		{
			Uscita.Log(TEXT("Valdorso.PNG.Fiducia <dialogo> <-100..100> [nome]  (sul server: per esempio Valdorso.PNG.Fiducia Bruno 50)"));
			return;
		}
		const FName Id(*Argomenti[0]);
		const int32 Voluta = FMath::Clamp(FCString::Atoi(*Argomenti[1]), ValdorsoFiducia::Minima, ValdorsoFiducia::Massima);
		Taccuino->CambiaFiducia(Id, Voluta - Taccuino->Fiducia(Id));
		Uscita.Logf(TEXT("Fiducia di %s: %d (%s)"), *Id.ToString(), Taccuino->Fiducia(Id),
			ValdorsoFiducia::Parola(ValdorsoFiducia::Livello(Taccuino->Fiducia(Id))));
	}

	void FiduciaComandoIeri(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		APlayerController* Giocatore = FiduciaGiocatore(Mondo, Argomenti, 1);
		UValdorsoTaccuinoComponent* Taccuino = Giocatore ? UValdorsoTaccuinoComponent::Di(Giocatore) : nullptr;
		if (Argomenti.Num() < 1 || !Taccuino || !Giocatore->HasAuthority() || !ValdorsoFiducia::Memorabile(Argomenti[0]))
		{
			Uscita.Log(TEXT("Valdorso.PNG.Ieri <tipo> [nome]: un fatto di ieri, per provare i ricordi. Tipi: Animale, Scheletro, Storia, Cerimonia, Sogno, Morte, Forziere, PortaSegreta, Trappola, Crescere, Passaggio, Gatto, Magazzino, Colpa, Multa, Cella"));
			return;
		}
		Taccuino->AnnotaFatto(Argomenti[0], FDateTime::UtcNow().ToUnixTimestamp() - 86400);
		Uscita.Logf(TEXT("Annotato: %s, ieri. Ora passa vicino a un PNG (o parlaci)."), *Argomenti[0]);
	}

	void FiduciaComandoAssolvi(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		APlayerController* Giocatore = FiduciaGiocatore(Mondo, Argomenti, 0);
		AValdorsoPlayerState* Stato = Giocatore ? Giocatore->GetPlayerState<AValdorsoPlayerState>() : nullptr;
		if (!Stato || !Stato->HasAuthority())
		{
			Uscita.Log(TEXT("Valdorso.Colpa.Assolvi [nome] (sul server)"));
			return;
		}
		Stato->Assolvi();
		Uscita.Log(TEXT("Assolto: il nome non e' piu' rosso, le guardie non cercano piu' nessuno."));
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice FiduciaComando1(TEXT("Valdorso.PNG.Fiducia"),
		TEXT("Cambia la fiducia di un PNG verso il colono. Valdorso.PNG.Fiducia <dialogo> <-100..100> [nome]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&FiduciaComandoImposta));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice FiduciaComando2(TEXT("Valdorso.PNG.Ieri"),
		TEXT("Annota un fatto di ieri al colono (per provare i ricordi dei PNG). Valdorso.PNG.Ieri <tipo> [nome]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&FiduciaComandoIeri));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice FiduciaComando3(TEXT("Valdorso.Colpa.Assolvi"),
		TEXT("Toglie la colpa al colono (il nome rosso, le guardie). Valdorso.Colpa.Assolvi [nome]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&FiduciaComandoAssolvi));
}

#undef LOCTEXT_NAMESPACE
