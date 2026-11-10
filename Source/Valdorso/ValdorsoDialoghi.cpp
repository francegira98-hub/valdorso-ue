// Valdorso - I dialoghi con i PNG (vedi il .h). Scritto da Claude il 07/11/2026.
// Unity build: le cose del namespace anonimo cominciano con Dialoghi, così non si scontrano con gli altri file.

#include "ValdorsoDialoghi.h"
#include "ValdorsoRune.h"

namespace
{
	/** Chi ha già parlato con chi: "Gaspare|nome del personaggio" (solo sul server, solo in memoria). */
	TSet<FString>& DialoghiMemoria()
	{
		static TSet<FString> Memoria;
		return Memoria;
	}

	FString DialoghiChiave(FName Id, const FString& Chi)
	{
		return Id.ToString() + TEXT("|") + Chi.TrimStartAndEnd().ToLower();
	}

	/**
	 * (08/11) I segni dei personaggi: "Gaspare|nome del personaggio|segno". Come la memoria qui sopra: solo sul server,
	 * solo in memoria (si dimenticano quando il server si spegne), con la stessa chiave per dialogo e personaggio.
	 */
	TSet<FString>& DialoghiSegni()
	{
		static TSet<FString> Segni;
		return Segni;
	}

	FString DialoghiChiaveSegno(FName Id, const FString& Chi, FName Segno)
	{
		return DialoghiChiave(Id, Chi) + TEXT("|") + Segno.ToString().ToLower();
	}
}

// I testi: Gaspare e le due guardie del passo (DialoghiScritti()).
#include "ValdorsoDialoghiTesti.inl"

namespace ValdorsoDialoghi
{
	const TArray<FValdorsoDialogo>& Tutti()
	{
		static const TArray<FValdorsoDialogo> Scritti = DialoghiScritti();
		return Scritti;
	}

	const FValdorsoDialogo* Trova(FName Id)
	{
		if (Id.IsNone())
		{
			return nullptr;
		}
		return Tutti().FindByPredicate([Id](const FValdorsoDialogo& Dialogo) { return Dialogo.Id == Id; });
	}

	FString Saluto(FName Id, float Ora, int32 Seme)
	{
		const int32 Momento = (Ora >= 6.f && Ora < 17.f) ? 0 : ((Ora >= 17.f && Ora < 21.f) ? 1 : 2);
		TArray<const TCHAR*> Adatti;
		for (const FSalutoScritto& S : SalutiScritti)
		{
			if (Id == FName(S.Id) && S.Momento == Momento)
			{
				Adatti.Add(S.Testo);
			}
		}
		if (Adatti.IsEmpty())
		{
			return FString();
		}
		return Adatti[FMath::Abs(Seme) % Adatti.Num()];
	}

	TArray<FString> TuttiISaluti(FName Id)
	{
		TArray<FString> Fuori;
		for (const FSalutoScritto& S : SalutiScritti)
		{
			if (Id == FName(S.Id))
			{
				Fuori.Add(S.Testo);
			}
		}
		return Fuori;
	}

	FName NodoIniziale(const FValdorsoDialogo& Dialogo, bool bGiaParlato)
	{
		if (bGiaParlato && !Dialogo.Ritorno.IsNone() && Dialogo.Trova(Dialogo.Ritorno))
		{
			return Dialogo.Ritorno;
		}
		return Dialogo.Inizio;
	}

	bool Avanza(const FValdorsoDialogo& Dialogo, FName Nodo, int32 Scelta, FName& OutProssimo, FName& OutEvento)
	{
		OutProssimo = NAME_None;
		OutEvento = NAME_None;
		const FValdorsoNodoDialogo* Attuale = Dialogo.Trova(Nodo);
		if (!Attuale || !Attuale->Scelte.IsValidIndex(Scelta))
		{
			return false;
		}
		const FValdorsoSceltaDialogo& Risposta = Attuale->Scelte[Scelta];
		// Una risposta verso un nodo che non c'è vale come fine (i test non lo permettono, ma il server non si blocca).
		OutProssimo = (Risposta.Verso.IsNone() || Dialogo.Trova(Risposta.Verso)) ? Risposta.Verso : FName(NAME_None);
		OutEvento = Risposta.Evento;
		return true;
	}

