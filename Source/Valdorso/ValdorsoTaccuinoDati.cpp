// Valdorso - Bussola e taccuino di viaggio: i dati e i conti (vedi il .h). Scritto da Claude il 07/11/2026.

#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoFiducia.h"

#define LOCTEXT_NAMESPACE "ValdorsoTaccuino"

namespace
{
	/** Una riga di ValdorsoLuoghiDati.inl. */
	struct FValdorsoLuogoDati
	{
		const TCHAR* Nome;
		double X;
		double Y;
		double Z;
		float Raggio;
	};

#include "ValdorsoLuoghiDati.inl"

	/** Le righe di un'iscrizione pulite e tagliate alle misure del taccuino (le vuote via). */
	TArray<FString> TaccuinoRighePulite(const TArray<FString>& Righe)
	{
		TArray<FString> Pulite;
		for (const FString& Riga : Righe)
		{
			if (Pulite.Num() >= ValdorsoTaccuino::MassimoRighe)
			{
				break;
			}
			FString Pulita = Riga.TrimStartAndEnd().Left(ValdorsoTaccuino::MassimoLettere);
			if (!Pulita.IsEmpty())
			{
				Pulite.Add(MoveTemp(Pulita));
			}
		}
		return Pulite;
	}

	/** Il titolo pulito; senza titolo è "Bacheca", come la scrive la pergamena. */
	FString TaccuinoTitoloPulito(const FString& Titolo)
	{
		const FString Pulito = Titolo.TrimStartAndEnd().Left(ValdorsoTaccuino::MassimoTitolo);
		return Pulito.IsEmpty() ? FString(TEXT("Bacheca")) : Pulito;
	}

	/** (08/11) Il nome di un dialogo o di un segno come si scrive nel file: senza spazi ai lati, non troppo lungo. */
	FString TaccuinoNomeRicordo(const FString& Nome)
	{
		return Nome.TrimStartAndEnd().Left(ValdorsoTaccuino::MassimoNomeDialogo);
	}

	/** (08/11) Il ricordo di quel dialogo (nullptr se non c'è; le maiuscole non contano). */
	const FValdorsoRicordoDialogo* TaccuinoTrovaRicordo(const TArray<FValdorsoRicordoDialogo>& Ricordi, const FString& IdDialogo)
	{
		return Ricordi.FindByPredicate([&IdDialogo](const FValdorsoRicordoDialogo& R) { return R.Dialogo.Equals(IdDialogo, ESearchCase::IgnoreCase); });
	}

	/** (08/11) Il ricordo di quel dialogo, da cambiare: se non c'è lo aggiunge (nullptr se la memoria è piena). */
	FValdorsoRicordoDialogo* TaccuinoRicordoDaCambiare(TArray<FValdorsoRicordoDialogo>& Ricordi, const FString& IdDialogo)
	{
		FValdorsoRicordoDialogo* Trovato = Ricordi.FindByPredicate([&IdDialogo](const FValdorsoRicordoDialogo& R) { return R.Dialogo.Equals(IdDialogo, ESearchCase::IgnoreCase); });
		if (Trovato || Ricordi.Num() >= ValdorsoTaccuino::MassimoDialoghi)
		{
			return Trovato;
		}
		FValdorsoRicordoDialogo& Nuovo = Ricordi.AddDefaulted_GetRef();
		Nuovo.Dialogo = IdDialogo;
		return &Nuovo;
	}

	/** (08/11) Aggiunge il segno al ricordo; falso se c'era già, è vuoto o i segni sono al massimo. */
	bool TaccuinoAggiungiSegno(FValdorsoRicordoDialogo& Ricordo, const FString& Segno)
	{
		if (Segno.IsEmpty() || Ricordo.Segni.Num() >= ValdorsoTaccuino::MassimoSegni
			|| Ricordo.Segni.ContainsByPredicate([&Segno](const FString& S) { return S.Equals(Segno, ESearchCase::IgnoreCase); }))
		{
			return false;
		}
		Ricordo.Segni.Add(Segno);
		return true;
	}
}

// ------------------------------------------------------------------------------------------------- i luoghi

namespace ValdorsoLuoghi
{
	const TArray<FValdorsoLuogo>& Tutti()
	{
		static const TArray<FValdorsoLuogo> Elenco = []()
		{
			TArray<FValdorsoLuogo> Fuori;
			for (const FValdorsoLuogoDati& Dati : GValdorsoLuoghiDati)
			{
				FValdorsoLuogo Luogo;
				Luogo.Nome = Dati.Nome;
				Luogo.Posizione = FVector(Dati.X, Dati.Y, Dati.Z);
				Luogo.Raggio = Dati.Raggio;
				Fuori.Add(MoveTemp(Luogo));
			}
			return Fuori;
		}();
		return Elenco;
	}

