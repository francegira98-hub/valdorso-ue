// Valdorso - Test automatici di v0.1.5 "Crescere" (chiusura) e v0.1.7 "La grotta del nord" (11/11): le guardie, le
// tecniche, il Vigore, i tasti di serie, gli scheletri, lo scavo della frana.
// Scritto da Claude l'11/11/2026. Si avviano come gli altri (Tools\Test Valdorso.cmd, filtro "Valdorso.Scherma" e
// "Valdorso.Grotta").

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoCombattimento.h"
#include "ValdorsoGrottaNord.h"
#include "ValdorsoScheletri.h"
#include "ValdorsoScherma.h"
#include "ValdorsoTasti.h"
#include "ValdorsoTrappole.h"
#include "ValdorsoVigore.h"

namespace ValdorsoTestScherma
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

// Le tre guardie: ognuna batte una e perde con l'altra (come sasso, carta, forbici), e con la stessa non c'e' vantaggio.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestSchermaGuardie, "Valdorso.Scherma.Guardie", ValdorsoTestScherma::Bandiere)
bool FValdorsoTestSchermaGuardie::RunTest(const FString& Parameters)
{
	const EValdorsoGuardia Tutte[] = { EValdorsoGuardia::Alta, EValdorsoGuardia::Lunga, EValdorsoGuardia::Bassa };
	for (EValdorsoGuardia A : Tutte)
	{
		int32 Vinte = 0;
		int32 Perse = 0;
		for (EValdorsoGuardia B : Tutte)
		{
			const int32 V = ValdorsoScherma::Vantaggio(A, B);
			TestEqual(TEXT("il vantaggio e' al contrario per l'altro"), ValdorsoScherma::Vantaggio(B, A), -V);
			Vinte += V > 0 ? 1 : 0;
			Perse += V < 0 ? 1 : 0;
		}
		TestEqual(TEXT("ogni guardia ne batte una"), Vinte, 1);
		TestEqual(TEXT("ogni guardia perde con una"), Perse, 1);
		TestEqual(TEXT("con la stessa: pari"), ValdorsoScherma::Vantaggio(A, A), 0);
		TestTrue(TEXT("i numeri della guardia sono sensati"), ValdorsoScherma::DannoGuardia(A) > 0.5f && ValdorsoScherma::DannoGuardia(A) < 1.6f);
		TestTrue(TEXT("la portata della guardia e' sensata"), ValdorsoScherma::PortataGuardia(A) > 0.5f && ValdorsoScherma::PortataGuardia(A) < 1.6f);
		TestFalse(TEXT("ogni guardia ha la spiegazione"), ValdorsoScherma::SpiegaGuardia(A).IsEmpty());
	}
	TestEqual(TEXT("Alta batte Bassa"), ValdorsoScherma::Vantaggio(EValdorsoGuardia::Alta, EValdorsoGuardia::Bassa), 1);
	TestTrue(TEXT("cambiare guardia diventa piu' svelto con le evoluzioni"),
		ValdorsoScherma::SecondiCambio(4) < ValdorsoScherma::SecondiCambio(0));
	return true;
}

