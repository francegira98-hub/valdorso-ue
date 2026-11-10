// Valdorso - La Gilda e il Balivo: i dati e le regole (vedi il .h). Scritto da Claude il 10/11/2026.

#include "ValdorsoGildaDati.h"
#include "ValdorsoLupi.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoTaccuinoDati.h"
#include "Math/RandomStream.h"

#define LOCTEXT_NAMESPACE "ValdorsoGilda"

namespace
{
	/** Un incarico che gli abitanti mettono sulla bacheca. */
	struct FGildaModello
	{
		const TCHAR* Id;
		const TCHAR* Committente;
		const TCHAR* Titolo;
		const TCHAR* Testo;
		EValdorsoTipoIncarico Tipo;
		const TCHAR* Obiettivo;
		int32 Quanti;
		EValdorsoGrado Grado;
		int32 Monete;
		int32 Punti;
		int32 Posti;
		int32 Giorni;
	};

	using GildaTipo = EValdorsoTipoIncarico;
	using GildaGrado = EValdorsoGrado;

	const FGildaModello GildaModelli[] = {
		{ TEXT("LupiPascoli"), TEXT("Gaspare, il vecchio cacciatore"), TEXT("I lupi del boschetto"),
			TEXT("Di notte scendono dal boschetto fino alle stalle. Tre, e non tornano piu'. Chi ha un'arma e non ha paura li cerchi dopo il tramonto."),
			GildaTipo::Caccia, TEXT("Lupo"), 3, GildaGrado::Bronzo, 45, 4, 2, 3 },
		{ TEXT("CinghialiCampi"), TEXT("Teodoro, il mugnaio"), TEXT("Cinghiali nel campo di grano"),
			TEXT("Mi hanno rivoltato mezzo campo. Due cinghiali, quelli grossi della palude: portatemeli via e vi pago io, parola di Teodoro."),
			GildaTipo::Caccia, TEXT("Cinghiale"), 2, GildaGrado::Comune, 25, 2, 2, 3 },
		{ TEXT("VolpePollaio"), TEXT("Agnese, delle fattorie"), TEXT("La volpe nel pollaio"),
			TEXT("Una volpe rossa, furba come il diavolo: viene dal cimitero, di notte. Mi ha preso tre galline in una settimana."),
			GildaTipo::Caccia, TEXT("Volpe"), 1, GildaGrado::Comune, 15, 1, 2, 2 },
		{ TEXT("OrsoCollina"), TEXT("La Gilda di Valdorso"), TEXT("La bestia della Collina dell'Orso"),
			TEXT("L'orso della collina ha sfondato il capanno dei carbonai. Non e' lavoro per novellini: andateci preparati, e non da soli."),
			GildaTipo::Caccia, TEXT("Orso"), 1, GildaGrado::Argento, 120, 10, 1, 4 },
		{ TEXT("CerviPascoli"), TEXT("Bruno, il locandiere"), TEXT("Carne di cervo per la locanda"),
			TEXT("Per lo stufato della festa mi servono due cervi dei pascoli. Di giorno, che di notte i cervi dormono e i lupi no."),
			GildaTipo::Caccia, TEXT("Cervo"), 2, GildaGrado::Comune, 30, 2, 2, 3 },
		{ TEXT("LegnaLocanda"), TEXT("Bruno, il locandiere"), TEXT("Legna per il camino"),
			TEXT("Il camino della locanda mangia piu' dei clienti. Sei fasci di rami secchi, portateli a Lucia e lei me li manda."),
			GildaTipo::Consegna, TEXT("Rami_Secchi"), 6, GildaGrado::Comune, 12, 1, 3, 2 },
		{ TEXT("FunghiStufato"), TEXT("Bruno, il locandiere"), TEXT("Funghi per lo stufato"),
			TEXT("Cinque manciate di funghi di bosco. Quelli buoni: se Gaspare dice di no, e' no."),
			GildaTipo::Consegna, TEXT("Funghi"), 5, GildaGrado::Comune, 10, 1, 3, 2 },
		{ TEXT("PelliFucina"), TEXT("Marta, la fabbra"), TEXT("Pelli di cinghiale per la fucina"),
			TEXT("Per i mantici e per le imbracciature degli scudi. Due pelli di cinghiale, intere, non bucherellate come un colabrodo."),
			GildaTipo::Consegna, TEXT("Pelle_Cinghiale"), 2, GildaGrado::Bronzo, 30, 3, 2, 3 },
		{ TEXT("CarneLocanda"), TEXT("Bruno, il locandiere"), TEXT("Carne per la locanda"),
			TEXT("Quattro pezzi di carne cruda, fresca di giornata. Se puzza, la riporti indietro e la mangi tu."),
			GildaTipo::Consegna, TEXT("Carne_Cruda"), 4, GildaGrado::Comune, 18, 2, 2, 2 },
		{ TEXT("MessaggioPasso"), TEXT("Tebaldo, guardia della Corona"), TEXT("Un messaggio al posto di guardia"),
			TEXT("Il cambio della guardia non e' arrivato. Qualcuno salga al posto di guardia del passo a vedere se Fulvio e' ancora vivo, poi torni a dirlo a Lucia."),
			GildaTipo::Raggiungi, TEXT("Posto di guardia del passo"), 1, GildaGrado::Comune, 15, 2, 3, 2 },
		{ TEXT("MappaRovine"), TEXT("Lucia, della Gilda"), TEXT("La strada delle Rovine"),
			TEXT("La mappa della Gilda delle Rovine degli Antichi e' vecchia di dieci anni. Andate fin dentro la cinta e tornate a raccontarmi."),
			GildaTipo::Raggiungi, TEXT("Rovine degli Antichi"), 1, GildaGrado::Bronzo, 30, 3, 2, 3 },
		{ TEXT("GrottaEst"), TEXT("Gaspare, il vecchio cacciatore"), TEXT("Che cosa c'e' nella grotta"),
			TEXT("Dalla grotta a est, di notte, si sentono martelli. Io non ci vado. Voi andateci, e tornate a dire a Lucia che cosa avete visto."),
			GildaTipo::Raggiungi, TEXT("Grotta est"), 1, GildaGrado::Bronzo, 35, 3, 2, 3 },
		{ TEXT("Cascata"), TEXT("Padre Anselmo"), TEXT("L'acqua della cascata"),
			TEXT("Per la benedizione delle sementi serve sapere se la cascata del nord e' gelata. Salite a vederla."),
			GildaTipo::Raggiungi, TEXT("Cascata"), 1, GildaGrado::Comune, 20, 2, 2, 3 },
	};

