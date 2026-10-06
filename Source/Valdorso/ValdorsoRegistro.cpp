// Valdorso - Il Registro di Val d'Orso (vedi il .h).
// Attenzione ai nomi: dentro il namespace ValdorsoRegistro ci sono le funzioni Voci, Domanda, Risposta, Testo e
// Problema; le variabili locali qui hanno nomi diversi (Unreal tratta come errore un nome che ne nasconde un altro).

#include "ValdorsoRegistro.h"
#include "ValdorsoRegole.h"

namespace ValdorsoRegistro
{
	namespace
	{
		const FValdorsoVoceRegistro VociSesso[] = {
			{ TEXT("uomo"), TEXT("Uomo"), TEXT("Uomo") },
			{ TEXT("donna"), TEXT("Donna"), TEXT("Donna") },
		};

		const FValdorsoVoceRegistro VociFede[] = {
			{ TEXT("solara"), TEXT("Solara, la Luce: giustizia e guarigione"), nullptr },
			{ TEXT("ignar"), TEXT("Ignar, il Fuoco: la forgia e la guerra"), nullptr },
			{ TEXT("nereia"), TEXT("Nereia, l'Acqua: il mare e i viaggi"), nullptr },
			{ TEXT("torvald"), TEXT("Torvald, la Terra: il raccolto e le montagne"), nullptr },
			{ TEXT("zefira"), TEXT("Zefira, l'Aria: la libertà e i messaggi"), nullptr },
			{ TEXT("vecchidei"), TEXT("I Vecchi Dei della foresta"), nullptr },
			{ TEXT("nessuna"), TEXT("Nessuna fede"), nullptr },
		};

		const FValdorsoVoceRegistro VociOrigine[] = {
			{ TEXT("lago"), TEXT("Un villaggio di pescatori sul lago"), nullptr },
			{ TEXT("minatori"), TEXT("Un borgo di minatori sui monti"), nullptr },
			{ TEXT("capitale"), TEXT("I quartieri poveri della capitale di Aurelia"), nullptr },
			{ TEXT("sud"), TEXT("Le campagne del sud"), nullptr },
			{ TEXT("confine"), TEXT("Le terre di confine, vicino alle rovine"), nullptr },
			{ TEXT("dimenticato"), TEXT("Non lo ricordi: ti sei svegliato nel bosco"), TEXT("Non lo ricordi: ti sei svegliata nel bosco") },
		};

		const FValdorsoVoceRegistro VociMestiere[] = {
			{ TEXT("contadino"), TEXT("Contadino"), TEXT("Contadina") },
			{ TEXT("pastore"), TEXT("Pastore"), TEXT("Pastora") },
			{ TEXT("apprendista"), TEXT("Apprendista di bottega"), TEXT("Apprendista di bottega") },
			{ TEXT("soldato"), TEXT("Soldato congedato"), TEXT("Soldata congedata") },
			{ TEXT("servo"), TEXT("Servo in una casa nobile"), TEXT("Serva in una casa nobile") },
			{ TEXT("allievo"), TEXT("Allievo mancato dell'accademia"), TEXT("Allieva mancata dell'accademia") },
			{ TEXT("orfano"), TEXT("Orfano cresciuto al tempio"), TEXT("Orfana cresciuta al tempio") },
			{ TEXT("mercante"), TEXT("Mercante caduto in rovina"), TEXT("Mercante caduta in rovina") },
			{ TEXT("cacciatore"), TEXT("Cacciatore"), TEXT("Cacciatrice") },
			{ TEXT("marinaio"), TEXT("Marinaio"), TEXT("Marinaia") },
		};

		const FValdorsoVoceRegistro VociMotivo[] = {
			{ TEXT("terra"), TEXT("La terra promessa dalla corona"), nullptr },
			{ TEXT("fuga"), TEXT("Fuggi da qualcosa, o da qualcuno"), nullptr },
			{ TEXT("scomparso"), TEXT("Cerchi una persona scomparsa"), nullptr },
			{ TEXT("sogno"), TEXT("Un sogno, o una visione"), nullptr },
			{ TEXT("debito"), TEXT("Un debito da saldare"), nullptr },
			{ TEXT("fede"), TEXT("La fede ti ha chiamato"), TEXT("La fede ti ha chiamata") },
			{ TEXT("avventura"), TEXT("Cerchi avventura"), nullptr },
		};

