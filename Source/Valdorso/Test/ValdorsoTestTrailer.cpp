// Valdorso - Test automatici dello strumento per il trailer (ValdorsoTrailer: la strada, lo sguardo, la tabella delle
// lunghezze, la velocità, le bande, i nomi e il file JSON). Scritto da Claude il 08/11/2026.
// Si avviano come gli altri (Tools\Test Valdorso.cmd); qui non si tocca il percorso di adesso né Saved/Trailer.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoTrailer.h"

namespace ValdorsoTestTrailer
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	FValdorsoTappaTrailer Tappa(const FVector& Dove, const FRotator& Verso, float Campo = 90.f, float Fuoco = 0.f)
	{
		FValdorsoTappaTrailer T;
		T.Posizione = Dove;
		T.Rotazione = Verso;
		T.Campo = Campo;
		T.Fuoco = Fuoco;
		return T;
	}

	/** I gradi tra due sguardi. */
	float Gradi(const FRotator& A, const FRotator& B)
	{
		return FMath::RadiansToDegrees(static_cast<float>(A.Quaternion().AngularDistance(B.Quaternion())));
	}
}

// La strada: passa dalle tappe, sta dritta sulle tappe in fila, non impazzisce con due tappe nello stesso punto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTrailerStrada, "Valdorso.Trailer.Strada", ValdorsoTestTrailer::Bandiere)
bool FValdorsoTestTrailerStrada::RunTest(const FString& Parameters)
{
	const FVector P0(0.0, 0.0, 0.0);
	const FVector P1(1000.0, 0.0, 0.0);
	const FVector P2(2000.0, 500.0, 100.0);
	const FVector P3(2500.0, 1500.0, 0.0);
	TestTrue(TEXT("all'inizio è sulla prima tappa del tratto"), FVector::Dist(ValdorsoTrailer::CatmullRomCentripeta(P0, P1, P2, P3, 0.f), P1) < 0.01);
	TestTrue(TEXT("alla fine è sulla seconda"), FVector::Dist(ValdorsoTrailer::CatmullRomCentripeta(P0, P1, P2, P3, 1.f), P2) < 0.01);

	const FVector Meta = ValdorsoTrailer::CatmullRomCentripeta(FVector(0.0), FVector(1000.0, 0.0, 0.0), FVector(2000.0, 0.0, 0.0), FVector(3000.0, 0.0, 0.0), 0.5f);
	TestTrue(TEXT("tappe in fila e uguali: a metà è a metà"), FVector::Dist(Meta, FVector(1500.0, 0.0, 0.0)) < 0.01);

	const FVector Ferma = ValdorsoTrailer::CatmullRomCentripeta(FVector(0.0), FVector(100.0, 0.0, 0.0), FVector(100.0, 0.0, 0.0), FVector(200.0, 0.0, 0.0), 0.5f);
	TestTrue(TEXT("due tappe nello stesso punto: resta lì (niente NaN)"), !Ferma.ContainsNaN() && FVector::Dist(Ferma, FVector(100.0, 0.0, 0.0)) < 0.5);

	// tappe a distanze molto diverse (centripeta): niente ricciolo, si va sempre avanti
	double Prima = -1.0;
	bool bSempreAvanti = true;
	for (int32 I = 0; I <= 20; ++I)
	{
		const FVector Qui = ValdorsoTrailer::CatmullRomCentripeta(FVector(-10000.0, 0.0, 0.0), FVector(0.0), FVector(100.0, 0.0, 0.0), FVector(100.0, 10000.0, 0.0), I / 20.f);
		bSempreAvanti &= Qui.X >= Prima;
		Prima = Qui.X;
	}
	TestTrue(TEXT("distanze molto diverse: niente ricciolo"), bSempreAvanti);
	return true;
}

