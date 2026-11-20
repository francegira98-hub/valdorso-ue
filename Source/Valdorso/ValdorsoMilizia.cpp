// Valdorso - La milizia (vedi ValdorsoMilizia.h).

#include "ValdorsoMilizia.h"
#include "ValdorsoMonete.h"
#include "ValdorsoContratto.h"
#include "ValdorsoFiducia.h"
#include "ValdorsoStatoColono.h"
#include "ValdorsoPlayerController.h"
#include "ValdorsoPNG.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoRegistroRonda.h"
#include "ValdorsoCampanella.h"
#include "ValdorsoTaccuino.h"
#include "ValdorsoZaino.h"
#include "ValdorsoZainoDati.h"
#include "Valdorso.h"
#include "ValdorsoCielo.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "ValdorsoMilizia"

namespace
{
	const TCHAR* const MiliziaTutteLeChiavi[] = { TEXT("Chiave_Caserma"), TEXT("Chiave_Torri"), TEXT("Chiave_Prigione"), TEXT("Chiave_Porta") };

	/** Nello zaino: toglie le chiavi che non spettano a quel grado e aggiunge quelle che mancano. Vero se ha cambiato. */
	bool MiliziaSistemaChiavi(FValdorsoZaino& Zaino, uint8 Grado)
	{
		const TArray<FName> Spettano = ValdorsoMilizia::ChiaviDelGrado(Grado);
		bool bCambiato = false;
		for (const TCHAR* Nome : MiliziaTutteLeChiavi)
		{
			const FName Chiave(Nome);
			const int32 Quante = ValdorsoZaino::Conta(Zaino, Chiave);
			if (Spettano.Contains(Chiave))
			{
				if (Quante < 1 && ValdorsoZaino::Aggiungi(Zaino, Chiave, 1) > 0)
				{
					bCambiato = true;
				}
			}
			else if (Quante > 0 && ValdorsoZaino::TogliOggetto(Zaino, Chiave, Quante))
			{
				bCambiato = true;
			}
		}
		return bCambiato;
	}

	void MiliziaAvvisa(APlayerController* Chi, const FText& Testo)
	{
		if (AValdorsoPlayerController* Giocatore = Cast<AValdorsoPlayerController>(Chi))
		{
			Giocatore->ClientAvviso(Testo, 5.f, 0.f);
		}
	}

	/** Vero se nello zaino manca una delle chiavi del suo grado (zaino pieno quando Livia gliele ha date): lo avvisa. */
	bool MiliziaMancanoChiavi(APlayerController* Chi)
	{
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		if (!Zaino)
		{
			return false;
		}
		for (const FName& Chiave : ValdorsoMilizia::ChiaviDelGrado(ValdorsoMilizia::GradoDi(Chi)))
		{
			if (ValdorsoZaino::Conta(Zaino->Contenuto(), Chiave) < 1)
			{
				MiliziaAvvisa(Chi, LOCTEXT("ZainoPieno", "Lo zaino e' pieno: una chiave e' rimasta alla capitana. Libera un posto e chiedigliela di nuovo."));
				return true;
			}
		}
		return false;
	}

	void MiliziaAnnota(APlayerController* Chi, AActor* Png, const FString& Cosa)
	{
		if (const APawn* Colono = Chi ? Chi->GetPawn() : nullptr)
		{
			ValdorsoRegistroEventi::Annota(TEXT("Milizia"), Colono, Png, Colono->GetActorLocation(), Cosa);
		}
	}
}

namespace
{
	int64 MiliziaAdesso()
	{
		return FDateTime::UtcNow().ToUnixTimestamp();
	}

	FString MiliziaNomeDi(const APlayerController* Chi)
	{
		const APlayerState* Stato = Chi ? Chi->GetPlayerState<APlayerState>() : nullptr;
		return Stato ? Stato->GetPlayerName() : FString(TEXT("?"));
	}

	/** Livia lo prende? Serve la sua fiducia (chi entra senza un personaggio salvato, dall'editor, non ha un taccuino:
	 *  passa) e nessuna colpa aperta. */
	bool MiliziaPuoEntrare(APlayerController* Chi)
	{
		const AValdorsoPlayerState* Stato = Chi ? Chi->GetPlayerState<AValdorsoPlayerState>() : nullptr;
		if (!Chi || (Stato && Stato->Colpevole()))
		{
			return false;
		}
		const UValdorsoTaccuinoComponent* Taccuino = UValdorsoTaccuinoComponent::Di(Chi);
		return !Taccuino || !Taccuino->HaPersonaggio()
			|| ValdorsoFiducia::Livello(ValdorsoFiducia::Di(Taccuino->Dati(), FName(TEXT("Livia")))) >= EValdorsoFiducia::Cordiale;
	}

	// ------------------------------------------------------------------------------------------------ i turni (19/11)
	// Un turno in corso sta solo nella memoria del server (se il server riparte, o il giocatore esce, il turno e' perso:
	// se ne prende un altro). Una volta al secondo si guarda dove sono i giocatori in turno.

	struct FMiliziaTurno
	{
		TWeakObjectPtr<APlayerController> Chi;
		uint8 Tipo = 0;                 // ValdorsoMilizia::TurnoRonda o TurnoGuardia
		TArray<FName> Mancano;          // la ronda: da chi deve ancora farsi vedere (il Dialogo del PNG), o i registri
		                                // ancora da firmare (la Tappa del registro)
		bool bRegistri = false;         // la ronda: si firma ai registri (vero) o ci si fa vedere dagli abitanti (falso)
		int32 Quanti = 0;               // la ronda: quanti erano all'inizio
		float Secondi = 0.f;            // la guardia: i secondi fatti al posto
		int32 MinutiDetti = 0;          // la guardia: l'ultimo minuto annunciato
		double Scade = 0.0;             // FPlatformTime::Seconds() oltre cui il turno e' perso
		double Ultimo = 0.0;            // FPlatformTime::Seconds() dell'ultimo giro (per contare il tempo vero passato)
		bool bFinito = false;           // fatto: resta da tornare dalla capitana
		FVector Posto = FVector::ZeroVector;   // la guardia in garitta: dove deve stare
	};

	TArray<FMiliziaTurno> MiliziaTurni;
	FTSTicker::FDelegateHandle MiliziaGiro;

	/** I punti della ronda: gli abitanti ai quattro angoli del villaggio (il Dialogo del PNG e come lo si nomina). */
	struct FMiliziaTappa
	{
		const TCHAR* Dialogo;
		const TCHAR* Nome;
	};
	const FMiliziaTappa MiliziaTappe[] = {
		{ TEXT("Bruno"), TEXT("Bruno, alla locanda") },
		{ TEXT("Marta"), TEXT("Marta, alla forgia") },
		{ TEXT("PadreAnselmo"), TEXT("Padre Anselmo, al tempio") },
		{ TEXT("Teodoro"), TEXT("Teodoro, al mulino") },
	};

	FString MiliziaNomeTappa(FName Dialogo)
	{
		for (const FMiliziaTappa& Tappa : MiliziaTappe)
		{
			if (Dialogo == FName(Tappa.Dialogo))
			{
				return Tappa.Nome;
			}
		}
		return Dialogo.ToString();
	}

