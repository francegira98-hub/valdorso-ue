// Valdorso - Test automatici di v0.1.6 "La valle che vive", parte 1 (11/11): le voci della locanda, i segnaposto dei
// dialoghi, l'Atto I, i sogni, il gatto. Scritto da Claude l'11/11/2026. Filtro "Valdorso.Vita".

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoGatto.h"
#include "ValdorsoPNG.h"
#include "ValdorsoSogni.h"
#include "ValdorsoVoci.h"

namespace ValdorsoTestVita
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

// Le voci: senza fatti una voce sola (la valle tranquilla); con tanti fatti al massimo tre, l'Atto I per primo; mai nomi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestVitaVoci, "Valdorso.Vita.Voci", ValdorsoTestVita::Bandiere)
bool FValdorsoTestVitaVoci::RunTest(const FString& Parameters)
{
	FValdorsoFattiValle Niente;
	const TArray<FString> Calma = ValdorsoVoci::DaiFatti(Niente);
	TestEqual(TEXT("senza fatti, una voce"), Calma.Num(), 1);

	FValdorsoFattiValle Tanti;
	Tanti.Atto = 1;
	Tanti.Frana = 2;
	Tanti.bCapitano = true;
	Tanti.Lupi = 4;
	Tanti.Orsi = 1;
	Tanti.Morti = 2;
	const TArray<FString> Voci = ValdorsoVoci::DaiFatti(Tanti);
	TestEqual(TEXT("al massimo tre voci"), Voci.Num(), 3);
	TestTrue(TEXT("l'Atto I per primo"), Voci.Num() > 0 && Voci[0].Contains(TEXT("frammento")));
	for (const FString& Voce : Voci)
	{
		TestTrue(TEXT("una voce non troppo lunga"), Voce.Len() > 20 && Voce.Len() <= 700);
	}

	FValdorsoFattiValle Lupi;
	Lupi.Lupi = 3;
	const TArray<FString> SoloLupi = ValdorsoVoci::DaiFatti(Lupi);
	TestTrue(TEXT("i lupi contati a parole"), SoloLupi.Num() == 1 && SoloLupi[0].Contains(TEXT("tre lupi")));
	return true;
}

// I segnaposto delle battute: {voci} diventa le voci, {se:atto1} e {senza:atto1} scelgono la riga giusta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestVitaSegnaposto, "Valdorso.Vita.Segnaposto", ValdorsoTestVita::Bandiere)
bool FValdorsoTestVitaSegnaposto::RunTest(const FString& Parameters)
{
	const TArray<FString> Base = { TEXT("Prima."), TEXT("{voci}"), TEXT("{se:atto1}Dopo il battito."), TEXT("{senza:atto1}Prima del battito."), TEXT("Fine.") };
	const TArray<FString> Voci = { TEXT("Voce uno."), TEXT("Voce due.") };

	TArray<FString> Prima = Base;
	ValdorsoVoci::RiempiCon(Prima, 0, Voci);
	TestEqual(TEXT("prima dell'Atto: cinque righe"), Prima.Num(), 5);
	TestTrue(TEXT("le voci al loro posto"), Prima.IsValidIndex(2) && Prima[1] == TEXT("Voce uno.") && Prima[2] == TEXT("Voce due."));
	TestTrue(TEXT("la riga di prima"), Prima.Contains(TEXT("Prima del battito.")));
	TestFalse(TEXT("non la riga di dopo"), Prima.Contains(TEXT("Dopo il battito.")));

	TArray<FString> Dopo = Base;
	ValdorsoVoci::RiempiCon(Dopo, 1, Voci);
	TestTrue(TEXT("la riga di dopo"), Dopo.Contains(TEXT("Dopo il battito.")));
	TestFalse(TEXT("non la riga di prima"), Dopo.Contains(TEXT("Prima del battito.")));
	for (const FString& Riga : Dopo)
	{
		TestFalse(TEXT("nessun segnaposto resta"), Riga.StartsWith(TEXT("{")));
	}
	return true;
}

