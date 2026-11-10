// Valdorso - Test automatici della Gilda e del Balivo (v0.1.4, 10/11): gradi, il giro di un foglio (staccato, assegnato,
// compiuto), rinunce e scadenze, gli incarichi degli abitanti, i comandi dello staff, i proclami, il foglio nello zaino.
// Scritto da Claude il 10/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.Gilda").

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoDialoghi.h"
#include "ValdorsoGildaDati.h"
#include "ValdorsoPermessi.h"
#include "ValdorsoRegistroEventi.h"
#include "ValdorsoLupi.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoZainoDati.h"
#include "Math/RandomStream.h"

namespace ValdorsoTestGilda
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	/** Un'ora qualunque (12/11/2026, a mezzogiorno). */
	constexpr int64 Ora = 1794484800;

	FValdorsoIncarico Incarico(EValdorsoTipoIncarico Tipo, FName Obiettivo, int32 Quanti, EValdorsoGrado Grado, int32 Monete, int32 Posti)
	{
		FValdorsoIncarico I;
		I.Titolo = TEXT("Prova");
		I.Testo = TEXT("Un incarico di prova.");
		I.Committente = TEXT("La Gilda");
		I.Tipo = Tipo;
		I.Obiettivo = Obiettivo;
		I.Quanti = Quanti;
		I.Grado = Grado;
		I.Monete = Monete;
		I.Punti = 3;
		I.Posti = Posti;
		I.Scadenza = Ora + 3 * 86400;
		I.bAbitanti = false;
		return I;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGildaGradi, "Valdorso.Gilda.Gradi", ValdorsoTestGilda::Bandiere)
