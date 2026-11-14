// Valdorso - Test automatici delle storie del Battito nel gioco (v0.2.4 "PNG veri", blocco 3, 14/11/2026, scritto da
// Claude): una storia del Sognatore si legge (con la sua finestra, o senza), i mattoni che il gioco sa fare, i nomi dei PNG
// e dei luoghi delle storie, i gradi dei premi, gli inneschi, lo stato salvato nel file del personaggio.
// Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.Battito").

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoBattito.h"
#include "ValdorsoCustode.h"
#include "ValdorsoQualita.h"
#include "ValdorsoTaccuinoDati.h"
#include "JsonObjectConverter.h"

namespace ValdorsoTestBattito
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Una storia come la scrive il Sognatore ("Le lucine della palude", accorciata). */
	const TCHAR* Lucine = TEXT(R"({
 "id": "fuochi_fatui_91f122", "seme": "fuochi_fatui", "colono": "Fra", "titolo": "Le lucine della palude",
 "rarita": "insolita", "difficolta": 2, "patente": 1, "png": "Nonna Edda", "luogo": "Palude", "zona": "",
 "innesco": [{"tipo": "luogo", "luogo": "Palude", "ora": "notte"}],
 "aggancio": "Sulla palude danzano tre lucine azzurre.",
 "passi": [
  {"mattone": "parla", "testo": "Chiedi a Nonna Edda cosa sono le lucine.", "png": "Nonna Edda"},
  {"mattone": "segui", "testo": "Segui le lucine contando i passi.", "luogo": "Palude"},
  {"mattone": "scegli", "testo": "Le lucine si fermano su un ciuffo d'erba.", "png": "Nonna Edda"}],
 "finali": [
  {"scelta": "Raccogli l'erba.", "esito": "Un'erba che non si vedeva da vent'anni.",
   "premio": {"oggetti": ["Erbe_Palude"], "monete": 10, "segno": "ErbaDeiFuochi"}},
  {"scelta": "Saluti le lucine.", "esito": "Le lucine si spengono.", "premio": {"segno": "SalutoAiFuochi", "iscrizione": "Le tre lucine"},
   "effetti": [{"tipo": "fiducia", "png": "Nonna Edda", "quanto": 1}]}],
 "rifiuto": {"testo": "Le lucine si spostano verso qualcun altro.", "effetti": [{"tipo": "passa"}]},
 "serie": null,
 "finestra": {"intestazione": "Il Battito ha intrecciato una storia", "rarita": "insolita", "colore": "#5f8a5a", "stelle": 2,
  "titolo": "Le lucine della palude", "trama": "Sulla palude danzano tre lucine azzurre.", "primo_passo": "Chiedi a Nonna Edda cosa sono le lucine.",
  "premi": "10 monete, Erbe della palude / oppure / niente", "se_rifiuti": "Le lucine si spostano verso qualcun altro.",
  "scadenza_giorni": 7, "bottoni": ["Accetta", "Rifiuta", "Ci penso"], "serie": ""}
})");
}