	const FValdorsoLuogo* Trova(const FString& Nome)
	{
		for (const FValdorsoLuogo& Luogo : Tutti())
		{
			if (Luogo.Nome == Nome)
			{
				return &Luogo;
			}
		}
		return nullptr;
	}

	bool Dentro(const FValdorsoLuogo& Luogo, const FVector& Dove)
	{
		const double DX = Dove.X - Luogo.Posizione.X;
		const double DY = Dove.Y - Luogo.Posizione.Y;
		return Luogo.Raggio > 0.f && DX * DX + DY * DY <= static_cast<double>(Luogo.Raggio) * static_cast<double>(Luogo.Raggio);
	}

	TArray<const FValdorsoLuogo*> Intorno(const FVector& Dove)
	{
		TArray<const FValdorsoLuogo*> Trovati;
		for (const FValdorsoLuogo& Luogo : Tutti())
		{
			if (Dentro(Luogo, Dove))
			{
				Trovati.Add(&Luogo);
			}
		}
		return Trovati;
	}
}

// ------------------------------------------------------------------------------------------------- la bussola

namespace ValdorsoBussola
{
	float Rotta(float Imbardata)
	{
		// Imbardata 0 = +X = est (rotta 90), 90 = +Y = sud (rotta 180), -90 = -Y = nord (rotta 0)
		const float Gradi = FMath::Fmod(Imbardata + 90.f, 360.f);
		return Gradi < 0.f ? Gradi + 360.f : Gradi;
	}

	float RottaVerso(const FVector& Da, const FVector& A)
	{
		const FVector Verso = A - Da;
		if (FMath::IsNearlyZero(Verso.X) && FMath::IsNearlyZero(Verso.Y))
		{
			return 0.f;
		}
		return Rotta(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Verso.Y, Verso.X))));
	}

	float Scarto(float RottaGuardata, float RottaCosa)
	{
		// da -180 (compreso) a 180
		float Gradi = FMath::Fmod(RottaCosa - RottaGuardata + 180.f, 360.f);
		if (Gradi < 0.f)
		{
			Gradi += 360.f;
		}
		return Gradi - 180.f;
	}

	bool SullaStriscia(float ScartoGradi, float Larghezza, float Campo, float& OutX)
	{
		if (Campo <= 0.f || Larghezza <= 0.f || FMath::Abs(ScartoGradi) > Campo * 0.5f)
		{
			return false;
		}
		OutX = Larghezza * 0.5f + ScartoGradi * (Larghezza / Campo);
		return true;
	}

	int32 PuntoCardinale(float RottaGradi)
	{
		const float Gradi = FMath::Fmod(RottaGradi, 360.f);
		const float Positivi = Gradi < 0.f ? Gradi + 360.f : Gradi;
		return FMath::RoundToInt(Positivi / 45.f) % 8;
	}

	FString Sigla(int32 Punto)
	{
		// In italiano l'ovest è O: SO e NO
		static const TCHAR* const Sigle[8] = { TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"), TEXT("S"), TEXT("SO"), TEXT("O"), TEXT("NO") };
		return (Punto >= 0 && Punto < 8) ? FString(Sigle[Punto]) : FString();
	}

	FString TestoDistanza(double Centimetri)
	{
		const double Metri = FMath::Max(0.0, Centimetri / 100.0);
		if (Metri < 995.0)
		{
			// sotto i 100 m a cinque metri, poi a decine (nessuno conta i passi uno per uno)
			const int32 Passo = Metri < 100.0 ? 5 : 10;
			const int32 Tondi = FMath::Max(Passo, FMath::RoundToInt(static_cast<float>(Metri / Passo)) * Passo);
			return FString::Printf(TEXT("%d m"), Tondi);
		}
		// "1,2 km": la virgola, all'italiana
		return FString::Printf(TEXT("%.1f km"), Metri / 1000.0).Replace(TEXT("."), TEXT(","));
	}
}

// ------------------------------------------------------------------------------------------------- il taccuino

namespace ValdorsoTaccuino
{
	bool ConosceLuogo(const FValdorsoTaccuino& Taccuino, const FString& Nome)
	{
		return Taccuino.Luoghi.ContainsByPredicate([&Nome](const FValdorsoLuogoScoperto& L) { return L.Nome == Nome; });
	}