// Zoom e fuoco: morbidi, ma mai oltre i valori delle tappe; lo sguardo: passa dalle tappe e prende la via corta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTrailerSguardo, "Valdorso.Trailer.Sguardo", ValdorsoTestTrailer::Bandiere)
bool FValdorsoTestTrailerSguardo::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("valore: all'inizio V1"), FMath::IsNearlyEqual(ValdorsoTrailer::ValoreMorbido(10.f, 30.f, 60.f, 90.f, 0.f), 30.f, 0.001f));
	TestTrue(TEXT("valore: alla fine V2"), FMath::IsNearlyEqual(ValdorsoTrailer::ValoreMorbido(10.f, 30.f, 60.f, 90.f, 1.f), 60.f, 0.001f));
	TestTrue(TEXT("valore: tra 90 e 30 con i vicini uguali, a metà 60"), FMath::IsNearlyEqual(ValdorsoTrailer::ValoreMorbido(90.f, 90.f, 30.f, 30.f, 0.5f), 60.f, 0.01f));
	bool bDentro = true;
	bool bScende = true;
	float Prima = 1000.f;
	for (int32 I = 0; I <= 20; ++I)
	{
		const float U = I / 20.f;
		const float Su = ValdorsoTrailer::ValoreMorbido(30.f, 60.f, 60.f, 30.f, U);
		bDentro &= FMath::IsNearlyEqual(Su, 60.f, 0.001f);
		const float Giu = ValdorsoTrailer::ValoreMorbido(100.f, 90.f, 20.f, 19.f, U);
		bScende &= Giu <= Prima + 0.001f && Giu >= 20.f - 0.001f && Giu <= 90.f + 0.001f;
		Prima = Giu;
	}
	TestTrue(TEXT("due tappe con lo stesso zoom: niente rimbalzo"), bDentro);
	TestTrue(TEXT("lo zoom che scende scende sempre, senza uscire dal tratto"), bScende);

	const FQuat Q0 = FRotator(0.f, 0.f, 0.f).Quaternion();
	const FQuat Q2 = FRotator(0.f, 90.f, 0.f).Quaternion();
	TestTrue(TEXT("sguardo: all'inizio la prima tappa"), Q0.AngularDistance(ValdorsoTrailer::RotazioneMorbida(Q0, Q0, Q2, Q2, 0.f)) < 0.001);
	TestTrue(TEXT("sguardo: alla fine la seconda"), Q2.AngularDistance(ValdorsoTrailer::RotazioneMorbida(Q0, Q0, Q2, Q2, 1.f)) < 0.001);
	const FRotator Meta = ValdorsoTrailer::RotazioneMorbida(Q0, Q0, Q2, Q2, 0.5f).Rotator();
	TestTrue(FString::Printf(TEXT("sguardo: a metà 45 gradi (%.2f)"), Meta.Yaw), FMath::IsNearlyEqual(Meta.Yaw, 45.0, 0.1));
	// la stessa rotazione scritta con l'altra metà della sfera dà lo stesso sguardo
	const FQuat Opposta(-Q2.X, -Q2.Y, -Q2.Z, -Q2.W);
	const FQuat ConOpposta = ValdorsoTrailer::RotazioneMorbida(Q0, Q0, Opposta, Opposta, 0.5f);
	TestTrue(TEXT("sguardo: niente giro lungo con la quaternione opposta"), ConOpposta.AngularDistance(Meta.Quaternion()) < 0.001);
	// da 350 a 10 gradi si passa da 0, non da 180
	const FRotator Corta = ValdorsoTrailer::RotazioneMorbida(FRotator(0.f, 350.f, 0.f).Quaternion(), FRotator(0.f, 350.f, 0.f).Quaternion(),
		FRotator(0.f, 10.f, 0.f).Quaternion(), FRotator(0.f, 10.f, 0.f).Quaternion(), 0.5f).Rotator();
	TestTrue(FString::Printf(TEXT("sguardo: da 350 a 10 passa da 0 (%.2f)"), Corta.Yaw), FMath::Abs(FRotator::NormalizeAxis(Corta.Yaw)) < 0.1);
	return true;
}

