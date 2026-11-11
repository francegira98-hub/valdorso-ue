// Valdorso - Crescere: le abilita' (vedi ValdorsoAbilitaDati.h). Scritto da Claude il 10/11/2026.

#include "ValdorsoAbilitaDati.h"
#include "ValdorsoRegistro.h"

#define LOCTEXT_NAMESPACE "ValdorsoCrescere"

namespace
{
	FValdorsoAbilitaTipo CrescereTipo(const TCHAR* Id, EValdorsoFamigliaAbilita Famiglia, std::initializer_list<const TCHAR*> Nomi,
		const TCHAR* Descrizione, std::initializer_list<const TCHAR*> Sblocchi, bool bInArrivo = false)
	{
		FValdorsoAbilitaTipo T;
		T.Id = FName(Id);
		T.Famiglia = Famiglia;
		int32 i = 0;
		for (const TCHAR* N : Nomi)
		{
			if (i < 5)
			{
				T.Nomi[i++] = N;
			}
		}
		// i nomi che mancano restano quello di prima
		for (; i < 5; ++i)
		{
			T.Nomi[i] = i > 0 ? T.Nomi[i - 1] : Id;
		}
		T.Descrizione = Descrizione;
		int32 s = 0;
		for (const TCHAR* Sb : Sblocchi)
		{
			if (s < 4)
			{
				T.Sblocchi[s++] = Sb;
			}
		}
		T.bInArrivo = bInArrivo;
		return T;
	}

