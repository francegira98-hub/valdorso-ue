// Valdorso - Test automatici dei pericoli (v0.1.4, 08/11): barre, rinascita al tempio, equilibrio, chat di prossimità.
// Scritto da Claude il 08/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.Pericoli").

#include "Misc/AutomationTest.h"
#include "Algo/Reverse.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoCombattimento.h"
#include "ValdorsoBotteghe.h"
#include "ValdorsoAcque.h"
#include "ValdorsoBagnato.h"
#include "ValdorsoDialoghi.h"
#include "ValdorsoVitaValle.h"
#include "ValdorsoLupi.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoZainoDati.h"

#include "ValdorsoBarre.h"
#include "ValdorsoChat.h"
#include "ValdorsoPericoli.h"
#include "ValdorsoTaccuinoDati.h"

namespace ValdorsoTestPericoli
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Fa passare il tempo a una barra (passi di 1/20 di secondo) con lo stesso valore; restituisce l'ora alla fine. */
	double Aspetta(FValdorsoStatoBarra& Stato, float Valore, float Massimo, double Da, double Secondi)
	{
		const float Passo = 0.05f;
		double Adesso = Da;
		while (Adesso < Da + Secondi - 0.0001)
		{
			Adesso += Passo;
			ValdorsoBarre::Aggiorna(Stato, Valore, Massimo, Adesso, Passo);
		}
		return Adesso;
	}
}

// Le barre: compaiono quando cambiano o non sono piene, restano 3 secondi dopo essere tornate piene, poi sfumano;
// la scia del danno scende piano; i numeri.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPericoliBarre, "Valdorso.Pericoli.Barre", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestPericoliBarre::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestPericoli;

	TestEqual(TEXT("metà"), ValdorsoBarre::Frazione(50.f, 100.f), 0.5f);
	TestEqual(TEXT("senza massimo vale piena"), ValdorsoBarre::Frazione(10.f, 0.f), 1.f);
	TestEqual(TEXT("mai oltre piena"), ValdorsoBarre::Frazione(130.f, 100.f), 1.f);
	TestEqual(TEXT("mai sotto zero"), ValdorsoBarre::Frazione(-5.f, 100.f), 0.f);
	TestTrue(TEXT("99,95 su 100 è piena"), ValdorsoBarre::Piena(ValdorsoBarre::Frazione(99.95f, 100.f)));
	TestFalse(TEXT("99 su 100 non è piena"), ValdorsoBarre::Piena(ValdorsoBarre::Frazione(99.f, 100.f)));

	// una barra piena appena letta: si vede un momento, poi sparisce
	FValdorsoStatoBarra Stato;
	TestFalse(TEXT("mai letta: non si vede"), ValdorsoBarre::DaMostrare(Stato, 0.0));
	ValdorsoBarre::Aggiorna(Stato, 100.f, 100.f, 0.0, 0.f);
	TestTrue(TEXT("la prima lettura si vede"), ValdorsoBarre::DaMostrare(Stato, 0.0));
	double Adesso = Aspetta(Stato, 100.f, 100.f, 0.0, 1.0);
	TestTrue(TEXT("dopo 1 s è comparsa tutta"), FMath::IsNearlyEqual(Stato.Presenza, 1.f, 0.01f));
	Adesso = Aspetta(Stato, 100.f, 100.f, Adesso, 1.8);
	TestTrue(TEXT("a 2,8 s si vede ancora"), ValdorsoBarre::DaMostrare(Stato, Adesso));
	Adesso = Aspetta(Stato, 100.f, 100.f, Adesso, 0.4);
	TestFalse(TEXT("a 3,2 s non deve più vedersi"), ValdorsoBarre::DaMostrare(Stato, Adesso));
	Adesso = Aspetta(Stato, 100.f, 100.f, Adesso, ValdorsoBarre::SecondiSfumatura + 0.1);
	TestTrue(TEXT("e dopo la sfumatura è sparita"), Stato.Presenza <= 0.01f);

	// cambia (un colpo): ricompare e resta finché non è piena
	ValdorsoBarre::Aggiorna(Stato, 80.f, 100.f, Adesso + 0.05, 0.05f);
	Adesso += 0.05;
	TestTrue(TEXT("dopo il colpo si vede"), ValdorsoBarre::DaMostrare(Stato, Adesso));
	Adesso = Aspetta(Stato, 80.f, 100.f, Adesso, 10.0);
	TestTrue(TEXT("non piena: resta anche dopo 10 s"), ValdorsoBarre::DaMostrare(Stato, Adesso) && FMath::IsNearlyEqual(Stato.Presenza, 1.f, 0.01f));
	// torna piena: 3 secondi e via
	ValdorsoBarre::Aggiorna(Stato, 100.f, 100.f, Adesso + 0.05, 0.05f);
	Adesso += 0.05;
	const double PienaDa = Adesso;
	Adesso = Aspetta(Stato, 100.f, 100.f, Adesso, 2.9);
	TestTrue(TEXT("piena da 2,9 s: si vede ancora"), ValdorsoBarre::DaMostrare(Stato, Adesso));
	Adesso = Aspetta(Stato, 100.f, 100.f, Adesso, 0.2);
	TestFalse(TEXT("piena da 3,1 s: sfuma"), ValdorsoBarre::DaMostrare(Stato, Adesso));
	TestTrue(TEXT("i 3 secondi si contano da quando è tornata piena"), Adesso - PienaDa >= ValdorsoBarre::SecondiDopoPiena);

	// la scia: da piena a metà, resta indietro e poi raggiunge la barra (0,5 barre al secondo)
	FValdorsoStatoBarra Colpita;
	ValdorsoBarre::Aggiorna(Colpita, 100.f, 100.f, 0.0, 0.f);
	ValdorsoBarre::Aggiorna(Colpita, 50.f, 100.f, 0.05, 0.05f);
	TestTrue(TEXT("subito dopo il colpo la scia è ancora alta"), Colpita.Scia > 0.9f && FMath::IsNearlyEqual(Colpita.Frazione, 0.5f));
	Aspetta(Colpita, 50.f, 100.f, 0.05, 1.1);
	TestTrue(TEXT("dopo un secondo la scia ha raggiunto la barra"), FMath::IsNearlyEqual(Colpita.Scia, 0.5f, 0.001f));
	ValdorsoBarre::Aggiorna(Colpita, 90.f, 100.f, 1.2, 0.05f);
	TestTrue(TEXT("quando la barra sale la scia la segue subito"), FMath::IsNearlyEqual(Colpita.Scia, 0.9f, 0.001f));

	TestEqual(TEXT("i numeri"), ValdorsoBarre::TestoNumeri(73.4f, 100.f), FString(TEXT("73 / 100")));
	TestEqual(TEXT("i numeri, mai sotto zero"), ValdorsoBarre::TestoNumeri(-2.f, 100.f), FString(TEXT("0 / 100")));
	TestEqual(TEXT("otto posti nella barra delle azioni"), ValdorsoBarre::NumeroAzioni, 8);
	return true;
}

