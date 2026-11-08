// Valdorso - Test automatici della bussola e del taccuino di viaggio (07/11).
// Scritto da Claude il 07/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.Taccuino").

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoArchivista.h"
#include "JsonObjectConverter.h"

namespace ValdorsoTestTaccuino
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	TArray<FString> Righe(std::initializer_list<const TCHAR*> Testi)
	{
		TArray<FString> Fuori;
		for (const TCHAR* T : Testi)
		{
			Fuori.Add(T);
		}
		return Fuori;
	}
}

// La bussola: l'imbardata di Unreal diventa la rotta (X = est, Y = sud: il nord è -Y), le lettere giuste, lo scarto
// a destra e a sinistra e il punto sulla striscia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestBussola, "Valdorso.Taccuino.Bussola", ValdorsoTestTaccuino::Bandiere)
bool FValdorsoTestBussola::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("guardando -Y si guarda a nord"), ValdorsoBussola::Rotta(-90.f), 0.f);
	TestEqual(TEXT("guardando +X si guarda a est"), ValdorsoBussola::Rotta(0.f), 90.f);
	TestEqual(TEXT("guardando +Y si guarda a sud"), ValdorsoBussola::Rotta(90.f), 180.f);
	TestEqual(TEXT("guardando -X si guarda a ovest"), ValdorsoBussola::Rotta(180.f), 270.f);
	TestEqual(TEXT("anche con l'imbardata oltre il giro"), ValdorsoBussola::Rotta(-450.f), 0.f);

	const FVector Qui(1000.0, 1000.0, 0.0);
	TestTrue(TEXT("un punto a nord (Y minore)"), FMath::IsNearlyEqual(ValdorsoBussola::RottaVerso(Qui, FVector(1000.0, 0.0, 0.0)), 0.f, 0.01f)
		|| FMath::IsNearlyEqual(ValdorsoBussola::RottaVerso(Qui, FVector(1000.0, 0.0, 0.0)), 360.f, 0.01f));
	TestTrue(TEXT("un punto a nord-est"), FMath::IsNearlyEqual(ValdorsoBussola::RottaVerso(Qui, FVector(2000.0, 0.0, 500.0)), 45.f, 0.01f));
	TestTrue(TEXT("un punto a sud-ovest"), FMath::IsNearlyEqual(ValdorsoBussola::RottaVerso(Qui, FVector(0.0, 2000.0, 0.0)), 225.f, 0.01f));

	TestEqual(TEXT("0 gradi = N"), ValdorsoBussola::Sigla(ValdorsoBussola::PuntoCardinale(0.f)), FString(TEXT("N")));
	TestEqual(TEXT("44 gradi = NE"), ValdorsoBussola::Sigla(ValdorsoBussola::PuntoCardinale(44.f)), FString(TEXT("NE")));
	TestEqual(TEXT("90 gradi = E"), ValdorsoBussola::Sigla(ValdorsoBussola::PuntoCardinale(90.f)), FString(TEXT("E")));
	TestEqual(TEXT("225 gradi = SO"), ValdorsoBussola::Sigla(ValdorsoBussola::PuntoCardinale(225.f)), FString(TEXT("SO")));
	TestEqual(TEXT("270 gradi = O (ovest)"), ValdorsoBussola::Sigla(ValdorsoBussola::PuntoCardinale(270.f)), FString(TEXT("O")));
	TestEqual(TEXT("350 gradi = N"), ValdorsoBussola::Sigla(ValdorsoBussola::PuntoCardinale(350.f)), FString(TEXT("N")));
	TestEqual(TEXT("-45 gradi = NO"), ValdorsoBussola::Sigla(ValdorsoBussola::PuntoCardinale(-45.f)), FString(TEXT("NO")));
	TestTrue(TEXT("fuori da 0..7 niente sigla"), ValdorsoBussola::Sigla(8).IsEmpty());

	TestEqual(TEXT("a destra"), ValdorsoBussola::Scarto(0.f, 30.f), 30.f);
	TestEqual(TEXT("a sinistra, oltre lo zero"), ValdorsoBussola::Scarto(10.f, 350.f), -20.f);
	TestEqual(TEXT("a destra, oltre lo zero"), ValdorsoBussola::Scarto(350.f, 10.f), 20.f);
	TestTrue(TEXT("alle spalle: 180 gradi"), FMath::IsNearlyEqual(FMath::Abs(ValdorsoBussola::Scarto(0.f, 180.f)), 180.f, 0.01f));

	float X = -1.f;
	TestTrue(TEXT("davanti: al centro della striscia"), ValdorsoBussola::SullaStriscia(0.f, 420.f, 180.f, X) && FMath::IsNearlyEqual(X, 210.f, 0.01f));
	TestTrue(TEXT("45 gradi a destra: a tre quarti"), ValdorsoBussola::SullaStriscia(45.f, 420.f, 180.f, X) && FMath::IsNearlyEqual(X, 315.f, 0.01f));
	TestTrue(TEXT("90 gradi a sinistra: sul bordo"), ValdorsoBussola::SullaStriscia(-90.f, 420.f, 180.f, X) && FMath::IsNearlyEqual(X, 0.f, 0.01f));
	TestFalse(TEXT("alle spalle non si vede"), ValdorsoBussola::SullaStriscia(120.f, 420.f, 180.f, X));

	TestEqual(TEXT("37 m -> 35 m"), ValdorsoBussola::TestoDistanza(3700.0), FString(TEXT("35 m")));
	TestEqual(TEXT("234 m -> 230 m"), ValdorsoBussola::TestoDistanza(23400.0), FString(TEXT("230 m")));
	TestEqual(TEXT("1240 m -> 1,2 km"), ValdorsoBussola::TestoDistanza(124000.0), FString(TEXT("1,2 km")));
	TestEqual(TEXT("vicinissimo: almeno 5 m"), ValdorsoBussola::TestoDistanza(50.0), FString(TEXT("5 m")));
	return true;
}

