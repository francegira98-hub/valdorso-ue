// Valdorso - Test automatici della v0.2.3 "Armi vere" (14/11/2026, scritto da Claude): la qualita' degli oggetti (i cinque
// gradi, le pile, la provenienza, il salvataggio), le armi che si consumano e si riparano, le armi forgiate, l'arco con due
// mani. Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.Armi").

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoArchivista.h"
#include "ValdorsoBotteghe.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoQualita.h"
#include "ValdorsoZainoDati.h"
#include "JsonObjectConverter.h"

namespace ValdorsoTestArmiVere
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

// I cinque gradi: i nomi, i colori (quelli delle storie del Battito) e quanto valgono, sempre in salita.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestArmiGradi, "Valdorso.Armi.Gradi", ValdorsoTestArmiVere::Bandiere)
bool FValdorsoTestArmiGradi::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("comune"), FString(ValdorsoQualita::Nome(ValdorsoQualita::Comune)), FString(TEXT("Comune")));
	TestEqual(TEXT("leggendaria"), FString(ValdorsoQualita::Nome(ValdorsoQualita::Leggendaria)), FString(TEXT("Leggendaria")));
	TestEqual(TEXT("oltre il massimo: leggendaria"), FString(ValdorsoQualita::Nome(200)), FString(TEXT("Leggendaria")));
	TestFalse(TEXT("la comune non si colora"), ValdorsoQualita::Colorata(ValdorsoQualita::Comune));
	TestTrue(TEXT("la buona si'"), ValdorsoQualita::Colorata(ValdorsoQualita::Buona));
	TestTrue(TEXT("la buona e' verde"), ValdorsoQualita::Colore(ValdorsoQualita::Buona).G > ValdorsoQualita::Colore(ValdorsoQualita::Buona).R);
	TestTrue(TEXT("la rara e' blu"), ValdorsoQualita::Colore(ValdorsoQualita::Rara).B > ValdorsoQualita::Colore(ValdorsoQualita::Rara).R);
	for (uint8 q = 1; q <= ValdorsoQualita::Massima; ++q)
	{
		TestTrue(TEXT("il danno sale"), ValdorsoQualita::Danno(q) > ValdorsoQualita::Danno(q - 1));
		TestTrue(TEXT("la durata sale"), ValdorsoQualita::Durata(q) > ValdorsoQualita::Durata(q - 1));
		TestTrue(TEXT("il ristoro sale"), ValdorsoQualita::Ristoro(q) > ValdorsoQualita::Ristoro(q - 1));
		TestTrue(TEXT("la parata sale"), ValdorsoQualita::Parata(q) > ValdorsoQualita::Parata(q - 1));
	}
	// ogni arma e ogni scudo che si impugna ha la sua famiglia e si consuma
	for (const FValdorsoTipoOggetto& Tipo : ValdorsoOggetti::Catalogo())
	{
		if (Tipo.Uso == EValdorsoUso::Impugna)
		{
			TestTrue(FString::Printf(TEXT("%s: ha una famiglia"), *Tipo.Id.ToString()),
				ValdorsoQualita::Famiglia(Tipo.Id) != EValdorsoFamigliaArma::Nessuna);
			TestTrue(FString::Printf(TEXT("%s: si consuma"), *Tipo.Id.ToString()), ValdorsoQualita::DurataMassima(Tipo.Id, 0) > 0);
		}
	}
	TestEqual(TEXT("il pane non si consuma come un'arma"), ValdorsoQualita::DurataMassima(TEXT("Pane_Nero"), 0), 0);
	TestEqual(TEXT("la famiglia dell'arco"), ValdorsoQualita::NomeFamiglia(ValdorsoQualita::Famiglia(TEXT("Arco"))), FName(TEXT("Arco")));
	TestTrue(TEXT("l'arco e' di legno"), ValdorsoQualita::DiLegno(TEXT("Arco")));
	TestFalse(TEXT("la spada e' di ferro"), ValdorsoQualita::DiLegno(TEXT("Spada_Corta")));
	return true;
}