	/** Il plurale di una specie ("lupi"). */
	FString GildaPluraleSpecie(FName Specie, int32 Quanti)
	{
		struct FCoppia { const TCHAR* Id; const TCHAR* Uno; const TCHAR* Tanti; };
		static const FCoppia Coppie[] = {
			{ TEXT("Lupo"), TEXT("lupo"), TEXT("lupi") },
			{ TEXT("Cinghiale"), TEXT("cinghiale"), TEXT("cinghiali") },
			{ TEXT("Orso"), TEXT("orso"), TEXT("orsi") },
			{ TEXT("Cervo"), TEXT("cervo"), TEXT("cervi") },
			{ TEXT("Lepre"), TEXT("lepre"), TEXT("lepri") },
			{ TEXT("Volpe"), TEXT("volpe"), TEXT("volpi") },
		};
		for (const FCoppia& C : Coppie)
		{
			if (Specie == FName(C.Id))
			{
				return Quanti == 1 ? C.Uno : C.Tanti;
			}
		}
		return Specie.ToString().ToLower();
	}

	FString GildaNomeOggetto(FName Id)
	{
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
		return Tipo ? FString(Tipo->Nome) : Id.ToString();
	}

	/** Un obiettivo valido per quel tipo? (la specie, l'oggetto, il luogo; per il luogo restituisce il nome giusto). */
	bool GildaObiettivoValido(EValdorsoTipoIncarico Tipo, FName& InOutObiettivo)
	{
		switch (Tipo)
		{
		case EValdorsoTipoIncarico::Caccia:
			for (const FValdorsoSpecie& S : ValdorsoAnimali::Tutte())
			{
				if (FCString::Stricmp(S.Id, *InOutObiettivo.ToString()) == 0)
				{
					InOutObiettivo = S.Id;
					return true;
				}
			}
			return false;
		case EValdorsoTipoIncarico::Consegna:
			for (const FValdorsoTipoOggetto& O : ValdorsoOggetti::Catalogo())
			{
				if (O.Id.ToString().Equals(InOutObiettivo.ToString(), ESearchCase::IgnoreCase) && !O.bRicordo)
				{
					InOutObiettivo = O.Id;
					return true;
				}
			}
			return false;
		case EValdorsoTipoIncarico::Raggiungi:
		{
			const FString Cercato = InOutObiettivo.ToString().Replace(TEXT("_"), TEXT(" "));
			for (const FValdorsoLuogo& L : ValdorsoLuoghi::Tutti())
			{
				if (L.Nome.Equals(Cercato, ESearchCase::IgnoreCase))
				{
					InOutObiettivo = FName(*L.Nome);
					return true;
				}
			}
			return false;
		}
		}
		return false;
	}

	FString GildaData(int64 Quando)
	{
		static const TCHAR* Mesi[] = { TEXT("gennaio"), TEXT("febbraio"), TEXT("marzo"), TEXT("aprile"), TEXT("maggio"), TEXT("giugno"),
			TEXT("luglio"), TEXT("agosto"), TEXT("settembre"), TEXT("ottobre"), TEXT("novembre"), TEXT("dicembre") };
		const FDateTime Data = FDateTime::FromUnixTimestamp(Quando);
		// la valle conta gli anni dalla fondazione del regno: il 2026 vero e' il 312
		return FString::Printf(TEXT("%d %s %d"), Data.GetDay(), Mesi[FMath::Clamp(Data.GetMonth(), 1, 12) - 1], Data.GetYear() - 2026 + 312);
	}
}

