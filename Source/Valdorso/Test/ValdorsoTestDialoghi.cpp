// Valdorso - Test automatici dei dialoghi con i PNG (07/11): Gaspare e le guardie del passo; (08/11) Nonna Edda, Bruno e
// Padre Anselmo, e la memoria dei dialoghi nel file del personaggio.
// Scritto da Claude il 07/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd), sotto "Valdorso.Dialoghi".

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoDialoghi.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoArchivista.h"
#include "JsonObjectConverter.h"

namespace ValdorsoTestDialoghi
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** (08/11) Tutte le battute di un dialogo in una riga sola, in minuscolo (per cercarci le parole). */
	FString TuttoIlTesto(const FValdorsoDialogo& Dialogo)
	{
		FString Fuori;
		for (const FValdorsoNodoDialogo& Nodo : Dialogo.Nodi)
		{
			Fuori += FString::Join(Nodo.Righe, TEXT(" ")) + TEXT(" ");
		}
		return Fuori.ToLower();
	}

	/** (08/11) Il numero (tra le visibili) della prima risposta che contiene quel testo; INDEX_NONE se non c'è. */
	int32 RispostaCon(const FValdorsoBattuta& Battuta, const FString& Pezzo)
	{
		return Battuta.Scelte.IndexOfByPredicate([&Pezzo](const FString& Testo) { return Testo.Contains(Pezzo); });
	}

	/**
	 * (08/11) Una memoria dei dialoghi come quella dei personaggi veri: dentro un taccuino, cioè nel file del
	 * personaggio (il componente del PlayerController fa lo stesso, e in più salva).
	 */
	class FMemoriaNelTaccuino final : public FValdorsoMemoriaDialoghi
	{
	public:
		FValdorsoTaccuino Dati;

		virtual bool GiaParlato(FName IdDialogo) const override { return ValdorsoTaccuino::HaParlato(Dati, IdDialogo); }
		virtual void SegnaParlato(FName IdDialogo) override { ValdorsoTaccuino::SegnaParlato(Dati, IdDialogo); }
		virtual bool HaSegno(FName IdDialogo, FName Segno) const override { return ValdorsoTaccuino::HaSegno(Dati, IdDialogo, Segno); }
		virtual void MettiSegno(FName IdDialogo, FName Segno) override { ValdorsoTaccuino::MettiSegno(Dati, IdDialogo, Segno); }
	};

	/**
	 * (08/11) Sceglie (come il server) la risposta visibile che contiene Pezzo nel nodo Nodo, con la memoria: dà i segni
	 * della risposta e del nodo dopo. Vero se c'era; OutProssimo è il nodo dopo.
	 */
	bool Scegli(const FValdorsoDialogo& Dialogo, FName Nodo, const FString& Pezzo, FValdorsoMemoriaDialoghi& Memoria, FName& OutProssimo)
	{
		OutProssimo = NAME_None;
		const FValdorsoNodoDialogo* Attuale = Dialogo.Trova(Nodo);
		const int32 Visibile = RispostaCon(ValdorsoDialoghi::Battuta(Dialogo, Nodo, 1, Memoria), Pezzo);
		const TArray<int32> Visibili = ValdorsoDialoghi::ScelteVisibili(Dialogo, Nodo, Memoria);
		FName Evento;
		if (!Attuale || !Visibili.IsValidIndex(Visibile) || !ValdorsoDialoghi::Avanza(Dialogo, Nodo, Visibili[Visibile], OutProssimo, Evento))
		{
			return false;
		}
		Memoria.MettiSegno(Dialogo.Id, Attuale->Scelte[Visibili[Visibile]].Segna);
		if (const FValdorsoNodoDialogo* Dopo = Dialogo.Trova(OutProssimo))
		{
			Memoria.MettiSegno(Dialogo.Id, Dopo->Segna);
		}
		return true;
	}
}

// Ogni dialogo scritto è sano: i nodi d'inizio e del ritorno ci sono, ogni risposta porta a un nodo che esiste, ci si
// arriva a ogni nodo, nessun vicolo cieco (tranne le fini volute), da ogni nodo si arriva a una fine, al massimo 4
// risposte, battute non vuote e non troppo lunghe, righe di rune che si leggono.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestDialoghiSani, "Valdorso.Dialoghi.Sani", ValdorsoTestDialoghi::Bandiere)
bool FValdorsoTestDialoghiSani::RunTest(const FString& Parameters)
{
	const TArray<FValdorsoDialogo>& Tutti = ValdorsoDialoghi::Tutti();
	TestTrue(TEXT("ci sono dei dialoghi"), Tutti.Num() >= 6);
	TSet<FName> Id;
	for (const FValdorsoDialogo& Dialogo : Tutti)
	{
		TestFalse(FString::Printf(TEXT("%s: Id doppio"), *Dialogo.Id.ToString()), Id.Contains(Dialogo.Id));
		Id.Add(Dialogo.Id);
		for (const FString& Problema : ValdorsoDialoghi::Problemi(Dialogo))
		{
			AddError(Problema);
		}
		// Ogni nodo ha almeno una via d'uscita: o è una fine voluta, o ha delle risposte (controllato anche sopra, qui
		// lo si dice in chiaro).
		for (const FValdorsoNodoDialogo& Nodo : Dialogo.Nodi)
		{
			TestTrue(FString::Printf(TEXT("%s/%s: una fine o delle risposte"), *Dialogo.Id.ToString(), *Nodo.Id.ToString()),
				Nodo.bFine != (Nodo.Scelte.Num() > 0));
		}
		TestNotNull(FString::Printf(TEXT("%s si trova per nome"), *Dialogo.Id.ToString()), ValdorsoDialoghi::Trova(Dialogo.Id));
	}
	for (const TCHAR* Atteso : { TEXT("Gaspare"), TEXT("GuardiaCorrado"), TEXT("GuardiaOttone"), TEXT("NonnaEdda"), TEXT("Bruno"),
		TEXT("PadreAnselmo"), TEXT("Lucia"), TEXT("CapoGilda"), TEXT("Marta"), TEXT("Livia"), TEXT("Tobia"), TEXT("Teodoro"),
		TEXT("Agnese"), TEXT("Elric") })
	{
		TestNotNull(FString::Printf(TEXT("c'è il dialogo %s"), Atteso), ValdorsoDialoghi::Trova(FName(Atteso)));
	}
	TestNull(TEXT("un dialogo che non c'è"), ValdorsoDialoghi::Trova(FName(TEXT("NessunoQui"))));
	TestNull(TEXT("nessun nome"), ValdorsoDialoghi::Trova(NAME_None));
	// (08/11) Le guardie hanno nomi loro: Corrado è il Balivo, Ottone il Gran Inquisitore delle Cronache (gli Id restano).
	const FValdorsoDialogo* Veterano = ValdorsoDialoghi::Trova(TEXT("GuardiaCorrado"));
	const FValdorsoDialogo* Giovane = ValdorsoDialoghi::Trova(TEXT("GuardiaOttone"));
	if (Veterano && Giovane)
	{
		TestEqual(TEXT("la guardia veterana è Tebaldo"), Veterano->NomeBreve, FString(TEXT("Tebaldo")));
		TestEqual(TEXT("la guardia giovane è Fulvio"), Giovane->NomeBreve, FString(TEXT("Fulvio")));
	}
	return true;
}