// Le pile: si impila solo con lo stesso grado e la stessa provenienza; si sposta e si salva tutto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestArmiPile, "Valdorso.Armi.Pile", ValdorsoTestArmiVere::Bandiere)
bool FValdorsoTestArmiPile::RunTest(const FString& Parameters)
{
	const FName Pane(TEXT("Pane_Nero"));
	FValdorsoZaino Z;
	ValdorsoZaino::Sistema(Z);
	TestEqual(TEXT("tre pani comuni"), ValdorsoZaino::Aggiungi(Z, Pane, 3), 3);
	TestEqual(TEXT("due pani buoni"), ValdorsoZaino::AggiungiCon(Z, Pane, 2, ValdorsoQualita::Buona, TEXT("Forno di Bruno")), 2);
	TestEqual(TEXT("due pile diverse"), Z.Posti[0].Quantita, 3);
	TestEqual(TEXT("la seconda pila"), Z.Posti[1].Quantita, 2);
	TestEqual(TEXT("la seconda e' buona"), static_cast<int32>(Z.Posti[1].Qualita), static_cast<int32>(ValdorsoQualita::Buona));
	TestEqual(TEXT("con la sua provenienza"), Z.Posti[1].Provenienza, FString(TEXT("Forno di Bruno")));
	TestEqual(TEXT("ancora un pane buono va sulla sua pila"), ValdorsoZaino::AggiungiCon(Z, Pane, 1, ValdorsoQualita::Buona, TEXT("Forno di Bruno")), 1);
	TestEqual(TEXT("tre buoni"), Z.Posti[1].Quantita, 3);
	TestEqual(TEXT("sei pani in tutto"), ValdorsoZaino::Conta(Z, Pane), 6);
	// spostare una pila sull'altra di un grado diverso: si scambiano, non si uniscono
	TestTrue(TEXT("si sposta"), ValdorsoZaino::Sposta(Z, 0, 1));
	TestEqual(TEXT("scambiate"), static_cast<int32>(Z.Posti[0].Qualita), static_cast<int32>(ValdorsoQualita::Buona));
	TestEqual(TEXT("non unite"), Z.Posti[1].Quantita, 3);
	TestEqual(TEXT("il testo della qualita'"), ValdorsoQualita::TestoQualita(Z.Posti[0]), FString(TEXT("Buona · Forno di Bruno")));
	TestTrue(TEXT("un pane comune non dice niente"), ValdorsoQualita::TestoQualita(Z.Posti[1]).IsEmpty());
	// nel baule e di nuovo fuori, con il grado
	TestTrue(TEXT("nel baule"), ValdorsoZaino::Deposita(Z, 0));
	TestEqual(TEXT("nel baule resta buono"), static_cast<int32>(Z.Baule[0].Qualita), static_cast<int32>(ValdorsoQualita::Buona));
	TestTrue(TEXT("e fuori"), ValdorsoZaino::Ritira(Z, 0));
	TestEqual(TEXT("fuori resta buono"), static_cast<int32>(Z.Posti[0].Qualita), static_cast<int32>(ValdorsoQualita::Buona));
	// si salva e si rilegge
	ValdorsoZaino::AggiungiCon(Z, TEXT("Spada_Corta"), 1, ValdorsoQualita::Rara, TEXT("Trovata nella grotta del nord"));
	const int32 PostoSpada = ValdorsoZaino::PostoDi(Z, TEXT("Spada_Corta"));
	Z.Posti[PostoSpada].Usura = 12;
	FValdorsoPersonaggio Personaggio;
	Personaggio.Zaino = Z;
	FString Testo;
	TestTrue(TEXT("si scrive"), FJsonObjectConverter::UStructToJsonObjectString(Personaggio, Testo));
	FValdorsoPersonaggio Riletto;
	TestTrue(TEXT("si rilegge"), FJsonObjectConverter::JsonObjectStringToUStruct(Testo, &Riletto, 0, 0));
	ValdorsoZaino::Sistema(Riletto.Zaino);
	TestEqual(TEXT("il grado resta"), static_cast<int32>(Riletto.Zaino.Posti[PostoSpada].Qualita), static_cast<int32>(ValdorsoQualita::Rara));
	TestEqual(TEXT("la provenienza resta"), Riletto.Zaino.Posti[PostoSpada].Provenienza, FString(TEXT("Trovata nella grotta del nord")));
	TestEqual(TEXT("l'usura resta"), Riletto.Zaino.Posti[PostoSpada].Usura, 12);
	// un grado impossibile (un file rovinato) torna nei limiti
	Riletto.Zaino.Posti[PostoSpada].Qualita = 77;
	Riletto.Zaino.Posti[PostoSpada].Usura = 99999;
	ValdorsoZaino::Sistema(Riletto.Zaino);
	TestEqual(TEXT("al massimo leggendaria"), static_cast<int32>(Riletto.Zaino.Posti[PostoSpada].Qualita), static_cast<int32>(ValdorsoQualita::Leggendaria));
	TestEqual(TEXT("l'usura al massimo"), Riletto.Zaino.Posti[PostoSpada].Usura,
		ValdorsoQualita::DurataMassima(TEXT("Spada_Corta"), ValdorsoQualita::Leggendaria));
	return true;
}