		const FValdorsoVoceRegistro VociRicordo[] = {
			{ TEXT("anello"), TEXT("L'anello di tua madre"), nullptr },
			{ TEXT("lettera"), TEXT("Una lettera mai aperta"), nullptr },
			{ TEXT("spada"), TEXT("La spada spezzata di tuo padre"), nullptr },
			{ TEXT("amuleto"), TEXT("Un amuleto di legno"), nullptr },
			{ TEXT("mappa"), TEXT("Una mappa strappata"), nullptr },
			{ TEXT("libro"), TEXT("Un libro di preghiere"), nullptr },
		};

		const FValdorsoVoceRegistro VociPaura[] = {
			{ TEXT("fuoco"), TEXT("Il fuoco"), nullptr },
			{ TEXT("acqua"), TEXT("L'acqua profonda"), nullptr },
			{ TEXT("buio"), TEXT("Il buio"), nullptr },
			{ TEXT("solitudine"), TEXT("La solitudine"), nullptr },
			{ TEXT("morti"), TEXT("I morti"), nullptr },
			{ TEXT("magia"), TEXT("La magia"), nullptr },
		};

		const FValdorsoVoceRegistro VociOnesta[] = {
			{ TEXT("onesto"), TEXT("Onesto"), TEXT("Onesta") },
			{ TEXT("furbo"), TEXT("Furbo"), TEXT("Furba") },
		};

		const FValdorsoVoceRegistro VociCoraggio[] = {
			{ TEXT("coraggioso"), TEXT("Coraggioso"), TEXT("Coraggiosa") },
			{ TEXT("prudente"), TEXT("Prudente"), TEXT("Prudente") },
		};

		const FValdorsoVoceRegistro VociDevozione[] = {
			{ TEXT("devoto"), TEXT("Devoto"), TEXT("Devota") },
			{ TEXT("scettico"), TEXT("Scettico"), TEXT("Scettica") },
		};

		const FValdorsoVoceRegistro VociAnimo[] = {
			{ TEXT("gentile"), TEXT("Gentile"), TEXT("Gentile") },
			{ TEXT("duro"), TEXT("Duro"), TEXT("Dura") },
		};

		const FValdorsoVoceRegistro VociMagia[] = {
			{ TEXT("si"), TEXT("Sì: una candela ha tremato, il vento è cambiato"), nullptr },
			{ TEXT("no"), TEXT("No, mai"), nullptr },
		};

		/** Le frasi del racconto, voce per voce: {M} e {F} per maschile e femminile si scelgono con Genere. */
		FString Genere(const TCHAR* Maschile, const TCHAR* Femminile, bool bDonna)
		{
			return bDonna ? Femminile : Maschile;
		}

		const FValdorsoVoceRegistro* Trova(EDomanda Quale, const FString& Scelta)
		{
			for (const FValdorsoVoceRegistro& Voce : Voci(Quale))
			{
				if (Scelta == Voce.Chiave)
				{
					return &Voce;
				}
			}
			return nullptr;
		}
	}

	TArrayView<const FValdorsoVoceRegistro> Voci(EDomanda Quale)
	{
		switch (Quale)
		{
		case EDomanda::Sesso: return VociSesso;
		case EDomanda::Fede: return VociFede;
		case EDomanda::Origine: return VociOrigine;
		case EDomanda::Mestiere: return VociMestiere;
		case EDomanda::Motivo: return VociMotivo;
		case EDomanda::Ricordo: return VociRicordo;
		case EDomanda::Paura: return VociPaura;
		case EDomanda::Onesta: return VociOnesta;
		case EDomanda::Coraggio: return VociCoraggio;
		case EDomanda::Devozione: return VociDevozione;
		case EDomanda::Animo: return VociAnimo;
		case EDomanda::RichiamoMagia: return VociMagia;
		default: return TArrayView<const FValdorsoVoceRegistro>();
		}
	}

	const TCHAR* Domanda(EDomanda Quale, const FString& Sesso)
	{
		if (Sesso == TEXT("donna"))
		{
			switch (Quale)
			{
			case EDomanda::Motivo: return TEXT("Perché sei venuta nella valle?");
			case EDomanda::Onesta: return TEXT("Sei onesta o furba?");
			case EDomanda::Coraggio: return TEXT("Sei coraggiosa o prudente?");
			case EDomanda::Devozione: return TEXT("Sei devota o scettica?");
			case EDomanda::Animo: return TEXT("Sei gentile o dura?");
			default: break;
			}
		}
		switch (Quale)
		{
		case EDomanda::Sesso: return TEXT("Chi sei?");
		case EDomanda::Fede: return TEXT("A quale fede affidi la tua anima?");
		case EDomanda::Origine: return TEXT("Da dove vieni?");
		case EDomanda::Mestiere: return TEXT("Cosa facevi prima?");
		case EDomanda::Motivo: return TEXT("Perché sei venuto nella valle?");
		case EDomanda::Ricordo: return TEXT("Cosa porti con te?");
		case EDomanda::Paura: return TEXT("Cosa temi di più?");
		case EDomanda::Onesta: return TEXT("Sei onesto o furbo?");
		case EDomanda::Coraggio: return TEXT("Sei coraggioso o prudente?");
		case EDomanda::Devozione: return TEXT("Sei devoto o scettico?");
		case EDomanda::Animo: return TEXT("Sei gentile o duro?");
		case EDomanda::RichiamoMagia: return TEXT("Hai mai sentito qualcosa rispondere alla tua volontà? Una candela che trema, il vento che cambia?");
		default: return TEXT("");
		}
	}

