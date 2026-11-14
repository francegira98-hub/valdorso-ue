// Valdorso - Lo zaino: i dati e le regole (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoZainoDati.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoRegistro.h"
#include "ValdorsoScherma.h"
#include "ValdorsoGildaDati.h"
#include "ValdorsoQualita.h"

namespace
{
	/** (12/11) Un posto in mezzo a dei posti (lo zaino o il baule): prima sulle pile uguali (solo oggetti senza cariche e
	 *  senza numero), poi nei posti liberi. Restituisce quanti ne sono entrati. */
	int32 BauleMettiIn(TArray<FValdorsoPostoZaino>& Posti, const FValdorsoPostoZaino& Cosa)
	{
		const FValdorsoTipoOggetto* Tipo = Cosa.Vuoto() ? nullptr : ValdorsoOggetti::Trova(Cosa.Id);
		if (!Tipo)
		{
			return 0;
		}
		int32 Restano = Cosa.Quantita;
		if (Tipo->Pila > 1 && Tipo->Cariche == 0 && Cosa.Numero == 0)
		{
			for (FValdorsoPostoZaino& Posto : Posti)
			{
				if (Restano <= 0)
				{
					break;
				}
				if (Posto.Id == Cosa.Id && Posto.Quantita > 0 && Posto.Quantita < Tipo->Pila && Posto.Numero == 0 && Posto.StessaFattura(Cosa))
				{
					const int32 Entrano = FMath::Min(Restano, Tipo->Pila - Posto.Quantita);
					Posto.Quantita += Entrano;
					Restano -= Entrano;
				}
			}
		}
		for (FValdorsoPostoZaino& Posto : Posti)
		{
			if (Restano <= 0)
			{
				break;
			}
			if (Posto.Vuoto())
			{
				const int32 Entrano = FMath::Min(Restano, FMath::Max(1, Tipo->Pila));
				Posto = Cosa;
				Posto.Quantita = Entrano;
				Restano -= Entrano;
			}
		}
		return Cosa.Quantita - Restano;
	}

	/** (12/11) Mette in ordine i posti del baule letti da un file (come quelli dello zaino). */
	void BauleSistema(TArray<FValdorsoPostoZaino>& Posti)
	{
		Posti.SetNum(ValdorsoZaino::PostiBaule);
		for (FValdorsoPostoZaino& Posto : Posti)
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
			Posto.Qualita = FMath::Min<uint8>(Posto.Qualita, ValdorsoQualita::Massima);
			Posto.Usura = FMath::Clamp(Posto.Usura, 0, FMath::Max(0, ValdorsoQualita::DurataMassima(Posto.Id, Posto.Qualita)));
		}
	}
}

namespace ValdorsoZaino
{
	bool SiPuoDepositare(const FValdorsoZaino& Zaino, int32 Posto)
	{
		// la tessera va sempre con te; il foglio di un incarico della Gilda anche (la Gilda lo cerca nello zaino)
		return Zaino.Posti.IsValidIndex(Posto) && !Zaino.Posti[Posto].Vuoto() && ValdorsoOggetti::Trova(Zaino.Posti[Posto].Id)
			&& Zaino.Posti[Posto].Id != FName(TEXT("Tessera")) && Zaino.Posti[Posto].Id != ValdorsoGilda::IdFoglio
			&& Zaino.Posti[Posto].Numero == 0;
	}

	bool Deposita(FValdorsoZaino& Zaino, int32 Posto)
	{
		if (!SiPuoDepositare(Zaino, Posto))
		{
			return false;
		}
		BauleSistema(Zaino.Baule);
		FValdorsoPostoZaino& Da = Zaino.Posti[Posto];
		const int32 Entrati = BauleMettiIn(Zaino.Baule, Da);
		if (Entrati <= 0)
		{
			return false;
		}
		Da.Quantita -= Entrati;
		if (Da.Quantita <= 0)
		{
			Da = FValdorsoPostoZaino();
		}
		SistemaMani(Zaino);
		return true;
	}