// La rinascita: davanti all'altare con la pietra, verso l'ingresso (dal centro del tempio), con i piedi sul pavimento.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPericoliRinascita, "Valdorso.Pericoli.Rinascita", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestPericoliRinascita::RunTest(const FString& Parameters)
{
	// i numeri veri: l'altare a (165300, 188058) e il centro del tempio a (165300, 188878) dall'angolo del paesaggio
	// (metti_png.py), cioè meno 150000 nel mondo; la pietra sta sull'altare, 118 cm sopra il pavimento (5513)
	const FVector Pietra(15300.0, 38058.0, 5513.0 + ValdorsoPericoli::AltezzaAltare);
	const FVector Centro(15300.0, 38878.0, 5513.0);
	const FVector Verso = ValdorsoPericoli::VersoIngresso(Pietra, Centro);
	TestTrue(TEXT("l'ingresso è a sud (+Y)"), Verso.Equals(FVector(0.0, 1.0, 0.0), 0.001));

	const float MezzaAltezza = 96.f;
	const FTransform Punto = ValdorsoPericoli::DavantiAllAltare(Pietra, Verso, MezzaAltezza);
	TestTrue(TEXT("2,5 m davanti all'altare"), Punto.GetLocation().Equals(FVector(15300.0, 38058.0 + ValdorsoPericoli::DistanzaDallAltare, 5513.0 + MezzaAltezza + 2.0), 0.01));
	TestTrue(TEXT("girato verso l'ingresso (imbardata 90)"), FMath::IsNearlyEqual(static_cast<float>(Punto.Rotator().Yaw), 90.f, 0.01f));
	TestTrue(TEXT("dritto (niente beccheggio)"), FMath::IsNearlyZero(static_cast<float>(Punto.Rotator().Pitch), 0.01f));

	// il centro del tempio più in basso o più in alto non conta (il verso è in piano)
	TestTrue(TEXT("il verso è in piano"), FMath::IsNearlyZero(ValdorsoPericoli::VersoIngresso(Pietra, Centro + FVector(0.0, 0.0, 900.0)).Z));
	// un altro verso: in diagonale, lungo 1
	const FVector Diagonale = ValdorsoPericoli::VersoIngresso(FVector::ZeroVector, FVector(300.0, 300.0, 0.0));
	TestTrue(TEXT("in diagonale, lungo 1"), FMath::IsNearlyEqual(Diagonale.Size(), 1.0, 0.0001) && FMath::IsNearlyEqual(Diagonale.X, Diagonale.Y, 0.0001));
	const FTransform Diagonalmente = ValdorsoPericoli::DavantiAllAltare(FVector::ZeroVector, Diagonale, MezzaAltezza);
	TestTrue(TEXT("girato in diagonale (imbardata 45)"), FMath::IsNearlyEqual(static_cast<float>(Diagonalmente.Rotator().Yaw), 45.f, 0.01f));
	// i due punti quasi uguali: verso sud, come il tempio
	TestTrue(TEXT("pietra e centro coincidono: a sud"), ValdorsoPericoli::VersoIngresso(Pietra, Pietra + FVector(10.0, 0.0, 0.0)).Equals(FVector(0.0, 1.0, 0.0), 0.001));
	TestTrue(TEXT("verso nullo: a sud"), ValdorsoPericoli::DavantiAllAltare(Pietra, FVector::ZeroVector, MezzaAltezza).GetLocation().Y > Pietra.Y);

	// con i dati del gioco: il luogo "Tempio del Cuore" c'è e sta davanti all'altare (verso sud)
	const FValdorsoLuogo* Tempio = ValdorsoLuoghi::Trova(TEXT("Tempio del Cuore"));
	TestNotNull(TEXT("il luogo Tempio del Cuore"), Tempio);
	if (Tempio)
	{
		TestTrue(TEXT("dalla pietra il tempio è verso sud"), ValdorsoPericoli::VersoIngresso(Pietra, Tempio->Posizione).Y > 0.99);
	}
	return true;
}