	FString& Risposta(FValdorsoRegistro& Registro, EDomanda Quale)
	{
		switch (Quale)
		{
		case EDomanda::Sesso: return Registro.Sesso;
		case EDomanda::Fede: return Registro.Fede;
		case EDomanda::Origine: return Registro.Origine;
		case EDomanda::Mestiere: return Registro.Mestiere;
		case EDomanda::Motivo: return Registro.Motivo;
		case EDomanda::Ricordo: return Registro.Ricordo;
		case EDomanda::Paura: return Registro.Paura;
		case EDomanda::Onesta: return Registro.Onesta;
		case EDomanda::Coraggio: return Registro.Coraggio;
		case EDomanda::Devozione: return Registro.Devozione;
		case EDomanda::Animo: return Registro.Animo;
		default: return Registro.RichiamoMagia;
		}
	}

	const FString& Risposta(const FValdorsoRegistro& Registro, EDomanda Quale)
	{
		return Risposta(const_cast<FValdorsoRegistro&>(Registro), Quale);
	}

	FString Testo(EDomanda Quale, const FString& Scelta, const FString& Sesso)
	{
		const FValdorsoVoceRegistro* Voce = Trova(Quale, Scelta);
		if (!Voce)
		{
			return FString();
		}
		return (Sesso == TEXT("donna") && Voce->Femminile) ? Voce->Femminile : Voce->Maschile;
	}

	FString Problema(const FValdorsoRegistro& Registro, bool bCompleto, const FString& NomePersonaggio)
	{
		for (int32 i = 0; i < static_cast<int32>(EDomanda::Numero); ++i)
		{
			const EDomanda Quale = static_cast<EDomanda>(i);
			const FString& Valore = Risposta(Registro, Quale);
			if (Valore.IsEmpty())
			{
				if (bCompleto)
				{
					return FString::Printf(TEXT("Manca una risposta: \"%s\""), Domanda(Quale, Registro.Sesso));
				}
				continue;
			}
			if (!Trova(Quale, Valore))
			{
				return TEXT("Una risposta del registro non è valida: aggiorna il gioco.");
			}
		}
		if ((bCompleto || Registro.Eta != 0) && (Registro.Eta < EtaMinima || Registro.Eta > EtaMassima))
		{
			return FString::Printf(TEXT("L'età va da %d a %d anni."), EtaMinima, EtaMassima);
		}
		// (06/10, passo 4.4) L'aspetto: se è scelto, ogni valore dentro il catalogo (la barba solo per gli uomini).
		const FString SullAspetto = ValdorsoAspetto::Problema(Registro.Aspetto, Registro.Sesso);
		if (!SullAspetto.IsEmpty())
		{
			return SullAspetto;
		}
		const FString SulRacconto = ValdorsoRegole::ProblemaTestoLibero(Registro.Racconto, MassimoRacconto);
		if (!SulRacconto.IsEmpty())
		{
			return SulRacconto;
		}
		const FString SullaStoria = ValdorsoRegole::ProblemaTestoLibero(Registro.Storia, MassimoStoria);
		if (!SullaStoria.IsEmpty())
		{
			return SullaStoria;
		}
		// (06/10, passo 4.5) La storia e il racconto ritoccato restano nel mondo di Valdorso.
		const FString NelMondo = ValdorsoRegole::ProblemaTestoNelMondo(Registro.Storia, NomePersonaggio);
		if (!NelMondo.IsEmpty())
		{
			return NelMondo;
		}
		const FString RaccontoNelMondo = ValdorsoRegole::ProblemaTestoNelMondo(Registro.Racconto, NomePersonaggio);
		if (!RaccontoNelMondo.IsEmpty())
		{
			return RaccontoNelMondo.Replace(TEXT("Nella storia"), TEXT("Nel racconto"));
		}
		// (05/10) La firma per ultima: si mette quando tutto il resto è a posto.
		return ProblemaFirma(Registro.Firma, bCompleto);
	}