// Una storia del Sognatore si legge tutta: passi, finali, premi, effetti, rifiuto e la finestra.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestBattitoLeggi, "Valdorso.Battito.Leggi", ValdorsoTestBattito::Bandiere)
bool FValdorsoTestBattitoLeggi::RunTest(const FString& Parameters)
{
	FValdorsoStoriaScritta S;
	FString Errore;
	if (!TestTrue(TEXT("si legge"), ValdorsoBattito::Leggi(ValdorsoTestBattito::Lucine, S, Errore)))
	{
		AddError(Errore);
		return false;
	}
	TestEqual(TEXT("l'id"), S.Id, FString(TEXT("fuochi_fatui_91f122")));
	TestEqual(TEXT("la rarita'"), S.Rarita, FString(TEXT("insolita")));
	TestEqual(TEXT("tre passi"), S.Passi.Num(), 3);
	TestEqual(TEXT("il secondo e' segui"), S.Passi[1].Mattone, FString(TEXT("segui")));
	TestEqual(TEXT("due finali"), S.Finali.Num(), 2);
	TestEqual(TEXT("le monete del primo"), S.Finali[0].Monete, 10);
	TestTrue(TEXT("l'oggetto del primo"), S.Finali[0].Oggetti.Num() == 1 && S.Finali[0].Oggetti[0] == TEXT("Erbe_Palude"));
	TestEqual(TEXT("l'iscrizione del secondo"), S.Finali[1].Iscrizione, FString(TEXT("Le tre lucine")));
	TestTrue(TEXT("l'effetto del secondo"), S.Finali[1].Effetti.Num() == 1 && S.Finali[1].Effetti[0].Quanto == 1);
	TestEqual(TEXT("un innesco di notte"), S.Inneschi.Num() == 1 ? S.Inneschi[0].Ora : FString(), FString(TEXT("notte")));
	TestTrue(TEXT("il rifiuto"), !S.RifiutoTesto.IsEmpty() && S.RifiutoEffetti.Num() == 1);
	TestEqual(TEXT("la finestra: il colore"), S.Finestra.Colore, FString(TEXT("#5f8a5a")));
	TestEqual(TEXT("la finestra: le stelle"), S.Finestra.Stelle, 2);
	TestEqual(TEXT("la finestra: l'id"), S.Finestra.Id, S.Id);
	TestTrue(TEXT("si puo' giocare"), ValdorsoBattito::Giocabile(S));

	// senza la finestra: la si fa dai campi della storia
	FString SenzaFinestra = ValdorsoTestBattito::Lucine;
	const int32 Dove = SenzaFinestra.Find(TEXT(",\n \"finestra\""));
	if (TestTrue(TEXT("(la prova toglie la finestra)"), Dove != INDEX_NONE))
	{
		SenzaFinestra = SenzaFinestra.Left(Dove) + TEXT("\n}");
		FValdorsoStoriaScritta Nuda;
		TestTrue(TEXT("si legge anche senza finestra"), ValdorsoBattito::Leggi(SenzaFinestra, Nuda, Errore));
		TestEqual(TEXT("la trama e' l'aggancio"), Nuda.Finestra.Trama, Nuda.Aggancio);
		TestEqual(TEXT("il primo passo"), Nuda.Finestra.PrimoPasso, Nuda.Passi[0].Testo);
		TestEqual(TEXT("la scadenza della rarita'"), Nuda.Finestra.ScadenzaGiorni, 7);
		TestFalse(TEXT("l'intestazione c'e'"), Nuda.Finestra.Intestazione.IsEmpty());
	}

	FValdorsoStoriaScritta Rotta;
	TestFalse(TEXT("un file rotto non si legge"), ValdorsoBattito::Leggi(TEXT("{ non e' json"), Rotta, Errore));
	TestFalse(TEXT("senza id non si legge"), ValdorsoBattito::Leggi(TEXT("{\"titolo\": \"x\"}"), Rotta, Errore));
	return true;
}

