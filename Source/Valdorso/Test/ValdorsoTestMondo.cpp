// Valdorso - Test automatici del mondo: le fasi del villaggio e il sigillo del passo.
// Scritto da Claude il 06/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoFasiVillaggio.h"
#include "ValdorsoSigillo.h"

namespace ValdorsoTestMondo
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
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
	// un sigillo girato come quello vero (verso la valle a -143 gradi), in un punto qualsiasi
	const FTransform Sigillo(FRotator(0.f, -143.23f, 0.f), FVector(262680.f, 250319.f, 6739.f));
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

	// la luce: di notte più forte che di giorno, e il battito (il primo colpo a 0,1 s) più forte della pausa (1,2 s)
	TestTrue(TEXT("di notte brilla di più"), ValdorsoSigillo::Luminosita(1.2f, 23.f) > ValdorsoSigillo::Luminosita(1.2f, 12.f) * 1.5f);
	TestTrue(TEXT("il colpo del battito si vede"), ValdorsoSigillo::Luminosita(0.1f, 12.f) > ValdorsoSigillo::Luminosita(1.2f, 12.f) * 1.2f);
	TestTrue(TEXT("di giorno a riposo vale circa 1"), FMath::IsNearlyEqual(ValdorsoSigillo::Luminosita(1.2f, 12.f), 1.f, 0.02f));
	return true;
}

#endif