	/** Legge i tratti senza guardare se sono abbastanza; OutPunti dice quanti punti ci sono. */
	bool LeggiTrattiFirma(const FString& Firma, TArray<TArray<FIntPoint>>& OutTratti, int32& OutPunti)
	{
		OutTratti.Reset();
		OutPunti = 0;
		if (Firma.IsEmpty() || Firma == FirmaColNome || Firma.Len() > MassimoLunghezzaFirma)
		{
			return false;
		}
		// Solo cifre, da una a tre (niente segni, decimali o numeri lunghissimi): il server non si fida del gioco.
		auto Cifre = [](const FString& Testo)
		{
			if (Testo.IsEmpty() || Testo.Len() > 3)
			{
				return false;
			}
			for (const TCHAR Carattere : Testo)
			{
				if (!FChar::IsDigit(Carattere))
				{
					return false;
				}
			}
			return true;
		};
		TArray<FString> Tratti;
		Firma.ParseIntoArray(Tratti, TEXT(";"), true);
		if (Tratti.Num() == 0 || Tratti.Num() > MassimoTrattiFirma)
		{
			return false;
		}
		for (const FString& Tratto : Tratti)
		{
			TArray<FString> Coppie;
			Tratto.ParseIntoArray(Coppie, TEXT(" "), true);
			if (Coppie.Num() == 0)
			{
				return false;
			}
			TArray<FIntPoint>& Nuovo = OutTratti.AddDefaulted_GetRef();
			for (const FString& Coppia : Coppie)
			{
				FString X, Y;
				if (!Coppia.Split(TEXT(","), &X, &Y) || !Cifre(X) || !Cifre(Y) || ++OutPunti > MassimoPuntiFirma)
				{
					return false;
				}
				Nuovo.Add(FIntPoint(FCString::Atoi(*X), FCString::Atoi(*Y)));
			}
		}
		return true;
	}

	bool LeggiFirma(const FString& Firma, TArray<TArray<FIntPoint>>& OutTratti)
	{
		int32 Punti = 0;
		return LeggiTrattiFirma(Firma, OutTratti, Punti) && Punti >= MinimoPuntiFirma;
	}

	FString ProblemaFirma(const FString& Firma, bool bCompleto)
	{
		if (Firma.IsEmpty())
		{
			return bCompleto ? FString(TEXT("Manca la tua firma: disegnala nel riquadro, oppure firma con il nome.")) : FString();
		}
		if (Firma == FirmaColNome)
		{
			return FString();
		}
		TArray<TArray<FIntPoint>> Tratti;
		int32 Punti = 0;
		if (!LeggiTrattiFirma(Firma, Tratti, Punti))
		{
			return TEXT("La firma non si legge: cancellala e rifalla.");
		}
		// Una firma appena cominciata va bene per la bozza, non per firmare.
		if (bCompleto && Punti < MinimoPuntiFirma)
		{
			return TEXT("La firma è troppo corta: scrivila per intero.");
		}
		return FString();
	}