// L'equilibrio: a zero si barcolla; tre pugni di fila non bastano, il quarto sì; la salute a zero fa cadere.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPericoliEquilibrio, "Valdorso.Pericoli.Equilibrio", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestPericoliEquilibrio::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("pieno: in piedi"), ValdorsoPericoli::PerdeEquilibrio(100.f));
	TestFalse(TEXT("1: ancora in piedi"), ValdorsoPericoli::PerdeEquilibrio(1.f));
	TestTrue(TEXT("0: barcolla"), ValdorsoPericoli::PerdeEquilibrio(0.f));
	TestTrue(TEXT("quasi zero: barcolla"), ValdorsoPericoli::PerdeEquilibrio(0.4f));

	// pugni ogni 0,8 secondi: tra un pugno e l'altro l'equilibrio ricresce di 5 al secondo
	float Equilibrio = 100.f;
	int32 Pugni = 0;
	while (!ValdorsoPericoli::PerdeEquilibrio(Equilibrio) && Pugni < 10)
	{
		Equilibrio = FMath::Clamp(Equilibrio - ValdorsoPericoli::EquilibrioPerPugno, 0.f, 100.f);
		++Pugni;
		if (!ValdorsoPericoli::PerdeEquilibrio(Equilibrio))
		{
			Equilibrio = FMath::Min(100.f, Equilibrio + ValdorsoPericoli::EquilibrioAlSecondo * 0.8f);
		}
	}
	TestEqual(TEXT("barcolla al quarto pugno di fila"), Pugni, 4);
	// e con calma (un pugno ogni 7 secondi) non barcolla mai
	Equilibrio = 100.f;
	for (int32 i = 0; i < 10; ++i)
	{
		Equilibrio = FMath::Clamp(Equilibrio - ValdorsoPericoli::EquilibrioPerPugno, 0.f, 100.f);
		TestFalse(TEXT("un pugno ogni 7 secondi: in piedi"), ValdorsoPericoli::PerdeEquilibrio(Equilibrio));
		Equilibrio = FMath::Min(100.f, Equilibrio + ValdorsoPericoli::EquilibrioAlSecondo * 7.f);
	}

	TestTrue(TEXT("salute 0: cade"), ValdorsoPericoli::Caduta(0.f));
	TestTrue(TEXT("salute sotto zero: cade"), ValdorsoPericoli::Caduta(-3.f));
	TestFalse(TEXT("salute 0,5: in piedi"), ValdorsoPericoli::Caduta(0.5f));
	TestFalse(TEXT("salute piena: in piedi"), ValdorsoPericoli::Caduta(100.f));
	return true;
}

// La chat: la frase pulita, il grido, una frase al secondo, chi sente, quanto restano le frasi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPericoliChat, "Valdorso.Pericoli.Chat", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestPericoliChat::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("spazi all'inizio, alla fine e in fila"), ValdorsoChat::Pulisci(TEXT("   ciao    a  tutti \n")), FString(TEXT("ciao a tutti")));
	TestEqual(TEXT("a capo e tabulazioni diventano spazi"), ValdorsoChat::Pulisci(TEXT("uno\r\ndue\tre")), FString(TEXT("uno due re")));
	TestEqual(TEXT("le lettere accentate restano"), ValdorsoChat::Pulisci(TEXT("Così è, perché sì")), FString(TEXT("Così è, perché sì")));
	TestTrue(TEXT("vuota resta vuota"), ValdorsoChat::Pulisci(TEXT(" \t\n ")).IsEmpty());

	FString Sporca(TEXT("a"));
	Sporca.AppendChar(static_cast<TCHAR>(0x01));
	Sporca.AppendChar(static_cast<TCHAR>(0x07));
	Sporca += TEXT("b");
	Sporca.AppendChar(static_cast<TCHAR>(0x202E));   // gira il verso del testo
	Sporca.AppendChar(static_cast<TCHAR>(0x200B));   // spazio invisibile
	Sporca += TEXT("c");
	Sporca.AppendChar(static_cast<TCHAR>(0x7F));
	Sporca.AppendChar(static_cast<TCHAR>(0x85));
	TestEqual(TEXT("via i caratteri di controllo e invisibili"), ValdorsoChat::Pulisci(Sporca), FString(TEXT("abc")));

	const FString Lunga = FString::ChrN(300, TEXT('a'));
	TestEqual(TEXT("al massimo 200 caratteri"), ValdorsoChat::Pulisci(Lunga).Len(), ValdorsoChat::LunghezzaMassima);
	const FString AlLimite = FString::ChrN(199, TEXT('a')) + TEXT(" b");
	TestTrue(TEXT("tagliata al limite: niente spazio in fondo"), !ValdorsoChat::Pulisci(AlLimite).EndsWith(TEXT(" ")) && ValdorsoChat::Pulisci(AlLimite).Len() <= ValdorsoChat::LunghezzaMassima);

	FString Grido(TEXT("/grida Aiuto, al lupo!"));
	TestTrue(TEXT("/grida è un grido"), ValdorsoChat::TogliGrido(Grido));
	TestEqual(TEXT("senza /grida"), Grido, FString(TEXT("Aiuto, al lupo!")));
	FString Maiuscolo(TEXT("/GRIDA ehi"));
	TestTrue(TEXT("anche in maiuscolo"), ValdorsoChat::TogliGrido(Maiuscolo) && Maiuscolo == TEXT("ehi"));
	FString Parola(TEXT("/gridare non è un comando"));
	TestFalse(TEXT("/gridare non è /grida"), ValdorsoChat::TogliGrido(Parola));
	TestEqual(TEXT("e resta com'era"), Parola, FString(TEXT("/gridare non è un comando")));
	FString InMezzo(TEXT("ciao /grida"));
	TestFalse(TEXT("solo all'inizio"), ValdorsoChat::TogliGrido(InMezzo));
	FString Solo(TEXT("/grida"));
	TestTrue(TEXT("/grida da solo: grido vuoto (il server lo scarta)"), ValdorsoChat::TogliGrido(Solo) && Solo.IsEmpty());

	TestFalse(TEXT("dopo mezzo secondo no"), ValdorsoChat::PuoParlare(10.0, 10.5));
	TestTrue(TEXT("dopo un secondo sì"), ValdorsoChat::PuoParlare(10.0, 11.0));
	TestTrue(TEXT("la prima frase sempre"), ValdorsoChat::PuoParlare(-1000.0, 0.0));
	TestTrue(TEXT("il server è un poco più largo (la rete avvicina le frasi)"), ValdorsoChat::PuoParlare(10.0, 10.8, ValdorsoChat::SecondiTraFrasiServer));
	TestFalse(TEXT("ma non troppo"), ValdorsoChat::PuoParlare(10.0, 10.5, ValdorsoChat::SecondiTraFrasiServer));

	const FVector Qui(1000.0, 2000.0, 100.0);
	TestTrue(TEXT("chi parla sente se stesso"), ValdorsoChat::Sente(Qui, Qui, false));
	TestTrue(TEXT("a 29,99 m si sente"), ValdorsoChat::Sente(Qui, Qui + FVector(2999.0, 0.0, 0.0), false));
	TestFalse(TEXT("a 30,01 m no"), ValdorsoChat::Sente(Qui, Qui + FVector(3001.0, 0.0, 0.0), false));
	TestFalse(TEXT("conta anche l'altezza (in cima alla torre)"), ValdorsoChat::Sente(Qui, Qui + FVector(2500.0, 0.0, 2500.0), false));
	TestTrue(TEXT("un grido a 79,99 m si sente"), ValdorsoChat::Sente(Qui, Qui + FVector(0.0, 7999.0, 0.0), true));
	TestFalse(TEXT("un grido a 80,01 m no"), ValdorsoChat::Sente(Qui, Qui + FVector(0.0, 8001.0, 0.0), true));

	TestEqual(TEXT("appena arrivata: si vede"), ValdorsoChat::Presenza(0.0), 1.f);
	TestEqual(TEXT("a 20 s: si vede ancora"), ValdorsoChat::Presenza(20.0), 1.f);
	TestTrue(TEXT("a 21 s: a metà"), FMath::IsNearlyEqual(ValdorsoChat::Presenza(21.0), 0.5f, 0.001f));
	TestEqual(TEXT("a 22 s: sparita"), ValdorsoChat::Presenza(22.0), 0.f);
	TestEqual(TEXT("molto dopo: sparita"), ValdorsoChat::Presenza(500.0), 0.f);

	FValdorsoFraseChat Frase;
	Frase.Nome = TEXT("Bruno");
	Frase.Testo = TEXT("Salve, forestiero.");
	TestEqual(TEXT("Nome: testo"), ValdorsoChat::Riga(Frase), FString(TEXT("Bruno: Salve, forestiero.")));
	Frase.bGrida = true;
	TestEqual(TEXT("Nome grida: testo"), ValdorsoChat::Riga(Frase), FString(TEXT("Bruno grida: Salve, forestiero.")));
	Frase.Nome.Empty();
	TestEqual(TEXT("un avviso: solo il testo"), ValdorsoChat::Riga(Frase), FString(TEXT("Salve, forestiero.")));

	TArray<FValdorsoFraseChat> Elenco;
	for (int32 i = 0; i < 11; ++i)
	{
		FValdorsoFraseChat Nuova;
		Nuova.Testo = FString::FromInt(i);
		ValdorsoChat::Aggiungi(Elenco, Nuova);
	}
	TestEqual(TEXT("si tengono le ultime 8"), Elenco.Num(), ValdorsoChat::FrasiTenute);
	TestEqual(TEXT("la più vecchia rimasta è la quarta"), Elenco[0].Testo, FString(TEXT("3")));
	TestEqual(TEXT("l'ultima è l'ultima arrivata"), Elenco.Last().Testo, FString(TEXT("10")));
	return true;
}