// La posa lungo il percorso: sulle tappe è la tappa, l'inclinazione sfuma, il fuoco delle tappe senza sfocatura.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTrailerPosa, "Valdorso.Trailer.Posa", ValdorsoTestTrailer::Bandiere)
bool FValdorsoTestTrailerPosa::RunTest(const FString& Parameters)
{
	FValdorsoPercorsoTrailer Percorso;
	Percorso.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(0.0, 0.0, 5000.0), FRotator(-10.f, 0.f, 0.f), 90.f, 0.f));
	Percorso.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(1000.0, 0.0, 5000.0), FRotator(-10.f, 30.f, 20.f), 60.f, 800.f));
	Percorso.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(2000.0, 600.0, 5200.0), FRotator(0.f, 60.f, 0.f), 40.f, 0.f));
	TestEqual(TEXT("tre tappe aperte = due tratti"), Percorso.Tratti(), 2);

	const FValdorsoTappaTrailer Seconda = ValdorsoTrailer::Posa(Percorso, 1.f);
	TestTrue(TEXT("chiave 1: la seconda tappa (posizione)"), FVector::Dist(Seconda.Posizione, Percorso.Tappe[1].Posizione) < 0.01);
	TestTrue(TEXT("chiave 1: la seconda tappa (sguardo)"), ValdorsoTestTrailer::Gradi(Seconda.Rotazione, Percorso.Tappe[1].Rotazione) < 0.01f);
	TestTrue(TEXT("chiave 1: la seconda tappa (zoom)"), FMath::IsNearlyEqual(Seconda.Campo, 60.f, 0.001f));
	TestTrue(TEXT("prima dell'inizio: la prima tappa"), FVector::Dist(ValdorsoTrailer::Posa(Percorso, -3.f).Posizione, Percorso.Tappe[0].Posizione) < 0.01);
	TestTrue(TEXT("dopo la fine: l'ultima tappa"), FVector::Dist(ValdorsoTrailer::Posa(Percorso, 7.f).Posizione, Percorso.Tappe[2].Posizione) < 0.01);

	const FValdorsoTappaTrailer Meta = ValdorsoTrailer::Posa(Percorso, 0.5f);
	TestTrue(FString::Printf(TEXT("l'inclinazione sfuma (%.2f)"), Meta.Rotazione.Roll), Meta.Rotazione.Roll > 2.0 && Meta.Rotazione.Roll < 18.0);
	TestTrue(FString::Printf(TEXT("il fuoco viene dalla tappa sfocata (%.1f)"), Meta.Fuoco), FMath::IsNearlyEqual(Meta.Fuoco, 800.f, 0.01f));

	// ad anello: tre tappe = tre tratti, e la chiave 3 è di nuovo la prima
	Percorso.bGiro = true;
	TestEqual(TEXT("ad anello tre tratti"), Percorso.Tratti(), 3);
	TestTrue(TEXT("ad anello la chiave 3 è la prima tappa"), FVector::Dist(ValdorsoTrailer::Posa(Percorso, 3.f).Posizione, Percorso.Tappe[0].Posizione) < 0.01);

	FValdorsoPercorsoTrailer Una;
	Una.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(5.0, 6.0, 7.0), FRotator::ZeroRotator));
	TestEqual(TEXT("una tappa sola: nessun tratto"), Una.Tratti(), 0);
	TestTrue(TEXT("una tappa sola: la posa è quella"), FVector::Dist(ValdorsoTrailer::Posa(Una, 0.7f).Posizione, FVector(5.0, 6.0, 7.0)) < 0.01);
	return true;
}