// Le armi in mano: la migliore non rovinata, si consumano, si rovinano, si riparano (alla bottega giusta).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestArmiUsura, "Valdorso.Armi.Usura", ValdorsoTestArmiVere::Bandiere)
bool FValdorsoTestArmiUsura::RunTest(const FString& Parameters)
{
	const FName Spada(TEXT("Spada_Corta"));
	FValdorsoZaino Z;
	ValdorsoZaino::Sistema(Z);
	ValdorsoZaino::Aggiungi(Z, Spada, 1);
	ValdorsoZaino::AggiungiCon(Z, Spada, 1, ValdorsoQualita::Epica, TEXT("Forgiata da Fra"));
	TestTrue(TEXT("si impugna"), ValdorsoZaino::Impugna(Z, Spada));
	const int32 Migliore = ValdorsoZaino::PostoInMano(Z, Spada);
	TestEqual(TEXT("in mano c'e' l'epica"), Z.Posti.IsValidIndex(Migliore) ? static_cast<int32>(Z.Posti[Migliore].Qualita) : -1, static_cast<int32>(ValdorsoQualita::Epica));
	TestEqual(TEXT("l'epica colpisce di piu'"), ValdorsoQualita::ForzaDelPosto(Z.Posti[Migliore], false), ValdorsoQualita::Danno(ValdorsoQualita::Epica));
	const int32 Massimo = ValdorsoQualita::DurataMassima(Spada, ValdorsoQualita::Epica);
	TestTrue(TEXT("l'epica dura di piu'"), Massimo > ValdorsoQualita::DurataMassima(Spada, ValdorsoQualita::Comune));
	TestFalse(TEXT("un colpo non la rovina"), ValdorsoZaino::Consuma(Z, false, 1));
	TestEqual(TEXT("un colpo segnato"), Z.Posti[Migliore].Usura, 1);
	TestTrue(TEXT("consumata"), ValdorsoQualita::Consumata(Z.Posti[Migliore]));
	TestTrue(TEXT("tutti gli altri la rovinano"), ValdorsoZaino::Consuma(Z, false, Massimo));
	TestTrue(TEXT("rovinata"), ValdorsoQualita::Rovinata(Z.Posti[Migliore]));
	TestEqual(TEXT("rovinata colpisce la meta'"), ValdorsoQualita::ForzaDelPosto(Z.Posti[Migliore], false),
		ValdorsoQualita::Danno(ValdorsoQualita::Epica) * ValdorsoQualita::FattoreRovinata);
	// rovinata l'epica, in mano passa la comune (che e' intera)
	const int32 Adesso = ValdorsoZaino::PostoInMano(Z, Spada);
	TestEqual(TEXT("adesso in mano la comune"), Z.Posti.IsValidIndex(Adesso) ? static_cast<int32>(Z.Posti[Adesso].Qualita) : -1, static_cast<int32>(ValdorsoQualita::Comune));
	TestTrue(TEXT("il testo dello stato dice rovinata"), ValdorsoQualita::TestoStato(Z.Posti[Migliore]).StartsWith(TEXT("Rovinata")));
	// la riparazione: Marta ripara il ferro, il falegname no
	const FValdorsoRicetta* Ferro = ValdorsoBotteghe::Trova(TEXT("Ripara_Ferro"));
	const FValdorsoRicetta* Legno = ValdorsoBotteghe::Trova(TEXT("Ripara_Legno"));
	TestNotNull(TEXT("Marta ripara"), Ferro);
	TestNotNull(TEXT("il falegname ripara"), Legno);
	if (Ferro && Legno)
	{
		// si ripara l'epica rovinata (anche se in mano adesso c'e' la comune), da Marta e non dal falegname
		Z.Monete = 100;
		TestEqual(TEXT("si ripara l'epica"), ValdorsoZaino::PostoDaRiparare(Z, Spada), Migliore);
		TestTrue(TEXT("Marta puo' ripararla"), ValdorsoBotteghe::Manca(Z, *Ferro).IsEmpty());
		TestFalse(TEXT("ma non il falegname"), ValdorsoBotteghe::Manca(Z, *Legno).IsEmpty());
		TestTrue(TEXT("Marta la ripara"), ValdorsoBotteghe::Fai(Z, *Ferro));
		TestEqual(TEXT("e si paga"), Z.Monete, 100 - Ferro->Costo);
		TestEqual(TEXT("l'epica torna nuova"), Z.Posti[Migliore].Usura, 0);
		TestEqual(TEXT("e torna in mano"), ValdorsoZaino::PostoInMano(Z, Spada), Migliore);
		TestFalse(TEXT("niente piu' da riparare"), ValdorsoBotteghe::Manca(Z, *Ferro).IsEmpty());
		// e la riga della bottega dice cosa fa
		TestFalse(TEXT("la riga della riparazione"), ValdorsoBotteghe::TestoDa(*Ferro).IsEmpty());
	}
	return true;
}