// Le tecniche: ognuna si ritrova dall'Id e da abilita' + livello, ha un nome, i livelli vanno da 1 a 4, quelle attive
// vanno nella barra (con la sigla), i talenti no.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestSchermaTecniche, "Valdorso.Scherma.Tecniche", ValdorsoTestScherma::Bandiere)
bool FValdorsoTestSchermaTecniche::RunTest(const FString& Parameters)
{
	const TArray<FValdorsoTecnica>& Tutte = ValdorsoScherma::Tecniche();
	TestTrue(TEXT("ci sono tecniche"), Tutte.Num() >= 20);
	TSet<FName> Viste;
	TSet<FString> Coppie;
	for (const FValdorsoTecnica& T : Tutte)
	{
		const FString Chi = T.Id.ToString();
		TestTrue(Chi + TEXT(": l'Id comincia con Tecnica."), Chi.StartsWith(TEXT("Tecnica.")));
		TestFalse(Chi + TEXT(": Id ripetuto"), Viste.Contains(T.Id));
		Viste.Add(T.Id);
		TestTrue(Chi + TEXT(": livello da 1 a 4"), T.Livello >= 1 && T.Livello <= 4);
		TestTrue(Chi + TEXT(": ha un nome"), FCString::Strlen(T.Nome) > 0);
		TestTrue(Chi + TEXT(": ha la descrizione"), FCString::Strlen(T.Descrizione) > 0);
		TestTrue(Chi + TEXT(": Tecnica(Id) la ritrova"), ValdorsoScherma::Tecnica(T.Id) == &T);
		const FString Coppia = FString::Printf(TEXT("%s/%d"), *T.Abilita.ToString(), T.Livello);
		TestFalse(Chi + TEXT(": due tecniche alla stessa soglia della stessa abilita'"), Coppie.Contains(Coppia));
		Coppie.Add(Coppia);
		TestTrue(Chi + TEXT(": TecnicaDi la ritrova"), ValdorsoScherma::TecnicaDi(T.Abilita, T.Livello) == &T);
		const bool bAttiva = T.Tipo == EValdorsoTipoTecnica::Colpo || T.Tipo == EValdorsoTipoTecnica::Stato;
		TestEqual(Chi + TEXT(": nella barra solo le attive"), ValdorsoScherma::DaBarra(T.Id), bAttiva);
		if (bAttiva)
		{
			TestTrue(Chi + TEXT(": la sigla per la barra"), FCString::Strlen(T.Sigla) > 0 && FCString::Strlen(T.Sigla) <= 4);
			TestTrue(Chi + TEXT(": costa fiato e ha la ricarica"), T.Stamina >= 0.f && T.Ricarica > 0.f);
			TestFalse(Chi + TEXT(": Sigla() non vuota"), ValdorsoScherma::Sigla(T.Id).IsEmpty());
		}
		if (T.Tipo == EValdorsoTipoTecnica::Colpo)
		{
			TestTrue(Chi + TEXT(": almeno un colpo"), T.Colpi >= 1 && T.Danno > 0.f);
		}
	}
	TestTrue(TEXT("un Id che non c'e'"), ValdorsoScherma::Tecnica(TEXT("Tecnica.NonEsiste")) == nullptr);
	TestFalse(TEXT("un oggetto non e' una tecnica"), ValdorsoScherma::DaBarra(TEXT("Mela")));
	return true;
}

// Il Vigore (Medio, scelta di Fra del 10/11): la corsa costa meno con l'Atletica, il nuoto veloce meno con il Nuoto,
// le cadute fanno male oltre una certa velocita' (la capriola un terzo).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestSchermaVigore, "Valdorso.Scherma.Vigore", ValdorsoTestScherma::Bandiere)
bool FValdorsoTestSchermaVigore::RunTest(const FString& Parameters)
{
	using namespace ValdorsoVigore;
	TestEqual(TEXT("corsa da novellino"), CalaCorsaPer(0.f, 0), CalaCorsa);
	TestTrue(TEXT("l'Atletica fa correre di piu'"), CalaCorsaPer(100.f, 0) < CalaCorsaPer(0.f, 0));
	TestTrue(TEXT("le evoluzioni ancora di piu'"), CalaCorsaPer(100.f, 4) < CalaCorsaPer(100.f, 1));
	TestTrue(TEXT("mai gratis"), CalaCorsaPer(100.f, 4) > 0.f);
	TestTrue(TEXT("il nuoto veloce costa meno con il Nuoto"), CalaNuotoPer(1) < CalaNuotoPer(0));
	TestEqual(TEXT("una caduta piccola non fa male"), DannoCaduta(CadutaInnocua - 1.f, false), 0.f);
	TestEqual(TEXT("una caduta forte"), DannoCaduta(CadutaInnocua + 500.f, false), 50.f);
	TestEqual(TEXT("con la capriola un terzo"), DannoCaduta(CadutaInnocua + 600.f, true), 20.f);
	TestFalse(TEXT("la capriola senza Atletica no"), CapriolaPossibile(1500.f, 0));
	TestTrue(TEXT("la capriola con l'Atletica"), CapriolaPossibile(1500.f, 1));
	TestFalse(TEXT("da troppo in alto no"), CapriolaPossibile(CapriolaFino + 100.f, 1));
	TestTrue(TEXT("evoluta, da piu' in alto si"), CapriolaPossibile(CapriolaFino + 100.f, 2));
	TestTrue(TEXT("la schivata non si ripete subito"), SecondiTraSchivate > SecondiInvulnerabile);
	return true;
}

