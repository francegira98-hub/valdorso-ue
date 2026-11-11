// Valdorso - Test automatici di v0.1.7 "Volti e giornate", parti B-E (11/11): i riti (la stella di nascita, l'anno e
// un giorno, la pietra dei segni), la segnalazione F8, la versione di rete, il permesso delle abilita'. Scritto da Claude
// l'11/11/2026. Filtro "Valdorso.Riti".

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoAcque.h"
#include "ValdorsoCarovana.h"
#include "ValdorsoDialoghi.h"
#include "ValdorsoOggetti.h"
#include "ValdorsoZainoDati.h"
#include "ValdorsoGesti.h"
#include "ValdorsoMusica.h"
#include "ValdorsoPermessi.h"
#include "ValdorsoRaccolta.h"
#include "ValdorsoScorte.h"
#include "ValdorsoRiti.h"
#include "ValdorsoSegnala.h"
#include "ValdorsoStorie.h"
#include "ValdorsoVersione.h"

namespace ValdorsoTestRiti
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

// La stella: dodici, sempre la stessa per lo stesso giorno; il primo e l'ultimo giorno dell'anno hanno la prima e l'ultima.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRitiStella, "Valdorso.Riti.Stella", ValdorsoTestRiti::Bandiere)
bool FValdorsoTestRitiStella::RunTest(const FString& Parameters)
{
	const TArray<FValdorsoStella>& Stelle = ValdorsoRiti::Stelle();
	TestEqual(TEXT("dodici stelle"), Stelle.Num(), 12);
	const int64 Capodanno = FDateTime(2026, 1, 1, 12).ToUnixTimestamp();
	const int64 SanSilvestro = FDateTime(2026, 12, 31, 12).ToUnixTimestamp();
	const int64 Bisestile = FDateTime(2028, 12, 31, 12).ToUnixTimestamp();
	TestTrue(TEXT("il primo giorno: la prima"), FCString::Strcmp(ValdorsoRiti::StellaDi(Capodanno).Nome, Stelle[0].Nome) == 0);
	TestTrue(TEXT("l'ultimo giorno: l'ultima"), FCString::Strcmp(ValdorsoRiti::StellaDi(SanSilvestro).Nome, Stelle[11].Nome) == 0);
	TestTrue(TEXT("anche nei bisestili"), FCString::Strcmp(ValdorsoRiti::StellaDi(Bisestile).Nome, Stelle[11].Nome) == 0);
	TestTrue(TEXT("sempre la stessa"), ValdorsoRiti::StellaDi(Capodanno + 3600).Nome == ValdorsoRiti::StellaDi(Capodanno).Nome);
	TSet<FString> Viste;
	for (int32 Giorno = 0; Giorno < 365; ++Giorno)
	{
		Viste.Add(ValdorsoRiti::StellaDi(Capodanno + static_cast<int64>(Giorno) * 86400).Nome);
	}
	TestEqual(TEXT("in un anno si vedono tutte"), Viste.Num(), 12);
	for (const FValdorsoStella& S : Stelle)
	{
		TestTrue(TEXT("ogni stella ha nome e motto"), FCString::Strlen(S.Nome) > 3 && FCString::Strlen(S.Motto) > 10);
	}
	return true;
}

