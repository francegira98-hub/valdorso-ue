// Valdorso - Test automatici dello zaino e dei bagnati (v0.1.4, seconda parte, 08/11): il catalogo degli oggetti,
// le regole dello zaino (aggiungere, impilare, spostare, il peso, la barra), quello che il colono porta dall'arrivo,
// il salvataggio nel file del personaggio, i doni dei PNG, bagnati e asciutti; (08/11) i falò e le cose da raccogliere.
// Scritto da Claude il 08/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.Zaino").

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoArchivista.h"
#include "ValdorsoBagnato.h"
#include "ValdorsoClima.h"
#include "ValdorsoFalo.h"
#include "ValdorsoRaccolta.h"
#include "ValdorsoTaccuinoDati.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoRegistro.h"
#include "ValdorsoTessera.h"
#include "ValdorsoZainoDati.h"
#include "JsonObjectConverter.h"

namespace ValdorsoTestZaino
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	FValdorsoZaino Vuoto()
	{
		FValdorsoZaino Zaino;
		ValdorsoZaino::Sistema(Zaino);
		return Zaino;
	}
}

// Il catalogo: ogni oggetto ha un nome, una descrizione, un peso sensato, una pila e un disegno; niente doppioni.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoCatalogo, "Valdorso.Zaino.Catalogo", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoCatalogo::RunTest(const FString& Parameters)
{
	TSet<FName> Visti;
	for (const FValdorsoTipoOggetto& Tipo : ValdorsoOggetti::Catalogo())
	{
		const FString Nome = Tipo.Id.ToString();
		TestFalse(FString::Printf(TEXT("%s: un Id"), *Nome), Tipo.Id.IsNone());
		TestFalse(FString::Printf(TEXT("%s: non doppio"), *Nome), Visti.Contains(Tipo.Id));
		Visti.Add(Tipo.Id);
		TestTrue(FString::Printf(TEXT("%s: un nome"), *Nome), FCString::Strlen(Tipo.Nome) > 0);
		TestTrue(FString::Printf(TEXT("%s: una descrizione"), *Nome), FCString::Strlen(Tipo.Descrizione) > 10);
		TestTrue(FString::Printf(TEXT("%s: un peso sensato"), *Nome), Tipo.Peso > 0.f && Tipo.Peso < 20.f);
		TestTrue(FString::Printf(TEXT("%s: una pila"), *Nome), Tipo.Pila >= 1 && Tipo.Pila <= 99);
		TestTrue(FString::Printf(TEXT("%s: un disegno"), *Nome), ValdorsoOggetti::Disegno(Tipo.Id).Num() > 0);
		for (const TArray<FVector2D>& Linea : ValdorsoOggetti::Disegno(Tipo.Id))
		{
			TestTrue(FString::Printf(TEXT("%s: ogni linea ha due punti"), *Nome), Linea.Num() >= 2);
			for (const FVector2D& P : Linea)
			{
				TestTrue(FString::Printf(TEXT("%s: il disegno sta nel riquadro"), *Nome), FMath::Abs(P.X) <= 1.001 && FMath::Abs(P.Y) <= 1.001);
			}
		}
		if (Tipo.Uso == EValdorsoUso::Leggi)
		{
			TestTrue(FString::Printf(TEXT("%s: c'è qualcosa da leggere"), *Nome), FCString::Strlen(Tipo.Lettura) > 0);
		}
		if (Tipo.Uso == EValdorsoUso::Bevi)
		{
			TestTrue(FString::Printf(TEXT("%s: ha dei sorsi"), *Nome), Tipo.Cariche > 0);
		}
	}
	TestNull(TEXT("un oggetto che non c'è"), ValdorsoOggetti::Trova(TEXT("NessunOggetto")));
	TestEqual(TEXT("niente disegno per chi non c'è"), ValdorsoOggetti::Disegno(TEXT("NessunOggetto")).Num(), 0);
	TestTrue(TEXT("la moneta ha un disegno"), ValdorsoOggetti::DisegnoMoneta().Num() > 0);
	TestEqual(TEXT("il peso all'italiana"), ValdorsoOggetti::TestoPeso(2.45f), FString(TEXT("2,5 kg")));
	return true;
}

