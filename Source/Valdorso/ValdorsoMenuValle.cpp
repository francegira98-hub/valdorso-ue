// Valdorso - Il menu nella valle vera al tramonto (vedi il .h). Scritto da Claude il 08/11/2026.

#include "ValdorsoMenuValle.h"
#include "ValdorsoCielo.h"
#include "ValdorsoTempo.h"
#include "ValdorsoFasiVillaggio.h"
#include "ValdorsoRovine.h"
#include "ValdorsoAmbiente.h"
#include "Valdorso.h"
#include "Algo/BinarySearch.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarMenuValle(
		TEXT("Valdorso.Menu.Valle"),
		1,
		TEXT("1 = dietro il menu la valle vera al tramonto (se c'è /Game/Prove/L_Valle); 0 = la scena di sempre (Lvl_Menu)."));

	TAutoConsoleVariable<float> CVarMenuValleOra(
		TEXT("Valdorso.Menu.Ora"),
		18.f,
		TEXT("L'ora della valle dietro il menu (0-24). Alle 18 il sole tocca l'orizzonte (luce a metà, rune a metà); alle 17,5 più sole e rune spente, alle 18,15 sole spento. Vale alla prossima apertura del menu."));

	/** Un giorno lunghissimo: il cielo del menu resta fermo (in un'ora vera il sole si sposta di meno di un minuto). */
	constexpr float MinutiFermiMenuValle = 1000000.f;

	/** Il tempo del menu non cambia mai da solo (minuti veri). */
	constexpr float MinutiSerenoMenuValle = 100000.f;

	/** Cerca l'attore nel livello; se non c'è, lo fa nascere (come la modalità di gioco della valle). */
	template <typename T>
	T* TrovaOFaiNascereMenuValle(UWorld* Mondo)
	{
		TActorIterator<T> It(Mondo);
		if (It)
		{
			return *It;
		}
		FActorSpawnParameters Parametri;
		Parametri.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return Mondo->SpawnActor<T>(T::StaticClass(), FTransform::Identity, Parametri);
	}
}

namespace ValdorsoMenuValle
{
	FVector Piazza()
	{
		return FVector(15300.f, 42228.f, 5517.f);
	}

	const TArray<FValdorsoTappaMenu>& Tappe()
	{
		// Le tappe del giro (cm del mondo: X est, Y sud, Z quota). (08/11) Il Landscape di L_Valle ha l'angolo nord-ovest a
		// (-150000, -150000): le posizioni dei dati della valle qui sono già spostate. Il villaggio va da X 0 a 36000 e da
		// Y 22000 a 63000, il terreno lì è tra 5000 e 5900 cm; i tetti più alti (il campanile del tempio, le torri) stanno
		// 19-23 m sopra il terreno. Il giro è lungo circa 1420 m: in 90 secondi fa 16 m al secondo.
		// Lo sguardo gira intorno al villaggio, sempre 250 m o più davanti: al massimo 5 gradi al secondo.
		// Il sole al tramonto sta a sud (+Y): da nord si guarda il cielo acceso, da sud il villaggio illuminato.
		// Suolo: il terreno più alto (con 15 m intorno) dei due tratti vicini alla tappa, da Altezze_3000.png.
		static const TArray<FValdorsoTappaMenu> Elenco = {
			{ FVector(14700.f, 23800.f, 10800.f), FVector(15000.f, 49000.f, 5500.f), 5530.f },   // 0: nord, sopra la porta: guarda il tramonto oltre il tempio
			{ FVector(27900.f, 26300.f, 10800.f), FVector(10000.f, 51000.f, 5000.f), 5530.f },   // 1: nord-est: il villaggio e il lago dietro
			{ FVector(38500.f, 37400.f, 10200.f), FVector(8000.f, 46000.f, 5000.f), 5530.f },   // 2: est, sopra la caserma: verso il lago
			{ FVector(37700.f, 52600.f, 9800.f), FVector(11000.f, 40000.f, 5800.f), 5410.f },    // 3: sud-est: il villaggio col sole alle spalle
			{ FVector(25400.f, 65400.f, 9600.f), FVector(15000.f, 39000.f, 6000.f), 5640.f },    // 4: sud: il tempio e la collina dell'Orso
			{ FVector(9200.f, 66200.f, 9500.f), FVector(19000.f, 40000.f, 6000.f), 5860.f },    // 5: sud-ovest: i monti a est, verso la cascata
			{ FVector(-3600.f, 54400.f, 9200.f), FVector(22000.f, 42000.f, 6000.f), 5860.f },    // 6: ovest, sopra il mulino
			{ FVector(-6100.f, 39000.f, 9600.f), FVector(22000.f, 46000.f, 5800.f), 5860.f },    // 7: ovest-nordovest: il villaggio verso il tramonto
			{ FVector(1600.f, 25400.f, 10400.f), FVector(20000.f, 49000.f, 5500.f), 5540.f },   // 8: nord-ovest, poi di nuovo la prima
		};
		return Elenco;
	}