// L'anno e un giorno: 366 giorni veri dalla firma, non prima; senza firma mai.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRitiAnno, "Valdorso.Riti.Anno", ValdorsoTestRiti::Bandiere)
bool FValdorsoTestRitiAnno::RunTest(const FString& Parameters)
{
	const int64 Firma = FDateTime(2026, 11, 11, 10).ToUnixTimestamp();
	TestFalse(TEXT("il giorno dopo no"), ValdorsoRiti::AnnoEUnGiorno(Firma, Firma + 86400));
	TestFalse(TEXT("un anno esatto no"), ValdorsoRiti::AnnoEUnGiorno(Firma, Firma + 365LL * 86400));
	TestTrue(TEXT("un anno e un giorno si'"), ValdorsoRiti::AnnoEUnGiorno(Firma, Firma + 366LL * 86400));
	TestFalse(TEXT("senza firma mai"), ValdorsoRiti::AnnoEUnGiorno(0, Firma));

	// la cerimonia nel dialogo di Padre Anselmo: c'e', si arriva, e da' il segno del cittadino
	const FValdorsoDialogo* Anselmo = ValdorsoDialoghi::Trova(FName(ValdorsoRiti::IdSacerdote));
	if (TestNotNull(TEXT("c'e' Padre Anselmo"), Anselmo))
	{
		const FValdorsoNodoDialogo* Cerimonia = Anselmo->Trova(TEXT("cerimonia"));
		const FValdorsoNodoDialogo* Presto = Anselmo->Trova(TEXT("presto"));
		TestNotNull(TEXT("il nodo della cerimonia"), Cerimonia);
		TestNotNull(TEXT("il nodo per chi e' in anticipo"), Presto);
		if (Cerimonia)
		{
			TestTrue(TEXT("la cerimonia fa cittadini"), Cerimonia->Segna == FName(ValdorsoRiti::SegnoCittadino));
		}
		const FValdorsoNodoDialogo* Ritorno = Anselmo->Trova(Anselmo->Ritorno);
		bool bScelta = false;
		for (const FValdorsoSceltaDialogo& S : Ritorno ? Ritorno->Scelte : TArray<FValdorsoSceltaDialogo>())
		{
			bScelta |= S.Evento == FName(ValdorsoRiti::EventoCerimonia) && S.VisibileSe == FName(ValdorsoRiti::SegnoAnno)
				&& S.NascostaSe == FName(ValdorsoRiti::SegnoCittadino);
		}
		TestTrue(TEXT("al ritorno, la risposta solo per chi ha il segno dell'anno"), bScelta);
	}
	return true;
}

// La pietra dei segni: uno per nome (senza badare alle maiuscole), si legge dal piu' nuovo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRitiPietra, "Valdorso.Riti.Pietra", ValdorsoTestRiti::Bandiere)
bool FValdorsoTestRitiPietra::RunTest(const FString& Parameters)
{
	TArray<FValdorsoSegnoInciso> Segni;
	const TArray<FString> Liscia = ValdorsoRiti::Righe(Segni, 5);
	TestEqual(TEXT("liscia: una riga"), Liscia.Num(), 1);
	TestTrue(TEXT("Aldo incide"), ValdorsoRiti::Incidi(Segni, TEXT("Aldo"), 1000000000, TEXT("della Lanterna")));
	TestFalse(TEXT("due volte no"), ValdorsoRiti::Incidi(Segni, TEXT(" aldo "), 1000000100, TEXT("della Lanterna")));
	TestFalse(TEXT("senza nome no"), ValdorsoRiti::Incidi(Segni, TEXT("   "), 1000000100, TEXT("della Lanterna")));
	TestTrue(TEXT("Bice incide"), ValdorsoRiti::Incidi(Segni, TEXT("Bice"), 1000000200, TEXT("dell'Orsa")));
	const TArray<FString> Righe = ValdorsoRiti::Righe(Segni, 1);
	TestEqual(TEXT("il titolo e un segno"), Righe.Num(), 2);
	TestTrue(TEXT("il piu' nuovo per primo"), Righe.Num() == 2 && Righe[1].StartsWith(TEXT("Bice")));
	return true;
}