// Il controllo dei problemi trova davvero i problemi (un dialogo storto apposta).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestDialoghiControllo, "Valdorso.Dialoghi.Controllo", ValdorsoTestDialoghi::Bandiere)
bool FValdorsoTestDialoghiControllo::RunTest(const FString& Parameters)
{
	FValdorsoDialogo Storto;
	Storto.Id = TEXT("Storto");
	Storto.NomeBreve = TEXT("Storto");
	Storto.Titolo = TEXT("Il dialogo storto");
	Storto.Inizio = TEXT("a");
	Storto.Nodo(TEXT("a"), { TEXT("Ciao.") })
		.Scelta(TEXT("Verso il nulla"), TEXT("manca"))
		.Scelta(TEXT("Verso b"), TEXT("b"));
	Storto.Nodo(TEXT("b"), { TEXT("Vicolo cieco.") });                     // né risposte né fine
	Storto.Nodo(TEXT("c"), { TEXT("Non ci arriva nessuno.") }).Fine();
	Storto.Nodo(TEXT("d"), { TEXT("Giro chiuso.") }).Scelta(TEXT("ancora"), TEXT("d"));
	const FString Tutto = FString::Join(ValdorsoDialoghi::Problemi(Storto), TEXT("\n"));
	TestTrue(TEXT("trova la risposta verso un nodo che non c'è"), Tutto.Contains(TEXT("'manca', che non c'è")));
	TestTrue(TEXT("trova il vicolo cieco"), Tutto.Contains(TEXT("Storto/b: vicolo cieco")));
	TestTrue(TEXT("trova il nodo irraggiungibile"), Tutto.Contains(TEXT("Storto/c: non ci si arriva mai")));
	TestTrue(TEXT("trova il giro chiuso"), Tutto.Contains(TEXT("Storto/d: da qui non si arriva mai a una fine")));

	FValdorsoDialogo Troppe;
	Troppe.Id = TEXT("Troppe");
	Troppe.NomeBreve = TEXT("Troppe");
	Troppe.Titolo = TEXT("Troppe risposte");
	Troppe.Inizio = TEXT("a");
	Troppe.Nodo(TEXT("a"), { TEXT("rune: Pippo") })
		.Scelta(TEXT("1"), NAME_None).Scelta(TEXT("2"), NAME_None).Scelta(TEXT("3"), NAME_None)
		.Scelta(TEXT("4"), NAME_None).Scelta(TEXT("5"), NAME_None);
	const FString Tutto2 = FString::Join(ValdorsoDialoghi::Problemi(Troppe), TEXT("\n"));
	TestTrue(TEXT("trova le risposte di troppo"), Tutto2.Contains(TEXT("5 risposte")));
	TestTrue(TEXT("trova la riga di rune sbagliata"), Tutto2.Contains(TEXT("la riga di rune non si legge")));

	// (08/11) Un nodo le cui risposte si possono nascondere tutte: con i segni resterebbe senza risposte.
	FValdorsoDialogo Nascosto;
	Nascosto.Id = TEXT("Nascosto");
	Nascosto.NomeBreve = TEXT("Nascosto");
	Nascosto.Titolo = TEXT("Tutto nascosto");
	Nascosto.Inizio = TEXT("a");
	Nascosto.Nodo(TEXT("a"), { TEXT("Ciao.") })
		.Scelta(TEXT("Una volta sola"), NAME_None).SceltaSegna(TEXT("Fatto")).SceltaNascostaSe(TEXT("Fatto"));
	const FString Tutto3 = FString::Join(ValdorsoDialoghi::Problemi(Nascosto), TEXT("\n"));
	TestTrue(TEXT("trova il nodo che resta senza risposte"), Tutto3.Contains(TEXT("Nascosto/a: con le risposte nascoste resta senza risposte")));
	TestTrue(TEXT("e che da lì, con i segni, non si esce"), Tutto3.Contains(TEXT("Nascosto/a: da qui non si arriva mai a una fine")));
	return true;
}