	TArray<int32> ScelteVisibili(const FValdorsoDialogo& Dialogo, FName Nodo, const FString& Chi)
	{
		return ScelteVisibili(Dialogo, Nodo, FValdorsoMemoriaPerNome(Chi));
	}

	TArray<int32> ScelteVisibili(const FValdorsoDialogo& Dialogo, FName Nodo, const FValdorsoMemoriaDialoghi& Memoria)
	{
		TArray<int32> Fuori;
		const FValdorsoNodoDialogo* Attuale = Dialogo.Trova(Nodo);
		if (!Attuale)
		{
			return Fuori;
		}
		for (int32 i = 0; i < Attuale->Scelte.Num() && Fuori.Num() < MassimoScelte; ++i)
		{
			const FName Nascosta = Attuale->Scelte[i].NascostaSe;
			if (!Nascosta.IsNone() && Memoria.HaSegno(Dialogo.Id, Nascosta))
			{
				continue;
			}
			Fuori.Add(i);
		}
		return Fuori;
	}

	FValdorsoBattuta Battuta(const FValdorsoDialogo& Dialogo, FName Nodo, int32 Passo, const FString& Chi)
	{
		return Battuta(Dialogo, Nodo, Passo, FValdorsoMemoriaPerNome(Chi));
	}

	FValdorsoBattuta Battuta(const FValdorsoDialogo& Dialogo, FName Nodo, int32 Passo, const FValdorsoMemoriaDialoghi& Memoria)
	{
		FValdorsoBattuta Fuori;
		Fuori.Chi = Dialogo.Titolo;
		Fuori.Passo = Passo;
		const FValdorsoNodoDialogo* Attuale = Dialogo.Trova(Nodo);
		if (!Attuale)
		{
			Fuori.bChiudi = true;
			return Fuori;
		}
		for (const FString& Riga : Attuale->Righe)
		{
			Fuori.Righe.Add(Riga.Left(MassimoLettere));
		}
		for (const int32 i : ScelteVisibili(Dialogo, Nodo, Memoria))
		{
			Fuori.Scelte.Add(Attuale->Scelte[i].Testo.Left(MassimoLettere));
		}
		Fuori.bFine = Attuale->bFine || Fuori.Scelte.Num() == 0;
		return Fuori;
	}