	TArray<FValdorsoAbilitaTipo> CrescereCatalogo()
	{
		using F = EValdorsoFamigliaAbilita;
		TArray<FValdorsoAbilitaTipo> C;
		// --- il corpo
		C.Add(CrescereTipo(TEXT("Atletica"), F::Corpo,
			{ TEXT("Atletica"), TEXT("Passo lungo"), TEXT("Gambe di lupo"), TEXT("Vento nelle gambe"), TEXT("Il passo dell'Orso") },
			TEXT("Correre, saltare, resistere alla fatica. Sale correndo: di piu' quando sei stanco o inseguito."),
			{ TEXT("La capriola d'atterraggio: cadendo rotoli e ti fai meno male"), TEXT("Lo scatto lungo: correndo il fiato cala un quarto piu' piano"),
			  TEXT("Il secondo fiato: la stamina torna un terzo piu' in fretta"), TEXT("La corsa senza fine: il fiato cala la meta'") }));
		C.Add(CrescereTipo(TEXT("Nuoto"), F::Corpo,
			{ TEXT("Nuoto"), TEXT("Bracciata sicura"), TEXT("Pesce di fiume"), TEXT("Figlio del lago"), TEXT("Il respiro dell'abisso") },
			TEXT("Stare a galla e andare sotto. Sale nuotando, di piu' sott'acqua."),
			{ TEXT("La bracciata leggera: il nuoto veloce costa meno"), TEXT("Il fiato lungo: sott'acqua dura la meta' in piu'"), TEXT("Il nuoto veloce: piu' forte"), TEXT("Il respiro dell'abisso: il fiato dura il doppio") }));
		C.Add(CrescereTipo(TEXT("Tempra"), F::Corpo,
			{ TEXT("Tempra"), TEXT("Pelle dura"), TEXT("Ossa di quercia"), TEXT("Roccia"), TEXT("L'inamovibile") },
			TEXT("Reggere i colpi. Sale prendendone (e restando in piedi)."),
			{ TEXT("I piedi piantati: si perde meno equilibrio"), TEXT("La pelle dura: i morsi fanno meno male"), TEXT("Ti rialzi prima"), TEXT("Il colpo che non ti piega") }));
		// --- le armi (ognuna sale con le armi della sua famiglia: AbilitaDellArma)
		C.Add(CrescereTipo(TEXT("Lotta"), F::Armi,
			{ TEXT("Lotta"), TEXT("Pugno svelto"), TEXT("Rissaiolo"), TEXT("Pugile"), TEXT("Mani di ferro") },
			TEXT("A mani nude. Sale colpendo con i pugni."),
			{ TEXT("La spallata (tecnica)"), TEXT("La presa: tieni fermo chi colpisci (tecnica)"), TEXT("Il colpo che stordisce (tecnica)"), TEXT("Le mani di ferro: i pugni come una mazza") }));
		C.Add(CrescereTipo(TEXT("Pugnali"), F::Armi,
			{ TEXT("Pugnali"), TEXT("Lama svelta"), TEXT("Coltello nel buio"), TEXT("Lama d'ombra"), TEXT("Il morso del serpente") },
			TEXT("Coltelli, pugnali, falcetti: corti e veloci."),
			{ TEXT("Il colpo doppio (tecnica)"), TEXT("La lama alle spalle: di piu' colpendo da dietro"), TEXT("Il taglio che sanguina (tecnica)"), TEXT("Il morso del serpente (tecnica)") }));
		C.Add(CrescereTipo(TEXT("Spade"), F::Armi,
			{ TEXT("Spade"), TEXT("Prima guardia"), TEXT("Spadaccino"), TEXT("Maestro di guardie"), TEXT("La lama della valle") },
			TEXT("Spade corte e lunghe, il gladio. Le guardie della scherma italiana (G) contano di piu' con la spada."),
			{ TEXT("L'affondo (tecnica)"), TEXT("La parata perfetta (tecnica)"), TEXT("Il maestro di guardie: le guardie contano una volta e mezzo"), TEXT("La lama della valle (tecnica)") }));
		C.Add(CrescereTipo(TEXT("Asce"), F::Armi,
			{ TEXT("Asce"), TEXT("Colpo pesante"), TEXT("Spaccalegna"), TEXT("Ascia di guerra"), TEXT("La scure del passo") },
			TEXT("Accette e asce da guerra: lente, ma fanno male."),
			{ TEXT("Il colpo che spacca gli scudi (tecnica)"), TEXT("Il giro: colpisce chi hai intorno (tecnica)"), TEXT("La furia (tecnica)"), TEXT("La scure del passo (tecnica)") }));
		C.Add(CrescereTipo(TEXT("Mazze"), F::Armi,
			{ TEXT("Mazze"), TEXT("Botta sicura"), TEXT("Spezzaossa"), TEXT("Martello di guerra"), TEXT("Il tuono") },
			TEXT("Martelli, mazze e clave: tolgono l'equilibrio."),
			{ TEXT("Il colpo che butta a terra (tecnica)"), TEXT("Contro le armature: gli scudi fermano la meta'"), TEXT("Lo stordimento (tecnica)"), TEXT("Il tuono (tecnica)") }));
		C.Add(CrescereTipo(TEXT("Aste"), F::Armi,
			{ TEXT("Armi in asta"), TEXT("Punta lontana"), TEXT("Muro di lance"), TEXT("Lancia del confine"), TEXT("La lunga ombra") },
			TEXT("Lance, forconi, bastoni: tengono lontano chi attacca."),
			{ TEXT("Il colpo lungo (tecnica)"), TEXT("Fermare la carica: di piu' a chi ti corre addosso"), TEXT("Il giro del bastone (tecnica)"), TEXT("La lunga ombra (tecnica)") }));
		C.Add(CrescereTipo(TEXT("Scudo"), F::Armi,
			{ TEXT("Scudo"), TEXT("Braccio fermo"), TEXT("Muro di legno"), TEXT("Scudo della legione"), TEXT("La rocca") },
			TEXT("Parare con lo scudo. Sale fermando i colpi che arrivano davanti."),
			{ TEXT("La spinta con lo scudo (tecnica)"), TEXT("La parata che toglie l'equilibrio"), TEXT("Il muro: si para anche di lato"), TEXT("La rocca (tecnica)") }));
		// --- il mondo
		C.Add(CrescereTipo(TEXT("Caccia"), F::Mondo,
			{ TEXT("Caccia"), TEXT("Occhio del cacciatore"), TEXT("Sulla pista del lupo"), TEXT("Cacciatore della valle"), TEXT("Il signore del bosco") },
			TEXT("Cacciare e scuoiare. Sale scuoiando le prede."),
			{ TEXT("Le pelli migliori: una pelle in piu'"), TEXT("Le tracce degli animali (con la v0.2)"), TEXT("Il passo del cacciatore: gli animali ti sentono da piu' vicino"), TEXT("Il signore del bosco: i lupi non ti attaccano per primi") }));
		C.Add(CrescereTipo(TEXT("Furtivita"), F::Mondo,
			{ TEXT("Furtivita'"), TEXT("Passo leggero"), TEXT("Ombra tra le ombre"), TEXT("Nessuno ti vede"), TEXT("Il fantasma") },
			TEXT("Muoversi senza farsi vedere. Arriva con la v0.1.5."),
			{ TEXT("La capriola d'atterraggio"), TEXT("Il passo che non si sente"), TEXT("Sparire nella folla"), TEXT("Il fantasma") }, true));
		C.Add(CrescereTipo(TEXT("Pesca"), F::Mondo,
			{ TEXT("Pesca"), TEXT("Lenza paziente"), TEXT("Pescatore"), TEXT("Signore del lago"), TEXT("Chi parla ai pesci") },
			TEXT("Pescare nel lago e nei fiumi. Arriva con la v0.2."), {}, true));
		// --- i mestieri (arrivano con la v0.2: per ora ci sono i 5 punti del vecchio mestiere)
		C.Add(CrescereTipo(TEXT("Coltivare"), F::Mestieri, { TEXT("Coltivare"), TEXT("Mani nella terra"), TEXT("Contadino"), TEXT("Maestro degli orti"), TEXT("Il raccolto d'oro") },
			TEXT("Gli orti e i campi. Arriva con la v0.2."), {}, true));
		C.Add(CrescereTipo(TEXT("Allevare"), F::Mestieri, { TEXT("Allevare"), TEXT("Pastore"), TEXT("Amico degli animali"), TEXT("Allevatore"), TEXT("Chi chiama le bestie") },
			TEXT("Pecore, capre, cavalli. Arriva con la v0.2."), {}, true));
		C.Add(CrescereTipo(TEXT("Bottega"), F::Mestieri, { TEXT("Lavoro di bottega"), TEXT("Apprendista"), TEXT("Artigiano"), TEXT("Mastro"), TEXT("Mani d'oro") },
			TEXT("Banchi da lavoro, attrezzi, oggetti con qualita'. Arriva con la v0.2."), {}, true));
		C.Add(CrescereTipo(TEXT("Commercio"), F::Mestieri, { TEXT("Commercio"), TEXT("Occhio per l'affare"), TEXT("Mercante"), TEXT("Mercante di fiera"), TEXT("Il principe del mercato") },
			TEXT("Comprare, vendere, mercanteggiare. Arriva con la v0.2."), {}, true));
		C.Add(CrescereTipo(TEXT("Eloquenza"), F::Mestieri, { TEXT("Eloquenza"), TEXT("Buone maniere"), TEXT("Lingua d'argento"), TEXT("Voce di corte"), TEXT("Chi convince i re") },
			TEXT("Parlare con i nobili, convincere, ingannare. Arriva con le voci e i legami."), {}, true));
		// --- lo spirito
		C.Add(CrescereTipo(TEXT("Studio"), F::Spirito, { TEXT("Studio"), TEXT("Lettore"), TEXT("Studioso"), TEXT("Sapiente"), TEXT("Custode dei tomi") },
			TEXT("Leggere, copiare, capire le rune. Arriva con le biblioteche e la magia."), {}, true));
		C.Add(CrescereTipo(TEXT("Preghiera"), F::Spirito, { TEXT("Preghiera"), TEXT("Devoto"), TEXT("Voce del tempio"), TEXT("Servo del Cuore"), TEXT("Il santo") },
			TEXT("Pregare e il favore della propria divinita'. Arriva con le fedi."), {}, true));
		C.Add(CrescereTipo(TEXT("Concentrazione"), F::Spirito, { TEXT("Concentrazione"), TEXT("Mente ferma"), TEXT("Volonta' che risponde"), TEXT("Il silenzio dentro"), TEXT("Chi piega il mondo") },
			TEXT("La volonta' che regge gli incantesimi. Arriva con la magia (v0.3)."), {}, true));
		return C;
	}
}