// Le armi forgiate: il grado dipende dalla mano (Bottega) e dalla fortuna; mai leggendarie.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestArmiForgia, "Valdorso.Armi.Forgia", ValdorsoTestArmiVere::Bandiere)
bool FValdorsoTestArmiForgia::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("chi non sa, senza fortuna: comune"), static_cast<int32>(ValdorsoQualita::Forgiata(0.f, 0.f)), static_cast<int32>(ValdorsoQualita::Comune));
	TestEqual(TEXT("chi non sa, con tutta la fortuna: al massimo comune"), static_cast<int32>(ValdorsoQualita::Forgiata(0.f, 1.f)), static_cast<int32>(ValdorsoQualita::Comune));
	TestEqual(TEXT("un artigiano con un po' di fortuna: buona"), static_cast<int32>(ValdorsoQualita::Forgiata(40.f, 0.6f)), static_cast<int32>(ValdorsoQualita::Buona));
	TestEqual(TEXT("un mastro fortunato: rara"), static_cast<int32>(ValdorsoQualita::Forgiata(60.f, 1.f)), static_cast<int32>(ValdorsoQualita::Rara));
	TestEqual(TEXT("mani d'oro e fortuna: epica"), static_cast<int32>(ValdorsoQualita::Forgiata(100.f, 0.8f)), static_cast<int32>(ValdorsoQualita::Epica));
	for (float Caso = 0.f; Caso <= 1.f; Caso += 0.1f)
	{
		TestTrue(TEXT("mai leggendaria"), ValdorsoQualita::Forgiata(100.f, Caso) < ValdorsoQualita::Leggendaria);
		TestTrue(TEXT("con piu' bravura non si scende"), ValdorsoQualita::Forgiata(80.f, Caso) >= ValdorsoQualita::Forgiata(30.f, Caso));
	}
	// la forgia del colono e le cose di Marta
	const FValdorsoRicetta* Forgia = ValdorsoBotteghe::Trova(TEXT("Forgia_Spada_Corta"));
	const FValdorsoRicetta* Buona = ValdorsoBotteghe::Trova(TEXT("Compra_Spada_Corta_Buona"));
	TestNotNull(TEXT("si forgia la spada"), Forgia);
	TestNotNull(TEXT("Marta vende la spada buona"), Buona);
	if (Forgia && Buona)
	{
		TestTrue(TEXT("la forgia la fa il colono"), Forgia->bForgia);
		FValdorsoZaino Z;
		ValdorsoZaino::Sistema(Z);
		Z.Monete = 200;
		TestTrue(TEXT("si compra la buona"), ValdorsoBotteghe::Fai(Z, *Buona));
		const int32 Posto = ValdorsoZaino::PostoDi(Z, TEXT("Spada_Corta"));
		TestEqual(TEXT("e' buona"), Z.Posti.IsValidIndex(Posto) ? static_cast<int32>(Z.Posti[Posto].Qualita) : -1, static_cast<int32>(ValdorsoQualita::Buona));
		TestEqual(TEXT("ed e' di Marta"), Z.Posti.IsValidIndex(Posto) ? Z.Posti[Posto].Provenienza : FString(), FString(TEXT("Fattura di Marta")));
		ValdorsoZaino::Aggiungi(Z, TEXT("Lingotto_Ferro"), 2);
		ValdorsoZaino::Aggiungi(Z, TEXT("Cuoio"), 1);
		TestTrue(TEXT("si forgia"), ValdorsoBotteghe::FaiCon(Z, *Forgia, ValdorsoQualita::Rara, TEXT("Forgiata da Fra")));
		TestEqual(TEXT("due spade"), ValdorsoZaino::Conta(Z, TEXT("Spada_Corta")), 2);
		TestEqual(TEXT("i lingotti usati"), ValdorsoZaino::Conta(Z, TEXT("Lingotto_Ferro")), 0);
	}
	return true;
}