// F8, la versione di rete, il permesso delle abilita'.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRitiFondamenta, "Valdorso.Riti.Fondamenta", ValdorsoTestRiti::Bandiere)
bool FValdorsoTestRitiFondamenta::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("le ultime due righe"), ValdorsoSegnala::UltimeRighe(TEXT("uno\ndue\ntre\n\n"), 2), FString(TEXT("due\ntre")));
	TestEqual(TEXT("piu' righe di quelle che ci sono"), ValdorsoSegnala::UltimeRighe(TEXT("uno"), 10), FString(TEXT("uno")));
	TestEqual(TEXT("niente"), ValdorsoSegnala::UltimeRighe(TEXT(""), 10), FString());
	TestTrue(TEXT("la cartella delle segnalazioni"), ValdorsoSegnala::Cartella().EndsWith(TEXT("Segnalazioni")));

	TestTrue(TEXT("il numero di rete non e' zero"), ValdorsoVersione::DiRete() != 0);
	TestEqual(TEXT("ed e' sempre lo stesso"), ValdorsoVersione::DiRete(), ValdorsoVersione::DiRete());

	int32 Maschera = 0;
	TestTrue(TEXT("il permesso abilita"), ValdorsoPermessi::DaTesto(TEXT("abilita"), Maschera) && Maschera == ValdorsoPermessi::Abilita);
	TestTrue(TEXT("tutti comprende abilita"), (ValdorsoPermessi::Tutti & ValdorsoPermessi::Abilita) != 0);
	TestEqual(TEXT("si riscrive"), ValdorsoPermessi::Testo(ValdorsoPermessi::Abilita), FString(TEXT("abilita")));
	return true;
}

// La musica sceglie il momento giusto; la ruota dei gesti capisce dove punta il mouse.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRitiMusicaGesti, "Valdorso.Riti.MusicaGesti", ValdorsoTestRiti::Bandiere)
bool FValdorsoTestRitiMusicaGesti::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("di giorno"), ValdorsoMusica::Momento(false, false, 10.f) == EValdorsoMomentoMusica::Giorno);
	TestTrue(TEXT("di notte"), ValdorsoMusica::Momento(false, false, 23.f) == EValdorsoMomentoMusica::Notte);
	TestTrue(TEXT("all'alba ancora notte"), ValdorsoMusica::Momento(false, false, 5.5f) == EValdorsoMomentoMusica::Notte);
	TestTrue(TEXT("nel tempio"), ValdorsoMusica::Momento(false, true, 23.f) == EValdorsoMomentoMusica::Tempio);
	TestTrue(TEXT("il combattimento vince su tutto"), ValdorsoMusica::Momento(true, true, 10.f) == EValdorsoMomentoMusica::Combattimento);

	const int32 Quanti = ValdorsoGesti::Tutti().Num();
	TestEqual(TEXT("otto gesti"), Quanti, 8);
	TestEqual(TEXT("al centro niente"), ValdorsoGesti::Spicchio(FVector2D(5.0, 5.0), Quanti, 45.f), -1);
	TestEqual(TEXT("in alto il primo"), ValdorsoGesti::Spicchio(FVector2D(0.0, -100.0), Quanti, 45.f), 0);
	TestEqual(TEXT("a destra il terzo"), ValdorsoGesti::Spicchio(FVector2D(100.0, 0.0), Quanti, 45.f), 2);
	TestEqual(TEXT("in basso il quinto"), ValdorsoGesti::Spicchio(FVector2D(0.0, 100.0), Quanti, 45.f), 4);
	TestEqual(TEXT("a sinistra il settimo"), ValdorsoGesti::Spicchio(FVector2D(-100.0, 0.0), Quanti, 45.f), 6);
	TestEqual(TEXT("quasi in alto, a sinistra: ancora il primo"), ValdorsoGesti::Spicchio(FVector2D(-20.0, -100.0), Quanti, 45.f), 0);
	return true;
}