bool FValdorsoTestGildaGradi::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("da zero: Comune"), ValdorsoGilda::GradoDi(0) == EValdorsoGrado::Comune);
	TestTrue(TEXT("9 punti: ancora Comune"), ValdorsoGilda::GradoDi(9) == EValdorsoGrado::Comune);
	TestTrue(TEXT("10 punti: Bronzo"), ValdorsoGilda::GradoDi(10) == EValdorsoGrado::Bronzo);
	TestTrue(TEXT("40 punti: Argento"), ValdorsoGilda::GradoDi(40) == EValdorsoGrado::Argento);
	TestTrue(TEXT("500 punti: Oro"), ValdorsoGilda::GradoDi(500) == EValdorsoGrado::Oro);
	TestEqual(TEXT("niente cauzione per il Bronzo"), ValdorsoGilda::Cauzione(ValdorsoTestGilda::Incarico(EValdorsoTipoIncarico::Caccia, TEXT("Lupo"), 1, EValdorsoGrado::Bronzo, 100, 1)), 0);
	TestEqual(TEXT("un decimo per l'Argento"), ValdorsoGilda::Cauzione(ValdorsoTestGilda::Incarico(EValdorsoTipoIncarico::Caccia, TEXT("Orso"), 1, EValdorsoGrado::Argento, 120, 1)), 12);
	TestEqual(TEXT("almeno uno"), ValdorsoGilda::Cauzione(ValdorsoTestGilda::Incarico(EValdorsoTipoIncarico::Caccia, TEXT("Orso"), 1, EValdorsoGrado::Oro, 5, 1)), 1);
	TestEqual(TEXT("fra 2 giorni"), ValdorsoGilda::TestoScadenza(ValdorsoTestGilda::Ora + 2 * 86400 + 10, ValdorsoTestGilda::Ora).ToString(), FString(TEXT("fra 2 giorni")));
	TestEqual(TEXT("fra 5 ore"), ValdorsoGilda::TestoScadenza(ValdorsoTestGilda::Ora + 5 * 3600, ValdorsoTestGilda::Ora).ToString(), FString(TEXT("fra 5 ore")));
	TestEqual(TEXT("scaduto"), ValdorsoGilda::TestoScadenza(ValdorsoTestGilda::Ora - 1, ValdorsoTestGilda::Ora).ToString(), FString(TEXT("scaduto")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGildaGiro, "Valdorso.Gilda.Giro", ValdorsoTestGilda::Bandiere)
bool FValdorsoTestGildaGiro::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestGilda;
	FValdorsoBachecheSalvate B;
	TestTrue(TEXT("si appende una caccia"), ValdorsoGilda::Metti(B, Incarico(EValdorsoTipoIncarico::Caccia, TEXT("Lupo"), 2, EValdorsoGrado::Comune, 30, 2), Ora));
	TestTrue(TEXT("e una consegna"), ValdorsoGilda::Metti(B, Incarico(EValdorsoTipoIncarico::Consegna, TEXT("Rami_Secchi"), 3, EValdorsoGrado::Comune, 10, 1), Ora));
	TestTrue(TEXT("e una del Bronzo"), ValdorsoGilda::Metti(B, Incarico(EValdorsoTipoIncarico::Raggiungi, TEXT("Cascata"), 1, EValdorsoGrado::Bronzo, 20, 1), Ora));
	const int32 Caccia = B.Incarichi[0].Numero;
	const int32 Consegna = B.Incarichi[1].Numero;
	const int32 Bronzo = B.Incarichi[2].Numero;
	TestNotEqual(TEXT("numeri diversi"), Caccia, Consegna);
	TestEqual(TEXT("tre fogli sulla bacheca"), ValdorsoGilda::Visibili(B, Ora).Num(), 3);

	FValdorsoGildaColono Io;
	TestFalse(TEXT("il Bronzo non lo prende un Comune"), ValdorsoGilda::Stacca(B, Io, Bronzo, Ora));
	TestTrue(TEXT("la caccia si stacca"), ValdorsoGilda::Stacca(B, Io, Caccia, Ora));
	TestFalse(TEXT("due volte no"), ValdorsoGilda::Stacca(B, Io, Caccia, Ora));
	TestEqual(TEXT("un posto preso"), ValdorsoGilda::Trova(B, Caccia)->Presi, 1);
	TestTrue(TEXT("prenotato"), ValdorsoGilda::Preso(Io, Caccia)->Stato == EValdorsoStatoIncarico::Prenotato);
	TestEqual(TEXT("una preda da prenotato non conta"), ValdorsoGilda::Avanza(Io, EValdorsoTipoIncarico::Caccia, TEXT("Lupo")), 0);
	TestTrue(TEXT("Lucia assegna"), ValdorsoGilda::Assegna(Io, Caccia, 0, Ora + 60));
	TestEqual(TEXT("un lupo"), ValdorsoGilda::Avanza(Io, EValdorsoTipoIncarico::Caccia, TEXT("Lupo")), 1);
	TestEqual(TEXT("un cinghiale non conta"), ValdorsoGilda::Avanza(Io, EValdorsoTipoIncarico::Caccia, TEXT("Cinghiale")), 0);
	TestFalse(TEXT("non ancora fatto"), ValdorsoGilda::Fatto(*ValdorsoGilda::Preso(Io, Caccia), 0));
	ValdorsoGilda::Avanza(Io, EValdorsoTipoIncarico::Caccia, TEXT("Lupo"));
	ValdorsoGilda::Avanza(Io, EValdorsoTipoIncarico::Caccia, TEXT("Lupo"));
	TestEqual(TEXT("non oltre il dovuto"), ValdorsoGilda::Preso(Io, Caccia)->Progresso, 2);
	TestTrue(TEXT("fatto"), ValdorsoGilda::Fatto(*ValdorsoGilda::Preso(Io, Caccia), 0));
	FValdorsoIncaricoPreso Fatto;
	TestTrue(TEXT("compiuto"), ValdorsoGilda::Compi(B, Io, Caccia, Fatto));
	TestEqual(TEXT("la paga del foglio"), Fatto.Foglio.Monete, 30);
	TestEqual(TEXT("i punti"), Io.Punti, 3);
	TestEqual(TEXT("niente piu' incarichi"), Io.Presi.Num(), 0);
	TestEqual(TEXT("sulla bacheca un compiuto"), ValdorsoGilda::Trova(B, Caccia)->Compiuti, 1);
	TestTrue(TEXT("resta un posto"), ValdorsoGilda::SullaBacheca(*ValdorsoGilda::Trova(B, Caccia), Ora));

	// la consegna: conta lo zaino
	TestTrue(TEXT("la consegna si stacca"), ValdorsoGilda::Stacca(B, Io, Consegna, Ora));
	TestTrue(TEXT("e si assegna"), ValdorsoGilda::Assegna(Io, Consegna, 0, Ora));
	TestFalse(TEXT("con due rami no"), ValdorsoGilda::Fatto(*ValdorsoGilda::Preso(Io, Consegna), 2));
	TestTrue(TEXT("con tre si'"), ValdorsoGilda::Fatto(*ValdorsoGilda::Preso(Io, Consegna), 3));
	TestFalse(TEXT("posti finiti: non e' piu' sulla bacheca"), ValdorsoGilda::SullaBacheca(*ValdorsoGilda::Trova(B, Consegna), Ora));

	// rinunciare da assegnato costa punti, e il posto torna libero
	TestTrue(TEXT("rinuncia"), ValdorsoGilda::Rinuncia(B, Io, Consegna));
	TestEqual(TEXT("due punti in meno"), Io.Punti, 1);
	TestTrue(TEXT("il foglio torna sulla bacheca"), ValdorsoGilda::SullaBacheca(*ValdorsoGilda::Trova(B, Consegna), Ora));

	// la prenotazione scade dopo mezz'ora, senza punti in meno
	TestTrue(TEXT("di nuovo"), ValdorsoGilda::Stacca(B, Io, Consegna, Ora));
	TestEqual(TEXT("prima di mezz'ora resta"), ValdorsoGilda::Scadute(B, Io, Ora + 60).Num(), 0);
	TestEqual(TEXT("dopo, via"), ValdorsoGilda::Scadute(B, Io, Ora + ValdorsoGilda::SecondiPrenotazione + 1).Num(), 1);
	TestEqual(TEXT("i punti restano"), Io.Punti, 1);
	TestEqual(TEXT("il posto e' libero"), ValdorsoGilda::Trova(B, Consegna)->Presi, 0);

	// al massimo tre incarichi
	FValdorsoGildaColono Tanti;
	Tanti.Punti = 50;
	Tanti.Grado = EValdorsoGrado::Argento;
	for (int32 k = 0; k < 4; ++k)
	{
		ValdorsoGilda::Metti(B, Incarico(EValdorsoTipoIncarico::Caccia, TEXT("Lepre"), 1, EValdorsoGrado::Comune, 5, 5), Ora);
	}
	int32 Staccati = 0;
	for (const FValdorsoIncarico& I : B.Incarichi)
	{
		Staccati += ValdorsoGilda::Stacca(B, Tanti, I.Numero, Ora) ? 1 : 0;
	}
	TestEqual(TEXT("al massimo tre"), Staccati, ValdorsoGilda::MassimoPresi);

	// l'Argento vuole la cauzione
	FValdorsoGildaColono Argento;
	Argento.Punti = 45;
	Argento.Grado = EValdorsoGrado::Argento;
	ValdorsoGilda::Metti(B, Incarico(EValdorsoTipoIncarico::Caccia, TEXT("Orso"), 1, EValdorsoGrado::Argento, 120, 1), Ora);
	const int32 Orso = B.Incarichi.Last().Numero;
	TestTrue(TEXT("l'orso si stacca"), ValdorsoGilda::Stacca(B, Argento, Orso, Ora));
	TestFalse(TEXT("senza la cauzione Lucia non lo assegna"), ValdorsoGilda::PercheNoAssegna(Argento, Orso, 5, Ora).IsEmpty());
	TestTrue(TEXT("con la cauzione si'"), ValdorsoGilda::Assegna(Argento, Orso, 12, Ora));
	TestEqual(TEXT("la cauzione segnata"), ValdorsoGilda::Preso(Argento, Orso)->Cauzione, 12);

	// gli scaduti si tolgono dalla bacheca
	TestTrue(TEXT("dopo la scadenza via tutti"), ValdorsoGilda::Pulisci(B, Ora + 4 * 86400) > 0);
	TestEqual(TEXT("bacheca vuota"), B.Incarichi.Num(), 0);
	TestEqual(TEXT("e l'orso dell'Argento scade (senza punti in meno)"), ValdorsoGilda::Scadute(B, Argento, Ora + 4 * 86400).Num(), 1);
	TestEqual(TEXT("punti dell'Argento uguali"), Argento.Punti, 45);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGildaAbitanti, "Valdorso.Gilda.Abitanti", ValdorsoTestGilda::Bandiere)