// Ogni risposta del Registro che porta un oggetto ha il suo oggetto nel catalogo (mestieri e ricordi).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoRegistro, "Valdorso.Zaino.Registro", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoRegistro::RunTest(const FString& Parameters)
{
	for (const FValdorsoVoceRegistro& Voce : ValdorsoRegistro::Voci(ValdorsoRegistro::EDomanda::Mestiere))
	{
		const FName Attrezzo = ValdorsoOggetti::AttrezzoDelMestiere(Voce.Chiave);
		TestNotNull(FString::Printf(TEXT("il mestiere %s ha il suo attrezzo"), Voce.Chiave), ValdorsoOggetti::Trova(Attrezzo));
	}
	for (const FValdorsoVoceRegistro& Voce : ValdorsoRegistro::Voci(ValdorsoRegistro::EDomanda::Ricordo))
	{
		const FValdorsoTipoOggetto* Ricordo = ValdorsoOggetti::Trova(ValdorsoOggetti::OggettoDelRicordo(Voce.Chiave));
		if (TestNotNull(FString::Printf(TEXT("il ricordo %s è nel catalogo"), Voce.Chiave), Ricordo))
		{
			TestTrue(FString::Printf(TEXT("il ricordo %s non si getta"), Voce.Chiave), Ricordo->bRicordo);
		}
	}

	// un colono vero: contadino, con l'anello, venuto per un debito
	FValdorsoRegistro Registro;
	Registro.Mestiere = TEXT("contadino");
	Registro.Ricordo = TEXT("anello");
	Registro.Motivo = TEXT("debito");
	FValdorsoZaino Zaino;
	ValdorsoZaino::DaiPartenza(Zaino, &Registro);
	TestTrue(TEXT("partenza data"), Zaino.bPartenzaData);
	TestEqual(TEXT("la tessera"), ValdorsoZaino::Conta(Zaino, TEXT("Tessera")), 1);
	TestEqual(TEXT("tre pani"), ValdorsoZaino::Conta(Zaino, TEXT("Pane_Nero")), 3);
	TestEqual(TEXT("la borraccia"), ValdorsoZaino::Conta(Zaino, TEXT("Borraccia")), 1);
	TestEqual(TEXT("il falcetto del contadino"), ValdorsoZaino::Conta(Zaino, TEXT("Falcetto")), 1);
	TestEqual(TEXT("l'anello"), ValdorsoZaino::Conta(Zaino, TEXT("Ricordo_Anello")), 1);
	TestEqual(TEXT("più Aureus per il debito"), Zaino.Monete, ValdorsoOggetti::MoneteDebito);
	TestEqual(TEXT("il tasto 1 è il pane"), Zaino.Azioni[0], FName(TEXT("Pane_Nero")));
	TestEqual(TEXT("il tasto 2 è la borraccia"), Zaino.Azioni[1], FName(TEXT("Borraccia")));
	const int32 Borraccia = ValdorsoZaino::PostoDi(Zaino, TEXT("Borraccia"));
	if (TestTrue(TEXT("la borraccia ha un posto"), Borraccia != INDEX_NONE))
	{
		TestEqual(TEXT("la borraccia parte piena"), Zaino.Posti[Borraccia].Cariche, ValdorsoOggetti::Trova(TEXT("Borraccia"))->Cariche);
	}
	TestTrue(TEXT("il peso dell'arrivo si porta bene"), ValdorsoOggetti::PuoCorrere(ValdorsoZaino::Peso(Zaino)));

	// un colono qualunque (l'accesso libero): le cose di tutti, gli Aureus di tutti
	FValdorsoZaino Qualunque;
	ValdorsoZaino::DaiPartenza(Qualunque, nullptr);
	TestEqual(TEXT("un colono qualunque: gli Aureus di tutti"), Qualunque.Monete, ValdorsoOggetti::MoneteIniziali);
	TestEqual(TEXT("un colono qualunque: niente ricordo"), ValdorsoZaino::Conta(Qualunque, TEXT("Ricordo_Anello")), 0);
	return true;
}

