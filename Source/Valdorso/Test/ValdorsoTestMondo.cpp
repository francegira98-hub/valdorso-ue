// Valdorso - Test automatici del mondo: le fasi del villaggio, il sigillo del passo, le scale a pioli, il nuoto e il tempo (06/11).
// Scritto da Claude il 06/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoFasiVillaggio.h"
#include "ValdorsoSigillo.h"
#include "ValdorsoScala.h"
#include "ValdorsoAcque.h"
#include "ValdorsoTempo.h"
#include "ValdorsoMovenze.h"
#include "ValdorsoAmbiente.h"
#include "ValdorsoCampane.h"
#include "ValdorsoValleViva.h"
#include "ValdorsoRovine.h"
#include "ValdorsoStatoMondo.h"
#include "ValdorsoTerritorio.h"
#include "ValdorsoModoFoto.h"
#include "ValdorsoRune.h"

namespace ValdorsoTestMondo
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

// (07/11) La lingua delle rune: ogni runa ha la sua forma, le parole si leggono e si scrivono, gli incantesimi delle
// Cronache sono validi e hanno il loro nome, le combinazioni sbagliate no.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestLinguaRune, "Valdorso.Mondo.LinguaRune", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestLinguaRune::RunTest(const FString& Parameters)
{
	int32 Elementi = 0;
	int32 Verbi = 0;
	for (int32 i = 0; i < static_cast<int32>(EValdorsoRuna::Num); ++i)
	{
		const EValdorsoRuna Runa = static_cast<EValdorsoRuna>(i);
		const FString Nome = ValdorsoRune::Nome(Runa);
		EValdorsoRuna Riletta;
		TestTrue(FString::Printf(TEXT("%s si rilegge"), *Nome), ValdorsoRune::DaNome(Nome.ToLower(), Riletta) && Riletta == Runa);
		TestFalse(FString::Printf(TEXT("%s ha un significato"), *Nome), ValdorsoRune::Significato(Runa).IsEmpty());
		const TArray<TArray<FVector2D>>& Linee = ValdorsoRune::Linee(Runa);
		TestTrue(FString::Printf(TEXT("%s ha una forma"), *Nome), Linee.Num() > 0);
		for (const TArray<FVector2D>& Linea : Linee)
		{
			TestTrue(FString::Printf(TEXT("%s: ogni linea ha almeno due punti"), *Nome), Linea.Num() >= 2);
			for (const FVector2D& P : Linea)
			{
				TestTrue(FString::Printf(TEXT("%s: dentro il suo quadrato"), *Nome), FMath::Abs(P.X) <= 1.05 && FMath::Abs(P.Y) <= 1.05);
			}
		}
		Elementi += ValdorsoRune::Elemento(Runa) ? 1 : 0;
		Verbi += ValdorsoRune::Verbo(Runa) ? 1 : 0;
	}
	TestEqual(TEXT("cinque elementi"), Elementi, 5);
	TestEqual(TEXT("cinque verbi"), Verbi, 5);
	TestFalse(TEXT("Ur non è un elemento"), ValdorsoRune::Elemento(EValdorsoRuna::Ur));
	TestFalse(TEXT("Ur non è un verbo"), ValdorsoRune::Verbo(EValdorsoRuna::Ur));

	TArray<EValdorsoRuna> Parola;
	TestTrue(TEXT("aeth-VAR-tor si legge"), ValdorsoRune::LeggiParola(TEXT("aeth-VAR-tor"), Parola));
	TestEqual(TEXT("e si riscrive"), ValdorsoRune::ScriviParola(Parola), FString(TEXT("Aeth-Var-Tor")));
	TestEqual(TEXT("Fiamma che inchioda"), ValdorsoRune::NomeIncantesimo(Parola).ToString(), FString(TEXT("Fiamma che inchioda")));
	TestEqual(TEXT("tre rune: grado III"), ValdorsoRune::Grado(Parola), 3);
	TestFalse(TEXT("una parola inventata non si legge"), ValdorsoRune::LeggiParola(TEXT("Aeth-Pippo"), Parola));

	auto Valida = [](const TCHAR* Testo)
	{
		TArray<EValdorsoRuna> P;
		return ValdorsoRune::LeggiParola(Testo, P) && ValdorsoRune::IncantesimoValido(P);
	};
	// le diciotto parole con un nome nelle Cronache
	for (const TCHAR* Testo : { TEXT("Aeth-Var"), TEXT("Nir-Ren"), TEXT("Lume-Tor"),
		TEXT("Aeth-Var-Tor"), TEXT("Aeth-Var-Ren"), TEXT("Aeth-Ka-Vel"), TEXT("Nir-Var-Tor"), TEXT("Nir-Ren-Ka"),
		TEXT("Nir-Var-Vel"), TEXT("Dorn-Var-Tor"), TEXT("Dorn-Ka-Ren"), TEXT("Dorn-Var-Vel"), TEXT("Sil-Var-Tor"),
		TEXT("Sil-Ka-Ren"), TEXT("Sil-Var-Vel"), TEXT("Lume-Vel-Tor"), TEXT("Lume-Ren-Ka"), TEXT("Lume-Vel-Ren") })
	{
		TArray<EValdorsoRuna> P;
		ValdorsoRune::LeggiParola(Testo, P);
		TestTrue(FString::Printf(TEXT("%s è un incantesimo"), Testo), ValdorsoRune::IncantesimoValido(P));
		TestFalse(FString::Printf(TEXT("%s ha un nome"), Testo), ValdorsoRune::NomeIncantesimo(P).IsEmpty());
	}
	TestTrue(TEXT("Dorn-Tor vale anche senza nome"), Valida(TEXT("Dorn-Tor")));
	TestFalse(TEXT("un elemento solo non basta"), Valida(TEXT("Aeth")));
	TestFalse(TEXT("prima l'elemento"), Valida(TEXT("Var-Aeth")));
	TestFalse(TEXT("due elementi no"), Valida(TEXT("Aeth-Nir")));
	TestFalse(TEXT("lo stesso verbo due volte no"), Valida(TEXT("Aeth-Var-Var")));
	TestFalse(TEXT("quattro rune no"), Valida(TEXT("Aeth-Var-Tor-Ka")));
	TestFalse(TEXT("Ur non entra negli incantesimi"), Valida(TEXT("Aeth-Ur")));

	TArray<TArray<EValdorsoRuna>> Parole;
	TestTrue(TEXT("riga di rune"), ValdorsoRune::LeggiRiga(TEXT("rune: Aeth Nir-Ren-Ka"), Parole));
	TestEqual(TEXT("due parole"), Parole.Num(), 2);
	TestFalse(TEXT("una riga normale non è di rune"), ValdorsoRune::LeggiRiga(TEXT("Le rune battono piano."), Parole));
	TestTrue(TEXT("la didascalia dopo | non conta"), ValdorsoRune::LeggiRiga(TEXT("rune: Aeth-Var | Dardo infuocato"), Parole) && Parole.Num() == 1);
	// il quaderno: tutte le rune e tutte le parole con un nome, ogni riga di rune si legge
	int32 RigheDiRune = 0;
	for (const FString& Riga : ValdorsoRune::RigheQuaderno())
	{
		if (Riga.StartsWith(ValdorsoRune::PrefissoRiga))
		{
			TestTrue(FString::Printf(TEXT("quaderno: %s si legge"), *Riga), ValdorsoRune::LeggiRiga(Riga, Parole));
			++RigheDiRune;
		}
	}
	TestEqual(TEXT("quaderno: 5 righe dell'alfabeto e 18 parole"), RigheDiRune, 5 + ValdorsoRune::ParoleConNome().Num());
	TestEqual(TEXT("18 parole con un nome"), ValdorsoRune::ParoleConNome().Num(), 18);
	return true;
}