bool FValdorsoTestGildaAbitanti::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestGilda;
	FRandomStream Caso(312);
	TSet<FName> Visti;
	for (int32 k = 0; k < 400; ++k)
	{
		FValdorsoBachecheSalvate B;
		B.Cassa = ValdorsoGilda::CassaMassima;
		if (!ValdorsoGilda::Genera(B, Ora, Caso))
		{
			AddError(TEXT("con la cassa piena gli abitanti mettono un incarico"));
			break;
		}
		const FValdorsoIncarico& I = B.Incarichi.Last();
		Visti.Add(I.Modello);
		const FString Nome = I.Modello.ToString();
		TestTrue(FString::Printf(TEXT("%s: titolo, testo e committente"), *Nome), !I.Titolo.IsEmpty() && !I.Testo.IsEmpty() && !I.Committente.IsEmpty());
		TestTrue(FString::Printf(TEXT("%s: la paga dalla cassa"), *Nome), B.Cassa == ValdorsoGilda::CassaMassima - I.Monete * I.Posti);
		TestTrue(FString::Printf(TEXT("%s: scade fra almeno un giorno"), *Nome), I.Scadenza > Ora + 86400 - 7 * 3600);
		TestTrue(FString::Printf(TEXT("%s: testo non troppo lungo"), *Nome), I.Testo.Len() <= 300 && I.Titolo.Len() <= 60);
		switch (I.Tipo)
		{
		case EValdorsoTipoIncarico::Caccia:
			TestNotNull(FString::Printf(TEXT("%s: la specie c'e'"), *Nome), ValdorsoAnimali::Specie(I.Obiettivo));
			break;
		case EValdorsoTipoIncarico::Consegna:
			TestNotNull(FString::Printf(TEXT("%s: l'oggetto c'e'"), *Nome), ValdorsoOggetti::Trova(I.Obiettivo));
			break;
		case EValdorsoTipoIncarico::Raggiungi:
			TestNotNull(FString::Printf(TEXT("%s: il luogo c'e'"), *Nome), ValdorsoLuoghi::Trova(I.Obiettivo.ToString()));
			break;
		}
	}
	TestEqual(TEXT("si vedono tutti i modelli"), Visti.Num(), ValdorsoGilda::NumeroModelli());

	// al massimo sei, mai due uguali; senza soldi niente
	FValdorsoBachecheSalvate B;
	B.Cassa = ValdorsoGilda::CassaMassima;
	int32 Messi = 0;
	for (int32 k = 0; k < 20; ++k)
	{
		Messi += ValdorsoGilda::Genera(B, Ora, Caso) ? 1 : 0;
	}
	TestTrue(TEXT("al massimo sei"), Messi <= ValdorsoGilda::MassimoAbitanti);
	TSet<FName> Modelli;
	for (const FValdorsoIncarico& I : B.Incarichi)
	{
		TestFalse(TEXT("mai due uguali"), Modelli.Contains(I.Modello));
		Modelli.Add(I.Modello);
	}
	FValdorsoBachecheSalvate Povera;
	Povera.Cassa = 0;
	TestFalse(TEXT("senza cassa niente"), ValdorsoGilda::Genera(Povera, Ora, Caso));

	// la rendita e il ritorno nella cassa di quello che non si e' speso
	FValdorsoBachecheSalvate R;
	R.Cassa = 100;
	R.UltimaRendita = Ora;
	ValdorsoGilda::Rendita(R, Ora + 2 * 86400 + 100);
	TestEqual(TEXT("due giorni di rendita"), R.Cassa, 100 + 2 * ValdorsoGilda::RenditaAlGiorno);
	FValdorsoIncarico Speso = Incarico(EValdorsoTipoIncarico::Caccia, TEXT("Lupo"), 1, EValdorsoGrado::Comune, 10, 3);
	Speso.bAbitanti = true;
	ValdorsoGilda::Metti(R, Speso, Ora);
	R.Incarichi.Last().Compiuti = 1;
	const int32 Prima = R.Cassa;
	ValdorsoGilda::Pulisci(R, Ora + 10 * 86400);
	TestEqual(TEXT("i due posti non fatti tornano nella cassa"), R.Cassa, FMath::Min(ValdorsoGilda::CassaMassima, Prima + 20));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGildaComando, "Valdorso.Gilda.Comando", ValdorsoTestGilda::Bandiere)
