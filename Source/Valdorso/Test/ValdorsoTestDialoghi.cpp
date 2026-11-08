// Valdorso - Test automatici dei dialoghi con i PNG (07/11): Gaspare e le guardie del passo.
// Scritto da Claude il 07/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd), sotto "Valdorso.Dialoghi".

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoDialoghi.h"

namespace ValdorsoTestDialoghi
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

// Ogni dialogo scritto è sano: i nodi d'inizio e del ritorno ci sono, ogni risposta porta a un nodo che esiste, ci si
// arriva a ogni nodo, nessun vicolo cieco (tranne le fini volute), da ogni nodo si arriva a una fine, al massimo 4
// risposte, battute non vuote e non troppo lunghe, righe di rune che si leggono.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestDialoghiSani, "Valdorso.Dialoghi.Sani", ValdorsoTestDialoghi::Bandiere)
bool FValdorsoTestDialoghiSani::RunTest(const FString& Parameters)
{
	const TArray<FValdorsoDialogo>& Tutti = ValdorsoDialoghi::Tutti();
	TestTrue(TEXT("ci sono dei dialoghi"), Tutti.Num() >= 3);
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
	for (const TCHAR* Atteso : { TEXT("Gaspare"), TEXT("GuardiaCorrado"), TEXT("GuardiaOttone") })
	{
		TestNotNull(FString::Printf(TEXT("c'è il dialogo %s"), Atteso), ValdorsoDialoghi::Trova(FName(Atteso)));
	}
	TestNull(TEXT("un dialogo che non c'è"), ValdorsoDialoghi::Trova(FName(TEXT("NessunoQui"))));
	TestNull(TEXT("nessun nome"), ValdorsoDialoghi::Trova(NAME_None));
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

#endif