// Le etichette scritte da crea_villaggio.py ("ValdorsoFase da-fino") e quando una cosa c'è.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestFasiVillaggio, "Valdorso.Mondo.FasiVillaggio", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestFasiVillaggio::RunTest(const FString& Parameters)
{
	int32 Da = 0;
	int32 Fino = 0;
	TestTrue(TEXT("\"ValdorsoFase 2-0\" si legge"), ValdorsoFasi::LeggiEtichetta(FName(TEXT("ValdorsoFase 2-0")), Da, Fino));
	TestEqual(TEXT("2-0: dalla fase 2"), Da, 2);
	TestEqual(TEXT("2-0: per sempre"), Fino, 0);
	TestTrue(TEXT("\"ValdorsoFase 1-1\" si legge"), ValdorsoFasi::LeggiEtichetta(FName(TEXT("ValdorsoFase 1-1")), Da, Fino));
	TestEqual(TEXT("1-1: da 1"), Da, 1);
	TestEqual(TEXT("1-1: fino a 1"), Fino, 1);

	// etichette che non sono di fase, o sbagliate: la cosa resta com'è
	TestFalse(TEXT("ValdorsoVillaggio non è una fase"), ValdorsoFasi::LeggiEtichetta(FName(TEXT("ValdorsoVillaggio")), Da, Fino));
	TestFalse(TEXT("2-1 è sbagliata"), ValdorsoFasi::LeggiEtichetta(FName(TEXT("ValdorsoFase 2-1")), Da, Fino));
	TestFalse(TEXT("0-3 è sbagliata"), ValdorsoFasi::LeggiEtichetta(FName(TEXT("ValdorsoFase 0-3")), Da, Fino));
	TestFalse(TEXT("senza numeri è sbagliata"), ValdorsoFasi::LeggiEtichetta(FName(TEXT("ValdorsoFase a-b")), Da, Fino));
	TestFalse(TEXT("senza trattino è sbagliata"), ValdorsoFasi::LeggiEtichetta(FName(TEXT("ValdorsoFase 2")), Da, Fino));

	// la palizzata stretta (1-1), le case dell'anello (2-0), la palizzata larga (2-2), il muro (3-0)
	TestTrue(TEXT("palizzata stretta nella fase 1"), ValdorsoFasi::CeNellaFase(1, 1, 1));
	TestFalse(TEXT("palizzata stretta non nella fase 2"), ValdorsoFasi::CeNellaFase(2, 1, 1));
	TestFalse(TEXT("case dell'anello non nella fase 1"), ValdorsoFasi::CeNellaFase(1, 2, 0));
	TestTrue(TEXT("case dell'anello nella fase 2"), ValdorsoFasi::CeNellaFase(2, 2, 0));
	TestTrue(TEXT("case dell'anello nella fase 3"), ValdorsoFasi::CeNellaFase(3, 2, 0));
	TestTrue(TEXT("palizzata larga nella fase 2"), ValdorsoFasi::CeNellaFase(2, 2, 2));
	TestFalse(TEXT("palizzata larga non nella fase 3"), ValdorsoFasi::CeNellaFase(3, 2, 2));
	TestFalse(TEXT("muro non nella fase 2"), ValdorsoFasi::CeNellaFase(2, 3, 0));
	TestTrue(TEXT("muro nella fase 3"), ValdorsoFasi::CeNellaFase(3, 3, 0));
	return true;
}

