// Valdorso - Test automatici degli account: sicurezza, regole e file dell'archivio.
// Scritto da Claude il 04/10/2026 (idea "subito" approvata da Fra).
//
// Come si avviano:
//   nell'editor  Strumenti -> Session Frontend -> scheda Automation -> filtro "Valdorso" -> spunta -> Start Tests
//   da fuori     Tools\Test Valdorso.cmd (avvia l'editor senza finestra, fa i test e scrive il risultato)
// Ogni test è piccolo e non tocca l'archivio vero: i file di prova vanno in Saved/Automation/Tmp e poi si cancellano.
// Se un test diventa rosso, qualcosa che prima funzionava si è rotto: il messaggio dice cosa.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ValdorsoSicurezza.h"
#include "ValdorsoRegole.h"
#include "ValdorsoArchivista.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/Guid.h"
#include "UObject/GCObjectScopeGuard.h"
#include "Misc/Paths.h"

namespace ValdorsoTestAccount
{
	constexpr EAutomationTestFlags Bandiere = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	TArray<uint8> Bytes(const char* Testo)
	{
		TArray<uint8> Out;
		Out.Append(reinterpret_cast<const uint8*>(Testo), FCStringAnsi::Strlen(Testo));
		return Out;
	}

	FString Esadecimale(const TArray<uint8>& Dati)
	{
		return BytesToHex(Dati.GetData(), Dati.Num()).ToLower();
	}
}