// I mattoni, i nomi dei PNG e dei luoghi, i gradi dei premi, gli inneschi da evento.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestBattitoRegole, "Valdorso.Battito.Regole", ValdorsoTestBattito::Bandiere)
bool FValdorsoTestBattitoRegole::RunTest(const FString& Parameters)
{
	for (const TCHAR* M : { TEXT("parla"), TEXT("porta"), TEXT("veglia"), TEXT("scegli"), TEXT("offerta"), TEXT("segui"), TEXT("fantasma") })
	{
		TestTrue(FString::Printf(TEXT("%s c'e'"), M), ValdorsoBattito::MattoneOggi(M));
	}
	for (const TCHAR* M : { TEXT("tasca"), TEXT("eco"), TEXT("png_nuovo"), TEXT("scrivi"), TEXT("stagione") })
	{
		TestFalse(FString::Printf(TEXT("%s non ancora"), M), ValdorsoBattito::MattoneOggi(M));
	}
	FValdorsoStoriaScritta S;
	FString Errore;
	ValdorsoBattito::Leggi(ValdorsoTestBattito::Lucine, S, Errore);
	S.Passi[1].Mattone = TEXT("tasca");
	TestFalse(TEXT("con un mattone che non c'e' la storia aspetta"), ValdorsoBattito::Giocabile(S));
	S.Passi.Reset();
	TestFalse(TEXT("senza passi non si gioca"), ValdorsoBattito::Giocabile(S));

	TestTrue(TEXT("Nonna Edda"), ValdorsoBattito::DialogoDelPng(TEXT("Nonna Edda")) == FName(TEXT("NonnaEdda")));
	TestTrue(TEXT("Tebaldo e' la guardia Corrado"), ValdorsoBattito::DialogoDelPng(TEXT("Tebaldo")) == FName(TEXT("GuardiaCorrado")));
	TestTrue(TEXT("Livia Serra"), ValdorsoBattito::DialogoDelPng(TEXT("Livia Serra")) == FName(TEXT("Livia")));
	TestTrue(TEXT("anche con il nome del dialogo"), ValdorsoBattito::DialogoDelPng(TEXT("PadreAnselmo")) == FName(TEXT("PadreAnselmo")));
	TestTrue(TEXT("chi non c'e' (Vera)"), ValdorsoBattito::DialogoDelPng(TEXT("Vera")).IsNone());
	TestTrue(TEXT("la locanda e' Bruno"), ValdorsoBattito::PngDelLuogo(TEXT("Locanda di Bruno")) == FName(TEXT("Bruno")));
	TestEqual(TEXT("il pozzo e' la piazza"), ValdorsoBattito::LuogoDelTaccuino(TEXT("Pozzo")), FString(TEXT("Piazza del villaggio")));
	TestEqual(TEXT("la palude e' nel taccuino"), ValdorsoBattito::LuogoDelTaccuino(TEXT("Palude")), FString(TEXT("Palude")));
	TestTrue(TEXT("la fucina no (e' Marta)"), ValdorsoBattito::LuogoDelTaccuino(TEXT("Fucina")).IsEmpty());

	TestTrue(TEXT("comune: premi comuni"), ValdorsoBattito::QualitaDelPremio(TEXT("comune")) == ValdorsoQualita::Comune);
	TestTrue(TEXT("rara: premi rari"), ValdorsoBattito::QualitaDelPremio(TEXT("rara")) == ValdorsoQualita::Rara);
	TestTrue(TEXT("epica: premi epici"), ValdorsoBattito::QualitaDelPremio(TEXT("epica")) == ValdorsoQualita::Epica);
	TestEqual(TEXT("le epiche scadono prima"), ValdorsoBattito::ScadenzaDi(TEXT("epica")), 3);
	TestEqual(TEXT("il nome"), ValdorsoBattito::NomeRarita(TEXT("insolita")), FString(TEXT("Insolita")));
	TestEqual(TEXT("il nome della rara"), ValdorsoBattito::NomeRarita(TEXT("rara")), FString(TEXT("Rara")));
	const FLinearColor Blu = ValdorsoBattito::ColoreRarita(TEXT("rara"), TEXT("#4a6fa5"));
	TestTrue(TEXT("il colore della finestra"), Blu.B > Blu.R);
	TestTrue(TEXT("senza colore: quello del grado"), ValdorsoBattito::ColoreRarita(TEXT("rara"), FString()).Equals(ValdorsoQualita::Colore(ValdorsoQualita::Rara)));

	FValdorsoStoriaScritta::FInnesco Dialogo;
	Dialogo.Tipo = TEXT("dialogo");
	Dialogo.Png = TEXT("Agnese");
	TestTrue(TEXT("parlare con Agnese"), ValdorsoBattito::InnescoDaEvento(Dialogo, TEXT("Dialogo"), FName(TEXT("Agnese"))));
	TestFalse(TEXT("parlare con Bruno no"), ValdorsoBattito::InnescoDaEvento(Dialogo, TEXT("Dialogo"), FName(TEXT("Bruno"))));
	FValdorsoStoriaScritta::FInnesco Evento;
	Evento.Tipo = TEXT("evento");
	Evento.Evento = TEXT("Animale");
	TestTrue(TEXT("un evento del registro"), ValdorsoBattito::InnescoDaEvento(Evento, TEXT("Animale"), NAME_None));
	TestFalse(TEXT("un altro no"), ValdorsoBattito::InnescoDaEvento(Evento, TEXT("Siede"), NAME_None));
	FValdorsoStoriaScritta::FInnesco Sogno;
	Sogno.Tipo = TEXT("sogno");
	TestTrue(TEXT("dopo un sogno"), ValdorsoBattito::InnescoDaEvento(Sogno, TEXT("Sogno"), NAME_None));

	TestEqual(TEXT("la cartella del colono"), ValdorsoBattito::NomeCartella(TEXT("Fra d'Orso")), FString(TEXT("Fra d_Orso")));
	ValdorsoBattito::Leggi(ValdorsoTestBattito::Lucine, S, Errore);
	TestTrue(TEXT("il passo con l'aiuto"), ValdorsoBattito::TestoPasso(S, 0).Contains(TEXT("(da Nonna Edda)")));
	TestTrue(TEXT("il passo della lucina"), ValdorsoBattito::TestoPasso(S, 1).Contains(TEXT("stalle vicino")));
	return true;
}

