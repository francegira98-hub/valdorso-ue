// Valdorso - Test automatici del menu nella valle vera al tramonto (ValdorsoMenuValle): il giro della telecamera.
// Scritto da Claude il 08/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoMenuValle.h"

namespace ValdorsoTestMenuValle
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Le posizioni delle tappe, come le usa il giro. */
	TArray<FVector> Posizioni()
	{
		TArray<FVector> Out;
		for (const FValdorsoTappaMenu& Tappa : ValdorsoMenuValle::Tappe())
		{
			Out.Add(Tappa.Posizione);
		}
		return Out;
	}
}

// Le tappe: quante sono, e ognuna tra 25 e 60 m sopra il suolo dei suoi tratti, con lo sguardo lontano.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestMenuValleTappe, "Valdorso.Menu.Valle.Tappe", ValdorsoTestMenuValle::Bandiere)
bool FValdorsoTestMenuValleTappe::RunTest(const FString& Parameters)
{
	const TArray<FValdorsoTappaMenu>& Tappe = ValdorsoMenuValle::Tappe();
	TestEqual(TEXT("nove tappe"), Tappe.Num(), 9);
	const FVector Piazza = ValdorsoMenuValle::Piazza();
	for (int32 i = 0; i < Tappe.Num(); ++i)
	{
		const FValdorsoTappaMenu& Tappa = Tappe[i];
		const float Sopra = static_cast<float>(Tappa.Posizione.Z) - Tappa.Suolo;
		TestTrue(FString::Printf(TEXT("tappa %d: almeno 25 m sopra il suolo (%.0f cm)"), i, Sopra), Sopra >= ValdorsoMenuValle::MinimoSopraSuolo);
		TestTrue(FString::Printf(TEXT("tappa %d: non più di 60 m sopra il suolo (%.0f cm)"), i, Sopra), Sopra <= 6000.f);
		TestTrue(FString::Printf(TEXT("tappa %d: il suolo è quello del villaggio (%.0f cm)"), i, Tappa.Suolo), Tappa.Suolo > 4000.f && Tappa.Suolo < 7000.f);
		TestTrue(FString::Printf(TEXT("tappa %d: lo sguardo è almeno 200 m davanti"), i), FVector::Dist(Tappa.Posizione, Tappa.Sguardo) >= 20000.0);
		TestTrue(FString::Printf(TEXT("tappa %d: lo sguardo va verso il basso"), i), Tappa.Sguardo.Z < Tappa.Posizione.Z);
		const double DallaPiazza = FVector::Dist2D(Tappa.Posizione, Piazza);
		TestTrue(FString::Printf(TEXT("tappa %d: intorno al villaggio (%.0f m dalla piazza)"), i, DallaPiazza / 100.0), DallaPiazza > 15000.0 && DallaPiazza < 40000.0);
	}
	return true;
}