namespace ValdorsoGilda
{
	EValdorsoGrado GradoDi(int32 Punti)
	{
		if (Punti >= PuntiPer(EValdorsoGrado::Oro))
		{
			return EValdorsoGrado::Oro;
		}
		if (Punti >= PuntiPer(EValdorsoGrado::Argento))
		{
			return EValdorsoGrado::Argento;
		}
		if (Punti >= PuntiPer(EValdorsoGrado::Bronzo))
		{
			return EValdorsoGrado::Bronzo;
		}
		return EValdorsoGrado::Comune;
	}

	int32 PuntiPer(EValdorsoGrado Grado)
	{
		switch (Grado)
		{
		case EValdorsoGrado::Bronzo: return 10;
		case EValdorsoGrado::Argento: return 40;
		case EValdorsoGrado::Oro: return 120;
		default: return 0;
		}
	}

	FText NomeGrado(EValdorsoGrado Grado)
	{
		switch (Grado)
		{
		case EValdorsoGrado::Bronzo: return LOCTEXT("Bronzo", "Bronzo");
		case EValdorsoGrado::Argento: return LOCTEXT("Argento", "Argento");
		case EValdorsoGrado::Oro: return LOCTEXT("Oro", "Oro");
		default: return LOCTEXT("Comune", "Comune");
		}
	}

	EValdorsoGrado GradoDopo(EValdorsoGrado Grado)
	{
		return Grado >= EValdorsoGrado::Oro ? EValdorsoGrado::Oro : static_cast<EValdorsoGrado>(static_cast<uint8>(Grado) + 1);
	}

	bool Pronto(const FValdorsoGildaColono& Colono)
	{
		return Colono.Grado < EValdorsoGrado::Oro && Colono.Punti >= PuntiPer(GradoDopo(Colono.Grado));
	}

	FText PercheNoNomina(const FValdorsoGildaColono& Colono, EValdorsoGrado Nuovo, bool bStaff, const FValdorsoGildaColono* Capo, bool bStessoColono)
	{
		if (Nuovo == Colono.Grado)
		{
			return FText::Format(LOCTEXT("GiaGrado", "Ha gia' il grado {0}."), NomeGrado(Nuovo));
		}
		if (bStaff)
		{
			return FText::GetEmpty();
		}
		if (!Capo || !Capo->bCapoGilda)
		{
			return LOCTEXT("NonCapo", "I gradi li danno lo staff e il capo gilda.");
		}
		if (bStessoColono)
		{
			return LOCTEXT("NonASe", "Il capo gilda non si promuove da solo: ci pensa lo staff.");
		}
		if (Nuovo != GradoDopo(Colono.Grado) || Nuovo < Colono.Grado)
		{
			return LOCTEXT("UnoAllaVolta", "Il capo gilda promuove di un grado alla volta (e non retrocede: lo fa lo staff).");
		}
		if (Nuovo > MassimoDelCapo)
		{
			return FText::Format(LOCTEXT("SoloStaff", "Il grado {0} lo da' solo lo staff."), NomeGrado(Nuovo));
		}
		if (Colono.Punti < PuntiPer(Nuovo))
		{
			return FText::Format(LOCTEXT("PochiPunti", "Per il grado {0} servono {1} punti (ne ha {2})."), NomeGrado(Nuovo),
				FText::AsNumber(PuntiPer(Nuovo)), FText::AsNumber(Colono.Punti));
		}
		return FText::GetEmpty();
	}

	bool Nomina(FValdorsoGildaColono& Colono, EValdorsoGrado Nuovo)
	{
		if (Colono.Grado == Nuovo)
		{
			return false;
		}
		Colono.Grado = Nuovo;
		// si ricomincia a contare il "sei pronto" dal grado nuovo (anche dopo una retrocessione)
		Colono.AvvisatoPronto = Nuovo;
		return true;
	}

	bool DaAvvisarePronto(FValdorsoGildaColono& Colono)
	{
		if (!Pronto(Colono))
		{
			return false;
		}
		const EValdorsoGrado Dopo = GradoDopo(Colono.Grado);
		if (Colono.AvvisatoPronto >= Dopo)
		{
			return false;
		}
		Colono.AvvisatoPronto = Dopo;
		return true;
	}

	FValdorsoIncarico* Trova(FValdorsoBachecheSalvate& Bacheche, int32 Numero)
	{
		return Bacheche.Incarichi.FindByPredicate([Numero](const FValdorsoIncarico& I) { return I.Numero == Numero; });
	}

	const FValdorsoIncarico* Trova(const FValdorsoBachecheSalvate& Bacheche, int32 Numero)
	{
		return Bacheche.Incarichi.FindByPredicate([Numero](const FValdorsoIncarico& I) { return I.Numero == Numero; });
	}