	FString ComponiRacconto(const FValdorsoRegistro& Registro, const FString& Nome)
	{
		const bool bDonna = Registro.Sesso == TEXT("donna");
		auto Minuscola = [](FString Parte)
		{
			if (!Parte.IsEmpty())
			{
				Parte[0] = FChar::ToLower(Parte[0]);
			}
			return Parte;
		};
		auto FraseDi = [bDonna](TArrayView<const FValdorsoVoceRegistro> Frasi, const FString& Scelta) -> FString
		{
			for (const FValdorsoVoceRegistro& Riga : Frasi)
			{
				if (Scelta == Riga.Chiave)
				{
					return bDonna ? Riga.Femminile : Riga.Maschile;
				}
			}
			return FString();
		};

		FString Scritto = FString::Printf(TEXT("Nell'anno 312 dopo il Crepuscolo, in una notte in cui il Cuore batteva lento, si presentò al tempio di Val d'Orso %s, %s di %d anni"),
			*Nome, *Genere(TEXT("un uomo"), TEXT("una donna"), bDonna), Registro.Eta);

		// Da dove viene.
		static const FValdorsoVoceRegistro FrasiOrigine[] = {
			{ TEXT("lago"), TEXT(", nato in un villaggio di pescatori sul lago, dove è l'acqua a decidere chi mangia"), TEXT(", nata in un villaggio di pescatori sul lago, dove è l'acqua a decidere chi mangia") },
			{ TEXT("minatori"), TEXT(", cresciuto in un borgo di minatori sui monti, tra il buio delle gallerie e il freddo delle cime"), TEXT(", cresciuta in un borgo di minatori sui monti, tra il buio delle gallerie e il freddo delle cime") },
			{ TEXT("capitale"), TEXT(", venuto dai quartieri poveri della capitale di Aurelia, dove si impara presto a guardarsi le spalle"), TEXT(", venuta dai quartieri poveri della capitale di Aurelia, dove si impara presto a guardarsi le spalle") },
			{ TEXT("sud"), TEXT(", figlio delle campagne del sud, abituato alla fatica dei campi"), TEXT(", figlia delle campagne del sud, abituata alla fatica dei campi") },
			{ TEXT("confine"), TEXT(", dalle terre di confine, cresciuto all'ombra delle rovine degli Antichi"), TEXT(", dalle terre di confine, cresciuta all'ombra delle rovine degli Antichi") },
			{ TEXT("dimenticato"), TEXT(", senza memoria del proprio passato: si era svegliato nel bosco, e non ricordava altro"), TEXT(", senza memoria del proprio passato: si era svegliata nel bosco, e non ricordava altro") },
		};
		Scritto += FraseDi(FrasiOrigine, Registro.Origine);
		Scritto += TEXT(".");

		// Cosa faceva e perché è venuto.
		const FString Mestiere = Testo(EDomanda::Mestiere, Registro.Mestiere, Registro.Sesso);
		if (!Mestiere.IsEmpty())
		{
			Scritto += FString::Printf(TEXT(" Prima era stat%s %s."), bDonna ? TEXT("a") : TEXT("o"), *Minuscola(Mestiere));
		}
		static const FValdorsoVoceRegistro FrasiMotivo[] = {
			{ TEXT("terra"), TEXT(" Era venuto per la terra promessa dalla corona."), TEXT(" Era venuta per la terra promessa dalla corona.") },
			{ TEXT("fuga"), TEXT(" Fuggiva da qualcosa, o da qualcuno, e non disse da cosa."), TEXT(" Fuggiva da qualcosa, o da qualcuno, e non disse da cosa.") },
			{ TEXT("scomparso"), TEXT(" Cercava una persona scomparsa, e ne disse il nome a bassa voce."), TEXT(" Cercava una persona scomparsa, e ne disse il nome a bassa voce.") },
			{ TEXT("sogno"), TEXT(" L'aveva chiamato un sogno, o una visione: un'ombra enorme tra gli alberi."), TEXT(" L'aveva chiamata un sogno, o una visione: un'ombra enorme tra gli alberi.") },
			{ TEXT("debito"), TEXT(" Aveva un debito da saldare, e qualcuno lo sapeva."), TEXT(" Aveva un debito da saldare, e qualcuno lo sapeva.") },
			{ TEXT("fede"), TEXT(" La fede l'aveva chiamato fin qui."), TEXT(" La fede l'aveva chiamata fin qui.") },
			{ TEXT("avventura"), TEXT(" Cercava l'avventura, come chi non ha ancora imparato a temerla."), TEXT(" Cercava l'avventura, come chi non ha ancora imparato a temerla.") },
		};
		Scritto += FraseDi(FrasiMotivo, Registro.Motivo);

		// Cosa porta e cosa teme.
		const FString Ricordo = Testo(EDomanda::Ricordo, Registro.Ricordo, Registro.Sesso);
		if (!Ricordo.IsEmpty())
		{
			FString Suo = Minuscola(Ricordo);
			Suo.ReplaceInline(TEXT("tua madre"), TEXT("sua madre"));
			Suo.ReplaceInline(TEXT("tuo padre"), TEXT("suo padre"));
			Scritto += FString::Printf(TEXT(" Portava con sé %s."), *Suo);
		}
		const FString Paura = Testo(EDomanda::Paura, Registro.Paura, Registro.Sesso);
		if (!Paura.IsEmpty())
		{
			Scritto += FString::Printf(TEXT(" Più di ogni cosa temeva %s."), *Minuscola(Paura));
		}

		// Il carattere.
		TArray<FString> Tratti;
		for (const EDomanda Quale : { EDomanda::Onesta, EDomanda::Coraggio, EDomanda::Devozione, EDomanda::Animo })
		{
			const FString Tratto = Testo(Quale, Risposta(Registro, Quale), Registro.Sesso);
			if (!Tratto.IsEmpty())
			{
				Tratti.Add(Minuscola(Tratto));
			}
		}
		if (Tratti.Num() > 0)
		{
			FString Elenco = Tratti[0];
			for (int32 i = 1; i < Tratti.Num(); ++i)
			{
				Elenco += (i == Tratti.Num() - 1 ? TEXT(" e ") : TEXT(", ")) + Tratti[i];
			}
			Scritto += FString::Printf(TEXT(" Il sacerdote annotò che era %s."), *Elenco);
		}

		// La fede e il richiamo della magia.
		if (Registro.Fede == TEXT("nessuna"))
		{
			Scritto += TEXT(" Non affidò la sua anima a nessun dio.");
		}
		else if (Registro.Fede == TEXT("vecchidei"))
		{
			Scritto += TEXT(" Affidò la sua anima ai Vecchi Dei della foresta, e il sacerdote finse di non sentire.");
		}
		else if (const FValdorsoVoceRegistro* Dio = Trova(EDomanda::Fede, Registro.Fede))
		{
			FString NomeDio = Dio->Maschile;
			int32 Virgola = INDEX_NONE;
			if (NomeDio.FindChar(TEXT(','), Virgola))
			{
				NomeDio.LeftInline(Virgola);
			}
			Scritto += FString::Printf(TEXT(" Affidò la sua anima a %s."), *NomeDio);
		}
		if (Registro.RichiamoMagia == TEXT("si"))
		{
			Scritto += TEXT(" Quando il frammento batté, la fiamma delle candele si piegò verso di lui.");
			if (bDonna)
			{
				Scritto.ReplaceInline(TEXT("verso di lui."), TEXT("verso di lei."));
			}
		}
		Scritto += TEXT(" Il Cuore batte ancora.");
		return Scritto;
	}