// I tasti di serie: ogni azione ha il suo, nessuno e' doppio, nessuno e' riservato (Esc, F9, i numeri della barra...).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestSchermaTasti, "Valdorso.Scherma.Tasti", ValdorsoTestScherma::Bandiere)
bool FValdorsoTestSchermaTasti::RunTest(const FString& Parameters)
{
	TMap<FName, int32> Usati;
	for (int32 i = 0; i < static_cast<int32>(EValdorsoTasto::Numero); ++i)
	{
		const EValdorsoTasto Azione = static_cast<EValdorsoTasto>(i);
		const FKey Tasto = ValdorsoTasti::DiSerie(Azione);
		const FString Chi = ValdorsoTasti::Nome(Azione).ToString();
		TestTrue(Chi + TEXT(": ha un tasto"), Tasto.IsValid());
		TestFalse(Chi + TEXT(": ha un nome"), Chi.IsEmpty());
		TestFalse(Chi + TEXT(": il tasto ha un nome in italiano"), ValdorsoTasti::NomeTasto(Tasto).IsEmpty());
		TestFalse(Chi + TEXT(": il tasto di serie non e' riservato"), ValdorsoTasti::Riservato(Tasto));
		if (const int32* Prima = Usati.Find(Tasto.GetFName()))
		{
			AddError(FString::Printf(TEXT("%s: lo stesso tasto di serie di %s"), *Chi,
				*ValdorsoTasti::Nome(static_cast<EValdorsoTasto>(*Prima)).ToString()));
		}
		Usati.Add(Tasto.GetFName(), i);
	}
	TestTrue(TEXT("Esc e' riservato"), ValdorsoTasti::Riservato(EKeys::Escape));
	return true;
}

// Gli scheletri: tre tipi in ordine di forza, le botte fanno di piu' delle lame, quando si svegliano, quando lasciano.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGrottaScheletri, "Valdorso.Grotta.Scheletri", ValdorsoTestScherma::Bandiere)
bool FValdorsoTestGrottaScheletri::RunTest(const FString& Parameters)
{
	using namespace ValdorsoScheletri;
	TestEqual(TEXT("quattro tipi (con il guardiano della forgia)"), Tipi().Num(), 4);
	const FValdorsoTipoScheletro* Guardiano = Tipo(TEXT("Guardiano"));
	TestTrue(TEXT("il guardiano e' il piu' forte, di pietra, e non teme il fuoco"),
		Guardiano && Guardiano->Forza == 80.f && Guardiano->Pietra > 0.f && !Guardiano->bTemeFuoco);
	const FValdorsoTipoScheletro* Semplice = Tipo(TEXT("Scheletro"));
	const FValdorsoTipoScheletro* Scudo = Tipo(TEXT("ScheletroScudo"));
	const FValdorsoTipoScheletro* Capitano = Tipo(TEXT("Capitano"));
	if (!TestTrue(TEXT("i tre tipi ci sono"), Semplice && Scudo && Capitano))
	{
		return false;
	}
	TestTrue(TEXT("forza in ordine"), Semplice->Forza < Scudo->Forza && Scudo->Forza < Capitano->Forza);
	TestTrue(TEXT("forza da 40 a 70"), Semplice->Forza >= 40.f && Capitano->Forza <= 70.f);
	TestTrue(TEXT("solo chi ha lo scudo para"), Semplice->Parata == 0.f && Scudo->Parata > 0.f);
	TestTrue(TEXT("il capitano e' grande"), Capitano->Grandezza > 1.f);
	TestTrue(TEXT("un tipo che non c'e'"), Tipo(TEXT("Drago")) == nullptr);
	TestTrue(TEXT("le botte fanno di piu'"), Debolezza(EValdorsoTipoColpo::Botta) > 1.f);
	TestTrue(TEXT("le lame di meno"), Debolezza(EValdorsoTipoColpo::Lama) < 1.f);
	TestTrue(TEXT("i morsi quasi niente"), Debolezza(EValdorsoTipoColpo::Morso) < Debolezza(EValdorsoTipoColpo::Lama));

	using S = EValdorsoStatoScheletro;
	TestTrue(TEXT("nessuno vicino: dorme"), Prossimo(S::Dorme, -1.f, false, false, 10.f, 0.f) == S::Dorme);
	TestTrue(TEXT("un colono lontano: dorme"), Prossimo(S::Dorme, Sveglia + 100.f, false, false, 10.f, 0.f) == S::Dorme);
	TestTrue(TEXT("un colono vicino: si alza"), Prossimo(S::Dorme, Sveglia - 100.f, false, false, 10.f, 0.f) == S::SiAlza);
	TestTrue(TEXT("colpito da lontano: si alza"), Prossimo(S::Dorme, -1.f, true, false, 10.f, 0.f) == S::SiAlza);
	TestTrue(TEXT("si alza piano"), Prossimo(S::SiAlza, 300.f, false, false, SecondiPerAlzarsi * 0.5f, 0.f) == S::SiAlza);
	TestTrue(TEXT("poi caccia"), Prossimo(S::SiAlza, 300.f, false, false, SecondiPerAlzarsi + 0.1f, 0.f) == S::Caccia);
	TestTrue(TEXT("caccia finche' e' vicino"), Prossimo(S::Caccia, 1000.f, false, false, 5.f, 500.f) == S::Caccia);
	TestTrue(TEXT("scappato lontano: torna"), Prossimo(S::Caccia, Lascia + 100.f, false, false, 5.f, 500.f) == S::Torna);
	TestTrue(TEXT("a casa: si riaddormenta"), Prossimo(S::Torna, -1.f, false, false, 5.f, 50.f) == S::Dorme);
	TestTrue(TEXT("tornando, se ritrova uno vicino: caccia"), Prossimo(S::Torna, 300.f, false, false, 1.f, 900.f) == S::Caccia);
	TestTrue(TEXT("ma non vicino al fuoco"), Prossimo(S::Torna, 300.f, false, true, 1.f, 900.f) == S::Torna);
	TestTrue(TEXT("morto resta morto"), Prossimo(S::Morto, 100.f, true, false, 1.f, 0.f) == S::Morto);
	return true;
}