// Il saluto cambia dalla seconda volta, per personaggio e per PNG; la memoria non confonde i nomi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestDialoghiSaluto, "Valdorso.Dialoghi.Saluto", ValdorsoTestDialoghi::Bandiere)
bool FValdorsoTestDialoghiSaluto::RunTest(const FString& Parameters)
{
	const FValdorsoDialogo* Gaspare = ValdorsoDialoghi::Trova(TEXT("Gaspare"));
	if (!TestNotNull(TEXT("c'è Gaspare"), Gaspare))
	{
		return false;
	}
	ValdorsoDialoghi::Dimentica();
	TestFalse(TEXT("all'inizio nessuno ha parlato con Gaspare"), ValdorsoDialoghi::GiaParlato(Gaspare->Id, TEXT("Livia della Rocca")));
	TestTrue(TEXT("la prima volta: il primo saluto"), ValdorsoDialoghi::NodoIniziale(*Gaspare, false) == Gaspare->Inizio);
	ValdorsoDialoghi::SegnaParlato(Gaspare->Id, TEXT("Livia della Rocca"));
	TestTrue(TEXT("poi se lo ricorda"), ValdorsoDialoghi::GiaParlato(Gaspare->Id, TEXT("Livia della Rocca")));
	TestTrue(TEXT("anche con maiuscole e spazi diversi"), ValdorsoDialoghi::GiaParlato(Gaspare->Id, TEXT(" livia della rocca ")));
	TestFalse(TEXT("un altro personaggio no"), ValdorsoDialoghi::GiaParlato(Gaspare->Id, TEXT("Aldo")));
	TestFalse(TEXT("un altro PNG no"), ValdorsoDialoghi::GiaParlato(TEXT("GuardiaCorrado"), TEXT("Livia della Rocca")));
	TestTrue(TEXT("la seconda volta: il saluto del ritorno"), ValdorsoDialoghi::NodoIniziale(*Gaspare, true) == Gaspare->Ritorno);
	TestTrue(TEXT("i due saluti sono diversi"), Gaspare->Inizio != Gaspare->Ritorno);
	const FValdorsoNodoDialogo* Primo = Gaspare->Trova(Gaspare->Inizio);
	const FValdorsoNodoDialogo* Ritorno = Gaspare->Trova(Gaspare->Ritorno);
	if (TestNotNull(TEXT("il primo saluto c'è"), Primo) && TestNotNull(TEXT("il ritorno c'è"), Ritorno))
	{
		TestTrue(TEXT("e dicono cose diverse"), FString::Join(Primo->Righe, TEXT(" ")) != FString::Join(Ritorno->Righe, TEXT(" ")));
	}
	ValdorsoDialoghi::Dimentica();
	TestFalse(TEXT("Dimentica pulisce la memoria"), ValdorsoDialoghi::GiaParlato(Gaspare->Id, TEXT("Livia della Rocca")));

	// Un dialogo senza ritorno usa sempre l'inizio.
	FValdorsoDialogo Semplice;
	Semplice.Id = TEXT("Semplice");
	Semplice.Inizio = TEXT("a");
	Semplice.Nodo(TEXT("a"), { TEXT("Salve.") }).Fine();
	TestTrue(TEXT("senza ritorno: sempre l'inizio"), ValdorsoDialoghi::NodoIniziale(Semplice, true) == FName(TEXT("a")));
	return true;
}

// Avanzare: le risposte valide portano al nodo giusto, quelle sbagliate non valgono; la battuta per il PC è pronta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestDialoghiAvanza, "Valdorso.Dialoghi.Avanza", ValdorsoTestDialoghi::Bandiere)
bool FValdorsoTestDialoghiAvanza::RunTest(const FString& Parameters)
{
	const FValdorsoDialogo* Gaspare = ValdorsoDialoghi::Trova(TEXT("Gaspare"));
	if (!TestNotNull(TEXT("c'è Gaspare"), Gaspare))
	{
		return false;
	}
	const FValdorsoNodoDialogo* Primo = Gaspare->Trova(Gaspare->Inizio);
	if (!TestNotNull(TEXT("il primo nodo"), Primo))
	{
		return false;
	}
	FName Prossimo;
	FName Evento;
	for (int32 i = 0; i < Primo->Scelte.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("la risposta %d vale"), i + 1), ValdorsoDialoghi::Avanza(*Gaspare, Gaspare->Inizio, i, Prossimo, Evento));
		TestTrue(FString::Printf(TEXT("la risposta %d porta dove deve"), i + 1), Prossimo == Primo->Scelte[i].Verso);
	}
	TestFalse(TEXT("una risposta che non c'è"), ValdorsoDialoghi::Avanza(*Gaspare, Gaspare->Inizio, Primo->Scelte.Num(), Prossimo, Evento));
	TestFalse(TEXT("una risposta negativa"), ValdorsoDialoghi::Avanza(*Gaspare, Gaspare->Inizio, -1, Prossimo, Evento));
	TestFalse(TEXT("un nodo che non c'è"), ValdorsoDialoghi::Avanza(*Gaspare, TEXT("nessuno"), 0, Prossimo, Evento));

	const FValdorsoBattuta Battuta = ValdorsoDialoghi::Battuta(*Gaspare, Gaspare->Inizio, 7);
	TestEqual(TEXT("la battuta ha il titolo di Gaspare"), Battuta.Chi, Gaspare->Titolo);
	TestEqual(TEXT("le stesse righe"), Battuta.Righe.Num(), Primo->Righe.Num());
	TestEqual(TEXT("le stesse risposte"), Battuta.Scelte.Num(), Primo->Scelte.Num());
	TestEqual(TEXT("il numero della battuta"), Battuta.Passo, 7);
	TestFalse(TEXT("non è la fine"), Battuta.bFine);
	TestFalse(TEXT("non si chiude"), Battuta.bChiudi);
	TestTrue(TEXT("un nodo che non c'è chiude"), ValdorsoDialoghi::Battuta(*Gaspare, TEXT("nessuno"), 1).bChiudi);
	const FValdorsoBattuta Congedo = ValdorsoDialoghi::Battuta(*Gaspare, TEXT("congedo"), 2);
	TestTrue(TEXT("il congedo è una fine"), Congedo.bFine && Congedo.Scelte.Num() == 0);

	// Gaspare non svela il segreto (l'Orso gli salvò la vita): lo fa solo intuire.
	for (const FValdorsoNodoDialogo& Nodo : Gaspare->Nodi)
	{
		const FString Tutto = FString::Join(Nodo.Righe, TEXT(" ")).ToLower();
		TestFalse(FString::Printf(TEXT("Gaspare/%s non dice il segreto"), *Nodo.Id.ToString()),
			Tutto.Contains(TEXT("salvò la vita")) || Tutto.Contains(TEXT("salvato la vita")) || Tutto.Contains(TEXT("trent'anni")));
	}
	return true;
}