namespace ValdorsoCrescere
{
	const TArray<FValdorsoAbilitaTipo>& Tutte()
	{
		static const TArray<FValdorsoAbilitaTipo> Catalogo = CrescereCatalogo();
		return Catalogo;
	}

	const FValdorsoAbilitaTipo* Trova(FName Id)
	{
		return Tutte().FindByPredicate([Id](const FValdorsoAbilitaTipo& T) { return T.Id == Id; });
	}

	FString Nome(FName Id, int32 Evoluzioni)
	{
		const FValdorsoAbilitaTipo* T = Trova(Id);
		return T ? FString(T->Nomi[FMath::Clamp(Evoluzioni, 0, 4)]) : FString();
	}

	FText NomeFamiglia(EValdorsoFamigliaAbilita Famiglia)
	{
		switch (Famiglia)
		{
		case EValdorsoFamigliaAbilita::Corpo: return LOCTEXT("Corpo", "Il corpo");
		case EValdorsoFamigliaAbilita::Armi: return LOCTEXT("Armi", "Le armi");
		case EValdorsoFamigliaAbilita::Mondo: return LOCTEXT("Mondo", "Il mondo");
		case EValdorsoFamigliaAbilita::Mestieri: return LOCTEXT("Mestieri", "I mestieri");
		default: return LOCTEXT("Spirito", "Lo spirito");
		}
	}