// La tabella delle lunghezze: la strada è lunga quanto deve, la velocità resta costante, il giro sul posto conta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTrailerTabella, "Valdorso.Trailer.Tabella", ValdorsoTestTrailer::Bandiere)
bool FValdorsoTestTrailerTabella::RunTest(const FString& Parameters)
{
	FValdorsoPercorsoTrailer Dritto;
	Dritto.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(0.0, 0.0, 0.0), FRotator::ZeroRotator));
	Dritto.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator));
	Dritto.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(2000.0, 0.0, 0.0), FRotator::ZeroRotator));
	FValdorsoTabellaTrailer Tabella;
	Tabella.Prepara(Dritto);
	TestTrue(FString::Printf(TEXT("dritto: lunga 20 m (%.1f cm)"), Tabella.Lunghezza()), FMath::IsNearlyEqual(Tabella.Lunghezza(), 2000.f, 1.f));
	TestTrue(TEXT("dritto: a 10 m la seconda tappa"), FMath::IsNearlyEqual(Tabella.Chiave(1000.f), 1.f, 0.01f));
	TestTrue(TEXT("aperta: prima dell'inizio si sta all'inizio"), FMath::IsNearlyEqual(Tabella.Chiave(-50.f), 0.f, 0.0001f));
	TestTrue(TEXT("aperta: dopo la fine si sta in fondo"), FMath::IsNearlyEqual(Tabella.Chiave(99999.f), 2.f, 0.0001f));

	// una strada curva, in salita e in discesa: a passi uguali di distanza, passi uguali di strada
	FValdorsoPercorsoTrailer Curva;
	Curva.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(0.0, 0.0, 0.0), FRotator::ZeroRotator));
	Curva.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator));
	Curva.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(1500.0, 800.0, 200.0), FRotator::ZeroRotator));
	Curva.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(1500.0, 2000.0, 0.0), FRotator::ZeroRotator));
	Curva.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(400.0, 2600.0, 0.0), FRotator::ZeroRotator));
	Tabella.Prepara(Curva);
	const float Lunga = Tabella.Lunghezza();
	TestTrue(FString::Printf(TEXT("curva: più lunga delle corde (%.0f cm)"), Lunga), Lunga > 4400.f && Lunga < 4700.f);
	float Minimo = BIG_NUMBER;
	float Massimo = 0.f;
	FVector Prima = ValdorsoTrailer::Posa(Curva, Tabella.Chiave(0.f)).Posizione;
	for (int32 I = 1; I <= 60; ++I)
	{
		const FVector Qui = ValdorsoTrailer::Posa(Curva, Tabella.Chiave(Lunga * I / 60.f)).Posizione;
		const float Passo = static_cast<float>(FVector::Dist(Prima, Qui));
		Minimo = FMath::Min(Minimo, Passo);
		Massimo = FMath::Max(Massimo, Passo);
		Prima = Qui;
	}
	const float Giusto = Lunga / 60.f;
	TestTrue(FString::Printf(TEXT("curva: velocità costante (passi tra %.1f e %.1f, giusto %.1f)"), Minimo, Massimo, Giusto),
		Minimo > Giusto * 0.97f && Massimo < Giusto * 1.03f);

	// un quadrato ad anello: si chiude, e dopo un giro si ricomincia
	FValdorsoPercorsoTrailer Quadrato;
	Quadrato.bGiro = true;
	Quadrato.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(0.0, 0.0, 0.0), FRotator::ZeroRotator));
	Quadrato.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator));
	Quadrato.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(1000.0, 1000.0, 0.0), FRotator::ZeroRotator));
	Quadrato.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(0.0, 1000.0, 0.0), FRotator::ZeroRotator));
	Tabella.Prepara(Quadrato);
	TestTrue(FString::Printf(TEXT("quadrato ad anello: tra 40 e 44 m (%.0f cm)"), Tabella.Lunghezza()), Tabella.Lunghezza() > 4000.f && Tabella.Lunghezza() < 4400.f);
	TestTrue(TEXT("ad anello: dopo un giro si è di nuovo all'inizio"), FMath::IsNearlyEqual(Tabella.Chiave(Tabella.Lunghezza() + 1.f), Tabella.Chiave(1.f), 0.001f));

	// due tappe nello stesso punto che guardano da parti diverse: il giro sul posto conta come strada (1 grado = 20 cm)
	FValdorsoPercorsoTrailer SulPosto;
	SulPosto.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(0.0, 0.0, 0.0), FRotator(0.f, 0.f, 0.f)));
	SulPosto.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(0.0, 0.0, 0.0), FRotator(0.f, 90.f, 0.f)));
	Tabella.Prepara(SulPosto);
	const float Giro = 90.f * ValdorsoTrailer::CmPerGrado;
	TestTrue(FString::Printf(TEXT("girarsi di 90 gradi vale 18 m (%.0f cm)"), Tabella.Lunghezza()), FMath::IsNearlyEqual(Tabella.Lunghezza(), Giro, Giro * 0.02f));

	FValdorsoPercorsoTrailer Vuoto;
	Tabella.Prepara(Vuoto);
	TestFalse(TEXT("senza tappe la tabella non è pronta"), Tabella.Pronta());
	TestTrue(TEXT("senza tappe la chiave è 0"), FMath::IsNearlyEqual(Tabella.Chiave(100.f), 0.f, 0.0001f));
	return true;
}