	FValdorsoIncaricoPreso* Preso(FValdorsoGildaColono& Colono, int32 Numero)
	{
		return Colono.Presi.FindByPredicate([Numero](const FValdorsoIncaricoPreso& P) { return P.Foglio.Numero == Numero; });
	}

	const FValdorsoIncaricoPreso* Preso(const FValdorsoGildaColono& Colono, int32 Numero)
	{
		return Colono.Presi.FindByPredicate([Numero](const FValdorsoIncaricoPreso& P) { return P.Foglio.Numero == Numero; });
	}

	bool SullaBacheca(const FValdorsoIncarico& Incarico, int64 Adesso)
	{
		return Incarico.Scadenza > Adesso && Incarico.Presi < Incarico.Posti;
	}

	TArray<FValdorsoIncarico> Visibili(const FValdorsoBachecheSalvate& Bacheche, int64 Adesso)
	{
		TArray<FValdorsoIncarico> Fuori;
		for (const FValdorsoIncarico& I : Bacheche.Incarichi)
		{
			if (SullaBacheca(I, Adesso))
			{
				Fuori.Add(I);
			}
		}
		Fuori.StableSort([](const FValdorsoIncarico& A, const FValdorsoIncarico& B)
		{
			if (A.Grado != B.Grado)
			{
				return A.Grado < B.Grado;
			}
			return A.Scadenza < B.Scadenza;
		});
		return Fuori;
	}

	FText PercheNoStacca(const FValdorsoIncarico& Incarico, const FValdorsoGildaColono& Colono, int64 Adesso)
	{
		if (Incarico.Scadenza <= Adesso)
		{
			return LOCTEXT("Scaduto", "Questo incarico e' scaduto.");
		}
		if (Preso(Colono, Incarico.Numero))
		{
			return LOCTEXT("GiaTuo", "Questo foglio ce l'hai gia'.");
		}
		if (Incarico.Presi >= Incarico.Posti)
		{
			return LOCTEXT("Pieno", "I posti sono finiti.");
		}
		if (Colono.Grado < Incarico.Grado)
		{
			return FText::Format(LOCTEXT("Grado", "Serve il grado {0} della Gilda (il tuo: {1})."), NomeGrado(Incarico.Grado), NomeGrado(Colono.Grado));
		}
		if (Colono.Presi.Num() >= MassimoPresi)
		{
			return FText::Format(LOCTEXT("Troppi", "Hai gia' {0} incarichi: prima finiscine uno (o rinuncia, da Lucia)."), FText::AsNumber(MassimoPresi));
		}
		return FText::GetEmpty();
	}

	bool Stacca(FValdorsoBachecheSalvate& Bacheche, FValdorsoGildaColono& Colono, int32 Numero, int64 Adesso)
	{
		FValdorsoIncarico* Incarico = Trova(Bacheche, Numero);
		if (!Incarico || !PercheNoStacca(*Incarico, Colono, Adesso).IsEmpty())
		{
			return false;
		}
		++Incarico->Presi;
		FValdorsoIncaricoPreso& Nuovo = Colono.Presi.AddDefaulted_GetRef();
		Nuovo.Foglio = *Incarico;
		Nuovo.Stato = EValdorsoStatoIncarico::Prenotato;
		Nuovo.ScadePrenotazione = FMath::Min(Adesso + SecondiPrenotazione, Incarico->Scadenza);
		return true;
	}

	int32 Cauzione(const FValdorsoIncarico& Incarico)
	{
		return Incarico.Grado >= EValdorsoGrado::Argento ? FMath::Max(1, Incarico.Monete / 10) : 0;
	}

	FText PercheNoAssegna(const FValdorsoGildaColono& Colono, int32 Numero, int32 Monete, int64 Adesso)
	{
		const FValdorsoIncaricoPreso* P = Preso(Colono, Numero);
		if (!P)
		{
			return LOCTEXT("NonTuo", "Questo foglio non e' tuo.");
		}
		if (P->Stato != EValdorsoStatoIncarico::Prenotato)
		{
			return LOCTEXT("GiaAssegnato", "Questo incarico ce l'hai gia' assegnato.");
		}
		if (P->ScadePrenotazione <= Adesso || P->Foglio.Scadenza <= Adesso)
		{
			return LOCTEXT("PrenotazioneScaduta", "La prenotazione e' scaduta: il foglio torna sulla bacheca.");
		}
		if (Colono.Grado < P->Foglio.Grado)
		{
			return FText::Format(LOCTEXT("GradoLucia", "Lucia scuote la testa: serve il grado {0}."), NomeGrado(P->Foglio.Grado));
		}
		const int32 Serve = Cauzione(P->Foglio);
		if (Monete < Serve)
		{
			return FText::Format(LOCTEXT("Cauzione", "Per questo incarico Lucia vuole una cauzione di {0} Aureus (torna alla fine)."), FText::AsNumber(Serve));
		}
		return FText::GetEmpty();
	}

