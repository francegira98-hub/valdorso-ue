// Valdorso - Il volo del primo ingresso (vedi il .h).

#include "ValdorsoVoloIngresso.h"
#include "Components/SplineComponent.h"

AValdorsoVoloIngresso::AValdorsoVoloIngresso()
{
	PrimaryActorTick.bCanEverTick = false;

	Percorso = CreateDefaultSubobject<USplineComponent>(TEXT("Percorso"));
	RootComponent = Percorso;
	Percorso->SetMobility(EComponentMobility::Movable);
#if WITH_EDITORONLY_DATA
	// Nell'editor la curva è color oro, così si riconosce tra le altre.
	Percorso->SetUnselectedSplineSegmentColor(FLinearColor(0.85f, 0.69f, 0.22f));
#endif
	UsaPuntiDiSerie();

	// Non serve a nessuno fuori dal menu: niente rete, niente urti.
	bReplicates = false;
	SetActorEnableCollision(false);
}

void AValdorsoVoloIngresso::PuntiDiSerie(TArray<FVector>& Out)
{
	// L'altare è in (0, 0), il frammento a 90 cm d'altezza, i bracieri in (-200, ±260), la telecamera del menu
	// in (-980, -120, 210). La luna è bassa verso +X, appena a sinistra (5 gradi): i punti lontani la seguono.
	// Le montagne cominciano a 220 m; in quella direzione le creste vicine stanno sotto i 30 m.
	Out = {
		FVector(-980.f, -120.f, 210.f),      // 0: la telecamera del menu (in gioco, dove si trova davvero)
		FVector(-300.f, 0.f, 230.f),         // 1: tra i due bracieri
		FVector(150.f, 0.f, 520.f),          // 2: sopra l'altare e il cerchio di rune
		FVector(1800.f, 150.f, 1300.f),      // 3: oltre le pietre ritte, sale
		FVector(8000.f, 700.f, 3000.f),      // 4: sopra il bosco
		FVector(22000.f, 1900.f, 4600.f),    // 5: dove cominciano le montagne
		FVector(42000.f, 3700.f, 5200.f)     // 6: verso il passo, con la luna davanti
	};
}

void AValdorsoVoloIngresso::UsaPuntiDiSerie()
{
	TArray<FVector> Punti;
	PuntiDiSerie(Punti);
	Percorso->SetSplinePoints(Punti, ESplineCoordinateSpace::World, true);
}

void AValdorsoVoloIngresso::PartiDa(const FVector& Posizione)
{
	if (Percorso->GetNumberOfSplinePoints() > 0)
	{
		Percorso->SetLocationAtSplinePoint(0, Posizione, ESplineCoordinateSpace::World, true);
	}
}

void AValdorsoVoloIngresso::Posa(float Avanzamento, FVector& OutPosizione, FVector& OutBersaglio) const
{
	const int32 Punti = Percorso->GetNumberOfSplinePoints();
	if (Punti < 2)
	{
		OutPosizione = GetActorLocation();
		OutBersaglio = OutPosizione + GetActorForwardVector() * 100.f;
		return;
	}

	// Il tempo scorre uguale, la curva no: parte piano, poi ogni tratto dura uguale anche se è molto più lungo,
	// così più ci si allontana più si corre.
	const float Ultimo = static_cast<float>(Punti - 1);
	auto Chiave = [this, Ultimo](float A)
	{
		return Ultimo * FMath::Pow(FMath::Clamp(A, 0.f, 1.f), FMath::Max(Partenza, 1.f));
	};

	const float K = Chiave(Avanzamento);
	OutPosizione = Percorso->GetLocationAtSplineInputKey(K, ESplineCoordinateSpace::World);

	// Lo sguardo va avanti lungo la curva, più lontano quando si corre.
	const float Lunghezza = Percorso->GetSplineLength();
	const float Qui = Percorso->GetDistanceAlongSplineAtSplineInputKey(K);
	const float Passo = 0.01f;
	const float Dopo = Percorso->GetDistanceAlongSplineAtSplineInputKey(Chiave(Avanzamento + Passo));
	const float Velocita = (Dopo - Qui) / FMath::Max(Passo * Durata, KINDA_SMALL_NUMBER);   // cm al secondo
	const float Avanti = SguardoAvanti + Velocita * 0.35f;

	const float Dove = Qui + Avanti;
	if (Dove <= Lunghezza)
	{
		OutBersaglio = Percorso->GetLocationAtDistanceAlongSpline(Dove, ESplineCoordinateSpace::World);
	}
	else
	{
		// Oltre la fine si guarda dritto, nella direzione dell'ultimo tratto.
		const FVector Fine = Percorso->GetLocationAtDistanceAlongSpline(Lunghezza, ESplineCoordinateSpace::World);
		const FVector Verso = Percorso->GetDirectionAtDistanceAlongSpline(Lunghezza, ESplineCoordinateSpace::World);
		OutBersaglio = Fine + Verso * (Dove - Lunghezza);
	}
	if (OutBersaglio.Equals(OutPosizione, 1.f))
	{
		OutBersaglio = OutPosizione + Percorso->GetDirectionAtSplineInputKey(K, ESplineCoordinateSpace::World) * 100.f;
	}
}