// Le regole: impilare, riempire, togliere, spostare (nel vuoto, sulla pila, scambiando), gettare, la barra.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoRegole, "Valdorso.Zaino.Regole", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoRegole::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestZaino;
	FValdorsoZaino Zaino = Vuoto();
	TestEqual(TEXT("24 posti"), Zaino.Posti.Num(), ValdorsoZaino::NumeroPosti);
	TestEqual(TEXT("8 tasti"), Zaino.Azioni.Num(), ValdorsoZaino::NumeroAzioni);
	TestEqual(TEXT("un oggetto che non c'è non entra"), ValdorsoZaino::Aggiungi(Zaino, TEXT("NessunOggetto"), 3), 0);

	// 14 pani: una pila da 10 e una da 4
	TestEqual(TEXT("14 pani entrano"), ValdorsoZaino::Aggiungi(Zaino, TEXT("Pane_Nero"), 14), 14);
	TestEqual(TEXT("prima pila piena"), Zaino.Posti[0].Quantita, 10);
	TestEqual(TEXT("seconda pila"), Zaino.Posti[1].Quantita, 4);
	TestEqual(TEXT("altri 3 vanno sulla seconda"), ValdorsoZaino::Aggiungi(Zaino, TEXT("Pane_Nero"), 3), 3);
	TestEqual(TEXT("seconda pila da 7"), Zaino.Posti[1].Quantita, 7);
	TestEqual(TEXT("17 in tutto"), ValdorsoZaino::Conta(Zaino, TEXT("Pane_Nero")), 17);

	// due borracce non si impilano
	ValdorsoZaino::Aggiungi(Zaino, TEXT("Borraccia"), 2);
	TestEqual(TEXT("due borracce, due posti"), Zaino.Posti[2].Quantita + Zaino.Posti[3].Quantita, 2);
	TestEqual(TEXT("piene"), Zaino.Posti[2].Cariche, 5);

	// togliere
	TestTrue(TEXT("togli un pane"), ValdorsoZaino::Togli(Zaino, 1, 1));
	TestEqual(TEXT("ne restano 6 lì"), Zaino.Posti[1].Quantita, 6);
	TestFalse(TEXT("non se ne tolgono più di quanti ce ne sono"), ValdorsoZaino::Togli(Zaino, 1, 7));
	TestTrue(TEXT("tutti e sei"), ValdorsoZaino::Togli(Zaino, 1, 6));
	TestTrue(TEXT("il posto è libero"), Zaino.Posti[1].Vuoto());

	// spostare: nel vuoto, sulla stessa pila, scambiando
	TestTrue(TEXT("nel posto libero"), ValdorsoZaino::Sposta(Zaino, 0, 10));
	TestTrue(TEXT("il posto di prima è libero"), Zaino.Posti[0].Vuoto());
	TestEqual(TEXT("e i pani sono lì"), Zaino.Posti[10].Quantita, 10);
	ValdorsoZaino::Aggiungi(Zaino, TEXT("Mela"), 1);  // va nel primo posto libero (lo 0)
	TestEqual(TEXT("la mela nel primo posto libero"), Zaino.Posti[0].Id, FName(TEXT("Mela")));
	TestTrue(TEXT("scambio pane e mela"), ValdorsoZaino::Sposta(Zaino, 0, 10));
	TestEqual(TEXT("ora il pane è nello 0"), Zaino.Posti[0].Id, FName(TEXT("Pane_Nero")));
	TestEqual(TEXT("e la mela nel 10"), Zaino.Posti[10].Id, FName(TEXT("Mela")));
	ValdorsoZaino::Aggiungi(Zaino, TEXT("Mela"), 15);  // 9 sulla pila del 10, poi 6 nel primo posto libero (l'1)
	TestEqual(TEXT("la pila del 10 è piena"), Zaino.Posti[10].Quantita, 10);
	TestEqual(TEXT("le altre nel primo posto libero"), Zaino.Posti[1].Quantita, 6);
	TestFalse(TEXT("su una pila piena non cambia niente"), ValdorsoZaino::Sposta(Zaino, 1, 10));
	ValdorsoZaino::Togli(Zaino, 10, 4);
	TestTrue(TEXT("con posto, si unisce"), ValdorsoZaino::Sposta(Zaino, 1, 10));
	TestEqual(TEXT("10 nel 10"), Zaino.Posti[10].Quantita, 10);
	TestEqual(TEXT("e il resto resta nell'1"), Zaino.Posti[1].Quantita, 2);
	TestFalse(TEXT("un posto vuoto non si sposta"), ValdorsoZaino::Sposta(Zaino, 20, 21));
	TestFalse(TEXT("fuori dalla griglia no"), ValdorsoZaino::Sposta(Zaino, 0, 99));

	// gettare: i ricordi no
	const int32 Anello = ValdorsoZaino::Aggiungi(Zaino, TEXT("Ricordo_Anello"), 1) == 1 ? ValdorsoZaino::PostoDi(Zaino, TEXT("Ricordo_Anello")) : INDEX_NONE;
	TestFalse(TEXT("l'anello non si getta"), ValdorsoZaino::SiPuoGettare(Zaino, Anello));
	TestTrue(TEXT("il pane sì"), ValdorsoZaino::SiPuoGettare(Zaino, 0));

	// la barra: solo quello che si usa, e un oggetto in un tasto solo
	TestFalse(TEXT("l'anello non va nella barra"), ValdorsoZaino::Assegna(Zaino, 0, TEXT("Ricordo_Anello")));
	TestTrue(TEXT("il pane nel tasto 3"), ValdorsoZaino::Assegna(Zaino, 2, TEXT("Pane_Nero")));
	TestTrue(TEXT("il pane nel tasto 5"), ValdorsoZaino::Assegna(Zaino, 4, TEXT("Pane_Nero")));
	TestTrue(TEXT("e il tasto 3 si vuota"), Zaino.Azioni[2].IsNone());
	TestFalse(TEXT("fuori dalla barra no"), ValdorsoZaino::Assegna(Zaino, 8, TEXT("Pane_Nero")));
	TestTrue(TEXT("vuotare un tasto"), ValdorsoZaino::Assegna(Zaino, 4, NAME_None));

	// la borraccia: la barra prende quella con dei sorsi
	Zaino.Posti[2].Cariche = 0;
	TestEqual(TEXT("la borraccia con i sorsi"), ValdorsoZaino::PostoDi(Zaino, TEXT("Borraccia")), 3);
	TestTrue(TEXT("si riempie"), ValdorsoZaino::RiempiBorracce(Zaino));
	TestFalse(TEXT("piene: niente da riempire"), ValdorsoZaino::RiempiBorracce(Zaino));

	// pieno: quello che non entra resta fuori
	FValdorsoZaino Pieno = Vuoto();
	TestEqual(TEXT("24 coltelli entrano"), ValdorsoZaino::Aggiungi(Pieno, TEXT("Coltello"), 30), ValdorsoZaino::NumeroPosti);
	return true;
}

// Il peso: monete e oggetti; oltre 30 kg non si corre, oltre 40 si cammina piano.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoPeso, "Valdorso.Zaino.Peso", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoPeso::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestZaino;
	FValdorsoZaino Zaino = Vuoto();
	TestEqual(TEXT("vuoto non pesa"), ValdorsoZaino::Peso(Zaino), 0.f);
	Zaino.Monete = 100;
	TestTrue(TEXT("cento Aureus, un chilo"), FMath::IsNearlyEqual(ValdorsoZaino::Peso(Zaino), 1.f, 0.001f));
	ValdorsoZaino::Aggiungi(Zaino, TEXT("Pane_Nero"), 10);
	TestTrue(TEXT("dieci pani, tre chili in più"), FMath::IsNearlyEqual(ValdorsoZaino::Peso(Zaino), 4.f, 0.001f));
	TestTrue(TEXT("si corre"), ValdorsoOggetti::PuoCorrere(25.f));
	TestFalse(TEXT("oltre la portata no"), ValdorsoOggetti::PuoCorrere(31.f));
	TestEqual(TEXT("si cammina normale"), ValdorsoOggetti::FattoreVelocita(35.f), 1.f);
	TestTrue(TEXT("sovraccarichi, piano"), ValdorsoOggetti::FattoreVelocita(41.f) < 1.f);
	return true;
}