// La spinta del sigillo: sempre via dal velo, dalla parte dove sta il colono, con un salto; e il battito della luce.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestSigillo, "Valdorso.Mondo.Sigillo", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestSigillo::RunTest(const FString& Parameters)
{
	// un sigillo girato come quello vero (verso la valle a -143 gradi), dove sta quello vero: (08/11) coordinate del mondo,
	// la porta del passo del generatore (262680, 250319) meno l'angolo del Landscape (150000, 150000)
	const FTransform Sigillo(FRotator(0.f, -143.23f, 0.f), FVector(112680.f, 100319.f, 6739.f));
	const FVector Avanti = Sigillo.GetUnitAxis(EAxis::X);

	const FVector DallaValle = Sigillo.GetLocation() + Avanti * 80.f + FVector(0.f, 0.f, 90.f);
	const FVector Spinta1 = ValdorsoSigillo::Spinta(Sigillo, DallaValle, 900.f, 380.f);
	TestTrue(TEXT("chi arriva dalla valle torna verso la valle"), FVector::DotProduct(Spinta1, Avanti) > 800.f);
	TestTrue(TEXT("con un salto in alto"), FMath::IsNearlyEqual(Spinta1.Z, 380.0, 0.01));

	const FVector DalPasso = Sigillo.GetLocation() - Avanti * 80.f;
	const FVector Spinta2 = ValdorsoSigillo::Spinta(Sigillo, DalPasso, 900.f, 380.f);
	TestTrue(TEXT("chi è dalla parte del passo viene spinto via dal velo, verso il passo"), FVector::DotProduct(Spinta2, Avanti) < -800.f);

	// anche di lato (lungo il velo) la spinta resta di traverso al velo, mai lungo
	const FVector DiLato = Sigillo.GetLocation() + Sigillo.GetUnitAxis(EAxis::Y) * 1500.f + Avanti * 10.f;
	const FVector Spinta3 = ValdorsoSigillo::Spinta(Sigillo, DiLato, 900.f, 380.f);
	TestTrue(TEXT("di lato: niente spinta lungo il velo"), FMath::Abs(FVector::DotProduct(Spinta3, Sigillo.GetUnitAxis(EAxis::Y))) < 1.f);

	// la luce: di notte più bassa che di giorno (l'esposizione automatica la fa sembrare più forte), e il battito (il primo colpo a 0,1 s) più forte della pausa (1,2 s)
	TestTrue(TEXT("di notte la luce vera è più bassa (l'occhio si abitua al buio)"), ValdorsoSigillo::Luminosita(1.2f, 23.f) < ValdorsoSigillo::Luminosita(1.2f, 12.f) * 0.5f);
	TestTrue(TEXT("il colpo del battito si vede"), ValdorsoSigillo::Luminosita(0.1f, 12.f) > ValdorsoSigillo::Luminosita(1.2f, 12.f) * 1.2f);
	TestTrue(TEXT("di giorno a riposo vale circa 1"), FMath::IsNearlyEqual(ValdorsoSigillo::Luminosita(1.2f, 12.f), 1.f, 0.02f));
	return true;
}

// Le scale a pioli: quando si esce in cima o si arriva in fondo, e il punto per E alla propria altezza.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestScale, "Valdorso.Mondo.Scale", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestScale::RunTest(const FString& Parameters)
{
	const float Altezza = 1320.f;   // la torre della milizia
	const float Mezza = 90.f;
	TestEqual(TEXT("a metà, salendo: niente"), ValdorsoScale::Estremo(700.f, Altezza, Mezza, 140.f), 0);
	TestEqual(TEXT("piedi sotto il ballatoio, salendo: esce in cima"), ValdorsoScale::Estremo(Altezza - 50.f + Mezza, Altezza, Mezza, 140.f), 1);
	TestEqual(TEXT("in cima ma fermo: resta"), ValdorsoScale::Estremo(Altezza - 50.f + Mezza, Altezza, Mezza, 0.f), 0);
	TestEqual(TEXT("in cima scendendo: resta (sta scendendo)"), ValdorsoScale::Estremo(Altezza - 50.f + Mezza, Altezza, Mezza, -140.f), 0);
	TestEqual(TEXT("piedi a terra, scendendo: in fondo"), ValdorsoScale::Estremo(Mezza + 3.f, Altezza, Mezza, -140.f), -1);
	TestEqual(TEXT("piedi a terra, salendo: resta (appena salito)"), ValdorsoScale::Estremo(Mezza + 3.f, Altezza, Mezza, 140.f), 0);
	// chi entra dalla cima parte 120 cm sotto il ballatoio: non deve uscire subito
	TestEqual(TEXT("partenza dalla cima: non esce subito"), ValdorsoScale::Estremo(Altezza - 120.f + Mezza, Altezza, Mezza, 140.f), 0);

	TestEqual(TEXT("E da terra: un metro su"), ValdorsoScale::QuotaInterazione(90.f, Altezza), 100.f);
	TestEqual(TEXT("E dal ballatoio: alla sua altezza"), ValdorsoScale::QuotaInterazione(Altezza + 90.f, Altezza), Altezza + 90.f);
	TestEqual(TEXT("E da molto sopra: non oltre un metro sopra la cima"), ValdorsoScale::QuotaInterazione(Altezza + 900.f, Altezza), Altezza + 100.f);
	return true;
}