	bool Assegna(FValdorsoGildaColono& Colono, int32 Numero, int32 Monete, int64 Adesso)
	{
		if (!PercheNoAssegna(Colono, Numero, Monete, Adesso).IsEmpty())
		{
			return false;
		}
		FValdorsoIncaricoPreso* P = Preso(Colono, Numero);
		P->Stato = EValdorsoStatoIncarico::Assegnato;
		P->Cauzione = Cauzione(P->Foglio);
		P->Progresso = 0;
		return true;
	}

	int32 Avanza(FValdorsoGildaColono& Colono, EValdorsoTipoIncarico Tipo, FName Obiettivo, int32 Quanti)
	{
		int32 Avanzati = 0;
		for (FValdorsoIncaricoPreso& P : Colono.Presi)
		{
			if (P.Stato != EValdorsoStatoIncarico::Assegnato || P.Foglio.Tipo != Tipo || P.Foglio.Obiettivo != Obiettivo
				|| Tipo == EValdorsoTipoIncarico::Consegna || P.Progresso >= P.Foglio.Quanti)
			{
				continue;
			}
			P.Progresso = FMath::Min(P.Foglio.Quanti, P.Progresso + FMath::Max(1, Quanti));
			++Avanzati;
		}
		return Avanzati;
	}

	bool Fatto(const FValdorsoIncaricoPreso& P, int32 Nello)
	{
		if (P.Stato != EValdorsoStatoIncarico::Assegnato)
		{
			return false;
		}
		return P.Foglio.Tipo == EValdorsoTipoIncarico::Consegna ? Nello >= P.Foglio.Quanti : P.Progresso >= P.Foglio.Quanti;
	}

	FText TestoLavoro(const FValdorsoIncarico& I)
	{
		switch (I.Tipo)
		{
		case EValdorsoTipoIncarico::Caccia:
			return FText::Format(LOCTEXT("LavoroCaccia", "Caccia: {0} {1}"), FText::AsNumber(I.Quanti), FText::FromString(GildaPluraleSpecie(I.Obiettivo, I.Quanti)));
		case EValdorsoTipoIncarico::Consegna:
			return FText::Format(LOCTEXT("LavoroConsegna", "Consegna: {0} × {1}"), FText::AsNumber(I.Quanti), FText::FromString(GildaNomeOggetto(I.Obiettivo)));
		case EValdorsoTipoIncarico::Raggiungi:
			return FText::Format(LOCTEXT("LavoroRaggiungi", "Raggiungi: {0}"), FText::FromName(I.Obiettivo));
		}
		return FText::GetEmpty();
	}

	FText TestoProgresso(const FValdorsoIncaricoPreso& P, int32 Nello)
	{
		if (P.Stato == EValdorsoStatoIncarico::Prenotato)
		{
			return LOCTEXT("DaAssegnare", "Da portare a Lucia, che lo assegna.");
		}
		const FValdorsoIncarico& I = P.Foglio;
		switch (I.Tipo)
		{
		case EValdorsoTipoIncarico::Caccia:
			return FText::Format(LOCTEXT("ProgCaccia", "{0} {1} su {2}"), FText::AsNumber(P.Progresso),
				FText::FromString(GildaPluraleSpecie(I.Obiettivo, P.Progresso)), FText::AsNumber(I.Quanti));
		case EValdorsoTipoIncarico::Consegna:
			return FText::Format(LOCTEXT("ProgConsegna", "Nello zaino: {0} su {1}"), FText::AsNumber(FMath::Min(Nello, I.Quanti)), FText::AsNumber(I.Quanti));
		case EValdorsoTipoIncarico::Raggiungi:
			return P.Progresso >= I.Quanti ? LOCTEXT("ProgRaggiunto", "Raggiunto: torna da Lucia.") : LOCTEXT("ProgDaRaggiungere", "Non ancora raggiunto.");
		}
		return FText::GetEmpty();
	}

	FText TestoScadenza(int64 Scadenza, int64 Adesso)
	{
		const int64 Resta = Scadenza - Adesso;
		if (Resta <= 0)
		{
			return LOCTEXT("ScadutoBreve", "scaduto");
		}
		if (Resta >= 2 * 86400)
		{
			return FText::Format(LOCTEXT("FraGiorni", "fra {0} giorni"), FText::AsNumber(Resta / 86400));
		}
		if (Resta >= 2 * 3600)
		{
			return FText::Format(LOCTEXT("FraOre", "fra {0} ore"), FText::AsNumber(Resta / 3600));
		}
		return FText::Format(LOCTEXT("FraMinuti", "fra {0} minuti"), FText::AsNumber(FMath::Max<int64>(1, Resta / 60)));
	}