// (09/11, v0.1.4 passo 4.4) La pace del battito: un colpo tra giocatori lascia sempre almeno 1 di salute (e si sa che e'
// stato fermato); chi e' gia' a 1 non perde piu' niente; fuori dalla pace o se il colpo e' piccolo passa tutto.
// Le spinte e i tremori dei colpi stanno nei limiti; i suoni hanno un nome per tipo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPaceBattito, "Valdorso.Pericoli.PaceDelBattito", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestPaceBattito::RunTest(const FString& Parameters)
{
	bool bFermato = false;
	TestEqual(TEXT("un colpo piccolo passa"), ValdorsoCombattimento::DannoNellaPace(10.f, 100.f, bFermato), 10.f);
	TestFalse(TEXT("non fermato"), bFermato);
	TestEqual(TEXT("il colpo che uccide lascia 1"), ValdorsoCombattimento::DannoNellaPace(50.f, 30.f, bFermato), 29.f);
	TestTrue(TEXT("fermato"), bFermato);
	TestEqual(TEXT("a 1 non si perde piu' niente"), ValdorsoCombattimento::DannoNellaPace(15.f, 1.f, bFermato), 0.f);
	TestTrue(TEXT("fermato anche qui"), bFermato);
	TestEqual(TEXT("niente danno negativo"), ValdorsoCombattimento::DannoNellaPace(-5.f, 80.f, bFermato), 0.f);
	TestFalse(TEXT("nessun mondo, nessuna pace"), ValdorsoCombattimento::InPace(nullptr, FVector::ZeroVector));
	for (uint8 i = 0; i <= static_cast<uint8>(EValdorsoTipoColpo::Scudo); ++i)
	{
		const EValdorsoTipoColpo Tipo = static_cast<EValdorsoTipoColpo>(i);
		TestTrue(TEXT("spinta ragionevole"), ValdorsoCombattimento::Spinta(Tipo) > 50.f && ValdorsoCombattimento::Spinta(Tipo) < 500.f);
		TestTrue(TEXT("tremore fra 0 e 1"), ValdorsoCombattimento::Tremore(Tipo) > 0.f && ValdorsoCombattimento::Tremore(Tipo) <= 1.f);
		TestTrue(TEXT("il suono ha un nome"), FString(ValdorsoCombattimento::NomeSuono(Tipo)).StartsWith(TEXT("S_Colpo_")));
	}
	return true;
}


