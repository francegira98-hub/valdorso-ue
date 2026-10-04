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

	FString Problema(const FValdorsoRegistro& Registro, bool bCompleto)
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
		const FString SulRacconto = ValdorsoRegole::ProblemaTestoLibero(Registro.Racconto, MassimoRacconto);
		if (!SulRacconto.IsEmpty())
		{
			return SulRacconto;
		}
		return ValdorsoRegole::ProblemaTestoLibero(Registro.Storia, MassimoStoria);
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
	}
}