	void Casuale(FValdorsoRegistro& Registro, FRandomStream& Dadi, bool bTutte)
	{
		for (int32 i = 0; i < static_cast<int32>(EDomanda::Numero); ++i)
		{
			const EDomanda Quale = static_cast<EDomanda>(i);
			FString& Campo = Risposta(Registro, Quale);
			const TArrayView<const FValdorsoVoceRegistro> Possibili = Voci(Quale);
			if ((bTutte || Campo.IsEmpty()) && Possibili.Num() > 0)
			{
				Campo = Possibili[Dadi.RandRange(0, Possibili.Num() - 1)].Chiave;
			}
		}
		if (bTutte || Registro.Eta == 0)
		{
			Registro.Eta = Dadi.RandRange(EtaMinima, 40);
		}
		// (06/10, passo 4.4) Anche l'aspetto, se non era ancora scelto; altrimenti lo si adatta al sesso uscito.
		if (bTutte || ValdorsoAspetto::NonScelto(Registro.Aspetto))
		{
			ValdorsoAspetto::Casuale(Registro.Aspetto, Registro.Sesso, Dadi);
		}
		else
		{
			ValdorsoAspetto::Adatta(Registro.Aspetto, Registro.Sesso);
		}
	}

	void CasualeUna(FValdorsoRegistro& Registro, EDomanda Quale, FRandomStream& Dadi)
	{
		const TArrayView<const FValdorsoVoceRegistro> Possibili = Voci(Quale);
		if (Possibili.Num() == 0)
		{
			return;
		}
		FString& Campo = Risposta(Registro, Quale);
		int32 Scelto = Dadi.RandRange(0, Possibili.Num() - 1);
		if (Possibili.Num() > 1 && Campo == Possibili[Scelto].Chiave)
		{
			// Rilanciare i dadi deve cambiare qualcosa.
			Scelto = (Scelto + 1 + Dadi.RandRange(0, Possibili.Num() - 2)) % Possibili.Num();
		}
		Campo = Possibili[Scelto].Chiave;
		if (Quale == EDomanda::Sesso)
		{
			// (06/10) Cambiando sesso, l'aspetto resta com'è dove si può (niente barba per una donna).
			ValdorsoAspetto::Adatta(Registro.Aspetto, Registro.Sesso);
		}
	}

	bool StesseRisposte(const FValdorsoRegistro& A, const FValdorsoRegistro& B)
	{
		for (int32 i = 0; i < static_cast<int32>(EDomanda::Numero); ++i)
		{
			const EDomanda Quale = static_cast<EDomanda>(i);
			if (!Risposta(A, Quale).Equals(Risposta(B, Quale), ESearchCase::CaseSensitive))
			{
				return false;
			}
		}
		return A.Eta == B.Eta
			&& A.Racconto.Equals(B.Racconto, ESearchCase::CaseSensitive)
			&& A.Storia.Equals(B.Storia, ESearchCase::CaseSensitive)
			&& A.Firma.Equals(B.Firma, ESearchCase::CaseSensitive)
			&& ValdorsoAspetto::Uguali(A.Aspetto, B.Aspetto);
	}