// (09/11, v0.1.4 passo 4.5) Le armi in mano: si impugna solo quello che c'e' nello zaino e nella mano giusta (lo scudo
// nella sinistra), di nuovo lo si rimette via; quello che si getta sparisce anche di mano. Lo scudo para solo davanti.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestArmi, "Valdorso.Pericoli.Armi", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestArmi::RunTest(const FString& Parameters)
{
	FValdorsoZaino Zaino;
	ValdorsoZaino::Sistema(Zaino);
	TestFalse(TEXT("senza la spada non la si impugna"), ValdorsoZaino::Impugna(Zaino, TEXT("Spada_Corta")));
	TestTrue(TEXT("mano vuota"), Zaino.Destra.IsNone());
	ValdorsoZaino::Aggiungi(Zaino, TEXT("Spada_Corta"), 1, -1);
	ValdorsoZaino::Aggiungi(Zaino, TEXT("Scudo_Legno"), 1, -1);
	ValdorsoZaino::Aggiungi(Zaino, TEXT("Pane_Nero"), 1, -1);
	TestTrue(TEXT("la spada nella destra"), ValdorsoZaino::Impugna(Zaino, TEXT("Spada_Corta")));
	TestTrue(TEXT("destra"), Zaino.Destra == FName(TEXT("Spada_Corta")));
	TestTrue(TEXT("lo scudo nella sinistra"), ValdorsoZaino::Impugna(Zaino, TEXT("Scudo_Legno")));
	TestTrue(TEXT("sinistra"), Zaino.Sinistra == FName(TEXT("Scudo_Legno")));
	TestFalse(TEXT("il pane non si impugna"), ValdorsoZaino::Impugna(Zaino, TEXT("Pane_Nero")));
	TestFalse(TEXT("niente da sistemare"), ValdorsoZaino::SistemaMani(Zaino));
	TestFalse(TEXT("di nuovo: la spada torna via"), ValdorsoZaino::Impugna(Zaino, TEXT("Spada_Corta")));
	TestTrue(TEXT("mano destra vuota"), Zaino.Destra.IsNone());
	ValdorsoZaino::TogliOggetto(Zaino, TEXT("Scudo_Legno"), 1);
	TestTrue(TEXT("lo scudo gettato non resta in mano"), ValdorsoZaino::SistemaMani(Zaino));
	TestTrue(TEXT("mano sinistra vuota"), Zaino.Sinistra.IsNone());
	// ogni arma ha danno, portata e la mano giusta
	for (const TCHAR* Id : { TEXT("Coltello"), TEXT("Pugnale"), TEXT("Falcetto"), TEXT("Martello"), TEXT("Bastone_Pastore"), TEXT("Spada_Corta"),
		TEXT("Accetta"), TEXT("Ascia_Guerra"), TEXT("Mazza"), TEXT("Clava"), TEXT("Lancia"), TEXT("Forcone"), TEXT("Bastone_Ferrato"),
		TEXT("Spada_Lunga"), TEXT("Gladio") })
	{
		const FValdorsoTipoOggetto* Tipo = ValdorsoOggetti::Trova(Id);
		TestNotNull(Id, Tipo);
		if (Tipo)
		{
			TestTrue(TEXT("nella destra"), Tipo->Posto == EValdorsoPosto::ManoDestra && Tipo->Uso == EValdorsoUso::Impugna);
			TestTrue(TEXT("danno piu' del pugno"), Tipo->DannoArma > 10.f && Tipo->PortataArma > 70.f);
		}
	}
	TestTrue(TEXT("davanti si para"), ValdorsoCombattimento::DaDavanti(FVector(1, 0, 0), FVector(300, 50, 0)));
	TestFalse(TEXT("di lato no"), ValdorsoCombattimento::DaDavanti(FVector(1, 0, 0), FVector(0, 300, 0)));
	TestFalse(TEXT("da dietro no"), ValdorsoCombattimento::DaDavanti(FVector(1, 0, 0), FVector(-300, 0, 0)));
	return true;
}

// (09/11, v0.1.4 passo 4.6) I lupi: escono di notte o con la nebbia; fiutano, accerchiano, mordono; chi sta al fuoco o
// nella pace non viene morso; feriti fuggono; il branco e' di 3 o 4.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestLupi, "Valdorso.Pericoli.Lupi", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestLupi::RunTest(const FString& Parameters)
{
	using E = EValdorsoStatoLupo;
	TestTrue(TEXT("di notte escono"), ValdorsoLupi::Fuori(0.f, false));
	TestTrue(TEXT("con la nebbia escono"), ValdorsoLupi::Fuori(1.f, true));
	TestFalse(TEXT("di giorno no"), ValdorsoLupi::Fuori(1.f, false));
	TestTrue(TEXT("nessuno: vaga"), ValdorsoLupi::Prossimo(E::Vaga, 1.f, -1.f, false, 0.f) == E::Vaga);
	TestTrue(TEXT("lontano: vaga"), ValdorsoLupi::Prossimo(E::Vaga, 1.f, 4000.f, false, 0.f) == E::Vaga);
	TestTrue(TEXT("a 25 m fiuta"), ValdorsoLupi::Prossimo(E::Vaga, 1.f, 2500.f, false, 0.f) == E::Fiuta);
	TestTrue(TEXT("a 10 m accerchia"), ValdorsoLupi::Prossimo(E::Fiuta, 1.f, 1000.f, false, 0.f) == E::Accerchia);
	TestTrue(TEXT("presto: gira ancora"), ValdorsoLupi::Prossimo(E::Accerchia, 1.f, 650.f, false, 2.f) == E::Accerchia);
	TestTrue(TEXT("dopo 5 secondi morde"), ValdorsoLupi::Prossimo(E::Accerchia, 1.f, 650.f, false, 5.5f) == E::Morde);
	TestTrue(TEXT("al fuoco non si morde"), ValdorsoLupi::Prossimo(E::Accerchia, 1.f, 900.f, true, 30.f) == E::Accerchia);
	TestTrue(TEXT("corso al fuoco: si torna a girare"), ValdorsoLupi::Prossimo(E::Morde, 1.f, 300.f, true, 1.f) == E::Accerchia);
	TestTrue(TEXT("ferito fugge"), ValdorsoLupi::Prossimo(E::Morde, 0.2f, 150.f, false, 1.f) == E::Fuggi);
	TestTrue(TEXT("fugge per un po'"), ValdorsoLupi::Prossimo(E::Fuggi, 0.2f, 150.f, false, 3.f) == E::Fuggi);
	TestTrue(TEXT("poi torna a vagare"), ValdorsoLupi::Prossimo(E::Fuggi, 0.2f, 150.f, false, 11.f) == E::Vaga);
	TestTrue(TEXT("ferito, dopo la fuga, non caccia piu'"), ValdorsoLupi::Prossimo(E::Vaga, 0.2f, 1500.f, false, 1.f) == E::Vaga);
	TestTrue(TEXT("morto resta morto"), ValdorsoLupi::Prossimo(E::Morto, 1.f, 150.f, false, 1.f) == E::Morto);
	TestTrue(TEXT("senza salute muore"), ValdorsoLupi::Prossimo(E::Accerchia, 0.f, 150.f, false, 1.f) == E::Morto);
	for (int32 i = 0; i < 6; ++i)
	{
		const int32 Quanti = ValdorsoLupi::QuantiNelBranco(i);
		TestTrue(TEXT("3 o 4"), Quanti == 3 || Quanti == 4);
	}
	TestTrue(TEXT("morde piu' veloce di quanto vaga"), ValdorsoLupi::Velocita(E::Morde) > ValdorsoLupi::Velocita(E::Vaga));
	TestNotNull(TEXT("la pelle"), ValdorsoOggetti::Trova(TEXT("Pelle_Lupo")));
	TestNotNull(TEXT("le zanne"), ValdorsoOggetti::Trova(TEXT("Zanne_Lupo")));
	return true;
}