	FName AbilitaDellArma(FName IdOggetto)
	{
		struct FArmaAbilita
		{
			const TCHAR* Arma;
			const TCHAR* Abilita;
		};
		static const FArmaAbilita Armi[] = {
			{ TEXT("Coltello"), TEXT("Pugnali") }, { TEXT("Pugnale"), TEXT("Pugnali") }, { TEXT("Falcetto"), TEXT("Pugnali") },
			{ TEXT("Spada_Corta"), TEXT("Spade") }, { TEXT("Spada_Lunga"), TEXT("Spade") }, { TEXT("Gladio"), TEXT("Spade") },
			{ TEXT("Accetta"), TEXT("Asce") }, { TEXT("Ascia_Guerra"), TEXT("Asce") },
			{ TEXT("Martello"), TEXT("Mazze") }, { TEXT("Mazza"), TEXT("Mazze") }, { TEXT("Clava"), TEXT("Mazze") },
			{ TEXT("Lancia"), TEXT("Aste") }, { TEXT("Forcone"), TEXT("Aste") }, { TEXT("Bastone_Pastore"), TEXT("Aste") },
			{ TEXT("Bastone_Ferrato"), TEXT("Aste") },
			// (11/11, v0.1.7) il piccone e il pugnale dei nani della grotta del nord
			{ TEXT("Piccone"), TEXT("Mazze") }, { TEXT("Pugnale_Nani"), TEXT("Pugnali") }, { TEXT("Martello_Nani"), TEXT("Mazze") },
		};
		if (!IdOggetto.IsNone())
		{
			for (const FArmaAbilita& A : Armi)
			{
				if (IdOggetto == FName(A.Arma))
				{
					return FName(A.Abilita);
				}
			}
		}
		return FName(TEXT("Lotta"));
	}

	FName AbilitaDelMestiere(const FString& Mestiere)
	{
		struct FMestiereAbilita
		{
			const TCHAR* Mestiere;
			const TCHAR* Abilita;
		};
		// come i vantaggi del Registro (ValdorsoRegistro.cpp, VantaggiMestiere)
		static const FMestiereAbilita Mestieri[] = {
			{ TEXT("contadino"), TEXT("Coltivare") }, { TEXT("pastore"), TEXT("Allevare") }, { TEXT("apprendista"), TEXT("Bottega") },
			{ TEXT("soldato"), TEXT("Pugnali") }, { TEXT("servo"), TEXT("Eloquenza") }, { TEXT("allievo"), TEXT("Studio") },
			{ TEXT("orfano"), TEXT("Preghiera") }, { TEXT("mercante"), TEXT("Commercio") }, { TEXT("cacciatore"), TEXT("Caccia") },
			{ TEXT("marinaio"), TEXT("Pesca") },
		};
		for (const FMestiereAbilita& M : Mestieri)
		{
			if (Mestiere.Equals(M.Mestiere, ESearchCase::IgnoreCase))
			{
				return FName(M.Abilita);
			}
		}
		return NAME_None;
	}

	const FValdorsoAbilitaValore* Valore(const FValdorsoAbilitaColono& Colono, FName Id)
	{
		return Colono.Valori.FindByPredicate([Id](const FValdorsoAbilitaValore& V) { return V.Id == Id; });
	}