// I luoghi della valle: ci sono, hanno un nome solo, stanno nella valle e si scoprono entrando nel loro raggio
// (in piano: la quota non conta).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestLuoghi, "Valdorso.Taccuino.Luoghi", ValdorsoTestTaccuino::Bandiere)
bool FValdorsoTestLuoghi::RunTest(const FString& Parameters)
{
	const TArray<FValdorsoLuogo>& Tutti = ValdorsoLuoghi::Tutti();
	TestTrue(TEXT("la valle ha i suoi luoghi"), Tutti.Num() >= 10);
	TSet<FString> Nomi;
	for (const FValdorsoLuogo& Luogo : Tutti)
	{
		TestFalse(TEXT("ogni luogo ha un nome"), Luogo.Nome.IsEmpty());
		TestFalse(FString::Printf(TEXT("%s: un nome solo"), *Luogo.Nome), Nomi.Contains(Luogo.Nome));
		Nomi.Add(Luogo.Nome);
		TestTrue(FString::Printf(TEXT("%s: un raggio sensato"), *Luogo.Nome), Luogo.Raggio >= 500.f && Luogo.Raggio <= 50000.f);
		// (08/11) il Landscape di L_Valle è centrato: la valle va da -1,5 km a +1,5 km (luoghi_gioco.py, ANGOLO_DI_SERIE)
		TestTrue(FString::Printf(TEXT("%s: dentro i 3 km della valle"), *Luogo.Nome),
			Luogo.Posizione.X >= -150000.0 && Luogo.Posizione.X <= 150000.0 && Luogo.Posizione.Y >= -150000.0 && Luogo.Posizione.Y <= 150000.0);
		TestTrue(FString::Printf(TEXT("%s: si ritrova per nome"), *Luogo.Nome), ValdorsoLuoghi::Trova(Luogo.Nome) == &Luogo);
	}
	TestNull(TEXT("un luogo che non c'è"), ValdorsoLuoghi::Trova(TEXT("Atlantide")));

	const FValdorsoLuogo* Mulino = ValdorsoLuoghi::Trova(TEXT("Mulino"));
	TestNotNull(TEXT("il mulino c'è"), Mulino);
	if (Mulino)
	{
		const FVector Centro = Mulino->Posizione;
		TestTrue(TEXT("nel centro si scopre"), ValdorsoLuoghi::Dentro(*Mulino, Centro));
		TestTrue(TEXT("appena dentro il raggio"), ValdorsoLuoghi::Dentro(*Mulino, Centro + FVector(Mulino->Raggio - 10.0, 0.0, 0.0)));
		TestFalse(TEXT("appena fuori no"), ValdorsoLuoghi::Dentro(*Mulino, Centro + FVector(0.0, Mulino->Raggio + 10.0, 0.0)));
		TestTrue(TEXT("la quota non conta (sul tetto del mulino)"), ValdorsoLuoghi::Dentro(*Mulino, Centro + FVector(0.0, 0.0, 5000.0)));
		bool bTrovato = false;
		for (const FValdorsoLuogo* Vicino : ValdorsoLuoghi::Intorno(Centro))
		{
			bTrovato |= Vicino == Mulino;
		}
		TestTrue(TEXT("Intorno trova il mulino"), bTrovato);
	}
	TestEqual(TEXT("fuori dal mondo nessun luogo"), ValdorsoLuoghi::Intorno(FVector(-500000.0, -500000.0, 0.0)).Num(), 0);

	// (08/11) Il posto di guardia del passo si scopre dove stanno le guardie: in PIE Ottone era in (112297, 99495) e
	// il taccuino restava vuoto (i luoghi erano spostati di 1,5 km, vedi luoghi_gioco.py). E anche alla porta stessa.
	const FValdorsoLuogo* Passo = ValdorsoLuoghi::Trova(TEXT("Posto di guardia del passo"));
	if (TestNotNull(TEXT("il posto di guardia del passo c'è"), Passo))
	{
		TestTrue(TEXT("si scopre accanto alle guardie"), ValdorsoLuoghi::Dentro(*Passo, FVector(112297.0, 99495.0, 6813.0)));
		TestTrue(TEXT("si scopre alla porta"), ValdorsoLuoghi::Dentro(*Passo, FVector(112200.0, 99960.0, 6739.0)));
	}
	return true;
}