// La velocità: partenza e arrivo dolci, in mezzo piena; i tasti 1-5; le bande del cinema; i nomi dei file.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTrailerVelocita, "Valdorso.Trailer.Velocita", ValdorsoTestTrailer::Bandiere)
bool FValdorsoTestTrailerVelocita::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("alla partenza quasi ferma"), FMath::IsNearlyEqual(ValdorsoTrailer::FattoreVelocita(0.f, 10000.f, 800.f), 0.05f, 0.001f));
	TestTrue(TEXT("in mezzo piena"), FMath::IsNearlyEqual(ValdorsoTrailer::FattoreVelocita(5.f, 100000.f, 800.f), 1.f, 0.001f));
	TestTrue(TEXT("in fondo quasi ferma"), FMath::IsNearlyEqual(ValdorsoTrailer::FattoreVelocita(5.f, 0.f, 800.f), 0.05f, 0.001f));
	TestTrue(TEXT("ad anello non frena mai"), FMath::IsNearlyEqual(ValdorsoTrailer::FattoreVelocita(5.f, -1.f, 800.f), 1.f, 0.001f));

	// 300 m a 8 m/s, a passi di 1/30 di secondo come i fotogrammi: 37,5 s più un po' per partire e arrivare
	float Tempo = 0.f;
	float Strada = 0.f;
	const float Lunga = 30000.f;
	while (Strada < Lunga - 0.5f && Tempo < 1000.f)
	{
		Strada += 800.f * ValdorsoTrailer::FattoreVelocita(Tempo, Lunga - Strada, 800.f) * (1.f / 30.f);
		Tempo += 1.f / 30.f;
	}
	TestTrue(FString::Printf(TEXT("300 m a 8 m/s: tra 37,5 e 42 s (%.2f s)"), Tempo), Tempo > 37.5f && Tempo < 42.f);

	TestTrue(TEXT("tasto 1 = 2 m/s"), FMath::IsNearlyEqual(ValdorsoTrailer::VelocitaTasto(1), 200.f));
	TestTrue(TEXT("tasto 3 = 8 m/s"), FMath::IsNearlyEqual(ValdorsoTrailer::VelocitaTasto(3), 800.f));
	TestTrue(TEXT("tasto 5 = 30 m/s"), FMath::IsNearlyEqual(ValdorsoTrailer::VelocitaTasto(5), 3000.f));
	TestTrue(TEXT("tasto 6 = niente"), FMath::IsNearlyEqual(ValdorsoTrailer::VelocitaTasto(6), 0.f));

	const float Banda = ValdorsoTrailer::AltezzaBande(FVector2D(1920.0, 1080.0));
	TestTrue(FString::Printf(TEXT("1920x1080: bande di circa 138 pixel (%.2f)"), Banda), FMath::IsNearlyEqual(Banda, (1080.f - 1920.f / 2.39f) * 0.5f, 0.01f));
	TestTrue(TEXT("dentro le bande resta 2,39 a 1"), FMath::IsNearlyEqual(1920.f / (1080.f - 2.f * Banda), 2.39f, 0.001f));
	TestTrue(TEXT("schermo più largo del cinema: niente bande"), FMath::IsNearlyEqual(ValdorsoTrailer::AltezzaBande(FVector2D(4000.0, 1000.0)), 0.f));
	TestTrue(TEXT("schermo senza misure: niente bande"), FMath::IsNearlyEqual(ValdorsoTrailer::AltezzaBande(FVector2D(0.0, 0.0)), 0.f));

	TestEqual(TEXT("nome con lo spazio"), ValdorsoTrailer::NomeValido(TEXT(" piazza alba ")), FString(TEXT("piazza_alba")));
	TestEqual(TEXT("niente cartelle nel nome"), ValdorsoTrailer::NomeValido(TEXT("../segreti")), FString(TEXT("___segreti")));
	TestEqual(TEXT("le lettere accentate diventano _"), ValdorsoTrailer::NomeValido(TEXT("città")), FString(TEXT("citt_")));
	TestTrue(TEXT("nome vuoto = non va"), ValdorsoTrailer::NomeValido(TEXT("   ")).IsEmpty());
	TestTrue(TEXT("solo segni = non va"), ValdorsoTrailer::NomeValido(TEXT("/// ")).IsEmpty());
	TestEqual(TEXT("i nomi di Windows"), ValdorsoTrailer::NomeValido(TEXT("con")), FString(TEXT("con_")));
	TestEqual(TEXT("al massimo 40 lettere"), ValdorsoTrailer::NomeValido(FString::ChrN(60, TEXT('a'))).Len(), 40);
	return true;
}