	FValdorsoAbilitaValore& Prendi(FValdorsoAbilitaColono& Colono, FName Id)
	{
		if (FValdorsoAbilitaValore* Gia = Colono.Valori.FindByPredicate([Id](const FValdorsoAbilitaValore& V) { return V.Id == Id; }))
		{
			return *Gia;
		}
		FValdorsoAbilitaValore& Nuovo = Colono.Valori.AddDefaulted_GetRef();
		Nuovo.Id = Id;
		return Nuovo;
	}

	float Punti(const FValdorsoAbilitaColono& Colono, FName Id)
	{
		const FValdorsoAbilitaValore* V = Valore(Colono, Id);
		return V ? V->Punti : 0.f;
	}

	float Totale(const FValdorsoAbilitaColono& Colono)
	{
		float Somma = 0.f;
		for (const FValdorsoAbilitaValore& V : Colono.Valori)
		{
			Somma += V.Punti;
		}
		return Somma;
	}

	float Soglia(int32 Evoluzioni)
	{
		return Evoluzioni >= 0 && Evoluzioni < 4 ? Passo * static_cast<float>(Evoluzioni + 1) : 0.f;
	}

	float Guadagno(float PuntiAdesso, float Difficolta, int32 Ripetizioni)
	{
		const float Rischio = FMath::Clamp(Difficolta, 0.f, 2.f);
		const float Alto = FMath::Clamp(PuntiAdesso / Massimo, 0.f, 1.f);
		// piu' sei bravo, piu' sali piano (a 0: tutto; a 100: un decimo)
		const float Curva = FMath::Lerp(1.f, 0.1f, FMath::Pow(Alto, 0.8f));
		// la stessa cosa di fila (lo stesso manichino) rende sempre meno
		const float Noia = 1.f / (1.f + 0.35f * static_cast<float>(FMath::Max(0, Ripetizioni)));
		return GuadagnoBase * Rischio * Curva * Noia;
	}

	FValdorsoEsitoUso Usa(FValdorsoAbilitaColono& Colono, FName Id, float Difficolta, int32 Ripetizioni, bool bMomentoCritico)
	{
		FValdorsoEsitoUso Risultato;
		const FValdorsoAbilitaTipo* Tipo = Trova(Id);
		if (!Tipo || Tipo->bInArrivo)
		{
			return Risultato;
		}
		// (prima di prendere il riferimento: Prendi puo' aggiungere un posto all'elenco)
		Prendi(Colono, Id);
		const float Somma = Totale(Colono);
		FValdorsoAbilitaValore& V = Prendi(Colono, Id);
		if (V.Verso != EValdorsoVerso::Sale || V.Evoluzioni >= 4)
		{
			return Risultato;
		}
		const float Prossima = Soglia(V.Evoluzioni);
		// ferma alla soglia: si carica, e si evolve solo nel momento critico
		if (V.Punti >= Prossima - KINDA_SMALL_NUMBER)
		{
			V.Punti = Prossima;
			const bool bEraPronta = V.Carica >= 1.f;
			const float CaricaPrima = V.Carica;
			V.Carica = FMath::Min(1.f, V.Carica + Guadagno(V.Punti, Difficolta, Ripetizioni) * CaricaPerPunto);
			Risultato.bCaricata = V.Carica > CaricaPrima;
			if (V.Carica >= 1.f && !bEraPronta)
			{
				Risultato.bPronta = true;
			}
			if (V.Carica >= 1.f && bMomentoCritico)
			{
				++V.Evoluzioni;
				V.Carica = 0.f;
				Risultato.bEvoluta = true;
				Risultato.Evoluzione = V.Evoluzioni;
			}
			return Risultato;
		}
		float Su = FMath::Min(Guadagno(V.Punti, Difficolta, Ripetizioni), Prossima - V.Punti);
		// il tetto: oltre, cala una di quelle messe a calare (la piu' alta)
		const float Spazio = FMath::Max(0.f, Tetto - Somma);
		if (Su > Spazio)
		{
			float Manca = Su - Spazio;
			FValdorsoAbilitaValore* Giu = nullptr;
			for (FValdorsoAbilitaValore& Altra : Colono.Valori)
			{
				if (Altra.Id != Id && Altra.Verso == EValdorsoVerso::Scende && Altra.Punti > 0.f && (!Giu || Altra.Punti > Giu->Punti))
				{
					Giu = &Altra;
				}
			}
			if (Giu)
			{
				const float Tolto = FMath::Min(Manca, Giu->Punti);
				Giu->Punti -= Tolto;
				Giu->Evoluzioni = FMath::Min(Giu->Evoluzioni, FMath::FloorToInt32(Giu->Punti / Passo + KINDA_SMALL_NUMBER));
				Giu->Carica = 0.f;
				Risultato.Calata = Giu->Id;
				Manca -= Tolto;
			}
			Su -= Manca;
			if (Su <= 0.f)
			{
				Risultato.bTettoPieno = true;
				return Risultato;
			}
		}
		const float Prima = V.Punti;
		V.Punti = FMath::Min(V.Punti + Su, Prossima);
		Risultato.Guadagno = V.Punti - Prima;
		Risultato.bPuntoIntero = FMath::FloorToInt32(V.Punti) > FMath::FloorToInt32(Prima);
		return Risultato;
	}