// Il file del personaggio: lo zaino si salva e si rilegge uguale; un file di prima (senza zaino) si legge vuoto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoSalvataggio, "Valdorso.Zaino.Salvataggio", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoSalvataggio::RunTest(const FString& Parameters)
{
	FValdorsoPersonaggio Personaggio;
	Personaggio.Nome = TEXT("Tessa");
	ValdorsoZaino::DaiPartenza(Personaggio.Zaino, nullptr);
	Personaggio.Zaino.Posti[0].Cariche = 0;
	FString Testo;
	TestTrue(TEXT("si scrive"), FJsonObjectConverter::UStructToJsonObjectString(Personaggio, Testo));
	FValdorsoPersonaggio Riletto;
	TestTrue(TEXT("si rilegge"), FJsonObjectConverter::JsonObjectStringToUStruct(Testo, &Riletto, 0, 0));
	ValdorsoZaino::Sistema(Riletto.Zaino);
	TestEqual(TEXT("gli Aureus"), Riletto.Zaino.Monete, Personaggio.Zaino.Monete);
	TestTrue(TEXT("la partenza resta data"), Riletto.Zaino.bPartenzaData);
	TestEqual(TEXT("i posti"), Riletto.Zaino.Posti.Num(), ValdorsoZaino::NumeroPosti);
	for (int32 i = 0; i < ValdorsoZaino::NumeroPosti; ++i)
	{
		TestTrue(FString::Printf(TEXT("il posto %d è uguale"), i), Riletto.Zaino.Posti[i] == Personaggio.Zaino.Posti[i]);
	}
	TestEqual(TEXT("la barra"), Riletto.Zaino.Azioni[0], Personaggio.Zaino.Azioni[0]);

	// un file di prima (del 07/11): niente zaino
	FValdorsoPersonaggio DiPrima;
	TestTrue(TEXT("un file senza zaino si legge"), FJsonObjectConverter::JsonObjectStringToUStruct(
		FString(TEXT("{\"versioneSchema\":2,\"nome\":\"Bruno\"}")), &DiPrima, 0, 0));
	ValdorsoZaino::Sistema(DiPrima.Zaino);
	TestFalse(TEXT("e la partenza non è ancora data"), DiPrima.Zaino.bPartenzaData);
	TestEqual(TEXT("vuoto"), ValdorsoZaino::Peso(DiPrima.Zaino), 0.f);

	// un file ritoccato a mano: via quello che non esiste, quantità nei limiti
	FValdorsoZaino Storto;
	Storto.Posti.SetNum(3);
	Storto.Posti[0].Id = TEXT("Spadone_Inventato");
	Storto.Posti[0].Quantita = 5;
	Storto.Posti[1].Id = TEXT("Pane_Nero");
	Storto.Posti[1].Quantita = 500;
	Storto.Monete = -20;
	Storto.Azioni = { TEXT("Ricordo_Anello"), TEXT("Pane_Nero") };
	ValdorsoZaino::Sistema(Storto);
	TestTrue(TEXT("via quello che non esiste"), Storto.Posti[0].Vuoto());
	TestEqual(TEXT("la pila nei limiti"), Storto.Posti[1].Quantita, 10);
	TestEqual(TEXT("niente debiti di Aureus"), Storto.Monete, 0);
	TestTrue(TEXT("via dalla barra quello che non si usa"), Storto.Azioni[0].IsNone());
	TestEqual(TEXT("il pane resta"), Storto.Azioni[1], FName(TEXT("Pane_Nero")));
	TestEqual(TEXT("la griglia intera"), Storto.Posti.Num(), ValdorsoZaino::NumeroPosti);
	return true;
}

// I doni dei PNG: Bruno alla prima scodella, Nonna Edda a chi le porta rispetto; gli altri segni niente.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoDoni, "Valdorso.Zaino.Doni", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoDoni::RunTest(const FString& Parameters)
{
	FName Oggetto;
	int32 Quanti = 0;
	TestTrue(TEXT("Bruno e la prima scodella"), ValdorsoOggetti::DonoDelSegno(TEXT("Bruno"), TEXT("PrimaScodella"), Oggetto, Quanti));
	TestEqual(TEXT("il pane nero"), Oggetto, FName(TEXT("Pane_Nero")));
	TestTrue(TEXT("Nonna Edda e il rispetto"), ValdorsoOggetti::DonoDelSegno(TEXT("NonnaEdda"), TEXT("DovutoRispetto"), Oggetto, Quanti));
	TestNotNull(TEXT("le erbe sono nel catalogo"), ValdorsoOggetti::Trova(Oggetto));
	TestFalse(TEXT("la moneta offerta alle guardie non porta niente"), ValdorsoOggetti::DonoDelSegno(TEXT("GuardiaOttone"), TEXT("MonetaOfferta"), Oggetto, Quanti));
	TestFalse(TEXT("il segno giusto al PNG sbagliato no"), ValdorsoOggetti::DonoDelSegno(TEXT("Gaspare"), TEXT("PrimaScodella"), Oggetto, Quanti));
	return true;
}

