// Valdorso - Test automatici della v0.2.4 "PNG veri" (14/11/2026, scritto da Claude): la fiducia dei PNG (le parole, la
// chiacchierata del giorno, i limiti), la valle che ricorda (i fatti, quelli di ieri, una volta sola per PNG), il file
// del personaggio che li tiene (anche con i doppioni), la colpa (quando un PNG cade) e il dialogo dell'arresto.
// Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.PNG").

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoDialoghi.h"
#include "ValdorsoFiducia.h"
#include "ValdorsoTaccuinoDati.h"
#include "JsonObjectConverter.h"

namespace ValdorsoTestPNGVeri
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Un giorno qualunque, a mezzogiorno (UTC). */
	constexpr int64 Oggi = 1794657600;
	constexpr int64 UnGiorno = 86400;
}

// Le parole della fiducia, i limiti, la chiacchierata (una al giorno, fino a Cordiale).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPngFiducia, "Valdorso.PNG.Fiducia", ValdorsoTestPNGVeri::Bandiere)
bool FValdorsoTestPngFiducia::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestPNGVeri;
	TestEqual(TEXT("zero: neutrale"), FString(ValdorsoFiducia::Parola(ValdorsoFiducia::Livello(0))), FString(TEXT("Neutrale")));
	TestEqual(TEXT("-30: diffidente"), FString(ValdorsoFiducia::Parola(ValdorsoFiducia::Livello(-30))), FString(TEXT("Diffidente")));
	TestEqual(TEXT("20: cordiale"), FString(ValdorsoFiducia::Parola(ValdorsoFiducia::Livello(20))), FString(TEXT("Cordiale")));
	TestEqual(TEXT("50: amico"), FString(ValdorsoFiducia::Parola(ValdorsoFiducia::Livello(50))), FString(TEXT("Amico")));
	TestEqual(TEXT("90: di famiglia"), FString(ValdorsoFiducia::Parola(ValdorsoFiducia::Livello(90))), FString(TEXT("Di famiglia")));
	for (int32 f = ValdorsoFiducia::Minima; f < ValdorsoFiducia::Massima; ++f)
	{
		if (static_cast<uint8>(ValdorsoFiducia::Livello(f + 1)) < static_cast<uint8>(ValdorsoFiducia::Livello(f)))
		{
			AddError(FString::Printf(TEXT("la parola scende salendo, a %d"), f));
		}
	}
	TestEqual(TEXT("non sotto -100"), ValdorsoFiducia::Applica(-90, -50), ValdorsoFiducia::Minima);
	TestEqual(TEXT("non sopra 100"), ValdorsoFiducia::Applica(90, 50), ValdorsoFiducia::Massima);
	TestEqual(TEXT("chiacchierare non porta oltre il tetto"), ValdorsoFiducia::DopoChiacchiera(ValdorsoFiducia::TettoChiacchiere - 1), ValdorsoFiducia::TettoChiacchiere);
	TestEqual(TEXT("chi e' gia' amico resta amico"), ValdorsoFiducia::DopoChiacchiera(60), 60);
	TestEqual(TEXT("chi non si fida, parlando, si riavvicina"), ValdorsoFiducia::DopoChiacchiera(-40), -40 + ValdorsoFiducia::PerChiacchiera);

	FValdorsoTaccuino T;
	const FName Bruno(TEXT("Bruno"));
	const int64 Giorno = ValdorsoFiducia::Giorno(Oggi);
	TestEqual(TEXT("all'inizio zero"), ValdorsoFiducia::Di(T, Bruno), 0);
	TestTrue(TEXT("la prima chiacchierata del giorno conta"), ValdorsoFiducia::Chiacchiera(T, Bruno, Giorno));
	TestFalse(TEXT("la seconda dello stesso giorno no"), ValdorsoFiducia::Chiacchiera(T, Bruno, Giorno));
	TestEqual(TEXT("e' salita"), ValdorsoFiducia::Di(T, Bruno), ValdorsoFiducia::PerChiacchiera);
	TestTrue(TEXT("il giorno dopo si'"), ValdorsoFiducia::Chiacchiera(T, Bruno, Giorno + 1));
	TestEqual(TEXT("le maiuscole non contano"), ValdorsoFiducia::Di(T, FName(TEXT("bruno"))), 2 * ValdorsoFiducia::PerChiacchiera);
	const FName Gaspare(TEXT("Gaspare"));
	TestTrue(TEXT("colpito: scende"), ValdorsoFiducia::Cambia(T, Gaspare, ValdorsoFiducia::PerColpito));
	TestTrue(TEXT("diffidente dopo un pugno"), ValdorsoFiducia::Livello(ValdorsoFiducia::Di(T, Gaspare)) == EValdorsoFiducia::Diffidente);
	TestTrue(TEXT("anche chi era un po' amico scende"), ValdorsoFiducia::Cambia(T, Bruno, ValdorsoFiducia::PerColpito) && ValdorsoFiducia::Di(T, Bruno) < 0);
	TestFalse(TEXT("cambiare di zero non e' cambiare"), ValdorsoFiducia::Cambia(T, Bruno, 0));
	TestFalse(TEXT("senza nome niente"), ValdorsoFiducia::Cambia(T, NAME_None, 5));
	TestEqual(TEXT("un altro PNG non sa niente"), ValdorsoFiducia::Di(T, FName(TEXT("Marta"))), 0);
	TestFalse(TEXT("chi non si fida ha il suo saluto"), ValdorsoFiducia::SalutoDellaFiducia(EValdorsoFiducia::Diffidente, TEXT("Fra"), 1).IsEmpty());
	TestTrue(TEXT("il neutrale usa quello solito"), ValdorsoFiducia::SalutoDellaFiducia(EValdorsoFiducia::Neutrale, TEXT("Fra"), 1).IsEmpty());
	for (int32 Seme = 0; Seme < 6; ++Seme)
	{
		TestTrue(TEXT("l'amico dice il nome"), ValdorsoFiducia::SalutoDellaFiducia(EValdorsoFiducia::Amico, TEXT("Fra"), Seme).Contains(TEXT("Fra")));
		TestTrue(TEXT("anche quello di famiglia"), ValdorsoFiducia::SalutoDellaFiducia(EValdorsoFiducia::DiFamiglia, TEXT("Fra"), Seme).Contains(TEXT("Fra")));
		TestFalse(TEXT("le grida ci sono"), ValdorsoFiducia::Grido(Seme % 2 == 0, Seme % 3 == 0, Seme).IsEmpty());
	}
	return true;
}