// (10/11) Gli animali: ogni specie ha la sua forma, il suo bottino nel catalogo e le sue tane in luoghi veri. Il
// cinghiale avverte e poi carica (e lascia stare chi se ne va o sta al fuoco); il cervo fugge e non attacca mai.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestAnimali, "Valdorso.Pericoli.Animali", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestAnimali::RunTest(const FString& Parameters)
{
	using E = EValdorsoStatoLupo;
	TestTrue(TEXT("sei specie"), ValdorsoAnimali::Tutte().Num() >= 6);
	for (const FValdorsoSpecie& S : ValdorsoAnimali::Tutte())
	{
		TestNotNull(S.Id, ValdorsoAnimali::Forma(S.Id));
		TestTrue(TEXT("salute"), S.Salute > 0.f);
		TestTrue(TEXT("corre piu' di quanto cammina"), S.Corsa > S.Passo);
		for (int32 i = 0; i < 3; ++i)
		{
			if (S.Bottino[i])
			{
				TestNotNull(S.Bottino[i], ValdorsoOggetti::Trova(S.Bottino[i]));
			}
		}
	}
	for (const FValdorsoTana& T : ValdorsoAnimali::Tane())
	{
		TestNotNull(T.Luogo, ValdorsoLuoghi::Trova(T.Luogo));
		TestNotNull(T.Specie, ValdorsoAnimali::Specie(T.Specie));
		for (int32 Volta = 0; Volta < 5; ++Volta)
		{
			const int32 Quanti = ValdorsoAnimali::QuantiNelGruppo(T, Volta);
			TestTrue(TEXT("quanti nel gruppo"), Quanti >= T.Minimo && Quanti <= T.Massimo);
		}
	}
	const FValdorsoSpecie* Cinghiale = ValdorsoAnimali::Specie(TEXT("Cinghiale"));
	const FValdorsoSpecie* Cervo = ValdorsoAnimali::Specie(TEXT("Cervo"));
	TestNotNull(TEXT("il cinghiale"), Cinghiale);
	TestNotNull(TEXT("il cervo"), Cervo);
	if (Cinghiale && Cervo)
	{
		TestTrue(TEXT("cinghiale: lontano vaga"), ValdorsoAnimali::Prossimo(*Cinghiale, E::Vaga, 1.f, 2000.f, false, 0.f) == E::Vaga);
		TestTrue(TEXT("cinghiale: vicino avverte"), ValdorsoAnimali::Prossimo(*Cinghiale, E::Vaga, 1.f, 600.f, false, 0.f) == E::Accerchia);
		TestTrue(TEXT("cinghiale: poi carica"), ValdorsoAnimali::Prossimo(*Cinghiale, E::Accerchia, 1.f, 600.f, false, 3.f) == E::Morde);
		TestTrue(TEXT("cinghiale: chi se ne va e' lasciato stare"), ValdorsoAnimali::Prossimo(*Cinghiale, E::Accerchia, 1.f, 1600.f, false, 1.f) == E::Vaga);
		TestTrue(TEXT("cinghiale: al fuoco no"), ValdorsoAnimali::Prossimo(*Cinghiale, E::Vaga, 1.f, 600.f, true, 0.f) == E::Vaga);
		TestTrue(TEXT("cervo: sente e fugge"), ValdorsoAnimali::Prossimo(*Cervo, E::Vaga, 1.f, 2000.f, false, 0.f) == E::Fuggi);
		TestTrue(TEXT("cervo: fugge finche' sente"), ValdorsoAnimali::Prossimo(*Cervo, E::Fuggi, 1.f, 2000.f, false, 30.f) == E::Fuggi);
		TestTrue(TEXT("cervo: lontano torna a brucare"), ValdorsoAnimali::Prossimo(*Cervo, E::Fuggi, 1.f, 5000.f, false, 30.f) == E::Vaga);
		TestTrue(TEXT("cervo: non attacca"), Cervo->Danno <= 0.f);
	}
	TestTrue(TEXT("di giorno i cervi"), ValdorsoAnimali::Fuori(EValdorsoOraTana::Giorno, 1.f, false));
	TestFalse(TEXT("di notte no"), ValdorsoAnimali::Fuori(EValdorsoOraTana::Giorno, 0.f, false));
	TestTrue(TEXT("l'orso sempre"), ValdorsoAnimali::Fuori(EValdorsoOraTana::Sempre, 0.f, false));
	FValdorsoZaino Zaino;
	ValdorsoZaino::Sistema(Zaino);
	TestFalse(TEXT("senza coltello non si scuoia"), ValdorsoAnimali::HaLama(Zaino));
	ValdorsoZaino::Aggiungi(Zaino, TEXT("Coltello"), 1, -1);
	TestTrue(TEXT("col coltello si'"), ValdorsoAnimali::HaLama(Zaino));
	return true;
}