// La curva: passa per le tappe, è continua (anche nella direzione) e si chiude su se stessa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestMenuValleCurva, "Valdorso.Menu.Valle.Curva", ValdorsoTestMenuValle::Bandiere)
bool FValdorsoTestMenuValleCurva::RunTest(const FString& Parameters)
{
	const TArray<FVector> Punti = ValdorsoTestMenuValle::Posizioni();
	const int32 N = Punti.Num();
	for (int32 i = 0; i < N; ++i)
	{
		TestTrue(FString::Printf(TEXT("la curva passa per la tappa %d"), i), ValdorsoMenuValle::SuGiro(Punti, static_cast<float>(i)).Equals(Punti[i], 0.5));
	}
	TestTrue(TEXT("dopo l'ultima tappa si torna alla prima"), ValdorsoMenuValle::SuGiro(Punti, static_cast<float>(N)).Equals(Punti[0], 0.5));
	TestTrue(TEXT("una chiave negativa gira all'indietro"), ValdorsoMenuValle::SuGiro(Punti, -1.f).Equals(Punti[N - 1], 0.5));

	// Niente salti: a passi piccoli i punti restano vicini; e la direzione non cambia di colpo (nemmeno sulle tappe).
	const int32 Passi = 400;
	const float Passo = static_cast<float>(N) / static_cast<float>(Passi);
	FVector Prima = ValdorsoMenuValle::SuGiro(Punti, 0.f);
	FVector DirezionePrima = (ValdorsoMenuValle::SuGiro(Punti, Passo) - Prima).GetSafeNormal();
	double PassoMassimo = 0.0;
	double CosenoMinimo = 1.0;
	for (int32 k = 1; k <= Passi; ++k)
	{
		const FVector Qui = ValdorsoMenuValle::SuGiro(Punti, Passo * static_cast<float>(k));
		PassoMassimo = FMath::Max(PassoMassimo, FVector::Dist(Prima, Qui));
		const FVector Direzione = (ValdorsoMenuValle::SuGiro(Punti, Passo * static_cast<float>(k + 1)) - Qui).GetSafeNormal();
		CosenoMinimo = FMath::Min(CosenoMinimo, static_cast<double>(FVector::DotProduct(Direzione, DirezionePrima)));
		Prima = Qui;
		DirezionePrima = Direzione;
	}
	TestTrue(FString::Printf(TEXT("nessun salto (passo più lungo %.0f cm)"), PassoMassimo), PassoMassimo < 1500.0);
	TestTrue(FString::Printf(TEXT("la direzione gira piano (coseno minimo %.4f)"), CosenoMinimo), CosenoMinimo > 0.99);

	// Il giro a velocità costante: la lunghezza, la chiusura, e la distanza percorsa è quella chiesta.
	FValdorsoGiroMenu Giro;
	Giro.Prepara(ValdorsoMenuValle::Tappe());
	TestEqual(TEXT("il giro ha tutte le tappe"), Giro.NumeroTappe(), N);
	TestTrue(FString::Printf(TEXT("il giro è lungo tra 1 e 2 km (%.0f m)"), Giro.Lunghezza() / 100.f), Giro.Lunghezza() > 100000.f && Giro.Lunghezza() < 200000.f);
	FVector Inizio, SguardoInizio, Fine, SguardoFine;
	Giro.Posa(0.f, Inizio, SguardoInizio);
	Giro.Posa(Giro.Lunghezza(), Fine, SguardoFine);
	TestTrue(TEXT("il giro parte dalla prima tappa"), Inizio.Equals(Punti[0], 1.0));
	TestTrue(TEXT("dopo un giro intero si è di nuovo all'inizio"), Fine.Equals(Inizio, 5.0) && SguardoFine.Equals(SguardoInizio, 5.0));
	FVector QuasiFine, SguardoQuasi;
	Giro.Posa(Giro.Lunghezza() - 10.f, QuasiFine, SguardoQuasi);
	TestTrue(TEXT("poco prima della fine si è vicini all'inizio (il giro si chiude)"), FVector::Dist(QuasiFine, Inizio) < 30.0);
	FVector Dopo, SguardoDopo;
	Giro.Posa(Giro.Lunghezza() * 2.f + 1000.f, Dopo, SguardoDopo);
	FVector Uguale, SguardoUguale;
	Giro.Posa(1000.f, Uguale, SguardoUguale);
	TestTrue(TEXT("oltre due giri si ricomincia"), Dopo.Equals(Uguale, 5.0));
	// a passi di 10 m lungo il giro, ogni passo è lungo circa 10 m (velocità costante)
	FVector A, SA;
	Giro.Posa(0.f, A, SA);
	double Corto = 1e9;
	double Lungo = 0.0;
	for (float D = 1000.f; D <= Giro.Lunghezza(); D += 1000.f)
	{
		FVector B, SB;
		Giro.Posa(D, B, SB);
		const double Tratto = FVector::Dist(A, B);
		Corto = FMath::Min(Corto, Tratto);
		Lungo = FMath::Max(Lungo, Tratto);
		A = B;
	}
	TestTrue(FString::Printf(TEXT("velocità costante (passi da %.0f a %.0f cm su 1000)"), Corto, Lungo), Corto > 950.0 && Lungo < 1010.0);
	return true;
}