// La frana del nord: circa duecento colpi di piccone (di meno con la Tempra), e i nomi della storia di Lucia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGrottaFrana, "Valdorso.Grotta.Frana", ValdorsoTestScherma::Bandiere)
bool FValdorsoTestGrottaFrana::RunTest(const FString& Parameters)
{
	const int32 ColpiNovellino = FMath::RoundToInt(1.f / ValdorsoGrottaNord::PassoScavo(0.f));
	const int32 ColpiForte = FMath::RoundToInt(1.f / ValdorsoGrottaNord::PassoScavo(100.f));
	TestEqual(TEXT("da soli, da novellini: duecento colpi"), ColpiNovellino, 200);
	TestTrue(TEXT("con la Tempra di meno"), ColpiForte < ColpiNovellino && ColpiForte > 100);
	TestTrue(TEXT("scavare costa fiato"), ValdorsoGrottaNord::StaminaScavo > 0.f);
	TestEqual(TEXT("tre cose da ritrovare"), ValdorsoGrottaNord::Tracce().Num(), 3);
	for (const FName& Traccia : ValdorsoGrottaNord::Tracce())
	{
		TestTrue(Traccia.ToString() + TEXT(": scrive nel taccuino"), ValdorsoGrottaNord::RigheTraccia(Traccia).Num() > 0);
	}
	TestFalse(TEXT("la storia ha un titolo"), ValdorsoGrottaNord::TitoloStoria().IsEmpty());
	TestTrue(TEXT("la storia ha le righe"), ValdorsoGrottaNord::RigheStoria().Num() > 0 && ValdorsoGrottaNord::RigheFine().Num() > 0);
	return true;
}

// (11/11) Le trappole dei nani: le lance escono dopo il ritardo e rientrano, la botola si apre e si richiude, il pendolo
// oscilla dentro la sua ampiezza e torna uguale a ogni periodo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestGrottaTrappole, "Valdorso.Grotta.Trappole", ValdorsoTestScherma::Bandiere)
bool FValdorsoTestGrottaTrappole::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTrappole;
	TestEqual(TEXT("le lance prima del ritardo: giu'"), LanceFuori(0.3, 0.45f), 0.f);
	TestEqual(TEXT("le lance subito dopo: fuori"), LanceFuori(0.6, 0.45f), 1.f);
	TestEqual(TEXT("le lance dopo tre secondi: di nuovo giu'"), LanceFuori(3.5, 0.45f), 0.f);
	TestFalse(TEXT("la botola regge un attimo"), BotolaAperta(0.5, 0.7f, 6.f));
	TestTrue(TEXT("poi cede"), BotolaAperta(1.0, 0.7f, 6.f));
	TestFalse(TEXT("e si richiude"), BotolaAperta(7.0, 0.7f, 6.f));
	for (double T = 0.0; T < 10.0; T += 0.37)
	{
		const float A = AngoloPendolo(T, 3.f, 0.25f, 26.f);
		if (FMath::Abs(A) > 26.001f)
		{
			AddError(TEXT("il pendolo esce dalla sua ampiezza"));
			break;
		}
		if (!FMath::IsNearlyEqual(A, AngoloPendolo(T + 3.0, 3.f, 0.25f, 26.f), 0.01f))
		{
			AddError(TEXT("il pendolo non torna uguale dopo un periodo"));
			break;
		}
	}
	TestTrue(TEXT("i due pendoli sfasati di mezzo periodo sono opposti"),
		FMath::IsNearlyEqual(AngoloPendolo(1.1, 3.f, 0.f, 26.f), -AngoloPendolo(1.1, 3.f, 0.5f, 26.f), 0.01f));
	TestTrue(TEXT("una trappola uccide"), DannoMortale > 10000.f);
	return true;
}

#endif