// Il nuoto: dove c'è l'acqua (lago, fiume) e quanto del corpo è sotto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestAcqua, "Valdorso.Mondo.Acqua", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestAcqua::RunTest(const FString& Parameters)
{
	// un lago quadrato di 100 m
	const TArray<FVector2D> Riva = { FVector2D(0.0, 0.0), FVector2D(10000.0, 0.0), FVector2D(10000.0, 10000.0), FVector2D(0.0, 10000.0) };
	TestTrue(TEXT("in mezzo al lago"), ValdorsoAcqua::DentroRiva(Riva, FVector2D(5000.0, 5000.0)));
	TestFalse(TEXT("fuori dal lago"), ValdorsoAcqua::DentroRiva(Riva, FVector2D(-100.0, 5000.0)));

	// un fiume dritto largo 10 m, che scende da 100 m a 90 m in 200 m
	FValdorsoCorsoAcqua Fiume;
	Fiume.Punti = { FVector(0.0, 0.0, 10000.0), FVector(20000.0, 0.0, 9000.0) };
	Fiume.Larghezze = { 1000.f, 1000.f };
	float Pelo = 0.f;
	TestTrue(TEXT("a metà del fiume, 3 m dal mezzo: acqua"), ValdorsoAcqua::PeloSulCorso(Fiume, FVector2D(10000.0, 300.0), 0.f, Pelo));
	TestTrue(TEXT("il pelo a metà è a 95 m"), FMath::IsNearlyEqual(Pelo, 9500.f, 1.f));
	TestFalse(TEXT("8 m dal mezzo, senza margine: asciutto"), ValdorsoAcqua::PeloSulCorso(Fiume, FVector2D(10000.0, 800.0), 0.f, Pelo));
	TestTrue(TEXT("8 m dal mezzo, con 3 m di margine: acqua"), ValdorsoAcqua::PeloSulCorso(Fiume, FVector2D(10000.0, 800.0), 300.f, Pelo));

	// il corpo (capsula alta 180 cm, centro a 1 m dal fondo)
	TestEqual(TEXT("acqua sotto i piedi: niente"), ValdorsoAcqua::Immersione(0.f, 100.f, 90.f), 0.f);
	TestTrue(TEXT("acqua al petto: circa 0,6"), FMath::IsNearlyEqual(ValdorsoAcqua::Immersione(118.f, 100.f, 90.f), 0.6f, 0.01f));
	TestEqual(TEXT("sotto del tutto: 1"), ValdorsoAcqua::Immersione(500.f, 100.f, 90.f), 1.f);
	return true;
}