// Bagnati e asciutti: nell'acqua subito fradici, sotto la pioggia piano (con il mantello la metà), sotto un tetto ci si
// asciuga il doppio più svelti.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoBagnato, "Valdorso.Zaino.Bagnato", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoBagnato::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("nell'acqua fradici"), ValdorsoBagnato::Passo(0.f, 0.5f, true, 0.f, false, false), 1.f);
	float Senza = 0.f;
	float Con = 0.f;
	for (int32 i = 0; i < 90; ++i)
	{
		Senza = ValdorsoBagnato::Passo(Senza, 1.f, false, 1.f, false, false);
		Con = ValdorsoBagnato::Passo(Con, 1.f, false, 1.f, false, true);
	}
	TestTrue(TEXT("90 secondi di pioggia fitta: fradici"), Senza >= 0.99f);
	TestTrue(TEXT("con il mantello la metà"), FMath::IsNearlyEqual(Con, 0.5f, 0.02f));
	TestEqual(TEXT("sotto un tetto la pioggia non bagna"), ValdorsoBagnato::Passo(0.f, 10.f, false, 1.f, true, false), 0.f);
	const float Fuori = ValdorsoBagnato::Passo(1.f, 60.f, false, 0.f, false, false);
	const float Dentro = ValdorsoBagnato::Passo(1.f, 60.f, false, 0.f, true, false);
	TestTrue(TEXT("ci si asciuga"), Fuori < 1.f);
	TestTrue(TEXT("sotto un tetto più svelti"), Dentro < Fuori);
	TestTrue(TEXT("bagnati: infreddoliti"), ValdorsoBagnato::Infreddolito(0.5f));
	TestFalse(TEXT("appena umidi no"), ValdorsoBagnato::Infreddolito(0.1f));
	TestEqual(TEXT("mai sotto zero"), ValdorsoBagnato::Passo(0.f, 100.f, false, 0.f, false, false), 0.f);
	return true;
}

// (08/11) Il falò: l'acciarino si usa, servono due fasci di rami (il colono ne porta quattro), la vita del fuoco
// alla fine, e accanto al fuoco ci si asciuga anche sotto la pioggia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoFalo, "Valdorso.Zaino.Falo", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoFalo::RunTest(const FString& Parameters)
{
	const FValdorsoTipoOggetto* Acciarino = ValdorsoOggetti::Trova(TEXT("Acciarino"));
	if (TestNotNull(TEXT("l'acciarino"), Acciarino))
	{
		TestTrue(TEXT("l'acciarino accende"), Acciarino->Uso == EValdorsoUso::Accendi);
	}
	TestNotNull(TEXT("i rami secchi"), ValdorsoOggetti::Trova(TEXT("Rami_Secchi")));
	FValdorsoZaino Zaino;
	ValdorsoZaino::DaiPartenza(Zaino, nullptr);
	TestEqual(TEXT("quattro fasci di rami all'arrivo"), ValdorsoZaino::Conta(Zaino, TEXT("Rami_Secchi")), 4);
	TestTrue(TEXT("l'acciarino va nella barra"), ValdorsoZaino::DaBarra(TEXT("Acciarino")));

	// togliere da qualunque posto, tutto o niente
	TestTrue(TEXT("due rami per il primo falò"), ValdorsoZaino::TogliOggetto(Zaino, TEXT("Rami_Secchi"), ValdorsoOggetti::RamiPerFalo));
	TestEqual(TEXT("ne restano due"), ValdorsoZaino::Conta(Zaino, TEXT("Rami_Secchi")), 2);
	TestFalse(TEXT("tre non ci sono: non se ne toglie nessuno"), ValdorsoZaino::TogliOggetto(Zaino, TEXT("Rami_Secchi"), 3));
	TestEqual(TEXT("e restano due"), ValdorsoZaino::Conta(Zaino, TEXT("Rami_Secchi")), 2);
	ValdorsoZaino::Aggiungi(Zaino, TEXT("Rami_Secchi"), 15);  // una pila piena e una da 7
	TestTrue(TEXT("da due pile insieme"), ValdorsoZaino::TogliOggetto(Zaino, TEXT("Rami_Secchi"), 12));
	TestEqual(TEXT("ne restano cinque"), ValdorsoZaino::Conta(Zaino, TEXT("Rami_Secchi")), 5);

	// la vita del fuoco: pieno fino all'ultimo mezzo minuto, poi cala, poi spento
	TestEqual(TEXT("appena acceso"), ValdorsoFaloRegole::Vita(ValdorsoFaloRegole::Durata), 1.f);
	TestEqual(TEXT("a un minuto dalla fine"), ValdorsoFaloRegole::Vita(60.f), 1.f);
	TestTrue(TEXT("a metà dello spegnimento"), FMath::IsNearlyEqual(ValdorsoFaloRegole::Vita(ValdorsoFaloRegole::Spegnimento * 0.5f), 0.5f, 0.01f));
	TestEqual(TEXT("spento"), ValdorsoFaloRegole::Vita(0.f), 0.f);
	TestEqual(TEXT("spento da un pezzo"), ValdorsoFaloRegole::Vita(-20.f), 0.f);

	// al fuoco ci si asciuga in mezzo minuto, anche sotto la pioggia; nell'acqua no
	TestTrue(TEXT("al fuoco, sotto la pioggia, ci si asciuga"), ValdorsoBagnato::Passo(1.f, 10.f, false, 1.f, false, false, true) < 1.f);
	TestEqual(TEXT("mezzo minuto al fuoco: asciutti"), ValdorsoBagnato::Passo(1.f, ValdorsoBagnato::SecondiAsciugaFuoco, false, 0.f, false, false, true), 0.f);
	TestEqual(TEXT("nell'acqua il fuoco non asciuga"), ValdorsoBagnato::Passo(0.f, 1.f, true, 0.f, false, false, true), 1.f);
	return true;
}