// Le guardie del passo non si comprano: c'è la risposta "offri una moneta", è annotata come Corruzione, e da lì in
// avanti nessuna strada apre il passo (né il sigillo né la porta): solo rifiuti e congedi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestDialoghiGuardie, "Valdorso.Dialoghi.Guardie", ValdorsoTestDialoghi::Bandiere)
bool FValdorsoTestDialoghiGuardie::RunTest(const FString& Parameters)
{
	for (const TCHAR* Nome : { TEXT("GuardiaCorrado"), TEXT("GuardiaOttone") })
	{
		const FValdorsoDialogo* Guardia = ValdorsoDialoghi::Trova(FName(Nome));
		if (!TestNotNull(FString::Printf(TEXT("c'è %s"), Nome), Guardia))
		{
			continue;
		}
		const FValdorsoSceltaDialogo* Moneta = nullptr;
		for (const FValdorsoNodoDialogo& Nodo : Guardia->Nodi)
		{
			for (const FValdorsoSceltaDialogo& Risposta : Nodo.Scelte)
			{
				if (!Moneta && Risposta.Evento == FName(TEXT("Corruzione")))
				{
					Moneta = &Risposta;
				}
			}
		}
		if (!TestNotNull(FString::Printf(TEXT("%s: si può offrire una moneta"), Nome), Moneta))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: la risposta parla di una moneta"), Nome), Moneta->Testo.Contains(TEXT("moneta")));
		const FValdorsoNodoDialogo* Rifiuto = Guardia->Trova(Moneta->Verso);
		if (!TestNotNull(FString::Printf(TEXT("%s: c'è il rifiuto"), Nome), Rifiuto))
		{
			continue;
		}
		// Dal rifiuto in avanti: nessuna battuta che lasci passare.
		TSet<FName> Visti;
		TArray<FName> DaVedere = { Rifiuto->Id };
		while (DaVedere.Num() > 0)
		{
			const FName Id = DaVedere.Pop();
			if (Visti.Contains(Id))
			{
				continue;
			}
			Visti.Add(Id);
			const FValdorsoNodoDialogo* Nodo = Guardia->Trova(Id);
			if (!Nodo)
			{
				continue;
			}
			const FString Tutto = FString::Join(Nodo->Righe, TEXT(" ")).ToLower();
			TestFalse(FString::Printf(TEXT("%s/%s non lascia passare"), Nome, *Id.ToString()),
				Tutto.Contains(TEXT("passa pure")) || Tutto.Contains(TEXT("puoi passare")) || Tutto.Contains(TEXT("ti apro")));
			for (const FValdorsoSceltaDialogo& Risposta : Nodo->Scelte)
			{
				if (!Risposta.Verso.IsNone())
				{
					DaVedere.Add(Risposta.Verso);
				}
			}
		}
		// Il primo saluto dice che il passo è chiuso (o lo dice appena si chiede di passare).
		bool bChiuso = false;
		for (const FValdorsoNodoDialogo& Nodo : Guardia->Nodi)
		{
			const FString Tutto = FString::Join(Nodo.Righe, TEXT(" ")).ToLower();
			bChiuso = bChiuso || Tutto.Contains(TEXT("nessuno passa il sigillo"));
		}
		TestTrue(FString::Printf(TEXT("%s: nessuno passa il sigillo"), Nome), bChiuso);
	}
	return true;
}