	FMiliziaTurno* MiliziaTurnoDi(const APlayerController* Chi)
	{
		return MiliziaTurni.FindByPredicate([Chi](const FMiliziaTurno& Turno) { return Turno.Chi.Get() == Chi; });
	}

	void MiliziaTogliTurno(const APlayerController* Chi)
	{
		MiliziaTurni.RemoveAll([Chi](const FMiliziaTurno& Turno) { return !Turno.Chi.IsValid() || Turno.Chi.Get() == Chi; });
	}

	/** Il PNG con quel dialogo che c'e' adesso nel mondo (non quelli partiti); nullptr se non c'e'. */
	AValdorsoPNG* MiliziaPng(UWorld* Mondo, FName Dialogo)
	{
		if (Mondo)
		{
			for (TActorIterator<AValdorsoPNG> It(Mondo); It; ++It)
			{
				if (*It && It->Dialogo == Dialogo && !It->Via() && !It->IsHidden())
				{
					return *It;
				}
			}
		}
		return nullptr;
	}

	/** Una volta al secondo: fa avanzare i turni in corso. Rende falso (e si ferma) quando non ce ne sono piu'. */
	bool MiliziaGiraTurni(float /*DurataDelFotogramma*/)
	{
		const double Adesso = FPlatformTime::Seconds();
		for (int32 i = MiliziaTurni.Num() - 1; i >= 0; --i)
		{
			FMiliziaTurno& Turno = MiliziaTurni[i];
			APlayerController* Chi = Turno.Chi.Get();
			if (!Chi)
			{
				MiliziaTurni.RemoveAt(i);
				continue;
			}
			// (secondo controllo) il tempo vero passato dall'ultimo giro: il numero che arriva qui e' la durata di un
			// fotogramma, non il secondo fra un giro e l'altro
			const float Passo = static_cast<float>(FMath::Clamp(Adesso - Turno.Ultimo, 0.0, 3.0));
			Turno.Ultimo = Adesso;
			if (Turno.bFinito)
			{
				// fatto, ma non e' mai tornato a farselo segnare: dopo un giorno della valle si dimentica
				if (Adesso > Turno.Scade + static_cast<double>(ValdorsoMilizia::SecondiPerGiorno))
				{
					MiliziaTurni.RemoveAt(i);
				}
				continue;
			}
			if (Adesso > Turno.Scade)
			{
				MiliziaAvvisa(Chi, LOCTEXT("TurnoScaduto", "Il turno e' scaduto: troppo tempo. Torna dalla capitana per prenderne un altro."));
				MiliziaTurni.RemoveAt(i);
				continue;
			}
			const APawn* Corpo = Chi->GetPawn();
			UWorld* Mondo = Chi->GetWorld();
			if (!Corpo || !Mondo)
			{
				continue;
			}
			const FVector Qui = Corpo->GetActorLocation();
			if (Turno.Tipo == ValdorsoMilizia::TurnoRonda && Turno.bRegistri)
			{
				// la ronda dei registri va avanti quando firma (ValdorsoMilizia::FirmaRegistro), non qui
			}
			else if (Turno.Tipo == ValdorsoMilizia::TurnoRonda)
			{
				for (int32 t = Turno.Mancano.Num() - 1; t >= 0; --t)
				{
					const AValdorsoPNG* Png = MiliziaPng(Mondo, Turno.Mancano[t]);
					if (!Png)
					{
						// oggi quell'abitante non c'e' (o in questa valle non c'e' proprio): la tappa e' fatta
						Turno.Mancano.RemoveAt(t);
						continue;
					}
					if (FVector::DistSquared(Png->GetActorLocation(), Qui) <= FMath::Square(ValdorsoMilizia::RaggioRonda))
					{
						const FString Nome = MiliziaNomeTappa(Turno.Mancano[t]);
						Turno.Mancano.RemoveAt(t);
						if (!Turno.Mancano.IsEmpty())
						{
							MiliziaAvvisa(Chi, FText::Format(LOCTEXT("RondaTappa", "Ronda: sei passato da {0}. Ne mancano {1}."),
								FText::FromString(Nome), FText::AsNumber(Turno.Mancano.Num())));
						}
					}
				}
				if (Turno.Mancano.IsEmpty())
				{
					Turno.bFinito = true;
					MiliziaAvvisa(Chi, LOCTEXT("RondaFinita", "Ronda finita: il giro e' fatto. Torna dalla capitana a farti segnare il turno."));
				}
			}
			else if (Turno.Tipo == ValdorsoMilizia::TurnoGaritta)
			{
				// (20/11) nella garitta della caserma: sullo stesso piano, a due passi dal punto
				if (FVector::DistSquared2D(Turno.Posto, Qui) <= FMath::Square(ValdorsoMilizia::RaggioGaritta) && FMath::Abs(Turno.Posto.Z - Qui.Z) < 300.0)
				{
					Turno.Secondi += Passo;
					const int32 Minuti = FMath::FloorToInt32(Turno.Secondi / 60.f);
					if (Turno.Secondi >= ValdorsoMilizia::SecondiDiGuardia)
					{
						Turno.bFinito = true;
						MiliziaAvvisa(Chi, LOCTEXT("GarittaFinita", "Guardia finita: il cambio e' arrivato. Torna dalla capitana a farti segnare il turno."));
					}
					else if (Minuti > Turno.MinutiDetti)
					{
						Turno.MinutiDetti = Minuti;
						const int32 Restano = FMath::CeilToInt32((ValdorsoMilizia::SecondiDiGuardia - Turno.Secondi) / 60.f);
						MiliziaAvvisa(Chi, Restano <= 1
							? LOCTEXT("GarittaUnMinuto", "Di guardia in garitta: ancora un minuto.")
							: FText::Format(LOCTEXT("GarittaMinuti", "Di guardia in garitta: ancora {0} minuti."), FText::AsNumber(Restano)));
					}
				}
			}
			else if (Turno.Tipo == ValdorsoMilizia::TurnoGuardia)
			{
				// al portale del passo, accanto a Tebaldo (se Tebaldo non c'e', accanto alla capitana)
				const AValdorsoPNG* Posto = MiliziaPng(Mondo, FName(TEXT("GuardiaCorrado")));
				if (!Posto)
				{
					Posto = MiliziaPng(Mondo, FName(TEXT("Livia")));
				}
				if (Posto && FVector::DistSquared(Posto->GetActorLocation(), Qui) <= FMath::Square(ValdorsoMilizia::RaggioGuardia))
				{
					Turno.Secondi += Passo;
					const int32 Minuti = FMath::FloorToInt32(Turno.Secondi / 60.f);
					if (Turno.Secondi >= ValdorsoMilizia::SecondiDiGuardia)
					{
						Turno.bFinito = true;
						MiliziaAvvisa(Chi, LOCTEXT("GuardiaFinita", "Guardia finita: il cambio e' arrivato. Torna dalla capitana a farti segnare il turno."));
					}
					else if (Minuti > Turno.MinutiDetti)
					{
						Turno.MinutiDetti = Minuti;
						const int32 Restano = FMath::CeilToInt32((ValdorsoMilizia::SecondiDiGuardia - Turno.Secondi) / 60.f);
						MiliziaAvvisa(Chi, Restano <= 1
							? LOCTEXT("GuardiaUnMinuto", "Di guardia al passo: ancora un minuto.")
							: FText::Format(LOCTEXT("GuardiaMinuti", "Di guardia al passo: ancora {0} minuti."), FText::AsNumber(Restano)));
					}
				}
			}
		}
		if (MiliziaTurni.IsEmpty())
		{
			MiliziaGiro.Reset();
			return false;
		}
		return true;
	}