	bool AggiungiLuogo(FValdorsoTaccuino& Taccuino, const FString& Nome, int64 Quando, float Ora)
	{
		if (Nome.IsEmpty() || ConosceLuogo(Taccuino, Nome))
		{
			return false;
		}
		FValdorsoLuogoScoperto Nuovo;
		Nuovo.Nome = Nome;
		Nuovo.Quando = Quando;
		Nuovo.Ora = Ora;
		Taccuino.Luoghi.Add(MoveTemp(Nuovo));
		return true;
	}

	EValdorsoLettura AnnotaIscrizione(FValdorsoTaccuino& Taccuino, const FString& Titolo, const TArray<FString>& Righe, int64 Quando)
	{
		TArray<FString> Pulite = TaccuinoRighePulite(Righe);
		if (Pulite.Num() == 0)
		{
			return EValdorsoLettura::Ignorata;  // una bacheca vuota non lascia niente da ricopiare
		}
		const FString Chiave = TaccuinoTitoloPulito(Titolo);
		const int32 Indice = Taccuino.Iscrizioni.IndexOfByPredicate([&Chiave](const FValdorsoIscrizioneLetta& I) { return I.Titolo == Chiave; });
		if (Indice != INDEX_NONE && Taccuino.Iscrizioni[Indice].Righe == Pulite)
		{
			return EValdorsoLettura::Ignorata;
		}
		const bool bAggiornata = Indice != INDEX_NONE;
		if (bAggiornata)
		{
			Taccuino.Iscrizioni.RemoveAt(Indice);
		}
		FValdorsoIscrizioneLetta Nuova;
		Nuova.Titolo = Chiave;
		Nuova.Righe = MoveTemp(Pulite);
		Nuova.Quando = Quando;
		Taccuino.Iscrizioni.Add(MoveTemp(Nuova));
		// pieno: via le più vecchie
		while (Taccuino.Iscrizioni.Num() > MassimoIscrizioni)
		{
			Taccuino.Iscrizioni.RemoveAt(0);
		}
		return bAggiornata ? EValdorsoLettura::Aggiornata : EValdorsoLettura::Nuova;
	}

	void Sistema(FValdorsoTaccuino& Taccuino)
	{
		TArray<FValdorsoLuogoScoperto> Luoghi;
		for (const FValdorsoLuogoScoperto& L : Taccuino.Luoghi)
		{
			if (!L.Nome.IsEmpty() && !Luoghi.ContainsByPredicate([&L](const FValdorsoLuogoScoperto& Altro) { return Altro.Nome == L.Nome; }))
			{
				Luoghi.Add(L);
			}
		}
		Taccuino.Luoghi = MoveTemp(Luoghi);

		// le iscrizioni si riannotano una per una: stesse regole di quando si leggono (l'ultima con un titolo vince)
		FValdorsoTaccuino Riordinato;
		for (const FValdorsoIscrizioneLetta& I : Taccuino.Iscrizioni)
		{
			AnnotaIscrizione(Riordinato, I.Titolo, I.Righe, I.Quando);
		}
		Taccuino.Iscrizioni = MoveTemp(Riordinato.Iscrizioni);

		// (08/11) la memoria dei dialoghi: via i nomi vuoti, i doppioni si uniscono (con i segni di tutti e due)
		TArray<FValdorsoRicordoDialogo> Ricordi;
		for (const FValdorsoRicordoDialogo& Letto : Taccuino.Dialoghi)
		{
			const FString IdDialogo = TaccuinoNomeRicordo(Letto.Dialogo);
			FValdorsoRicordoDialogo* Unito = IdDialogo.IsEmpty() ? nullptr : TaccuinoRicordoDaCambiare(Ricordi, IdDialogo);
			if (!Unito)
			{
				continue;
			}
			Unito->bParlato = Unito->bParlato || Letto.bParlato;
			for (const FString& Segno : Letto.Segni)
			{
				TaccuinoAggiungiSegno(*Unito, TaccuinoNomeRicordo(Segno));
			}
			// (14/11, v0.2.4) la fiducia: dei doppioni vale quella piu' lontana da zero (di solito il piu' recente non
			// esiste: e' solo prudenza), e le date piu' nuove
			const int32 Letta = FMath::Clamp(Letto.Fiducia, ValdorsoFiducia::Minima, ValdorsoFiducia::Massima);
			if (FMath::Abs(Letta) > FMath::Abs(Unito->Fiducia))
			{
				Unito->Fiducia = Letta;
			}
			Unito->UltimoGiorno = FMath::Max(Unito->UltimoGiorno, Letto.UltimoGiorno);
			Unito->UltimoRicordo = FMath::Max(Unito->UltimoRicordo, Letto.UltimoRicordo);
		}
		Taccuino.Dialoghi = MoveTemp(Ricordi);
		// (14/11, v0.2.4) i fatti che la valle ricorda
		ValdorsoFiducia::SistemaFatti(Taccuino.Fatti);
		// (14/11, v0.2.4) le storie del Battito: via quelle senza id e i doppioni (vale la prima), al massimo 300
		TArray<FValdorsoStoriaViva> Storie;
		for (const FValdorsoStoriaViva& Letta : Taccuino.Storie)
		{
			const FString Id = Letta.Id.TrimStartAndEnd().Left(128);
			if (Id.IsEmpty() || Storie.ContainsByPredicate([&Id](const FValdorsoStoriaViva& S) { return S.Id.Equals(Id, ESearchCase::IgnoreCase); }))
			{
				continue;
			}
			FValdorsoStoriaViva& Pulita = Storie.Add_GetRef(Letta);
			Pulita.Id = Id;
			Pulita.Seme = Letta.Seme.Left(128);
			Pulita.Stato = FMath::Min<uint8>(Letta.Stato, 5);
			Pulita.Passo = FMath::Clamp(Letta.Passo, 0, 64);
			Pulita.Finale = FMath::Clamp(Letta.Finale, 0, 16);
		}
		while (Storie.Num() > 300)
		{
			Storie.RemoveAt(0);
		}
		Taccuino.Storie = MoveTemp(Storie);
	}