	bool ImpostaVerso(FValdorsoAbilitaColono& Colono, FName Id, EValdorsoVerso Verso)
	{
		if (!Trova(Id))
		{
			return false;
		}
		Prendi(Colono, Id).Verso = Verso;
		return true;
	}

	bool ImpostaPunti(FValdorsoAbilitaColono& Colono, FName Id, float Nuovi)
	{
		if (!Trova(Id))
		{
			return false;
		}
		FValdorsoAbilitaValore& V = Prendi(Colono, Id);
		V.Punti = FMath::Clamp(Nuovi, 0.f, Massimo);
		V.Evoluzioni = FMath::Clamp(FMath::FloorToInt32(V.Punti / Passo + KINDA_SMALL_NUMBER), 0, 4);
		V.Carica = 0.f;
		return true;
	}

	bool DaiVantaggi(FValdorsoAbilitaColono& Colono, const FValdorsoRegistro& Registro)
	{
		if (Colono.bVantaggiDati)
		{
			return false;
		}
		Colono.bVantaggiDati = true;
		bool bDato = false;
		auto Aggiungi = [&Colono, &bDato](FName Id)
		{
			if (!Id.IsNone() && Trova(Id))
			{
				FValdorsoAbilitaValore& V = Prendi(Colono, Id);
				V.Punti = FMath::Min(V.Punti + PuntiVantaggio, Soglia(V.Evoluzioni) > 0.f ? Soglia(V.Evoluzioni) : Massimo);
				bDato = true;
			}
		};
		Aggiungi(AbilitaDelMestiere(Registro.Mestiere));
		if (Registro.RichiamoMagia == TEXT("si"))
		{
			Aggiungi(FName(TEXT("Concentrazione")));
		}
		return bDato;
	}

	void Sistema(FValdorsoAbilitaColono& Colono)
	{
		TSet<FName> Viste;
		// (in avanti: delle doppie resta la prima)
		for (int32 i = 0; i < Colono.Valori.Num(); ++i)
		{
			FValdorsoAbilitaValore& V = Colono.Valori[i];
			if (!Trova(V.Id) || Viste.Contains(V.Id))
			{
				Colono.Valori.RemoveAt(i--);
				continue;
			}
			Viste.Add(V.Id);
			V.Punti = FMath::Clamp(V.Punti, 0.f, Massimo);
			V.Evoluzioni = FMath::Clamp(V.Evoluzioni, 0, FMath::Min(4, FMath::FloorToInt32(V.Punti / Passo + KINDA_SMALL_NUMBER)));
			// non si salta una soglia: si resta ferma li' a caricarsi
			if (V.Evoluzioni < 4)
			{
				V.Punti = FMath::Min(V.Punti, Soglia(V.Evoluzioni));
			}
			V.Carica = FMath::Clamp(V.Carica, 0.f, 1.f);
			if (static_cast<uint8>(V.Verso) > static_cast<uint8>(EValdorsoVerso::Scende))
			{
				V.Verso = EValdorsoVerso::Sale;
			}
		}
	}

	FText Annuncio(FName Id, int32 Evoluzione)
	{
		return FText::Format(LOCTEXT("Annuncio", "Il Cuore batte, e risponde: la tua {0} diventa {1}."),
			FText::FromString(Nome(Id, Evoluzione - 1)), FText::FromString(Nome(Id, Evoluzione)));
	}