// Il taccuino: i luoghi non si scrivono due volte, le iscrizioni si ricopiano pulite, i doppioni si ignorano, un testo
// nuovo con lo stesso titolo aggiorna, oltre le 200 si toglie la più vecchia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTaccuinoRegole, "Valdorso.Taccuino.Regole", ValdorsoTestTaccuino::Bandiere)
bool FValdorsoTestTaccuinoRegole::RunTest(const FString& Parameters)
{
	FValdorsoTaccuino T;
	TestTrue(TEXT("un luogo nuovo si aggiunge"), ValdorsoTaccuino::AggiungiLuogo(T, TEXT("Mulino"), 100, 18.5f));
	TestFalse(TEXT("lo stesso luogo non si aggiunge due volte"), ValdorsoTaccuino::AggiungiLuogo(T, TEXT("Mulino"), 200, 9.f));
	TestFalse(TEXT("un nome vuoto no"), ValdorsoTaccuino::AggiungiLuogo(T, FString(), 200, 9.f));
	TestEqual(TEXT("un luogo solo"), T.Luoghi.Num(), 1);
	TestEqual(TEXT("con la sua data"), T.Luoghi[0].Quando, static_cast<int64>(100));
	TestTrue(TEXT("lo conosce"), ValdorsoTaccuino::ConosceLuogo(T, TEXT("Mulino")));
	TestFalse(TEXT("il ponte no"), ValdorsoTaccuino::ConosceLuogo(T, TEXT("Ponte")));

	const TArray<FString> Testo = ValdorsoTestTaccuino::Righe({ TEXT("  Cercasi braccia per il raccolto.  "), TEXT(""), TEXT("rune: Aeth-Var | Dardo infuocato") });
	TestTrue(TEXT("un'iscrizione nuova"), ValdorsoTaccuino::AnnotaIscrizione(T, TEXT("Gilda di Valdorso"), Testo, 10) == EValdorsoLettura::Nuova);
	TestEqual(TEXT("le righe vuote via"), T.Iscrizioni[0].Righe.Num(), 2);
	TestEqual(TEXT("e gli spazi in più"), T.Iscrizioni[0].Righe[0], FString(TEXT("Cercasi braccia per il raccolto.")));
	TestTrue(TEXT("riletta uguale: niente doppione"), ValdorsoTaccuino::AnnotaIscrizione(T, TEXT("Gilda di Valdorso"), Testo, 20) == EValdorsoLettura::Ignorata);
	TestEqual(TEXT("sempre una"), T.Iscrizioni.Num(), 1);
	TestEqual(TEXT("la data resta la prima"), T.Iscrizioni[0].Quando, static_cast<int64>(10));
	TestTrue(TEXT("una bacheca vuota non si annota"), ValdorsoTaccuino::AnnotaIscrizione(T, TEXT("Vuota"), TArray<FString>(), 30) == EValdorsoLettura::Ignorata);
	TestTrue(TEXT("senza titolo diventa Bacheca"), ValdorsoTaccuino::AnnotaIscrizione(T, TEXT("  "), ValdorsoTestTaccuino::Righe({ TEXT("Avviso") }), 30) == EValdorsoLettura::Nuova);
	TestEqual(TEXT("col titolo Bacheca"), T.Iscrizioni.Last().Titolo, FString(TEXT("Bacheca")));
	TestTrue(TEXT("lo stesso titolo con avvisi nuovi si aggiorna"),
		ValdorsoTaccuino::AnnotaIscrizione(T, TEXT("Gilda di Valdorso"), ValdorsoTestTaccuino::Righe({ TEXT("Il raccolto è finito.") }), 40) == EValdorsoLettura::Aggiornata);
	TestEqual(TEXT("sempre due iscrizioni"), T.Iscrizioni.Num(), 2);
	TestEqual(TEXT("l'aggiornata va in fondo"), T.Iscrizioni.Last().Titolo, FString(TEXT("Gilda di Valdorso")));
	TestEqual(TEXT("con il testo nuovo"), T.Iscrizioni.Last().Righe[0], FString(TEXT("Il raccolto è finito.")));

	// le misure: 20 righe di 300 lettere, il titolo di 60
	TArray<FString> Lungo;
	for (int32 i = 0; i < 30; ++i)
	{
		Lungo.Add(FString::ChrN(400, TEXT('a')));
	}
	ValdorsoTaccuino::AnnotaIscrizione(T, FString::ChrN(100, TEXT('T')), Lungo, 50);
	TestEqual(TEXT("al massimo 20 righe"), T.Iscrizioni.Last().Righe.Num(), ValdorsoTaccuino::MassimoRighe);
	TestEqual(TEXT("di 300 lettere"), T.Iscrizioni.Last().Righe[0].Len(), ValdorsoTaccuino::MassimoLettere);
	TestEqual(TEXT("titolo di 60"), T.Iscrizioni.Last().Titolo.Len(), ValdorsoTaccuino::MassimoTitolo);

	// il massimo: oltre 200 via le più vecchie
	FValdorsoTaccuino Pieno;
	for (int32 i = 0; i < ValdorsoTaccuino::MassimoIscrizioni + 25; ++i)
	{
		ValdorsoTaccuino::AnnotaIscrizione(Pieno, FString::Printf(TEXT("Pietra %d"), i), ValdorsoTestTaccuino::Righe({ TEXT("Segni consumati.") }), i);
	}
	TestEqual(TEXT("al massimo 200"), Pieno.Iscrizioni.Num(), ValdorsoTaccuino::MassimoIscrizioni);
	TestEqual(TEXT("la prima rimasta è la 25"), Pieno.Iscrizioni[0].Titolo, FString(TEXT("Pietra 25")));
	TestEqual(TEXT("l'ultima è l'ultima letta"), Pieno.Iscrizioni.Last().Titolo, FString::Printf(TEXT("Pietra %d"), ValdorsoTaccuino::MassimoIscrizioni + 24));

	// Sistema: un file scritto a mano (o vecchio) torna in ordine
	FValdorsoTaccuino Disordinato;
	Disordinato.Luoghi.SetNum(3);
	Disordinato.Luoghi[0].Nome = TEXT("Ponte");
	Disordinato.Luoghi[1].Nome = TEXT("Ponte");
	Disordinato.Luoghi[2].Nome = FString();
	Disordinato.Iscrizioni.SetNum(2);
	Disordinato.Iscrizioni[0].Titolo = TEXT("La frana");
	Disordinato.Iscrizioni[0].Righe = ValdorsoTestTaccuino::Righe({ TEXT("Prima") });
	Disordinato.Iscrizioni[1].Titolo = TEXT("La frana");
	Disordinato.Iscrizioni[1].Righe = ValdorsoTestTaccuino::Righe({ TEXT("Dopo") });
	ValdorsoTaccuino::Sistema(Disordinato);
	TestEqual(TEXT("un ponte solo, niente nomi vuoti"), Disordinato.Luoghi.Num(), 1);
	TestEqual(TEXT("una frana sola"), Disordinato.Iscrizioni.Num(), 1);
	TestEqual(TEXT("vince l'ultima"), Disordinato.Iscrizioni[0].Righe[0], FString(TEXT("Dopo")));

	TestTrue(TEXT("ora sconosciuta: niente momento"), ValdorsoTaccuino::Momento(-1.f).IsEmpty());
	TestEqual(TEXT("alle 6 è l'alba"), ValdorsoTaccuino::Momento(6.f).ToString(), FString(TEXT("all'alba")));
	TestEqual(TEXT("alle 19 il tramonto"), ValdorsoTaccuino::Momento(19.f).ToString(), FString(TEXT("al tramonto")));
	TestEqual(TEXT("alle 2 è notte"), ValdorsoTaccuino::Momento(2.f).ToString(), FString(TEXT("di notte")));
	return true;
}