// (08/11) Le cose da raccogliere: sempre gli stessi posti, nei luoghi giusti, lontani tra loro, oggetti veri.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoRaccolta, "Valdorso.Zaino.Raccolta", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoRaccolta::RunTest(const FString& Parameters)
{
	const TArray<FValdorsoPuntoRaccolta>& Punti = ValdorsoRaccolta::Punti();
	// (10/11) con le cozze del lago una settantina
	TestTrue(TEXT("una settantina di posti"), Punti.Num() >= 50 && Punti.Num() <= 90);
	TestEqual(TEXT("sempre gli stessi"), ValdorsoRaccolta::Punti().Num(), Punti.Num());
	int32 Rami = 0;
	int32 Erbe = 0;
	int32 Cozze = 0;
	for (int32 i = 0; i < Punti.Num(); ++i)
	{
		const FValdorsoPuntoRaccolta& Punto = Punti[i];
		if (Punto.Oggetto == FName(TEXT("Cozze")))
		{
			++Cozze;
			TestTrue(FString::Printf(TEXT("posto %d: le cozze stanno sul fondale"), i), Punto.bFondale);
		}
		TestNotNull(FString::Printf(TEXT("posto %d: un oggetto vero"), i), ValdorsoOggetti::Trova(Punto.Oggetto));
		TestTrue(FString::Printf(TEXT("posto %d: se ne prende almeno uno"), i), Punto.Quantita >= 1);
		TestTrue(FString::Printf(TEXT("posto %d: ricresce"), i), Punto.Ricrescita >= 60.f);
		Rami += Punto.Oggetto == FName(TEXT("Rami_Secchi")) ? 1 : 0;
		Erbe += Punto.Oggetto == FName(TEXT("Erbe_Palude")) ? 1 : 0;
		// dentro un luogo della valle
		bool bInUnLuogo = false;
		for (const FValdorsoLuogo& Luogo : ValdorsoLuoghi::Tutti())
		{
			bInUnLuogo |= FVector::Dist2D(Luogo.Posizione, Punto.Posizione) <= Luogo.Raggio;
		}
		TestTrue(FString::Printf(TEXT("posto %d: in un luogo della valle"), i), bInUnLuogo);
		for (int32 k = i + 1; k < Punti.Num(); ++k)
		{
			if (FVector::Dist2D(Punti[k].Posizione, Punto.Posizione) < ValdorsoRaccolta::DistanzaMinima)
			{
				AddError(FString::Printf(TEXT("i posti %d e %d sono troppo vicini"), i, k));
			}
		}
	}
	TestTrue(TEXT("ci sono rami per i falò"), Rami >= 10);
	TestTrue(TEXT("ci sono le erbe della palude"), Erbe >= 5);
	TestTrue(TEXT("ci sono le cozze del lago"), Cozze >= 15);
	return true;
}

// (08/11) La mappa della valle: nella partenza; chi era già arrivato la riceve una volta sola con Aggiorna.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestZainoMappa, "Valdorso.Zaino.Mappa", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestZainoMappa::RunTest(const FString& Parameters)
{
	FValdorsoZaino Nuovo;
	ValdorsoZaino::DaiPartenza(Nuovo, nullptr);
	TestEqual(TEXT("la mappa all'arrivo"), ValdorsoZaino::Conta(Nuovo, TEXT("Mappa_Valle")), 1);
	TestEqual(TEXT("la partenza segna tutti gli aggiornamenti"), Nuovo.Aggiornamenti, ValdorsoZaino::UltimoAggiornamento);
	TestFalse(TEXT("niente da aggiornare"), ValdorsoZaino::Aggiorna(Nuovo));

	// un colono arrivato prima (partenza data, senza mappa)
	FValdorsoZaino DiPrima;
	ValdorsoZaino::Sistema(DiPrima);
	DiPrima.bPartenzaData = true;
	ValdorsoZaino::Aggiungi(DiPrima, TEXT("Pane_Nero"), 2);
	TestTrue(TEXT("riceve la mappa"), ValdorsoZaino::Aggiorna(DiPrima));
	TestEqual(TEXT("una"), ValdorsoZaino::Conta(DiPrima, TEXT("Mappa_Valle")), 1);
	TestFalse(TEXT("una volta sola"), ValdorsoZaino::Aggiorna(DiPrima));
	TestEqual(TEXT("sempre una"), ValdorsoZaino::Conta(DiPrima, TEXT("Mappa_Valle")), 1);

	// chi non è ancora arrivato non riceve niente (prima la partenza)
	FValdorsoZaino MaiArrivato;
	ValdorsoZaino::Sistema(MaiArrivato);
	TestFalse(TEXT("prima la partenza"), ValdorsoZaino::Aggiorna(MaiArrivato));
	return true;
}