// Il tempo della valle: i nomi per il comando, le sfumature e la scelta del tempo dopo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTempo, "Valdorso.Mondo.Tempo", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestTempo::RunTest(const FString& Parameters)
{
	EValdorsoTempo T = EValdorsoTempo::Sereno;
	TestTrue(TEXT("\"pioggia\" si capisce"), ValdorsoTempo::DaNome(TEXT("pioggia"), T) && T == EValdorsoTempo::Pioggia);
	TestTrue(TEXT("\"4\" è il temporale"), ValdorsoTempo::DaNome(TEXT("4"), T) && T == EValdorsoTempo::Temporale);
	TestFalse(TEXT("\"grandine\" non c'è"), ValdorsoTempo::DaNome(TEXT("grandine"), T));
	TestFalse(TEXT("\"9\" non c'è"), ValdorsoTempo::DaNome(TEXT("9"), T));
	for (int32 i = 0; i <= 4; ++i)
	{
		EValdorsoTempo Indietro;
		TestTrue(TEXT("ogni nome torna indietro"), ValdorsoTempo::DaNome(ValdorsoTempo::Nome(static_cast<EValdorsoTempo>(i)), Indietro)
			&& Indietro == static_cast<EValdorsoTempo>(i));
	}

	const FValdorsoAria Sereno = ValdorsoTempo::Aria(EValdorsoTempo::Sereno);
	const FValdorsoAria Pioggia = ValdorsoTempo::Aria(EValdorsoTempo::Pioggia);
	TestEqual(TEXT("col sereno non piove"), Sereno.Pioggia, 0.f);
	TestTrue(TEXT("con la pioggia piove"), Pioggia.Pioggia > 0.5f);
	const FValdorsoAria Meta = ValdorsoTempo::Mescola(Sereno, Pioggia, 0.5f);
	TestTrue(TEXT("a metà del cambio piove a metà"), FMath::IsNearlyEqual(Meta.Pioggia, Pioggia.Pioggia * 0.5f, 0.001f));

	// il tempo dopo: con il caso a 0 si resta sul primo della lista (sereno), con il caso quasi a 1 l'ultimo possibile
	TestTrue(TEXT("caso 0: sereno"), ValdorsoTempo::Prossimo(EValdorsoTempo::Sereno, 0.f, 12.f) == EValdorsoTempo::Sereno);
	TestTrue(TEXT("caso quasi 1 dopo il sereno: temporale"), ValdorsoTempo::Prossimo(EValdorsoTempo::Sereno, 0.9999f, 12.f) == EValdorsoTempo::Temporale);
	TestTrue(TEXT("dopo la nebbia, caso quasi 1: la pioggia (il temporale non viene mai)"), ValdorsoTempo::Prossimo(EValdorsoTempo::Nebbia, 0.9999f, 12.f) == EValdorsoTempo::Pioggia);

	// (06/11) con la pioggia è cupo e il sole quasi non passa; col sereno tutto come prima
	TestTrue(TEXT("col sereno passa tutto il sole"), FMath::IsNearlyEqual(ValdorsoTempo::LuceSole(Sereno), 1.f, 0.001f));
	TestTrue(TEXT("con la pioggia il sole quasi sparisce"), ValdorsoTempo::LuceSole(Pioggia) < 0.1f);
	TestTrue(TEXT("col sereno non è cupo"), ValdorsoTempo::Cupo(Sereno) < 0.001f);
	TestTrue(TEXT("con la pioggia è cupo"), ValdorsoTempo::Cupo(Pioggia) > 0.6f);
	// la nebbia: col sereno resta quella del livello; con la nebbia è molto più fitta
	const FValdorsoAria Nebbia = ValdorsoTempo::Aria(EValdorsoTempo::Nebbia);
	TestTrue(TEXT("col sereno la nebbia è quella di serie"), FMath::IsNearlyEqual(ValdorsoTempo::NebbiaVicina(0.01f, Sereno), 0.01f, 0.0001f));
	TestTrue(TEXT("con la nebbia è fitta"), ValdorsoTempo::NebbiaVicina(0.01f, Nebbia) > 0.25f);
	// la densità da dare al livello: a 0 cm sopra è la stessa; 1000 cm sopra con caduta 1 raddoppia (2^1)
	TestTrue(TEXT("nebbia all'altezza della nebbia"), FMath::IsNearlyEqual(ValdorsoTempo::DensitaPerAltezza(0.2f, 0.5f, 0.f), 0.2f, 0.0001f));
	TestTrue(TEXT("nebbia 10 m più su"), FMath::IsNearlyEqual(ValdorsoTempo::DensitaPerAltezza(0.2f, 1.f, 1000.f), 0.4f, 0.0001f));
	TestTrue(TEXT("e il contrario torna indietro"), FMath::IsNearlyEqual(
		ValdorsoTempo::DensitaPerAltezza(ValdorsoTempo::DensitaPerAltezza(0.05f, 0.2f, -30000.f), 0.2f, 30000.f), 0.05f, 0.0001f));
	// le ombre delle nuvole: col sereno poche (più di 3/4 del terreno al sole), con la pioggia quasi tutto coperto e più morbido
	const FValdorsoOmbreNuvole OmbreSereno = ValdorsoTempo::OmbreNuvole(Sereno);
	const FValdorsoOmbreNuvole OmbrePioggia = ValdorsoTempo::OmbreNuvole(Pioggia);
	TestTrue(TEXT("col sereno poche nuvole"), OmbreSereno.Soglia > 0.7f);
	TestTrue(TEXT("con la pioggia quasi tutto coperto"), OmbrePioggia.Soglia < 0.3f);
	TestTrue(TEXT("con la pioggia le ombre sono più morbide"), OmbrePioggia.Morbidezza > OmbreSereno.Morbidezza);
	return true;
}

// (06/11) Le movenze: sulla scala dalla velocità in su, in acqua da dove va il colono.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestMovenze, "Valdorso.Mondo.Movenze", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestMovenze::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("sale"), ValdorsoMovenze::Scala(140.f) == EValdorsoMovenza::ScalaSu);
	TestTrue(TEXT("scende"), ValdorsoMovenze::Scala(-140.f) == EValdorsoMovenza::ScalaGiu);
	TestTrue(TEXT("fermo sulla scala"), ValdorsoMovenze::Scala(5.f) == EValdorsoMovenza::ScalaFerma);
	TestTrue(TEXT("a galla"), ValdorsoMovenze::Nuoto(FVector(10.0, 5.0, 0.0)) == EValdorsoMovenza::NuotoFermo);
	TestTrue(TEXT("nuota avanti"), ValdorsoMovenze::Nuoto(FVector(250.0, 30.0, 0.0)) == EValdorsoMovenza::NuotoAvanti);
	TestTrue(TEXT("nuota a destra"), ValdorsoMovenze::Nuoto(FVector(20.0, 200.0, 0.0)) == EValdorsoMovenza::NuotoDestra);
	TestTrue(TEXT("nuota a sinistra"), ValdorsoMovenze::Nuoto(FVector(20.0, -200.0, 0.0)) == EValdorsoMovenza::NuotoSinistra);
	// stendersi nuotando: se l'animazione è già stesa di 30 gradi se ne aggiungono 45; mai all'indietro
	TestTrue(TEXT("si aggiunge quello che manca"), FMath::IsNearlyEqual(ValdorsoMovenze::Inclinazione(75.f, 30.f), 45.f, 0.001f));
	TestTrue(TEXT("già steso: niente"), FMath::IsNearlyEqual(ValdorsoMovenze::Inclinazione(75.f, 85.f), 0.f, 0.001f));
	// inclinato di 90 gradi intorno al centro: la testa (1 m sopra) va 1 m avanti
	const FTransform Testa(FRotator::ZeroRotator, FVector(0.0, 0.0, 100.0));
	const FVector Dove = ValdorsoMovenze::Inclinato(Testa, 90.f, FVector::ZeroVector).GetLocation();
	TestTrue(TEXT("la testa va avanti"), Dove.Equals(FVector(100.0, 0.0, 0.0), 0.01));
	return true;
}