bool FValdorsoTestGildaComando::RunTest(const FString& Parameters)
{
	FValdorsoIncarico I;
	FString Errore;
	TestTrue(TEXT("una caccia dello staff"), ValdorsoGilda::DaComando(TEXT("caccia lupo 3 bronzo 40 2 5 I lupi del boschetto | Tre lupi, prima della luna nuova."), I, Errore));
	TestTrue(TEXT("la specie con la maiuscola giusta"), I.Obiettivo == FName(TEXT("Lupo")));
	TestEqual(TEXT("quanti"), I.Quanti, 3);
	TestTrue(TEXT("grado"), I.Grado == EValdorsoGrado::Bronzo);
	TestEqual(TEXT("monete"), I.Monete, 40);
	TestEqual(TEXT("posti"), I.Posti, 2);
	TestEqual(TEXT("cinque giorni"), I.Scadenza, static_cast<int64>(5 * 86400));
	TestEqual(TEXT("il titolo"), I.Titolo, FString(TEXT("I lupi del boschetto")));
	TestEqual(TEXT("il testo"), I.Testo, FString(TEXT("Tre lupi, prima della luna nuova.")));
	TestFalse(TEXT("dello staff"), I.bAbitanti);
	TestTrue(TEXT("un luogo con gli spazi"), ValdorsoGilda::DaComando(TEXT("raggiungi grotta_est 9 comune 10 1 2 La grotta | Andate a vedere."), I, Errore));
	TestTrue(TEXT("il nome del luogo"), I.Obiettivo == FName(TEXT("Grotta est")));
	TestEqual(TEXT("raggiungere e' uno"), I.Quanti, 1);
	TestTrue(TEXT("una consegna"), ValdorsoGilda::DaComando(TEXT("consegna funghi 5 comune 8 3 2 Funghi | Per Bruno."), I, Errore));
	TestTrue(TEXT("l'Id dell'oggetto"), I.Obiettivo == FName(TEXT("Funghi")));
	TestFalse(TEXT("senza testo no"), ValdorsoGilda::DaComando(TEXT("caccia lupo 3 bronzo 40 2 5 Titolo"), I, Errore));
	TestFalse(TEXT("una bestia che non c'e'"), ValdorsoGilda::DaComando(TEXT("caccia drago 1 oro 900 1 3 Il drago | Ah."), I, Errore));
	TestFalse(TEXT("un grado che non c'e'"), ValdorsoGilda::DaComando(TEXT("caccia lupo 1 platino 9 1 3 X | Y."), I, Errore));
	TestFalse(TEXT("i ricordi non si consegnano"), ValdorsoGilda::DaComando(TEXT("consegna Tessera 1 comune 9 1 3 X | Y."), I, Errore));
	TestFalse(TEXT("numeri fuori misura"), ValdorsoGilda::DaComando(TEXT("caccia lupo 99 comune 9 1 3 X | Y."), I, Errore));
	TestTrue(TEXT("e c'e' un perche'"), Errore.Len() > 5 && Errore.Len() <= 200);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGildaProclami, "Valdorso.Gilda.Proclami", ValdorsoTestGilda::Bandiere)