	bool Compi(FValdorsoBachecheSalvate& Bacheche, FValdorsoGildaColono& Colono, int32 Numero, FValdorsoIncaricoPreso& OutPreso)
	{
		const int32 Indice = Colono.Presi.IndexOfByPredicate([Numero](const FValdorsoIncaricoPreso& P) { return P.Foglio.Numero == Numero; });
		if (Indice == INDEX_NONE || Colono.Presi[Indice].Stato != EValdorsoStatoIncarico::Assegnato)
		{
			return false;
		}
		OutPreso = Colono.Presi[Indice];
		Colono.Presi.RemoveAt(Indice);
		Colono.Punti += FMath::Max(0, OutPreso.Foglio.Punti);
		++Colono.Compiuti;
		// sulla bacheca un posto compiuto: la sua paga era tenuta da parte, adesso e' spesa (non torna nella cassa)
		if (FValdorsoIncarico* I = Trova(Bacheche, Numero))
		{
			++I->Compiuti;
		}
		return true;
	}

	bool Rinuncia(FValdorsoBachecheSalvate& Bacheche, FValdorsoGildaColono& Colono, int32 Numero)
	{
		const int32 Indice = Colono.Presi.IndexOfByPredicate([Numero](const FValdorsoIncaricoPreso& P) { return P.Foglio.Numero == Numero; });
		if (Indice == INDEX_NONE)
		{
			return false;
		}
		const bool bAssegnato = Colono.Presi[Indice].Stato == EValdorsoStatoIncarico::Assegnato;
		Colono.Presi.RemoveAt(Indice);
		if (bAssegnato)
		{
			Colono.Punti = FMath::Max(0, Colono.Punti - PuntiRinuncia);
			++Colono.Abbandonati;
		}
		if (FValdorsoIncarico* I = Trova(Bacheche, Numero))
		{
			I->Presi = FMath::Max(I->Compiuti, I->Presi - 1);
		}
		return true;
	}

	TArray<FValdorsoIncaricoPreso> Scadute(FValdorsoBachecheSalvate& Bacheche, FValdorsoGildaColono& Colono, int64 Adesso)
	{
		TArray<FValdorsoIncaricoPreso> Tolti;
		for (int32 i = Colono.Presi.Num() - 1; i >= 0; --i)
		{
			const FValdorsoIncaricoPreso& P = Colono.Presi[i];
			const bool bPrenotazione = P.Stato == EValdorsoStatoIncarico::Prenotato && P.ScadePrenotazione <= Adesso;
			if (!bPrenotazione && P.Foglio.Scadenza > Adesso)
			{
				continue;
			}
			Tolti.Add(P);
			if (FValdorsoIncarico* I = Trova(Bacheche, P.Foglio.Numero))
			{
				I->Presi = FMath::Max(I->Compiuti, I->Presi - 1);
			}
			Colono.Presi.RemoveAt(i);
		}
		return Tolti;
	}

	int32 Pulisci(FValdorsoBachecheSalvate& Bacheche, int64 Adesso)
	{
		int32 Tolti = 0;
		for (int32 i = Bacheche.Incarichi.Num() - 1; i >= 0; --i)
		{
			const FValdorsoIncarico& I = Bacheche.Incarichi[i];
			if (I.Scadenza > Adesso && I.Compiuti < I.Posti)
			{
				continue;
			}
			if (I.bAbitanti)
			{
				// i posti non compiuti tornano nella cassa del villaggio
				Bacheche.Cassa = FMath::Min(CassaMassima, Bacheche.Cassa + I.Monete * FMath::Max(0, I.Posti - I.Compiuti));
			}
			Bacheche.Incarichi.RemoveAt(i);
			++Tolti;
		}
		return Tolti;
	}

	void Rendita(FValdorsoBachecheSalvate& Bacheche, int64 Adesso)
	{
		if (Bacheche.UltimaRendita <= 0 || Bacheche.UltimaRendita > Adesso)
		{
			Bacheche.UltimaRendita = Adesso;
			return;
		}
		const int64 Giorni = (Adesso - Bacheche.UltimaRendita) / 86400;
		if (Giorni <= 0)
		{
			return;
		}
		Bacheche.UltimaRendita += Giorni * 86400;
		Bacheche.Cassa = static_cast<int32>(FMath::Min<int64>(CassaMassima, Bacheche.Cassa + Giorni * RenditaAlGiorno));
	}