// (06/11) I suoni della valle e le campane: quando si sentono, e il punto d'acqua più vicino.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestSuoniValle, "Valdorso.Mondo.SuoniValle", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestSuoniValle::RunTest(const FString& Parameters)
{
	// le campane: solo le ore canoniche, tanti rintocchi quante le ore (a mezzogiorno 12)
	TestEqual(TEXT("prima: 6 rintocchi"), ValdorsoCampane::Rintocchi(6), 6);
	TestEqual(TEXT("mezzogiorno: 12"), ValdorsoCampane::Rintocchi(12), 12);
	TestEqual(TEXT("compieta: 9"), ValdorsoCampane::Rintocchi(21), 9);
	TestEqual(TEXT("alle 7 tace"), ValdorsoCampane::Rintocchi(7), 0);
	TestEqual(TEXT("di notte tace"), ValdorsoCampane::Rintocchi(3), 0);
	// quando si sentono
	TestTrue(TEXT("la fucina a mezzogiorno"), ValdorsoAmbiente::Volume(EValdorsoQuando::Lavoro, 12.f, 1.f, 0.f) > 0.99f);
	TestTrue(TEXT("la fucina alle 3 tace"), ValdorsoAmbiente::Volume(EValdorsoQuando::Lavoro, 3.f, 0.f, 0.f) < 0.01f);
	TestTrue(TEXT("la locanda di sera è piena"), ValdorsoAmbiente::Volume(EValdorsoQuando::Sera, 21.f, 0.f, 0.f) > 0.99f);
	TestTrue(TEXT("i grilli di giorno tacciono"), ValdorsoAmbiente::Grilli(1.f, 0.f) < 0.01f);
	TestTrue(TEXT("gli uccelli con la pioggia tacciono"), ValdorsoAmbiente::Uccelli(10.f, 1.f, 0.8f) < 0.01f);
	TestTrue(TEXT("con la pioggia c'è più vento"), ValdorsoAmbiente::Vento(0.9f, 0.7f) > ValdorsoAmbiente::Vento(0.f, 0.f));
	// il punto più vicino sul fiume: sulla linea, con il pelo di lì
	FVector Q;
	const TArray<FVector> Linea = { FVector(0.0, 0.0, 100.0), FVector(1000.0, 0.0, 200.0) };
	TestTrue(TEXT("c'è un punto"), ValdorsoAmbiente::PiuVicinoSuLinea(Linea, FVector(500.0, 300.0, 5000.0), Q));
	TestTrue(TEXT("il punto è a metà, con il pelo a metà"), Q.Equals(FVector(500.0, 0.0, 150.0), 0.01));
	const TArray<FVector2D> Riva = { FVector2D(0.0, 0.0), FVector2D(100.0, 0.0), FVector2D(100.0, 100.0), FVector2D(0.0, 100.0) };
	TestTrue(TEXT("sulla riva"), ValdorsoAmbiente::PiuVicinoSuRiva(Riva, 50.f, FVector(-30.0, 50.0, 0.0), Q) && Q.Equals(FVector(0.0, 50.0, 50.0), 0.01));
	return true;
}

// (06/11) La valle che reagisce: da quanto lontano scappano gli uccelli, quando saltano i pesci, l'arco del salto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestValleViva, "Valdorso.Mondo.ValleViva", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestValleViva::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("chi corre li spaventa da più lontano"), ValdorsoValleViva::DistanzaSpavento(500.f) > ValdorsoValleViva::DistanzaSpavento(150.f));
	TestTrue(TEXT("chi è fermo deve andarci vicino"), ValdorsoValleViva::DistanzaSpavento(0.f) < 400.f);
	TestTrue(TEXT("al tramonto saltano"), ValdorsoValleViva::OraDeiPesci(18.5f));
	TestTrue(TEXT("all'alba saltano"), ValdorsoValleViva::OraDeiPesci(6.f));
	TestFalse(TEXT("a mezzogiorno no"), ValdorsoValleViva::OraDeiPesci(12.f));
	const FVector Inizio(0.0, 0.0, 100.0);
	const FVector Meta = ValdorsoValleViva::Salto(Inizio, FVector(1.0, 0.0, 0.0), 120.f, 80.f, 0.5f);
	TestTrue(TEXT("a metà salto è in alto e a metà strada"), Meta.Equals(FVector(60.0, 0.0, 180.0), 0.01));
	const FVector Fine = ValdorsoValleViva::Salto(Inizio, FVector(1.0, 0.0, 0.0), 120.f, 80.f, 1.f);
	TestTrue(TEXT("alla fine rientra"), Fine.Equals(FVector(120.0, 0.0, 100.0), 0.01));
	// le rane: tacciono se passi vicino muovendoti, non se sei lontano o fermo
	TestTrue(TEXT("le rane tacciono"), ValdorsoAmbiente::RaneSpaventate(800.f, 300.f));
	TestFalse(TEXT("lontano cantano"), ValdorsoAmbiente::RaneSpaventate(3000.f, 300.f));
	TestFalse(TEXT("fermo cantano"), ValdorsoAmbiente::RaneSpaventate(800.f, 0.f));
	return true;
}