// (08/11) La moneta si offre una volta sola: dopo l'offerta (il segno "MonetaOfferta", per personaggio e per guardia)
// la risposta "(Offri una moneta)" non c'è più tra le risposte di quella guardia; le altre restano, e i numeri che il
// PC rimanda si riportano alle risposte vere. E ogni dialogo resta sano con tutti i segni: ogni nodo ha ancora almeno
// una risposta visibile, o è una fine.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestDialoghiMoneta, "Valdorso.Dialoghi.MonetaUnaVolta", ValdorsoTestDialoghi::Bandiere)
bool FValdorsoTestDialoghiMoneta::RunTest(const FString& Parameters)
{
	const FString Offri = TEXT("(Offri una moneta)");
	const FString Colono = TEXT("Prova3");
	const FString Altro = TEXT("Prova2");
	auto Offre = [&Offri](const FValdorsoBattuta& Battuta)
	{
		return Battuta.Scelte.ContainsByPredicate([&Offri](const FString& Testo) { return Testo.StartsWith(Offri); });
	};

	ValdorsoDialoghi::Dimentica();
	for (const TCHAR* Nome : { TEXT("GuardiaCorrado"), TEXT("GuardiaOttone") })
	{
		const FValdorsoDialogo* Guardia = ValdorsoDialoghi::Trova(FName(Nome));
		if (!TestNotNull(FString::Printf(TEXT("c'è %s"), Nome), Guardia))
		{
			continue;
		}
		// I nodi dove si offre la moneta, e il segno che l'offerta lascia.
		TArray<FName> NodiMoneta;
		FName Segno;
		for (const FValdorsoNodoDialogo& Nodo : Guardia->Nodi)
		{
			for (const FValdorsoSceltaDialogo& Risposta : Nodo.Scelte)
			{
				if (Risposta.Testo.StartsWith(Offri))
				{
					NodiMoneta.AddUnique(Nodo.Id);
					TestFalse(FString::Printf(TEXT("%s/%s: l'offerta lascia un segno"), Nome, *Nodo.Id.ToString()), Risposta.Segna.IsNone());
					TestFalse(FString::Printf(TEXT("%s/%s: l'offerta si nasconde"), Nome, *Nodo.Id.ToString()), Risposta.NascostaSe.IsNone());
					TestTrue(FString::Printf(TEXT("%s/%s: si nasconde con il suo stesso segno"), Nome, *Nodo.Id.ToString()),
						Risposta.NascostaSe == Risposta.Segna);
					Segno = Risposta.Segna;
				}
			}
		}
		if (!TestTrue(FString::Printf(TEXT("%s: da qualche parte si offre la moneta"), Nome), NodiMoneta.Num() > 0 && !Segno.IsNone()))
		{
			continue;
		}

		// Prima: chi non ha offerto niente vede l'offerta.
		for (const FName& IdNodo : NodiMoneta)
		{
			TestTrue(FString::Printf(TEXT("%s/%s: prima si può offrire"), Nome, *IdNodo.ToString()),
				Offre(ValdorsoDialoghi::Battuta(*Guardia, IdNodo, 1, Colono)));
		}

		// L'offerta: si sceglie davvero (come fa il server, dal numero visibile al numero vero) e si dà il segno.
		const FName Primo = NodiMoneta[0];
		const FValdorsoNodoDialogo* NodoPrimo = Guardia->Trova(Primo);
		if (!TestNotNull(FString::Printf(TEXT("%s: il nodo dell'offerta"), Nome), NodoPrimo))
		{
			continue;
		}
		const TArray<int32> Prima = ValdorsoDialoghi::ScelteVisibili(*Guardia, Primo, Colono);
		const int32 Visibile = Prima.IndexOfByPredicate([NodoPrimo, &Offri](int32 Vera) { return NodoPrimo->Scelte[Vera].Testo.StartsWith(Offri); });
		if (!TestTrue(FString::Printf(TEXT("%s: l'offerta è tra le visibili"), Nome), Visibile != INDEX_NONE))
		{
			continue;
		}
		const int32 VeraMoneta = Prima[Visibile];
		FName Prossimo;
		FName Evento;
		TestTrue(FString::Printf(TEXT("%s: l'offerta vale"), Nome), ValdorsoDialoghi::Avanza(*Guardia, Primo, VeraMoneta, Prossimo, Evento));
		TestTrue(FString::Printf(TEXT("%s: è una corruzione"), Nome), Evento == FName(TEXT("Corruzione")));
		ValdorsoDialoghi::MettiSegno(Guardia->Id, Colono, NodoPrimo->Scelte[VeraMoneta].Segna);
		TestTrue(FString::Printf(TEXT("%s: il segno resta"), Nome), ValdorsoDialoghi::HaSegno(Guardia->Id, Colono, Segno));
		TestTrue(FString::Printf(TEXT("%s: anche con maiuscole e spazi diversi"), Nome), ValdorsoDialoghi::HaSegno(Guardia->Id, TEXT(" prova3 "), Segno));
		TestFalse(FString::Printf(TEXT("%s: un altro personaggio no"), Nome), ValdorsoDialoghi::HaSegno(Guardia->Id, Altro, Segno));

		// Dopo: in nessun nodo di quella guardia (primo saluto, ritorno, "passare"...) l'offerta c'è ancora.
		for (const FValdorsoNodoDialogo& Nodo : Guardia->Nodi)
		{
			const FValdorsoBattuta Battuta = ValdorsoDialoghi::Battuta(*Guardia, Nodo.Id, 2, Colono);
			TestFalse(FString::Printf(TEXT("%s/%s: dopo l'offerta la moneta non si offre più"), Nome, *Nodo.Id.ToString()), Offre(Battuta));
			TestTrue(FString::Printf(TEXT("%s/%s: resta una risposta o è una fine"), Nome, *Nodo.Id.ToString()),
				Battuta.bFine || Battuta.Scelte.Num() > 0);
		}
		// Le risposte che restano sono le stesse di prima, meno l'offerta; e i numeri visibili portano dove devono.
		const TArray<int32> Dopo = ValdorsoDialoghi::ScelteVisibili(*Guardia, Primo, Colono);
		TestTrue(FString::Printf(TEXT("%s: una risposta in meno"), Nome), Dopo.Num() == Prima.Num() - 1);
		TestFalse(FString::Printf(TEXT("%s: senza l'offerta"), Nome), Dopo.Contains(VeraMoneta));
		const FValdorsoBattuta Senza = ValdorsoDialoghi::Battuta(*Guardia, Primo, 3, Colono);
		if (TestTrue(FString::Printf(TEXT("%s: la battuta ha le risposte visibili"), Nome), Senza.Scelte.Num() == Dopo.Num()))
		{
			for (int32 i = 0; i < Dopo.Num(); ++i)
			{
				TestTrue(FString::Printf(TEXT("%s: la risposta visibile %d è quella vera"), Nome, i + 1), Senza.Scelte[i] == NodoPrimo->Scelte[Dopo[i]].Testo);
			}
		}
		// Un altro personaggio la vede ancora; e senza nome (i test di prima) si vedono tutte.
		TestTrue(FString::Printf(TEXT("%s: un altro personaggio può ancora offrire"), Nome), Offre(ValdorsoDialoghi::Battuta(*Guardia, Primo, 4, Altro)));
		TestTrue(FString::Printf(TEXT("%s: senza nome si vedono tutte"), Nome), Offre(ValdorsoDialoghi::Battuta(*Guardia, Primo, 5)));
	}

	// Il segno è di quella guardia: offrire a Corrado non nasconde l'offerta a Ottone (lo si controlla da capo).
	ValdorsoDialoghi::Dimentica();
	const FValdorsoDialogo* Ottone = ValdorsoDialoghi::Trova(TEXT("GuardiaOttone"));
	const FValdorsoDialogo* Corrado = ValdorsoDialoghi::Trova(TEXT("GuardiaCorrado"));
	if (Ottone && Corrado)
	{
		TestTrue(TEXT("Ottone: dopo Dimentica si può offrire"), Offre(ValdorsoDialoghi::Battuta(*Ottone, Ottone->Ritorno, 1, Colono)));
		ValdorsoDialoghi::MettiSegno(Corrado->Id, Colono, TEXT("MonetaOfferta"));
		TestTrue(TEXT("Ottone: la moneta offerta a Corrado non conta"), Offre(ValdorsoDialoghi::Battuta(*Ottone, Ottone->Ritorno, 2, Colono)));
		TestFalse(TEXT("Corrado: invece sì"), Offre(ValdorsoDialoghi::Battuta(*Corrado, TEXT("passare"), 3, Colono)));
	}

	// Ogni dialogo con tutti i segni insieme: ogni nodo ha ancora almeno una risposta visibile, o è una fine.
	ValdorsoDialoghi::Dimentica();
	const FString ConTutti = TEXT("Tutti i segni");
	for (const FValdorsoDialogo& Dialogo : ValdorsoDialoghi::Tutti())
	{
		for (const FValdorsoNodoDialogo& Nodo : Dialogo.Nodi)
		{
			ValdorsoDialoghi::MettiSegno(Dialogo.Id, ConTutti, Nodo.Segna);
			for (const FValdorsoSceltaDialogo& Risposta : Nodo.Scelte)
			{
				ValdorsoDialoghi::MettiSegno(Dialogo.Id, ConTutti, Risposta.Segna);
				ValdorsoDialoghi::MettiSegno(Dialogo.Id, ConTutti, Risposta.NascostaSe);
			}
		}
		for (const FValdorsoNodoDialogo& Nodo : Dialogo.Nodi)
		{
			const FString Qui = FString::Printf(TEXT("%s/%s"), *Dialogo.Id.ToString(), *Nodo.Id.ToString());
			const FValdorsoBattuta Battuta = ValdorsoDialoghi::Battuta(Dialogo, Nodo.Id, 1, ConTutti);
			TestFalse(FString::Printf(TEXT("%s: con tutti i segni non si chiude"), *Qui), Battuta.bChiudi);
			TestTrue(FString::Printf(TEXT("%s: con tutti i segni, una fine o almeno una risposta"), *Qui),
				Nodo.bFine ? (Battuta.bFine && Battuta.Scelte.Num() == 0) : (!Battuta.bFine && Battuta.Scelte.Num() > 0));
			TestTrue(FString::Printf(TEXT("%s: le risposte visibili sono quelle della battuta"), *Qui),
				ValdorsoDialoghi::ScelteVisibili(Dialogo, Nodo.Id, ConTutti).Num() == Battuta.Scelte.Num());
		}
	}
	ValdorsoDialoghi::Dimentica();
	TestFalse(TEXT("Dimentica pulisce anche i segni"), ValdorsoDialoghi::HaSegno(TEXT("GuardiaCorrado"), Colono, TEXT("MonetaOfferta")));
	return true;
}