// (09/11) Il caldo e il freddo: la stagione, l'ora, la quota e il tempo fanno l'aria; bagnati si sente più freddo, il
// mantello, il tetto e il fuoco riparano; le soglie, i freni della stamina e le ore canoniche.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestClima, "Valdorso.Clima.Temperatura", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestClima::RunTest(const FString& Parameters)
{
	using namespace ValdorsoClima;
	const float Piazza = 5500.f;
	TestTrue(TEXT("l'estate è più calda dell'inverno"), Aria(12.f, TEXT("estate"), Piazza, 0.f, 0.f, 0.f) > Aria(12.f, TEXT("inverno"), Piazza, 0.f, 0.f, 0.f) + 10.f);
	TestTrue(TEXT("alle tre del pomeriggio più caldo che all'alba"), Aria(15.f, TEXT("autunno"), Piazza, 0.f, 0.f, 0.f) > Aria(5.f, TEXT("autunno"), Piazza, 0.f, 0.f, 0.f) + 8.f);
	TestTrue(TEXT("le nuvole appiattiscono il giorno"),
		Aria(15.f, TEXT("autunno"), Piazza, 1.f, 0.f, 0.f) - Aria(5.f, TEXT("autunno"), Piazza, 1.f, 0.f, 0.f)
		< Aria(15.f, TEXT("autunno"), Piazza, 0.f, 0.f, 0.f) - Aria(5.f, TEXT("autunno"), Piazza, 0.f, 0.f, 0.f));
	TestTrue(TEXT("in alto più freddo"), Aria(12.f, TEXT("autunno"), 18000.f, 0.f, 0.f, 0.f) < Aria(12.f, TEXT("autunno"), Piazza, 0.f, 0.f, 0.f) - 1.f);
	TestTrue(TEXT("la pioggia rinfresca"), Aria(12.f, TEXT("estate"), Piazza, 0.f, 1.f, 0.f) < Aria(12.f, TEXT("estate"), Piazza, 0.f, 0.f, 0.f));

	const float Fuori = 6.f;
	TestTrue(TEXT("bagnati più freddo"), Percepita(Fuori, 1.f, false, 10.f, false, false, false) < Fuori - 5.f);
	TestTrue(TEXT("il mantello ripara"), Percepita(Fuori, 0.f, false, 10.f, true, false, false) > Fuori);
	TestEqual(TEXT("il mantello non scalda d'estate"), Percepita(28.f, 0.f, false, 10.f, true, false, false), 28.f);
	TestTrue(TEXT("al fuoco si sta caldi"), Percepita(-5.f, 1.f, false, 10.f, false, false, true) >= 18.f);
	TestTrue(TEXT("ma non si arrostisce"), Percepita(25.f, 0.f, false, 10.f, false, false, true) <= 26.f);
	TestEqual(TEXT("nell'acqua conta l'acqua"), Percepita(30.f, 1.f, true, 4.f, true, true, true), 2.f);

	TestTrue(TEXT("sotto zero: gelo"), Sensazione(-1.f) == EValdorsoSensazione::Gelo);
	TestTrue(TEXT("5 gradi: freddo"), Sensazione(5.f) == EValdorsoSensazione::Freddo);
	TestTrue(TEXT("20 gradi: mite"), Sensazione(20.f) == EValdorsoSensazione::Mite);
	TestTrue(TEXT("35 gradi: afa"), Sensazione(35.f) == EValdorsoSensazione::Afa);
	TestEqual(TEXT("mite: nessun freno"), FrenoTemperatura(20.f), 0.f);
	TestTrue(TEXT("il gelo frena più del freddo"), FrenoTemperatura(-3.f) > FrenoTemperatura(4.f));
	TestEqual(TEXT("bagnati e gelati: al massimo tre quarti"), FrenoTotale(0.5f, 0.5f), 0.75f);

	TestEqual(TEXT("le 18:40"), OraEMinuti(18.f + 40.f / 60.f + 0.001f), FString(TEXT("18:40")));
	TestEqual(TEXT("lo zero davanti"), OraEMinuti(7.0834f), FString(TEXT("07:05")));
	TestEqual(TEXT("vespro"), OraCanonica(19.f).ToString(), FString(TEXT("Vespro")));
	TestEqual(TEXT("mattutino"), OraCanonica(1.f).ToString(), FString(TEXT("Mattutino")));
	TestEqual(TEXT("compieta"), OraCanonica(23.9f).ToString(), FString(TEXT("Compieta")));
	return true;
}