// (10/11) Le botteghe: ogni ricetta usa oggetti che esistono, ha un Id solo suo e qualcosa da dare; cuocere toglie la
// carne cruda e da' quella cotta; comprare costa Aureus (senza non si compra); vendere li da'; lo zaino pieno ferma.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestBotteghe, "Valdorso.Pericoli.Botteghe", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestBotteghe::RunTest(const FString& Parameters)
{
	TSet<FName> Visti;
	for (const FValdorsoRicetta& R : ValdorsoBotteghe::Ricette())
	{
		TestFalse(FString::Printf(TEXT("%s: non doppia"), R.Id), Visti.Contains(FName(R.Id)));
		Visti.Add(FName(R.Id));
		// (14/11, v0.2.3) le riparazioni non danno niente di nuovo: rimettono a posto l'arma in mano
		TestTrue(FString::Printf(TEXT("%s: da' qualcosa"), R.Id), (R.Risultato && R.QuantiRisultato > 0) || R.Guadagno > 0 || R.Ripara != 0);
		if (R.Risultato)
		{
			TestNotNull(R.Risultato, ValdorsoOggetti::Trova(R.Risultato));
		}
		for (const FValdorsoIngrediente& I : R.Serve)
		{
			if (I.Id)
			{
				TestNotNull(I.Id, ValdorsoOggetti::Trova(I.Id));
			}
		}
	}
	for (uint8 b = 0; b <= static_cast<uint8>(EValdorsoTipoBanco::Carovana); ++b)
	{
		TestTrue(TEXT("ogni banco ha le sue ricette"), ValdorsoBotteghe::Del(static_cast<EValdorsoTipoBanco>(b)).Num() > 0);
	}
	FValdorsoZaino Zaino;
	ValdorsoZaino::Sistema(Zaino);
	const FValdorsoRicetta* Cuoci = ValdorsoBotteghe::Trova(TEXT("Cuoci_Carne"));
	const FValdorsoRicetta* Compra = ValdorsoBotteghe::Trova(TEXT("Compra_Spada_Corta"));
	const FValdorsoRicetta* Vendi = ValdorsoBotteghe::Trova(TEXT("Vendi_Pelle_Lupo"));
	TestNotNull(TEXT("cuocere"), Cuoci);
	TestNotNull(TEXT("comprare"), Compra);
	TestNotNull(TEXT("vendere"), Vendi);
	if (Cuoci && Compra && Vendi)
	{
		TestFalse(TEXT("senza carne non si cuoce"), ValdorsoBotteghe::Fai(Zaino, *Cuoci));
		ValdorsoZaino::Aggiungi(Zaino, TEXT("Carne_Cruda"), 2, -1);
		TestTrue(TEXT("si cuoce"), ValdorsoBotteghe::Fai(Zaino, *Cuoci));
		TestEqual(TEXT("una cruda in meno"), ValdorsoZaino::Conta(Zaino, TEXT("Carne_Cruda")), 1);
		TestEqual(TEXT("una cotta"), ValdorsoZaino::Conta(Zaino, TEXT("Carne_Cotta")), 1);
		Zaino.Monete = 10;
		TestFalse(TEXT("10 Aureus non bastano"), ValdorsoBotteghe::Manca(Zaino, *Compra).IsEmpty());
		TestFalse(TEXT("e non si compra"), ValdorsoBotteghe::Fai(Zaino, *Compra));
		TestEqual(TEXT("gli Aureus restano"), Zaino.Monete, 10);
		Zaino.Monete = 40;
		TestTrue(TEXT("con 40 si'"), ValdorsoBotteghe::Fai(Zaino, *Compra));
		TestEqual(TEXT("ne restano 5"), Zaino.Monete, 5);
		TestEqual(TEXT("la spada c'e'"), ValdorsoZaino::Conta(Zaino, TEXT("Spada_Corta")), 1);
		ValdorsoZaino::Aggiungi(Zaino, TEXT("Pelle_Lupo"), 1, -1);
		TestTrue(TEXT("si vende"), ValdorsoBotteghe::Fai(Zaino, *Vendi));
		TestEqual(TEXT("8 Aureus in piu'"), Zaino.Monete, 13);
		TestEqual(TEXT("la pelle non c'e' piu'"), ValdorsoZaino::Conta(Zaino, TEXT("Pelle_Lupo")), 0);
		// lo zaino pieno: 24 posti occupati da cose che non si impilano
		FValdorsoZaino Pieno;
		ValdorsoZaino::Sistema(Pieno);
		ValdorsoZaino::Aggiungi(Pieno, TEXT("Scudo_Legno"), ValdorsoZaino::NumeroPosti, -1);
		Pieno.Monete = 100;
		TestFalse(TEXT("zaino pieno: non si compra"), ValdorsoBotteghe::Fai(Pieno, *Compra));
		TestEqual(TEXT("e non si paga"), Pieno.Monete, 100);
	}
	return true;
}

// (10/11) Sott'acqua: il fiato dura mezzo minuto e torna in quattro secondi; la corrente dei fiumi va dove l'acqua scende.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestFiato, "Valdorso.Pericoli.Fiato", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestFiato::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("fuori dall'acqua resta pieno"), ValdorsoBagnato::PassoFiato(1.f, 10.f, false), 1.f);
	TestEqual(TEXT("a meta' dopo quindici secondi"), ValdorsoBagnato::PassoFiato(1.f, ValdorsoBagnato::SecondiFiato * 0.5f, true), 0.5f, 0.001f);
	TestEqual(TEXT("finito dopo mezzo minuto"), ValdorsoBagnato::PassoFiato(1.f, ValdorsoBagnato::SecondiFiato + 1.f, true), 0.f);
	TestEqual(TEXT("si riprende in pochi secondi"), ValdorsoBagnato::PassoFiato(0.f, ValdorsoBagnato::SecondiRespiro, false), 1.f);
	TestTrue(TEXT("si trattiene piu' a lungo di quanto si riprende"), ValdorsoBagnato::SecondiFiato > 3.f * ValdorsoBagnato::SecondiRespiro);

	// un fiume dritto lungo X che scende verso +X, largo 10 m
	FValdorsoCorsoAcqua Fiume;
	Fiume.Punti = { FVector(0.f, 0.f, 500.f), FVector(1000.f, 0.f, 480.f), FVector(2000.f, 0.f, 460.f) };
	Fiume.Larghezze = { 1000.f, 1000.f, 1000.f };
	FVector2D Verso;
	TestTrue(TEXT("in mezzo al fiume c'e' corrente"), ValdorsoAcqua::CorrenteSulCorso(Fiume, FVector2D(500.f, 100.f), 0.f, Verso));
	TestTrue(TEXT("va dove l'acqua scende"), Verso.X > 0.99f);
	TestFalse(TEXT("fuori dal fiume no"), ValdorsoAcqua::CorrenteSulCorso(Fiume, FVector2D(500.f, 900.f), 0.f, Verso));
	// lo stesso fiume scritto al contrario: la corrente va sempre verso valle
	FValdorsoCorsoAcqua Rovescio = Fiume;
	Algo::Reverse(Rovescio.Punti);
	TestTrue(TEXT("al contrario c'e' corrente"), ValdorsoAcqua::CorrenteSulCorso(Rovescio, FVector2D(1500.f, 0.f), 0.f, Verso));
	TestTrue(TEXT("e va ancora verso valle"), Verso.X > 0.99f);
	return true;
}