// Il taccuino si salva nel file del personaggio e si rilegge uguale; un file di prima (senza taccuino) si legge
// con il taccuino vuoto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTaccuinoFile, "Valdorso.Taccuino.File", ValdorsoTestTaccuino::Bandiere)
bool FValdorsoTestTaccuinoFile::RunTest(const FString& Parameters)
{
	FValdorsoPersonaggio Dati;
	Dati.Id = TEXT("prova");
	Dati.Nome = TEXT("Tessa");
	ValdorsoTaccuino::AggiungiLuogo(Dati.Taccuino, TEXT("Rovine degli Antichi"), 1762500000, 21.25f);
	ValdorsoTaccuino::AggiungiLuogo(Dati.Taccuino, TEXT("Collina dell'Orso"), 1762503600, -1.f);
	ValdorsoTaccuino::AnnotaIscrizione(Dati.Taccuino, TEXT("La lastra della cripta"),
		ValdorsoTestTaccuino::Righe({ TEXT("Qui dorme chi vegliava."), TEXT("rune: Ur") }), 1762500100);

	FString Testo;
	TestTrue(TEXT("si scrive"), FJsonObjectConverter::UStructToJsonObjectString(Dati, Testo));
	FValdorsoPersonaggio Riletto;
	TestTrue(TEXT("si rilegge"), FJsonObjectConverter::JsonObjectStringToUStruct(Testo, &Riletto, 0, 0));
	TestEqual(TEXT("due luoghi"), Riletto.Taccuino.Luoghi.Num(), 2);
	if (Riletto.Taccuino.Luoghi.Num() == 2)
	{
		TestEqual(TEXT("il primo nome"), Riletto.Taccuino.Luoghi[0].Nome, FString(TEXT("Rovine degli Antichi")));
		TestEqual(TEXT("la data"), Riletto.Taccuino.Luoghi[0].Quando, static_cast<int64>(1762500000));
		TestTrue(TEXT("l'ora della valle"), FMath::IsNearlyEqual(Riletto.Taccuino.Luoghi[0].Ora, 21.25f, 0.001f));
		TestEqual(TEXT("l'apostrofo resta"), Riletto.Taccuino.Luoghi[1].Nome, FString(TEXT("Collina dell'Orso")));
	}
	TestEqual(TEXT("un'iscrizione"), Riletto.Taccuino.Iscrizioni.Num(), 1);
	if (Riletto.Taccuino.Iscrizioni.Num() == 1)
	{
		TestEqual(TEXT("il titolo"), Riletto.Taccuino.Iscrizioni[0].Titolo, FString(TEXT("La lastra della cripta")));
		TestTrue(TEXT("le righe, con le rune"), Riletto.Taccuino.Iscrizioni[0].Righe
			== ValdorsoTestTaccuino::Righe({ TEXT("Qui dorme chi vegliava."), TEXT("rune: Ur") }));
	}

	// un personaggio salvato prima del taccuino (07/11): nel file non c'è
	const FString Vecchio = TEXT("{\"versioneSchema\":2,\"versione\":7,\"id\":\"vecchio\",\"accountId\":\"a1\",\"nome\":\"Bruno\",\"bHaPosizione\":false}");
	FValdorsoPersonaggio DiPrima;
	TestTrue(TEXT("il file di prima si legge"), FJsonObjectConverter::JsonObjectStringToUStruct(Vecchio, &DiPrima, 0, 0));
	TestEqual(TEXT("il nome c'è"), DiPrima.Nome, FString(TEXT("Bruno")));
	TestEqual(TEXT("nessun luogo"), DiPrima.Taccuino.Luoghi.Num(), 0);
	TestEqual(TEXT("nessuna iscrizione"), DiPrima.Taccuino.Iscrizioni.Num(), 0);
	return true;
}

#endif
