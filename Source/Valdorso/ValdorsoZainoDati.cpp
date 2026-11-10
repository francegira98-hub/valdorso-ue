// Valdorso - Lo zaino: i dati e le regole (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoZainoDati.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoRegistro.h"

namespace ValdorsoZaino
{
	void Sistema(FValdorsoZaino& Zaino)
	{
		Zaino.Posti.SetNum(NumeroPosti);
		for (FValdorsoPostoZaino& Posto : Zaino.Posti)
		{
			const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Posto.Id);
			if (!Tipo || Posto.Quantita <= 0)
			{
				Posto = FValdorsoPostoZaino();
				continue;
			}
			Posto.Quantita = FMath::Clamp(Posto.Quantita, 1, Tipo->Pila);
			Posto.Cariche = FMath::Clamp(Posto.Cariche, 0, Tipo->Cariche);
			Posto.Numero = Tipo->Pila == 1 ? FMath::Max(0, Posto.Numero) : 0;
		}
		Zaino.Monete = FMath::Clamp(Zaino.Monete, 0, MassimoMonete);
		Zaino.Azioni.SetNum(NumeroAzioni);
		TSet<FName> GiaInBarra;
		for (FName& Azione : Zaino.Azioni)
		{
			// solo quello che si usa, e ogni oggetto in un tasto solo
			if (!DaBarra(Azione) || GiaInBarra.Contains(Azione))
			{
				Azione = NAME_None;
				continue;
			}
			GiaInBarra.Add(Azione);
		}
		SistemaMani(Zaino);
	}

	bool SistemaMani(FValdorsoZaino& Zaino)
	{
		bool bCambiato = false;
		auto Controlla = [&Zaino, &bCambiato](FName& Mano, EValdorsoPosto Voluto)
		{
			if (Mano.IsNone())
			{
				return;
			}
			const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Mano);
			if (!Tipo || Tipo->Posto != Voluto || Conta(Zaino, Mano) <= 0)
			{
				Mano = NAME_None;
				bCambiato = true;
			}
		};
		Controlla(Zaino.Destra, EValdorsoPosto::ManoDestra);
		Controlla(Zaino.Sinistra, EValdorsoPosto::ManoSinistra);
		return bCambiato;
	}

	bool Impugna(FValdorsoZaino& Zaino, FName Id)
	{
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
		if (!Tipo || Tipo->Posto == EValdorsoPosto::Nessuno || Conta(Zaino, Id) <= 0)
		{
			return false;
		}
		FName& Mano = Tipo->Posto == EValdorsoPosto::ManoSinistra ? Zaino.Sinistra : Zaino.Destra;
		if (Mano == Id)
		{
			Mano = NAME_None;
			return false;
		}
		Mano = Id;
		return true;
	}

	int32 Aggiungi(FValdorsoZaino& Zaino, FName Id, int32 Quantita, int32 Cariche)
	{
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
		if (!Tipo || Quantita <= 0)
		{
			return 0;
		}
		if (Zaino.Posti.Num() != NumeroPosti)
		{
			Zaino.Posti.SetNum(NumeroPosti);
		}
		const int32 CaricheNuove = Cariche < 0 ? Tipo->Cariche : FMath::Min(Cariche, Tipo->Cariche);
		int32 Restano = Quantita;
		// prima sulle pile che ci sono (solo per gli oggetti senza cariche: due borracce non si impilano)
		if (Tipo->Pila > 1 && Tipo->Cariche == 0)
		{
			for (FValdorsoPostoZaino& Posto : Zaino.Posti)
			{
				if (Restano <= 0)
				{
					break;
				}
				if (Posto.Id == Id && Posto.Quantita > 0 && Posto.Quantita < Tipo->Pila)
				{
					const int32 Entrano = FMath::Min(Restano, Tipo->Pila - Posto.Quantita);
					Posto.Quantita += Entrano;
					Restano -= Entrano;
				}
			}
		}
		// poi nei posti liberi
		for (FValdorsoPostoZaino& Posto : Zaino.Posti)
		{
			if (Restano <= 0)
			{
				break;
			}
			if (Posto.Vuoto())
			{
				const int32 Entrano = FMath::Min(Restano, Tipo->Pila);
				Posto.Id = Id;
				Posto.Quantita = Entrano;
				Posto.Cariche = CaricheNuove;
				Posto.Numero = 0;
				Restano -= Entrano;
			}
		}
		return Quantita - Restano;
	}

	int32 AggiungiNumerato(FValdorsoZaino& Zaino, FName Id, int32 Numero)
	{
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
		if (!Tipo)
		{
			return INDEX_NONE;
		}
		if (Zaino.Posti.Num() != NumeroPosti)
		{
			Zaino.Posti.SetNum(NumeroPosti);
		}
		for (int32 i = 0; i < Zaino.Posti.Num(); ++i)
		{
			FValdorsoPostoZaino& Posto = Zaino.Posti[i];
			if (Posto.Vuoto())
			{
				Posto = FValdorsoPostoZaino();
				Posto.Id = Id;
				Posto.Quantita = 1;
				Posto.Cariche = Tipo->Cariche;
				Posto.Numero = Numero;
				return i;
			}
		}
		return INDEX_NONE;
	}

	int32 PostoNumerato(const FValdorsoZaino& Zaino, FName Id, int32 Numero)
	{
		return Zaino.Posti.IndexOfByPredicate([Id, Numero](const FValdorsoPostoZaino& Posto)
		{
			return !Posto.Vuoto() && Posto.Id == Id && Posto.Numero == Numero;
		});
	}

	bool Togli(FValdorsoZaino& Zaino, int32 Posto, int32 Quantita)
	{
		if (!Zaino.Posti.IsValidIndex(Posto) || Quantita <= 0 || Zaino.Posti[Posto].Vuoto() || Zaino.Posti[Posto].Quantita < Quantita)
		{
			return false;
		}
		FValdorsoPostoZaino& Dove = Zaino.Posti[Posto];
		Dove.Quantita -= Quantita;
		if (Dove.Quantita <= 0)
		{
			Dove = FValdorsoPostoZaino();
		}
		return true;
	}

	bool TogliOggetto(FValdorsoZaino& Zaino, FName Id, int32 Quantita)
	{
		if (Quantita <= 0 || Conta(Zaino, Id) < Quantita)
		{
			return false;
		}
		int32 Restano = Quantita;
		for (int32 i = Zaino.Posti.Num() - 1; i >= 0 && Restano > 0; --i)
		{
			FValdorsoPostoZaino& Posto = Zaino.Posti[i];
			if (Posto.Vuoto() || Posto.Id != Id)
			{
				continue;
			}
			const int32 Via = FMath::Min(Restano, Posto.Quantita);
			Togli(Zaino, i, Via);
			Restano -= Via;
		}
		return true;
	}

	int32 Conta(const FValdorsoZaino& Zaino, FName Id)
	{
		int32 Totale = 0;
		for (const FValdorsoPostoZaino& Posto : Zaino.Posti)
		{
			if (!Posto.Vuoto() && Posto.Id == Id)
			{
				Totale += Posto.Quantita;
			}
		}
		return Totale;
	}

	int32 PostoDi(const FValdorsoZaino& Zaino, FName Id)
	{
		int32 Primo = INDEX_NONE;
		for (int32 i = 0; i < Zaino.Posti.Num(); ++i)
		{
			const FValdorsoPostoZaino& Posto = Zaino.Posti[i];
			if (Posto.Vuoto() || Posto.Id != Id)
			{
				continue;
			}
			const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
			// per la borraccia: meglio una che ha ancora dei sorsi
			if (!Tipo || Tipo->Cariche == 0 || Posto.Cariche > 0)
			{
				return i;
			}
			if (Primo == INDEX_NONE)
			{
				Primo = i;
			}
		}
		return Primo;
	}

	bool Sposta(FValdorsoZaino& Zaino, int32 Da, int32 A)
	{
		if (Da == A || !Zaino.Posti.IsValidIndex(Da) || !Zaino.Posti.IsValidIndex(A) || Zaino.Posti[Da].Vuoto())
		{
			return false;
		}
		FValdorsoPostoZaino& Sorgente = Zaino.Posti[Da];
		FValdorsoPostoZaino& Arrivo = Zaino.Posti[A];
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Sorgente.Id);
		if (!Arrivo.Vuoto() && Arrivo.Id == Sorgente.Id && Tipo && Tipo->Pila > 1 && Tipo->Cariche == 0)
		{
			// la stessa pila: si unisce fin dove sta (se è già piena, si scambia: cioè niente)
			const int32 Entrano = FMath::Min(Sorgente.Quantita, Tipo->Pila - Arrivo.Quantita);
			if (Entrano <= 0)
			{
				return false;
			}
			Arrivo.Quantita += Entrano;
			Sorgente.Quantita -= Entrano;
			if (Sorgente.Quantita <= 0)
			{
				Sorgente = FValdorsoPostoZaino();
			}
			return true;
		}
		Swap(Sorgente, Arrivo);
		return true;
	}

	float Peso(const FValdorsoZaino& Zaino)
	{
		float Totale = Zaino.Monete * ValdorsoOggetti::PesoMoneta;
		for (const FValdorsoPostoZaino& Posto : Zaino.Posti)
		{
			if (const FValdorsoTipoOggetto* Tipo = Posto.Vuoto() ? nullptr : ValdorsoOggetti::Trova(Posto.Id))
			{
				Totale += Tipo->Peso * Posto.Quantita;
			}
		}
		return Totale;
	}

	bool SiPuoGettare(const FValdorsoZaino& Zaino, int32 Posto)
	{
		if (!Zaino.Posti.IsValidIndex(Posto) || Zaino.Posti[Posto].Vuoto())
		{
			return false;
		}
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Zaino.Posti[Posto].Id);
		return Tipo && !Tipo->bRicordo;
	}

	bool DaBarra(FName Id)
	{
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
		return Tipo && Tipo->Uso != EValdorsoUso::Niente;
	}

	bool Assegna(FValdorsoZaino& Zaino, int32 Tasto, FName Id)
	{
		if (Tasto < 0 || Tasto >= NumeroAzioni || (!Id.IsNone() && !DaBarra(Id)))
		{
			return false;
		}
		Zaino.Azioni.SetNum(NumeroAzioni);
		if (Zaino.Azioni[Tasto] == Id)
		{
			return false;
		}
		// un oggetto sta in un tasto solo: se c'era già da un'altra parte, lì si vuota
		if (!Id.IsNone())
		{
			for (FName& Altro : Zaino.Azioni)
			{
				if (Altro == Id)
				{
					Altro = NAME_None;
				}
			}
		}
		Zaino.Azioni[Tasto] = Id;
		return true;
	}

	void DaiPartenza(FValdorsoZaino& Zaino, const FValdorsoRegistro* Registro)
	{
		Sistema(Zaino);
		for (const TPair<FName, int32>& Voce : ValdorsoOggetti::Partenza(Registro))
		{
			Aggiungi(Zaino, Voce.Key, Voce.Value);
		}
		Zaino.Monete = FMath::Clamp(Zaino.Monete + ValdorsoOggetti::MonetePartenza(Registro), 0, MassimoMonete);
		// i primi tasti: 1 il pane, 2 la borraccia (se sono liberi)
		const FName Primi[] = { TEXT("Pane_Nero"), TEXT("Borraccia") };
		for (int32 i = 0; i < UE_ARRAY_COUNT(Primi); ++i)
		{
			if (Zaino.Azioni[i].IsNone() && !Zaino.Azioni.Contains(Primi[i]))
			{
				Zaino.Azioni[i] = Primi[i];
			}
		}
		Zaino.bPartenzaData = true;
		Zaino.Aggiornamenti = UltimoAggiornamento;
	}

	bool Aggiorna(FValdorsoZaino& Zaino)
	{
		bool bDato = false;
		if (!Zaino.bPartenzaData)
		{
			return false;
		}
		if (Zaino.Aggiornamenti < 1)
		{
			// 1: la mappa della valle (prima il tasto M la apriva a tutti)
			if (Conta(Zaino, TEXT("Mappa_Valle")) == 0)
			{
				if (Aggiungi(Zaino, TEXT("Mappa_Valle"), 1) == 0)
				{
					return false;  // lo zaino è pieno: si riprova la prossima volta che il personaggio entra
				}
				bDato = true;
			}
		}
		Zaino.Aggiornamenti = FMath::Max(Zaino.Aggiornamenti, UltimoAggiornamento);
		return bDato;
	}

	bool RiempiBorracce(FValdorsoZaino& Zaino)
	{
		bool bRiempite = false;
		for (FValdorsoPostoZaino& Posto : Zaino.Posti)
		{
			const FValdorsoTipoOggetto* Tipo = Posto.Vuoto() ? nullptr : ValdorsoOggetti::Trova(Posto.Id);
			if (Tipo && Tipo->Uso == EValdorsoUso::Bevi && Posto.Cariche < Tipo->Cariche)
			{
				Posto.Cariche = Tipo->Cariche;
				bRiempite = true;
			}
		}
		return bRiempite;
	}
}