	TArray<FString> Problemi(const FValdorsoDialogo& Dialogo)
	{
		TArray<FString> Fuori;
		const FString Nome = Dialogo.Id.ToString();
		if (Dialogo.Id.IsNone())
		{
			Fuori.Add(TEXT("un dialogo senza Id"));
		}
		if (Dialogo.NomeBreve.IsEmpty() || Dialogo.Titolo.IsEmpty())
		{
			Fuori.Add(FString::Printf(TEXT("%s: manca il nome breve o il titolo"), *Nome));
		}
		if (!Dialogo.Trova(Dialogo.Inizio))
		{
			Fuori.Add(FString::Printf(TEXT("%s: il nodo d'inizio '%s' non c'è"), *Nome, *Dialogo.Inizio.ToString()));
		}
		if (!Dialogo.Ritorno.IsNone() && !Dialogo.Trova(Dialogo.Ritorno))
		{
			Fuori.Add(FString::Printf(TEXT("%s: il nodo del ritorno '%s' non c'è"), *Nome, *Dialogo.Ritorno.ToString()));
		}

		// Ogni nodo da solo: Id unico, battute, risposte, fine.
		TSet<FName> Visti;
		for (const FValdorsoNodoDialogo& Nodo : Dialogo.Nodi)
		{
			const FString Qui = FString::Printf(TEXT("%s/%s"), *Nome, *Nodo.Id.ToString());
			if (Nodo.Id.IsNone())
			{
				Fuori.Add(FString::Printf(TEXT("%s: un nodo senza Id"), *Nome));
			}
			if (Visti.Contains(Nodo.Id))
			{
				Fuori.Add(FString::Printf(TEXT("%s: Id doppio"), *Qui));
			}
			Visti.Add(Nodo.Id);
			if (Nodo.Righe.Num() == 0)
			{
				Fuori.Add(FString::Printf(TEXT("%s: il PNG non dice niente"), *Qui));
			}
			for (const FString& Riga : Nodo.Righe)
			{
				if (Riga.TrimStartAndEnd().IsEmpty())
				{
					Fuori.Add(FString::Printf(TEXT("%s: una battuta vuota"), *Qui));
				}
				if (Riga.Len() > MassimoLettere)
				{
					Fuori.Add(FString::Printf(TEXT("%s: una battuta di %d lettere (al massimo %d)"), *Qui, Riga.Len(), MassimoLettere));
				}
				TArray<TArray<EValdorsoRuna>> Parole;
				if (Riga.TrimStart().StartsWith(ValdorsoRune::PrefissoRiga) && !ValdorsoRune::LeggiRiga(Riga, Parole))
				{
					Fuori.Add(FString::Printf(TEXT("%s: la riga di rune non si legge (%s)"), *Qui, *Riga));
				}
			}
			if (Nodo.Scelte.Num() > MassimoScelte)
			{
				Fuori.Add(FString::Printf(TEXT("%s: %d risposte (al massimo %d)"), *Qui, Nodo.Scelte.Num(), MassimoScelte));
			}
			if (Nodo.bFine && Nodo.Scelte.Num() > 0)
			{
				Fuori.Add(FString::Printf(TEXT("%s: è una fine ma ha delle risposte"), *Qui));
			}
			if (!Nodo.bFine && Nodo.Scelte.Num() == 0)
			{
				Fuori.Add(FString::Printf(TEXT("%s: vicolo cieco (nessuna risposta e non è una fine)"), *Qui));
			}
			// (08/11) Anche con nascoste tutte le risposte NascostaSe deve restarne almeno una (contano le prime
			// MassimoScelte, come le manda Battuta).
			int32 SempreVisibili = 0;
			for (int32 i = 0; i < Nodo.Scelte.Num() && i < MassimoScelte; ++i)
			{
				SempreVisibili += Nodo.Scelte[i].NascostaSe.IsNone() ? 1 : 0;
			}
			if (!Nodo.bFine && Nodo.Scelte.Num() > 0 && SempreVisibili == 0)
			{
				Fuori.Add(FString::Printf(TEXT("%s: con le risposte nascoste resta senza risposte"), *Qui));
			}
			for (const FValdorsoSceltaDialogo& Risposta : Nodo.Scelte)
			{
				if (Risposta.Testo.TrimStartAndEnd().IsEmpty())
				{
					Fuori.Add(FString::Printf(TEXT("%s: una risposta senza testo"), *Qui));
				}
				if (Risposta.Testo.Len() > MassimoLettere)
				{
					Fuori.Add(FString::Printf(TEXT("%s: una risposta troppo lunga"), *Qui));
				}
				if (!Risposta.Verso.IsNone() && !Dialogo.Trova(Risposta.Verso))
				{
					Fuori.Add(FString::Printf(TEXT("%s: la risposta \"%s\" porta a '%s', che non c'è"), *Qui, *Risposta.Testo,
						*Risposta.Verso.ToString()));
				}
			}
		}

		// Raggiungibili: dall'inizio e dal ritorno.
		TSet<FName> Raggiunti;
		TArray<FName> DaVedere;
		DaVedere.Add(Dialogo.Inizio);
		if (!Dialogo.Ritorno.IsNone())
		{
			DaVedere.Add(Dialogo.Ritorno);
		}
		while (DaVedere.Num() > 0)
		{
			const FName Id = DaVedere.Pop();
			if (Raggiunti.Contains(Id))
			{
				continue;
			}
			const FValdorsoNodoDialogo* Nodo = Dialogo.Trova(Id);
			if (!Nodo)
			{
				continue;
			}
			Raggiunti.Add(Id);
			for (const FValdorsoSceltaDialogo& Risposta : Nodo->Scelte)
			{
				if (!Risposta.Verso.IsNone())
				{
					DaVedere.Add(Risposta.Verso);
				}
			}
		}
		for (const FValdorsoNodoDialogo& Nodo : Dialogo.Nodi)
		{
			if (!Raggiunti.Contains(Nodo.Id))
			{
				Fuori.Add(FString::Printf(TEXT("%s/%s: non ci si arriva mai"), *Nome, *Nodo.Id.ToString()));
			}
		}

		// Da ogni nodo si deve poter arrivare a una fine (un nodo fine, o una risposta che chiude): niente giri chiusi.
		// (08/11) Contano solo le risposte che non si nascondono mai: così vale anche per chi ha tutti i segni.
		TSet<FName> Escono;
		bool bCambiato = true;
		while (bCambiato)
		{
			bCambiato = false;
			for (const FValdorsoNodoDialogo& Nodo : Dialogo.Nodi)
			{
				if (Escono.Contains(Nodo.Id))
				{
					continue;
				}
				bool bEsce = Nodo.bFine;
				for (const FValdorsoSceltaDialogo& Risposta : Nodo.Scelte)
				{
					bEsce = bEsce || (Risposta.NascostaSe.IsNone() && (Risposta.Verso.IsNone() || Escono.Contains(Risposta.Verso)));
				}
				if (bEsce)
				{
					Escono.Add(Nodo.Id);
					bCambiato = true;
				}
			}
		}
		for (const FValdorsoNodoDialogo& Nodo : Dialogo.Nodi)
		{
			if (!Escono.Contains(Nodo.Id))
			{
				Fuori.Add(FString::Printf(TEXT("%s/%s: da qui non si arriva mai a una fine"), *Nome, *Nodo.Id.ToString()));
			}
		}
		return Fuori;
	}