// (v0.1.8) Le storie: i segni di un altro PNG ("Altro:Segno"), l'ora della veglia, i nodi delle quattro storie.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRitiStorie, "Valdorso.Riti.Storie", ValdorsoTestRiti::Bandiere)
bool FValdorsoTestRitiStorie::RunTest(const FString& Parameters)
{
	FName Dialogo;
	FName Segno;
	ValdorsoDialoghi::DividiSegno(TEXT("NonnaEdda"), TEXT("Livia:LetteraData"), Dialogo, Segno);
	TestTrue(TEXT("il segno di un altro PNG"), Dialogo == FName(TEXT("Livia")) && Segno == FName(TEXT("LetteraData")));
	ValdorsoDialoghi::DividiSegno(TEXT("NonnaEdda"), TEXT("CorvoPartito"), Dialogo, Segno);
	TestTrue(TEXT("senza due punti: di questo PNG"), Dialogo == FName(TEXT("NonnaEdda")) && Segno == FName(TEXT("CorvoPartito")));

	ValdorsoDialoghi::Dimentica();
	FValdorsoMemoriaPerNome Memoria(TEXT("Prova delle storie"));
	TestFalse(TEXT("all'inizio niente"), ValdorsoDialoghi::HaSegnoDi(Memoria, TEXT("NonnaEdda"), TEXT("Livia:LetteraData")));
	ValdorsoDialoghi::MettiSegnoDi(Memoria, TEXT("NonnaEdda"), TEXT("Livia:LetteraData"));
	TestTrue(TEXT("il segno va a Livia"), Memoria.HaSegno(TEXT("Livia"), TEXT("LetteraData")));
	TestFalse(TEXT("non a Nonna Edda"), Memoria.HaSegno(TEXT("NonnaEdda"), TEXT("LetteraData")));
	TestTrue(TEXT("e si legge da Nonna Edda"), ValdorsoDialoghi::HaSegnoDi(Memoria, TEXT("NonnaEdda"), TEXT("Livia:LetteraData")));
	// la risposta del corvo si vede solo ora
	const FValdorsoDialogo* Edda = ValdorsoDialoghi::Trova(TEXT("NonnaEdda"));
	if (TestNotNull(TEXT("c'e' Nonna Edda"), Edda))
	{
		const TArray<int32> Viste = ValdorsoDialoghi::ScelteVisibili(*Edda, Edda->Ritorno, Memoria);
		bool bCorvo = false;
		for (const int32 i : Viste)
		{
			bCorvo |= Edda->Trova(Edda->Ritorno)->Scelte[i].Verso == FName(TEXT("corvo"));
		}
		TestTrue(TEXT("con la lettera si vede la risposta del corvo"), bCorvo);
	}
	ValdorsoDialoghi::Dimentica();

	TestTrue(TEXT("si veglia a mezzanotte"), ValdorsoStorie::OraDiVeglia(0.f));
	TestTrue(TEXT("e alle 22"), ValdorsoStorie::OraDiVeglia(22.f));
	TestFalse(TEXT("non a mezzogiorno"), ValdorsoStorie::OraDiVeglia(12.f));

	struct FAttesa { const TCHAR* Dialogo; const TCHAR* Nodo; };
	const FAttesa Nodi[] = {
		{ TEXT("Livia"), TEXT("scrivi") }, { TEXT("Livia"), TEXT("partita") }, { TEXT("NonnaEdda"), TEXT("corvo_via") },
		{ TEXT("Tobia"), TEXT("sasso_sogno") }, { TEXT("Tobia"), TEXT("sasso_trovato") }, { TEXT("Tobia"), TEXT("non_ancora") },
		{ TEXT("Marta"), TEXT("canta") }, { TEXT("Marta"), TEXT("senza_cuore") },
		{ TEXT("PadreAnselmo"), TEXT("veglia_inizio") }, { TEXT("PadreAnselmo"), TEXT("di_giorno") },
		{ TEXT("NonnaEdda"), TEXT("acqua_torbida") }, { TEXT("NonnaEdda"), TEXT("acqua_niente") },
		{ TEXT("Teodoro"), TEXT("grano_nascosto") }, { TEXT("Teodoro"), TEXT("grano_ok") }, { TEXT("Teodoro"), TEXT("grano_gia") },
	};
	for (const FAttesa& A : Nodi)
	{
		const FValdorsoDialogo* D = ValdorsoDialoghi::Trova(FName(A.Dialogo));
		TestTrue(FString::Printf(TEXT("%s/%s c'e'"), A.Dialogo, A.Nodo), D && D->Trova(FName(A.Nodo)) != nullptr);
	}
	return true;
}