// I sogni: il primo sempre; di giorno niente; il battito e i martelli una volta sola, prima degli altri.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestVitaSogni, "Valdorso.Vita.Sogni", ValdorsoTestVita::Bandiere)
bool FValdorsoTestVitaSogni::RunTest(const FString& Parameters)
{
	TSet<FName> Fatti;
	TestTrue(TEXT("il primo sogno, anche di giorno"), ValdorsoSogni::Scegli(true, false, 0.99f, 0, 0, Fatti, 0) == FName(TEXT("primo")));
	TestTrue(TEXT("di giorno niente"), ValdorsoSogni::Scegli(false, false, 0.f, 0, 0, Fatti, 0).IsNone());
	TestTrue(TEXT("una notte senza sogni"), ValdorsoSogni::Scegli(false, true, 0.9f, 0, 0, Fatti, 0).IsNone());
	TestTrue(TEXT("con l'Atto I, il battito"), ValdorsoSogni::Scegli(false, true, 0.f, 1, 2, Fatti, 0) == FName(TEXT("battito")));
	Fatti.Add(FName(TEXT("battito")));
	TestTrue(TEXT("poi i martelli"), ValdorsoSogni::Scegli(false, true, 0.f, 1, 2, Fatti, 0) == FName(TEXT("martelli")));
	Fatti.Add(FName(TEXT("martelli")));
	const FName Valle = ValdorsoSogni::Scegli(false, true, 0.f, 1, 2, Fatti, 4);
	TestTrue(TEXT("poi i sogni della valle"), Valle == FName(TEXT("luci")) || Valle == FName(TEXT("passo")) || Valle == FName(TEXT("collina")));
	TSet<FName> Id;
	for (const FValdorsoSogno& S : ValdorsoSogni::Tutti())
	{
		TestFalse(TEXT("Id doppio"), Id.Contains(FName(S.Id)));
		Id.Add(FName(S.Id));
		TestTrue(TEXT("Trova lo ritrova"), ValdorsoSogni::Trova(FName(S.Id)) == &S);
		TestTrue(TEXT("da tre a sei righe"), S.Righe.Num() >= 3 && S.Righe.Num() <= 6);
		for (const TCHAR* Riga : S.Righe)
		{
			TestTrue(TEXT("una riga del sogno non troppo lunga"), FCString::Strlen(Riga) > 10 && FCString::Strlen(Riga) <= 260);
		}
	}
	static const TCHAR* Necessari[] = { TEXT("primo"), TEXT("battito"), TEXT("martelli"), TEXT("luci"), TEXT("passo"), TEXT("collina") };
	for (const TCHAR* Necessario : Necessari)
	{
		TestTrue(FString(TEXT("c'e' il sogno ")) + Necessario, ValdorsoSogni::Trova(FName(Necessario)) != nullptr);
	}
	return true;
}

// Il gatto: un indizio alla volta, quello dell'Atto I solo dopo l'Atto I, poi niente; le pose sono tre.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestVitaGatto, "Valdorso.Vita.Gatto", ValdorsoTestVita::Bandiere)
bool FValdorsoTestVitaGatto::RunTest(const FString& Parameters)
{
	TSet<FName> Saputi;
	const FValdorsoIndizioGatto* Primo = ValdorsoGatto::Prossimo(Saputi, 0);
	TestTrue(TEXT("c'e' un primo indizio"), Primo != nullptr);
	TestTrue(TEXT("prima dell'Atto I, non quello del battito"), Primo && !Primo->bAttoI);
	const FValdorsoIndizioGatto* ConAtto = ValdorsoGatto::Prossimo(Saputi, 1);
	TestTrue(TEXT("con l'Atto I, prima il battito"), ConAtto && ConAtto->bAttoI);
	int32 Quanti = 0;
	while (const FValdorsoIndizioGatto* Ancora = ValdorsoGatto::Prossimo(Saputi, 1))
	{
		TestFalse(TEXT("mai lo stesso due volte"), Saputi.Contains(FName(Ancora->Id)));
		Saputi.Add(FName(Ancora->Id));
		if (++Quanti > 50)
		{
			break;
		}
	}
	TestEqual(TEXT("tutti gli indizi, poi niente"), Quanti, ValdorsoGatto::Indizi().Num());
	for (int64 Giro = -5; Giro < 200; ++Giro)
	{
		const int32 Posa = ValdorsoGatto::PosaDelGiro(Giro, 7);
		if (Posa < 0 || Posa > 2)
		{
			AddError(TEXT("una posa fuori dalle tre"));
			break;
		}
	}
	return true;
}

// (11/11) La giornata dei PNG: vale l'ultima tappa cominciata; prima della prima, l'ultima della sera prima.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestVitaGiornata, "Valdorso.Vita.Giornata", ValdorsoTestVita::Bandiere)
bool FValdorsoTestVitaGiornata::RunTest(const FString& Parameters)
{
	TArray<FValdorsoTappa> Vuota;
	TestEqual(TEXT("senza tappe, nessuna"), ValdorsoGiornata::TappaPer(Vuota, 10.f), (int32)INDEX_NONE);

	// le tappe non sono per forza in ordine
	TArray<FValdorsoTappa> Giorno;
	Giorno.AddDefaulted(4);
	Giorno[0].Ora = 13.f;
	Giorno[1].Ora = 7.f;
	Giorno[2].Ora = 22.5f;
	Giorno[2].bAssente = true;
	Giorno[3].Ora = 19.f;
	TestEqual(TEXT("alle 8 la tappa delle 7"), ValdorsoGiornata::TappaPer(Giorno, 8.f), 1);
	TestEqual(TEXT("alle 7 in punto la tappa delle 7"), ValdorsoGiornata::TappaPer(Giorno, 7.f), 1);
	TestEqual(TEXT("alle 15 quella delle 13"), ValdorsoGiornata::TappaPer(Giorno, 15.f), 0);
	TestEqual(TEXT("alle 20 quella delle 19"), ValdorsoGiornata::TappaPer(Giorno, 20.f), 3);
	TestEqual(TEXT("alle 23 a casa"), ValdorsoGiornata::TappaPer(Giorno, 23.f), 2);
	TestEqual(TEXT("alle 3 di notte ancora a casa (la sera prima)"), ValdorsoGiornata::TappaPer(Giorno, 3.f), 2);

	TArray<FValdorsoTappa> Una;
	Una.AddDefaulted(1);
	Una[0].Ora = 9.f;
	TestEqual(TEXT("una tappa sola vale sempre"), ValdorsoGiornata::TappaPer(Una, 2.f), 0);
	return true;
}

#endif