	bool HaParlato(const FValdorsoTaccuino& Taccuino, FName IdDialogo)
	{
		const FValdorsoRicordoDialogo* Ricordo = IdDialogo.IsNone() ? nullptr
			: TaccuinoTrovaRicordo(Taccuino.Dialoghi, TaccuinoNomeRicordo(IdDialogo.ToString()));
		return Ricordo && Ricordo->bParlato;
	}

	bool SegnaParlato(FValdorsoTaccuino& Taccuino, FName IdDialogo)
	{
		const FString Nome = IdDialogo.IsNone() ? FString() : TaccuinoNomeRicordo(IdDialogo.ToString());
		FValdorsoRicordoDialogo* Ricordo = Nome.IsEmpty() ? nullptr : TaccuinoRicordoDaCambiare(Taccuino.Dialoghi, Nome);
		if (!Ricordo || Ricordo->bParlato)
		{
			return false;
		}
		Ricordo->bParlato = true;
		return true;
	}

	bool HaSegno(const FValdorsoTaccuino& Taccuino, FName IdDialogo, FName Segno)
	{
		if (IdDialogo.IsNone() || Segno.IsNone())
		{
			return false;
		}
		const FValdorsoRicordoDialogo* Ricordo = TaccuinoTrovaRicordo(Taccuino.Dialoghi, TaccuinoNomeRicordo(IdDialogo.ToString()));
		const FString Cercato = TaccuinoNomeRicordo(Segno.ToString());
		return Ricordo && Ricordo->Segni.ContainsByPredicate([&Cercato](const FString& S) { return S.Equals(Cercato, ESearchCase::IgnoreCase); });
	}

	bool MettiSegno(FValdorsoTaccuino& Taccuino, FName IdDialogo, FName Segno)
	{
		if (IdDialogo.IsNone() || Segno.IsNone())
		{
			return false;
		}
		const FString Nome = TaccuinoNomeRicordo(IdDialogo.ToString());
		const FString Pulito = TaccuinoNomeRicordo(Segno.ToString());
		if (Nome.IsEmpty() || Pulito.IsEmpty() || HaSegno(Taccuino, IdDialogo, Segno))
		{
			return false;
		}
		FValdorsoRicordoDialogo* Ricordo = TaccuinoRicordoDaCambiare(Taccuino.Dialoghi, Nome);
		return Ricordo && TaccuinoAggiungiSegno(*Ricordo, Pulito);
	}

	FText Momento(float Ora)
	{
		if (Ora < 0.f)
		{
			return FText::GetEmpty();
		}
		const float H = FMath::Fmod(Ora, 24.f);
		if (H >= 5.f && H < 7.f)
		{
			return LOCTEXT("Alba", "all'alba");
		}
		if (H >= 7.f && H < 11.f)
		{
			return LOCTEXT("Mattina", "di mattina");
		}
		if (H >= 11.f && H < 14.f)
		{
			return LOCTEXT("Mezzogiorno", "verso mezzogiorno");
		}
		if (H >= 14.f && H < 18.f)
		{
			return LOCTEXT("Pomeriggio", "di pomeriggio");
		}
		if (H >= 18.f && H < 20.f)
		{
			return LOCTEXT("Tramonto", "al tramonto");
		}
		if (H >= 20.f && H < 23.f)
		{
			return LOCTEXT("Sera", "di sera");
		}
		return LOCTEXT("Notte", "di notte");
	}
}

#undef LOCTEXT_NAMESPACE