bool FValdorsoTestGildaProclami::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestGilda;
	FValdorsoBachecheSalvate B;
	ValdorsoGilda::Proclama(B, TEXT("Il mercato si tiene ogni giorno."), TEXT("Fra"), false, Ora);
	const FValdorsoProclama& Legge = ValdorsoGilda::Proclama(B, TEXT("Di notte la porta del passo resta chiusa."), TEXT("Fra"), true, Ora);
	TestEqual(TEXT("la legge vale dal giorno dopo"), Legge.ValeDal, Ora + ValdorsoGilda::SecondiLegge);
	const TArray<FString> Righe = ValdorsoGilda::RigheProclami(B, Ora);
	TestEqual(TEXT("due righe"), Righe.Num(), 2);
	TestTrue(TEXT("la piu' nuova in cima"), Righe[0].Contains(TEXT("porta del passo")));
	TestTrue(TEXT("firmata dal Balivo"), Righe[0].Contains(TEXT("Per ordine del Balivo")));
	TestTrue(TEXT("con la data della valle"), Righe[1].StartsWith(TEXT("12 novembre 312")));
	TestTrue(TEXT("la legge dice da quando vale"), Righe[0].Contains(TEXT("vale dal 13 novembre 312")));
	TestTrue(TEXT("il giorno dopo e' legge"), ValdorsoGilda::RigheProclami(B, Ora + 2 * 86400)[0].Contains(TEXT("Legge della valle")));
	for (int32 k = 0; k < 30; ++k)
	{
		ValdorsoGilda::Proclama(B, FString::Printf(TEXT("Proclama %d"), k), TEXT("Fra"), false, Ora);
	}
	TestEqual(TEXT("al massimo venti"), B.Proclami.Num(), ValdorsoGilda::MassimoProclami);
	TestTrue(TEXT("i numeri crescono"), B.Proclami.Last().Numero > B.Proclami[0].Numero);
	TestTrue(TEXT("tagliato"), ValdorsoGilda::Proclama(B, FString::ChrN(1000, TEXT('a')), TEXT("Fra"), false, Ora).Testo.Len() == ValdorsoGilda::LettereProclama);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGildaFoglio, "Valdorso.Gilda.Foglio", ValdorsoTestGilda::Bandiere)