// Le quote: in ogni punto della curva la telecamera sta almeno 25 m sopra il suolo dei due tratti vicini.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestMenuValleQuote, "Valdorso.Menu.Valle.Quote", ValdorsoTestMenuValle::Bandiere)
bool FValdorsoTestMenuValleQuote::RunTest(const FString& Parameters)
{
	const TArray<FValdorsoTappaMenu>& Tappe = ValdorsoMenuValle::Tappe();
	const TArray<FVector> Punti = ValdorsoTestMenuValle::Posizioni();
	const int32 N = Punti.Num();
	float Minimo = 1e9f;
	for (int32 i = 0; i < N; ++i)
	{
		const float Suolo = FMath::Max(Tappe[i].Suolo, Tappe[(i + 1) % N].Suolo);
		for (int32 k = 0; k <= 50; ++k)
		{
			const FVector P = ValdorsoMenuValle::SuGiro(Punti, static_cast<float>(i) + static_cast<float>(k) / 50.f);
			Minimo = FMath::Min(Minimo, static_cast<float>(P.Z) - Suolo);
		}
	}
	TestTrue(FString::Printf(TEXT("sempre almeno 25 m sopra il suolo (il punto più basso: %.0f cm)"), Minimo), Minimo >= ValdorsoMenuValle::MinimoSopraSuolo);

	// Il volo lungo del primo ingresso, da ogni tappa: parte dalla telecamera, passa sopra i tetti, sale verso i monti.
	for (int32 i = 0; i < N; ++i)
	{
		TArray<FVector> Volo;
		ValdorsoMenuValle::VoloLungo(Punti[i], Volo);
		TestEqual(FString::Printf(TEXT("volo dalla tappa %d: sette punti"), i), Volo.Num(), 7);
		if (Volo.Num() < 3)
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("volo dalla tappa %d: parte dalla telecamera"), i), Volo[0].Equals(Punti[i], 0.1));
		for (int32 k = 1; k < Volo.Num(); ++k)
		{
			TestTrue(FString::Printf(TEXT("volo dalla tappa %d, punto %d: sopra i tetti"), i, k), Volo[k].Z >= 9500.0);
			if (k >= 2)
			{
				TestTrue(FString::Printf(TEXT("volo dalla tappa %d, punto %d: sale"), i, k), Volo[k].Z > Volo[k - 1].Z);
			}
		}
	}

	// La sonda sotto la telecamera: sale solo se l'ostacolo è più vicino del minimo, e di quanto manca.
	TestEqual(TEXT("ostacolo lontano: non si sale"), ValdorsoMenuValle::Alzata(10000.f, 5000.f, 2000.f), 0.f);
	TestEqual(TEXT("ostacolo al minimo: non si sale"), ValdorsoMenuValle::Alzata(10000.f, 8000.f, 2000.f), 0.f);
	TestEqual(TEXT("ostacolo a 5 m: si salgono 15 m"), ValdorsoMenuValle::Alzata(10000.f, 9500.f, 2000.f), 1500.f);
	TestEqual(TEXT("dentro l'ostacolo: si esce di 20 m"), ValdorsoMenuValle::Alzata(10000.f, 10000.f, 2000.f), 2000.f);
	return true;
}

// La velocità: un giro in 60-90 secondi, e con "Meno movimento sullo schermo" molto più piano.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestMenuValleVelocita, "Valdorso.Menu.Valle.MenoMovimenti", ValdorsoTestMenuValle::Bandiere)
bool FValdorsoTestMenuValleVelocita::RunTest(const FString& Parameters)
{
	FValdorsoGiroMenu Giro;
	Giro.Prepara(ValdorsoMenuValle::Tappe());
	const float Normale = ValdorsoMenuValle::Velocita(Giro.Lunghezza(), ValdorsoMenuValle::SecondiGiro, false);
	const float Piano = ValdorsoMenuValle::Velocita(Giro.Lunghezza(), ValdorsoMenuValle::SecondiGiro, true);
	TestTrue(TEXT("un giro dura tra 60 e 90 secondi"), ValdorsoMenuValle::SecondiGiro >= 60.f && ValdorsoMenuValle::SecondiGiro <= 90.f);
	TestTrue(FString::Printf(TEXT("un volo lento: tra 8 e 25 m al secondo (%.1f)"), Normale / 100.f), Normale >= 800.f && Normale <= 2500.f);
	TestTrue(FString::Printf(TEXT("un giro intero in SecondiGiro (%.1f s)"), Giro.Lunghezza() / Normale), FMath::IsNearlyEqual(Giro.Lunghezza() / Normale, ValdorsoMenuValle::SecondiGiro, 0.01f));
	TestTrue(TEXT("con meno movimento si va più piano"), Piano < Normale);
	TestTrue(TEXT("con meno movimento al massimo un terzo"), Piano <= Normale / 3.f + 0.01f);
	TestTrue(TEXT("con meno movimento si va al fattore giusto"), FMath::IsNearlyEqual(Piano, Normale * ValdorsoMenuValle::FattoreMenoMovimenti, 0.01f));
	TestTrue(TEXT("ma la telecamera non è ferma (si muove ancora)"), Piano > 0.f);
	TestEqual(TEXT("senza giro, ferma"), ValdorsoMenuValle::Velocita(0.f, 90.f, false), 0.f);
	TestTrue(TEXT("secondi a zero: nessuna divisione per zero"), FMath::IsFinite(ValdorsoMenuValle::Velocita(1000.f, 0.f, false)));
	return true;
}

#endif