// (v0.2) L'acqua della valle: che acqua da' ogni posto, il secchio pieno torna vuoto, la borraccia non riempie i secchi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRitiAcqua, "Valdorso.Riti.Acqua", ValdorsoTestRiti::Bandiere)
bool FValdorsoTestRitiAcqua::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("la pozza della cascata: pura"), ValdorsoAcqua::Qualita(TEXT("Pozza della Cascata")), 2);
	TestEqual(TEXT("il lago: chiara"), ValdorsoAcqua::Qualita(TEXT("Lago")), 1);
	TestEqual(TEXT("il fiume: chiara"), ValdorsoAcqua::Qualita(TEXT("Fiume")), 1);
	TestEqual(TEXT("la palude: torbida"), ValdorsoAcqua::Qualita(TEXT("Palude di Nonna Edda")), 0);
	TestEqual(TEXT("le miniere allagate: torbida"), ValdorsoAcqua::Qualita(TEXT("Acqua delle miniere")), 0);
	TestEqual(TEXT("senza nome: chiara"), ValdorsoAcqua::Qualita(FString()), 1);

	const FValdorsoTipoOggetto* Secchio = ValdorsoOggetti::Trova(TEXT("Secchio"));
	if (TestNotNull(TEXT("c'e' il secchio"), Secchio))
	{
		TestTrue(TEXT("il secchio si riempie"), Secchio->Uso == EValdorsoUso::Raccogli);
	}
	for (const TCHAR* Id : { TEXT("Secchio_Acqua_Pura"), TEXT("Secchio_Acqua_Chiara"), TEXT("Secchio_Acqua_Torbida") })
	{
		const FValdorsoTipoOggetto* Pieno = ValdorsoOggetti::Trova(FName(Id));
		TestTrue(FString::Printf(TEXT("%s: si beve e torna secchio"), Id), Pieno && Pieno->Uso == EValdorsoUso::Bevi && Pieno->Cariche > 0
			&& FName(Pieno->Vuoto) == FName(TEXT("Secchio")));
	}
	// entrando nell'acqua si riempie la borraccia, non il secchio (la sua acqua ha una qualita')
	FValdorsoZaino Zaino;
	Zaino.Posti.SetNum(4);
	Zaino.Posti[0].Id = TEXT("Secchio_Acqua_Pura");
	Zaino.Posti[0].Quantita = 1;
	Zaino.Posti[0].Cariche = 2;
	Zaino.Posti[1].Id = TEXT("Borraccia");
	Zaino.Posti[1].Quantita = 1;
	Zaino.Posti[1].Cariche = 0;
	TestTrue(TEXT("la borraccia si riempie"), ValdorsoZaino::RiempiBorracce(Zaino));
	TestEqual(TEXT("il secchio resta com'era"), Zaino.Posti[0].Cariche, 2);
	return true;
}