	void MiliziaAvviaGiro()
	{
		if (!MiliziaGiro.IsValid())
		{
			MiliziaGiro = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&MiliziaGiraTurni), 1.f);
		}
	}

	/** Il numero del giorno della valle di adesso (per "un turno al giorno"). */
	int64 MiliziaGiornoDiOggi()
	{
		return MiliziaAdesso() / ValdorsoMilizia::SecondiPerGiorno;
	}
}

namespace ValdorsoMilizia
{
	const TCHAR* NomeGrado(uint8 Grado)
	{
		switch (Grado)
		{
		case Recluta: return TEXT("recluta");
		case Guardia: return TEXT("guardia");
		case Sergente: return TEXT("sergente");
		case ViceComandante: return TEXT("vice comandante");
		case Comandante: return TEXT("comandante");
		default: return TEXT("");
		}
	}

	FString Qualifica(uint8 Grado)
	{
		switch (Grado)
		{
		case Recluta: return TEXT("Recluta delle Guardie di Valdorso");
		case Guardia: return TEXT("Guardia di Valdorso");
		case Sergente: return TEXT("Sergente delle Guardie di Valdorso");
		case ViceComandante: return TEXT("Vice Comandante delle Guardie di Valdorso");
		case Comandante: return TEXT("Comandante delle Guardie di Valdorso");
		default: return FString();
		}
	}

	int32 SoldoAlGiorno(uint8 Grado)
	{
		switch (Grado)
		{
		case Recluta: return 3;
		case Guardia: return 5;
		case Sergente: return 8;
		case ViceComandante: return 12;
		case Comandante: return 16;
		default: return 0;
		}
	}

	TArray<FName> ChiaviDelGrado(uint8 Grado)
	{
		TArray<FName> Chiavi;
		if (Grado >= Recluta)
		{
			Chiavi.Add(FName(TEXT("Chiave_Caserma")));
		}
		if (Grado >= Guardia)
		{
			Chiavi.Add(FName(TEXT("Chiave_Prigione")));
		}
		// (18/11, Fra) le torri delle mura e le porte: solo chi comanda
		if (Grado >= ViceComandante)
		{
			Chiavi.Add(FName(TEXT("Chiave_Torri")));
			Chiavi.Add(FName(TEXT("Chiave_Porta")));
		}
		return Chiavi;
	}

	uint8 GradoDopoITurni(uint8 Grado, int32 Turni)
	{
		if (Grado == Nessuno)
		{
			return Nessuno;
		}
		uint8 Spetta = Recluta;
		if (Turni >= TurniPerSergente)
		{
			Spetta = Sergente;
		}
		else if (Turni >= TurniPerGuardia)
		{
			Spetta = Guardia;
		}
		return FMath::Max(Grado, Spetta);
	}

	int32 GiorniAllaFineDellaFerma(int64 DalUtc, int64 AdessoUtc)
	{
		if (DalUtc <= 0)
		{
			return 0;
		}
		const int64 Fine = DalUtc + static_cast<int64>(GiorniDiFerma) * SecondiPerGiorno;
		if (AdessoUtc >= Fine)
		{
			return 0;
		}
		// arrotondato in su: "manca un giorno" finche' non e' finita davvero
		return static_cast<int32>((Fine - AdessoUtc + SecondiPerGiorno - 1) / SecondiPerGiorno);
	}

	int32 GiorniDaPagare(int64 PagatoUtc, int64 AdessoUtc)
	{
		if (PagatoUtc <= 0 || AdessoUtc <= PagatoUtc)
		{
			return 0;
		}
		return static_cast<int32>(FMath::Min<int64>((AdessoUtc - PagatoUtc) / SecondiPerGiorno, GiorniArretrati));
	}

	uint8 GradoDi(const APlayerController* Chi)
	{
		const UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		return Zaino ? FMath::Min<uint8>(Zaino->Contenuto().GradoMilizia, Comandante) : Nessuno;
	}

	FValdorsoContrattoVista Contratto()
	{
		FValdorsoContrattoVista Vista;
		Vista.Tipo = TipoContratto;
		Vista.Intestazione = TEXT("BALIVATO DI VAL D'ORSO");
		Vista.Titolo = TEXT("Contratto di arruolamento nelle Guardie di Valdorso");
		Vista.Righe.Add(TEXT("Io sottoscritto, colono della valle dell'Orso, di mia volonta' e senza costrizione, entro nelle Guardie di Valdorso con il grado di recluta."));
		Vista.Righe.Add(TEXT("Giuro di servire il Balivo e la gente della valle, di obbedire al Comandante delle Guardie e a chi ne tiene il posto, di fare i turni che mi sono dati e di tacere quello che vedo in servizio."));
		Vista.Righe.Add(FString::Printf(TEXT("Mi lego per una ferma di %d giorni: prima di allora non posso lasciare il servizio."), GiorniDiFerma));
		Vista.Righe.Add(FString::Printf(TEXT("Ricevo dal Balivo un soldo di %d Lucio per ogni giorno in cui ho fatto il mio turno, che cresce con il grado, un letto in caserma e le chiavi del mio grado. Le chiavi sono del Balivo: le rendo il giorno che lascio."),
			SoldoAlGiorno(Recluta)));
		Vista.Righe.Add(TEXT("Chi porta questa divisa e alza le mani sulla gente, ruba o tradisce, e' cacciato lo stesso giorno, e questo contratto resta scritto come rotto."));
		Vista.Righe.Add(TEXT("Questo foglio e' conservato fra i documenti del Balivo."));
		Vista.Controparte = TEXT("Per il Balivo, mancando il Comandante: L. Serra, capitana della Guardia Reale");
		return Vista;
	}