// Il file: quello che si salva si rilegge uguale; i file sbagliati non si caricano; i numeri strani si sistemano.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTrailerFile, "Valdorso.Trailer.File", ValdorsoTestTrailer::Bandiere)
bool FValdorsoTestTrailerFile::RunTest(const FString& Parameters)
{
	FValdorsoPercorsoTrailer Percorso;
	Percorso.Velocita = 1500.f;
	Percorso.bGiro = true;
	Percorso.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(15300.25, 42228.5, 5517.75), FRotator(-12.5f, 170.25f, 8.f), 55.5f, 0.f));
	Percorso.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(15800.0, 41000.0, 6000.0), FRotator(-3.f, -95.f, -20.f), 30.f, 1234.5f));
	Percorso.Tappe.Add(ValdorsoTestTrailer::Tappa(FVector(-2040.0, 43860.0, 5015.0), FRotator(10.f, 0.f, 0.f), 90.f, 0.f));

	const FString Testo = ValdorsoTrailer::InJson(Percorso);
	FValdorsoPercorsoTrailer Letto;
	TestTrue(TEXT("si rilegge"), ValdorsoTrailer::DaJson(Testo, Letto));
	TestEqual(TEXT("le stesse tappe"), Letto.Tappe.Num(), 3);
	TestTrue(TEXT("la stessa velocità"), FMath::IsNearlyEqual(Letto.Velocita, 1500.f, 0.001f));
	TestTrue(TEXT("lo stesso giro"), Letto.bGiro);
	if (Letto.Tappe.Num() == 3)
	{
		for (int32 I = 0; I < 3; ++I)
		{
			const FValdorsoTappaTrailer& A = Percorso.Tappe[I];
			const FValdorsoTappaTrailer& B = Letto.Tappe[I];
			TestTrue(FString::Printf(TEXT("tappa %d: la posizione"), I), FVector::Dist(A.Posizione, B.Posizione) < 0.01);
			TestTrue(FString::Printf(TEXT("tappa %d: lo sguardo con l'inclinazione"), I),
				FMath::IsNearlyEqual(A.Rotazione.Pitch, B.Rotazione.Pitch, 0.001) && FMath::IsNearlyEqual(A.Rotazione.Yaw, B.Rotazione.Yaw, 0.001)
				&& FMath::IsNearlyEqual(A.Rotazione.Roll, B.Rotazione.Roll, 0.001));
			TestTrue(FString::Printf(TEXT("tappa %d: zoom e fuoco"), I), FMath::IsNearlyEqual(A.Campo, B.Campo, 0.001f) && FMath::IsNearlyEqual(A.Fuoco, B.Fuoco, 0.001f));
		}
	}

	FValdorsoPercorsoTrailer Niente;
	TestFalse(TEXT("un file rovinato non si carica"), ValdorsoTrailer::DaJson(TEXT("{ rovinato"), Niente));
	TestFalse(TEXT("un altro file JSON non si carica"), ValdorsoTrailer::DaJson(TEXT("{\"gioco\":\"Valdorso\",\"tipo\":\"stato\",\"tappe\":[]}"), Niente));
	TestFalse(TEXT("senza tappe non è un percorso"), ValdorsoTrailer::DaJson(TEXT("{\"gioco\":\"Valdorso\",\"tipo\":\"trailer\"}"), Niente));

	// scritto a mano: lo zoom fuori misura, il fuoco negativo, una voce che non è una tappa, la velocità esagerata
	const FString AMano = TEXT("{\"gioco\":\"Valdorso\",\"tipo\":\"trailer\",\"velocita\":999999,\"tappe\":[{\"x\":1,\"y\":2,\"z\":3,\"campo\":500,\"fuoco\":-4},7,{\"x\":4}]}");
	FValdorsoPercorsoTrailer Sistemato;
	TestTrue(TEXT("scritto a mano: si carica"), ValdorsoTrailer::DaJson(AMano, Sistemato));
	TestEqual(TEXT("scritto a mano: la voce strana si salta"), Sistemato.Tappe.Num(), 2);
	TestTrue(TEXT("scritto a mano: velocità al massimo 100 m/s"), FMath::IsNearlyEqual(Sistemato.Velocita, 10000.f, 0.001f));
	TestFalse(TEXT("scritto a mano: senza \"giro\" non è ad anello"), Sistemato.bGiro);
	if (Sistemato.Tappe.Num() == 2)
	{
		TestTrue(TEXT("scritto a mano: zoom al massimo 100"), FMath::IsNearlyEqual(Sistemato.Tappe[0].Campo, 100.f, 0.001f));
		TestTrue(TEXT("scritto a mano: fuoco mai negativo"), FMath::IsNearlyEqual(Sistemato.Tappe[0].Fuoco, 0.f, 0.001f));
		TestTrue(TEXT("scritto a mano: quello che manca vale come di serie"), FMath::IsNearlyEqual(Sistemato.Tappe[1].Campo, 90.f, 0.001f)
			&& FMath::IsNearlyEqual(Sistemato.Tappe[1].Posizione.X, 4.0, 0.001));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