// (10/11) Il cielo e i boschi che vivono: le ore di stormi, rondini e pipistrelli, le foglie d'autunno, le celle degli
// alberi; e i saluti dei PNG a chi passa (ognuno ne ha per il giorno, la sera e la notte, corti per la chat).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestVitaValle, "Valdorso.Pericoli.VitaValle", ValdorsoTestPericoli::Bandiere)
bool FValdorsoTestVitaValle::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("a mezzogiorno gli stormi"), ValdorsoVitaValle::OraDegliStormi(12.f));
	TestFalse(TEXT("di notte no"), ValdorsoVitaValle::OraDegliStormi(23.f));
	TestTrue(TEXT("al tramonto le rondini"), ValdorsoVitaValle::OraDelleRondini(18.5f));
	TestFalse(TEXT("a mezzogiorno no"), ValdorsoVitaValle::OraDelleRondini(12.f));
	TestTrue(TEXT("di notte i pipistrelli"), ValdorsoVitaValle::OraDeiPipistrelli(23.f, 0.05f));
	TestFalse(TEXT("di giorno no"), ValdorsoVitaValle::OraDeiPipistrelli(12.f, 1.f));
	TestTrue(TEXT("d'autunno le foglie"), ValdorsoVitaValle::StagioneDelleFoglie(TEXT("autunno")));
	TestFalse(TEXT("d'estate no"), ValdorsoVitaValle::StagioneDelleFoglie(TEXT("estate")));
	TestTrue(TEXT("le celle di 10 m"), ValdorsoVitaValle::Cella(FVector(1500.f, -1.f, 0.f)) == FIntPoint(1, -1));
	TestTrue(TEXT("in primavera i petali"), ValdorsoVitaValle::StagioneDeiPetali(TEXT("primavera")));
	TestFalse(TEXT("d'autunno niente petali"), ValdorsoVitaValle::StagioneDeiPetali(TEXT("autunno")));
	TestTrue(TEXT("d'estate a mezzogiorno le farfalle"), ValdorsoVitaValle::OraDelleFarfalle(12.f, TEXT("estate")));
	TestFalse(TEXT("d'inverno niente farfalle"), ValdorsoVitaValle::OraDelleFarfalle(12.f, TEXT("inverno")));
	TestTrue(TEXT("d'estate di notte le lucciole"), ValdorsoVitaValle::OraDelleLucciole(23.f, 0.05f, TEXT("estate")));
	TestFalse(TEXT("d'autunno niente lucciole"), ValdorsoVitaValle::OraDelleLucciole(23.f, 0.05f, TEXT("autunno")));
	TestTrue(TEXT("la sera i camini fumano tanto"), ValdorsoVitaValle::QuantoFumo(19.f, TEXT("estate")) >= 0.99f);
	TestTrue(TEXT("d'inverno fumano piu' che d'estate"), ValdorsoVitaValle::QuantoFumo(14.f, TEXT("inverno")) > ValdorsoVitaValle::QuantoFumo(14.f, TEXT("estate")));
	int32 NottiDelDrago = 0;
	for (int32 g = 0; g < 60; ++g)
	{
		NottiDelDrago += ValdorsoVitaValle::NotteDelDrago(FDateTime(2026, 11, 1) + FTimespan::FromDays(g)) ? 1 : 0;
	}
	TestTrue(TEXT("il drago passa ogni tanto (non mai, non sempre)"), NottiDelDrago >= 5 && NottiDelDrago <= 30);
	TestTrue(TEXT("il drago al calare della sera"), ValdorsoVitaValle::OraDelDrago(20.2f));
	TestFalse(TEXT("non a mezzogiorno"), ValdorsoVitaValle::OraDelDrago(12.f));
	const FString StagioneVera = ValdorsoVitaValle::StagioneAdesso();
	TestTrue(TEXT("la stagione e' una delle quattro"), StagioneVera == TEXT("primavera") || StagioneVera == TEXT("estate") || StagioneVera == TEXT("autunno") || StagioneVera == TEXT("inverno"));

	for (const FValdorsoDialogo& Dialogo : ValdorsoDialoghi::Tutti())
	{
		const FString Nome = Dialogo.Id.ToString();
		for (const float Ora : { 10.f, 19.f, 2.f })
		{
			TestFalse(FString::Printf(TEXT("%s saluta alle %.0f"), *Nome, Ora), ValdorsoDialoghi::Saluto(Dialogo.Id, Ora, 7).IsEmpty());
		}
		for (const FString& Riga : ValdorsoDialoghi::TuttiISaluti(Dialogo.Id))
		{
			TestTrue(FString::Printf(TEXT("%s: il saluto sta nella chat"), *Nome), Riga.Len() > 5 && Riga.Len() <= 200);
		}
	}
	TestTrue(TEXT("chi non c'e' non saluta"), ValdorsoDialoghi::Saluto(TEXT("NessunoQui"), 10.f, 1).IsEmpty());
	return true;
}

#endif