// (06/11) Le rune delle rovine: battono due volte e poi tacciono, di giorno sono spente.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRune, "Valdorso.Mondo.Rune", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestRune::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("il primo colpo"), ValdorsoRune::Battito(0.1f) > 0.9f);
	TestTrue(TEXT("il secondo colpo"), ValdorsoRune::Battito(0.38f) > 0.6f);
	TestTrue(TEXT("la pausa"), ValdorsoRune::Battito(1.0f) < 0.05f);
	TestTrue(TEXT("si ripete"), FMath::IsNearlyEqual(ValdorsoRune::Battito(1.7f), ValdorsoRune::Battito(0.1f), 0.001f));
	TestTrue(TEXT("di giorno spente"), ValdorsoRune::Luce(0.f, 1.f) < 0.001f);
	TestTrue(TEXT("di notte accese"), ValdorsoRune::Luce(1.f, 0.f) > 1.f);
	TestTrue(TEXT("sul colpo più forti"), ValdorsoRune::Luce(1.f, 1.f) > ValdorsoRune::Luce(1.f, 0.f));
	return true;
}

// (07/11) Lo stato del mondo che il server racconta: il JSON va e torna uguale, l'ora, l'indirizzo della pagina, la notizia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestStatoMondo, "Valdorso.Mondo.StatoMondo", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestStatoMondo::RunTest(const FString& Parameters)
{
	FValdorsoStatoMondo Stato;
	Stato.Versione = TEXT("0.1.3");
	Stato.NomeVersione = TEXT("La valle");
	Stato.Ora = 18.67f;
	Stato.Tempo = TEXT("Sereno");
	Stato.Stagione = TEXT("autunno");
	Stato.Coloni = 3;
	Stato.Posti = 64;
	Stato.Notizia = TEXT("Stasera alla locanda si canta: \"la notte dei morti\".");
	Stato.Aggiornato = TEXT("2026-11-07T20:00:00.000Z");
	FValdorsoStatoMondo Letto;
	TestTrue(TEXT("il JSON si rilegge"), ValdorsoStatoMondo::DaJson(ValdorsoStatoMondo::InJson(Stato), Letto));
	TestEqual(TEXT("versione"), Letto.Versione, Stato.Versione);
	TestTrue(TEXT("ora"), FMath::IsNearlyEqual(Letto.Ora, 18.67f, 0.01f));
	TestEqual(TEXT("tempo"), Letto.Tempo, Stato.Tempo);
	TestEqual(TEXT("coloni"), Letto.Coloni, 3);
	TestEqual(TEXT("posti"), Letto.Posti, 64);
	TestEqual(TEXT("la notizia con le virgolette"), Letto.Notizia, Stato.Notizia);
	TestTrue(TEXT("aperto"), Letto.bAperto);
	TestFalse(TEXT("un JSON di un altro gioco non vale"), ValdorsoStatoMondo::DaJson(TEXT("{\"gioco\":\"Altro\"}"), Letto));
	TestFalse(TEXT("un testo rotto non vale"), ValdorsoStatoMondo::DaJson(TEXT("<html>"), Letto));

	TestEqual(TEXT("18,67 = 18:40"), ValdorsoStatoMondo::OraLeggibile(18.67f), FString(TEXT("18:40")));
	TestEqual(TEXT("7,1 = 07:06"), ValdorsoStatoMondo::OraLeggibile(7.1f), FString(TEXT("07:06")));
	TestEqual(TEXT("23,999 = 23:59"), ValdorsoStatoMondo::OraLeggibile(23.999f), FString(TEXT("23:59")));
	TestTrue(TEXT("ora sconosciuta = niente"), ValdorsoStatoMondo::OraLeggibile(-1.f).IsEmpty());

	TestEqual(TEXT("indirizzo con porta"), ValdorsoStatoMondo::IndirizzoStato(TEXT("127.0.0.1:7777")), FString(TEXT("http://127.0.0.1:7780/stato")));
	TestEqual(TEXT("nome senza porta"), ValdorsoStatoMondo::IndirizzoStato(TEXT("valle.example.org")), FString(TEXT("http://valle.example.org:7780/stato")));
	TestEqual(TEXT("con unreal://"), ValdorsoStatoMondo::IndirizzoStato(TEXT("unreal://10.0.0.5:7777")), FString(TEXT("http://10.0.0.5:7780/stato")));
	TestTrue(TEXT("vuoto = vuoto"), ValdorsoStatoMondo::IndirizzoStato(TEXT("  ")).IsEmpty());

	TestEqual(TEXT("vale l'ultima riga scritta"), ValdorsoStatoMondo::NotiziaDaFile(TEXT("# commento\nPrima notizia\nSeconda notizia\n\n# altro commento\n")), FString(TEXT("Seconda notizia")));
	TestTrue(TEXT("solo commenti = niente"), ValdorsoStatoMondo::NotiziaDaFile(TEXT("# solo\n#commenti")).IsEmpty());

	TestEqual(TEXT("novembre = autunno"), ValdorsoStatoMondo::StagioneDi(FDateTime(2026, 11, 7)), FString(TEXT("autunno")));
	TestEqual(TEXT("gennaio = inverno"), ValdorsoStatoMondo::StagioneDi(FDateTime(2027, 1, 15)), FString(TEXT("inverno")));
	TestTrue(TEXT("la riga dice l'ora"), ValdorsoStatoMondo::Riga(Stato).ToString().Contains(TEXT("18:40")));
	Stato.bAperto = false;
	TestTrue(TEXT("valle chiusa"), ValdorsoStatoMondo::Riga(Stato).ToString().Contains(TEXT("chiusa")));
	return true;
}