// (08/11) I tre PNG nuovi: ognuno ha un primo saluto e un ritorno diversi (come tutti i dialoghi), e dice le cose che
// deve dire. Nonna Edda: le rune Nir-Ren e Nir-Ren-Ka, il dovuto (Niente / Solo rispetto), il nome della ragazza mai
// detto, la domanda sul fondo del lago una volta sola. Bruno: la prima scodella la offre la casa (una volta), le quattro
// voci girano, Livia e il Balivo. Padre Anselmo: il battito, il territorio, tutte le fedi, l'anno e un giorno.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestDialoghiNuoviPng, "Valdorso.Dialoghi.NuoviPNG", ValdorsoTestDialoghi::Bandiere)
bool FValdorsoTestDialoghiNuoviPng::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestDialoghi;

	for (const FValdorsoDialogo& Dialogo : ValdorsoDialoghi::Tutti())
	{
		const FValdorsoNodoDialogo* Primo = Dialogo.Trova(Dialogo.Inizio);
		const FValdorsoNodoDialogo* Ritorno = Dialogo.Trova(Dialogo.Ritorno);
		if (TestNotNull(FString::Printf(TEXT("%s: il primo saluto"), *Dialogo.Id.ToString()), Primo)
			&& TestNotNull(FString::Printf(TEXT("%s: il ritorno"), *Dialogo.Id.ToString()), Ritorno))
		{
			TestTrue(FString::Printf(TEXT("%s: il ritorno saluta in un altro modo"), *Dialogo.Id.ToString()),
				FString::Join(Primo->Righe, TEXT(" ")) != FString::Join(Ritorno->Righe, TEXT(" ")));
		}
	}

	const FValdorsoDialogo* Edda = ValdorsoDialoghi::Trova(TEXT("NonnaEdda"));
	const FValdorsoDialogo* Bruno = ValdorsoDialoghi::Trova(TEXT("Bruno"));
	const FValdorsoDialogo* Anselmo = ValdorsoDialoghi::Trova(TEXT("PadreAnselmo"));
	if (!TestNotNull(TEXT("c'è Nonna Edda"), Edda) || !TestNotNull(TEXT("c'è Bruno"), Bruno) || !TestNotNull(TEXT("c'è Padre Anselmo"), Anselmo))
	{
		return false;
	}

	// --- Nonna Edda
	const FString TestoEdda = TuttoIlTesto(*Edda);
	TestTrue(TEXT("Edda canta Nir-Ren-Ka, Acqua che veglia"), TestoEdda.Contains(TEXT("rune: nir-ren-ka")) && TestoEdda.Contains(TEXT("acqua che veglia")));
	TestTrue(TEXT("Edda sa la Guarigione lieve"), TestoEdda.Contains(TEXT("rune: nir-ren |")) && TestoEdda.Contains(TEXT("guarigione lieve")));
	TestTrue(TEXT("Edda chiede il dovuto"), TestoEdda.Contains(TEXT("porta il dovuto")));
	TestFalse(TEXT("Edda non dice il nome della ragazza"), TestoEdda.Contains(TEXT("iole")));
	const FValdorsoBattuta PrimaEdda = ValdorsoDialoghi::Battuta(*Edda, Edda->Inizio, 1);
	TestTrue(TEXT("Edda: si può non portare niente"), RispostaCon(PrimaEdda, TEXT("Niente")) != INDEX_NONE);
	TestTrue(TEXT("Edda: o solo rispetto"), RispostaCon(PrimaEdda, TEXT("Solo rispetto")) != INDEX_NONE);
	{
		FMemoriaNelTaccuino Memoria;
		const FString Fondo = TEXT("fondo del lago");
		TestTrue(TEXT("Edda: prima si può chiedere del lago"), RispostaCon(ValdorsoDialoghi::Battuta(*Edda, TEXT("altro"), 1, Memoria), Fondo) != INDEX_NONE);
		FName Dopo;
		TestTrue(TEXT("Edda: la domanda sul lago vale"), Scegli(*Edda, TEXT("altro"), Fondo, Memoria, Dopo));
		const FValdorsoNodoDialogo* Risposta = Edda->Trova(Dopo);
		TestTrue(TEXT("Edda: sul lago congeda"), Risposta && Risposta->bFine);
		TestFalse(TEXT("Edda: poi non si chiede più"), RispostaCon(ValdorsoDialoghi::Battuta(*Edda, TEXT("altro"), 2, Memoria), Fondo) != INDEX_NONE);
	}

	// --- Bruno
	const FString TestoBruno = TuttoIlTesto(*Bruno);
	TestTrue(TEXT("Bruno: la prima la offre la casa"), TestoBruno.Contains(TEXT("la prima la offre la casa")));
	TestTrue(TEXT("Bruno: il frammento ha saltato un battito"), TestoBruno.Contains(TEXT("saltato un battito")));
	TestTrue(TEXT("Bruno: Livia Serra"), TestoBruno.Contains(TEXT("livia serra")));
	TestTrue(TEXT("Bruno: il Balivo Corrado Valli"), TestoBruno.Contains(TEXT("corrado valli")));
	{
		FMemoriaNelTaccuino Memoria;
		const FString Zuppa = TEXT("scodella");
		TestTrue(TEXT("Bruno: la prima volta c'è la scodella"), RispostaCon(ValdorsoDialoghi::Battuta(*Bruno, Bruno->Inizio, 1, Memoria), Zuppa) != INDEX_NONE);
		FName Dopo;
		TestTrue(TEXT("Bruno: la scodella si chiede"), Scegli(*Bruno, Bruno->Inizio, Zuppa, Memoria, Dopo));
		TestTrue(TEXT("Bruno: e arriva"), Dopo == FName(TEXT("scodella")));
		for (const TCHAR* Dove : { TEXT("primo"), TEXT("altro") })
		{
			TestFalse(FString::Printf(TEXT("Bruno/%s: la seconda scodella non è gratis"), Dove),
				RispostaCon(ValdorsoDialoghi::Battuta(*Bruno, FName(Dove), 2, Memoria), Zuppa) != INDEX_NONE);
		}
	}
	// Le voci girano: dalla prima, "Un'altra voce?" (la prima risposta) porta a quattro voci diverse e poi di nuovo alla prima.
	{
		TArray<FName> Voci;
		FName Voce = TEXT("voci");
		for (int32 i = 0; i < 4; ++i)
		{
			const FValdorsoNodoDialogo* Nodo = Bruno->Trova(Voce);
			if (!TestNotNull(FString::Printf(TEXT("Bruno: la voce %d"), i + 1), Nodo) || Nodo->Scelte.Num() == 0)
			{
				break;
			}
			TestFalse(FString::Printf(TEXT("Bruno: la voce %d è nuova"), i + 1), Voci.Contains(Voce));
			Voci.Add(Voce);
			Voce = Nodo->Scelte[0].Verso;
		}
		TestEqual(TEXT("Bruno: quattro voci"), Voci.Num(), 4);
		TestTrue(TEXT("Bruno: dopo l'ultima si ricomincia"), Voce == FName(TEXT("voci")));
	}

	// --- Padre Anselmo
	const FString TestoAnselmo = TuttoIlTesto(*Anselmo);
	for (const TCHAR* Fede : { TEXT("solara"), TEXT("ignar"), TEXT("nereia"), TEXT("torvald"), TEXT("zefira"), TEXT("vecchi dei") })
	{
		TestTrue(FString::Printf(TEXT("Anselmo parla di %s"), Fede), TestoAnselmo.Contains(Fede));
	}
	TestTrue(TEXT("Anselmo: due colpi e una pausa"), TestoAnselmo.Contains(TEXT("due colpi e una pausa")));
	TestTrue(TEXT("Anselmo: Val d'Orso, il primo territorio"), TestoAnselmo.Contains(TEXT("primo territorio")));
	TestTrue(TEXT("Anselmo: l'anno e un giorno"), TestoAnselmo.Contains(TEXT("anno e un giorno")));
	TestTrue(TEXT("Anselmo: Tobia e Nico"), TestoAnselmo.Contains(TEXT("tobia")) && TestoAnselmo.Contains(TEXT("nico")));
	return true;
}