// La valle che ricorda: un fatto di ieri, detto una volta da ogni PNG; quelli di oggi aspettano domani; quelli troppo
// vecchi no; uno per tipo e per giorno.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPngRicordi, "Valdorso.PNG.Ricordi", ValdorsoTestPNGVeri::Bandiere)
bool FValdorsoTestPngRicordi::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestPNGVeri;
	TestTrue(TEXT("la caccia si ricorda"), ValdorsoFiducia::Memorabile(TEXT("Animale")));
	TestTrue(TEXT("la colpa si ricorda"), ValdorsoFiducia::Memorabile(TEXT("Colpa")));
	TestFalse(TEXT("un passo nel dialogo no"), ValdorsoFiducia::Memorabile(TEXT("Dialogo")));
	TestFalse(TEXT("sedersi no"), ValdorsoFiducia::Memorabile(TEXT("Siede")));

	FValdorsoTaccuino T;
	const FName Bruno(TEXT("Bruno"));
	const FName Marta(TEXT("Marta"));
	FValdorsoFatto Fatto;
	TestFalse(TEXT("il dialogo non diventa un fatto"), ValdorsoFiducia::AnnotaFatto(T, TEXT("Dialogo"), Oggi - UnGiorno));
	TestTrue(TEXT("la caccia di ieri"), ValdorsoFiducia::AnnotaFatto(T, TEXT("Animale"), Oggi - UnGiorno));
	TestTrue(TEXT("piu' tardi, lo stesso giorno: si aggiorna"), ValdorsoFiducia::AnnotaFatto(T, TEXT("Animale"), Oggi - UnGiorno + 600));
	TestEqual(TEXT("uno per tipo e per giorno"), T.Fatti.Num(), 1);
	TestTrue(TEXT("la storia di oggi"), ValdorsoFiducia::AnnotaFatto(T, TEXT("Storia"), Oggi - 60));

	TestTrue(TEXT("Bruno ha qualcosa da dire"), ValdorsoFiducia::FattoDaRicordare(T, Bruno, Oggi, Fatto));
	TestEqual(TEXT("la caccia (la storia e' di oggi: domani)"), Fatto.Tipo, FString(TEXT("Animale")));
	const FString Riga = ValdorsoFiducia::RigaDelFatto(Fatto, Oggi, 3);
	TestTrue(TEXT("dice ieri"), Riga.StartsWith(TEXT("Ieri")));
	TestFalse(TEXT("senza segnaposto"), Riga.Contains(TEXT("{")));
	TestTrue(TEXT("Bruno l'ha detto"), ValdorsoFiducia::SegnaRicordato(T, Bruno, Fatto.Quando));
	TestFalse(TEXT("Bruno non lo ripete"), ValdorsoFiducia::FattoDaRicordare(T, Bruno, Oggi, Fatto));
	TestTrue(TEXT("Marta invece si'"), ValdorsoFiducia::FattoDaRicordare(T, Marta, Oggi, Fatto));

	// domani: la storia di oggi diventa "ieri"; la caccia "l'altro giorno"
	TestTrue(TEXT("domani Bruno parla della storia"), ValdorsoFiducia::FattoDaRicordare(T, Bruno, Oggi + UnGiorno, Fatto));
	TestEqual(TEXT("la storia"), Fatto.Tipo, FString(TEXT("Storia")));
	TestTrue(TEXT("Marta, domani, sceglie la piu' nuova"), ValdorsoFiducia::FattoDaRicordare(T, Marta, Oggi + UnGiorno, Fatto) && Fatto.Tipo == TEXT("Storia"));
	FValdorsoFatto Caccia;
	Caccia.Tipo = TEXT("Animale");
	Caccia.Quando = Oggi - UnGiorno;
	TestTrue(TEXT("due giorni fa: l'altro giorno"), ValdorsoFiducia::RigaDelFatto(Caccia, Oggi + UnGiorno, 1).StartsWith(TEXT("L'altro giorno")));
	TestFalse(TEXT("dopo troppi giorni non se ne parla piu'"),
		ValdorsoFiducia::FattoDaRicordare(T, Marta, Oggi + (ValdorsoFiducia::GiorniRicordo + 1) * UnGiorno, Fatto));

	// ogni tipo memorabile ha le sue righe, senza segnaposto rimasti
	for (const TCHAR* Tipo : { TEXT("Animale"), TEXT("Scheletro"), TEXT("Storia"), TEXT("Cerimonia"), TEXT("Sogno"), TEXT("Morte"),
		TEXT("Forziere"), TEXT("PortaSegreta"), TEXT("Trappola"), TEXT("Crescere"), TEXT("Passaggio"), TEXT("Gatto"), TEXT("Magazzino"),
		TEXT("Colpa"), TEXT("Multa"), TEXT("Cella") })
	{
		FValdorsoFatto Uno;
		Uno.Tipo = Tipo;
		Uno.Quando = Oggi - UnGiorno;
		for (int32 Seme = 0; Seme < 4; ++Seme)
		{
			const FString Detta = ValdorsoFiducia::RigaDelFatto(Uno, Oggi, Seme);
			TestTrue(FString::Printf(TEXT("%s: una riga da chat"), Tipo), Detta.Len() > 10 && Detta.Len() <= 200 && !Detta.Contains(TEXT("{")));
		}
	}

	// non piu' di MassimoFatti (via i piu' vecchi)
	FValdorsoTaccuino Pieno;
	for (int32 i = 0; i < ValdorsoFiducia::MassimoFatti + 10; ++i)
	{
		ValdorsoFiducia::AnnotaFatto(Pieno, TEXT("Animale"), Oggi - (ValdorsoFiducia::MassimoFatti + 10 - i) * UnGiorno);
	}
	TestEqual(TEXT("al massimo"), Pieno.Fatti.Num(), ValdorsoFiducia::MassimoFatti);
	TestTrue(TEXT("restano i piu' nuovi"), Pieno.Fatti.Last().Quando == Oggi - UnGiorno);
	return true;
}