	bool Genera(FValdorsoBachecheSalvate& Bacheche, int64 Adesso, FRandomStream& Caso)
	{
		int32 Degli = 0;
		TSet<FName> GiaSu;
		for (const FValdorsoIncarico& I : Bacheche.Incarichi)
		{
			if (I.bAbitanti)
			{
				++Degli;
				GiaSu.Add(I.Modello);
			}
		}
		if (Degli >= MassimoAbitanti || Bacheche.Incarichi.Num() >= MassimoIncarichi)
		{
			return false;
		}
		TArray<const FGildaModello*> Possibili;
		for (const FGildaModello& M : GildaModelli)
		{
			if (!GiaSu.Contains(FName(M.Id)) && M.Monete * M.Posti <= Bacheche.Cassa)
			{
				Possibili.Add(&M);
			}
		}
		if (Possibili.IsEmpty())
		{
			return false;
		}
		const FGildaModello& M = *Possibili[Caso.RandRange(0, Possibili.Num() - 1)];
		FValdorsoIncarico I;
		I.Titolo = M.Titolo;
		I.Testo = M.Testo;
		I.Committente = M.Committente;
		I.Tipo = M.Tipo;
		I.Obiettivo = M.Obiettivo;
		I.Quanti = M.Quanti;
		I.Grado = M.Grado;
		I.Monete = M.Monete;
		I.Punti = M.Punti;
		I.Posti = M.Posti;
		// la scadenza: i giorni del modello, con qualche ora in piu' o in meno (i fogli non scadono tutti insieme)
		I.Scadenza = Adesso + M.Giorni * 86400 + Caso.RandRange(-6, 6) * 3600;
		I.bAbitanti = true;
		I.Modello = M.Id;
		if (!Metti(Bacheche, I, Adesso))
		{
			return false;
		}
		Bacheche.Cassa -= M.Monete * M.Posti;
		return true;
	}

	int32 NumeroModelli()
	{
		return UE_ARRAY_COUNT(GildaModelli);
	}

	bool DaComando(const FString& Riga, FValdorsoIncarico& Out, FString& Errore)
	{
		static const TCHAR* Forma = TEXT("Scrivi: /incarico caccia|consegna|raggiungi obiettivo quanti comune|bronzo|argento|oro monete posti giorni titolo | testo");
		FString Prima;
		FString Testo;
		if (!Riga.Split(TEXT("|"), &Prima, &Testo))
		{
			Errore = Forma;
			return false;
		}
		TArray<FString> Parti;
		Prima.TrimStartAndEnd().ParseIntoArrayWS(Parti);
		if (Parti.Num() < 8)
		{
			Errore = Forma;
			return false;
		}
		FValdorsoIncarico I;
		const FString Tipo = Parti[0].ToLower();
		if (Tipo == TEXT("caccia")) { I.Tipo = EValdorsoTipoIncarico::Caccia; }
		else if (Tipo == TEXT("consegna")) { I.Tipo = EValdorsoTipoIncarico::Consegna; }
		else if (Tipo == TEXT("raggiungi")) { I.Tipo = EValdorsoTipoIncarico::Raggiungi; }
		else
		{
			Errore = TEXT("Il tipo e' caccia, consegna o raggiungi.");
			return false;
		}
		I.Obiettivo = FName(*Parti[1]);
		if (!GildaObiettivoValido(I.Tipo, I.Obiettivo))
		{
			Errore = FString::Printf(TEXT("Non conosco \"%s\": una specie (Lupo...), l'Id di un oggetto (Rami_Secchi...) o un luogo (Grotta_est...)."), *Parti[1].Left(30));
			return false;
		}
		auto Numero = [&Parti](int32 Quale, int32 Minimo, int32 Massimo, int32& OutValore)
		{
			if (!Parti[Quale].IsNumeric())
			{
				return false;
			}
			OutValore = FCString::Atoi(*Parti[Quale]);
			return OutValore >= Minimo && OutValore <= Massimo;
		};
		int32 Giorni = 0;
		if (!Numero(2, 1, 50, I.Quanti) || !Numero(4, 0, 5000, I.Monete) || !Numero(5, 1, 20, I.Posti) || !Numero(6, 1, 30, Giorni))
		{
			Errore = TEXT("Quanti 1-50, monete 0-5000, posti 1-20, giorni 1-30.");
			return false;
		}
		const FString Grado = Parti[3].ToLower();
		if (Grado == TEXT("comune")) { I.Grado = EValdorsoGrado::Comune; }
		else if (Grado == TEXT("bronzo")) { I.Grado = EValdorsoGrado::Bronzo; }
		else if (Grado == TEXT("argento")) { I.Grado = EValdorsoGrado::Argento; }
		else if (Grado == TEXT("oro")) { I.Grado = EValdorsoGrado::Oro; }
		else
		{
			Errore = TEXT("Il grado e' comune, bronzo, argento o oro.");
			return false;
		}
		if (I.Tipo == EValdorsoTipoIncarico::Raggiungi)
		{
			I.Quanti = 1;
		}
		TArray<FString> Titolo;
		for (int32 k = 7; k < Parti.Num(); ++k)
		{
			Titolo.Add(Parti[k]);
		}
		I.Titolo = FString::Join(Titolo, TEXT(" ")).Left(60);
		I.Testo = Testo.TrimStartAndEnd().Left(300);
		if (I.Titolo.IsEmpty() || I.Testo.IsEmpty())
		{
			Errore = Forma;
			return false;
		}
		I.Punti = FMath::Clamp(1 + static_cast<int32>(I.Grado) * 3, 1, 10);
		I.Committente = TEXT("La Gilda di Valdorso");
		I.bAbitanti = false;
		// la scadenza la mette chi chiama (con l'ora del server): qui i giorni in secondi
		I.Scadenza = static_cast<int64>(Giorni) * 86400;
		Out = I;
		return true;
	}

