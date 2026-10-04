// Valdorso - Il volo del primo ingresso (v0.1.2, passo 2.5). Scritto da Claude il 04/10/2026.
//
// La prima volta che si entra nella valle (con il codice d'invito) la telecamera non vola soltanto verso
// il frammento: passa tra i bracieri, sale sopra l'altare e il cerchio di rune e si fionda verso le montagne,
// con la luna davanti. Il percorso è questa curva (spline): in Lvl_Menu si spostano i suoi punti con il mouse
// e il volo cambia senza toccare il codice. Il primo punto non conta: in gioco diventa la posizione della
// telecamera del menu, così il volo parte sempre da dove si trova lo sguardo.
//
// Se nel livello la curva non c'è, il controllore del menu ne crea una al volo con i punti di serie
// (PuntiDiSerie), gli stessi che mette lo script Content/Python/volo_ingresso.py.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdorsoVoloIngresso.generated.h"

class USplineComponent;

UCLASS()
class VALDORSO_API AValdorsoVoloIngresso : public AActor
{
	GENERATED_BODY()

public:
	AValdorsoVoloIngresso();

	UPROPERTY(VisibleAnywhere, Category = "Valdorso|Volo")
	USplineComponent* Percorso;

	/** Quanto dura il volo, in secondi (la proposta approvata dice 6-8). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Volo", meta = (ClampMin = "3.0", ClampMax = "15.0"))
	float Durata = 7.f;

	/**
	 * Quanto parte piano: 1 = velocità costante da punto a punto; più alto = parte più piano e alla fine si fionda.
	 * Con 1,5 si arriva tra i bracieri dopo circa 2 secondi e sopra l'altare dopo poco più di 3.
	 */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Volo", meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float Partenza = 1.5f;

	/** Quanto avanti guarda la telecamera lungo la curva, in centimetri (cresce con la velocità). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Volo")
	float SguardoAvanti = 450.f;

	/** Quando comincia il nero, come frazione del volo (0,82 = negli ultimi 1,3 secondi su 7). */
	UPROPERTY(EditAnywhere, Category = "Valdorso|Volo", meta = (ClampMin = "0.3", ClampMax = "0.98"))
	float InizioBuio = 0.82f;

	/** I punti di serie, in coordinate del mondo di Lvl_Menu (l'altare è al centro, la luna verso +X). */
	static void PuntiDiSerie(TArray<FVector>& Out);

	/** Rimette i punti di serie nella curva (in coordinate del mondo). */
	void UsaPuntiDiSerie();

	/** In gioco: il primo punto diventa la posizione della telecamera. */
	void PartiDa(const FVector& Posizione);

	/** Posizione e sguardo della telecamera ad Avanzamento (da 0 a 1 nel tempo del volo). */
	void Posa(float Avanzamento, FVector& OutPosizione, FVector& OutBersaglio) const;
};