// ------------------------------------------------------------------------------------------------
// Sicurezza
// ------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestImpronta, "Valdorso.Sicurezza.Impronta", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestImpronta::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestAccount;
	const TArray<uint8> Sale = Bytes("salt");
	TArray<uint8> Impronta;

	// Valori noti di PBKDF2-HMAC-SHA256 (gli stessi di qualunque altra libreria): se cambiano, le password salvate non entrano più.
	TestTrue(TEXT("1 giro riesce"), ValdorsoSicurezza::CalcolaImpronta(TEXT("password"), Sale, 1, Impronta));
	TestEqual(TEXT("1 giro"), Esadecimale(Impronta), FString(TEXT("120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b")));
	ValdorsoSicurezza::CalcolaImpronta(TEXT("password"), Sale, 2, Impronta);
	TestEqual(TEXT("2 giri"), Esadecimale(Impronta), FString(TEXT("ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43")));
	ValdorsoSicurezza::CalcolaImpronta(TEXT("password"), Sale, 4096, Impronta);
	TestEqual(TEXT("4096 giri"), Esadecimale(Impronta), FString(TEXT("c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a")));

	// Le lettere accentate passano in UTF-8, come in ogni altro programma.
	ValdorsoSicurezza::CalcolaImpronta(TEXT("perché"), Sale, 1, Impronta);
	TestEqual(TEXT("password con accento"), Esadecimale(Impronta), FString(TEXT("981e7f38fd8310d69288e866296e47ac679bfa6695d9306d777ca1093a379084")));

	// Sale diverso, impronta diversa.
	TArray<uint8> Altra;
	ValdorsoSicurezza::CalcolaImpronta(TEXT("password"), Bytes("sale"), 1, Altra);
	ValdorsoSicurezza::CalcolaImpronta(TEXT("password"), Sale, 1, Impronta);
	TestNotEqual(TEXT("sale diverso"), Esadecimale(Altra), Esadecimale(Impronta));
	TestEqual(TEXT("lunghezza"), Impronta.Num(), ValdorsoSicurezza::LunghezzaImpronta);

	// Richieste sbagliate: rifiutate.
	TestFalse(TEXT("0 giri"), ValdorsoSicurezza::CalcolaImpronta(TEXT("password"), Sale, 0, Impronta));
	TestFalse(TEXT("senza sale"), ValdorsoSicurezza::CalcolaImpronta(TEXT("password"), TArray<uint8>(), 1, Impronta));

	// I giri veri restano quelli decisi (OWASP 2023).
	TestTrue(TEXT("almeno 600.000 giri"), ValdorsoSicurezza::IterazioniCorrenti >= 600000);
	TestTrue(TEXT("sale di almeno 16 byte"), ValdorsoSicurezza::LunghezzaSale >= 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestConfronto, "Valdorso.Sicurezza.Confronto", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestConfronto::RunTest(const FString& Parameters)
{
	using namespace ValdorsoTestAccount;
	TestTrue(TEXT("uguali"), ValdorsoSicurezza::UgualiTempoCostante(Bytes("abcdef"), Bytes("abcdef")));
	TestFalse(TEXT("diversi"), ValdorsoSicurezza::UgualiTempoCostante(Bytes("abcdef"), Bytes("abcdeg")));
	TestFalse(TEXT("lunghezze diverse"), ValdorsoSicurezza::UgualiTempoCostante(Bytes("abc"), Bytes("abcd")));
	TestFalse(TEXT("vuoti"), ValdorsoSicurezza::UgualiTempoCostante(TArray<uint8>(), TArray<uint8>()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestSha256, "Valdorso.Sicurezza.Sha256", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestSha256::RunTest(const FString& Parameters)
{
	// Gli inviti si riconoscono da questa impronta: se cambiasse, gli inviti già creati non varrebbero più.
	TestEqual(TEXT("abc"), ValdorsoSicurezza::Sha256Esadecimale(TEXT("abc")).ToLower(),
		FString(TEXT("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")));
	TestEqual(TEXT("con accento"), ValdorsoSicurezza::Sha256Esadecimale(TEXT("Valdorso è viva")).ToLower(),
		FString(TEXT("228167da75e4417fd2ea815ad3c521015ff04daaf7aa8028480e8225979cbf41")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestCasuale, "Valdorso.Sicurezza.Casuale", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestCasuale::RunTest(const FString& Parameters)
{
	TArray<uint8> A, B;
	TestTrue(TEXT("byte casuali"), ValdorsoSicurezza::BytesCasuali(A, 32));
	ValdorsoSicurezza::BytesCasuali(B, 32);
	TestEqual(TEXT("quanti byte"), A.Num(), 32);
	TestFalse(TEXT("due estrazioni diverse"), A == B);

	const TCHAR* Alfabeto = TEXT("23456789ABCDEFGHJKMNPQRSTUVWXYZ");
	const FString Testo = ValdorsoSicurezza::TestoCasuale(Alfabeto, 500);
	TestEqual(TEXT("lunghezza del testo"), Testo.Len(), 500);
	bool bSoloAlfabeto = true;
	TSet<TCHAR> Usati;
	for (const TCHAR C : Testo)
	{
		bSoloAlfabeto &= FCString::Strchr(Alfabeto, C) != nullptr;
		Usati.Add(C);
	}
	TestTrue(TEXT("solo caratteri dell'alfabeto (niente 0, O, 1, I, L)"), bSoloAlfabeto);
	TestTrue(TEXT("usa quasi tutto l'alfabeto"), Usati.Num() >= 28);
	TestTrue(TEXT("alfabeto troppo corto: niente"), ValdorsoSicurezza::TestoCasuale(TEXT("A"), 10).IsEmpty());
	TestTrue(TEXT("zero caratteri: niente"), ValdorsoSicurezza::TestoCasuale(Alfabeto, 0).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestChiaviRSA, "Valdorso.Sicurezza.ChiaviServer", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestChiaviRSA::RunTest(const FString& Parameters)
{
	// Come al collegamento: il client chiude la chiave di sessione con la pubblica, solo la privata la apre.
	FString Privata, Pubblica, AltraPrivata, AltraPubblica;
	if (!TestTrue(TEXT("coppia di chiavi"), ValdorsoSicurezza::CreaCoppiaChiavi(Privata, Pubblica))
		|| !TestTrue(TEXT("seconda coppia"), ValdorsoSicurezza::CreaCoppiaChiavi(AltraPrivata, AltraPubblica)))
	{
		return false;
	}
	TestTrue(TEXT("la pubblica non contiene la privata"), !Pubblica.Contains(TEXT("PRIVATE")));

	TArray<uint8> Sessione, Chiusa, Aperta;
	ValdorsoSicurezza::BytesCasuali(Sessione, 32);
	TestTrue(TEXT("cifra con la pubblica"), ValdorsoSicurezza::CifraConPubblica(Pubblica, Sessione, Chiusa));
	TestTrue(TEXT("decifra con la privata"), ValdorsoSicurezza::DecifraConPrivata(Privata, Chiusa, Aperta));
	TestTrue(TEXT("torna uguale"), Aperta == Sessione);

	TArray<uint8> Sbagliata;
	TestFalse(TEXT("un'altra chiave privata non apre"), ValdorsoSicurezza::DecifraConPrivata(AltraPrivata, Chiusa, Sbagliata) && Sbagliata == Sessione);

	// Il token viaggia nell'indirizzo: Base64 senza + / =, e torna identico.
	const FString Testo = ValdorsoSicurezza::Base64PerIndirizzo(Chiusa);
	TestFalse(TEXT("senza + / ="), Testo.Contains(TEXT("+")) || Testo.Contains(TEXT("/")) || Testo.Contains(TEXT("=")));
	TArray<uint8> Riletta;
	TestTrue(TEXT("Base64 si rilegge"), ValdorsoSicurezza::DaBase64PerIndirizzo(Testo, Riletta));
	TestTrue(TEXT("Base64 torna uguale"), Riletta == Chiusa);
	return true;
}

// ------------------------------------------------------------------------------------------------
// Regole
// ------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestNomi, "Valdorso.Regole.Nomi", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestNomi::RunTest(const FString& Parameters)
{
	for (const TCHAR* Buono : { TEXT("Fra"), TEXT("Gaspare_il.Orso"), TEXT("bruno-99"), TEXT("Abcdefghijklmnopqrst") })
	{
		TestTrue(FString(TEXT("valido: ")) + Buono, ValdorsoRegole::NomeValido(Buono));
	}
	for (const TCHAR* Cattivo : { TEXT("Fr"), TEXT("Abcdefghijklmnopqrstu"), TEXT("1Fra"), TEXT("_Fra"), TEXT("Fra Gira"),
		TEXT("Frà"), TEXT("Fra!"), TEXT("") })
	{
		TestFalse(FString(TEXT("non valido: \"")) + Cattivo + TEXT("\""), ValdorsoRegole::NomeValido(Cattivo));
	}
	TestTrue(TEXT("riservato: admin"), ValdorsoRegole::NomeRiservato(TEXT("admin")));
	TestTrue(TEXT("riservato: valdorso"), ValdorsoRegole::NomeRiservato(TEXT("valdorso")));
	TestFalse(TEXT("non riservato: fra"), ValdorsoRegole::NomeRiservato(TEXT("fra")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestPassword, "Valdorso.Regole.Password", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestPassword::RunTest(const FString& Parameters)
{
	auto Va = [](const TCHAR* Pw, const TCHAR* Nome) { return ValdorsoRegole::ProblemaPassword(Pw, Nome).IsEmpty(); };

	TestTrue(TEXT("buona"), Va(TEXT("La valle mi accoglie 7"), TEXT("Fra")));
	TestTrue(TEXT("buona, esattamente 10"), Va(TEXT("Orso.Bruno"), TEXT("Fra")));
	TestFalse(TEXT("9 caratteri"), Va(TEXT("Orso.Brun"), TEXT("Fra")));
	TestFalse(TEXT("129 caratteri"), Va(*FString::ChrN(129, TEXT('a')).Replace(TEXT("aaaaa"), TEXT("abcde")), TEXT("Fra")));
	TestFalse(TEXT("contiene il nome, anche in maiuscolo"), Va(TEXT("IlMioNomeGASPARE7"), TEXT("Gaspare")));
	TestFalse(TEXT("comune"), Va(TEXT("Password123"), TEXT("Fra")));
	TestFalse(TEXT("comune: valdorso2026"), Va(TEXT("valdorso2026"), TEXT("Fra")));
	TestFalse(TEXT("troppo ripetuta"), Va(TEXT("abababababab"), TEXT("Fra")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestCodici, "Valdorso.Regole.CodiciInvito", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestCodici::RunTest(const FString& Parameters)
{
	// Il giocatore può scrivere il codice come vuole: con o senza VALD, trattini, spazi, minuscole.
	const FString Atteso = TEXT("7K3PABCDEFGH");
	TestEqual(TEXT("forma piena"), ValdorsoRegole::NormalizzaCodice(TEXT("VALD-7K3P-ABCD-EFGH")), Atteso);
	TestEqual(TEXT("minuscole e spazi"), ValdorsoRegole::NormalizzaCodice(TEXT(" vald 7k3p abcd efgh ")), Atteso);
	TestEqual(TEXT("senza VALD"), ValdorsoRegole::NormalizzaCodice(TEXT("7K3P-ABCD-EFGH")), Atteso);
	TestEqual(TEXT("un codice che comincia con VALD resta intero"), ValdorsoRegole::NormalizzaCodice(TEXT("VALDABCDEFGH")),
		FString(TEXT("VALDABCDEFGH")));
	TestEqual(TEXT("lunghezza del codice"), ValdorsoRegole::LunghezzaCodice, 12);

	// Scritto in modi diversi, l'impronta salvata sul server è la stessa.
	TestEqual(TEXT("stessa impronta"),
		ValdorsoSicurezza::Sha256Esadecimale(ValdorsoRegole::NormalizzaCodice(TEXT("vald-7k3p-abcd-efgh"))),
		ValdorsoSicurezza::Sha256Esadecimale(Atteso));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestBlocchi, "Valdorso.Regole.Blocchi", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestBlocchi::RunTest(const FString& Parameters)
{
	// 15 minuti, poi 30, 60... fino a 24 ore.
	TestEqual(TEXT("primo blocco: 15 minuti"), ValdorsoRegole::DurataBlocco(1), static_cast<int64>(15 * 60));
	TestEqual(TEXT("secondo: 30 minuti"), ValdorsoRegole::DurataBlocco(2), static_cast<int64>(30 * 60));
	TestEqual(TEXT("terzo: 1 ora"), ValdorsoRegole::DurataBlocco(3), static_cast<int64>(60 * 60));
	TestEqual(TEXT("settimo: 16 ore"), ValdorsoRegole::DurataBlocco(7), static_cast<int64>(16 * 60 * 60));
	TestEqual(TEXT("ottavo: 24 ore (il massimo)"), ValdorsoRegole::DurataBlocco(8), static_cast<int64>(24 * 60 * 60));
	TestEqual(TEXT("centesimo: sempre 24 ore"), ValdorsoRegole::DurataBlocco(100), static_cast<int64>(24 * 60 * 60));
	TestEqual(TEXT("zero: come il primo"), ValdorsoRegole::DurataBlocco(0), static_cast<int64>(15 * 60));
	TestEqual(TEXT("5 tentativi prima del blocco"), ValdorsoRegole::TentativiPrimaDelBlocco, 5);
	return true;
}

// ------------------------------------------------------------------------------------------------
// File dell'archivio
// ------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestFileArchivio, "Valdorso.Archivio.FileECopie", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestFileArchivio::RunTest(const FString& Parameters)
{
	IFileManager& File = IFileManager::Get();
	const FString Cartella = FPaths::ConvertRelativePathToFull(FPaths::AutomationTransientDir() / TEXT("ValdorsoArchivio") / FGuid::NewGuid().ToString());
	File.MakeDirectory(*Cartella, true);
	const FString Percorso = Cartella / TEXT("Inviti.json");

	auto Versione = [](int64 Numero)
	{
		FValdorsoArchivioInviti Dati;
		Dati.Versione = Numero;
		FString Testo;
		FJsonObjectConverter::UStructToJsonObjectString(Dati, Testo);
		return Testo;
	};
	auto Leggi = [&Percorso](int64& OutVersione)
	{
		FValdorsoArchivioInviti Dati;
		const bool bOk = ValdorsoArchivioFile::LeggiFile(Percorso, Dati);
		OutVersione = Dati.Versione;
		return bOk;
	};

	// Tre scritture: il file ha l'ultima, .bak1 la penultima, .bak2 la prima; nessun .tmp in giro.
	ValdorsoArchivioFile::ScriviAtomico(Percorso, Versione(1));
	TestFalse(TEXT("prima scrittura: ancora nessuna copia"), File.FileExists(*(Percorso + TEXT(".bak1"))));
	ValdorsoArchivioFile::ScriviAtomico(Percorso, Versione(2));
	ValdorsoArchivioFile::ScriviAtomico(Percorso, Versione(3));
	TestFalse(TEXT("niente .tmp rimasto"), File.FileExists(*(Percorso + TEXT(".tmp"))));

	int64 Letta = 0;
	TestTrue(TEXT("si legge"), Leggi(Letta));
	TestEqual(TEXT("il file ha l'ultima versione"), Letta, static_cast<int64>(3));

	// Il file si rovina (per esempio il PC si spegne a metà): si legge la copia .bak1.
	// Gli avvisi di Unreal sui file rovinati qui sono attesi (0 = quante volte capita).
	AddExpectedError(TEXT("Unable to parse"), EAutomationExpectedErrorFlags::Contains, 0);
	FFileHelper::SaveStringToFile(TEXT("{ rovinato"), *Percorso);
	TestTrue(TEXT("rovinato: si legge lo stesso"), Leggi(Letta));
	TestEqual(TEXT("rovinato: dalla copia .bak1"), Letta, static_cast<int64>(2));

	// Rovinata anche .bak1: si legge .bak2.
	FFileHelper::SaveStringToFile(TEXT(""), *(Percorso + TEXT(".bak1")));
	TestTrue(TEXT("rovinate due: si legge lo stesso"), Leggi(Letta));
	TestEqual(TEXT("rovinate due: dalla copia .bak2"), Letta, static_cast<int64>(1));

	// Rovinate tutte: si dice di no, senza inventare niente.
	FFileHelper::SaveStringToFile(TEXT("niente"), *(Percorso + TEXT(".bak2")));
	TestFalse(TEXT("rovinate tutte: niente"), Leggi(Letta));

	File.DeleteDirectory(*Cartella, false, true);
	return true;
}

// ------------------------------------------------------------------------------------------------
// Rientro senza password
// ------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValdorsoTestRientro, "Valdorso.Archivio.Rientro", ValdorsoTestAccount::Bandiere)
bool FValdorsoTestRientro::RunTest(const FString& Parameters)
{
	// Un archivista vuoto, solo in memoria (senza Initialize: non legge e non scrive file).
	// Un sottosistema deve stare dentro un'istanza del gioco: se ne crea una di prova.
	UGameInstance* IstanzaProva = NewObject<UGameInstance>(GetTransientPackage());
	FGCObjectScopeGuard Guardia(IstanzaProva);
	UValdorsoArchivista* Archivista = NewObject<UValdorsoArchivista>(IstanzaProva);

	const FString Primo = Archivista->CreaBiglietto(TEXT("account-di-prova"));
	TestEqual(TEXT("biglietto di 43 caratteri (32 byte)"), Primo.Len(), 43);
	TestFalse(TEXT("biglietto senza + / ="), Primo.Contains(TEXT("+")) || Primo.Contains(TEXT("/")) || Primo.Contains(TEXT("=")));
	const FString Secondo = Archivista->CreaBiglietto(TEXT("account-di-prova"));
	TestNotEqual(TEXT("ogni biglietto è diverso"), Primo, Secondo);
	TestTrue(TEXT("senza account non si crea"), Archivista->CreaBiglietto(FString()).IsEmpty());

	auto Prova = [Archivista](const FString& Biglietto)
	{
		EValdorsoEsitoAccount Risultato = EValdorsoEsitoAccount::Ok;
		Archivista->RientraConBiglietto(Biglietto, TEXT("127.0.0.1"), [&Risultato](const FValdorsoEsitoAccount& Esito) { Risultato = Esito.Esito; });
		return static_cast<int32>(Risultato);
	};
	const int32 NonValido = static_cast<int32>(EValdorsoEsitoAccount::RientroNonValido);

	// Il primo biglietto è stato sostituito dal secondo: non vale più.
	TestEqual(TEXT("il biglietto vecchio non vale"), Prova(Primo), NonValido);
	// Il secondo vale, ma l'account non esiste in questo archivio vuoto: rifiutato, e intanto è consumato.
	TestEqual(TEXT("account sconosciuto"), Prova(Secondo), NonValido);
	TestEqual(TEXT("un biglietto vale una volta"), Prova(Secondo), NonValido);
	TestEqual(TEXT("biglietto inventato"), Prova(TEXT("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ")), NonValido);
	TestEqual(TEXT("biglietto vuoto"), Prova(FString()), NonValido);

	// Annullato (bando, sospensione, password reimpostata): non vale.
	const FString Terzo = Archivista->CreaBiglietto(TEXT("altro-account"));
	Archivista->AnnullaBiglietto(TEXT("altro-account"));
	TestEqual(TEXT("biglietto annullato"), Prova(Terzo), NonValido);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