// Il file del personaggio: la fiducia e i fatti si salvano, si rileggono e Sistema li tiene (anche con i doppioni).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPngFile, "Valdorso.PNG.File", ValdorsoTestPNGVeri::Bandiere)
bool FValdorsoTestPngFile::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestPNGVeri;
	FValdorsoTaccuino T;
	ValdorsoFiducia::Cambia(T, TEXT("Bruno"), 42);
	ValdorsoFiducia::Chiacchiera(T, TEXT("Marta"), ValdorsoFiducia::Giorno(Oggi));
	ValdorsoFiducia::SegnaRicordato(T, TEXT("Marta"), Oggi - 100);
	ValdorsoFiducia::AnnotaFatto(T, TEXT("Morte"), Oggi - UnGiorno);
	FString Json;
	TestTrue(TEXT("si scrive"), FJsonObjectConverter::UStructToJsonObjectString(T, Json));
	FValdorsoTaccuino Letto;
	TestTrue(TEXT("si rilegge"), FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Letto, 0, 0));
	ValdorsoTaccuino::Sistema(Letto);
	TestEqual(TEXT("la fiducia di Bruno resta"), ValdorsoFiducia::Di(Letto, TEXT("Bruno")), 42);
	TestEqual(TEXT("quella di Marta pure"), ValdorsoFiducia::Di(Letto, TEXT("Marta")), ValdorsoFiducia::PerChiacchiera);
	TestFalse(TEXT("e la chiacchierata di oggi e' gia' fatta"), ValdorsoFiducia::Chiacchiera(Letto, TEXT("Marta"), ValdorsoFiducia::Giorno(Oggi)));
	TestEqual(TEXT("il fatto resta"), Letto.Fatti.Num(), 1);

	// un file con doppioni e valori strani
	FValdorsoTaccuino Storto;
	FValdorsoRicordoDialogo A;
	A.Dialogo = TEXT("Bruno");
	A.Fiducia = 10;
	FValdorsoRicordoDialogo B;
	B.Dialogo = TEXT(" bruno ");
	B.Fiducia = -500;
	B.UltimoRicordo = 77;
	Storto.Dialoghi = { A, B };
	FValdorsoFatto Ignoto;
	Ignoto.Tipo = TEXT("NonEsiste");
	Ignoto.Quando = Oggi;
	FValdorsoFatto Vuoto;
	Vuoto.Tipo = TEXT("Morte");
	Vuoto.Quando = 0;
	Storto.Fatti = { Ignoto, Vuoto };
	ValdorsoTaccuino::Sistema(Storto);
	TestEqual(TEXT("i doppioni si uniscono"), Storto.Dialoghi.Num(), 1);
	TestEqual(TEXT("la fiducia resta nei limiti (e vale quella piu' forte)"), ValdorsoFiducia::Di(Storto, TEXT("Bruno")), ValdorsoFiducia::Minima);
	TestEqual(TEXT("il ricordo piu' nuovo resta"), Storto.Dialoghi[0].UltimoRicordo, static_cast<int64>(77));
	TestEqual(TEXT("via i fatti sconosciuti o senza data"), Storto.Fatti.Num(), 0);
	return true;
}