// Lo stato delle storie nel file del personaggio: si salva, si rilegge, Sistema toglie doppioni e valori strani.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestBattitoFile, "Valdorso.Battito.File", ValdorsoTestBattito::Bandiere)
bool FValdorsoTestBattitoFile::RunTest(const FString& Parameters)
{
	FValdorsoTaccuino T;
	FValdorsoStoriaViva A;
	A.Id = TEXT("fuochi_fatui_91f122");
	A.Seme = TEXT("fuochi_fatui");
	A.Stato = static_cast<uint8>(EValdorsoStatoStoria::Accettata);
	A.Passo = 1;
	A.Offerta = 1794657600;
	FValdorsoStoriaViva Doppia = A;
	Doppia.Passo = 2;
	FValdorsoStoriaViva Strana;
	Strana.Id = TEXT("strana");
	Strana.Stato = 200;
	Strana.Passo = 9999;
	FValdorsoStoriaViva Vuota;
	T.Storie = { A, Doppia, Strana, Vuota };
	FString Json;
	TestTrue(TEXT("si scrive"), FJsonObjectConverter::UStructToJsonObjectString(T, Json));
	FValdorsoTaccuino Letto;
	TestTrue(TEXT("si rilegge"), FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Letto, 0, 0));
	ValdorsoTaccuino::Sistema(Letto);
	TestEqual(TEXT("via il doppione e quella senza id"), Letto.Storie.Num(), 2);
	if (Letto.Storie.Num() == 2)
	{
		TestEqual(TEXT("vale la prima"), Letto.Storie[0].Passo, 1);
		TestEqual(TEXT("il seme resta"), Letto.Storie[0].Seme, FString(TEXT("fuochi_fatui")));
		TestTrue(TEXT("lo stato nei limiti"), Letto.Storie[1].Stato <= static_cast<uint8>(EValdorsoStatoStoria::Scaduta));
		TestTrue(TEXT("il passo nei limiti"), Letto.Storie[1].Passo <= 64);
	}
	return true;
}

// Il custode del Battito: le sue regole (contare, sommare le salite, i passi troppo veloci, gli spostamenti di colpo).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestBattitoCustode, "Valdorso.Battito.Custode", ValdorsoTestBattito::Bandiere)
bool FValdorsoTestBattitoCustode::RunTest(const FString& Parameters)
{
	TArray<double> Momenti = { 1.0, 2.0, 50.0, 95.0, 99.0 };
	TestEqual(TEXT("negli ultimi 10 secondi"), ValdorsoCustode::Conta(Momenti, 100.0, 10.0), 2);
	TestEqual(TEXT("i vecchi se ne vanno"), Momenti.Num(), 2);
	TArray<TPair<double, int32>> Salite = { TPair<double, int32>(0.0, 300), TPair<double, int32>(3000.0, 100), TPair<double, int32>(3500.0, 250) };
	TestEqual(TEXT("le salite dell'ultima ora"), ValdorsoCustode::Salite(Salite, 3700.0, 3600.0), 350);
	TestFalse(TEXT("correre non e' troppo veloce"), ValdorsoCustode::TroppoVeloce(900.0, 1.0));
	TestTrue(TEXT("40 metri in un secondo si'"), ValdorsoCustode::TroppoVeloce(4000.0, 1.0));
	TestFalse(TEXT("senza tempo non si giudica"), ValdorsoCustode::TroppoVeloce(4000.0, 0.0));
	TestTrue(TEXT("la rinascita sposta di colpo"), ValdorsoCustode::SpostaDiColpo(TEXT("Rinascita")));
	TestTrue(TEXT("anche l'arresto"), ValdorsoCustode::SpostaDiColpo(TEXT("Arresto")));
	TestTrue(TEXT("anche lo staff"), ValdorsoCustode::SpostaDiColpo(TEXT("Staff")));
	TestFalse(TEXT("un colpo no"), ValdorsoCustode::SpostaDiColpo(TEXT("Colpo")));
	TestTrue(TEXT("una storia leggendaria non basta ad allarmare"), ValdorsoCustode::SogliaMonete > 250);
	return true;
}

#endif