// (09/11) La tessera del colono: il numero è sempre lo stesso per lo stesso personaggio, la tessera nuova ha qualifica,
// rilascio e il timbro della capitana; Sistema rilascia quella che manca e accorcia quella troppo lunga; Vista legge il
// Registro (la fede corta) e si salva nel file del personaggio.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTessera, "Valdorso.Tessera.Regole", ValdorsoTestZaino::Bandiere)
bool FValdorsoTestTessera::RunTest(const FString& Parameters)
{
	const FString Numero = ValdorsoTessera::Numero(TEXT("personaggio-1"));
	TestEqual(TEXT("sempre lo stesso numero"), ValdorsoTessera::Numero(TEXT("personaggio-1")), Numero);
	TestEqual(TEXT("maiuscole o minuscole, uguale"), ValdorsoTessera::Numero(TEXT("PERSONAGGIO-1")), Numero);
	TestNotEqual(TEXT("un altro personaggio, un altro numero"), ValdorsoTessera::Numero(TEXT("personaggio-2")), Numero);
	TestEqual(TEXT("lungo come VO-312-XXXX"), Numero.Len(), 11);
	TestTrue(TEXT("comincia con VO-312-"), Numero.StartsWith(TEXT("VO-312-")));
	for (int32 i = 7; i < Numero.Len(); ++i)
	{
		TestTrue(TEXT("niente 0, O, 1 o I"), Numero[i] != TEXT('0') && Numero[i] != TEXT('O') && Numero[i] != TEXT('1') && Numero[i] != TEXT('I'));
	}

	const FValdorsoTessera Nuova = ValdorsoTessera::Nuova(TEXT("personaggio-1"));
	TestEqual(TEXT("la nuova ha il numero"), Nuova.Numero, Numero);
	TestFalse(TEXT("la qualifica"), Nuova.Qualifica.IsEmpty());
	TestFalse(TEXT("il rilascio"), Nuova.Rilascio.IsEmpty());
	TestEqual(TEXT("il timbro della capitana"), Nuova.Timbri.Num(), 1);

	// un file senza tessera (un personaggio di prima): Sistema la rilascia
	FValdorsoTessera DiPrima;
	ValdorsoTessera::Sistema(DiPrima, TEXT("personaggio-1"));
	TestEqual(TEXT("rilasciata"), DiPrima.Numero, Numero);

	// un file rovinato: tutto troppo lungo e troppi timbri
	FValdorsoTessera Rovinata = Nuova;
	Rovinata.Qualifica = FString::ChrN(500, TEXT('a'));
	for (int32 i = 0; i < 40; ++i)
	{
		Rovinata.Timbri.Add(FString::ChrN(300, TEXT('t')));
		Rovinata.Note.Add(TEXT("nota"));
	}
	ValdorsoTessera::Sistema(Rovinata, TEXT("personaggio-1"));
	TestEqual(TEXT("qualifica accorciata"), Rovinata.Qualifica.Len(), ValdorsoTessera::MassimaLunghezza);
	TestEqual(TEXT("timbri al massimo"), Rovinata.Timbri.Num(), ValdorsoTessera::MassimoTimbri);
	TestEqual(TEXT("note al massimo"), Rovinata.Note.Num(), ValdorsoTessera::MassimoNote);
	TestEqual(TEXT("il primo timbro resta"), Rovinata.Timbri[0], Nuova.Timbri[0]);
	for (const FString& Timbro : Rovinata.Timbri)
	{
		TestTrue(TEXT("timbro accorciato"), Timbro.Len() <= ValdorsoTessera::MassimaLunghezza);
	}
	FValdorsoTessera SenzaQualifica = Nuova;
	SenzaQualifica.Qualifica.Reset();
	ValdorsoTessera::Sistema(SenzaQualifica, TEXT("personaggio-1"));
	TestFalse(TEXT("la qualifica torna"), SenzaQualifica.Qualifica.IsEmpty());

	// quello che si vede: dal Registro
	FValdorsoRegistro Registro;
	Registro.Sesso = TEXT("donna");
	Registro.Eta = 27;
	Registro.Fede = TEXT("nereia");
	Registro.Mestiere = TEXT("contadino");
	Registro.Firma = TEXT("@");
	const FValdorsoTesseraVista Vista = ValdorsoTessera::Vista(TEXT("Mara"), &Registro, Nuova);
	TestEqual(TEXT("il nome"), Vista.Nome, FString(TEXT("Mara")));
	TestEqual(TEXT("l'età"), Vista.Eta, 27);
	TestEqual(TEXT("la fede corta"), Vista.Fede, FString(TEXT("Nereia")));
	TestFalse(TEXT("il mestiere"), Vista.Mestiere.IsEmpty());
	TestEqual(TEXT("la firma"), Vista.Firma, FString(TEXT("@")));
	TestEqual(TEXT("il numero"), Vista.Tessera.Numero, Numero);
	const FValdorsoTesseraVista SenzaRegistro = ValdorsoTessera::Vista(TEXT("Prova"), nullptr, Nuova);
	TestEqual(TEXT("senza Registro: solo nome e tessera"), SenzaRegistro.Eta, 0);

	// la tessera si salva nel file del personaggio e torna uguale
	FValdorsoPersonaggio Personaggio;
	Personaggio.Tessera = Rovinata;
	FString Testo;
	TestTrue(TEXT("si scrive"), FJsonObjectConverter::UStructToJsonObjectString(Personaggio, Testo));
	FValdorsoPersonaggio Letto;
	TestTrue(TEXT("si legge"), FJsonObjectConverter::JsonObjectStringToUStruct(Testo, &Letto, 0, 0));
	TestEqual(TEXT("il numero torna"), Letto.Tessera.Numero, Numero);
	TestEqual(TEXT("i timbri tornano"), Letto.Tessera.Timbri.Num(), ValdorsoTessera::MassimoTimbri);
	return true;
}

#endif