// (v0.1.9 - v0.2.0) Mani e materiali: gli attrezzi per raccogliere, i nuovi materiali, le scorte del villaggio.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRitiMateriali, "Valdorso.Riti.Materiali", ValdorsoTestRiti::Bandiere)
bool FValdorsoTestRitiMateriali::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("la legna con l'accetta"), ValdorsoRaccolta::Attrezzo(TEXT("Legna")) == FName(TEXT("Accetta")));
	TestTrue(TEXT("la pietra con il piccone"), ValdorsoRaccolta::Attrezzo(TEXT("Pietra_Grezza")) == FName(TEXT("Piccone")));
	TestTrue(TEXT("il ferro con il piccone"), ValdorsoRaccolta::Attrezzo(TEXT("Ferro_Grezzo")) == FName(TEXT("Piccone")));
	TestTrue(TEXT("i funghi a mani nude"), ValdorsoRaccolta::Attrezzo(TEXT("Funghi")).IsNone());
	for (const TCHAR* Id : { TEXT("Legna"), TEXT("Pietra_Grezza"), TEXT("Ferro_Grezzo"), TEXT("Asse"), TEXT("Lingotto_Ferro"), TEXT("Chiodi"), TEXT("Pietra_Squadrata") })
	{
		TestNotNull(FString::Printf(TEXT("c'e' %s"), Id), ValdorsoOggetti::Trova(FName(Id)));
	}
	// in valle si trovano: almeno un albero secco, un masso, un filone
	int32 Legna = 0;
	int32 Pietra = 0;
	int32 Ferro = 0;
	for (const FValdorsoPuntoRaccolta& P : ValdorsoRaccolta::Punti())
	{
		Legna += P.Oggetto == FName(TEXT("Legna")) ? 1 : 0;
		Pietra += P.Oggetto == FName(TEXT("Pietra_Grezza")) ? 1 : 0;
		Ferro += P.Oggetto == FName(TEXT("Ferro_Grezzo")) ? 1 : 0;
	}
	TestTrue(TEXT("alberi secchi in valle"), Legna > 0);
	TestTrue(TEXT("massi in valle"), Pietra > 0);
	TestTrue(TEXT("filoni di ferro in valle"), Ferro > 0);

	// le scorte: dieci minuti = un po' di grano e di legna; d'inverno la legna il doppio; mai sotto zero
	FValdorsoScorteValle S;
	S.Grano = 100.f;
	S.Legna = 100.f;
	S.UltimoConsumo = 1000000;
	ValdorsoScorte::Consuma(S, 1000000 + 600 * 6, false);
	TestEqual(TEXT("un'ora: 6 di grano"), S.Grano, 94.f);
	TestEqual(TEXT("un'ora: 6 di legna"), S.Legna, 94.f);
	ValdorsoScorte::Consuma(S, 1000000 + 600 * 12, true);
	TestEqual(TEXT("d'inverno la legna il doppio"), S.Legna, 82.f);
	ValdorsoScorte::Consuma(S, 1000000 + 600 * 12 + 599, true);
	TestEqual(TEXT("meno di dieci minuti: niente"), S.Grano, 88.f);
	S.Grano = 0.5f;
	ValdorsoScorte::Consuma(S, S.UltimoConsumo + 6000, false);
	TestEqual(TEXT("mai sotto zero"), S.Grano, 0.f);
	TestTrue(TEXT("la legna si paga"), ValdorsoScorte::Prezzo(TEXT("Legna")) > 0);
	TestEqual(TEXT("i funghi no"), ValdorsoScorte::Prezzo(TEXT("Funghi")), 0);
	float Vale = 0.f;
	TestTrue(TEXT("la squadrata vale tre pietre"), ValdorsoScorte::Scorta(S, TEXT("Pietra_Squadrata"), Vale) == &S.Pietra && Vale == 3.f);
	TestEqual(TEXT("quattro righe al magazzino"), ValdorsoScorte::Righe(S).Num(), 4);

	// la carovana: un giorno ogni tre, sempre lo stesso per tutti
	int32 Giorni = 0;
	for (int64 g = 0; g < 30; ++g)
	{
		Giorni += ValdorsoCarovana::Giorno(g * 86400 + 3600, 3) ? 1 : 0;
	}
	TestEqual(TEXT("in trenta giorni, dieci carovane"), Giorni, 10);
	TestTrue(TEXT("ogni giorno, se OgniGiorni e' 1"), ValdorsoCarovana::Giorno(123456789, 1));
	TestNotNull(TEXT("c'e' il dialogo di Sibilla"), ValdorsoDialoghi::Trova(TEXT("Sibilla")));
	return true;
}

#endif