	bool MettiGrado(APlayerController* Chi, uint8 Grado)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		if (!Chi || !Chi->HasAuthority() || !Zaino)
		{
			return false;
		}
		Grado = FMath::Min(Grado, Comandante);
		const int64 Adesso = MiliziaAdesso();
		return Zaino->Cambia([Grado, Adesso](FValdorsoZaino& Dentro)
		{
			bool bCambiato = Dentro.GradoMilizia != Grado;
			Dentro.GradoMilizia = Grado;
			if (Grado == Nessuno)
			{
				if (Dentro.TurniMilizia != 0 || Dentro.MiliziaDalUtc != 0 || Dentro.MiliziaPagatoUtc != 0
					|| Dentro.MiliziaGiorniSoldo != 0 || Dentro.MiliziaGiornoTurno != 0)
				{
					Dentro.TurniMilizia = 0;
					Dentro.MiliziaDalUtc = 0;
					Dentro.MiliziaPagatoUtc = 0;
					Dentro.MiliziaGiorniSoldo = 0;
					Dentro.MiliziaGiornoTurno = 0;
					bCambiato = true;
				}
			}
			else if (Dentro.MiliziaDalUtc <= 0)
			{
				// entra adesso (o e' un personaggio arruolato prima del 18/11): la ferma e il soldo partono da qui
				Dentro.MiliziaDalUtc = Adesso;
				Dentro.MiliziaPagatoUtc = Adesso;
				bCambiato = true;
			}
			else if (Dentro.MiliziaPagatoUtc <= 0)
			{
				// (secondo controllo) un file senza la data del soldo: senza, non verrebbe pagato mai
				Dentro.MiliziaPagatoUtc = Adesso;
				bCambiato = true;
			}
			return MiliziaSistemaChiavi(Dentro, Grado) || bCambiato;
		});
	}

	void Espelli(APlayerController* Chi)
	{
		if (GradoDi(Chi) == Nessuno)
		{
			return;
		}
		MiliziaTogliTurno(Chi);
		MettiGrado(Chi, Nessuno);
		if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi))
		{
			const int64 Adesso = MiliziaAdesso();
			if (Zaino->Cambia([Adesso](FValdorsoZaino& Dentro)
				{
					return ValdorsoContratti::Chiudi(Dentro.Documenti, TipoContratto, ValdorsoContratti::Rotto, Adesso);
				}))
			{
				ValdorsoContratti::ScriviAlBalivo(MiliziaNomeDi(Chi), TipoContratto, Contratto().Titolo, TEXT("rotto: cacciato per una colpa"), FString());
			}
		}
		MiliziaAvvisa(Chi, LOCTEXT("Espulso", "Chi porta la divisa non alza le mani sulla gente. Sei fuori dalle Guardie di Valdorso: le chiavi tornano al Balivo, e il tuo contratto resta scritto come rotto."));
		MiliziaAnnota(Chi, nullptr, TEXT("cacciato dalle guardie per una colpa"));
	}

	// ---------------------------------------------------------------------------- le firme sui registri della ronda
	// (19/11, chiesto da Fra) Ogni registro tiene le sue firme: nome, giorno e ora. Stanno sul server, un file per
	// registro (Saved/Server/Ronda/<Tappa>.txt, una riga per firma, le ultime RondaFirmeTenute), e chi usa il registro
	// le legge su un foglio (la pergamena del contratto, senza firma: UValdorsoZainoComponent::MostraFoglio).
	namespace
	{
		constexpr int32 RondaFirmeTenute = 60;
		constexpr int32 RondaFirmeMostrate = 12;

		TMap<FName, TArray<FString>> RondaFirme;

		FString RondaFile(FName Tappa)
		{
			// solo lettere e cifre nel nome del file (la Tappa la scrive lo script, ma non si sa mai)
			FString Pulito;
			for (const TCHAR C : Tappa.ToString())
			{
				Pulito.AppendChar(FChar::IsAlnum(C) ? C : TEXT('_'));
			}
			return FPaths::ProjectSavedDir() / TEXT("Server") / TEXT("Ronda") / ((Pulito.IsEmpty() ? FString(TEXT("Registro")) : Pulito) + TEXT(".txt"));
		}

		TArray<FString>& RondaFirmeDi(FName Tappa)
		{
			if (TArray<FString>* Gia = RondaFirme.Find(Tappa))
			{
				return *Gia;
			}
			TArray<FString>& Nuove = RondaFirme.Add(Tappa);
			FFileHelper::LoadFileToStringArray(Nuove, *RondaFile(Tappa));
			Nuove.RemoveAll([](const FString& Riga) { return Riga.TrimStartAndEnd().IsEmpty(); });
			return Nuove;
		}

		/** "19 novembre, ore 14:30" (il giorno vero, l'ora della valle). */
		FString RondaQuando(UWorld* Mondo)
		{
			static const TCHAR* Mesi[] = { TEXT("gennaio"), TEXT("febbraio"), TEXT("marzo"), TEXT("aprile"), TEXT("maggio"), TEXT("giugno"),
				TEXT("luglio"), TEXT("agosto"), TEXT("settembre"), TEXT("ottobre"), TEXT("novembre"), TEXT("dicembre") };
			const FDateTime Oggi = FDateTime::Now();
			FString Testo = FString::Printf(TEXT("%d %s"), Oggi.GetDay(), Mesi[FMath::Clamp(Oggi.GetMonth() - 1, 0, 11)]);
			if (Mondo)
			{
				for (TActorIterator<AValdorsoCielo> It(Mondo); It; ++It)
				{
					const float Ora = FMath::Fmod(FMath::Max(It->Ora(), 0.f), 24.f);
					const int32 Ore = FMath::FloorToInt32(Ora);
					const int32 Minuti = FMath::Clamp(FMath::FloorToInt32((Ora - Ore) * 60.f), 0, 59);
					Testo += FString::Printf(TEXT(", ore %d:%02d"), Ore, Minuti);
					break;
				}
			}
			return Testo;
		}

		void RondaAggiungiFirma(FName Tappa, const FString& Nome, UWorld* Mondo)
		{
			TArray<FString>& Firme = RondaFirmeDi(Tappa);
			Firme.Add(FString::Printf(TEXT("%s  -  %s"), *Nome.Replace(TEXT("\n"), TEXT(" ")).Replace(TEXT("\r"), TEXT(" ")), *RondaQuando(Mondo)));
			if (Firme.Num() > RondaFirmeTenute)
			{
				Firme.RemoveAt(0, Firme.Num() - RondaFirmeTenute);
			}
			const FString File = RondaFile(Tappa);
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
			if (!FFileHelper::SaveStringArrayToFile(Firme, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
			{
				UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Ronda: non riesco a scrivere %s"), *File);
			}
		}

		/** Fa leggere il foglio del registro a quel giocatore: le ultime firme, la piu' nuova in cima. */
		void RondaMostraFoglio(APlayerController* Chi, const AValdorsoRegistroRonda* Libro, const FString& Esito)
		{
			UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
			if (!Zaino || !Libro)
			{
				return;
			}
			FValdorsoContrattoVista Vista;
			Vista.Tipo = FName(TEXT("RegistroRonda"));
			Vista.Intestazione = TEXT("GUARDIE DI VALDORSO");
			Vista.Titolo = FString::Printf(TEXT("Registro della ronda: %s"), Libro->Luogo.IsEmpty() ? *Libro->Tappa.ToString() : *Libro->Luogo);
			const TArray<FString>& Firme = RondaFirmeDi(Libro->Tappa);
			for (int32 i = Firme.Num() - 1; i >= 0 && Vista.Righe.Num() < RondaFirmeMostrate; --i)
			{
				Vista.Righe.Add(Firme[i]);
			}
			if (Vista.Righe.IsEmpty())
			{
				Vista.Righe.Add(TEXT("(La pagina e' ancora bianca: nessuna guardia ha firmato qui.)"));
			}
			else if (Firme.Num() > Vista.Righe.Num())
			{
				Vista.Righe.Add(TEXT("(Le pagine di prima sono piene di altre firme.)"));
			}
			Vista.Controparte = Esito.IsEmpty() ? FString(TEXT("Chi firma risponde del suo giro alla capitana.")) : Esito;
			Zaino->MostraFoglio(Vista);
		}
	}

	void FirmaRegistro(APlayerController* Chi, AActor* Registro)
	{
		const AValdorsoRegistroRonda* Libro = Cast<AValdorsoRegistroRonda>(Registro);
		if (!Chi || !Libro || Libro->Tappa.IsNone())
		{
			return;
		}
		// (19/11) in ogni caso gli si apre il foglio con le firme: quando esce di qui (firma messa o no)
		// (secondo controllo) l'esito si scrive anche in fondo al foglio: l'avviso resterebbe sotto, scurito
		FString Esito;
		ON_SCOPE_EXIT { RondaMostraFoglio(Chi, Libro, Esito); };
		const auto Dico = [Chi, &Esito](const FText& Testo)
		{
			MiliziaAvvisa(Chi, Testo);
			Esito = Testo.ToString();
		};
		if (GradoDi(Chi) == 0)
		{
			Dico(LOCTEXT("RegistroNonGuardia", "Il registro della ronda: righe di firme e di ore. Qui firmano le Guardie di Valdorso."));
			return;
		}
		FMiliziaTurno* Turno = MiliziaTurnoDi(Chi);
		if (!Turno || Turno->Tipo != TurnoRonda)
		{
			Dico(LOCTEXT("RegistroNonDiRonda", "Non sei di ronda: i turni li da' la capitana Livia, al passo."));
			return;
		}
		if (Turno->bFinito)
		{
			Dico(LOCTEXT("RegistroGiroFatto", "Il giro e' gia' fatto: torna dalla capitana a farti segnare il turno."));
			return;
		}
		if (!Turno->bRegistri)
		{
			Dico(LOCTEXT("RegistroAltraRonda", "Per questa ronda basta farsi vedere dagli abitanti: il registro lo firmerai dalla prossima."));
			return;
		}
		if (Turno->Mancano.Remove(Libro->Tappa) == 0)
		{
			Dico(LOCTEXT("RegistroGiaFirmato", "Qui hai gia' firmato. Vai agli altri registri."));
			return;
		}
		const FString Dove = Libro->Luogo.IsEmpty() ? Libro->Tappa.ToString() : Libro->Luogo;
		UValdorsoZainoComponent* ZainoDiChi = UValdorsoZainoComponent::Di(Chi);
		const FString NomeIntero = ZainoDiChi ? ZainoDiChi->NomeSullaTessera() : FString();
		RondaAggiungiFirma(Libro->Tappa, NomeIntero.IsEmpty() ? MiliziaNomeDi(Chi) : NomeIntero, Chi->GetWorld());
		MiliziaAnnota(Chi, Registro, FString::Printf(TEXT("ronda: firma il registro (%s)"), *Dove));
		if (Turno->Mancano.IsEmpty())
		{
			Turno->bFinito = true;
			Dico(LOCTEXT("RondaFinitaRegistri", "Ultima firma: il giro e' fatto. Torna dalla capitana a farti segnare il turno."));
		}
		else
		{
			Dico(FText::Format(LOCTEXT("RegistroFirmato", "Hai firmato il registro: {0}. Ne mancano {1}."),
				FText::FromString(Dove), FText::AsNumber(Turno->Mancano.Num())));
		}
	}

	bool Firmato(APlayerController* Chi, AActor* Png, const FString& Firma)
	{
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		if (!Chi || !Chi->HasAuthority() || !Zaino || GradoDi(Chi) != Nessuno)
		{
			return false;
		}
		if (!MiliziaPuoEntrare(Chi))
		{
			MiliziaAvvisa(Chi, LOCTEXT("FirmaNo", "La capitana ritira il foglio: per ora non ti prende."));
			return false;
		}
		MettiGrado(Chi, Recluta);
		if (GradoDi(Chi) != Recluta)
		{
			return false;
		}
		const FString Titolo = Contratto().Titolo;
		const int64 Adesso = MiliziaAdesso();
		Zaino->Cambia([&Titolo, Adesso](FValdorsoZaino& Dentro)
		{
			ValdorsoContratti::Aggiungi(Dentro.Documenti, TipoContratto, Titolo, Adesso);
			return true;
		});
		ValdorsoContratti::ScriviAlBalivo(MiliziaNomeDi(Chi), TipoContratto, Titolo, TEXT("firmato"), Firma);
		if (!MiliziaMancanoChiavi(Chi))
		{
			MiliziaAvvisa(Chi, LOCTEXT("Arruolato", "Hai firmato: sei una recluta delle Guardie di Valdorso. Nello zaino c'e' la chiave della caserma, e il contratto e' fra i documenti del Balivo."));
		}
		MiliziaAnnota(Chi, Png, TEXT("ha firmato il contratto: recluta"));
		return true;
	}

	bool EventoDialogo(FName Evento, APlayerController* Chi, AActor* Png, FName& InOutProssimo)
	{
		const bool bChiedi = Evento == FName(TEXT("MiliziaChiedi"));
		const bool bContratto = Evento == FName(TEXT("MiliziaContratto"));
		const bool bChiavi = Evento == FName(TEXT("MiliziaChiavi"));
		const bool bPaga = Evento == FName(TEXT("MiliziaPaga"));
		const bool bLasciaChiedi = Evento == FName(TEXT("MiliziaLasciaChiedi"));
		const bool bLascia = Evento == FName(TEXT("MiliziaLascia"));
		const bool bRonda = Evento == FName(TEXT("MiliziaRonda"));
		const bool bGuardia = Evento == FName(TEXT("MiliziaGuardia"));
		if ((!bChiedi && !bContratto && !bChiavi && !bPaga && !bLasciaChiedi && !bLascia && !bRonda && !bGuardia) || !Chi)
		{
			return false;
		}
		UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Chi);
		if (!Zaino)
		{
			InOutProssimo = FName(TEXT("milizia_no"));
			return true;
		}
		const uint8 Grado = GradoDi(Chi);
		if (Grado == Nessuno)
		{
			// non e' delle guardie: puo' solo chiedere di entrare, e leggere il contratto
			if (!MiliziaPuoEntrare(Chi))
			{
				InOutProssimo = FName(TEXT("milizia_no"));
				return true;
			}
			if (!bContratto)
			{
				InOutProssimo = FName(TEXT("milizia_offerta"));
				return true;
			}
			// il dialogo si chiude, e AValdorsoPNG apre la pergamena (UValdorsoZainoComponent::OffriContratto)
			InOutProssimo = NAME_None;
			return true;
		}
		// e' delle guardie. Chi e' stato arruolato prima del 18/11 non ha la data: parte da adesso
		// (secondo controllo) se cosi' gli tornano anche le chiavi, piu' sotto vale come "chiavi ridate"
		bool bGiaCambiato = false;
		if (Zaino->Contenuto().MiliziaDalUtc <= 0 || Zaino->Contenuto().MiliziaPagatoUtc <= 0)
		{
			bGiaCambiato = MettiGrado(Chi, Grado);
		}
		const int64 Adesso = MiliziaAdesso();
		if (bLasciaChiedi || bLascia)
		{
			const int32 Mancano = GiorniAllaFineDellaFerma(Zaino->Contenuto().MiliziaDalUtc, Adesso);
			if (Mancano > 0)
			{
				MiliziaAvvisa(Chi, Mancano == 1
					? LOCTEXT("FermaUno", "Il contratto ti lega ancora per un giorno.")
					: FText::Format(LOCTEXT("Ferma", "Il contratto ti lega ancora per {0} giorni."), FText::AsNumber(Mancano)));
				InOutProssimo = FName(TEXT("milizia_ferma"));
				return true;
			}
			if (bLasciaChiedi)
			{
				InOutProssimo = FName(TEXT("milizia_lascia"));
				return true;
			}
			MiliziaTogliTurno(Chi);
			MettiGrado(Chi, Nessuno);
			if (Zaino->Cambia([Adesso](FValdorsoZaino& Dentro)
				{
					return ValdorsoContratti::Chiudi(Dentro.Documenti, TipoContratto, ValdorsoContratti::Concluso, Adesso);
				}))
			{
				ValdorsoContratti::ScriviAlBalivo(MiliziaNomeDi(Chi), TipoContratto, Contratto().Titolo, TEXT("concluso: ferma finita, ha lasciato"), FString());
			}
			MiliziaAvvisa(Chi, LOCTEXT("Lasciata", "Hai lasciato le Guardie di Valdorso con la ferma onorata. Le chiavi sono tornate al Balivo."));
			MiliziaAnnota(Chi, Png, TEXT("ha lasciato le guardie (ferma finita)"));
			InOutProssimo = FName(TEXT("milizia_lasciata"));
			return true;
		}
		if (bPaga)
		{
			// (19/11) si pagano i giorni in cui ha fatto il turno (al massimo GiorniArretrati tenuti da parte)
			const int32 Giorni = FMath::Clamp(Zaino->Contenuto().MiliziaGiorniSoldo, 0, GiorniArretrati);
			const int32 Soldo = Giorni * SoldoAlGiorno(Grado);
			if (Soldo <= 0)
			{
				InOutProssimo = FName(TEXT("milizia_paga_no"));
				return true;
			}
			Zaino->Cambia([Soldo, Adesso](FValdorsoZaino& Dentro)
			{
				Dentro.Monete = FMath::Min(Dentro.Monete + Soldo, ValdorsoZaino::MassimoMonete);
				Dentro.MiliziaGiorniSoldo = 0;
				Dentro.MiliziaPagatoUtc = Adesso;
				return true;
			});
			MiliziaAvvisa(Chi, Giorni == 1
				? FText::Format(LOCTEXT("SoldoUnoMonete", "Il soldo di un giorno: {0}."), ValdorsoMonete::Scritto(Soldo))
				: FText::Format(LOCTEXT("SoldoMonete", "Il soldo di {0} giorni: {1}."), FText::AsNumber(Giorni), ValdorsoMonete::Scritto(Soldo)));
			MiliziaAnnota(Chi, Png, FString::Printf(TEXT("soldo: %d giorni, %d Lucio"), Giorni, Soldo));
			InOutProssimo = FName(TEXT("milizia_paga"));
			return true;
		}
		// (19/11) i turni: la ronda e la guardia al passo
		FMiliziaTurno* InCorso = MiliziaTurnoDi(Chi);
		if (bRonda || bGuardia)
		{
			if (InCorso)
			{
				InOutProssimo = FName(TEXT("milizia_turno_gia"));
				return true;
			}
			if (Zaino->Contenuto().MiliziaGiornoTurno == MiliziaGiornoDiOggi())
			{
				InOutProssimo = FName(TEXT("milizia_turno_domani"));
				return true;
			}
			FMiliziaTurno Preso;
			Preso.Chi = Chi;
			Preso.Tipo = bRonda ? TurnoRonda : TurnoGuardia;
			Preso.Scade = FPlatformTime::Seconds() + SecondiPerUnTurno;
			Preso.Ultimo = FPlatformTime::Seconds();
			if (bRonda)
			{
				// (19/11 pomeriggio) se nel villaggio ci sono i registri della ronda (almeno due), il giro e' quello: si
				// firma a ognuno, di giorno e di notte. Altrimenti, come prima, ci si fa vedere dagli abitanti
				FString Luoghi;
				for (TActorIterator<AValdorsoRegistroRonda> It(Chi->GetWorld()); It; ++It)
				{
					if (*It && !It->IsHidden() && !It->Tappa.IsNone() && !Preso.Mancano.Contains(It->Tappa))
					{
						Preso.Mancano.Add(It->Tappa);
						Luoghi += (Luoghi.IsEmpty() ? TEXT("") : TEXT("; ")) + (It->Luogo.IsEmpty() ? It->Tappa.ToString() : It->Luogo);
					}
				}
				if (Preso.Mancano.Num() >= 2)
				{
					Preso.bRegistri = true;
					Preso.Quanti = Preso.Mancano.Num();
					MiliziaAvvisa(Chi, FText::Format(LOCTEXT("RondaRegistri", "Ronda: firma i registri ({0}). Poi torna dalla capitana."), FText::FromString(Luoghi)));
					MiliziaTurni.Add(Preso);
					MiliziaAvviaGiro();
					MiliziaAnnota(Chi, Png, TEXT("turno: comincia la ronda dei registri"));
					InOutProssimo = FName(TEXT("milizia_ronda"));
					return true;
				}
				Preso.Mancano.Reset();
				for (const FMiliziaTappa& Tappa : MiliziaTappe)
				{
					if (MiliziaPng(Chi->GetWorld(), FName(Tappa.Dialogo)))
					{
						Preso.Mancano.Add(FName(Tappa.Dialogo));
					}
				}
				Preso.Quanti = Preso.Mancano.Num();
				if (Preso.Quanti < 2)
				{
					// (secondo controllo) di notte gli abitanti sono a casa o via: una ronda senza nessuno da cui farsi
					// vedere sarebbe regalata. A quell'ora resta la guardia al passo
					InOutProssimo = FName(TEXT("milizia_turno_notte"));
					return true;
				}
				FString Elenco;
				for (const FName& Chiave : Preso.Mancano)
				{
					Elenco += (Elenco.IsEmpty() ? TEXT("") : TEXT("; ")) + MiliziaNomeTappa(Chiave);
				}
				MiliziaAvvisa(Chi, FText::Format(LOCTEXT("RondaParte", "Ronda: fatti vedere da {0}. Poi torna dalla capitana."), FText::FromString(Elenco)));
			}
			else
			{
				// (20/11, v0.2.7.1) un giorno si' e uno no, se la caserma c'e' (fase 3), la guardia e' nella sua garitta
				const AValdorsoCampanella* Caserma = nullptr;
				for (TActorIterator<AValdorsoCampanella> It(Chi->GetWorld()); It; ++It)
				{
					if (*It && !It->IsHidden() && !It->Garitta.IsZero())
					{
						Caserma = *It;
						break;
					}
				}
				if (Caserma && (MiliziaGiornoDiOggi() % 2) == 1)
				{
					Preso.Tipo = TurnoGaritta;
					Preso.Posto = Caserma->Garitta;
					MiliziaAvvisa(Chi, FText::Format(LOCTEXT("GarittaParte", "Guardia in garitta: {0} minuti nella garitta della caserma, accanto al portale. Poi torna dalla capitana."),
						FText::AsNumber(FMath::RoundToInt32(SecondiDiGuardia / 60.f))));
					MiliziaTurni.Add(Preso);
					MiliziaAvviaGiro();
					MiliziaAnnota(Chi, Png, TEXT("turno: comincia la guardia in garitta"));
					InOutProssimo = FName(TEXT("milizia_garitta"));
					return true;
				}
				MiliziaAvvisa(Chi, FText::Format(LOCTEXT("GuardiaParte", "Guardia al passo: {0} minuti al portale, accanto a Tebaldo. Poi torna dalla capitana."),
					FText::AsNumber(FMath::RoundToInt32(SecondiDiGuardia / 60.f))));
			}
			MiliziaTurni.Add(Preso);
			MiliziaAvviaGiro();
			MiliziaAnnota(Chi, Png, bRonda ? TEXT("turno: comincia la ronda") : TEXT("turno: comincia la guardia al passo"));
			InOutProssimo = FName(bRonda ? TEXT("milizia_ronda") : TEXT("milizia_guardia"));
			return true;
		}
		bool bTurnoSegnato = false;
		if (bChiedi && InCorso && InCorso->bFinito)
		{
			// torna dalla capitana a turno finito: lo segna (uno al giorno) e gli mette da parte il soldo di oggi
			const int64 Oggi = MiliziaGiornoDiOggi();
			const bool bRondaFatta = InCorso->Tipo == TurnoRonda;
			MiliziaTogliTurno(Chi);
			InCorso = nullptr;
			// (secondo controllo) "segnato" solo se lo e' davvero (uno al giorno)
			bTurnoSegnato = Zaino->Cambia([Oggi](FValdorsoZaino& Dentro)
			{
				if (Dentro.MiliziaGiornoTurno == Oggi)
				{
					return false;
				}
				Dentro.MiliziaGiornoTurno = Oggi;
				Dentro.TurniMilizia = FMath::Min(Dentro.TurniMilizia + 1, 100000);
				Dentro.MiliziaGiorniSoldo = FMath::Min(Dentro.MiliziaGiorniSoldo + 1, GiorniArretrati);
				return true;
			});
			if (bTurnoSegnato)
			{
				MiliziaAvvisa(Chi, FText::Format(LOCTEXT("TurnoSegnato", "Turno segnato: ne hai fatti {0}. Il soldo di oggi ti aspetta dalla capitana."),
					FText::AsNumber(Zaino->Contenuto().TurniMilizia)));
				MiliziaAnnota(Chi, Png, bRondaFatta ? TEXT("turno fatto: ronda") : TEXT("turno fatto: guardia"));
			}
		}
		// chiede (o rivuole le chiavi): se i turni bastano sale di grado, e in ogni caso riceve le chiavi che gli spettano
		const uint8 Nuovo = GradoDopoITurni(Grado, Zaino->Contenuto().TurniMilizia);
		const bool bCambiato = MettiGrado(Chi, Nuovo) || bGiaCambiato;
		const bool bMancano = MiliziaMancanoChiavi(Chi);
		if (Nuovo > Grado)
		{
			if (!bMancano)
			{
				MiliziaAvvisa(Chi, FText::Format(LOCTEXT("Promosso", "Sei salito di grado: ora sei {0} delle Guardie di Valdorso."),
					FText::FromString(NomeGrado(Nuovo))));
			}
			MiliziaAnnota(Chi, Png, FString::Printf(TEXT("promosso: %s"), NomeGrado(Nuovo)));
			InOutProssimo = FName(TEXT("milizia_promosso"));
			return true;
		}
		if (bMancano)
		{
			// lo zaino e' pieno (avviso gia' dato): niente "le hai tutte"
			InOutProssimo = FName(TEXT("milizia_dentro"));
			return true;
		}
		if (bChiavi && bCambiato)
		{
			MiliziaAvvisa(Chi, LOCTEXT("ChiaviRidate", "La capitana ti ha ridato le chiavi del tuo grado."));
			MiliziaAnnota(Chi, Png, TEXT("chiavi ridate"));
		}
		if (bTurnoSegnato)
		{
			InOutProssimo = FName(TEXT("milizia_turno_fatto"));
			return true;
		}
		InOutProssimo = bChiavi ? FName(bCambiato ? TEXT("milizia_chiavi") : TEXT("milizia_chiavi_no")) : FName(TEXT("milizia_dentro"));
		return true;
	}
}

