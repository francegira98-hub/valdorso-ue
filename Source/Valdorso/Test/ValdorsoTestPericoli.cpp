// Valdorso - Test automatici dei pericoli (v0.1.4, 08/11): barre, rinascita al tempio, equilibrio, chat di prossimità.
// Scritto da Claude il 08/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.Pericoli").

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

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

#endif