	FString ProssimoSblocco(FName Id, int32 Evoluzioni)
	{
		const FValdorsoAbilitaTipo* T = Trova(Id);
		return T && Evoluzioni >= 0 && Evoluzioni < 4 ? FString(T->Sblocchi[Evoluzioni]) : FString();
	}

	float Forza(const FValdorsoAbilitaColono& Colono)
	{
		float Arma = 0.f;
		for (const FValdorsoAbilitaValore& V : Colono.Valori)
		{
			const FValdorsoAbilitaTipo* T = Trova(V.Id);
			if (T && T->Famiglia == EValdorsoFamigliaAbilita::Armi && V.Id != FName(TEXT("Scudo")))
			{
				Arma = FMath::Max(Arma, V.Punti);
			}
		}
		return FMath::Clamp(Arma * 0.7f + Punti(Colono, FName(TEXT("Tempra"))) * 0.3f, 0.f, Massimo);
	}

	float ForzaSpecie(FName Specie)
	{
		struct FForzaNome
		{
			const TCHAR* Nome;
			float Forza;
		};
		static const FForzaNome Bestie[] = {
			{ TEXT("Lepre"), 0.f }, { TEXT("Cervo"), 5.f }, { TEXT("Volpe"), 8.f }, { TEXT("Lupo"), 25.f },
			{ TEXT("Cinghiale"), 30.f }, { TEXT("Orso"), 60.f },
		};
		for (const FForzaNome& F : Bestie)
		{
			if (Specie == FName(F.Nome))
			{
				return F.Forza;
			}
		}
		return 20.f;
	}

	float ForzaAbitante(FName Dialogo)
	{
		struct FForzaNome
		{
			const TCHAR* Nome;
			float Forza;
		};
		// le guardie del passo sono molto forti (non si battono: il Design), Lucia era avventuriera di rango B
		static const FForzaNome Abitanti[] = {
			{ TEXT("GuardiaCorrado"), 90.f }, { TEXT("GuardiaOttone"), 90.f }, { TEXT("CapoGilda"), 70.f }, { TEXT("Lucia"), 60.f },
			{ TEXT("Bruno"), 35.f }, { TEXT("Gaspare"), 15.f }, { TEXT("NonnaEdda"), 20.f }, { TEXT("PadreAnselmo"), 10.f },
		};
		for (const FForzaNome& F : Abitanti)
		{
			if (Dialogo == FName(F.Nome))
			{
				return F.Forza;
			}
		}
		return 15.f;
	}

	FText Confronto(float Mia, float Sua, FLinearColor& OutColore)
	{
		const float D = Sua - Mia;
		if (D > 30.f)
		{
			OutColore = FLinearColor(0.55f, 0.02f, 0.02f);
			return LOCTEXT("ConfrontoPezzi", "Ti farebbe a pezzi");
		}
		if (D > 12.f)
		{
			OutColore = FLinearColor(0.85f, 0.12f, 0.08f);
			return LOCTEXT("ConfrontoMolto", "Molto piu' forte di te");
		}
		if (D > 4.f)
		{
			OutColore = FLinearColor(0.95f, 0.5f, 0.1f);
			return LOCTEXT("ConfrontoForte", "Piu' forte di te");
		}
		if (D >= -4.f)
		{
			OutColore = FLinearColor(0.95f, 0.85f, 0.3f);
			return LOCTEXT("ConfrontoPari", "Alla tua pari");
		}
		if (D >= -12.f)
		{
			OutColore = FLinearColor(0.45f, 0.8f, 0.3f);
			return LOCTEXT("ConfrontoDebole", "Piu' debole di te");
		}
		OutColore = FLinearColor(0.6f, 0.6f, 0.6f);
		return LOCTEXT("ConfrontoNiente", "Non e' un pericolo per te");
	}

	float DifficoltaContro(float Mia, float Sua)
	{
		return FMath::Clamp(1.f + (Sua - Mia) / 40.f, 0.3f, 2.f);
	}

	bool PericoloVero(float Mia, float Sua)
	{
		return Sua - Mia > 12.f;  // come "Molto piu' forte di te"
	}
}

#undef LOCTEXT_NAMESPACE