// La colpa: quando un PNG cade, e il dialogo dell'arresto (multa e cella).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPngColpa, "Valdorso.PNG.Colpa", ValdorsoTestPNGVeri::Bandiere)
bool FValdorsoTestPngColpa::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("un pugno: barcolla"), ValdorsoFiducia::Cade(8.f, 8.f));
	TestTrue(TEXT("un colpo forte: cade"), ValdorsoFiducia::Cade(25.f, 25.f));
	TestTrue(TEXT("tanti pugni: cade"), ValdorsoFiducia::Cade(32.f, 8.f));
	TestTrue(TEXT("la colpa dura piu' dell'attesa delle guardie"), ValdorsoFiducia::SecondiColpa > ValdorsoFiducia::SecondiPrimaDelleGuardie + ValdorsoFiducia::SecondiDopoLaFuga);
	TestTrue(TEXT("chi vede perde meno fiducia di chi e' colpito"), ValdorsoFiducia::PerTestimone > ValdorsoFiducia::PerColpito);
	TestTrue(TEXT("un pugno basta per diventare diffidente"), ValdorsoFiducia::Livello(ValdorsoFiducia::PerColpito) == EValdorsoFiducia::Diffidente);

	const FValdorsoDialogo* Arresto = ValdorsoDialoghi::Trova(FName(ValdorsoFiducia::DialogoArresto));
	if (!TestNotNull(TEXT("c'e' il dialogo dell'arresto"), Arresto))
	{
		return false;
	}
	TestEqual(TEXT("senza problemi"), ValdorsoDialoghi::Problemi(*Arresto).Num(), 0);
	bool bMulta = false;
	bool bCella = false;
	bool bNonBasta = false;
	for (const FValdorsoNodoDialogo& Nodo : Arresto->Nodi)
	{
		bNonBasta = bNonBasta || Nodo.Id == FName(TEXT("non_basta"));
		for (const FValdorsoSceltaDialogo& Scelta : Nodo.Scelte)
		{
			bMulta = bMulta || (Scelta.Evento == FName(TEXT("Multa")) && Scelta.Verso == FName(TEXT("multa")));
			bCella = bCella || (Scelta.Evento == FName(TEXT("Cella")) && Scelta.Verso == FName(TEXT("cella")));
		}
	}
	TestTrue(TEXT("si puo' pagare la multa"), bMulta);
	TestTrue(TEXT("si puo' scegliere la cella"), bCella);
	TestTrue(TEXT("c'e' il nodo di chi non ha le monete"), bNonBasta);
	TestTrue(TEXT("il prezzo e' scritto giusto"), FString::Join(Arresto->Trova(Arresto->Inizio)->Righe, TEXT(" ")).Contains(TEXT("venti Aureus")) && ValdorsoFiducia::Multa == 20);
	return true;
}

#endif