// ---------------------------------------------------------------------------------------------------- per le prove

namespace
{
	/** I giocatori sul server con quel nome (tutti con il nome vuoto), come nei comandi della fiducia. */
	TArray<APlayerController*> MiliziaGiocatori(const TArray<FString>& Argomenti, int32 Da)
	{
		const FString Nome = Argomenti.IsValidIndex(Da) ? Argomenti[Da] : FString();
		TArray<APlayerController*> Trovati;
		for (TObjectIterator<AValdorsoPlayerState> It; It; ++It)
		{
			AValdorsoPlayerState* Stato = *It;
			const UWorld* MondoSuo = IsValid(Stato) ? Stato->GetWorld() : nullptr;
			if (!MondoSuo || !MondoSuo->IsGameWorld() || !Stato->HasAuthority() || !Stato->GetPlayerController())
			{
				continue;
			}
			if (Nome.IsEmpty() || Stato->GetPlayerName().Equals(Nome, ESearchCase::IgnoreCase))
			{
				Trovati.Add(Stato->GetPlayerController());
			}
		}
		return Trovati;
	}

	void MiliziaComandoGrado(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		const TArray<APlayerController*> Chi = MiliziaGiocatori(Argomenti, 1);
		if (Argomenti.Num() < 1 || Chi.IsEmpty())
		{
			Uscita.Log(TEXT("Valdorso.Milizia.Grado <0-5> [nome]: 0 fuori, 1 recluta, 2 guardia, 3 sergente, 4 vice comandante, 5 comandante (serve essere in gioco)"));
			return;
		}
		const uint8 Grado = static_cast<uint8>(FMath::Clamp(FCString::Atoi(*Argomenti[0]), 0, 5));
		for (APlayerController* Giocatore : Chi)
		{
			ValdorsoMilizia::MettiGrado(Giocatore, Grado);
			Uscita.Logf(TEXT("%s: grado %d %s, con le sue chiavi (sul server)"),
				*Giocatore->GetPlayerState<APlayerState>()->GetPlayerName(), Grado, ValdorsoMilizia::NomeGrado(Grado));
		}
	}