// (07/11) La valle come territorio e il battito della pietra del villaggio.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestTerritorio, "Valdorso.Mondo.Territorio", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestTerritorio::RunTest(const FString& Parameters)
{
	// (08/11) Coordinate del mondo: il Landscape di L_Valle ha l'angolo nord-ovest in (-150000, -150000), i punti del
	// generatore meno 150000 (ValdorsoLuoghiDati.inl). La piazza e il posto di guardia del passo, come nei luoghi.
	TestEqual(TEXT("la piazza è in Val d'Orso"), ValdorsoTerritori::IdDi(FVector(15300.0, 42228.0, 5517.0)), FString(TEXT("valdorso")));
	TestEqual(TEXT("anche il passo"), ValdorsoTerritori::IdDi(FVector(111399.0, 99361.4, 6739.0)), FString(TEXT("valdorso")));
	TestEqual(TEXT("anche la grotta nord, in cima ai monti"), ValdorsoTerritori::IdDi(FVector(0.0, -108120.0, 16502.0)), FString(TEXT("valdorso")));
	TestTrue(TEXT("il signore è la Corona"), ValdorsoTerritori::ValDOrso().Signore.ToString().Contains(TEXT("Corona")));
	TestEqual(TEXT("forza 1 = piena"), ValdorsoTerritori::ParolaForza(1.f).ToString(), FString(TEXT("piena")));
	TestEqual(TEXT("forza 0,4 = debole"), ValdorsoTerritori::ParolaForza(0.4f).ToString(), FString(TEXT("debole")));

	// sopra metà forza il battito non salta mai; a forza 0 salta circa un giro su due
	int32 SaltiForte = 0;
	int32 SaltiVuota = 0;
	for (int64 Giro = 0; Giro < 2000; ++Giro)
	{
		SaltiForte += ValdorsoTerritori::SaltaIlBattito(Giro, 0.6f) ? 1 : 0;
		SaltiVuota += ValdorsoTerritori::SaltaIlBattito(Giro, 0.f) ? 1 : 0;
	}
	TestEqual(TEXT("forte: nessun salto"), SaltiForte, 0);
	TestTrue(TEXT("vuota: circa metà dei giri saltano"), SaltiVuota > 800 && SaltiVuota < 1200);
	TestTrue(TEXT("uguale per tutti"), ValdorsoTerritori::SaltaIlBattito(77, 0.1f) == ValdorsoTerritori::SaltaIlBattito(77, 0.1f));

	TestTrue(TEXT("il primo colpo"), ValdorsoTerritori::Battito(0.1, 1.f) > 0.9f);
	TestTrue(TEXT("il secondo colpo"), ValdorsoTerritori::Battito(0.42, 1.f) > 0.6f);
	TestTrue(TEXT("la pausa"), ValdorsoTerritori::Battito(1.1, 1.f) < 0.05f);
	// un giro saltato: niente colpo
	int64 Saltato = -1;
	for (int64 Giro = 0; Giro < 200 && Saltato < 0; ++Giro)
	{
		if (ValdorsoTerritori::SaltaIlBattito(Giro, 0.f))
		{
			Saltato = Giro;
		}
	}
	TestTrue(TEXT("c'è un giro saltato"), Saltato >= 0);
	if (Saltato >= 0)
	{
		TestTrue(TEXT("nel giro saltato non batte"), ValdorsoTerritori::Battito(Saltato * 1.6 + 0.1, 0.f) < 0.001f);
	}
	return true;
}

// (07/11) La modalità foto: la telecamera non va oltre 30 m dal colono, lo zoom resta tra 15 e 100 gradi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestFoto, "Valdorso.Mondo.Foto", ValdorsoTestMondo::Bandiere)
bool FValdorsoTestFoto::RunTest(const FString& Parameters)
{
	const FVector Colono(1000.0, 2000.0, 300.0);
	const FVector Vicino = Colono + FVector(1000.0, 0.0, 500.0);
	TestTrue(TEXT("dentro il raggio non cambia"), ValdorsoFoto::DentroIlRaggio(Colono, Vicino, 3000.f).Equals(Vicino, 0.01));
	const FVector Lontano = Colono + FVector(8000.0, 6000.0, 0.0);
	const FVector Fermato = ValdorsoFoto::DentroIlRaggio(Colono, Lontano, 3000.f);
	TestTrue(TEXT("fuori dal raggio si ferma a 30 m"), FMath::IsNearlyEqual(FVector::Dist(Colono, Fermato), 3000.0, 0.1));
	TestTrue(TEXT("nella stessa direzione"), FVector::DotProduct((Fermato - Colono).GetSafeNormal(), (Lontano - Colono).GetSafeNormal()) > 0.999);
	TestTrue(TEXT("zoom avanti stringe"), ValdorsoFoto::CampoDopoZoom(90.f, 1.f) < 90.f);
	TestTrue(TEXT("zoom indietro allarga"), ValdorsoFoto::CampoDopoZoom(60.f, -1.f) > 60.f);
	TestEqual(TEXT("mai sotto 15 gradi"), ValdorsoFoto::CampoDopoZoom(20.f, 50.f), 15.f);
	TestEqual(TEXT("mai sopra 100 gradi"), ValdorsoFoto::CampoDopoZoom(90.f, -50.f), 100.f);
	return true;
}

#endif