	const TCHAR* Vantaggio(EDomanda Quale, const FString& Scelta)
	{
		struct FRigaVantaggio
		{
			const TCHAR* Chiave;
			const TCHAR* Frase;
		};
		static const FRigaVantaggio VantaggiOrigine[] = {
			{ TEXT("lago"), TEXT("Si nuota con meno fatica, e i pescatori ti trattano da uno di loro.") },
			{ TEXT("minatori"), TEXT("Nel buio delle grotte vedi un poco meglio, e riconosci i filoni.") },
			{ TEXT("capitale"), TEXT("Riconosci borsaioli e truffatori; ricettatori e gente di strada si fidano di te.") },
			{ TEXT("sud"), TEXT("Fame e fatica dei campi pesano meno.") },
			{ TEXT("confine"), TEXT("Conosci una runa degli Antichi.") },
			{ TEXT("dimenticato"), TEXT("Nessun vantaggio, ma una storia nascosta da scoprire: la memoria perduta.") },
		};
		static const FRigaVantaggio VantaggiMestiere[] = {
			{ TEXT("contadino"), TEXT("5 punti nell'arte di coltivare, e un attrezzo del vecchio mestiere.") },
			{ TEXT("pastore"), TEXT("5 punti nell'arte di allevare, e un attrezzo del vecchio mestiere.") },
			{ TEXT("apprendista"), TEXT("5 punti nel lavoro di bottega, e un attrezzo del vecchio mestiere.") },
			{ TEXT("soldato"), TEXT("5 punti nel combattere, e qualcosa del vecchio equipaggiamento.") },
			{ TEXT("servo"), TEXT("5 punti nel trattare con i nobili, e qualcosa della vecchia casa.") },
			{ TEXT("allievo"), TEXT("5 punti nello studio, e un quaderno di appunti dell'accademia.") },
			{ TEXT("orfano"), TEXT("5 punti nella preghiera, e un ricordo del tempio.") },
			{ TEXT("mercante"), TEXT("5 punti nel commercio, e una bilancia da mercante.") },
			{ TEXT("cacciatore"), TEXT("5 punti nella caccia, e un attrezzo del vecchio mestiere.") },
			{ TEXT("marinaio"), TEXT("5 punti nella pesca e nei nodi, e un attrezzo del vecchio mestiere.") },
		};
		static const FRigaVantaggio VantaggiMotivo[] = {
			{ TEXT("terra"), TEXT("L'aiuto del balivo e il diritto a un pezzo di terra.") },
			{ TEXT("fuga"), TEXT("La fama, buona o cattiva, cresce più piano. Ma qualcuno ti cerca.") },
			{ TEXT("scomparso"), TEXT("Una storia nascosta: la persona che cerchi.") },
			{ TEXT("sogno"), TEXT("Sogni che anticipano fatti veri della valle, legati all'Orso.") },
			{ TEXT("debito"), TEXT("Più Aureus all'inizio, ma un creditore verrà a riscuotere.") },
			{ TEXT("fede"), TEXT("Un po' di favore della tua divinità.") },
			{ TEXT("avventura"), TEXT("Scoprire luoghi nuovi ti dà di più.") },
		};
		static const FRigaVantaggio VantaggiRicordo[] = {
			{ TEXT("anello"), TEXT("Si può dare in pegno per un prestito, e poi riscattare. Non si perde morendo.") },
			{ TEXT("lettera"), TEXT("Una storia nascosta, quando troverai il coraggio di aprirla. Non si perde morendo.") },
			{ TEXT("spada"), TEXT("Un fabbro può riforgiarla in un'arma con una storia. Non si perde morendo.") },
			{ TEXT("amuleto"), TEXT("Una volta al giorno aiuta contro la paura. Non si perde morendo.") },
			{ TEXT("mappa"), TEXT("Indica un luogo nascosto della valle; l'altra metà è da trovare. Non si perde morendo.") },
			{ TEXT("libro"), TEXT("Si prega meglio, ed è il primo tomo della magia della luce. Non si perde morendo.") },
		};
		static const FRigaVantaggio VantaggiPaura[] = {
			{ TEXT("fuoco"), TEXT("Davanti al fuoco, all'inizio, esiti. Si vince affrontandolo, e vale un titolo.") },
			{ TEXT("acqua"), TEXT("All'inizio nuoti più lentamente. Si vince affrontandola, e vale un titolo.") },
			{ TEXT("buio"), TEXT("Nel buio, all'inizio, ti muovi incerto. Si vince affrontandolo, e vale un titolo.") },
			{ TEXT("solitudine"), TEXT("In solitudine, all'inizio, ti stanchi prima. Si vince affrontandola, e vale un titolo.") },
			{ TEXT("morti"), TEXT("I non morti, all'inizio, ti spaventano di più. Si vince affrontandoli, e vale un titolo.") },
			{ TEXT("magia"), TEXT("La magia, all'inizio, ti mette a disagio. Si vince affrontandola, e vale un titolo.") },
		};
		static const FRigaVantaggio VantaggiCarattere[] = {
			{ TEXT("onesto"), TEXT("Gli abitanti ti credono più facilmente.") },
			{ TEXT("furbo"), TEXT("Mercanteggi e inganni meglio.") },
			{ TEXT("coraggioso"), TEXT("Le minacce ti fanno meno effetto.") },
			{ TEXT("prudente"), TEXT("Noti prima trappole e pericoli.") },
			{ TEXT("devoto"), TEXT("Il favore divino cresce prima.") },
			{ TEXT("scettico"), TEXT("Illusioni e inganni magici fanno meno presa.") },
			{ TEXT("gentile"), TEXT("Animali e abitanti timidi si avvicinano.") },
			{ TEXT("duro"), TEXT("Intimidisci meglio.") },
		};
		static const FRigaVantaggio VantaggiMagia[] = {
			{ TEXT("si"), TEXT("5 punti di Concentrazione e, un giorno, una lettera dall'accademia.") },
			{ TEXT("no"), TEXT("Non perdi niente: la magia si può sempre studiare.") },
		};

		TArrayView<const FRigaVantaggio> Elenco;
		switch (Quale)
		{
		case EDomanda::Origine: Elenco = VantaggiOrigine; break;
		case EDomanda::Mestiere: Elenco = VantaggiMestiere; break;
		case EDomanda::Motivo: Elenco = VantaggiMotivo; break;
		case EDomanda::Ricordo: Elenco = VantaggiRicordo; break;
		case EDomanda::Paura: Elenco = VantaggiPaura; break;
		case EDomanda::Onesta:
		case EDomanda::Coraggio:
		case EDomanda::Devozione:
		case EDomanda::Animo: Elenco = VantaggiCarattere; break;
		case EDomanda::RichiamoMagia: Elenco = VantaggiMagia; break;
		default: break;
		}
		for (const FRigaVantaggio& Voce : Elenco)
		{
			if (Scelta == Voce.Chiave)
			{
				return Voce.Frase;
			}
		}
		return TEXT("");
	}