	FVector CatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
	{
		const float T2 = T * T;
		const float T3 = T2 * T;
		return 0.5f * ((2.f * P1)
			+ (P2 - P0) * T
			+ (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2
			+ (3.f * P1 - P0 - 3.f * P2 + P3) * T3);
	}

	FVector SuGiro(const TArray<FVector>& Punti, float Chiave)
	{
		const int32 N = Punti.Num();
		if (N == 0)
		{
			return FVector::ZeroVector;
		}
		if (N == 1)
		{
			return Punti[0];
		}
		float K = FMath::Fmod(Chiave, static_cast<float>(N));
		if (K < 0.f)
		{
			K += static_cast<float>(N);
		}
		const int32 I = FMath::Clamp(FMath::FloorToInt(K), 0, N - 1);
		const float T = FMath::Clamp(K - static_cast<float>(I), 0.f, 1.f);
		auto Punto = [&Punti, N](int32 Indice) -> const FVector&
		{
			return Punti[((Indice % N) + N) % N];
		};
		return CatmullRom(Punto(I - 1), Punto(I), Punto(I + 1), Punto(I + 2), T);
	}

	float Velocita(float Lunghezza, float Secondi, bool bMenoMovimenti)
	{
		const float Normale = FMath::Max(Lunghezza, 0.f) / FMath::Max(Secondi, 1.f);
		return bMenoMovimenti ? Normale * FattoreMenoMovimenti : Normale;
	}

	float Alzata(float QuotaTelecamera, float QuotaSotto, float Minimo)
	{
		return FMath::Max(0.f, Minimo - (QuotaTelecamera - QuotaSotto));
	}

	void VoloLungo(const FVector& Partenza, TArray<FVector>& Out)
	{
		// Dalla telecamera del giro verso la piazza, sopra i tetti (il campanile arriva a circa 7800 cm), poi oltre il
		// villaggio, dalla parte opposta, salendo sempre più ripido verso i monti. Controllato sulla mappa delle altezze
		// partendo da ogni tappa del giro: sempre almeno 33 m sopra il terreno. Il nero arriva a quattro quinti del volo.
		struct FPassoVolo
		{
			float Oltre;   // cm oltre la piazza (negativo: prima della piazza)
			float Quota;   // cm del mondo
		};
		static const FPassoVolo Passi[] = {
			{ -6000.f, 9800.f },
			{ 8000.f, 10400.f },
			{ 30000.f, 13000.f },
			{ 60000.f, 21500.f },
			{ 85000.f, 37000.f },
			{ 110000.f, 60000.f },
		};
		const FVector Centro = Piazza();
		FVector Verso = FVector(Centro.X - Partenza.X, Centro.Y - Partenza.Y, 0.f).GetSafeNormal();
		if (Verso.IsNearlyZero())
		{
			Verso = FVector(0.f, -1.f, 0.f);   // sopra la piazza: verso nord
		}
		Out.Reset();
		Out.Add(Partenza);
		for (const FPassoVolo& Passo : Passi)
		{
			Out.Add(FVector(Centro.X + Verso.X * Passo.Oltre, Centro.Y + Verso.Y * Passo.Oltre, Passo.Quota));
		}
	}

	bool Attiva()
	{
		return CVarMenuValle.GetValueOnGameThread() != 0;
	}

	float OraDelMenu()
	{
		const float Ora = FMath::Fmod(CVarMenuValleOra.GetValueOnGameThread(), 24.f);
		return Ora < 0.f ? Ora + 24.f : Ora;
	}

	FString OpzioniLivello()
	{
		return FString::Printf(TEXT("game=%s"), *AValdorsoMenuValleGameMode::StaticClass()->GetPathName());
	}
}

// --------------------------------------------------------------------------------------------- il giro della telecamera

void FValdorsoGiroMenu::Prepara(const TArray<FValdorsoTappaMenu>& Tappe, int32 PassiPerTratto)
{
	Posizioni.Reset();
	Sguardi.Reset();
	Distanze.Reset();
	Totale = 0.f;
	Passi = FMath::Max(PassiPerTratto, 1);
	for (const FValdorsoTappaMenu& Tappa : Tappe)
	{
		Posizioni.Add(Tappa.Posizione);
		Sguardi.Add(Tappa.Sguardo);
	}
	const int32 N = Posizioni.Num();
	if (N < 2)
	{
		return;
	}
	// La curva misurata a pezzetti: così la telecamera va a velocità costante anche dove le tappe sono più lontane.
	Distanze.Reserve(N * Passi + 1);
	Distanze.Add(0.f);
	FVector Prima = ValdorsoMenuValle::SuGiro(Posizioni, 0.f);
	for (int32 i = 1; i <= N * Passi; ++i)
	{
		const FVector Qui = ValdorsoMenuValle::SuGiro(Posizioni, static_cast<float>(i) / static_cast<float>(Passi));
		Totale += static_cast<float>(FVector::Dist(Prima, Qui));
		Distanze.Add(Totale);
		Prima = Qui;
	}
}

float FValdorsoGiroMenu::Chiave(float Distanza) const
{
	if (Totale <= KINDA_SMALL_NUMBER || Distanze.Num() < 2)
	{
		return 0.f;
	}
	float D = FMath::Fmod(Distanza, Totale);
	if (D < 0.f)
	{
		D += Totale;
	}
	// il pezzetto in cui cade D (Distanze cresce sempre)
	const int32 Dopo = Algo::UpperBound(Distanze, D);
	const int32 J = FMath::Clamp(Dopo - 1, 0, Distanze.Num() - 2);
	const float Lungo = Distanze[J + 1] - Distanze[J];
	const float Frazione = Lungo > KINDA_SMALL_NUMBER ? FMath::Clamp((D - Distanze[J]) / Lungo, 0.f, 1.f) : 0.f;
	return (static_cast<float>(J) + Frazione) / static_cast<float>(Passi);
}

void FValdorsoGiroMenu::Posa(float Distanza, FVector& OutPosizione, FVector& OutSguardo) const
{
	if (Posizioni.Num() == 0)
	{
		OutPosizione = FVector::ZeroVector;
		OutSguardo = FVector(100.f, 0.f, 0.f);
		return;
	}
	const float K = Chiave(Distanza);
	OutPosizione = ValdorsoMenuValle::SuGiro(Posizioni, K);
	OutSguardo = ValdorsoMenuValle::SuGiro(Sguardi, K);
	if (OutSguardo.Equals(OutPosizione, 1.f))
	{
		OutSguardo = OutPosizione + FVector(100.f, 0.f, 0.f);
	}
}

// --------------------------------------------------------------------------------------------- la modalità di gioco

AValdorsoMenuValleGameMode::AValdorsoMenuValleGameMode()
{
	// Il controllore del menu e nessun personaggio: li mette già la modalità del menu (AValdorsoMenuGameMode).
}

void AValdorsoMenuValleGameMode::StartPlay()
{
	Super::StartPlay();
	UWorld* Mondo = GetWorld();
	if (!Mondo)
	{
		return;
	}

	// Il cielo: fermo al tramonto (qui non c'è un server: decide questo PC). Prima la durata del giorno, poi l'ora.
	const float Ora = ValdorsoMenuValle::OraDelMenu();
	if (AValdorsoCielo* Cielo = TrovaOFaiNascereMenuValle<AValdorsoCielo>(Mondo))
	{
		Cielo->ImpostaMinutiPerGiorno(MinutiFermiMenuValle);
		Cielo->ImpostaOra(Ora);
	}

	// Il tempo: sereno, e non cambia da solo.
	AValdorsoTempo* Tempo = AValdorsoTempo::Di(Mondo);
	if (!Tempo)
	{
		Tempo = TrovaOFaiNascereMenuValle<AValdorsoTempo>(Mondo);
	}
	if (Tempo)
	{
		Tempo->MinutiMinimi = MinutiSerenoMenuValle;
		Tempo->MinutiMassimi = MinutiSerenoMenuValle;
		Tempo->SecondiCambio = 1.f;
		Tempo->ImpostaTempo(EValdorsoTempo::Sereno);
	}

	// Le fasi del villaggio (senza, si vedrebbero insieme le cose di tutte le fasi), le rune e i suoni della valle.
	TrovaOFaiNascereMenuValle<AValdorsoFasiVillaggio>(Mondo);
	TrovaOFaiNascereMenuValle<AValdorsoRuneNotte>(Mondo);
	TrovaOFaiNascereMenuValle<AValdorsoAmbiente>(Mondo);

	UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Menu nella valle: le %.2f, sereno"), Ora);
}