// L'arco: due mani (niente scudo), le frecce si impilano a venti.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestArmiArco, "Valdorso.Armi.Arco", ValdorsoTestArmiVere::Bandiere)
bool FValdorsoTestArmiArco::RunTest(const FString& Parameters)
{
	const FValdorsoTipoOggetto* Arco = ValdorsoOggetti::Trova(ValdorsoOggetti::IdArco);
	const FValdorsoTipoOggetto* Frecce = ValdorsoOggetti::Trova(ValdorsoOggetti::IdFrecce);
	TestNotNull(TEXT("c'e' l'arco"), Arco);
	TestNotNull(TEXT("ci sono le frecce"), Frecce);
	if (!Arco || !Frecce)
	{
		return true;
	}
	TestTrue(TEXT("l'arco vuole due mani"), Arco->bDueMani);
	TestTrue(TEXT("si impugna"), Arco->Uso == EValdorsoUso::Impugna);
	TestEqual(TEXT("le frecce a venti"), Frecce->Pila, 20);
	FValdorsoZaino Z;
	ValdorsoZaino::Sistema(Z);
	ValdorsoZaino::Aggiungi(Z, Arco->Id, 1);
	ValdorsoZaino::Aggiungi(Z, TEXT("Scudo_Legno"), 1);
	TestTrue(TEXT("lo scudo in mano"), ValdorsoZaino::Impugna(Z, TEXT("Scudo_Legno")));
	TestTrue(TEXT("l'arco in mano"), ValdorsoZaino::Impugna(Z, Arco->Id));
	TestTrue(TEXT("lo scudo torna via"), Z.Sinistra.IsNone());
	TestTrue(TEXT("di nuovo lo scudo"), ValdorsoZaino::Impugna(Z, TEXT("Scudo_Legno")));
	TestTrue(TEXT("e l'arco torna via"), Z.Destra.IsNone());
	TestEqual(TEXT("trenta frecce: due pile"), ValdorsoZaino::Aggiungi(Z, Frecce->Id, 30), 30);
	TestEqual(TEXT("tutte contate"), ValdorsoZaino::Conta(Z, Frecce->Id), 30);
	// il falegname le vende a dieci e insegna a farle
	const FValdorsoRicetta* Mazzo = ValdorsoBotteghe::Trova(TEXT("Compra_Frecce_10"));
	TestNotNull(TEXT("le frecce dal falegname"), Mazzo);
	if (Mazzo)
	{
		TestEqual(TEXT("a dieci"), Mazzo->QuantiRisultato, 10);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