	EDomanda PrimaMancante(const FValdorsoRegistro& Registro)
	{
		if (Registro.Eta < EtaMinima || Registro.Eta > EtaMassima)
		{
			return EDomanda::Sesso;
		}
		for (int32 i = 0; i < static_cast<int32>(EDomanda::Numero); ++i)
		{
			const EDomanda Quale = static_cast<EDomanda>(i);
			if (Risposta(Registro, Quale).IsEmpty())
			{
				return Quale;
			}
		}
		return EDomanda::Numero;
	}

	FValdorsoOggettoRicordo OggettoDelRicordo(const FString& Chiave)
	{
		static const FValdorsoOggettoRicordo Ricordi[] = {
			{ TEXT("Ricordo_Anello"), TEXT("L'anello di tua madre"), TEXT("Un anello d'argento consumato. Si può dare in pegno e poi riscattare.") },
			{ TEXT("Ricordo_Lettera"), TEXT("Una lettera mai aperta"), TEXT("Il sigillo è intatto. Dentro c'è una storia, quando troverai il coraggio.") },
			{ TEXT("Ricordo_Spada"), TEXT("La spada spezzata di tuo padre"), TEXT("Due pezzi avvolti in un panno. Un fabbro potrebbe riforgiarla.") },
			{ TEXT("Ricordo_Amuleto"), TEXT("Un amuleto di legno"), TEXT("Intagliato a forma di zampa. Stretto in mano, la paura pesa meno.") },
			{ TEXT("Ricordo_Mappa"), TEXT("Una mappa strappata"), TEXT("Mezza mappa della valle, con un segno. L'altra metà è da qualche parte.") },
			{ TEXT("Ricordo_Libro"), TEXT("Un libro di preghiere"), TEXT("Le pagine sanno di cera. Il primo tomo della luce.") },
		};
		static const TCHAR* const Chiavi[] = { TEXT("anello"), TEXT("lettera"), TEXT("spada"), TEXT("amuleto"), TEXT("mappa"), TEXT("libro") };
		for (int32 i = 0; i < UE_ARRAY_COUNT(Chiavi); ++i)
		{
			if (Chiave == Chiavi[i])
			{
				return Ricordi[i];
			}
		}
		return FValdorsoOggettoRicordo{ NAME_None, TEXT(""), TEXT("") };
	}
}