bool FValdorsoTestGildaFoglio::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("il foglio e' nel catalogo"), ValdorsoOggetti::Trova(ValdorsoGilda::IdFoglio));
	const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(ValdorsoGilda::IdFoglio);
	if (Tipo)
	{
		TestEqual(TEXT("uno per posto"), Tipo->Pila, 1);
		TestTrue(TEXT("non si getta"), Tipo->bRicordo);
	}
	FValdorsoZaino Z;
	ValdorsoZaino::Sistema(Z);
	const int32 A = ValdorsoZaino::AggiungiNumerato(Z, ValdorsoGilda::IdFoglio, 7);
	const int32 B = ValdorsoZaino::AggiungiNumerato(Z, ValdorsoGilda::IdFoglio, 9);
	TestTrue(TEXT("due fogli in due posti"), A != INDEX_NONE && B != INDEX_NONE && A != B);
	TestEqual(TEXT("si trova il 9"), ValdorsoZaino::PostoNumerato(Z, ValdorsoGilda::IdFoglio, 9), B);
	TestEqual(TEXT("l'8 no"), ValdorsoZaino::PostoNumerato(Z, ValdorsoGilda::IdFoglio, 8), static_cast<int32>(INDEX_NONE));
	ValdorsoZaino::Sistema(Z);
	TestEqual(TEXT("il numero resta dopo Sistema"), Z.Posti[A].Numero, 7);
	TestTrue(TEXT("spostando il numero va con il foglio"), ValdorsoZaino::Sposta(Z, A, 20));
	TestEqual(TEXT("al posto nuovo"), ValdorsoZaino::PostoNumerato(Z, ValdorsoGilda::IdFoglio, 7), 20);
	TestFalse(TEXT("non si getta"), ValdorsoZaino::SiPuoGettare(Z, 20));
	ValdorsoZaino::Aggiungi(Z, TEXT("Pane_Nero"), 3);
	const int32 Pane = ValdorsoZaino::PostoDi(Z, TEXT("Pane_Nero"));
	Z.Posti[Pane].Numero = 5;
	ValdorsoZaino::Sistema(Z);
	TestEqual(TEXT("chi si impila non ha numeri"), Z.Posti[Pane].Numero, 0);

	// Lucia apre il bancone da ogni saluto (la risposta con l'evento BanconeGilda, che chiude il dialogo)
	const FValdorsoDialogo* Lucia = ValdorsoDialoghi::Trova(TEXT("Lucia"));
	if (TestNotNull(TEXT("c'e' il dialogo di Lucia"), Lucia))
	{
		for (const FName Saluto : { Lucia->Inizio, Lucia->Ritorno })
		{
			const FValdorsoNodoDialogo* Nodo = Lucia->Trova(Saluto);
			const bool bBancone = Nodo && Nodo->Scelte.ContainsByPredicate([](const FValdorsoSceltaDialogo& S)
			{
				return S.Evento == ValdorsoGilda::EventoBancone && S.Verso.IsNone() && S.NascostaSe.IsNone();
			});
			TestTrue(FString::Printf(TEXT("Lucia/%s: \"Ho un foglio\" apre il bancone"), *Saluto.ToString()), bBancone);
		}
	}
	return true;
}