	bool Ritira(FValdorsoZaino& Zaino, int32 PostoBaule)
	{
		if (!Zaino.Baule.IsValidIndex(PostoBaule) || Zaino.Baule[PostoBaule].Vuoto())
		{
			return false;
		}
		if (Zaino.Posti.Num() != NumeroPosti)
		{
			Zaino.Posti.SetNum(NumeroPosti);
		}
		FValdorsoPostoZaino& Da = Zaino.Baule[PostoBaule];
		const int32 Entrati = BauleMettiIn(Zaino.Posti, Da);
		if (Entrati <= 0)
		{
			return false;
		}
		Da.Quantita -= Entrati;
		if (Da.Quantita <= 0)
		{
			Da = FValdorsoPostoZaino();
		}
		return true;
	}

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
			Posto.Qualita = FMath::Min<uint8>(Posto.Qualita, ValdorsoQualita::Massima);
			Posto.Usura = FMath::Clamp(Posto.Usura, 0, FMath::Max(0, ValdorsoQualita::DurataMassima(Posto.Id, Posto.Qualita)));
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
		BauleSistema(Zaino.Baule);
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
		// (14/11, v0.2.3) l'arco si tiene con due mani: niente scudo insieme
		const FValdorsoTipoOggetto* NellaDestra = Zaino.Destra.IsNone() ? nullptr : ValdorsoOggetti::Trova(Zaino.Destra);
		if (NellaDestra && NellaDestra->bDueMani && !Zaino.Sinistra.IsNone())
		{
			Zaino.Sinistra = NAME_None;
			bCambiato = true;
		}
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
		// (14/11, v0.2.3) l'arco vuole tutte e due le mani: prenderlo rimette via lo scudo, e prendere lo scudo rimette via l'arco
		if (Tipo->bDueMani)
		{
			Zaino.Sinistra = NAME_None;
		}
		else if (Tipo->Posto == EValdorsoPosto::ManoSinistra)
		{
			const FValdorsoTipoOggetto* NellaDestra = Zaino.Destra.IsNone() ? nullptr : ValdorsoOggetti::Trova(Zaino.Destra);
			if (NellaDestra && NellaDestra->bDueMani)
			{
				Zaino.Destra = NAME_None;
			}
		}
		return true;
	}

	/** (14/11) Aggiungi e AggiungiCon passano tutti e due da qui (il grado e la provenienza decidono con chi si impila). */
	int32 AggiungiInterno(FValdorsoZaino& Zaino, FName Id, int32 Quantita, int32 Cariche, uint8 Qualita, const FString& Provenienza);

	int32 Aggiungi(FValdorsoZaino& Zaino, FName Id, int32 Quantita, int32 Cariche)
	{
		return AggiungiInterno(Zaino, Id, Quantita, Cariche, ValdorsoQualita::Comune, FString());
	}

	int32 AggiungiCon(FValdorsoZaino& Zaino, FName Id, int32 Quantita, uint8 Qualita, const FString& Provenienza)
	{
		return AggiungiInterno(Zaino, Id, Quantita, -1, FMath::Min<uint8>(Qualita, ValdorsoQualita::Massima), Provenienza);
	}

	int32 AggiungiInterno(FValdorsoZaino& Zaino, FName Id, int32 Quantita, int32 Cariche, uint8 Qualita, const FString& Provenienza)
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
				if (Posto.Id == Id && Posto.Quantita > 0 && Posto.Quantita < Tipo->Pila && Posto.Numero == 0
					&& Posto.Qualita == Qualita && Posto.Provenienza == Provenienza && Posto.Usura == 0)
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
				Posto.Qualita = Qualita;
				Posto.Provenienza = Provenienza;
				Posto.Usura = 0;
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
		if (!Arrivo.Vuoto() && Arrivo.Id == Sorgente.Id && Tipo && Tipo->Pila > 1 && Tipo->Cariche == 0 && Arrivo.StessaFattura(Sorgente))
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
		// (11/11, v0.1.5) anche le tecniche attive (ValdorsoScherma.h): nella barra si usano con il loro tasto
		if (ValdorsoScherma::DaBarra(Id))
		{
			return true;
		}
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
		if (Zaino.Aggiornamenti < 2)
		{
			// (11/11, v0.1.5) 2: la tessera della carovana, il ricordo del viaggio (dalle Cronache)
			if (Conta(Zaino, TEXT("Tessera_Carovana")) == 0)
			{
				if (Aggiungi(Zaino, TEXT("Tessera_Carovana"), 1) == 0)
				{
					Zaino.Aggiornamenti = 1;
					return bDato;  // lo zaino è pieno: si riprova la prossima volta
				}
				bDato = true;
			}
		}
		Zaino.Aggiornamenti = FMath::Max(Zaino.Aggiornamenti, UltimoAggiornamento);
		return bDato;
	}

	int32 PostoInMano(const FValdorsoZaino& Zaino, FName Id)
	{
		if (Id.IsNone())
		{
			return INDEX_NONE;
		}
		int32 Migliore = INDEX_NONE;
		int32 Punti = -1;
		for (int32 i = 0; i < Zaino.Posti.Num(); ++i)
		{
			const FValdorsoPostoZaino& Posto = Zaino.Posti[i];
			if (Posto.Vuoto() || Posto.Id != Id)
			{
				continue;
			}
			// prima le non rovinate, poi il grado piu' alto, poi la prima
			const int32 Questo = (ValdorsoQualita::Rovinata(Posto) ? 0 : 10) + Posto.Qualita;
			if (Questo > Punti)
			{
				Punti = Questo;
				Migliore = i;
			}
		}
		return Migliore;
	}

	bool Consuma(FValdorsoZaino& Zaino, bool bSinistra, int32 Quanto)
	{
		const int32 Posto = PostoInMano(Zaino, bSinistra ? Zaino.Sinistra : Zaino.Destra);
		if (!Zaino.Posti.IsValidIndex(Posto) || Quanto <= 0)
		{
			return false;
		}
		FValdorsoPostoZaino& Arma = Zaino.Posti[Posto];
		const int32 Massimo = ValdorsoQualita::DurataMassima(Arma.Id, Arma.Qualita);
		if (Massimo <= 0 || Arma.Usura >= Massimo)
		{
			return false;
		}
		Arma.Usura = FMath::Min(Massimo, Arma.Usura + Quanto);
		return Arma.Usura >= Massimo;
	}

	int32 PostoDaRiparare(const FValdorsoZaino& Zaino, FName Id)
	{
		if (Id.IsNone())
		{
			return INDEX_NONE;
		}
		int32 Migliore = INDEX_NONE;
		for (int32 i = 0; i < Zaino.Posti.Num(); ++i)
		{
			const FValdorsoPostoZaino& Posto = Zaino.Posti[i];
			if (Posto.Vuoto() || Posto.Id != Id || !ValdorsoQualita::Consumata(Posto))
			{
				continue;
			}
			const FValdorsoPostoZaino* Prima = Zaino.Posti.IsValidIndex(Migliore) ? &Zaino.Posti[Migliore] : nullptr;
			if (!Prima || Posto.Qualita > Prima->Qualita || (Posto.Qualita == Prima->Qualita && Posto.Usura > Prima->Usura))
			{
				Migliore = i;
			}
		}
		return Migliore;
	}

	bool Ripara(FValdorsoZaino& Zaino, bool bSinistra)
	{
		const int32 Posto = PostoDaRiparare(Zaino, bSinistra ? Zaino.Sinistra : Zaino.Destra);
		if (!Zaino.Posti.IsValidIndex(Posto))
		{
			return false;
		}
		Zaino.Posti[Posto].Usura = 0;
		return true;
	}

	bool RiempiBorracce(FValdorsoZaino& Zaino)
	{
		bool bRiempite = false;
		for (FValdorsoPostoZaino& Posto : Zaino.Posti)
		{
			const FValdorsoTipoOggetto* Tipo = Posto.Vuoto() ? nullptr : ValdorsoOggetti::Trova(Posto.Id);
			// (11/11, v0.2) i secchi no: la loro acqua ha una qualita', si riempiono vuoti (EValdorsoUso::Raccogli)
			if (Tipo && Tipo->Uso == EValdorsoUso::Bevi && Posto.Cariche < Tipo->Cariche && FCString::Strlen(Tipo->Vuoto) == 0)
			{
				Posto.Cariche = Tipo->Cariche;
				bRiempite = true;
			}
		}
		return bRiempite;
	}
}