// (08/11) La memoria dei dialoghi nel file del personaggio: chi ha già parlato con chi e i segni restano dopo lo
// spegnimento del server (si scrivono e si rileggono con il personaggio), i file di prima si leggono con la memoria
// vuota, i doppioni si uniscono; e attraverso la memoria del taccuino la moneta si offre una volta sola anche dopo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestDialoghiRicordo, "Valdorso.Dialoghi.Ricordo", ValdorsoTestDialoghi::Bandiere)
bool FValdorsoTestDialoghiRicordo::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestDialoghi;

	// Le regole.
	FValdorsoTaccuino Taccuino;
	TestFalse(TEXT("all'inizio non ha parlato con nessuno"), ValdorsoTaccuino::HaParlato(Taccuino, TEXT("Gaspare")));
	TestTrue(TEXT("la prima volta cambia"), ValdorsoTaccuino::SegnaParlato(Taccuino, TEXT("Gaspare")));
	TestFalse(TEXT("la seconda no"), ValdorsoTaccuino::SegnaParlato(Taccuino, TEXT("Gaspare")));
	TestTrue(TEXT("se lo ricorda"), ValdorsoTaccuino::HaParlato(Taccuino, TEXT("Gaspare")));
	TestFalse(TEXT("un altro PNG no"), ValdorsoTaccuino::HaParlato(Taccuino, TEXT("Bruno")));
	TestFalse(TEXT("nessun dialogo"), ValdorsoTaccuino::SegnaParlato(Taccuino, NAME_None));
	TestTrue(TEXT("un segno nuovo"), ValdorsoTaccuino::MettiSegno(Taccuino, TEXT("GuardiaCorrado"), TEXT("MonetaOfferta")));
	TestFalse(TEXT("lo stesso segno, anche con altre maiuscole"), ValdorsoTaccuino::MettiSegno(Taccuino, TEXT("GuardiaCorrado"), TEXT("monetaofferta")));
	TestTrue(TEXT("il segno c'è"), ValdorsoTaccuino::HaSegno(Taccuino, TEXT("GuardiaCorrado"), TEXT("MonetaOfferta")));
	TestFalse(TEXT("ma non con un'altra guardia"), ValdorsoTaccuino::HaSegno(Taccuino, TEXT("GuardiaOttone"), TEXT("MonetaOfferta")));
	TestFalse(TEXT("un segno vuoto non c'è mai"), ValdorsoTaccuino::MettiSegno(Taccuino, TEXT("GuardiaCorrado"), NAME_None));
	TestFalse(TEXT("il segno non vuol dire averci parlato"), ValdorsoTaccuino::HaParlato(Taccuino, TEXT("GuardiaCorrado")));
	for (int32 i = 0; i < ValdorsoTaccuino::MassimoSegni + 5; ++i)
	{
		ValdorsoTaccuino::MettiSegno(Taccuino, TEXT("Pieno"), FName(*FString::Printf(TEXT("Segno%d"), i)));
	}
	const FValdorsoRicordoDialogo* Pieno = Taccuino.Dialoghi.FindByPredicate([](const FValdorsoRicordoDialogo& R) { return R.Dialogo == TEXT("Pieno"); });
	TestTrue(TEXT("i segni hanno un massimo"), Pieno && Pieno->Segni.Num() == ValdorsoTaccuino::MassimoSegni);

	// Nel file del personaggio, e di nuovo fuori (come dopo lo spegnimento del server).
	FValdorsoPersonaggio Dati;
	Dati.Id = TEXT("prova");
	Dati.Nome = TEXT("Tessa");
	Dati.Taccuino = Taccuino;
	FString Testo;
	TestTrue(TEXT("si scrive"), FJsonObjectConverter::UStructToJsonObjectString(Dati, Testo));
	FValdorsoPersonaggio Riletto;
	TestTrue(TEXT("si rilegge"), FJsonObjectConverter::JsonObjectStringToUStruct(Testo, &Riletto, 0, 0));
	ValdorsoTaccuino::Sistema(Riletto.Taccuino);
	TestTrue(TEXT("dopo: ha parlato con Gaspare"), ValdorsoTaccuino::HaParlato(Riletto.Taccuino, TEXT("Gaspare")));
	TestTrue(TEXT("dopo: la moneta a Corrado c'è ancora"), ValdorsoTaccuino::HaSegno(Riletto.Taccuino, TEXT("GuardiaCorrado"), TEXT("MonetaOfferta")));
	TestFalse(TEXT("dopo: con Bruno ancora niente"), ValdorsoTaccuino::HaParlato(Riletto.Taccuino, TEXT("Bruno")));

	// Un personaggio salvato prima (con il taccuino ma senza la memoria dei dialoghi).
	const FString Vecchio = TEXT("{\"versioneSchema\":2,\"versione\":9,\"id\":\"vecchio\",\"accountId\":\"a1\",\"nome\":\"Bruno\",\"taccuino\":{\"luoghi\":[],\"iscrizioni\":[]}}");
	FValdorsoPersonaggio DiPrima;
	TestTrue(TEXT("il file di prima si legge"), FJsonObjectConverter::JsonObjectStringToUStruct(Vecchio, &DiPrima, 0, 0));
	TestEqual(TEXT("senza memoria dei dialoghi"), DiPrima.Taccuino.Dialoghi.Num(), 0);
	TestFalse(TEXT("e quindi il primo saluto"), ValdorsoTaccuino::HaParlato(DiPrima.Taccuino, TEXT("Gaspare")));

	// Un file ritoccato a mano: nomi vuoti via, doppioni uniti (con i segni di tutti e due).
	FValdorsoTaccuino Storto;
	Storto.Dialoghi.Reserve(3);  // i riferimenti qui sotto restano validi
	FValdorsoRicordoDialogo& Uno = Storto.Dialoghi.AddDefaulted_GetRef();
	Uno.Dialogo = TEXT("Bruno");
	Uno.Segni.Add(TEXT("PrimaScodella"));
	FValdorsoRicordoDialogo& Vuoto = Storto.Dialoghi.AddDefaulted_GetRef();
	Vuoto.bParlato = true;
	FValdorsoRicordoDialogo& Due = Storto.Dialoghi.AddDefaulted_GetRef();
	Due.Dialogo = TEXT(" bruno ");
	Due.bParlato = true;
	Due.Segni.Add(TEXT("primascodella"));
	Due.Segni.Add(TEXT("Altro"));
	Due.Segni.Add(FString());
	ValdorsoTaccuino::Sistema(Storto);
	if (TestEqual(TEXT("un ricordo solo"), Storto.Dialoghi.Num(), 1))
	{
		TestTrue(TEXT("ha parlato"), Storto.Dialoghi[0].bParlato);
		TestEqual(TEXT("due segni, senza doppioni"), Storto.Dialoghi[0].Segni.Num(), 2);
	}

	// Attraverso la memoria del taccuino (come il PNG con un personaggio vero): la moneta si offre una volta, anche
	// dopo il salvataggio; la memoria per nome del server non ne sa niente.
	ValdorsoDialoghi::Dimentica();
	const FValdorsoDialogo* Corrado = ValdorsoDialoghi::Trova(TEXT("GuardiaCorrado"));
	if (!TestNotNull(TEXT("c'è Corrado"), Corrado))
	{
		return false;
	}
	const FString Offri = TEXT("(Offri una moneta)");
	FMemoriaNelTaccuino Memoria;
	TestFalse(TEXT("la memoria nuova: primo saluto"), Memoria.GiaParlato(Corrado->Id));
	Memoria.SegnaParlato(Corrado->Id);
	TestTrue(TEXT("poi il ritorno"), ValdorsoDialoghi::NodoIniziale(*Corrado, Memoria.GiaParlato(Corrado->Id)) == Corrado->Ritorno);
	FName Dopo;
	TestTrue(TEXT("la moneta si offre"), Scegli(*Corrado, TEXT("passare"), Offri, Memoria, Dopo));
	TestFalse(TEXT("poi non più"), RispostaCon(ValdorsoDialoghi::Battuta(*Corrado, TEXT("passare"), 2, Memoria), Offri) != INDEX_NONE);
	TestFalse(TEXT("la memoria per nome non ne sa niente"), ValdorsoDialoghi::HaSegno(Corrado->Id, TEXT("Tessa"), TEXT("MonetaOfferta")));
	FValdorsoPersonaggio ConMemoria;
	ConMemoria.Id = TEXT("prova2");
	ConMemoria.Taccuino = Memoria.Dati;
	FString TestoMemoria;
	FValdorsoPersonaggio Riacceso;
	TestTrue(TEXT("la memoria si scrive"), FJsonObjectConverter::UStructToJsonObjectString(ConMemoria, TestoMemoria));
	TestTrue(TEXT("e si rilegge"), FJsonObjectConverter::JsonObjectStringToUStruct(TestoMemoria, &Riacceso, 0, 0));
	FMemoriaNelTaccuino DopoRiavvio;
	DopoRiavvio.Dati = Riacceso.Taccuino;
	ValdorsoTaccuino::Sistema(DopoRiavvio.Dati);
	TestTrue(TEXT("dopo il riavvio: il saluto del ritorno"), DopoRiavvio.GiaParlato(Corrado->Id));
	TestFalse(TEXT("dopo il riavvio: la moneta non si offre più"), RispostaCon(ValdorsoDialoghi::Battuta(*Corrado, TEXT("passare"), 3, DopoRiavvio), Offri) != INDEX_NONE);
	TestTrue(TEXT("con la memoria per nome invece sì"), RispostaCon(ValdorsoDialoghi::Battuta(*Corrado, TEXT("passare"), 4, TEXT("Tessa")), Offri) != INDEX_NONE);
	return true;
}

#endif