// (10/11) Il grado lo da' una persona: i punti dicono solo quando si e' pronti. Lo staff da' e toglie qualunque grado; il
// capo gilda dei giocatori solo il grado dopo, a chi ha i punti, fino all'Argento, e non a se stesso.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGildaNomina, "Valdorso.Gilda.Nomina", ValdorsoTestGilda::Bandiere)
bool FValdorsoTestGildaNomina::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestGilda;
	FValdorsoGildaColono Io;
	Io.Punti = 15;
	TestTrue(TEXT("i punti non danno il grado"), Io.Grado == EValdorsoGrado::Comune);
	TestTrue(TEXT("ma dicono che e' pronto"), ValdorsoGilda::Pronto(Io));
	TestTrue(TEXT("lo si dice una volta"), ValdorsoGilda::DaAvvisarePronto(Io));
	TestFalse(TEXT("non due"), ValdorsoGilda::DaAvvisarePronto(Io));
	// un Comune con 15 punti non stacca un foglio del Bronzo
	FValdorsoBachecheSalvate B;
	ValdorsoGilda::Metti(B, Incarico(EValdorsoTipoIncarico::Caccia, TEXT("Lupo"), 1, EValdorsoGrado::Bronzo, 10, 1), Ora);
	TestFalse(TEXT("senza grado il Bronzo no"), ValdorsoGilda::PercheNoStacca(B.Incarichi[0], Io, Ora).IsEmpty());

	FValdorsoGildaColono Capo;
	Capo.bCapoGilda = true;
	FValdorsoGildaColono Qualunque;
	TestFalse(TEXT("chi non e' capo non nomina"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Bronzo, false, &Qualunque, false).IsEmpty());
	TestTrue(TEXT("il capo nomina chi ha i punti"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Bronzo, false, &Capo, false).IsEmpty());
	TestFalse(TEXT("un grado alla volta"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Argento, false, &Capo, false).IsEmpty());
	TestFalse(TEXT("non se stesso"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Bronzo, false, &Capo, true).IsEmpty());
	TestTrue(TEXT("la nomina"), ValdorsoGilda::Nomina(Io, EValdorsoGrado::Bronzo));
	TestTrue(TEXT("adesso e' Bronzo"), Io.Grado == EValdorsoGrado::Bronzo);
	TestTrue(TEXT("il foglio del Bronzo si stacca"), ValdorsoGilda::PercheNoStacca(B.Incarichi[0], Io, Ora).IsEmpty());
	TestFalse(TEXT("senza i punti dell'Argento il capo no"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Argento, false, &Capo, false).IsEmpty());
	Io.Punti = 500;
	TestTrue(TEXT("con i punti si'"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Argento, false, &Capo, false).IsEmpty());
	ValdorsoGilda::Nomina(Io, EValdorsoGrado::Argento);
	TestFalse(TEXT("l'Oro lo da' solo lo staff"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Oro, false, &Capo, false).IsEmpty());
	TestTrue(TEXT("lo staff lo da'"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Oro, true, nullptr, false).IsEmpty());
	TestFalse(TEXT("il capo non retrocede"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Comune, false, &Capo, false).IsEmpty());
	TestTrue(TEXT("lo staff si'"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Comune, true, nullptr, false).IsEmpty());
	TestFalse(TEXT("lo stesso grado no"), ValdorsoGilda::PercheNoNomina(Io, EValdorsoGrado::Argento, true, nullptr, false).IsEmpty());

	// i permessi dello staff
	int32 Maschera = 0;
	TestTrue(TEXT("una lista di permessi"), ValdorsoPermessi::DaTesto(TEXT("gilda, Registro"), Maschera));
	TestEqual(TEXT("gilda e registro"), Maschera, ValdorsoPermessi::Gilda | ValdorsoPermessi::Registro);
	TestEqual(TEXT("si riscrive uguale"), ValdorsoPermessi::Testo(Maschera), FString(TEXT("gilda, registro")));
	TestTrue(TEXT("tutti"), ValdorsoPermessi::DaTesto(TEXT("tutti"), Maschera) && Maschera == ValdorsoPermessi::Tutti);
	TestTrue(TEXT("nessuno"), ValdorsoPermessi::DaTesto(TEXT("nessuno"), Maschera) && Maschera == 0);
	TestFalse(TEXT("un permesso che non c'e'"), ValdorsoPermessi::DaTesto(TEXT("gilda,volare"), Maschera));

	// il registro eventi ricorda gli ultimi, il piu' nuovo per primo
	ValdorsoRegistroEventi::Ricorda(TEXT("prova-registro uno"));
	ValdorsoRegistroEventi::Ricorda(TEXT("prova-registro due"));
	const TArray<FString> Ultimi = ValdorsoRegistroEventi::Ultimi(2, TEXT("prova-registro"));
	TestTrue(TEXT("il registro: il piu' nuovo per primo"), Ultimi.Num() == 2 && Ultimi[0].EndsWith(TEXT("due")) && Ultimi[1].EndsWith(TEXT("uno")));
	return true;
}

#endif
