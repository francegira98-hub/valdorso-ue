// Valdorso - Test automatici di Crescere (scheletro della v0.1.5): le abilita' che salgono, le soglie, il tetto, i vantaggi.
// Scritto da Claude il 10/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.Crescere").

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoAbilitaDati.h"
#include "ValdorsoRegistro.h"

namespace ValdorsoTestCrescere
{
	constexpr EAutomationTestFlags BandiereCrescere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestCrescereCatalogo, "Valdorso.Crescere.Catalogo", ValdorsoTestCrescere::BandiereCrescere)
bool FValdorsoTestCrescereCatalogo::RunTest(const FString& Parameters)
{
	TSet<FName> Viste;
	for (const FValdorsoAbilitaTipo& T : ValdorsoCrescere::Tutte())
	{
		TestFalse(*FString::Printf(TEXT("%s: una volta sola"), *T.Id.ToString()), Viste.Contains(T.Id));
		Viste.Add(T.Id);
		for (int32 i = 0; i < 5; ++i)
		{
			TestTrue(*FString::Printf(TEXT("%s: il nome %d"), *T.Id.ToString(), i), FCString::Strlen(T.Nomi[i]) > 0);
		}
		TestTrue(*FString::Printf(TEXT("%s: la descrizione"), *T.Id.ToString()), FCString::Strlen(T.Descrizione) > 0);
	}
	// ogni arma della mano destra ha la sua abilita', e a mani nude la Lotta
	TestTrue(TEXT("a mani nude: la lotta"), ValdorsoCrescere::AbilitaDellArma(NAME_None) == FName(TEXT("Lotta")));
	TestTrue(TEXT("la spada lunga: le spade"), ValdorsoCrescere::AbilitaDellArma(FName(TEXT("Spada_Lunga"))) == FName(TEXT("Spade")));
	TestTrue(TEXT("la lancia: le aste"), ValdorsoCrescere::AbilitaDellArma(FName(TEXT("Lancia"))) == FName(TEXT("Aste")));
	const TCHAR* Mestieri[] = { TEXT("contadino"), TEXT("pastore"), TEXT("apprendista"), TEXT("soldato"), TEXT("servo"), TEXT("allievo"),
		TEXT("orfano"), TEXT("mercante"), TEXT("cacciatore"), TEXT("marinaio") };
	for (const TCHAR* M : Mestieri)
	{
		const FName Id = ValdorsoCrescere::AbilitaDelMestiere(M);
		TestTrue(*FString::Printf(TEXT("il mestiere %s ha un'abilita' del catalogo"), M), ValdorsoCrescere::Trova(Id) != nullptr);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestCrescereSoglie, "Valdorso.Crescere.Soglie", ValdorsoTestCrescere::BandiereCrescere)
bool FValdorsoTestCrescereSoglie::RunTest(const FString& Parameters)
{
	const FName Atletica(TEXT("Atletica"));
	FValdorsoAbilitaColono C;
	const FValdorsoAbilitaColono& Letto = C;
	// sale usandola, e piu' in alto sale piu' piano
	const FValdorsoEsitoUso Primo = ValdorsoCrescere::Usa(C, Atletica, 1.f, 0, false);
	TestTrue(TEXT("il primo uso fa salire"), Primo.Guadagno > 0.f);
	TestTrue(TEXT("piu' in alto si sale piu' piano"), ValdorsoCrescere::Guadagno(80.f, 1.f, 0) < ValdorsoCrescere::Guadagno(10.f, 1.f, 0));
	TestTrue(TEXT("la stessa cosa di fila rende meno"), ValdorsoCrescere::Guadagno(10.f, 1.f, 5) < ValdorsoCrescere::Guadagno(10.f, 1.f, 0));
	TestTrue(TEXT("un manichino rende meno di un lupo"), ValdorsoCrescere::Guadagno(10.f, 0.25f, 0) < ValdorsoCrescere::Guadagno(10.f, 1.f, 0));
	// alla soglia si ferma
	for (int32 i = 0; i < 2000; ++i)
	{
		ValdorsoCrescere::Usa(C, Atletica, 1.f, 0, false);
	}
	TestEqual(TEXT("ferma a 25"), ValdorsoCrescere::Punti(C, Atletica), 25.f);
	const FValdorsoAbilitaValore* V = ValdorsoCrescere::Valore(Letto, Atletica);
	TestTrue(TEXT("carica"), V && V->Carica >= 1.f);
	TestTrue(TEXT("non ancora evoluta (nessun momento critico)"), V && V->Evoluzioni == 0);
	// nel momento critico si evolve, e il nome cambia
	const FValdorsoEsitoUso Critico = ValdorsoCrescere::Usa(C, Atletica, 1.f, 0, true);
	TestTrue(TEXT("si evolve nel momento critico"), Critico.bEvoluta && Critico.Evoluzione == 1);
	TestTrue(TEXT("il nome nuovo"), ValdorsoCrescere::Nome(Atletica, 1) != ValdorsoCrescere::Nome(Atletica, 0));
	TestFalse(TEXT("l'annuncio"), ValdorsoCrescere::Annuncio(Atletica, 1).IsEmpty());
	// e poi riprende a salire
	TestTrue(TEXT("dopo l'evoluzione si sale ancora"), ValdorsoCrescere::Usa(C, Atletica, 1.f, 0, false).Guadagno > 0.f);
	// ferma: non sale
	ValdorsoCrescere::ImpostaVerso(C, Atletica, EValdorsoVerso::Ferma);
	TestEqual(TEXT("ferma non sale"), ValdorsoCrescere::Usa(C, Atletica, 1.f, 0, false).Guadagno, 0.f);
	// quelle in arrivo non salgono ancora usandole
	TestEqual(TEXT("la pesca non sale ancora"), ValdorsoCrescere::Usa(C, FName(TEXT("Pesca")), 1.f, 0, false).Guadagno, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestCrescereTetto, "Valdorso.Crescere.Tetto", ValdorsoTestCrescere::BandiereCrescere)
bool FValdorsoTestCrescereTetto::RunTest(const FString& Parameters)
{
	FValdorsoAbilitaColono C;
	const FValdorsoAbilitaColono& Letto = C;
	// sette abilita' a 100: il tetto e' pieno
	const TCHAR* Piene[] = { TEXT("Atletica"), TEXT("Nuoto"), TEXT("Tempra"), TEXT("Lotta"), TEXT("Pugnali"), TEXT("Spade"), TEXT("Asce") };
	for (const TCHAR* Id : Piene)
	{
		ValdorsoCrescere::ImpostaPunti(C, FName(Id), 100.f);
	}
	TestEqual(TEXT("700 in tutto"), ValdorsoCrescere::Totale(C), ValdorsoCrescere::Tetto);
	const FName Mazze(TEXT("Mazze"));
	const FValdorsoEsitoUso Pieno = ValdorsoCrescere::Usa(C, Mazze, 1.f, 0, false);
	TestTrue(TEXT("al tetto non si sale"), Pieno.bTettoPieno && Pieno.Guadagno == 0.f);
	// una messa a calare fa spazio
	ValdorsoCrescere::ImpostaVerso(C, FName(TEXT("Asce")), EValdorsoVerso::Scende);
	const FValdorsoEsitoUso Spazio = ValdorsoCrescere::Usa(C, Mazze, 1.f, 0, false);
	TestTrue(TEXT("cala l'ascia e sale la mazza"), Spazio.Guadagno > 0.f && Spazio.Calata == FName(TEXT("Asce")));
	TestTrue(TEXT("il totale resta al tetto"), ValdorsoCrescere::Totale(C) <= ValdorsoCrescere::Tetto + KINDA_SMALL_NUMBER);
	TestTrue(TEXT("calando perde l'evoluzione"), ValdorsoCrescere::Valore(Letto, FName(TEXT("Asce")))->Evoluzioni == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestCrescereVantaggi, "Valdorso.Crescere.Vantaggi", ValdorsoTestCrescere::BandiereCrescere)
bool FValdorsoTestCrescereVantaggi::RunTest(const FString& Parameters)
{
	FValdorsoRegistro Registro;
	Registro.bFirmato = true;
	Registro.Mestiere = TEXT("marinaio");
	Registro.RichiamoMagia = TEXT("si");
	FValdorsoAbilitaColono C;
	const FValdorsoAbilitaColono& Letto = C;
	TestTrue(TEXT("i vantaggi"), ValdorsoCrescere::DaiVantaggi(C, Registro));
	TestEqual(TEXT("5 nella pesca"), ValdorsoCrescere::Punti(C, FName(TEXT("Pesca"))), ValdorsoCrescere::PuntiVantaggio);
	TestEqual(TEXT("5 nella concentrazione"), ValdorsoCrescere::Punti(C, FName(TEXT("Concentrazione"))), ValdorsoCrescere::PuntiVantaggio);
	TestFalse(TEXT("una volta sola"), ValdorsoCrescere::DaiVantaggi(C, Registro));
	TestEqual(TEXT("sempre 5"), ValdorsoCrescere::Punti(C, FName(TEXT("Pesca"))), ValdorsoCrescere::PuntiVantaggio);
	// il file rovinato si rimette in ordine
	FValdorsoAbilitaValore& Strana = C.Valori.AddDefaulted_GetRef();
	Strana.Id = FName(TEXT("NonEsiste"));
	FValdorsoAbilitaValore& Troppo = C.Valori.AddDefaulted_GetRef();
	Troppo.Id = FName(TEXT("Nuoto"));
	Troppo.Punti = 60.f;
	Troppo.Evoluzioni = 0;
	ValdorsoCrescere::Sistema(C);
	TestTrue(TEXT("via quella che non esiste"), ValdorsoCrescere::Valore(Letto, FName(TEXT("NonEsiste"))) == nullptr);
	TestEqual(TEXT("non si salta una soglia"), ValdorsoCrescere::Punti(C, FName(TEXT("Nuoto"))), 25.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestCrescereForza, "Valdorso.Crescere.Forza", ValdorsoTestCrescere::BandiereCrescere)
bool FValdorsoTestCrescereForza::RunTest(const FString& Parameters)
{
	FValdorsoAbilitaColono C;
	TestEqual(TEXT("un colono nuovo non e' forte"), ValdorsoCrescere::Forza(C), 0.f);
	ValdorsoCrescere::ImpostaPunti(C, FName(TEXT("Spade")), 50.f);
	ValdorsoCrescere::ImpostaPunti(C, FName(TEXT("Tempra")), 50.f);
	TestEqual(TEXT("spade 50 e tempra 50: forza 50"), ValdorsoCrescere::Forza(C), 50.f);
	ValdorsoCrescere::ImpostaPunti(C, FName(TEXT("Scudo")), 100.f);
	TestEqual(TEXT("lo scudo non e' un'arma per la forza"), ValdorsoCrescere::Forza(C), 50.f);
	TestTrue(TEXT("l'orso e' piu' forte del lupo"), ValdorsoCrescere::ForzaSpecie(FName(TEXT("Orso"))) > ValdorsoCrescere::ForzaSpecie(FName(TEXT("Lupo"))));
	TestTrue(TEXT("le guardie del passo sono molto forti"), ValdorsoCrescere::ForzaAbitante(FName(TEXT("GuardiaCorrado"))) >= 90.f);
	FLinearColor Colore;
	TestFalse(TEXT("un confronto ha una frase"), ValdorsoCrescere::Confronto(0.f, 60.f, Colore).IsEmpty());
	TestTrue(TEXT("piu' forte: rosso"), Colore.R > Colore.G);
	ValdorsoCrescere::Confronto(60.f, 0.f, Colore);
	TestTrue(TEXT("molto piu' debole: grigio"), FMath::IsNearlyEqual(Colore.R, Colore.G));
	TestTrue(TEXT("contro i piu' forti si cresce di piu'"), ValdorsoCrescere::DifficoltaContro(10.f, 40.f) > ValdorsoCrescere::DifficoltaContro(40.f, 10.f));
	TestTrue(TEXT("un orso per un colono nuovo e' un pericolo vero"), ValdorsoCrescere::PericoloVero(0.f, 60.f));
	TestFalse(TEXT("una lepre no"), ValdorsoCrescere::PericoloVero(0.f, 0.f));
	return true;
}

#endif