	bool GiaParlato(FName Id, const FString& Chi)
	{
		return DialoghiMemoria().Contains(DialoghiChiave(Id, Chi));
	}

	void SegnaParlato(FName Id, const FString& Chi)
	{
		DialoghiMemoria().Add(DialoghiChiave(Id, Chi));
	}

	bool HaSegno(FName Id, const FString& Chi, FName Segno)
	{
		return !Segno.IsNone() && DialoghiSegni().Contains(DialoghiChiaveSegno(Id, Chi, Segno));
	}

	void MettiSegno(FName Id, const FString& Chi, FName Segno)
	{
		if (!Segno.IsNone())
		{
			DialoghiSegni().Add(DialoghiChiaveSegno(Id, Chi, Segno));
		}
	}

	void Dimentica()
	{
		DialoghiMemoria().Reset();
		DialoghiSegni().Reset();
	}
}

// --- (08/11) La memoria per nome (accesso libero dell'editor, test) -------------------------------------------------

bool FValdorsoMemoriaPerNome::GiaParlato(FName IdDialogo) const
{
	return !Chi.TrimStartAndEnd().IsEmpty() && ValdorsoDialoghi::GiaParlato(IdDialogo, Chi);
}

void FValdorsoMemoriaPerNome::SegnaParlato(FName IdDialogo)
{
	if (!Chi.TrimStartAndEnd().IsEmpty())
	{
		ValdorsoDialoghi::SegnaParlato(IdDialogo, Chi);
	}
}

bool FValdorsoMemoriaPerNome::HaSegno(FName IdDialogo, FName Segno) const
{
	return !Chi.TrimStartAndEnd().IsEmpty() && ValdorsoDialoghi::HaSegno(IdDialogo, Chi, Segno);
}

void FValdorsoMemoriaPerNome::MettiSegno(FName IdDialogo, FName Segno)
{
	if (!Chi.TrimStartAndEnd().IsEmpty())
	{
		ValdorsoDialoghi::MettiSegno(IdDialogo, Chi, Segno);
	}
}