	void MiliziaComandoTurni(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		const TArray<APlayerController*> Chi = MiliziaGiocatori(Argomenti, 1);
		if (Argomenti.Num() < 1 || Chi.IsEmpty())
		{
			Uscita.Log(TEXT("Valdorso.Milizia.Turni <n> [nome]: mette i turni fatti (5 = guardia, 20 = sergente: poi si parla con Livia)"));
			return;
		}
		const int32 Turni = FMath::Max(0, FCString::Atoi(*Argomenti[0]));
		for (APlayerController* Giocatore : Chi)
		{
			if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Giocatore))
			{
				Zaino->Cambia([Turni](FValdorsoZaino& Dentro)
				{
					const bool bCambiato = Dentro.TurniMilizia != Turni;
					Dentro.TurniMilizia = Turni;
					return bCambiato;
				});
				Uscita.Logf(TEXT("%s: %d turni nella milizia (sul server)"),
					*Giocatore->GetPlayerState<APlayerState>()->GetPlayerName(), Turni);
			}
		}
	}

	/** Per provare la ferma e il soldo: finge che il servizio sia cominciato n giorni della valle fa (e che da allora
	 *  il soldo non sia stato ritirato). */
	void MiliziaComandoGiorni(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		const TArray<APlayerController*> Chi = MiliziaGiocatori(Argomenti, 1);
		if (Argomenti.Num() < 1 || Chi.IsEmpty())
		{
			Uscita.Log(TEXT("Valdorso.Milizia.Giorni <n> [nome]: finge n giorni di servizio gia' fatti (30 = ferma finita; poi si parla con Livia)"));
			return;
		}
		const int64 Giorni = FMath::Clamp(FCString::Atoi(*Argomenti[0]), 0, 10000);
		const int64 Da = FDateTime::UtcNow().ToUnixTimestamp() - Giorni * ValdorsoMilizia::SecondiPerGiorno;
		for (APlayerController* Giocatore : Chi)
		{
			UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Giocatore);
			if (!Zaino || Zaino->Contenuto().GradoMilizia == ValdorsoMilizia::Nessuno)
			{
				Uscita.Logf(TEXT("%s: non e' delle guardie"), *Giocatore->GetPlayerState<APlayerState>()->GetPlayerName());
				continue;
			}
			Zaino->Cambia([Da](FValdorsoZaino& Dentro)
			{
				Dentro.MiliziaDalUtc = Da;
				Dentro.MiliziaPagatoUtc = Da;
				return true;
			});
			Uscita.Logf(TEXT("%s: in servizio da %d giorni della valle (sul server)"),
				*Giocatore->GetPlayerState<APlayerState>()->GetPlayerName(), static_cast<int32>(Giorni));
		}
	}

	/** Per provare i turni senza fare il giro: da' per fatto il turno in corso (o una ronda, se non ne ha uno). */
	void MiliziaComandoTurno(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		const TArray<APlayerController*> Chi = MiliziaGiocatori(Argomenti, 0);
		if (Chi.IsEmpty())
		{
			Uscita.Log(TEXT("Valdorso.Milizia.Turno [nome]: da' per fatto il turno in corso, o una ronda (poi si parla con Livia). Serve essere in gioco"));
			return;
		}
		for (APlayerController* Giocatore : Chi)
		{
			if (ValdorsoMilizia::GradoDi(Giocatore) == ValdorsoMilizia::Nessuno)
			{
				Uscita.Logf(TEXT("%s: non e' delle guardie"), *Giocatore->GetPlayerState<APlayerState>()->GetPlayerName());
				continue;
			}
			FMiliziaTurno* Turno = MiliziaTurnoDi(Giocatore);
			if (!Turno)
			{
				FMiliziaTurno Finto;
				Finto.Chi = Giocatore;
				Finto.Tipo = ValdorsoMilizia::TurnoRonda;
				Finto.Scade = FPlatformTime::Seconds() + ValdorsoMilizia::SecondiPerUnTurno;
				Finto.Ultimo = FPlatformTime::Seconds();
				Turno = &MiliziaTurni[MiliziaTurni.Add(Finto)];
				MiliziaAvviaGiro();
			}
			Turno->Mancano.Reset();
			Turno->bFinito = true;
			Uscita.Logf(TEXT("%s: turno dato per fatto (ora parla con Livia)"), *Giocatore->GetPlayerState<APlayerState>()->GetPlayerName());
		}
	}

	/** Per provare piu' turni di fila: toglie "per oggi hai gia' fatto il turno". */
	void MiliziaComandoDomani(const TArray<FString>& Argomenti, UWorld* Mondo, FOutputDevice& Uscita)
	{
		const TArray<APlayerController*> Chi = MiliziaGiocatori(Argomenti, 0);
		if (Chi.IsEmpty())
		{
			Uscita.Log(TEXT("Valdorso.Milizia.Domani [nome]: toglie il segno del turno di oggi (se ne puo' prendere un altro). Serve essere in gioco"));
			return;
		}
		for (APlayerController* Giocatore : Chi)
		{
			if (UValdorsoZainoComponent* Zaino = UValdorsoZainoComponent::Di(Giocatore))
			{
				Zaino->Cambia([](FValdorsoZaino& Dentro)
				{
					const bool bCambiato = Dentro.MiliziaGiornoTurno != 0;
					Dentro.MiliziaGiornoTurno = 0;
					return bCambiato;
				});
				Uscita.Logf(TEXT("%s: puo' prendere un altro turno"), *Giocatore->GetPlayerState<APlayerState>()->GetPlayerName());
			}
		}
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice MiliziaComando1(TEXT("Valdorso.Milizia.Grado"),
		TEXT("Mette il grado nella milizia e le chiavi che spettano. Valdorso.Milizia.Grado <0-5> [nome]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&MiliziaComandoGrado));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice MiliziaComando2(TEXT("Valdorso.Milizia.Turni"),
		TEXT("Mette i turni fatti nella milizia (per provare le promozioni). Valdorso.Milizia.Turni <n> [nome]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&MiliziaComandoTurni));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice MiliziaComando3(TEXT("Valdorso.Milizia.Giorni"),
		TEXT("Finge n giorni di servizio gia' fatti (per provare la ferma e il soldo). Valdorso.Milizia.Giorni <n> [nome]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&MiliziaComandoGiorni));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice MiliziaComando4(TEXT("Valdorso.Milizia.Turno"),
		TEXT("Da' per fatto il turno in corso, o una ronda (poi si parla con Livia). Valdorso.Milizia.Turno [nome]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&MiliziaComandoTurno));
	FAutoConsoleCommandWithWorldArgsAndOutputDevice MiliziaComando5(TEXT("Valdorso.Milizia.Domani"),
		TEXT("Toglie il segno del turno di oggi (per provare piu' turni di fila). Valdorso.Milizia.Domani [nome]"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&MiliziaComandoDomani));
}

#undef LOCTEXT_NAMESPACE