	bool Metti(FValdorsoBachecheSalvate& Bacheche, FValdorsoIncarico Incarico, int64 Adesso)
	{
		if (Bacheche.Incarichi.Num() >= MassimoIncarichi || Incarico.Scadenza <= Adesso)
		{
			return false;
		}
		Incarico.Numero = Bacheche.ProssimoNumero++;
		Incarico.Presi = 0;
		Incarico.Compiuti = 0;
		Bacheche.Incarichi.Add(MoveTemp(Incarico));
		return true;
	}

	const FValdorsoProclama& Proclama(FValdorsoBachecheSalvate& Bacheche, const FString& Testo, const FString& Autore, bool bLegge, int64 Adesso)
	{
		FValdorsoProclama P;
		P.Numero = Bacheche.ProssimoProclama++;
		P.Testo = Testo.TrimStartAndEnd().Left(LettereProclama);
		P.Autore = Autore;
		P.Scritto = Adesso;
		P.bLegge = bLegge;
		P.ValeDal = bLegge ? Adesso + SecondiLegge : Adesso;
		while (Bacheche.Proclami.Num() >= MassimoProclami)
		{
			Bacheche.Proclami.RemoveAt(0);
		}
		return Bacheche.Proclami.Add_GetRef(MoveTemp(P));
	}

	TArray<FString> RigheProclami(const FValdorsoBachecheSalvate& Bacheche, int64 Adesso)
	{
		TArray<FString> Righe;
		for (int32 i = Bacheche.Proclami.Num() - 1; i >= 0; --i)
		{
			const FValdorsoProclama& P = Bacheche.Proclami[i];
			FString Riga = FString::Printf(TEXT("%s - Per ordine del Balivo: %s"), *GildaData(P.Scritto), *P.Testo);
			if (P.bLegge)
			{
				Riga += P.ValeDal > Adesso ? FString::Printf(TEXT(" (Legge: vale dal %s.)"), *GildaData(P.ValeDal)) : FString(TEXT(" (Legge della valle.)"));
			}
			Righe.Add(Riga);
		}
		return Righe;
	}

	void Sistema(FValdorsoBachecheSalvate& Bacheche)
	{
		int32 Massimo = 0;
		for (FValdorsoIncarico& I : Bacheche.Incarichi)
		{
			Massimo = FMath::Max(Massimo, I.Numero);
			I.Quanti = FMath::Clamp(I.Quanti, 1, 50);
			I.Posti = FMath::Clamp(I.Posti, 1, 20);
			I.Compiuti = FMath::Clamp(I.Compiuti, 0, I.Posti);
			I.Presi = FMath::Clamp(I.Presi, I.Compiuti, I.Posti);
			I.Monete = FMath::Clamp(I.Monete, 0, 5000);
		}
		Bacheche.ProssimoNumero = FMath::Max(Bacheche.ProssimoNumero, Massimo + 1);
		int32 MassimoP = 0;
		for (const FValdorsoProclama& P : Bacheche.Proclami)
		{
			MassimoP = FMath::Max(MassimoP, P.Numero);
		}
		Bacheche.ProssimoProclama = FMath::Max(Bacheche.ProssimoProclama, MassimoP + 1);
		while (Bacheche.Proclami.Num() > MassimoProclami)
		{
			Bacheche.Proclami.RemoveAt(0);
		}
		Bacheche.Cassa = FMath::Clamp(Bacheche.Cassa, 0, CassaMassima);
	}

	void Sistema(FValdorsoGildaColono& Colono)
	{
		Colono.Punti = FMath::Max(0, Colono.Punti);
		Colono.Grado = static_cast<EValdorsoGrado>(FMath::Clamp(static_cast<int32>(Colono.Grado), 0, 3));
		// i file di prima: chi aveva gia' il grado dai punti se lo tiene
		if (Colono.VersioneGradi < 1)
		{
			Colono.Grado = FMath::Max(Colono.Grado, GradoDi(Colono.Punti));
			Colono.VersioneGradi = 1;
		}
		Colono.AvvisatoPronto = static_cast<EValdorsoGrado>(FMath::Clamp(static_cast<int32>(Colono.AvvisatoPronto), static_cast<int32>(Colono.Grado), 3));
		TSet<int32> Visti;
		for (int32 i = Colono.Presi.Num() - 1; i >= 0; --i)
		{
			const int32 N = Colono.Presi[i].Foglio.Numero;
			if (N <= 0 || Visti.Contains(N))
			{
				Colono.Presi.RemoveAt(i);
				continue;
			}
			Visti.Add(N);
		}
		while (Colono.Presi.Num() > MassimoPresi)
		{
			Colono.Presi.RemoveAt(0);
		}
	}
}

#undef LOCTEXT_NAMESPACE
